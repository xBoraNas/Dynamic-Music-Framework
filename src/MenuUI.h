#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>

namespace DynamicBossMusic::UI {
    struct ModSettings {
        bool enabled = true;
        bool enableCombatMusic = true;
        float masterVolume = 0.80f;
        float fadeDuration = 2.0f;
        float combatAmbientFadeDuration = 1.5f;
        bool muteVanillaCombat = true;

        bool enablePhase2 = true;
        float phase2HealthThreshold = 0.50f; // %50 can
        bool randomizeTracks = true;

        bool enableAmbient = false;
    };

    // Özel Boss -> Şarkı / Kategori eşleştirmesi
    struct BossOverride {
        std::string bossName;
        std::string assignedCategory;
        std::string assignedTrack; // Boşsa kategoriden rastgele çalar
    };

    inline ModSettings Settings;
    inline std::vector<BossOverride> Overrides;

    // Arayüz önizleme durumu
    inline std::string CurrentlyPlayingTrack = "None";
    inline std::string SelectedCategory = "Dragons";
    inline std::vector<std::string> Categories = {
        "Dragons", "DragonPriests", "Vampires", "UniqueBosses", "Phase2", "GeneralBoss",
        "Inns", "Cities", "Shops", "Dungeons"
    };
    inline std::unordered_map<std::string, std::vector<std::string>> CategoryTracks;
    inline std::set<std::string> DisabledTracks; // "category|filename"

    void RegisterMenu();
    void __stdcall RenderMainMenu();
}