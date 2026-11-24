#include "AmbientDetector.h"
#include "AudioEngine.h"
#include "MusicDetector.h"
#include "MenuUI.h"

#include <chrono>

namespace DynamicBossMusic::Ambient {

    static std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    void AmbientDetector::Register() {
        if (_running) return;
        _running = true;
        _thread = std::thread(&AmbientDetector::PollLoop, this);
    }

    void AmbientDetector::Shutdown() {
        _running = false;
        if (_thread.joinable()) _thread.join();
    }

    void AmbientDetector::StopCurrent() {
        _currentCategory = "None";
        _ambientPlaying = false;
        _lastLocationFormID = 0;
        _lastCellFormID = 0;
        Audio::AudioEngine::GetSingleton().Stop(UI::Settings.combatAmbientFadeDuration);
    }

    void AmbientDetector::ResetState() {
        _lastCellFormID = 0;
        _lastLocationFormID = 0;
        _currentCategory = "None";
        _ambientPlaying = false;
    }

    void AmbientDetector::HandleExternalMusicCompatibility() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        // During combat/boss, don't interfere
        if (DynamicBossMusic::Combat::MusicDetector::GetSingleton()->IsBossActive()) return;
        if (player->IsInCombat()) return;

        // Special scene menus
        bool specialMenuOpen = false;
        if (auto* ui = RE::UI::GetSingleton()) {
            specialMenuOpen = ui->IsMenuOpen("OStim") || ui->IsMenuOpen("Crafting") || ui->IsMenuOpen("Lockpicking") || ui->IsMenuOpen("Map Menu");
        }
        if (specialMenuOpen) {
            if (_ambientPlaying) StopCurrent();
            return;
        }

        bool highPriorityExternal = false;
        if (auto* dh = RE::TESDataHandler::GetSingleton()) {
            for (auto* mt : dh->GetFormArray<RE::BGSMusicType>()) {
                if (!mt) continue;
                if (mt->typeStatus != RE::BSIMusicType::MUSIC_STATUS::kPlaying) continue;

                const char* edid = mt->GetFormEditorID();
                std::string id = edid ? edid : "";

                // Skip combat/boss music types entirely
                if (id.find("Combat") != std::string::npos || id.find("Boss") != std::string::npos) {
                    if (DynamicBossMusic::Combat::MusicDetector::GetSingleton()->IsBossActive()) {
                        mt->DoFinish(true);
                    }
                    continue;
                }

                bool isHighPriority = (mt->priority <= 20) ||
                    (id.find("Bard") != std::string::npos) ||
                    (id.find("OStim") != std::string::npos) ||
                    (id.find("Quest") != std::string::npos) ||
                    (id.find("LevelUp") != std::string::npos);

                if (isHighPriority) {
                    highPriorityExternal = true;
                    if (_ambientPlaying) {
                        Audio::AudioEngine::GetSingleton().SetPaused(true, 0.2f);
                    }
                } else {
                    // Low-priority vanilla/overhaul ambient music: suppress if our ambient is playing
                    if (_ambientPlaying && Audio::AudioEngine::GetSingleton().IsPlaying()) {
                        mt->DoFinish(true);
                    }
                }
            }
        }

