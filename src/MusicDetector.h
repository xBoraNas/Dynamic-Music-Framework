#pragma once

#include <RE/Skyrim.h>
#include <string>
#include <unordered_set>

namespace DynamicBossMusic::Combat {

    class MusicDetector :
        public RE::BSTEventSink<RE::TESCombatEvent>,
        public RE::BSTEventSink<RE::TESHitEvent>,
        public RE::BSTEventSink<RE::TESDeathEvent>,
        public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static MusicDetector* GetSingleton() {
            static MusicDetector instance;
            return &instance;
        }

        void RegisterEvents();
        bool IsBossActive() const { return !_activeBosses.empty(); }
        void ValidateActiveBosses();
        void ForceEndEncounter();

        // SMF arayüzündeki "Grab Crosshair Target" butonu için yardımcı fonksiyon
        static std::string GetCrosshairTargetName();

        // Olay dinleyicileri
        RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

    private:
        MusicDetector() = default;

        bool IsBossActor(RE::Actor* a_actor) const;
        std::string ResolveCategoryForBoss(RE::Actor* a_actor) const;
        void StartBossEncounter(RE::Actor* a_boss);
        void EndBossEncounter();
        void SuppressVanillaCombatMusic();

        std::unordered_set<RE::FormID> _activeBosses;
        RE::FormID _primaryBossFormID = 0;
        bool _phase2Triggered = false;
    };
}
