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
  void SwitchTab(int new_tab);
  void UpdateTabVisibility();

  HINSTANCE instance_ = nullptr;
  HWND parent_ = nullptr;
  HWND hwnd_ = nullptr;
  std::wstring config_path_;
  Config config_;
  ApplyCallback on_apply_;

  HFONT font_title_ = nullptr;
  HFONT font_card_title_ = nullptr;
  HFONT font_bold_ = nullptr;
  HFONT font_regular_ = nullptr;
  HFONT font_small_ = nullptr;

  int current_tab_ = 0;  // 0: 外观尺寸, 1: 色彩模式, 2: 全局快捷键, 3: 系统与更新

  // Tab 0: 外观尺寸
  HWND slider_gap_ = nullptr;
  HWND lbl_gap_val_ = nullptr;
  HWND slider_h_width_ = nullptr;
  HWND lbl_h_width_val_ = nullptr;
  HWND slider_v_width_ = nullptr;
  HWND lbl_v_width_val_ = nullptr;
  HWND preview_crosshair_ = nullptr;

  // Tab 1: 色彩与模式
  HWND chk_auto_theme_ = nullptr;
  HWND preview_light_ = nullptr;
  HWND btn_color_light_ = nullptr;
  HWND slider_alpha_light_ = nullptr;
  HWND lbl_alpha_light_val_ = nullptr;
  LineColor temp_light_color_;

  HWND preview_dark_ = nullptr;
  HWND btn_color_dark_ = nullptr;
  HWND slider_alpha_dark_ = nullptr;
  HWND lbl_alpha_dark_val_ = nullptr;
  LineColor temp_dark_color_;

  // Tab 2: 快捷键 (支持单键复用与任意按键顺序)
  HWND chk_toggle_ctrl_ = nullptr;
  HWND chk_toggle_alt_ = nullptr;
  HWND chk_toggle_win_ = nullptr;
  HWND chk_toggle_shift_ = nullptr;
  HWND combo_toggle_vk_ = nullptr;

  HWND chk_exit_ctrl_ = nullptr;
  HWND chk_exit_alt_ = nullptr;
  HWND chk_exit_win_ = nullptr;
  HWND chk_exit_shift_ = nullptr;
  HWND combo_exit_vk_ = nullptr;

  HWND btn_sync_hotkey_ = nullptr;

  // Tab 3: 系统与更新
  HWND chk_show_on_startup_ = nullptr;
  HWND chk_auto_dpi_ = nullptr;
  HWND chk_autostart_ = nullptr;
  HWND chk_auto_check_update_ = nullptr;
  HWND btn_check_update_ = nullptr;

  // 底部统一动作栏
  HWND btn_save_ = nullptr;
  HWND btn_defaults_ = nullptr;
  HWND btn_close_ = nullptr;

  HBRUSH br_bg_ = nullptr;
  HBRUSH br_card_ = nullptr;
  HBRUSH br_seg_bg_ = nullptr;
  HBRUSH br_tab_active_ = nullptr;
  COLORREF cr_text_ = RGB(0, 0, 0);
  COLORREF cr_secondary_ = RGB(120, 120, 120);
  COLORREF cr_card_border_ = RGB(220, 220, 225);
  COLORREF cr_divider_ = RGB(230, 230, 235);
  COLORREF cr_accent_ = RGB(0, 122, 255);
  bool is_dark_mode_ = false;
};
