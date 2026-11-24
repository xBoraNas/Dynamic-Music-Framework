#pragma once

#include <RE/Skyrim.h>
#include <string>
#include <thread>
#include <atomic>

namespace DynamicBossMusic::Ambient {

    class AmbientDetector {
    public:
        static AmbientDetector* GetSingleton() {
            static AmbientDetector instance;
            return &instance;
        }

        void Register();
        void Shutdown();
        void StopCurrent();
        void ResetState();
        void NotifySettingChanged();
        void CheckNow();
        void OnBossStart();
        void HandleExternalMusicCompatibility();

    private:
        AmbientDetector() = default;
        ~AmbientDetector() { Shutdown(); }

        void PollLoop();
        std::string ResolveCategory() const;

        std::atomic<bool> _running{ false };
        std::thread _thread;
        RE::FormID _lastCellFormID = 0;
        RE::FormID _lastLocationFormID = 0;
        std::string _currentCategory = "None";
        bool _ambientPlaying = false;
        bool _wasEnabled = false;
    };
}