        // If no high-priority external music, resume our ambient if paused
        if (!highPriorityExternal && _ambientPlaying && !Audio::AudioEngine::GetSingleton().IsPlaying()) {
            Audio::AudioEngine::GetSingleton().SetPaused(false, 0.5f);
        }
    }

    void AmbientDetector::OnBossStart() {
        _lastCellFormID = 0;
        _lastLocationFormID = 0;
        _currentCategory = "None";
        _ambientPlaying = false;
    }

    std::string AmbientDetector::ResolveCategory() const {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return "";

        auto* cell = player->GetParentCell();
        if (!cell) return "";

        if (auto* loc = player->GetCurrentLocation()) {
            bool interior = cell->IsInteriorCell();

            if (loc->HasKeywordString("LocTypeInn") || loc->HasKeywordString("LocTypeTavern")) return "Inns";
            if (loc->HasKeywordString("LocTypeShop") || loc->HasKeywordString("LocTypeMerchant")) return "Shops";
            if (loc->HasKeywordString("LocTypeDungeon")) return "Dungeons";
            if (loc->HasKeywordString("LocTypeCity")) {
                if (!interior) {
                    auto* ws = cell->GetRuntimeData().worldSpace;
                    if (ws) {
                        const char* edid = ws->GetFormEditorID();
                        std::string wsName = edid ? edid : "";
                        if (wsName == "Tamriel") {
                            // Sur disi kirsal alan: sehirl muzigini sonlandir
                            return "";
                        }
                    }
                    return "Cities";
                }
                return "";
            }
            if (loc->HasKeywordString("LocTypeTown")) {
                if (!interior) return "Cities";
                return "";
            }
            return "";
        }

        if (!cell->IsInteriorCell()) return "";

        const char* cellName = cell->GetName();
        std::string name = cellName ? cellName : "";
        std::string lower = ToLower(name);
        if (lower.find("inn") != std::string::npos || lower.find("tavern") != std::string::npos || lower.find("mare") != std::string::npos) return "Inns";
        if (lower.find("shop") != std::string::npos || lower.find("store") != std::string::npos) return "Shops";

        return "";
    }

    void AmbientDetector::CheckNow() {
        DynamicBossMusic::Combat::MusicDetector::GetSingleton()->ValidateActiveBosses();

        if (!UI::Settings.enabled) return;

        if (UI::Settings.enableAmbient != _wasEnabled) {
            _wasEnabled = UI::Settings.enableAmbient;
            _lastLocationFormID = 0;
            _lastCellFormID = 0;
            if (!UI::Settings.enableAmbient) {
                StopCurrent();
                return;
            }
        }
        if (!UI::Settings.enableAmbient) return;

        if (DynamicBossMusic::Combat::MusicDetector::GetSingleton()->IsBossActive()) {
            _ambientPlaying = false;
            _currentCategory = "None";
            _lastLocationFormID = 0;
            return;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        if (auto* ui = RE::UI::GetSingleton()) {
            if (ui->IsMenuOpen("Loading Menu") || ui->IsMenuOpen("Main Menu")) {
                _lastLocationFormID = 0;
                return;
            }
        }

        if (player->IsInCombat()) {
            if (DynamicBossMusic::Combat::MusicDetector::GetSingleton()->IsBossActive()) {
                _ambientPlaying = false;
                _currentCategory = "None";
                _lastLocationFormID = 0;
                return;
            }
            if (_ambientPlaying) {
                StopCurrent();
            }
            _lastLocationFormID = 0;
            _lastCellFormID = 0;
            _ambientPlaying = false;
            _currentCategory = "None";
            return;
        }

        auto* cell = player->GetParentCell();
        if (!cell) return;

        auto* loc = player->GetCurrentLocation();
        RE::FormID locId = loc ? loc->GetFormID() : cell->GetFormID();
        if (locId == _lastLocationFormID && cell->GetFormID() == _lastCellFormID && _ambientPlaying && Audio::AudioEngine::GetSingleton().IsPlaying()) {
            return;
        }
        _lastLocationFormID = locId;
        _lastCellFormID = cell->GetFormID();

        std::string category = ResolveCategory();
        if (category.empty()) {
            Audio::AudioEngine::GetSingleton().Stop(UI::Settings.combatAmbientFadeDuration);
            _currentCategory = "None";
            _ambientPlaying = false;
            return;
        }

        bool hasEnabled = false;
        if (UI::CategoryTracks.count(category) > 0) {
            for (const auto& t : UI::CategoryTracks[category]) {
                std::string key = category + "|" + t;
                if (UI::DisabledTracks.count(key) == 0) { hasEnabled = true; break; }
            }
        }
        if (!hasEnabled) {
            if (Audio::AudioEngine::GetSingleton().IsPlaying()) {
                Audio::AudioEngine::GetSingleton().Stop(UI::Settings.combatAmbientFadeDuration);
            }
            _currentCategory = "None";
            _ambientPlaying = false;
            return;
        }

        if (category == _currentCategory && _ambientPlaying && Audio::AudioEngine::GetSingleton().IsPlaying()) {
            return;
        }

        _currentCategory = category;
        _ambientPlaying = true;
        Audio::AudioEngine::GetSingleton().PlayRandomFromCategory(category, UI::Settings.combatAmbientFadeDuration);

        HandleExternalMusicCompatibility();
    }

    void AmbientDetector::PollLoop() {
        while (_running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (!UI::Settings.enabled) continue;

            auto* task = SKSE::GetTaskInterface();
            if (!task) continue;
            task->AddTask([this]() { CheckNow(); });
        }
    }

    void AmbientDetector::NotifySettingChanged() {
        _wasEnabled = UI::Settings.enableAmbient;
        _lastCellFormID = 0;
        _lastLocationFormID = 0;
        _currentCategory = "None";
        if (!UI::Settings.enableAmbient) {
            StopCurrent();
        } else {
            auto* task = SKSE::GetTaskInterface();
            if (task) task->AddTask([this]() { CheckNow(); });
        }
    }

}