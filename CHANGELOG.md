# Changelog

## 1.0.1 (2026-11-24) - Bugfixes
- Ambient: city music no longer continues in inns/open world; location change and interior checks fixed.
- Ambient: disabled tracks no longer fall back to combat music; empty categories stop music instead.
- Unchecking a currently playing track now stops it; all-disabled stops current music.
- Ambient toggle re-enables music immediately without waiting for zone change.
- Boss combat end restores ambient music via ResetState.
- Focus/alt-tab pause now uses process ID (fixes false pause).
- Game-thread safety: AmbientDetector and AudioEngine dialogue checks moved to SKSE task interface.
- Dialogue ducking now fades instead of instant volume cut.
- Wait/Sleep menu now fades music instead of hard pause.
- Multiple concurrent bosses supported.
- Track Play buttons use real resolved file paths.
- Thread-safe folder scanning with minimal lock time.
- Menu categories split into Combat / Ambient tabs.

## 1.0.0 (2026-11-24)
- Initial release of Dynamic Music Framework (SKSE + SKSEMenuFramework).
- Boss combat music: per-boss/per-category tracks, Phase 2 transition on HP threshold, fade in/out.
- Ambient/zone music: Inns, Cities, Shops, Dungeons, triggered on location change.
- Multi-boss support (concurrent bosses).
- Per-track enable/disable checkboxes and Combat/Ambient master toggles.
- Vanilla combat music suppression.
- Alt-tab pause, loading screen delay, dialogue volume ducking.
- Wait/Sleep menu fade pause.
- Thread-safe folder scanning and audio engine.
- Folder structure: `Data/Music/DynamicMusic/{Combat,Ambient}/<Category>/`
- Single DLL (static spdlog/fmt), no extra dependencies.