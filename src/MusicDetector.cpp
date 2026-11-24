#include "MusicDetector.h"
#include "AudioEngine.h"
#include "AmbientDetector.h"
#include "MenuUI.h"

#include <algorithm>

namespace DynamicBossMusic::Combat {

    static std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    void MusicDetector::RegisterEvents() {
        auto* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        if (sourceHolder) {
            sourceHolder->AddEventSink<RE::TESCombatEvent>(this);
            sourceHolder->AddEventSink<RE::TESHitEvent>(this);
            sourceHolder->AddEventSink<RE::TESDeathEvent>(this);
        }

        auto* ui = RE::UI::GetSingleton();
        if (ui) {
            ui->AddEventSink<RE::MenuOpenCloseEvent>(this);
        }
    }

    std::string MusicDetector::GetCrosshairTargetName() {
        auto* crosshair = RE::CrosshairPickData::GetSingleton();
        if (!crosshair) {
            return "";
        }

        auto targetRef = crosshair->targetActor.get();
        if (!targetRef) {
            return "";
        }

        const char* name = targetRef->GetName();
        return name ? std::string(name) : "";
    }

    bool MusicDetector::IsBossActor(RE::Actor* a_actor) const {
        if (!a_actor || a_actor->IsDead() || a_actor->IsPlayerTeammate()) {
            return false;
        }

        const char* rawName = a_actor->GetName();
        std::string actorName = rawName ? rawName : "";

        // 1. Kullanıcının Boss Overrides tablosunda tanımlıysa kesinlikle Boss'tur
        for (const auto& ov : UI::Overrides) {
            if (!ov.bossName.empty() && ToLower(ov.bossName) == ToLower(actorName)) {
                return true;
            }
        }

        // 2. Zindan Boss'u (Location Boss Ref) kontrolü
        if (auto* loc = a_actor->GetCurrentLocation()) {
            for (const auto& refData : loc->specialRefs) {
                if (refData.refData.refID == a_actor->GetFormID()) {
                    return true;
                }
            }
        }

        // 3. Ejderha ve Dragon Priest'ler her zaman Boss sayılır
        if (a_actor->HasKeywordString("ActorTypeDragon") || a_actor->HasKeywordString("ActorTypeDragonPriest")) {
            return true;
        }

        // 4. Benzersiz (Unique) ve yüksek seviyeli düşman kontrolü (örn. Ebony Warrior, Harkon, Miraak)
        if (auto* npc = a_actor->GetActorBase()) {
            if (npc->IsUnique() && a_actor->GetLevel() >= 25) {
                return true;
            }
        }

        // Console-spawned or high-level bosses
        if (a_actor->GetLevel() >= 30) {
            return true;
        }

        return false;
    }

    std::string MusicDetector::ResolveCategoryForBoss(RE::Actor* a_actor) const {
        const char* rawName = a_actor->GetName();
        std::string actorName = rawName ? rawName : "";

        // 1. Özel Atama Kontrolü
        for (const auto& ov : UI::Overrides) {
            if (!ov.bossName.empty() && ToLower(ov.bossName) == ToLower(actorName)) {
                return ov.assignedCategory;
            }
        }

        // 2. Irk / Keyword Tabanlı Kategori Seçimi
        if (a_actor->HasKeywordString("ActorTypeDragon")) {
            return "Dragons";
        }
        if (a_actor->HasKeywordString("ActorTypeDragonPriest")) {
            return "DragonPriests";
        }
        if (a_actor->HasKeywordString("Vampire")) {
            return "Vampires";
        }

        if (auto* npc = a_actor->GetActorBase()) {
            if (npc->IsUnique()) {
                return "UniqueBosses";
            }
        }

        return "GeneralBoss";
    }

    void MusicDetector::SuppressVanillaCombatMusic() {
        if (!UI::Settings.muteVanillaCombat) {
            return;
        }

        // Skyrim'in varsayılan savaş müziklerini sustur
        if (auto* dataHandler = RE::TESDataHandler::GetSingleton()) {
            for (auto* musicType : dataHandler->GetFormArray<RE::BGSMusicType>()) {
                if (!musicType) continue;
                const char* edid = musicType->GetFormEditorID();
                std::string id = edid ? edid : "";
                if (id.find("Combat") != std::string::npos || id.find("Boss") != std::string::npos) {
                    musicType->DoFinish(true);
                }
            }
        }
    }

