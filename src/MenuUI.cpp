#include "MenuUI.h"
#include "AudioEngine.h"
#include "MusicDetector.h"
#include "AmbientDetector.h"
#include "ConfigManager.h"
#include "SKSEMCP/utils.hpp"

#include <cstring>

namespace DynamicBossMusic::UI {
    using ImVec2 = ImGuiMCP::ImVec2;
    using ImVec4 = ImGuiMCP::ImVec4;
    

    void RegisterMenu() {
        if (!SKSEMenuFramework::IsInstalled()) {
            return;
        }

        SKSEMenuFramework::SetSection("Dynamic Music Framework");
        SKSEMenuFramework::AddSectionItem("Settings & Audio", RenderMainMenu);
    }

    void __stdcall RenderMainMenu() {
        if (ImGuiMCP::Checkbox("Enable Dynamic Music Framework", &Settings.enabled)) {
            if (!Settings.enabled) {
                Combat::MusicDetector::GetSingleton()->ForceEndEncounter();
                Ambient::AmbientDetector::GetSingleton()->StopCurrent();
                Ambient::AmbientDetector::GetSingleton()->ResetState();
                Audio::AudioEngine::GetSingleton().Stop(Settings.fadeDuration);
            } else {
                Ambient::AmbientDetector::GetSingleton()->NotifySettingChanged();
                Config::Save();
            }
        }
        if (ImGuiMCP::Checkbox("Enable Combat Music", &Settings.enableCombatMusic)) {
            if (!Settings.enableCombatMusic) Audio::AudioEngine::GetSingleton().Stop(UI::Settings.fadeDuration);
            Config::Save();
        }
        if (ImGuiMCP::Checkbox("Enable Ambient/Zone Music", &Settings.enableAmbient)) {
            Ambient::AmbientDetector::GetSingleton()->NotifySettingChanged();
            Config::Save();
        }
        ImGuiMCP::SameLine();
        ImGuiMCP::TextDisabled("(?)");
        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("Automatically plays custom boss themes from Data\\Music\\DynamicBoss\\");
        }

        ImGuiMCP::Separator();

        if (!Settings.enabled) {
            ImGuiMCP::BeginDisabled();
        }

