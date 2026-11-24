#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "MenuUI.h"
#include "AudioEngine.h"
#include "MusicDetector.h"
#include "AmbientDetector.h"
#include "ConfigManager.h"

namespace {
    void SetupLog() {
        auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) return;

        auto logFile = *logsFolder / "DynamicBossMusic.log";
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile.string(), true);
        auto logger = std::make_shared<spdlog::logger>("global", std::move(fileSink));

        spdlog::set_default_logger(std::move(logger));
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
    }

    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg) {
        switch (a_msg->type) {
            case SKSE::MessagingInterface::kDataLoaded:
                DynamicBossMusic::Config::Load();
                if (DynamicBossMusic::Audio::AudioEngine::GetSingleton().Initialize()) {
                    SKSE::log::info("AudioEngine initialized successfully.");
                } else {
                    SKSE::log::error("Failed to initialize AudioEngine!");
                }
                DynamicBossMusic::Combat::MusicDetector::GetSingleton()->RegisterEvents();
                DynamicBossMusic::Ambient::AmbientDetector::GetSingleton()->Register();
                DynamicBossMusic::UI::RegisterMenu();
                SKSE::log::info("DynamicBossMusic loaded and ready.");
                break;

            case SKSE::MessagingInterface::kPreLoadGame:
            case SKSE::MessagingInterface::kNewGame:
                // Kayıt yüklenirken veya yeni oyunda çalan müziği temizle
                DynamicBossMusic::Ambient::AmbientDetector::GetSingleton()->StopCurrent();
                DynamicBossMusic::Audio::AudioEngine::GetSingleton().Stop(0.3f);
                break;

            default:
                break;
        }
    }
}


SKSEPluginInfo(
    .Version = SKSE::PluginDeclaration::VersionNumber{ 1, 0, 0, 0 },
    .Name = "DynamicMusicFramework",
    .Author = "Sonbora",
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
);

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SetupLog();
    SKSE::Init(a_skse);

    auto* messaging = SKSE::GetMessagingInterface();
    if (messaging) {
        messaging->RegisterListener(OnSKSEMessage);
    }

    return true;
}
