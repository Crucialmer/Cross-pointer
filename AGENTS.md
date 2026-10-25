# AGENTS.md

## Project

Windows desktop app: movable crosshair overlay rendered via Direct2D transparent layered windows. C++20, Win32 API only.

## Build

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

- **MSVC-only** — `CMakeLists.txt` enforces `FATAL_ERROR` for non-MSVC compilers.
- **Ninja generator** — pass `-G Ninja` with cmake.
- Build output is written to the **project root** (`${PROJECT_SOURCE_DIR}`).
- Release links `/MT` (static runtime); Debug uses `/MDd` (dynamic).
- Executable output name is strictly `CrossPointer.exe`.

## Architecture

| File(s)                      | Role                                                                                                                                                                     |
| ---------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `main.cpp`                   | `WinMain` entry — single-instance mutex, precision 1ms timer, DPI awareness, immersive dark mode support, message loop, tray icon integration                           |
| `src/crosshair.cpp/h`        | Four transparent `WS_EX_LAYERED` + `WS_EX_TRANSPARENT` topmost popup windows (L/R top/bottom). Direct2D rendering, ~125fps throttle                                     |
| `src/tray.cpp/h`             | Native Win32 system tray manager (`Shell_NotifyIconW`), left-click toggle, double-click config open, right-click context menu, dynamic tooltip                             |
| `src/config.cpp/h`           | INI parsing, system theme detection (`RegGetValueW`), light/dark theme color mapping, hotkey parsing                                                                    |
| `src/hotkey.cpp/h`           | Registers/unregisters global hotkeys via `RegisterHotKey`                                                                                                               |
| `src/config_file_util.cpp/h` | Embedded `DEFAULT_INI` string; manages `CrossPointer.ini` next to EXE, strict pure-green execution without AppData fallbacks                                              |
| `res/resource.rc`            | App icon (macOS 26 squircle aesthetic) and version info                                                                                                                  |

## Configuration (CrossPointer.ini)

- Auto-generated next to the EXE on first launch.
- Config is re-read on every toggle or tray reload.
- Sections: `[Theme]`, `[Light_Mode]`, `[Dark_Mode]`, `[Crosshair]`, `[Horizontal]`, `[Vertical]`, `[Hide_Show_Hotkey]`, `[Exit_Hotkey]`.
- Supports automatic light/dark theme switching based on Windows system settings.