        if (ImGuiMCP::BeginTabBar("DynamicBossTabs")) {

            // =================================================================
            // SEKME 1: GENERAL & AUDIO
            // =================================================================
            if (ImGuiMCP::BeginTabItem("General & Audio")) {
                ImGuiMCP::Spacing();
                ImGuiMCP::TextColored(ImVec4(0.95f, 0.65f, 0.20f, 1.0f), "Audio Controls");

                if (ImGuiMCP::SliderFloat("Master Volume", &Settings.masterVolume, 0.0f, 1.0f, "%.2f")) {
                    Audio::AudioEngine::GetSingleton().SetMasterVolume(Settings.masterVolume);
                }

                ImGuiMCP::SliderFloat("Fade In / Out (sec)", &Settings.fadeDuration, 0.0f, 5.0f, "%.1f s");
                ImGuiMCP::SliderFloat("Combat <-> Ambient Fade (sec)", &Settings.combatAmbientFadeDuration, 0.0f, 5.0f, "%.1f s");
                ImGuiMCP::Checkbox("Mute Vanilla Skyrim Combat Music", &Settings.muteVanillaCombat);
                ImGuiMCP::Checkbox("Shuffle Tracks in Category", &Settings.randomizeTracks);

                ImGuiMCP::Spacing();
                ImGuiMCP::Separator();
                ImGuiMCP::Spacing();

                ImGuiMCP::TextColored(ImVec4(0.95f, 0.30f, 0.30f, 1.0f), "Phase 2 (Climax Transition)");
                ImGuiMCP::Checkbox("Enable Phase 2 Music Switch", &Settings.enablePhase2);
                if (Settings.enablePhase2) {
                    float thresholdPct = Settings.phase2HealthThreshold * 100.0f;
                    if (ImGuiMCP::SliderFloat("Trigger at Boss HP %", &thresholdPct, 10.0f, 80.0f, "%.0f%%")) {
                        Settings.phase2HealthThreshold = thresholdPct / 100.0f;
                    }
                }

                ImGuiMCP::Spacing();
                ImGuiMCP::Separator();
                ImGuiMCP::Spacing();

                ImGuiMCP::TextColored(ImVec4(0.40f, 0.80f, 1.0f, 1.0f), "Live Preview Player");
                ImGuiMCP::Text("Now Playing: %s", CurrentlyPlayingTrack.c_str());

                if (ImGuiMCP::Button("Play Test Track")) {
                    Audio::AudioEngine::GetSingleton().PlayRandomFromCategory(SelectedCategory, 0.5f);
                }
                ImGuiMCP::SameLine();
                if (ImGuiMCP::Button("Stop Audio")) {
                    Audio::AudioEngine::GetSingleton().Stop(0.5f);
                }

                ImGuiMCP::Spacing();
                ImGuiMCP::Separator();
                if (ImGuiMCP::Button("Save Settings")) {
                    Config::Save();
                }

                ImGuiMCP::EndTabItem();
            }

            // =================================================================
            // SEKME 2: CATEGORIES & TRACKS
            // =================================================================
            if (ImGuiMCP::BeginTabItem("Categories & Tracks")) {
                ImGuiMCP::Spacing();
                if (ImGuiMCP::Button("Rescan Music Folders")) {
                    Audio::AudioEngine::GetSingleton().ScanDirectories();
                }
                ImGuiMCP::SameLine();
                ImGuiMCP::TextDisabled("Path: Data\\Music\\DynamicMusic\\{Combat,Ambient}\\<Category>\\");

                ImGuiMCP::Spacing();

                if (ImGuiMCP::BeginTabBar("CatTypeTabs")) {
                    // Combat
                    if (ImGuiMCP::BeginTabItem("Combat")) {
                        if (SelectedCategory == "Inns" || SelectedCategory == "Cities" || SelectedCategory == "Shops" || SelectedCategory == "Dungeons") SelectedCategory = "Dragons";
                        ImGuiMCP::BeginChild("CombatCatList", ImVec2(200, 320), true);
                        for (const auto& cat : Categories) {
                            if (cat == "Inns" || cat == "Cities" || cat == "Shops" || cat == "Dungeons") continue;
                            bool isSelected = (SelectedCategory == cat);
                            if (ImGuiMCP::Selectable(cat.c_str(), isSelected)) SelectedCategory = cat;
                        }
                        ImGuiMCP::EndChild();
                        ImGuiMCP::SameLine();
                        ImGuiMCP::BeginChild("CombatTrackList", ImVec2(0, 320), true);
                        ImGuiMCP::Text("Tracks in [%s]:", SelectedCategory.c_str());
                        ImGuiMCP::Separator();
                        auto& tracks = CategoryTracks[SelectedCategory];
                        if (tracks.empty()) ImGuiMCP::TextDisabled("No audio files found (.mp3, .wav, .ogg, .flac)");
                        else for (const auto& track : tracks) {
                            std::string key = SelectedCategory + "|" + track;
                            bool enabled = (UI::DisabledTracks.count(key) == 0);
                            if (ImGuiMCP::Checkbox(("##chk" + track).c_str(), &enabled)) {
                                if (enabled) UI::DisabledTracks.erase(key);
                                else UI::DisabledTracks.insert(key);
                                Config::Save();
                                if (!enabled && Audio::AudioEngine::GetSingleton().GetCurrentTrackName() == track) {
                                    Audio::AudioEngine::GetSingleton().Stop(UI::Settings.fadeDuration);
                                }
                            }
                            ImGuiMCP::SameLine();
                            ImGuiMCP::Text("%s", track.c_str());
                            ImGuiMCP::SameLine(ImGuiMCP::GetWindowWidth() - 75);
                            std::string btnLabel = "Play##" + track;
                            if (ImGuiMCP::SmallButton(btnLabel.c_str())) {
                                std::string fullPath = Audio::AudioEngine::GetSingleton().GetTrackFullPath(SelectedCategory, track);
                                Audio::AudioEngine::GetSingleton().PlayFile(fullPath, track, 0.3f);
                            }
                        }
                        ImGuiMCP::EndChild();
                        ImGuiMCP::EndTabItem();
                    }
                    // Ambient
                    if (ImGuiMCP::BeginTabItem("Ambient / Zones")) {
                        if (!(SelectedCategory == "Inns" || SelectedCategory == "Cities" || SelectedCategory == "Shops" || SelectedCategory == "Dungeons")) SelectedCategory = "Inns";
                        ImGuiMCP::BeginChild("AmbientCatList", ImVec2(200, 320), true);
                        for (const auto& cat : Categories) {
                            if (!(cat == "Inns" || cat == "Cities" || cat == "Shops" || cat == "Dungeons")) continue;
                            bool isSelected = (SelectedCategory == cat);
                            if (ImGuiMCP::Selectable(cat.c_str(), isSelected)) SelectedCategory = cat;
                        }
                        ImGuiMCP::EndChild();
                        ImGuiMCP::SameLine();
                        ImGuiMCP::BeginChild("AmbientTrackList", ImVec2(0, 320), true);
                        ImGuiMCP::Text("Tracks in [%s]:", SelectedCategory.c_str());
                        ImGuiMCP::Separator();
                        auto& tracks = CategoryTracks[SelectedCategory];
                        if (tracks.empty()) ImGuiMCP::TextDisabled("No audio files found (.mp3, .wav, .ogg, .flac)");
                        else for (const auto& track : tracks) {
                            std::string key = SelectedCategory + "|" + track;
                            bool enabled = (UI::DisabledTracks.count(key) == 0);
                            if (ImGuiMCP::Checkbox(("##chk" + track).c_str(), &enabled)) {
                                if (enabled) UI::DisabledTracks.erase(key);
                                else UI::DisabledTracks.insert(key);
                                Config::Save();
                                if (!enabled && Audio::AudioEngine::GetSingleton().GetCurrentTrackName() == track) {
                                    Audio::AudioEngine::GetSingleton().Stop(UI::Settings.fadeDuration);
                                }
                            }
                            ImGuiMCP::SameLine();
                            ImGuiMCP::Text("%s", track.c_str());
                            ImGuiMCP::SameLine(ImGuiMCP::GetWindowWidth() - 75);
                            std::string btnLabel = "Play##" + track;
                            if (ImGuiMCP::SmallButton(btnLabel.c_str())) {
                                std::string fullPath = Audio::AudioEngine::GetSingleton().GetTrackFullPath(SelectedCategory, track);
                                Audio::AudioEngine::GetSingleton().PlayFile(fullPath, track, 0.3f);
                            }
                        }
                        ImGuiMCP::EndChild();
                        ImGuiMCP::EndTabItem();
                    }
                    ImGuiMCP::EndTabBar();
                }

                ImGuiMCP::EndTabItem();
            }

            // =================================================================
            // SEKME 3: BOSS OVERRIDES
            // =================================================================
            if (ImGuiMCP::BeginTabItem("Boss Overrides")) {
                ImGuiMCP::Spacing();
                ImGuiMCP::TextWrapped(
                    "Assign specific music categories or tracks to individual bosses (e.g., Alduin, Miraak, Harkon, Karstaag)."
                );
                ImGuiMCP::Spacing();

                static char newBossBuf[128] = "";
                ImGuiMCP::InputTextWithHint("##BossName", "Enter Boss Name or grab target...", newBossBuf, sizeof(newBossBuf));
                ImGuiMCP::SameLine();
                if (ImGuiMCP::Button("Grab Crosshair Target")) {
                    std::string targetName = Combat::MusicDetector::GetCrosshairTargetName();
                    if (!targetName.empty()) {
                        std::strncpy(newBossBuf, targetName.c_str(), sizeof(newBossBuf) - 1);
                        newBossBuf[sizeof(newBossBuf) - 1] = '\0';
                    }
                }
                ImGuiMCP::SameLine();
                if (ImGuiMCP::Button("Add Override") && newBossBuf[0] != '\0') {
                    Overrides.push_back({ newBossBuf, SelectedCategory, "" });
                    newBossBuf[0] = '\0';
                    Config::Save();
                }

                ImGuiMCP::Spacing();
                ImGuiMCP::Separator();

                if (ImGuiMCP::BeginTable("OverridesTable", 3, ImGuiMCP::ImGuiTableFlags_Borders | ImGuiMCP::ImGuiTableFlags_RowBg)) {
                    ImGuiMCP::TableSetupColumn("Boss Name");
                    ImGuiMCP::TableSetupColumn("Assigned Category");
                    ImGuiMCP::TableSetupColumn("Action", ImGuiMCP::ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGuiMCP::TableHeadersRow();

                    for (int i = 0; i < static_cast<int>(Overrides.size()); ++i) {
                        ImGuiMCP::TableNextRow();
                        ImGuiMCP::TableSetColumnIndex(0);
                        ImGuiMCP::Text("%s", Overrides[i].bossName.c_str());

                        ImGuiMCP::TableSetColumnIndex(1);
                        ImGuiMCP::Text("%s", Overrides[i].assignedCategory.c_str());

                        ImGuiMCP::TableSetColumnIndex(2);
                        std::string delBtn = "Remove##" + std::to_string(i);
                        if (ImGuiMCP::SmallButton(delBtn.c_str())) {
                            Overrides.erase(Overrides.begin() + i);
                            Config::Save();
                            break;
                        }
                    }
                    ImGuiMCP::EndTable();
                }

                ImGuiMCP::EndTabItem();
            }

            ImGuiMCP::EndTabBar();
        }

        if (!Settings.enabled) {
            ImGuiMCP::EndDisabled();
        }
    }
}
