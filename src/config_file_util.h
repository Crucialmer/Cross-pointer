#pragma once

#include <string>

inline constexpr char kDefaultIni[] = R"([Crosshair]
# The gap (empty space) around the mouse cursor; valid range: 0-400
# 16px provides a balanced, elegant spacing consistent with modern macOS design aesthetics.
Gap = 16

[Horizontal]
# Valid range: 1-200
Width = 2
# Apple System Azure Blue (#007AFF) with soft 70% translucency (Alpha = 180)
# Clean, high-clarity, non-intrusive on dark and light backgrounds.
R = 0
G = 122
B = 255
Alpha = 180

[Vertical]
# Valid range: 1-200
Width = 2
R = 0
G = 122
B = 255
Alpha = 180

[Hide_Show_Hotkey]
# Supported modifiers: Ctrl, Alt, Win, Shift (comma-separated)
# F1-F24 may be used without modifiers by leaving Mod empty.
# Invalid modifiers fall back to Ctrl+Alt+Win.
Mod = Ctrl,Alt,Win

# Letters A-Z; numbers 0-9/Numpad0-Numpad9; F1-F24;
# Space, Tab, Backspace, Escape, Enter, Delete, Insert; XButton1/2;
# and common OEM punctuation keys are supported.
# An invalid value falls back to H.
VK = H

[Exit_Hotkey]
# Same rules as Hide_Show_Hotkey. An invalid modifier falls back to
# Ctrl+Alt+Win; an invalid key falls back to E.
Mod = Ctrl,Alt,Win
VK = E
)";

[[nodiscard]] std::wstring GetConfigPath();
[[nodiscard]] bool EnsureConfigExists(const std::wstring& path);
