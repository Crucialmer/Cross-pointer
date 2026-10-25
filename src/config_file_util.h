#pragma once

#include <string>

inline constexpr char kDefaultIni[] = R"([Theme]
# 1 = 跟随系统自动切换黑白模式 (Auto follow Windows Light/Dark theme)
# 0 = 关闭自动切换，使用固定颜色
AutoFollowSystem = 1

[Light_Mode]
# 浅色模式（白底文档/表格）：推荐高对比沉稳蔚蓝
R = 0
G = 102
B = 230
Alpha = 200

[Dark_Mode]
# 深色模式（深色代码/夜间背景）：推荐明亮激光微光青
R = 0
G = 210
B = 255
Alpha = 180

[Crosshair]
# The gap (empty space) around the mouse cursor; valid range: 0-400
Gap = 16

[Horizontal]
# Valid range: 1-200
Width = 2
# 当 AutoFollowSystem = 0 时使用的固定颜色
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
