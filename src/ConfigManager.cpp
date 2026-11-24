#include "ConfigManager.h"
#include "MenuUI.h"
#include "AudioEngine.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace DynamicBossMusic::Config {

    static const fs::path kConfigPath = "Data\\SKSE\\Plugins\\DynamicBossMusic.json";

    void Load() {
        if (!fs::exists(kConfigPath)) {
            Save(); // İlk çalıştırmada varsayılan JSON dosyasını oluştur
            return;
        }

        try {
            std::ifstream file(kConfigPath);
            if (!file.is_open()) return;

            json j;
            file >> j;

            if (j.contains("enabled")) UI::Settings.enabled = j["enabled"].get<bool>();
            if (j.contains("enableCombatMusic")) UI::Settings.enableCombatMusic = j["enableCombatMusic"].get<bool>();
            if (j.contains("masterVolume")) UI::Settings.masterVolume = j["masterVolume"].get<float>();
            if (j.contains("fadeDuration")) UI::Settings.fadeDuration = j["fadeDuration"].get<float>();
            if (j.contains("combatAmbientFadeDuration")) UI::Settings.combatAmbientFadeDuration = j["combatAmbientFadeDuration"].get<float>();
            if (j.contains("muteVanillaCombat")) UI::Settings.muteVanillaCombat = j["muteVanillaCombat"].get<bool>();
            if (j.contains("enablePhase2")) UI::Settings.enablePhase2 = j["enablePhase2"].get<bool>();
            if (j.contains("phase2HealthThreshold")) UI::Settings.phase2HealthThreshold = j["phase2HealthThreshold"].get<float>();
            if (j.contains("randomizeTracks")) UI::Settings.randomizeTracks = j["randomizeTracks"].get<bool>();
            if (j.contains("enableAmbient")) UI::Settings.enableAmbient = j["enableAmbient"].get<bool>();

            if (j.contains("overrides") && j["overrides"].is_array()) {
                UI::Overrides.clear();
                for (const auto& item : j["overrides"]) {
                    UI::BossOverride ov;
                    ov.bossName = item.value("bossName", "");
                    ov.assignedCategory = item.value("assignedCategory", "GeneralBoss");
                    ov.assignedTrack = item.value("assignedTrack", "");
                    if (!ov.bossName.empty()) {
                        UI::Overrides.push_back(std::move(ov));
                    }
                }
            }

            if (j.contains("disabledTracks") && j["disabledTracks"].is_array()) {
                UI::DisabledTracks.clear();
                for (const auto& t : j["disabledTracks"]) {
                    UI::DisabledTracks.insert(t.get<std::string>());
                }
            }

            Audio::AudioEngine::GetSingleton().SetMasterVolume(UI::Settings.masterVolume);
        } catch (...) {
            // Bozuk JSON durumunda varsayılan ayarlarla devam et
        }
    }

    void Save() {
        try {
            std::error_code ec;
            fs::create_directories(kConfigPath.parent_path(), ec);

            json j;
            j["enabled"] = UI::Settings.enabled;
            j["enableCombatMusic"] = UI::Settings.enableCombatMusic;
            j["masterVolume"] = UI::Settings.masterVolume;
            j["fadeDuration"] = UI::Settings.fadeDuration;
            j["combatAmbientFadeDuration"] = UI::Settings.combatAmbientFadeDuration;
            j["muteVanillaCombat"] = UI::Settings.muteVanillaCombat;
            j["enablePhase2"] = UI::Settings.enablePhase2;
            j["phase2HealthThreshold"] = UI::Settings.phase2HealthThreshold;
            j["randomizeTracks"] = UI::Settings.randomizeTracks;
            j["enableAmbient"] = UI::Settings.enableAmbient;

            json overridesArr = json::array();
            for (const auto& ov : UI::Overrides) {
                overridesArr.push_back({
                    { "bossName", ov.bossName },
                    { "assignedCategory", ov.assignedCategory },
                    { "assignedTrack", ov.assignedTrack }
                });
            }
            j["overrides"] = overridesArr;

            json disabledArr = json::array();
            for (const auto& t : UI::DisabledTracks) disabledArr.push_back(t);
            j["disabledTracks"] = disabledArr;

            std::ofstream file(kConfigPath);
            if (file.is_open()) {
                file << j.dump(4);
            }
        } catch (...) {
        }
    }
}