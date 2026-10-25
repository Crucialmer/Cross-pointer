#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <windows.h>

struct ColorChannel {
  std::uint8_t value = 0;

  constexpr ColorChannel() = default;
  constexpr explicit ColorChannel(int input)
      : value(static_cast<std::uint8_t>(std::clamp(input, 0, 255))) {}
};

struct LineColor {
  ColorChannel r{0};
  ColorChannel g{122};
  ColorChannel b{255};
  ColorChannel alpha{180};
};

struct LineConfig {
  unsigned int width = 2;
  ColorChannel r{0};
  ColorChannel g{122};
  ColorChannel b{255};
  ColorChannel alpha{180};

  void Clamp() noexcept { width = std::clamp(width, 1U, 200U); }
};

struct HotkeyConfig {
  unsigned int mod = MOD_SHIFT | MOD_ALT;
  unsigned int vk = 'E';

  void Clamp(char mode) noexcept;
};

struct Config {
  unsigned int gap = 16;
  bool auto_theme = true;
  bool auto_dpi = true;  // 适配高 DPI / 4K / 8K 分辨率按比例缩放
  bool auto_check_update = true;  // 启动时静默检查是否有新版本更新
  bool show_on_startup = true;   // 启动软件时是否默认显示十字线 (true = 显示, false = 隐藏)

  LineColor light_theme{ColorChannel(0), ColorChannel(102), ColorChannel(230),
                        ColorChannel(200)};
  LineColor dark_theme{ColorChannel(0), ColorChannel(210), ColorChannel(255),
                       ColorChannel(180)};

  LineConfig horizontal;
  LineConfig vertical;
  HotkeyConfig hotkey_h_s{MOD_SHIFT | MOD_ALT, 'E'};
  HotkeyConfig hotkey_exit{MOD_SHIFT | MOD_ALT, 'Q'};

  [[nodiscard]] bool Load(const wchar_t* filename);
  [[nodiscard]] bool Save(const wchar_t* filename) const;

  static unsigned int ParseMod(const wchar_t* text, bool function_key);
  static std::pair<unsigned int, bool> ParseVK(const wchar_t* text, char mode);
  static std::wstring ModToString(unsigned int mod);
  static std::wstring VKToString(unsigned int vk);

  void ClampAll() noexcept;
};

[[nodiscard]] bool IsSystemInLightTheme() noexcept;
[[nodiscard]] Config GetThemedConfig(const Config& base, bool is_light) noexcept;
