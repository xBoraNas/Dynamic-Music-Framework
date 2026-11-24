# Dynamic Music Framework

SKSE plugin for Skyrim Special Edition / Anniversary Edition that plays custom music during boss fights and ambient zones, with a real-time in-game menu powered by SKSE Menu Framework.

## Features
- **Boss Combat Music:** Per-boss or per-category custom tracks, configurable HP-threshold Phase 2 transitions, and smooth crossfades.
- **Ambient / Zone Music:** Optional background music for **Inns**, **Cities** (automatically distinguishes city walls from exterior outskirts), **Shops**, and **Dungeons**.
- **Independent Fade Controls:** Separate fade duration sliders for general transitions and **Combat ↔ Ambient** switches.
- **Multi-Boss & Follower Support:** Tracks concurrent bosses and triggers music even when a follower initiates combat.
- **Smart Mod Compatibility:** Automatically pauses for map music mods (`MapMenu`), dialogue, and high-priority scene/bard audio while preventing vanilla or music overhaul tracks from overlapping.
- **Live In-Game UI:** Per-track enable/disable checkboxes, audio preview player, shuffle toggle, and 1-click crosshair target boss overrides.
- **Alt-Tab & Menu Handling:** Automatically pauses when unfocused or inside pause menus, with volume ducking during dialogue.
- **Zero Dependencies & Native Streaming:** Powered by `miniaudio` (`.mp3`, `.wav`, `.ogg`, `.flac`) in a single standalone DLL with no extra runtime dependencies.

## Requirements
- Skyrim Special Edition (1.5.97) or Anniversary Edition (1.6.x / 1.7.x, including 1.7.104)
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)

## Installation
1. Install via **Mod Organizer 2** or **Vortex** (or extract the archive contents directly into your `Skyrim Special Edition\Data\` folder).
2. Place your audio files (`.mp3`, `.wav`, `.ogg`, `.flac`) into:
   - `Data/Music/DynamicMusic/Combat/<Category>/`
   - `Data/Music/DynamicMusic/Ambient/<Category>/`
3. Launch Skyrim via SKSE.

**Categories (auto-created on first run):**
- **Combat:** `Dragons`, `DragonPriests`, `Vampires`, `UniqueBosses`, `Phase2`, `GeneralBoss`
- **Ambient:** `Inns`, `Cities`, `Shops`, `Dungeons`

## Usage
- Open the **SKSE Menu Framework** overlay → **Dynamic Music Framework** → **Settings & Audio**.
- Adjust master volume, fade durations, and Combat/Ambient toggles.
- Use the **Boss Overrides** tab to assign specific bosses to categories (supports **Grab Crosshair Target**).
- Use the **Categories & Tracks** tab (Combat / Ambient) to preview and enable/disable individual tracks.

## Build
- Visual Studio 2022/2026 + CMake 3.21+
- vcpkg with colorglass registry (CommonLibSSE-NG via FetchContent)
- Configure & Build:
  ```bash
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=".../vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
  cmake --build build --config Release

## License
This project is licensed under the [MIT License](LICENSE).

### Third-Party Libraries
- [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) (MIT License)
- [miniaudio](https://github.com/mackron/miniaudio) (Public Domain / MIT-0)
- [nlohmann/json](https://github.com/nlohmann/json) (MIT License)
