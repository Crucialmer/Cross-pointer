#pragma once

#include <windows.h>
#include <functional>
#include <string>

#include "./config.h"

class SettingsWindow {
 public:
  using ApplyCallback = std::function<bool(const Config& new_config)>;

  SettingsWindow() noexcept = default;
  ~SettingsWindow();

  SettingsWindow(const SettingsWindow&) = delete;
  SettingsWindow& operator=(const SettingsWindow&) = delete;

  // 打开或激活偏好设置窗口
  void Show(HINSTANCE instance, HWND parent, const Config& current_config,
            const std::wstring& config_path, ApplyCallback on_apply);

  void Close() noexcept;
  [[nodiscard]] bool IsOpen() const noexcept { return hwnd_ != nullptr; }

 private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp,
                                  LPARAM lp) noexcept;
  LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

  void CreateControls();
  void LayoutControls(int dpi);
  void UpdateThemeStyles();
  void LoadValuesToUI();
  bool SaveValuesFromUI();
  void UpdateColorPreview(HWND hwnd_preview, const LineColor& color);

  HINSTANCE instance_ = nullptr;
  HWND parent_ = nullptr;
  HWND hwnd_ = nullptr;
  std::wstring config_path_;
  Config config_;
  ApplyCallback on_apply_;

  HFONT font_title_ = nullptr;
  HFONT font_bold_ = nullptr;
  HFONT font_regular_ = nullptr;
  HFONT font_small_ = nullptr;

  // 控件句柄
  HWND chk_auto_theme_ = nullptr;
  HWND chk_auto_dpi_ = nullptr;
  HWND chk_autostart_ = nullptr;
  HWND chk_auto_check_update_ = nullptr;
  HWND btn_check_update_ = nullptr;

  HWND slider_gap_ = nullptr;
  HWND lbl_gap_val_ = nullptr;
  HWND slider_h_width_ = nullptr;
  HWND lbl_h_width_val_ = nullptr;
  HWND slider_v_width_ = nullptr;
  HWND lbl_v_width_val_ = nullptr;

  // 浅色色彩
  HWND preview_light_ = nullptr;
  HWND btn_color_light_ = nullptr;
  HWND slider_alpha_light_ = nullptr;
  HWND lbl_alpha_light_val_ = nullptr;
  LineColor temp_light_color_;

  // 深色色彩
  HWND preview_dark_ = nullptr;
  HWND btn_color_dark_ = nullptr;
  HWND slider_alpha_dark_ = nullptr;
  HWND lbl_alpha_dark_val_ = nullptr;
  LineColor temp_dark_color_;

  // 快捷键 显隐
  HWND chk_toggle_ctrl_ = nullptr;
  HWND chk_toggle_alt_ = nullptr;
  HWND chk_toggle_win_ = nullptr;
  HWND chk_toggle_shift_ = nullptr;
  HWND combo_toggle_vk_ = nullptr;

  // 快捷键 退出
  HWND chk_exit_ctrl_ = nullptr;
  HWND chk_exit_alt_ = nullptr;
  HWND chk_exit_win_ = nullptr;
  HWND chk_exit_shift_ = nullptr;
  HWND combo_exit_vk_ = nullptr;

  // 底部按钮
  HWND btn_save_ = nullptr;
  HWND btn_defaults_ = nullptr;
  HWND btn_close_ = nullptr;

  HBRUSH br_bg_ = nullptr;
  HBRUSH br_card_ = nullptr;
  COLORREF cr_text_ = RGB(0, 0, 0);
  COLORREF cr_secondary_ = RGB(120, 120, 120);
  COLORREF cr_card_border_ = RGB(220, 220, 225);
  bool is_dark_mode_ = false;
};