    void MusicDetector::StartBossEncounter(RE::Actor* a_boss) {
        if (!UI::Settings.enabled || !UI::Settings.enableCombatMusic || !a_boss) {
            return;
        }

        RE::FormID id = a_boss->GetFormID();
        bool wasEmpty = _activeBosses.empty();
        _activeBosses.insert(id);
        if (_primaryBossFormID == 0) _primaryBossFormID = id;

        if (!wasEmpty) {
            return; // already playing boss music
        }

        _phase2Triggered = false;
        SuppressVanillaCombatMusic();
        Ambient::AmbientDetector::GetSingleton()->OnBossStart();

        std::string category = ResolveCategoryForBoss(a_boss);
        Audio::AudioEngine::GetSingleton().PlayRandomFromCategory(category, UI::Settings.combatAmbientFadeDuration);
    }

    void MusicDetector::EndBossEncounter() {
        _activeBosses.clear();
        _primaryBossFormID = 0;
        _phase2Triggered = false;
        SuppressVanillaCombatMusic();
        Audio::AudioEngine::GetSingleton().Stop(UI::Settings.combatAmbientFadeDuration);

        Ambient::AmbientDetector::GetSingleton()->ResetState();
    }

    void MusicDetector::ValidateActiveBosses() {
        bool changed = false;
        std::vector<RE::FormID> toRemove;
        for (auto id : _activeBosses) {
            auto* actor = RE::TESForm::LookupByID<RE::Actor>(id);
            if (!actor || actor->IsDead() || !actor->IsInCombat()) {
                toRemove.push_back(id);
                changed = true;
            }
        }
        for (auto id : toRemove) _activeBosses.erase(id);
        if (changed && _primaryBossFormID != 0 && _activeBosses.find(_primaryBossFormID) == _activeBosses.end()) {
            _primaryBossFormID = _activeBosses.empty() ? 0 : *_activeBosses.begin();
        }
        if (_activeBosses.empty() && changed) {
            EndBossEncounter();
        }
        // Safety: if no active bosses and ambient disabled, ensure music stops
        if (_activeBosses.empty() && !UI::Settings.enableAmbient && Audio::AudioEngine::GetSingleton().IsPlaying()) {
            Audio::AudioEngine::GetSingleton().Stop(UI::Settings.combatAmbientFadeDuration);
        }
    }

    void MusicDetector::ForceEndEncounter() {
        _activeBosses.clear();
        _primaryBossFormID = 0;
        _phase2Triggered = false;
        Audio::AudioEngine::GetSingleton().Stop(UI::Settings.combatAmbientFadeDuration);
        Ambient::AmbientDetector::GetSingleton()->ResetState();
    }

