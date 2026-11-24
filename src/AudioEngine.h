#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>

struct ma_engine;
struct ma_sound;

namespace DynamicBossMusic::Audio {

    class AudioEngine {
    public:
        static AudioEngine& GetSingleton() {
            static AudioEngine instance;
            return instance;
        }

        bool Initialize();
        void Shutdown();

        // Data\Music\DynamicBoss altındaki klasörleri oluşturur ve tarar
        void ScanDirectories();

        // Oynatma kontrolleri
        void PlayRandomFromCategory(std::string category, float fadeInSec = 2.0f);
        void PlayFile(const std::string& fullPath, const std::string& displayName, float fadeInSec = 2.0f);
        void Stop(float fadeOutSec = 2.0f);

        // Oyun içi durum güncellemeleri
        void SetMasterVolume(float volume);
        void SetPaused(bool paused, float customFadeSec = -1.0f);
        void SetDialogueOpen(bool open);
        bool IsPlaying() const;
        std::string GetCurrentTrackName() const;
        std::string GetTrackFullPath(const std::string& category, const std::string& trackName) const;

    private:
        AudioEngine() = default;
        ~AudioEngine() { Shutdown(); }

        AudioEngine(const AudioEngine&) = delete;
        AudioEngine& operator=(const AudioEngine&) = delete;

        void FadeWorkerLoop();
        void StopImmediateInternal();
        void StartSoundLocked(const std::string& fullPath, const std::string& displayName, float fadeInSec);

        ma_engine* _engine = nullptr;
        ma_sound* _activeSound = nullptr;

        mutable std::mutex _mutex;
        std::string _basePath = "Data\\Music\\DynamicMusic";
        std::string _legacyBasePath = "Data\\Music\\DynamicBoss";
        std::string _currentTrackName = "None";
        std::string _lastPlayedTrack;

        // Crossfade / pending play
        bool _hasPending = false;
        std::string _pendingPath;
        std::string _pendingName;
        float _pendingFadeIn = 2.0f;

        // Focus handling
        void* _gameWindow = nullptr;
        bool _focusPaused = false;

        // Pause fade handling
        bool _paused = false;
        bool _resumeRequested = false;
        float _pauseFadeSec = 2.0f;

        // Dialogue ducking
        std::atomic<bool> _isDialogueOpen{ false };
        bool _dialogueDucked = false;

        // Kategorilere göre tam dosya yolları
        std::unordered_map<std::string, std::vector<std::string>> _categoryFilePaths;

        // Fade ve arka plan kontrolü
        std::atomic<bool> _running{ false };
        std::atomic<bool> _isPlaying{ false };
        std::atomic<bool> _isFadingOut{ false };
        std::atomic<float> _targetVolume{ 0.80f };
        std::atomic<float> _currentFadeVolume{ 0.0f };
        std::atomic<float> _fadeStepPerTick{ 0.02f };

        std::thread _workerThread;
    };
}