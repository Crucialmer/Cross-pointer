#include "./settings_window.h"

#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "./autostart.h"
#include "./config_file_util.h"
#include "./updater.h"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

namespace {
constexpr wchar_t kSettingsClassName[] = L"CrossPointerSettingsClass";

constexpr UINT IDC_TAB_SEGMENTED = 2999;

// Tab 0: 外观尺寸
constexpr UINT IDC_SLIDER_GAP = 3010;
constexpr UINT IDC_SLIDER_H_WIDTH = 3011;
constexpr UINT IDC_SLIDER_V_WIDTH = 3012;
constexpr UINT IDC_PREVIEW_CROSSHAIR = 3013;

// Tab 1: 色彩模式
constexpr UINT IDC_CHK_AUTO_THEME = 3020;
constexpr UINT IDC_PREVIEW_LIGHT = 3021;
constexpr UINT IDC_BTN_COLOR_LIGHT = 3022;
constexpr UINT IDC_SLIDER_ALPHA_LIGHT = 3023;
constexpr UINT IDC_PREVIEW_DARK = 3024;
constexpr UINT IDC_BTN_COLOR_DARK = 3025;
constexpr UINT IDC_SLIDER_ALPHA_DARK = 3026;

// Tab 2: 快捷键
constexpr UINT IDC_CHK_TOGGLE_CTRL = 3030;
constexpr UINT IDC_CHK_TOGGLE_ALT = 3031;
constexpr UINT IDC_CHK_TOGGLE_WIN = 3032;
constexpr UINT IDC_CHK_TOGGLE_SHIFT = 3033;
constexpr UINT IDC_COMBO_TOGGLE_VK = 3034;

constexpr UINT IDC_CHK_EXIT_CTRL = 3040;
constexpr UINT IDC_CHK_EXIT_ALT = 3041;
constexpr UINT IDC_CHK_EXIT_WIN = 3042;
constexpr UINT IDC_CHK_EXIT_SHIFT = 3043;
constexpr UINT IDC_COMBO_EXIT_VK = 3044;
constexpr UINT IDC_BTN_SYNC_HOTKEY = 3045;

// Tab 3: 系统与更新
constexpr UINT IDC_CHK_SHOW_ON_STARTUP = 3049;
constexpr UINT IDC_CHK_AUTO_DPI = 3050;
constexpr UINT IDC_CHK_AUTOSTART = 3051;
constexpr UINT IDC_CHK_AUTO_UPDATE = 3052;
constexpr UINT IDC_BTN_CHECK_UPDATE = 3053;

// 底部动作按钮
constexpr UINT IDC_BTN_SAVE = 3060;
constexpr UINT IDC_BTN_DEFAULTS = 3061;
constexpr UINT IDC_BTN_CLOSE = 3062;

int GetWindowDpi(HWND hwnd) noexcept {
  using GetDpiFn = UINT(WINAPI*)(HWND);
  static const auto fn = []() -> GetDpiFn {
    if (const HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
      return reinterpret_cast<GetDpiFn>(GetProcAddress(user32, "GetDpiForWindow"));
    }
    return nullptr;
  }();
  if (fn != nullptr && hwnd != nullptr) {
    const UINT dpi = fn(hwnd);
    if (dpi > 0) return static_cast<int>(dpi);
  }
  const HDC hdc = GetDC(nullptr);
  const int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
  ReleaseDC(nullptr, hdc);
  return dpi > 0 ? dpi : 96;
}

int Scale(int value, int dpi) noexcept {
  return MulDiv(value, dpi, 96);
}

HFONT CreateModernFont(int ptSize, int weight, int dpi) {
  const int height = -MulDiv(ptSize, dpi, 72);
  return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                     L"Segoe UI");
}

const std::vector<std::pair<std::wstring, unsigned int>>& GetKeyList() {
  static const std::vector<std::pair<std::wstring, unsigned int>> list = {
      {L"A", 'A'}, {L"B", 'B'}, {L"C", 'C'}, {L"D", 'D'}, {L"E", 'E'},
      {L"F", 'F'}, {L"G", 'G'}, {L"H", 'H'}, {L"I", 'I'}, {L"J", 'J'},
      {L"K", 'K'}, {L"L", 'L'}, {L"M", 'M'}, {L"N", 'N'}, {L"O", 'O'},
      {L"P", 'P'}, {L"Q", 'Q'}, {L"R", 'R'}, {L"S", 'S'}, {L"T", 'T'},
      {L"U", 'U'}, {L"V", 'V'}, {L"W", 'W'}, {L"X", 'X'}, {L"Y", 'Y'},
      {L"Z", 'Z'}, {L"0", '0'}, {L"1", '1'}, {L"2", '2'}, {L"3", '3'},
      {L"4", '4'}, {L"5", '5'}, {L"6", '6'}, {L"7", '7'}, {L"8", '8'},
      {L"9", '9'}, {L"F1", VK_F1}, {L"F2", VK_F2}, {L"F3", VK_F3},
      {L"F4", VK_F4}, {L"F5", VK_F5}, {L"F6", VK_F6}, {L"F7", VK_F7},
      {L"F8", VK_F8}, {L"F9", VK_F9}, {L"F10", VK_F10}, {L"F11", VK_F11},
      {L"F12", VK_F12}, {L"Space", VK_SPACE}, {L"Tab", VK_TAB},
      {L"Enter", VK_RETURN}, {L"Esc", VK_ESCAPE}, {L"Backspace", VK_BACK},
      {L"Delete", VK_DELETE}, {L"Insert", VK_INSERT},
      {L"Plus (+)", VK_OEM_PLUS}, {L"Minus (-)", VK_OEM_MINUS},
      {L"Comma (,)", VK_OEM_COMMA}, {L"Period (.)", VK_OEM_PERIOD}
  };
  return list;
}
}  // namespace

SettingsWindow::~SettingsWindow() {
  Close();
}

void SettingsWindow::Close() noexcept {
  if (hwnd_ != nullptr) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
  }
  if (font_title_ != nullptr) { DeleteObject(font_title_); font_title_ = nullptr; }
  if (font_card_title_ != nullptr) { DeleteObject(font_card_title_); font_card_title_ = nullptr; }
  if (font_bold_ != nullptr) { DeleteObject(font_bold_); font_bold_ = nullptr; }
  if (font_regular_ != nullptr) { DeleteObject(font_regular_); font_regular_ = nullptr; }
  if (font_small_ != nullptr) { DeleteObject(font_small_); font_small_ = nullptr; }
  if (br_bg_ != nullptr) { DeleteObject(br_bg_); br_bg_ = nullptr; }
  if (br_card_ != nullptr) { DeleteObject(br_card_); br_card_ = nullptr; }
  if (br_seg_bg_ != nullptr) { DeleteObject(br_seg_bg_); br_seg_bg_ = nullptr; }
  if (br_tab_active_ != nullptr) { DeleteObject(br_tab_active_); br_tab_active_ = nullptr; }
}

