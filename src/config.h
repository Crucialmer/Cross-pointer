#pragma once

#include <algorithm>
#include <cstdint>
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
  unsigned int mod = 0;
  unsigned int vk = 0;

  void Clamp(char mode) noexcept;
};

struct Config {
  unsigned int gap = 16;
  bool auto_theme = true;

  LineColor light_theme{ColorChannel(0), ColorChannel(102), ColorChannel(230),
                        ColorChannel(200)};
  LineColor dark_theme{ColorChannel(0), ColorChannel(210), ColorChannel(255),
                       ColorChannel(180)};

  LineConfig horizontal;
  LineConfig vertical;
  HotkeyConfig hotkey_h_s;
  HotkeyConfig hotkey_exit;

  [[nodiscard]] bool Load(const wchar_t* filename);

  static unsigned int ParseMod(const wchar_t* text, bool function_key);
  static std::pair<unsigned int, bool> ParseVK(const wchar_t* text, char mode);

  void ClampAll() noexcept;
};

[[nodiscard]] bool IsSystemInLightTheme() noexcept;
[[nodiscard]] Config GetThemedConfig(const Config& base, bool is_light) noexcept;
