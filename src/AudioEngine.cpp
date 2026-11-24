#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <windows.h>
#include <RE/U/UI.h>

#include "AudioEngine.h"
#include "MenuUI.h"

#include <filesystem>
#include <random>
#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

namespace DynamicBossMusic::Audio {

    static bool IsSupportedAudioExtension(const fs::path& ext) {
        std::string e = ext.string();
        std::transform(e.begin(), e.end(), e.begin(), ::tolower);
        return (e == ".mp3" || e == ".wav" || e == ".ogg" || e == ".flac");
    }

    bool AudioEngine::Initialize() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_engine != nullptr) {
                return true;
            }
        }

        _engine = new ma_engine();
        ma_result result = ma_engine_init(nullptr, _engine);
        if (result != MA_SUCCESS) {
            delete _engine;
            _engine = nullptr;
            return false;
        }

        _gameWindow = GetForegroundWindow();

        _running = true;
        _workerThread = std::thread(&AudioEngine::FadeWorkerLoop, this);

        ScanDirectories();
        return true;
    }

    void AudioEngine::Shutdown() {
        _running = false;
        if (_workerThread.joinable()) {
            _workerThread.join();
        }

        std::lock_guard<std::mutex> lock(_mutex);
        StopImmediateInternal();

        if (_engine != nullptr) {
            ma_engine_uninit(_engine);
            delete _engine;
            _engine = nullptr;
        }
    }

    void AudioEngine::ScanDirectories() {
        // Build locals outside lock
        std::unordered_map<std::string, std::vector<std::string>> newPaths;
        std::unordered_map<std::string, std::vector<std::string>> newTracks;
        std::vector<std::string> newCategories = UI::Categories;

        auto isAmbientCat = [](const std::string& c) {
            return c == "Inns" || c == "Cities" || c == "Shops" || c == "Dungeons";
        };

        // New structure
        for (const auto& group : { std::string("Combat"), std::string("Ambient") }) {
            fs::path groupDir = fs::path(_basePath) / group;
            std::error_code ec;
            fs::create_directories(groupDir, ec);

            for (const auto& cat : UI::Categories) {
                bool ambient = isAmbientCat(cat);
                if (ambient != (group == "Ambient")) continue;
                fs::create_directories(groupDir / cat, ec);
            }
        }

        std::error_code ec;
        fs::create_directories(_legacyBasePath, ec);

        auto scanDir = [&](const fs::path& baseDir) {
            if (!fs::exists(baseDir) || !fs::is_directory(baseDir)) return;
            for (const auto& entry : fs::directory_iterator(baseDir)) {
                if (!entry.is_directory()) continue;
                std::string name = entry.path().filename().string();

                if (name == "Combat" || name == "Ambient") {
                    for (const auto& sub : fs::directory_iterator(entry.path())) {
                        if (!sub.is_directory()) continue;
                        std::string subName = sub.path().filename().string();
                        if (std::find(newCategories.begin(), newCategories.end(), subName) == newCategories.end()) {
                            newCategories.push_back(subName);
                        }
                        std::vector<std::string> fps, fns;
                        for (const auto& fe : fs::directory_iterator(sub.path())) {
                            if (fe.is_regular_file() && IsSupportedAudioExtension(fe.path().extension())) {
                                fps.push_back(fe.path().string());
                                fns.push_back(fe.path().filename().string());
                            }
                        }
                        std::sort(fps.begin(), fps.end());
                        std::sort(fns.begin(), fns.end());
                        newPaths[subName] = std::move(fps);
                        newTracks[subName] = std::move(fns);
                    }
                    continue;
                }

                if (std::find(newCategories.begin(), newCategories.end(), name) == newCategories.end()) {
                    newCategories.push_back(name);
                }
                std::vector<std::string> fps, fns;
                for (const auto& fe : fs::directory_iterator(entry.path())) {
                    if (fe.is_regular_file() && IsSupportedAudioExtension(fe.path().extension())) {
                        fps.push_back(fe.path().string());
                        fns.push_back(fe.path().filename().string());
                    }
                }
                std::sort(fps.begin(), fps.end());
                std::sort(fns.begin(), fns.end());
                newPaths[name] = std::move(fps);
                newTracks[name] = std::move(fns);
            }
        };

        scanDir(_basePath);
        scanDir(_legacyBasePath);

        // Swap under lock
        std::lock_guard<std::mutex> lock(_mutex);
        _categoryFilePaths.swap(newPaths);
        UI::CategoryTracks.swap(newTracks);
        UI::Categories.swap(newCategories);
    }
    void AudioEngine::PlayRandomFromCategory(std::string category, float fadeInSec) {
        std::string chosenPath;
        std::string chosenName;
        bool noTracks = false;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _categoryFilePaths.find(category);
            if (it == _categoryFilePaths.end() || it->second.empty()) {
                // Seçilen kategori boşsa GeneralBoss'a düş
                it = _categoryFilePaths.find("GeneralBoss");
                if (it == _categoryFilePaths.end() || it->second.empty()) {
                    return;
                }
            }

            // Filter disabled tracks
            std::vector<std::string> enabled;
            for (const auto& p : it->second) {
                std::string key = category + "|" + fs::path(p).filename().string();
                if (UI::DisabledTracks.count(key) == 0) enabled.push_back(p);
            }
            if (enabled.empty() && category != "GeneralBoss") {
                auto git = _categoryFilePaths.find("GeneralBoss");
                if (git != _categoryFilePaths.end() && !git->second.empty()) {
                    std::vector<std::string> genEnabled;
                    for (const auto& p : git->second) {
                        std::string key = std::string("GeneralBoss") + "|" + fs::path(p).filename().string();
                        if (UI::DisabledTracks.count(key) == 0) genEnabled.push_back(p);
                    }
                    if (!genEnabled.empty()) {
                        enabled = genEnabled;
                        category = "GeneralBoss";
                    }
                }
            }
            noTracks = enabled.empty();

            if (!noTracks) {
                const auto& list = enabled;
                if (list.size() == 1 || !UI::Settings.randomizeTracks) {
                    chosenPath = list[0];
                } else {
                    // Aynı şarkının üst üste iki kez çalmasını engelle
                    static std::mt19937 rng(std::random_device{}());
                    std::uniform_int_distribution<size_t> dist(0, list.size() - 1);
                    for (int attempt = 0; attempt < 5; ++attempt) {
                        chosenPath = list[dist(rng)];
                        if (chosenPath != _lastPlayedTrack) {
                            break;
                        }
                    }
                }
                chosenName = fs::path(chosenPath).filename().string();
            }
        }

        if (noTracks) {
            Stop(UI::Settings.fadeDuration);
            return;
        }

        // Crossfade: if currently playing, queue pending and fade out
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_isPlaying && _activeSound != nullptr) {
                _pendingPath = chosenPath;
                _pendingName = chosenName;
                _pendingFadeIn = fadeInSec;
                _hasPending = true;

                _isFadingOut = true;
                float ticks = (UI::Settings.combatAmbientFadeDuration * 1000.0f) / 25.0f;
                _fadeStepPerTick = std::max(0.005f, _currentFadeVolume.load() / std::max(1.0f, ticks));
                return;
            }
        }

        PlayFile(chosenPath, chosenName, fadeInSec);
    }

    void AudioEngine::PlayFile(const std::string& fullPath, const std::string& displayName, float fadeInSec) {
        std::lock_guard<std::mutex> lock(_mutex);
        _hasPending = false;
        _paused = false;
        _resumeRequested = false;
        if (_engine == nullptr || !fs::exists(fullPath)) {
            return;
        }

        StopImmediateInternal();

        _activeSound = new ma_sound();
        ma_uint32 flags = MA_SOUND_FLAG_STREAM; // Büyük müzik dosyalarını RAM'i şişirmeden stream eder
        ma_result res = ma_sound_init_from_file(_engine, fullPath.c_str(), flags, nullptr, nullptr, _activeSound);
        if (res != MA_SUCCESS) {
            delete _activeSound;
            _activeSound = nullptr;
            return;
        }

        ma_sound_set_looping(_activeSound, MA_TRUE);

        _targetVolume = UI::Settings.masterVolume;
        _isFadingOut = false;

        if (fadeInSec <= 0.05f) {
            _currentFadeVolume = _targetVolume.load();
            _fadeStepPerTick = 1.0f;
        } else {
            _currentFadeVolume = 0.0f;
            float ticks = (fadeInSec * 1000.0f) / 25.0f; // Her 25ms'de bir adım
            _fadeStepPerTick = (_targetVolume.load() / std::max(1.0f, ticks));
        }

        ma_sound_set_volume(_activeSound, _currentFadeVolume.load());
        ma_sound_start(_activeSound);

        _isPlaying = true;
        _currentTrackName = displayName;
        _lastPlayedTrack = fullPath;
        _paused = false;
        _resumeRequested = false;
        UI::CurrentlyPlayingTrack = displayName;
    }

    void AudioEngine::Stop(float fadeOutSec) {
        std::lock_guard<std::mutex> lock(_mutex);
        _hasPending = false;
        _paused = false;
        _resumeRequested = false;
        if (!_isPlaying || _activeSound == nullptr) {
            return;
        }

        if (fadeOutSec <= 0.05f) {
            StopImmediateInternal();
            return;
        }

        _isFadingOut = true;
        float ticks = (fadeOutSec * 1000.0f) / 25.0f;
        _fadeStepPerTick = std::max(0.005f, _currentFadeVolume.load() / std::max(1.0f, ticks));
    }

    void AudioEngine::SetMasterVolume(float volume) {
        _targetVolume = std::clamp(volume, 0.0f, 1.0f);
        std::lock_guard<std::mutex> lock(_mutex);
        if (_activeSound != nullptr && !_isFadingOut) {
            _currentFadeVolume = _targetVolume.load();
            ma_sound_set_volume(_activeSound, _currentFadeVolume.load());
        }
    }

    void AudioEngine::SetPaused(bool paused, float customFadeSec) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_activeSound == nullptr || !_isPlaying) {
            return;
        }
        float fadeSec = (customFadeSec >= 0.0f) ? customFadeSec : UI::Settings.fadeDuration;
        _pauseFadeSec = fadeSec;
        if (paused) {
            if (_paused) return;
            _paused = true;
            _isFadingOut = true;
            float ticks = (fadeSec * 1000.0f) / 25.0f;
            _fadeStepPerTick = std::max(0.005f, _currentFadeVolume.load() / std::max(1.0f, ticks));
        } else {
            if (!_paused) return;
            _paused = false;
            _resumeRequested = true;
        }
    }

    void AudioEngine::SetDialogueOpen(bool open) {
        _isDialogueOpen = open;
    }

    bool AudioEngine::IsPlaying() const {
        return _isPlaying.load();
    }

    std::string AudioEngine::GetCurrentTrackName() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _currentTrackName;
    }

    std::string AudioEngine::GetTrackFullPath(const std::string& category, const std::string& trackName) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _categoryFilePaths.find(category);
        if (it == _categoryFilePaths.end()) return "";
        for (const auto& p : it->second) {
            if (fs::path(p).filename().string() == trackName) return p;
        }
        return "";
    }

    void AudioEngine::StartSoundLocked(const std::string& fullPath, const std::string& displayName, float fadeInSec) {
        _activeSound = new ma_sound();
        ma_uint32 flags = MA_SOUND_FLAG_STREAM;
        ma_result res = ma_sound_init_from_file(_engine, fullPath.c_str(), flags, nullptr, nullptr, _activeSound);
        if (res != MA_SUCCESS) {
            delete _activeSound;
            _activeSound = nullptr;
            return;
        }

        ma_sound_set_looping(_activeSound, MA_TRUE);

        _targetVolume = UI::Settings.masterVolume;
        _isFadingOut = false;

        if (fadeInSec <= 0.05f) {
            _currentFadeVolume = _targetVolume.load();
            _fadeStepPerTick = 1.0f;
        } else {
            _currentFadeVolume = 0.0f;
            float ticks = (fadeInSec * 1000.0f) / 25.0f;
            _fadeStepPerTick = (_targetVolume.load() / std::max(1.0f, ticks));
        }

        ma_sound_set_volume(_activeSound, _currentFadeVolume.load());
        ma_sound_start(_activeSound);

        _isPlaying = true;
        _currentTrackName = displayName;
        _lastPlayedTrack = fullPath;
        _paused = false;
        _resumeRequested = false;
        UI::CurrentlyPlayingTrack = displayName;
    }

    void AudioEngine::StopImmediateInternal() {
        if (_activeSound != nullptr) {
            ma_sound_stop(_activeSound);
            ma_sound_uninit(_activeSound);
            delete _activeSound;
            _activeSound = nullptr;
        }
        _isPlaying = false;
        _isFadingOut = false;
        _currentFadeVolume = 0.0f;
        _currentTrackName = "None";
        UI::CurrentlyPlayingTrack = "None";

        if (_hasPending) {
            std::string p = _pendingPath;
            std::string n = _pendingName;
            float f = _pendingFadeIn;
            _hasPending = false;
            if (_engine != nullptr) {
                StartSoundLocked(p, n, f);
            }
        }
    }

    void AudioEngine::FadeWorkerLoop() {
        while (_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));

            std::lock_guard<std::mutex> lock(_mutex);
            if (!_isPlaying || _activeSound == nullptr) {
                continue;
            }

            // Alt-tab / focus handling: pause when game is not foreground
            DWORD fgPid = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &fgPid);
            bool foreground = (fgPid == GetCurrentProcessId());
            if (!foreground && !_focusPaused) {
                ma_sound_stop(_activeSound);
                _focusPaused = true;
            } else if (foreground && _focusPaused) {
                if (!_paused) ma_sound_start(_activeSound);
                _focusPaused = false;
            }
            if (_focusPaused) {
                continue;
            }

            // Resume from pause fade
            if (_resumeRequested) {
                _resumeRequested = false;
                _isFadingOut = false;
                ma_sound_start(_activeSound);
                _currentFadeVolume = 0.0f;
                float ticks = (_pauseFadeSec * 1000.0f) / 25.0f;
                _fadeStepPerTick = (UI::Settings.masterVolume / std::max(1.0f, ticks));
            }

            // Dialogue ducking state
            _dialogueDucked = _isDialogueOpen.load();

            float cur = _currentFadeVolume.load();
            float step = _fadeStepPerTick.load();

            if (_isFadingOut) {
                cur -= step;
                if (cur <= 0.001f) {
                    if (_paused) {
                        ma_sound_stop(_activeSound);
                        _currentFadeVolume = 0.0f;
                        _isFadingOut = false;
                    } else {
                        StopImmediateInternal();
                    }
                } else {
                    _currentFadeVolume = cur;
                    ma_sound_set_volume(_activeSound, cur);
                }
            } else {
                float master = UI::Settings.masterVolume;
                float desired = _dialogueDucked ? (master * 0.3f) : master;
                float delta = 0.02f;
                if (cur < desired - 0.001f) {
                    cur = std::min(desired, cur + delta);
                    _currentFadeVolume = cur;
                    ma_sound_set_volume(_activeSound, cur);
                } else if (cur > desired + 0.001f) {
                    cur = std::max(desired, cur - delta);
                    _currentFadeVolume = cur;
                    ma_sound_set_volume(_activeSound, cur);
                }
            }
        }
    }
}