void SettingsWindow::Show(HINSTANCE instance, HWND parent,
                          const Config& current_config,
                          const std::wstring& config_path,
                          ApplyCallback on_apply) {
  instance_ = instance;
  parent_ = parent;
  config_ = current_config;
  config_path_ = config_path;
  on_apply_ = std::move(on_apply);
  temp_light_color_ = config_.light_theme;
  temp_dark_color_ = config_.dark_theme;
  current_tab_ = 0;

  if (hwnd_ != nullptr) {
    SetForegroundWindow(hwnd_);
    ShowWindow(hwnd_, SW_RESTORE);
    return;
  }

  INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
  InitCommonControlsEx(&icex);

  WNDCLASSEXW wc = {sizeof(wc)};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = instance_;
  wc.lpszClassName = kSettingsClassName;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(101));
  RegisterClassExW(&wc);

  const int dpi = GetWindowDpi(parent);
  RECT win_rc = {0, 0, Scale(620, dpi), Scale(550, dpi)};
  AdjustWindowRectEx(&win_rc, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, WS_EX_DLGMODALFRAME);
  const int win_width = win_rc.right - win_rc.left;
  const int win_height = win_rc.bottom - win_rc.top;

  // 居中屏幕显示
  const int screen_w = GetSystemMetrics(SM_CXSCREEN);
  const int screen_h = GetSystemMetrics(SM_CYSCREEN);
  const int x = (screen_w - win_width) / 2;
  const int y = (screen_h - win_height) / 2;

  hwnd_ = CreateWindowExW(
      WS_EX_DLGMODALFRAME, kSettingsClassName,
      L"CrossPointer 偏好设置",
      WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
      x, y, win_width, win_height, parent_, nullptr, instance_, this);

  if (hwnd_ == nullptr) return;

  // macOS 26 质感：启用 DWM 原生圆角与深浅黑白自适应标题栏
  const DWMNCRENDERINGPOLICY ncp = DWMNCRP_ENABLED;
  DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_POLICY, &ncp, sizeof(ncp));
  const DWORD round_pref = DWMWCP_ROUND;
  DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &round_pref, sizeof(round_pref));

  UpdateThemeStyles();
  CreateControls();
  LayoutControls(dpi);
  LoadValuesToUI();
  UpdateTabVisibility();

  ShowWindow(hwnd_, SW_SHOW);
  UpdateWindow(hwnd_);
  SetForegroundWindow(hwnd_);
}

void SettingsWindow::UpdateThemeStyles() {
  is_dark_mode_ = !IsSystemInLightTheme();
  const BOOL dark_attr = is_dark_mode_ ? TRUE : FALSE;
  DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_attr, sizeof(dark_attr));

  if (br_bg_ != nullptr) DeleteObject(br_bg_);
  if (br_card_ != nullptr) DeleteObject(br_card_);
  if (br_seg_bg_ != nullptr) DeleteObject(br_seg_bg_);
  if (br_tab_active_ != nullptr) DeleteObject(br_tab_active_);

  if (is_dark_mode_) {
    // macOS 26 深空黑曜石沉浸质感
    br_bg_ = CreateSolidBrush(RGB(28, 28, 30));
    br_card_ = CreateSolidBrush(RGB(36, 38, 44));
    br_seg_bg_ = CreateSolidBrush(RGB(42, 44, 52));
    br_tab_active_ = CreateSolidBrush(RGB(0, 122, 255));
    cr_text_ = RGB(245, 245, 247);
    cr_secondary_ = RGB(158, 165, 175);
    cr_card_border_ = RGB(56, 60, 72);
    cr_divider_ = RGB(50, 54, 64);
    cr_accent_ = RGB(0, 122, 255);
  } else {
    // macOS 26 纯白通透质感
    br_bg_ = CreateSolidBrush(RGB(242, 243, 247));
    br_card_ = CreateSolidBrush(RGB(255, 255, 255));
    br_seg_bg_ = CreateSolidBrush(RGB(228, 230, 235));
    br_tab_active_ = CreateSolidBrush(RGB(0, 122, 255));
    cr_text_ = RGB(28, 28, 30);
    cr_secondary_ = RGB(108, 115, 128);
    cr_card_border_ = RGB(225, 228, 235);
    cr_divider_ = RGB(235, 237, 242);
    cr_accent_ = RGB(0, 122, 255);
  }
}

void SettingsWindow::CreateControls() {
  auto create_chk = [&](const wchar_t* text, UINT id) {
    return CreateWindowExW(0, L"BUTTON", text,
                           WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,
                           0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
                           instance_, nullptr);
  };
  auto create_slider = [&](UINT id, int min_val, int max_val) {
    HWND sl = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                              WS_CHILD | TBS_AUTOTICKS | WS_TABSTOP,
                              0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
                              instance_, nullptr);
    SendMessageW(sl, TBM_SETRANGE, TRUE, MAKELPARAM(min_val, max_val));
    return sl;
  };
  auto create_lbl = [&](const wchar_t* text) {
    return CreateWindowExW(0, L"STATIC", text, WS_CHILD | SS_LEFT,
                           0, 0, 10, 10, hwnd_, nullptr, instance_, nullptr);
  };
  auto create_btn = [&](const wchar_t* text, UINT id) {
    return CreateWindowExW(0, L"BUTTON", text,
                           WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
                           0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
                           instance_, nullptr);
  };
  auto create_combo = [&](UINT id) {
    HWND cb = CreateWindowExW(0, L"COMBOBOX", L"",
                              WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                              0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
                              instance_, nullptr);
    for (const auto& item : GetKeyList()) {
      SendMessageW(cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.first.c_str()));
    }
    return cb;
  };

  // Tab 0: 外观尺寸
  slider_gap_ = create_slider(IDC_SLIDER_GAP, 0, 80);
  lbl_gap_val_ = create_lbl(L"16 px");
  slider_h_width_ = create_slider(IDC_SLIDER_H_WIDTH, 1, 20);
  lbl_h_width_val_ = create_lbl(L"2 px");
  slider_v_width_ = create_slider(IDC_SLIDER_V_WIDTH, 1, 20);
  lbl_v_width_val_ = create_lbl(L"2 px");
  preview_crosshair_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_OWNERDRAW,
                                       0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_PREVIEW_CROSSHAIR)), instance_, nullptr);

  // Tab 1: 色彩模式
  chk_auto_theme_ = create_chk(L"跟随 Windows 10/11 系统深浅黑白模式实时自动切换 (推荐)", IDC_CHK_AUTO_THEME);
  preview_light_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_OWNERDRAW,
                                   0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_PREVIEW_LIGHT)), instance_, nullptr);
  btn_color_light_ = create_btn(L"选择浅色色彩...", IDC_BTN_COLOR_LIGHT);
  slider_alpha_light_ = create_slider(IDC_SLIDER_ALPHA_LIGHT, 20, 255);
  lbl_alpha_light_val_ = create_lbl(L"200");

  preview_dark_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_OWNERDRAW,
                                  0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_PREVIEW_DARK)), instance_, nullptr);
  btn_color_dark_ = create_btn(L"选择深色色彩...", IDC_BTN_COLOR_DARK);
  slider_alpha_dark_ = create_slider(IDC_SLIDER_ALPHA_DARK, 20, 255);
  lbl_alpha_dark_val_ = create_lbl(L"180");

  // Tab 2: 快捷键
  chk_toggle_ctrl_ = create_chk(L"Ctrl", IDC_CHK_TOGGLE_CTRL);
  chk_toggle_alt_ = create_chk(L"Alt", IDC_CHK_TOGGLE_ALT);
  chk_toggle_win_ = create_chk(L"Win", IDC_CHK_TOGGLE_WIN);
  chk_toggle_shift_ = create_chk(L"Shift", IDC_CHK_TOGGLE_SHIFT);
  combo_toggle_vk_ = create_combo(IDC_COMBO_TOGGLE_VK);

  chk_exit_ctrl_ = create_chk(L"Ctrl", IDC_CHK_EXIT_CTRL);
  chk_exit_alt_ = create_chk(L"Alt", IDC_CHK_EXIT_ALT);
  chk_exit_win_ = create_chk(L"Win", IDC_CHK_EXIT_WIN);
  chk_exit_shift_ = create_chk(L"Shift", IDC_CHK_EXIT_SHIFT);
  combo_exit_vk_ = create_combo(IDC_COMBO_EXIT_VK);
  btn_sync_hotkey_ = create_btn(L"⚡ 将退出热键设为与显隐相同 (单键显隐+退出)", IDC_BTN_SYNC_HOTKEY);

  // Tab 3: 系统与更新
  chk_show_on_startup_ = create_chk(L"启动软件时默认显示十字辅助线 (Show on Startup)", IDC_CHK_SHOW_ON_STARTUP);
  chk_auto_dpi_ = create_chk(L"适配 4K / 8K 分辨率与高 DPI 屏幕 (Auto-DPI 微米级缩放)", IDC_CHK_AUTO_DPI);
  chk_autostart_ = create_chk(L"开机自动启动 CrossPointer (当前用户 HKCU 独立便携接管)", IDC_CHK_AUTOSTART);
  chk_auto_check_update_ = create_chk(L"启动时自动静默检查新版本更新 (Silent Update Check)", IDC_CHK_AUTO_UPDATE);
  btn_check_update_ = create_btn(L"🔍 立即检查新版本...", IDC_BTN_CHECK_UPDATE);

  // 底部统一动作栏 (始终可见)
  btn_save_ = CreateWindowExW(0, L"BUTTON", L"💾 保存并应用",
                              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                              0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_BTN_SAVE)),
                              instance_, nullptr);
  btn_defaults_ = CreateWindowExW(0, L"BUTTON", L"🔄 恢复默认",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                  0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_BTN_DEFAULTS)),
                                  instance_, nullptr);
  btn_close_ = CreateWindowExW(0, L"BUTTON", L"关闭",
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                               0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_BTN_CLOSE)),
                               instance_, nullptr);
}