    // =========================================================================
    // SAVAŞ BAŞLANGICI & BİTİŞİ (TESCombatEvent)
    // =========================================================================
    RE::BSEventNotifyControl MusicDetector::ProcessEvent(
        const RE::TESCombatEvent* a_event,
        RE::BSTEventSource<RE::TESCombatEvent>*) {

        if (!a_event || !UI::Settings.enabled) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* actor = a_event->actor ? a_event->actor->As<RE::Actor>() : nullptr;
        auto* target = a_event->targetActor ? a_event->targetActor->As<RE::Actor>() : nullptr;

        if (!actor || !target) {
            return RE::BSEventNotifyControl::kContinue;
        }

        // Oyuncu bir Boss ile savaşa girdiğinde
        if (target == player && a_event->newState == RE::ACTOR_COMBAT_STATE::kCombat) {
            if (IsBossActor(actor)) {
                StartBossEncounter(actor);
            }
        }
        // Aktif Boss savaştan çıktığında veya oyuncu kaçtığında
        else if (_activeBosses.count(actor->GetFormID()) && a_event->newState == RE::ACTOR_COMBAT_STATE::kNone) {
            _activeBosses.erase(actor->GetFormID());
            if (_primaryBossFormID == actor->GetFormID()) {
                _primaryBossFormID = _activeBosses.empty() ? 0 : *_activeBosses.begin();
            }
            if (_activeBosses.empty()) {
                EndBossEncounter();
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    // =========================================================================
    // PHASE 2 CAN YÜZDESİ TAKİBİ (TESHitEvent)
    // =========================================================================
    RE::BSEventNotifyControl MusicDetector::ProcessEvent(
        const RE::TESHitEvent* a_event,
        RE::BSTEventSource<RE::TESHitEvent>*) {

        if (!a_event || !UI::Settings.enabled || !UI::Settings.enablePhase2) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (_primaryBossFormID == 0 || _phase2Triggered || !a_event->target) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (a_event->target->GetFormID() != _primaryBossFormID) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* boss = a_event->target->As<RE::Actor>();
        if (!boss) {
            return RE::BSEventNotifyControl::kContinue;
        }

        float currentHP = boss->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
        if (boss->IsDead() || currentHP <= 0.0f) {
            RE::FormID id = boss->GetFormID();
            if (_activeBosses.count(id)) {
                _activeBosses.erase(id);
                if (_primaryBossFormID == id) {
                    _primaryBossFormID = _activeBosses.empty() ? 0 : *_activeBosses.begin();
                }
                if (_activeBosses.empty()) {
                    EndBossEncounter();
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }

        float maxHP = boss->AsActorValueOwner()->GetPermanentActorValue(RE::ActorValue::kHealth);

        if (maxHP > 0.0f && (currentHP / maxHP) <= UI::Settings.phase2HealthThreshold) {
            _phase2Triggered = true;
            SuppressVanillaCombatMusic();
            Audio::AudioEngine::GetSingleton().PlayRandomFromCategory("Phase2", UI::Settings.fadeDuration);
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    // =========================================================================
    // BOSS ÖLDÜĞÜNDE MÜZİĞİ BİTİR (TESDeathEvent)
    // =========================================================================
    RE::BSEventNotifyControl MusicDetector::ProcessEvent(
        const RE::TESDeathEvent* a_event,
        RE::BSTEventSource<RE::TESDeathEvent>*) {

        if (!a_event || !a_event->actorDying || _activeBosses.empty()) {
            return RE::BSEventNotifyControl::kContinue;
        }

        RE::FormID dyingId = a_event->actorDying->GetFormID();
        if (_activeBosses.count(dyingId)) {
            _activeBosses.erase(dyingId);
            if (_primaryBossFormID == dyingId) {
                _primaryBossFormID = _activeBosses.empty() ? 0 : *_activeBosses.begin();
            }
            if (_activeBosses.empty()) {
                EndBossEncounter();
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    // =========================================================================
    // OYUN MENÜSÜ AÇILDIĞINDA MÜZİĞİ DURAKLAT (MenuOpenCloseEvent)
    // =========================================================================
    RE::BSEventNotifyControl MusicDetector::ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {

        if (!a_event || !Audio::AudioEngine::GetSingleton().IsPlaying()) {
            return RE::BSEventNotifyControl::kContinue;
        }

        // Oyun duraklatan menüler açıldığında müziği duraklat
        if (a_event->menuName == RE::DialogueMenu::MENU_NAME) {
            Audio::AudioEngine::GetSingleton().SetDialogueOpen(a_event->opening);
        }

        if (a_event->menuName == RE::JournalMenu::MENU_NAME ||
            a_event->menuName == RE::InventoryMenu::MENU_NAME ||
            a_event->menuName == RE::MagicMenu::MENU_NAME ||
            a_event->menuName == RE::MapMenu::MENU_NAME ||
            a_event->menuName == RE::SleepWaitMenu::MENU_NAME) {

            float fade = (a_event->menuName == RE::MapMenu::MENU_NAME) ? 0.2f : UI::Settings.fadeDuration;
            Audio::AudioEngine::GetSingleton().SetPaused(a_event->opening, fade);
        }

        return RE::BSEventNotifyControl::kContinue;
    }
}
