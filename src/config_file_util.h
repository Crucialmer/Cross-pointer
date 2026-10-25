#pragma once

#include <string>

inline constexpr char kDefaultIni[] = R"([General]
# 软件启动时是否立即显示十字辅助线 (1 = 默认显示, 0 = 启动后静默常驻托盘，需按热键呼出)
ShowOnStartup = 1

[Theme]
# 1 = 跟随系统自动切换黑白模式 (Auto follow Windows Light/Dark theme)
# 0 = 关闭自动切换，使用固定颜色
AutoFollowSystem = 1

[Display]
# 是否开启高 DPI / 4K / 8K 分辨率自适应缩放 (1 = 开启, 0 = 关闭)
AutoDpi = 1

[Update]
# 启动时自动检查更新 (1 = 开启, 0 = 关闭)
AutoCheckUpdate = 1

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
# Supported modifiers: Shift, Alt, Ctrl, Win (order-independent, comma or plus separated)
# F1-F24 may be used without modifiers by leaving Mod empty.
# Invalid modifiers fall back to Shift+Alt.
Mod = Shift,Alt

# Letters A-Z; numbers 0-9/Numpad0-Numpad9; F1-F24;
# Space, Tab, Backspace, Escape, Enter, Delete, Insert; XButton1/2;
# and common OEM punctuation keys are supported.
# An invalid value falls back to E.
VK = E

[Exit_Hotkey]
# Same rules as Hide_Show_Hotkey. An invalid modifier falls back to
# Shift+Alt; an invalid key falls back to Q.
Mod = Shift,Alt
VK = Q
)";

[[nodiscard]] std::wstring GetConfigPath();
[[nodiscard]] bool EnsureConfigExists(const std::wstring& path);