void SettingsWindow::LayoutControls(int dpi) {
  if (font_title_ != nullptr) DeleteObject(font_title_);
  if (font_card_title_ != nullptr) DeleteObject(font_card_title_);
  if (font_bold_ != nullptr) DeleteObject(font_bold_);
  if (font_regular_ != nullptr) DeleteObject(font_regular_);
  if (font_small_ != nullptr) DeleteObject(font_small_);

  font_title_ = CreateModernFont(13, FW_BOLD, dpi);
  font_card_title_ = CreateModernFont(11, FW_BOLD, dpi);
  font_bold_ = CreateModernFont(9, FW_SEMIBOLD, dpi);
  font_regular_ = CreateModernFont(9, FW_NORMAL, dpi);
  font_small_ = CreateModernFont(8, FW_NORMAL, dpi);

  auto set_font = [&](HWND h, HFONT f) {
    if (h != nullptr) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
  };

  for (HWND h : {chk_auto_theme_, chk_show_on_startup_, chk_auto_dpi_, chk_autostart_,
                 chk_auto_check_update_, btn_check_update_,
                 chk_toggle_ctrl_, chk_toggle_alt_, chk_toggle_win_, chk_toggle_shift_,
                 chk_exit_ctrl_, chk_exit_alt_, chk_exit_win_, chk_exit_shift_,
                 combo_toggle_vk_, combo_exit_vk_, btn_sync_hotkey_,
                 btn_color_light_, btn_color_dark_,
                 btn_defaults_, btn_close_}) {
    set_font(h, font_regular_);
  }
  set_font(btn_save_, font_bold_);
  set_font(btn_sync_hotkey_, font_bold_);
  for (HWND h : {lbl_gap_val_, lbl_h_width_val_, lbl_v_width_val_,
                 lbl_alpha_light_val_, lbl_alpha_dark_val_}) {
    set_font(h, font_bold_);
  }

  const int pad_x = Scale(24, dpi);
  const int card_w = Scale(572, dpi);
  const int inner_x = pad_x + Scale(20, dpi);

  // Tab 0: 外观尺寸
  SetWindowPos(slider_gap_, nullptr, inner_x + Scale(260, dpi), Scale(196, dpi), Scale(180, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_gap_val_, nullptr, inner_x + Scale(448, dpi), Scale(198, dpi), Scale(55, dpi), Scale(20, dpi), SWP_NOZORDER);

  SetWindowPos(slider_h_width_, nullptr, inner_x + Scale(260, dpi), Scale(261, dpi), Scale(180, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_h_width_val_, nullptr, inner_x + Scale(448, dpi), Scale(263, dpi), Scale(55, dpi), Scale(20, dpi), SWP_NOZORDER);

  SetWindowPos(slider_v_width_, nullptr, inner_x + Scale(260, dpi), Scale(326, dpi), Scale(180, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_v_width_val_, nullptr, inner_x + Scale(448, dpi), Scale(328, dpi), Scale(55, dpi), Scale(20, dpi), SWP_NOZORDER);

  SetWindowPos(preview_crosshair_, nullptr, inner_x + Scale(260, dpi), Scale(390, dpi), Scale(260, dpi), Scale(64, dpi), SWP_NOZORDER);

  // Tab 1: 色彩模式
  SetWindowPos(chk_auto_theme_, nullptr, inner_x, Scale(192, dpi), card_w - Scale(40, dpi), Scale(24, dpi), SWP_NOZORDER);

  SetWindowPos(preview_light_, nullptr, inner_x + Scale(260, dpi), Scale(246, dpi), Scale(36, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(btn_color_light_, nullptr, inner_x + Scale(304, dpi), Scale(245, dpi), Scale(110, dpi), Scale(26, dpi), SWP_NOZORDER);
  SetWindowPos(slider_alpha_light_, nullptr, inner_x + Scale(422, dpi), Scale(246, dpi), Scale(90, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_alpha_light_val_, nullptr, inner_x + Scale(518, dpi), Scale(248, dpi), Scale(30, dpi), Scale(20, dpi), SWP_NOZORDER);

  SetWindowPos(preview_dark_, nullptr, inner_x + Scale(260, dpi), Scale(312, dpi), Scale(36, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(btn_color_dark_, nullptr, inner_x + Scale(304, dpi), Scale(311, dpi), Scale(110, dpi), Scale(26, dpi), SWP_NOZORDER);
  SetWindowPos(slider_alpha_dark_, nullptr, inner_x + Scale(422, dpi), Scale(312, dpi), Scale(90, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_alpha_dark_val_, nullptr, inner_x + Scale(518, dpi), Scale(314, dpi), Scale(30, dpi), Scale(20, dpi), SWP_NOZORDER);

  // Tab 2: 快捷键 (按键顺序无关，单键复用显隐退出)
  const int k_x = inner_x + Scale(220, dpi);
  SetWindowPos(chk_toggle_ctrl_, nullptr, k_x, Scale(196, dpi), Scale(52, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_toggle_alt_, nullptr, k_x + Scale(54, dpi), Scale(196, dpi), Scale(50, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_toggle_win_, nullptr, k_x + Scale(106, dpi), Scale(196, dpi), Scale(52, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_toggle_shift_, nullptr, k_x + Scale(160, dpi), Scale(196, dpi), Scale(58, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(combo_toggle_vk_, nullptr, k_x + Scale(224, dpi), Scale(194, dpi), Scale(84, dpi), Scale(200, dpi), SWP_NOZORDER);

  SetWindowPos(chk_exit_ctrl_, nullptr, k_x, Scale(260, dpi), Scale(52, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_exit_alt_, nullptr, k_x + Scale(54, dpi), Scale(260, dpi), Scale(50, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_exit_win_, nullptr, k_x + Scale(106, dpi), Scale(260, dpi), Scale(52, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_exit_shift_, nullptr, k_x + Scale(160, dpi), Scale(260, dpi), Scale(58, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(combo_exit_vk_, nullptr, k_x + Scale(224, dpi), Scale(258, dpi), Scale(84, dpi), Scale(200, dpi), SWP_NOZORDER);

  SetWindowPos(btn_sync_hotkey_, nullptr, inner_x + Scale(220, dpi), Scale(292, dpi), Scale(288, dpi), Scale(28, dpi), SWP_NOZORDER);

  // Tab 3: 系统与更新
  SetWindowPos(chk_show_on_startup_, nullptr, inner_x, Scale(186, dpi), card_w - Scale(40, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_auto_dpi_, nullptr, inner_x, Scale(242, dpi), card_w - Scale(40, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_autostart_, nullptr, inner_x, Scale(298, dpi), card_w - Scale(40, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(chk_auto_check_update_, nullptr, inner_x, Scale(354, dpi), card_w - Scale(40, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(btn_check_update_, nullptr, inner_x, Scale(414, dpi), Scale(160, dpi), Scale(34, dpi), SWP_NOZORDER);

  // 底部统一动作栏 (Y = 488px)
  const int btn_y = Scale(488, dpi);
  SetWindowPos(btn_save_, nullptr, pad_x, btn_y, Scale(140, dpi), Scale(38, dpi), SWP_NOZORDER);
  SetWindowPos(btn_defaults_, nullptr, pad_x + Scale(155, dpi), btn_y, Scale(115, dpi), Scale(38, dpi), SWP_NOZORDER);
  SetWindowPos(btn_close_, nullptr, pad_x + card_w - Scale(90, dpi), btn_y, Scale(90, dpi), Scale(38, dpi), SWP_NOZORDER);
}

void SettingsWindow::UpdateTabVisibility() {
  auto show_ctrls = [&](const std::vector<HWND>& ctrls, bool show) {
    for (HWND h : ctrls) {
      if (h != nullptr) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
    }
  };

  show_ctrls({slider_gap_, lbl_gap_val_, slider_h_width_, lbl_h_width_val_,
              slider_v_width_, lbl_v_width_val_, preview_crosshair_},
             current_tab_ == 0);

  show_ctrls({chk_auto_theme_, preview_light_, btn_color_light_,
              slider_alpha_light_, lbl_alpha_light_val_, preview_dark_,
              btn_color_dark_, slider_alpha_dark_, lbl_alpha_dark_val_},
             current_tab_ == 1);

  show_ctrls({chk_toggle_ctrl_, chk_toggle_alt_, chk_toggle_win_,
              chk_toggle_shift_, combo_toggle_vk_, chk_exit_ctrl_,
              chk_exit_alt_, chk_exit_win_, chk_exit_shift_, combo_exit_vk_,
              btn_sync_hotkey_},
             current_tab_ == 2);

  show_ctrls({chk_show_on_startup_, chk_auto_dpi_, chk_autostart_, chk_auto_check_update_,
              btn_check_update_},
             current_tab_ == 3);
}

void SettingsWindow::SwitchTab(int new_tab) {
  if (new_tab < 0 || new_tab > 3 || new_tab == current_tab_) return;
  current_tab_ = new_tab;
  UpdateTabVisibility();
  InvalidateRect(hwnd_, nullptr, TRUE);
}

void SettingsWindow::LoadValuesToUI() {
  // Gap & Width
  SendMessageW(slider_gap_, TBM_SETPOS, TRUE, config_.gap);
  SetWindowTextW(lbl_gap_val_, (std::to_wstring(config_.gap) + L" px").c_str());

  SendMessageW(slider_h_width_, TBM_SETPOS, TRUE, config_.horizontal.width);
  SetWindowTextW(lbl_h_width_val_, (std::to_wstring(config_.horizontal.width) + L" px").c_str());

  SendMessageW(slider_v_width_, TBM_SETPOS, TRUE, config_.vertical.width);
  SetWindowTextW(lbl_v_width_val_, (std::to_wstring(config_.vertical.width) + L" px").c_str());

  // Themes & Colors
  Button_SetCheck(chk_auto_theme_, config_.auto_theme ? BST_CHECKED : BST_UNCHECKED);
  SendMessageW(slider_alpha_light_, TBM_SETPOS, TRUE, temp_light_color_.alpha.value);
  SetWindowTextW(lbl_alpha_light_val_, std::to_wstring(temp_light_color_.alpha.value).c_str());

  SendMessageW(slider_alpha_dark_, TBM_SETPOS, TRUE, temp_dark_color_.alpha.value);
  SetWindowTextW(lbl_alpha_dark_val_, std::to_wstring(temp_dark_color_.alpha.value).c_str());

  // Hotkeys - Toggle
  Button_SetCheck(chk_toggle_ctrl_, (config_.hotkey_h_s.mod & MOD_CONTROL) ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_toggle_alt_, (config_.hotkey_h_s.mod & MOD_ALT) ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_toggle_win_, (config_.hotkey_h_s.mod & MOD_WIN) ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_toggle_shift_, (config_.hotkey_h_s.mod & MOD_SHIFT) ? BST_CHECKED : BST_UNCHECKED);

  const auto& keys = GetKeyList();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i].second == config_.hotkey_h_s.vk) {
      SendMessageW(combo_toggle_vk_, CB_SETCURSEL, i, 0);
      break;
    }
  }

  // Hotkeys - Exit
  Button_SetCheck(chk_exit_ctrl_, (config_.hotkey_exit.mod & MOD_CONTROL) ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_exit_alt_, (config_.hotkey_exit.mod & MOD_ALT) ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_exit_win_, (config_.hotkey_exit.mod & MOD_WIN) ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_exit_shift_, (config_.hotkey_exit.mod & MOD_SHIFT) ? BST_CHECKED : BST_UNCHECKED);

  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i].second == config_.hotkey_exit.vk) {
      SendMessageW(combo_exit_vk_, CB_SETCURSEL, i, 0);
      break;
    }
  }

  // System
  Button_SetCheck(chk_show_on_startup_, config_.show_on_startup ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_auto_dpi_, config_.auto_dpi ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_autostart_, autostart::IsAutoStartEnabled() ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_auto_check_update_, config_.auto_check_update ? BST_CHECKED : BST_UNCHECKED);

  InvalidateRect(preview_light_, nullptr, TRUE);
  InvalidateRect(preview_dark_, nullptr, TRUE);
  InvalidateRect(preview_crosshair_, nullptr, TRUE);
}

bool SettingsWindow::SaveValuesFromUI() {
  Config updated = config_;

  // Gap & Width
  updated.gap = static_cast<unsigned int>(SendMessageW(slider_gap_, TBM_GETPOS, 0, 0));
  updated.horizontal.width = static_cast<unsigned int>(SendMessageW(slider_h_width_, TBM_GETPOS, 0, 0));
  updated.vertical.width = static_cast<unsigned int>(SendMessageW(slider_v_width_, TBM_GETPOS, 0, 0));

  // Themes & Colors
  updated.auto_theme = Button_GetCheck(chk_auto_theme_) == BST_CHECKED;
  temp_light_color_.alpha = ColorChannel(static_cast<int>(SendMessageW(slider_alpha_light_, TBM_GETPOS, 0, 0)));
  temp_dark_color_.alpha = ColorChannel(static_cast<int>(SendMessageW(slider_alpha_dark_, TBM_GETPOS, 0, 0)));
  updated.light_theme = temp_light_color_;
  updated.dark_theme = temp_dark_color_;

  // Hotkey - Toggle
  unsigned int t_mod = 0;
  if (Button_GetCheck(chk_toggle_ctrl_) == BST_CHECKED) t_mod |= MOD_CONTROL;
  if (Button_GetCheck(chk_toggle_alt_) == BST_CHECKED) t_mod |= MOD_ALT;
  if (Button_GetCheck(chk_toggle_win_) == BST_CHECKED) t_mod |= MOD_WIN;
  if (Button_GetCheck(chk_toggle_shift_) == BST_CHECKED) t_mod |= MOD_SHIFT;
  const int t_idx = static_cast<int>(SendMessageW(combo_toggle_vk_, CB_GETCURSEL, 0, 0));
  const unsigned int t_vk = (t_idx >= 0 && t_idx < static_cast<int>(GetKeyList().size())) ? GetKeyList()[t_idx].second : 'E';

  // Hotkey - Exit
  unsigned int e_mod = 0;
  if (Button_GetCheck(chk_exit_ctrl_) == BST_CHECKED) e_mod |= MOD_CONTROL;
  if (Button_GetCheck(chk_exit_alt_) == BST_CHECKED) e_mod |= MOD_ALT;
  if (Button_GetCheck(chk_exit_win_) == BST_CHECKED) e_mod |= MOD_WIN;
  if (Button_GetCheck(chk_exit_shift_) == BST_CHECKED) e_mod |= MOD_SHIFT;
  const int e_idx = static_cast<int>(SendMessageW(combo_exit_vk_, CB_GETCURSEL, 0, 0));
  const unsigned int e_vk = (e_idx >= 0 && e_idx < static_cast<int>(GetKeyList().size())) ? GetKeyList()[e_idx].second : 'Q';

  updated.hotkey_h_s.mod = t_mod;
  updated.hotkey_h_s.vk = t_vk;
  updated.hotkey_exit.mod = e_mod;
  updated.hotkey_exit.vk = e_vk;
  updated.ClampAll();

  // System
  updated.show_on_startup = Button_GetCheck(chk_show_on_startup_) == BST_CHECKED;
  updated.auto_dpi = Button_GetCheck(chk_auto_dpi_) == BST_CHECKED;
  updated.auto_check_update = Button_GetCheck(chk_auto_check_update_) == BST_CHECKED;
  const bool enable_autostart = Button_GetCheck(chk_autostart_) == BST_CHECKED;
  (void)autostart::SetAutoStart(enable_autostart);

  // 保存至 INI 文件
  if (!updated.Save(config_path_.c_str())) {
    MessageBoxW(hwnd_, L"无法写入 CrossPointer.ini 配置文件，请检查写入权限。", L"错误", MB_OK | MB_ICONERROR);
    return false;
  }

  // 应用到运行期
  config_ = updated;
  if (on_apply_) {
    if (!on_apply_(config_)) {
      MessageBoxW(hwnd_, L"新热键已被系统其他程序占用，请尝试更换其他组合键。", L"警告", MB_OK | MB_ICONWARNING);
      return false;
    }
  }

  const bool is_same_hotkey = (t_mod == e_mod && t_vk == e_vk);
  if (is_same_hotkey) {
    MessageBoxW(hwnd_,
                L"偏好设置已成功保存并即刻生效！\n\n"
                L"✨ 已启用单键智能复用模式：\n"
                L"• 十字线未显示时按键：即刻显示十字线并重载配置\n"
                L"• 十字线已显示时按键：即刻安全退出程序\n"
                L"（无需记忆两个按键，任意修饰键按下顺序均可无缝触发）",
                L"提示", MB_OK | MB_ICONINFORMATION);
  } else {
    MessageBoxW(hwnd_, L"偏好设置已成功保存并即刻生效！", L"提示", MB_OK | MB_ICONINFORMATION);
  }
  return true;
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) noexcept {
  SettingsWindow* self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lp);
    self = static_cast<SettingsWindow*>(create->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    return TRUE;
  }
  if (self != nullptr) {
    return self->HandleMessage(hwnd, msg, wp, lp);
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT SettingsWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_SETTINGCHANGE: {
      UpdateThemeStyles();
      InvalidateRect(hwnd, nullptr, TRUE);
      break;
    }
    case WM_DPICHANGED: {
      const int dpi = LOWORD(wp);
      const auto* prc = reinterpret_cast<const RECT*>(lp);
      SetWindowPos(hwnd, nullptr, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, SWP_NOZORDER | SWP_NOACTIVATE);
      LayoutControls(dpi);
      InvalidateRect(hwnd, nullptr, TRUE);
      return 0;
    }
    case WM_LBUTTONDOWN: {
      const int dpi = GetWindowDpi(hwnd);
      const int mouse_x = GET_X_LPARAM(lp);
      const int mouse_y = GET_Y_LPARAM(lp);

      const int seg_x = Scale(24, dpi);
      const int seg_y = Scale(82, dpi);
      const int seg_w = Scale(572, dpi);
      const int seg_h = Scale(34, dpi);

      if (mouse_x >= seg_x && mouse_x <= seg_x + seg_w &&
          mouse_y >= seg_y && mouse_y <= seg_y + seg_h) {
        const int tab_w = seg_w / 4;
        const int clicked_tab = (mouse_x - seg_x) / tab_w;
        if (clicked_tab >= 0 && clicked_tab < 4) {
          SwitchTab(clicked_tab);
        }
        return 0;
      }
      break;
    }
    case WM_HSCROLL: {
      // 实时更新数值标签并刷新预览
      if (reinterpret_cast<HWND>(lp) == slider_gap_) {
        const int val = static_cast<int>(SendMessageW(slider_gap_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_gap_val_, (std::to_wstring(val) + L" px").c_str());
        InvalidateRect(preview_crosshair_, nullptr, TRUE);
      } else if (reinterpret_cast<HWND>(lp) == slider_h_width_) {
        const int val = static_cast<int>(SendMessageW(slider_h_width_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_h_width_val_, (std::to_wstring(val) + L" px").c_str());
        InvalidateRect(preview_crosshair_, nullptr, TRUE);
      } else if (reinterpret_cast<HWND>(lp) == slider_v_width_) {
        const int val = static_cast<int>(SendMessageW(slider_v_width_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_v_width_val_, (std::to_wstring(val) + L" px").c_str());
        InvalidateRect(preview_crosshair_, nullptr, TRUE);
      } else if (reinterpret_cast<HWND>(lp) == slider_alpha_light_) {
        const int val = static_cast<int>(SendMessageW(slider_alpha_light_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_alpha_light_val_, std::to_wstring(val).c_str());
        temp_light_color_.alpha = ColorChannel(val);
        InvalidateRect(preview_light_, nullptr, TRUE);
      } else if (reinterpret_cast<HWND>(lp) == slider_alpha_dark_) {
        const int val = static_cast<int>(SendMessageW(slider_alpha_dark_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_alpha_dark_val_, std::to_wstring(val).c_str());
        temp_dark_color_.alpha = ColorChannel(val);
        InvalidateRect(preview_dark_, nullptr, TRUE);
      }
      return 0;
    }
    case WM_COMMAND: {
      const UINT id = LOWORD(wp);
      if (id == IDC_BTN_SYNC_HOTKEY) {
        // 一键将退出热键同步为显隐热键 (单键复用)
        Button_SetCheck(chk_exit_ctrl_, Button_GetCheck(chk_toggle_ctrl_));
        Button_SetCheck(chk_exit_alt_, Button_GetCheck(chk_toggle_alt_));
        Button_SetCheck(chk_exit_win_, Button_GetCheck(chk_toggle_win_));
        Button_SetCheck(chk_exit_shift_, Button_GetCheck(chk_toggle_shift_));
        const LRESULT cur = SendMessageW(combo_toggle_vk_, CB_GETCURSEL, 0, 0);
        SendMessageW(combo_exit_vk_, CB_SETCURSEL, cur, 0);
        MessageBoxW(hwnd_,
                    L"已成功将退出热键同步为与显隐热键完全一致！\n\n"
                    L"点击「保存并应用」后，只需按这一个快捷键：\n"
                    L"• 十字线隐藏时：按下显示十字线\n"
                    L"• 十字线显示时：按下退出程序",
                    L"快捷键同步提示", MB_OK | MB_ICONINFORMATION);
        return 0;
      }
      if (id == IDC_BTN_CHECK_UPDATE) {
        updater::CheckUpdateAsync(hwnd_, /*silent_if_latest=*/false);
        return 0;
      }
      if (id == IDC_BTN_SAVE) {
        SaveValuesFromUI();
        return 0;
      }
      if (id == IDC_BTN_CLOSE) {
        Close();
        return 0;
      }
      if (id == IDC_BTN_DEFAULTS) {
        config_ = Config(); // 恢复默认结构体
        temp_light_color_ = config_.light_theme;
        temp_dark_color_ = config_.dark_theme;
        LoadValuesToUI();
        return 0;
      }
      if (id == IDC_BTN_COLOR_LIGHT) {
        static COLORREF custom_colors[16] = {0};
        CHOOSECOLORW cc = {sizeof(cc)};
        cc.hwndOwner = hwnd;
        cc.lpCustColors = custom_colors;
        cc.rgbResult = RGB(temp_light_color_.r.value, temp_light_color_.g.value, temp_light_color_.b.value);
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColorW(&cc)) {
          temp_light_color_.r = ColorChannel(GetRValue(cc.rgbResult));
          temp_light_color_.g = ColorChannel(GetGValue(cc.rgbResult));
          temp_light_color_.b = ColorChannel(GetBValue(cc.rgbResult));
          InvalidateRect(preview_light_, nullptr, TRUE);
        }
        return 0;
      }
      if (id == IDC_BTN_COLOR_DARK) {
        static COLORREF custom_colors[16] = {0};
        CHOOSECOLORW cc = {sizeof(cc)};
        cc.hwndOwner = hwnd;
        cc.lpCustColors = custom_colors;
        cc.rgbResult = RGB(temp_dark_color_.r.value, temp_dark_color_.g.value, temp_dark_color_.b.value);
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColorW(&cc)) {
          temp_dark_color_.r = ColorChannel(GetRValue(cc.rgbResult));
          temp_dark_color_.g = ColorChannel(GetGValue(cc.rgbResult));
          temp_dark_color_.b = ColorChannel(GetBValue(cc.rgbResult));
          InvalidateRect(preview_dark_, nullptr, TRUE);
        }
        return 0;
      }
      break;
    }
    case WM_DRAWITEM: {
      const auto* dis = reinterpret_cast<const DRAWITEMSTRUCT*>(lp);
      if (dis->CtlID == IDC_PREVIEW_LIGHT || dis->CtlID == IDC_PREVIEW_DARK) {
        const LineColor& col = (dis->CtlID == IDC_PREVIEW_LIGHT) ? temp_light_color_ : temp_dark_color_;
        HBRUSH br = CreateSolidBrush(RGB(col.r.value, col.g.value, col.b.value));
        HPEN pen = CreatePen(PS_SOLID, 1, cr_card_border_);
        HBRUSH old_br = SelectBrush(dis->hDC, br);
        HPEN old_pen = SelectPen(dis->hDC, pen);
        RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 6, 6);
        SelectBrush(dis->hDC, old_br);
        SelectPen(dis->hDC, old_pen);
        DeleteObject(br);
        DeleteObject(pen);
        return TRUE;
      }
      if (dis->CtlID == IDC_PREVIEW_CROSSHAIR) {
        // 实时尺寸十字线预览
        HBRUSH bg_br = CreateSolidBrush(is_dark_mode_ ? RGB(20, 22, 26) : RGB(236, 238, 242));
        HPEN border_pen = CreatePen(PS_SOLID, 1, cr_card_border_);
        FillRect(dis->hDC, &dis->rcItem, bg_br);
        FrameRect(dis->hDC, &dis->rcItem, bg_br);

        const int gap = static_cast<int>(SendMessageW(slider_gap_, TBM_GETPOS, 0, 0));
        const int h_w = static_cast<int>(SendMessageW(slider_h_width_, TBM_GETPOS, 0, 0));
        const int v_w = static_cast<int>(SendMessageW(slider_v_width_, TBM_GETPOS, 0, 0));

        const int cx = (dis->rcItem.left + dis->rcItem.right) / 2;
        const int cy = (dis->rcItem.top + dis->rcItem.bottom) / 2;

        COLORREF cross_color = is_dark_mode_ ? RGB(0, 225, 255) : RGB(0, 140, 255);
        HBRUSH cross_br = CreateSolidBrush(cross_color);

        // 水平左臂与右臂
        const int half_gap = gap / 2;
        RECT rc_hl = {dis->rcItem.left + 10, cy - h_w / 2, cx - half_gap, cy - h_w / 2 + std::max(1, h_w)};
        RECT rc_hr = {cx + half_gap, cy - h_w / 2, dis->rcItem.right - 10, cy - h_w / 2 + std::max(1, h_w)};
        if (rc_hl.right > rc_hl.left) FillRect(dis->hDC, &rc_hl, cross_br);
        if (rc_hr.right > rc_hr.left) FillRect(dis->hDC, &rc_hr, cross_br);

        // 垂直上臂与下臂
        RECT rc_vt = {cx - v_w / 2, dis->rcItem.top + 6, cx - v_w / 2 + std::max(1, v_w), cy - half_gap};
        RECT rc_vb = {cx - v_w / 2, cy + half_gap, cx - v_w / 2 + std::max(1, v_w), dis->rcItem.bottom - 6};
        if (rc_vt.bottom > rc_vt.top) FillRect(dis->hDC, &rc_vt, cross_br);
        if (rc_vb.bottom > rc_vb.top) FillRect(dis->hDC, &rc_vb, cross_br);

        DeleteObject(bg_br);
        DeleteObject(border_pen);
        DeleteObject(cross_br);
        return TRUE;
      }
      break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
      HDC hdc = reinterpret_cast<HDC>(wp);
      SetBkMode(hdc, TRANSPARENT);
      SetTextColor(hdc, cr_text_);
      return reinterpret_cast<LRESULT>(br_card_);
    }
    case WM_PAINT: {
      PAINTSTRUCT ps = {};
      HDC hdc = BeginPaint(hwnd, &ps);
      const int dpi = GetWindowDpi(hwnd);
      RECT client = {};
      GetClientRect(hwnd, &client);

      // 1. 全局背景
      FillRect(hdc, &client, br_bg_);

      // 2. Hero Header (Top 70px)
      const int icon_size = Scale(44, dpi);
      const int icon_x = Scale(24, dpi);
      const int icon_y = Scale(16, dpi);
      const int icon_r = Scale(10, dpi);

      HBRUSH icon_br = CreateSolidBrush(is_dark_mode_ ? RGB(32, 40, 52) : RGB(215, 225, 240));
      HPEN icon_pen = CreatePen(PS_SOLID, 1, is_dark_mode_ ? RGB(70, 85, 110) : RGB(180, 195, 215));
      HBRUSH old_br = SelectBrush(hdc, icon_br);
      HPEN old_pen = SelectPen(hdc, icon_pen);
      RoundRect(hdc, icon_x, icon_y, icon_x + icon_size, icon_y + icon_size, icon_r, icon_r);

      // 图标内部激光十字微缩图
      HPEN cyan_pen = CreatePen(PS_SOLID, std::max(2, Scale(3, dpi)), RGB(0, 225, 255));
      SelectPen(hdc, cyan_pen);
      const int icx = icon_x + icon_size / 2;
      const int icy = icon_y + icon_size / 2;
      MoveToEx(hdc, icx - Scale(14, dpi), icy, nullptr);
      LineTo(hdc, icx - Scale(5, dpi), icy);
      MoveToEx(hdc, icx + Scale(5, dpi), icy, nullptr);
      LineTo(hdc, icx + Scale(14, dpi), icy);
      MoveToEx(hdc, icx, icy - Scale(14, dpi), nullptr);
      LineTo(hdc, icx, icy - Scale(5, dpi));
      MoveToEx(hdc, icx, icy + Scale(5, dpi), nullptr);
      LineTo(hdc, icx, icy + Scale(14, dpi));
      DeleteObject(cyan_pen);

      SelectBrush(hdc, old_br);
      SelectPen(hdc, old_pen);
      DeleteObject(icon_br);
      DeleteObject(icon_pen);

      // 标题与副标题
      SetBkMode(hdc, TRANSPARENT);
      SelectFont(hdc, font_title_);
      SetTextColor(hdc, cr_text_);
      TextOutW(hdc, icon_x + icon_size + Scale(14, dpi), icon_y + Scale(2, dpi), L"CrossPointer 偏好设置", 14);

      SelectFont(hdc, font_small_);
      SetTextColor(hdc, cr_secondary_);
      TextOutW(hdc, icon_x + icon_size + Scale(14, dpi), icon_y + Scale(25, dpi), L"v1.3 • macOS 26 极简屏幕十字辅助线 • 纯绿色便携", 28);

      // 右侧运行状态胶囊 Badge
      const int badge_w = Scale(98, dpi);
      const int badge_h = Scale(24, dpi);
      const int badge_x = client.right - Scale(24, dpi) - badge_w;
      const int badge_y = icon_y + Scale(9, dpi);
      HBRUSH badge_br = CreateSolidBrush(is_dark_mode_ ? RGB(28, 44, 36) : RGB(232, 248, 238));
      HPEN badge_pen = CreatePen(PS_SOLID, 1, RGB(52, 199, 89));
      SelectBrush(hdc, badge_br);
      SelectPen(hdc, badge_pen);
      RoundRect(hdc, badge_x, badge_y, badge_x + badge_w, badge_y + badge_h, badge_h, badge_h);
      SelectBrush(hdc, old_br);
      SelectPen(hdc, old_pen);
      DeleteObject(badge_br);
      DeleteObject(badge_pen);

      SelectFont(hdc, font_small_);
      SetTextColor(hdc, RGB(52, 199, 89));
      TextOutW(hdc, badge_x + Scale(11, dpi), badge_y + Scale(4, dpi), L"● 运行中 (1ms)", 10);

      // Header 分割线
      HPEN div_pen = CreatePen(PS_SOLID, 1, cr_divider_);
      SelectPen(hdc, div_pen);
      MoveToEx(hdc, Scale(24, dpi), Scale(70, dpi), nullptr);
      LineTo(hdc, client.right - Scale(24, dpi), Scale(70, dpi));

      // 3. Segmented Control Capsule Tab Bar (Y: 82px, Height: 34px)
      const int seg_x = Scale(24, dpi);
      const int seg_y = Scale(82, dpi);
      const int seg_w = Scale(572, dpi);
      const int seg_h = Scale(34, dpi);
      const int seg_r = Scale(8, dpi);

      SelectBrush(hdc, br_seg_bg_);
      HPEN null_pen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
      SelectPen(hdc, null_pen);
      RoundRect(hdc, seg_x, seg_y, seg_x + seg_w, seg_y + seg_h, seg_r, seg_r);
      DeleteObject(null_pen);

      const wchar_t* tab_titles[4] = {L"外观尺寸", L"色彩模式", L"全局快捷键", L"系统与更新"};
      const int tab_item_w = seg_w / 4;

      for (int i = 0; i < 4; i++) {
        const int cur_tx = seg_x + i * tab_item_w + Scale(2, dpi);
        const int cur_ty = seg_y + Scale(2, dpi);
        const int cur_tw = tab_item_w - Scale(4, dpi);
        const int cur_th = seg_h - Scale(4, dpi);

        if (i == current_tab_) {
          SelectBrush(hdc, br_tab_active_);
          HPEN active_pen = CreatePen(PS_SOLID, 1, cr_accent_);
          SelectPen(hdc, active_pen);
          RoundRect(hdc, cur_tx, cur_ty, cur_tx + cur_tw, cur_ty + cur_th, Scale(6, dpi), Scale(6, dpi));
          DeleteObject(active_pen);

          SelectFont(hdc, font_bold_);
          SetTextColor(hdc, RGB(255, 255, 255));
        } else {
          SelectFont(hdc, font_regular_);
          SetTextColor(hdc, cr_secondary_);
        }

        RECT trc = {cur_tx, cur_ty, cur_tx + cur_tw, cur_ty + cur_th};
        DrawTextW(hdc, tab_titles[i], -1, &trc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      }

      // 4. Main Inset Grouped Card (Y: 126px, Height: 348px)
      const int card_x = Scale(24, dpi);
      const int card_y = Scale(126, dpi);
      const int card_w = Scale(572, dpi);
      const int card_h = Scale(348, dpi);
      const int card_r = Scale(12, dpi);

      SelectBrush(hdc, br_card_);
      HPEN card_pen = CreatePen(PS_SOLID, 1, cr_card_border_);
      SelectPen(hdc, card_pen);
      RoundRect(hdc, card_x, card_y, card_x + card_w, card_y + card_h, card_r, card_r);
      SelectBrush(hdc, old_br);
      SelectPen(hdc, old_pen);
      DeleteObject(card_pen);

      // Card 内部文字与排版
      const int inner_x = card_x + Scale(20, dpi);
      const int inner_w = card_w - Scale(40, dpi);

      SelectPen(hdc, div_pen);

      if (current_tab_ == 0) {
        // TAB 0: 外观尺寸
        SelectFont(hdc, font_card_title_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(18, dpi), L"屏幕十字线外观与物理尺寸", 12);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(40, dpi), L"针对高刷新率与高 PPI 屏幕调优，调整滑块即时生效", 24);

        // Row 1: Gap
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(70, dpi), L"中心空隙留白 (Gap)", 12);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(90, dpi), L"中心留白距离，0 为十字线无间隙直接相交，推荐 16 px", 28);
        MoveToEx(hdc, inner_x, card_y + Scale(122, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(122, dpi));

        // Row 2: Horizontal Width
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(135, dpi), L"水平横线粗细 (Width)", 14);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(155, dpi), L"水平横向贯穿辅线的笔画宽度，推荐 2 px", 20);
        MoveToEx(hdc, inner_x, card_y + Scale(187, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(187, dpi));

        // Row 3: Vertical Width
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(200, dpi), L"垂直竖线粗细 (Width)", 14);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(220, dpi), L"垂直纵向贯穿辅线的笔画宽度，推荐 2 px", 20);
        MoveToEx(hdc, inner_x, card_y + Scale(252, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(252, dpi));

        // Row 4: Real-time Preview Box
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(266, dpi), L"实时十字线尺寸预览效果", 11);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(288, dpi), L"即时联动当前水平粗细、垂直粗细与中心留白", 20);
      } else if (current_tab_ == 1) {
        // TAB 1: 色彩模式
        SelectFont(hdc, font_card_title_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(18, dpi), L"主题自适应色彩与不透明度", 12);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(40, dpi), L"支持浅色与深色双调色盘，跟随 Windows 模式无缝自动切换", 27);

        MoveToEx(hdc, inner_x, card_y + Scale(100, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(100, dpi));

        // Section 1: 浅色模式
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(116, dpi), L"浅色模式专属色彩", 8);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(138, dpi), L"用于浅色桌面或浅色办公文档背景下的高对比色彩", 22);

        MoveToEx(hdc, inner_x, card_y + Scale(172, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(172, dpi));

        // Section 2: 深色模式
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(186, dpi), L"深色模式专属色彩", 8);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(208, dpi), L"用于深色 IDE、暗黑终端或夜间环境下的舒适荧光青蓝色彩", 26);

        // Section 3: Inset 技巧卡片
        const int tip_y = card_y + Scale(242, dpi);
        const int tip_h = Scale(88, dpi);
        RECT tip_rc = {inner_x, tip_y, inner_x + inner_w, tip_y + tip_h};
        HBRUSH tip_br = CreateSolidBrush(is_dark_mode_ ? RGB(28, 32, 40) : RGB(244, 246, 250));
        HPEN tip_pen = CreatePen(PS_SOLID, 1, is_dark_mode_ ? RGB(48, 56, 70) : RGB(220, 225, 235));
        SelectBrush(hdc, tip_br);
        SelectPen(hdc, tip_pen);
        RoundRect(hdc, tip_rc.left, tip_rc.top, tip_rc.right, tip_rc.bottom, Scale(8, dpi), Scale(8, dpi));
        SelectBrush(hdc, old_br);
        SelectPen(hdc, old_pen);
        DeleteObject(tip_br);
        DeleteObject(tip_pen);

        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_accent_);
        TextOutW(hdc, inner_x + Scale(14, dpi), tip_y + Scale(12, dpi), L"💡 调色技巧与专业建议", 11);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x + Scale(14, dpi), tip_y + Scale(34, dpi),
                 L"• 浅色模式推荐使用饱和度适中的深天蓝或石墨灰，不刺眼且易于审阅对齐。", 34);
        TextOutW(hdc, inner_x + Scale(14, dpi), tip_y + Scale(54, dpi),
                 L"• 深色模式推荐使用 macOS Azure 激光青（#00E1FF），配合 180 透明度带来极佳对齐感。", 42);
      } else if (current_tab_ == 2) {
        // TAB 2: 全局快捷键
        SelectFont(hdc, font_card_title_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(18, dpi), L"系统级全局快捷键与单键复用", 13);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(40, dpi), L"支持无缝按键顺序识别，以及同一快捷键复用兼具显示与退出", 26);

        // Row 1: 显隐
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(70, dpi), L"显隐十字线热键", 7);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(92, dpi), L"随时在所有窗口上显示或隐藏辅助线", 16);
        MoveToEx(hdc, inner_x, card_y + Scale(122, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(122, dpi));

        // Row 2: 退出
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(134, dpi), L"退出程序热键", 6);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(156, dpi), L"快捷退出 CrossPointer 进程，可设为与显隐相同", 22);

        // Section 3: Callout 升级说明卡片
        const int tip_y = card_y + Scale(204, dpi);
        const int tip_h = Scale(128, dpi);
        RECT tip_rc = {inner_x, tip_y, inner_x + inner_w, tip_y + tip_h};
        HBRUSH tip_br = CreateSolidBrush(is_dark_mode_ ? RGB(26, 36, 48) : RGB(236, 244, 255));
        HPEN tip_pen = CreatePen(PS_SOLID, 1, cr_accent_);
        SelectBrush(hdc, tip_br);
        SelectPen(hdc, tip_pen);
        RoundRect(hdc, tip_rc.left, tip_rc.top, tip_rc.right, tip_rc.bottom, Scale(8, dpi), Scale(8, dpi));
        SelectBrush(hdc, old_br);
        SelectPen(hdc, old_pen);
        DeleteObject(tip_br);
        DeleteObject(tip_pen);

        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, cr_accent_);
        TextOutW(hdc, inner_x + Scale(14, dpi), tip_y + Scale(10, dpi), L"✨ 全局快捷键两大核心升级 (v1.3 新增)", 20);

        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x + Scale(14, dpi), tip_y + Scale(32, dpi),
                 L"1. 单快捷键智能复用 (Show/Hide + Exit Toggle)：", 29);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x + Scale(28, dpi), tip_y + Scale(50, dpi),
                 L"当显隐与退出设为相同时：未显示时按键显示十字线；已显示时按键退出程序。", 35);
        TextOutW(hdc, inner_x + Scale(28, dpi), tip_y + Scale(68, dpi),
                 L"无需记忆多个快捷键，只需一键即可兼顾显示与退出！", 23);

        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x + Scale(14, dpi), tip_y + Scale(88, dpi),
                 L"2. 按键顺序无关（Order-Independent Modifiers）：", 29);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x + Scale(28, dpi), tip_y + Scale(106, dpi),
                 L"无论先按 Ctrl 还是先按 Alt/Win/Shift，只要修饰键处于按下状态均能 100% 精准触发。", 43);
      } else if (current_tab_ == 3) {
        // TAB 3: 系统与更新
        SelectFont(hdc, font_card_title_);
        SetTextColor(hdc, cr_text_);
        TextOutW(hdc, inner_x, card_y + Scale(16, dpi), L"系统整合、启动偏好与在线更新", 16);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x, card_y + Scale(36, dpi), L"100% 纯绿色便携软件设计，零注册表垃圾残留，零外部临时文件", 28);

        // Row 1: Show on Startup
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x + Scale(24, dpi), card_y + Scale(80, dpi),
                 L"勾选时打开软件即刻显示十字线；取消勾选则启动后静默常驻托盘，需按热键呼出", 37);
        MoveToEx(hdc, inner_x, card_y + Scale(108, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(108, dpi));

        // Row 2: High DPI & 8K
        TextOutW(hdc, inner_x + Scale(24, dpi), card_y + Scale(136, dpi),
                 L"内置 PerMonitorV2 顶级感知，光标跨屏时物理微米级动态缩放，彻底告别过细线条", 39);
        MoveToEx(hdc, inner_x, card_y + Scale(164, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(164, dpi));

        // Row 3: Autostart
        TextOutW(hdc, inner_x + Scale(24, dpi), card_y + Scale(192, dpi),
                 L"仅在当前用户注册表独立启动，纯便携绿色生效，取消勾选即刻彻底清除", 32);
        MoveToEx(hdc, inner_x, card_y + Scale(220, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(220, dpi));

        // Row 4: Update Check
        TextOutW(hdc, inner_x + Scale(24, dpi), card_y + Scale(248, dpi),
                 L"后台异步轻量查询 GitHub Releases，发现新版本时在托盘温和提示", 31);
        MoveToEx(hdc, inner_x, card_y + Scale(276, dpi), nullptr);
        LineTo(hdc, inner_x + inner_w, card_y + Scale(276, dpi));

        // Row 5: Manual Check Button & Info
        SelectFont(hdc, font_bold_);
        SetTextColor(hdc, RGB(52, 199, 89));
        TextOutW(hdc, inner_x + Scale(175, dpi), card_y + Scale(294, dpi),
                 L"当前版本: CrossPointer v1.3 • 已是最新正式版", 26);
        SelectFont(hdc, font_small_);
        SetTextColor(hdc, cr_secondary_);
        TextOutW(hdc, inner_x + Scale(175, dpi), card_y + Scale(314, dpi),
                 L"官方开源仓库: https://github.com/Crucialmer/CrossPointer", 32);
      }

      DeleteObject(div_pen);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_CLOSE: {
      Close();
      return 0;
    }
    case WM_DESTROY: {
      hwnd_ = nullptr;
      return 0;
    }
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}
