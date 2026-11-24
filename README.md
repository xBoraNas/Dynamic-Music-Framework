# Dynamic Music Framework

SKSE plugin for Skyrim Special Edition that plays custom music during boss fights and ambient zones, with an in-game menu powered by SKSE Menu Framework.

## Features
- Boss combat music: per-boss / per-category tracks, Phase 2 transition on HP threshold, fade in/out.
- Ambient/zone music: Inns, Cities, Shops, Dungeons, triggered on location change.
- Multi-boss support (concurrent bosses).
- Per-track enable/disable checkboxes and Combat/Ambient master toggles.
- Vanilla combat music suppression.
- Alt-tab pause, loading screen delay, dialogue volume ducking.
- Wait/Sleep menu fade pause.
- Thread-safe folder scanning and audio engine (miniaudio).
- Single DLL, no extra runtime dependencies.

## Requirements
- Skyrim Special Edition 1.6.x / 1.7.x (including 1.7.104)
- SKSE64
- SKSE Menu Framework
- Address Library for SKSE Plugins

## Installation
1. Extract the zip to your Skyrim Special Edition root (overwrite).
2. Put audio files (`.mp3`, `.wav`, `.ogg`, `.flac`) into:
   - `Data/Music/DynamicMusic/Combat/<Category>/`
   - `Data/Music/DynamicMusic/Ambient/<Category>/`
3. Launch Skyrim via SKSE.

Categories (auto-created on first run):
- Combat: Dragons, DragonPriests, Vampires, UniqueBosses, Phase2, GeneralBoss
- Ambient: Inns, Cities, Shops, Dungeons

## Usage
- Open the SKSE Menu Framework menu → "Dynamic Music Framework".
- Adjust volume, fade durations, toggles.
- Use "Boss Overrides" tab to assign specific bosses to categories (crosshair grab supported).
- Use "Categories & Tracks" tabs (Combat / Ambient) to preview and enable/disable tracks.

## Build
- Visual Studio 2022/2026 + CMake 3.21+
- vcpkg with colorglass registry (CommonLibSSE-NG via FetchContent)
- Configure:
  ```
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=".../vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
  cmake --build build --config Release
  ```

## License
See repository.