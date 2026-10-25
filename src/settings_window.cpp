#include "./settings_window.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <cmath>
#include <vector>

#include "./autostart.h"
#include "./config_file_util.h"

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

constexpr UINT IDC_CHK_AUTO_THEME = 3001;
constexpr UINT IDC_CHK_AUTO_DPI = 3002;
constexpr UINT IDC_CHK_AUTOSTART = 3003;

constexpr UINT IDC_SLIDER_GAP = 3010;
constexpr UINT IDC_SLIDER_H_WIDTH = 3011;
constexpr UINT IDC_SLIDER_V_WIDTH = 3012;

constexpr UINT IDC_BTN_COLOR_LIGHT = 3020;
constexpr UINT IDC_SLIDER_ALPHA_LIGHT = 3021;
constexpr UINT IDC_BTN_COLOR_DARK = 3022;
constexpr UINT IDC_SLIDER_ALPHA_DARK = 3023;

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

constexpr UINT IDC_BTN_SAVE = 3050;
constexpr UINT IDC_BTN_DEFAULTS = 3051;
constexpr UINT IDC_BTN_CLOSE = 3052;

constexpr UINT IDC_PREVIEW_LIGHT = 3060;
constexpr UINT IDC_PREVIEW_DARK = 3061;

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
                     L"Segoe UI Variable Text");
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
      {L"XButton1", VK_XBUTTON1}, {L"XButton2", VK_XBUTTON2},
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
  if (font_bold_ != nullptr) { DeleteObject(font_bold_); font_bold_ = nullptr; }
  if (font_regular_ != nullptr) { DeleteObject(font_regular_); font_regular_ = nullptr; }
  if (font_small_ != nullptr) { DeleteObject(font_small_); font_small_ = nullptr; }
  if (br_bg_ != nullptr) { DeleteObject(br_bg_); br_bg_ = nullptr; }
  if (br_card_ != nullptr) { DeleteObject(br_card_); br_card_ = nullptr; }
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
  const int win_width = Scale(560, dpi);
  const int win_height = Scale(720, dpi);

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

  // macOS 26 质感：启用圆角与沉浸式黑白自适应标题栏
  const DWMNCRENDERINGPOLICY ncp = DWMNCRP_ENABLED;
  DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_POLICY, &ncp, sizeof(ncp));
  const DWORD round_pref = DWMWCP_ROUND;
  DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &round_pref, sizeof(round_pref));

  UpdateThemeStyles();
  CreateControls();
  LayoutControls(dpi);
  LoadValuesToUI();

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

  if (is_dark_mode_) {
    // macOS 26 深空黑曜石沉浸质感
    br_bg_ = CreateSolidBrush(RGB(24, 25, 28));
    br_card_ = CreateSolidBrush(RGB(38, 41, 48));
    cr_text_ = RGB(245, 245, 247);
    cr_secondary_ = RGB(158, 165, 175);
    cr_card_border_ = RGB(55, 60, 70);
  } else {
    // macOS 26 纯白磨砂通透质感
    br_bg_ = CreateSolidBrush(RGB(242, 243, 247));
    br_card_ = CreateSolidBrush(RGB(255, 255, 255));
    cr_text_ = RGB(28, 28, 30);
    cr_secondary_ = RGB(108, 115, 128);
    cr_card_border_ = RGB(225, 228, 235);
  }
}

void SettingsWindow::CreateControls() {
  auto create_chk = [&](const wchar_t* text, UINT id) {
    return CreateWindowExW(0, L"BUTTON", text,
                           WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                           0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(id),
                           instance_, nullptr);
  };
  auto create_slider = [&](UINT id, int min_val, int max_val) {
    HWND sl = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                              WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | WS_TABSTOP,
                              0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(id),
                              instance_, nullptr);
    SendMessageW(sl, TBM_SETRANGE, TRUE, MAKELPARAM(min_val, max_val));
    return sl;
  };
  auto create_lbl = [&](const wchar_t* text) {
    return CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
                           0, 0, 10, 10, hwnd_, nullptr, instance_, nullptr);
  };
  auto create_btn = [&](const wchar_t* text, UINT id) {
    return CreateWindowExW(0, L"BUTTON", text,
                           WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                           0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(id),
                           instance_, nullptr);
  };
  auto create_combo = [&](UINT id) {
    HWND cb = CreateWindowExW(0, L"COMBOBOX", L"",
                              WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                              0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(id),
                              instance_, nullptr);
    for (const auto& item : GetKeyList()) {
      SendMessageW(cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.first.c_str()));
    }
    return cb;
  };

  // 卡片 1: 外观
  slider_gap_ = create_slider(IDC_SLIDER_GAP, 0, 80);
  lbl_gap_val_ = create_lbl(L"16 px");
  slider_h_width_ = create_slider(IDC_SLIDER_H_WIDTH, 1, 20);
  lbl_h_width_val_ = create_lbl(L"2 px");
  slider_v_width_ = create_slider(IDC_SLIDER_V_WIDTH, 1, 20);
  lbl_v_width_val_ = create_lbl(L"2 px");

  // 卡片 2: 主题与颜色
  chk_auto_theme_ = create_chk(L"跟随 Windows 10/11 系统深浅黑白模式实时切换", IDC_CHK_AUTO_THEME);
  preview_light_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                  0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(IDC_PREVIEW_LIGHT), instance_, nullptr);
  btn_color_light_ = create_btn(L"选择浅色色彩...", IDC_BTN_COLOR_LIGHT);
  slider_alpha_light_ = create_slider(IDC_SLIDER_ALPHA_LIGHT, 20, 255);
  lbl_alpha_light_val_ = create_lbl(L"200");

  preview_dark_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(IDC_PREVIEW_DARK), instance_, nullptr);
  btn_color_dark_ = create_btn(L"选择深色色彩...", IDC_BTN_COLOR_DARK);
  slider_alpha_dark_ = create_slider(IDC_SLIDER_ALPHA_DARK, 20, 255);
  lbl_alpha_dark_val_ = create_lbl(L"180");

  // 卡片 3: 快捷键
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

  // 卡片 4: 系统
  chk_auto_dpi_ = create_chk(L"适配 4K / 8K 分辨率高 DPI 屏幕（物理微米级动态缩放）", IDC_CHK_AUTO_DPI);
  chk_autostart_ = create_chk(L"开机自动启动 CrossPointer（纯便携自启动，零系统污染）", IDC_CHK_AUTOSTART);

  // 底部按钮
  btn_save_ = create_btn(L"💾 保存并应用", IDC_BTN_SAVE);
  btn_defaults_ = create_btn(L"🔄 恢复默认", IDC_BTN_DEFAULTS);
  btn_close_ = create_btn(L"关闭", IDC_BTN_CLOSE);
}

void SettingsWindow::LayoutControls(int dpi) {
  if (font_title_ != nullptr) DeleteObject(font_title_);
  if (font_bold_ != nullptr) DeleteObject(font_bold_);
  if (font_regular_ != nullptr) DeleteObject(font_regular_);
  if (font_small_ != nullptr) DeleteObject(font_small_);

  font_title_ = CreateModernFont(13, FW_BOLD, dpi);
  font_bold_ = CreateModernFont(10, FW_SEMIBOLD, dpi);
  font_regular_ = CreateModernFont(9, FW_NORMAL, dpi);
  font_small_ = CreateModernFont(8, FW_NORMAL, dpi);

  auto set_font = [&](HWND h, HFONT f) {
    if (h != nullptr) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
  };

  // 设置所有控件字体
  for (HWND h : {chk_auto_theme_, chk_auto_dpi_, chk_autostart_,
                 chk_toggle_ctrl_, chk_toggle_alt_, chk_toggle_win_, chk_toggle_shift_,
                 chk_exit_ctrl_, chk_exit_alt_, chk_exit_win_, chk_exit_shift_,
                 combo_toggle_vk_, combo_exit_vk_,
                 btn_color_light_, btn_color_dark_,
                 btn_save_, btn_defaults_, btn_close_}) {
    set_font(h, font_regular_);
  }
  set_font(btn_save_, font_bold_);
  for (HWND h : {lbl_gap_val_, lbl_h_width_val_, lbl_v_width_val_,
                 lbl_alpha_light_val_, lbl_alpha_dark_val_}) {
    set_font(h, font_small_);
  }

  // 布局计算 (以 Card 容器为基准)
  const int pad_x = Scale(18, dpi);
  const int card_w = Scale(524, dpi);
  const int c_inner_x = pad_x + Scale(14, dpi);

  // Card 1 内部控件
  SetWindowPos(slider_gap_, nullptr, c_inner_x + Scale(90, dpi), Scale(54, dpi), Scale(170, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_gap_val_, nullptr, c_inner_x + Scale(265, dpi), Scale(57, dpi), Scale(45, dpi), Scale(20, dpi), SWP_NOZORDER);

  SetWindowPos(slider_h_width_, nullptr, c_inner_x + Scale(90, dpi), Scale(86, dpi), Scale(170, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_h_width_val_, nullptr, c_inner_x + Scale(265, dpi), Scale(89, dpi), Scale(45, dpi), Scale(20, dpi), SWP_NOZORDER);

  SetWindowPos(slider_v_width_, nullptr, c_inner_x + Scale(90, dpi), Scale(118, dpi), Scale(170, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_v_width_val_, nullptr, c_inner_x + Scale(265, dpi), Scale(121, dpi), Scale(45, dpi), Scale(20, dpi), SWP_NOZORDER);

  // Card 2 内部控件
  SetWindowPos(chk_auto_theme_, nullptr, c_inner_x, Scale(182, dpi), card_w - Scale(28, dpi), Scale(22, dpi), SWP_NOZORDER);

  // 浅色模式颜色行
  SetWindowPos(preview_light_, nullptr, c_inner_x + Scale(80, dpi), Scale(212, dpi), Scale(36, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(btn_color_light_, nullptr, c_inner_x + Scale(122, dpi), Scale(211, dpi), Scale(110, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(slider_alpha_light_, nullptr, c_inner_x + Scale(280, dpi), Scale(212, dpi), Scale(120, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_alpha_light_val_, nullptr, c_inner_x + Scale(406, dpi), Scale(215, dpi), Scale(40, dpi), Scale(20, dpi), SWP_NOZORDER);

  // 深色模式颜色行
  SetWindowPos(preview_dark_, nullptr, c_inner_x + Scale(80, dpi), Scale(246, dpi), Scale(36, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(btn_color_dark_, nullptr, c_inner_x + Scale(122, dpi), Scale(245, dpi), Scale(110, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(slider_alpha_dark_, nullptr, c_inner_x + Scale(280, dpi), Scale(246, dpi), Scale(120, dpi), Scale(24, dpi), SWP_NOZORDER);
  SetWindowPos(lbl_alpha_dark_val_, nullptr, c_inner_x + Scale(406, dpi), Scale(249, dpi), Scale(40, dpi), Scale(20, dpi), SWP_NOZORDER);

  // Card 3 快捷键内部控件
  // 显隐快捷键
  int k_x = c_inner_x + Scale(75, dpi);
  SetWindowPos(chk_toggle_ctrl_, nullptr, k_x, Scale(314, dpi), Scale(52, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(chk_toggle_alt_, nullptr, k_x + Scale(55, dpi), Scale(314, dpi), Scale(50, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(chk_toggle_win_, nullptr, k_x + Scale(108, dpi), Scale(314, dpi), Scale(52, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(chk_toggle_shift_, nullptr, k_x + Scale(163, dpi), Scale(314, dpi), Scale(58, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(combo_toggle_vk_, nullptr, k_x + Scale(230, dpi), Scale(312, dpi), Scale(110, dpi), Scale(200, dpi), SWP_NOZORDER);

  // 退出快捷键
  SetWindowPos(chk_exit_ctrl_, nullptr, k_x, Scale(350, dpi), Scale(52, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(chk_exit_alt_, nullptr, k_x + Scale(55, dpi), Scale(350, dpi), Scale(50, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(chk_exit_win_, nullptr, k_x + Scale(108, dpi), Scale(350, dpi), Scale(52, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(chk_exit_shift_, nullptr, k_x + Scale(163, dpi), Scale(350, dpi), Scale(58, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(combo_exit_vk_, nullptr, k_x + Scale(230, dpi), Scale(348, dpi), Scale(110, dpi), Scale(200, dpi), SWP_NOZORDER);

  // Card 4 系统项
  SetWindowPos(chk_auto_dpi_, nullptr, c_inner_x, Scale(418, dpi), card_w - Scale(28, dpi), Scale(22, dpi), SWP_NOZORDER);
  SetWindowPos(chk_autostart_, nullptr, c_inner_x, Scale(448, dpi), card_w - Scale(28, dpi), Scale(22, dpi), SWP_NOZORDER);

  // 底部动作按钮
  const int btn_y = Scale(510, dpi);
  SetWindowPos(btn_save_, nullptr, pad_x, btn_y, Scale(130, dpi), Scale(34, dpi), SWP_NOZORDER);
  SetWindowPos(btn_defaults_, nullptr, pad_x + Scale(140, dpi), btn_y, Scale(110, dpi), Scale(34, dpi), SWP_NOZORDER);
  SetWindowPos(btn_close_, nullptr, pad_x + card_w - Scale(90, dpi), btn_y, Scale(90, dpi), Scale(34, dpi), SWP_NOZORDER);
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
  Button_SetCheck(chk_auto_dpi_, config_.auto_dpi ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(chk_autostart_, autostart::IsAutoStartEnabled() ? BST_CHECKED : BST_UNCHECKED);

  InvalidateRect(preview_light_, nullptr, TRUE);
  InvalidateRect(preview_dark_, nullptr, TRUE);
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
  const unsigned int t_vk = (t_idx >= 0 && t_idx < static_cast<int>(GetKeyList().size())) ? GetKeyList()[t_idx].second : 'H';

  // Hotkey - Exit
  unsigned int e_mod = 0;
  if (Button_GetCheck(chk_exit_ctrl_) == BST_CHECKED) e_mod |= MOD_CONTROL;
  if (Button_GetCheck(chk_exit_alt_) == BST_CHECKED) e_mod |= MOD_ALT;
  if (Button_GetCheck(chk_exit_win_) == BST_CHECKED) e_mod |= MOD_WIN;
  if (Button_GetCheck(chk_exit_shift_) == BST_CHECKED) e_mod |= MOD_SHIFT;
  const int e_idx = static_cast<int>(SendMessageW(combo_exit_vk_, CB_GETCURSEL, 0, 0));
  const unsigned int e_vk = (e_idx >= 0 && e_idx < static_cast<int>(GetKeyList().size())) ? GetKeyList()[e_idx].second : 'E';

  if (t_mod == e_mod && t_vk == e_vk) {
    MessageBoxW(hwnd_, L"显隐热键与退出热键不能相同，请修改后再保存。", L"热键冲突", MB_OK | MB_ICONWARNING);
    return false;
  }

  updated.hotkey_h_s.mod = t_mod;
  updated.hotkey_h_s.vk = t_vk;
  updated.hotkey_exit.mod = e_mod;
  updated.hotkey_exit.vk = e_vk;
  updated.ClampAll();

  // System
  updated.auto_dpi = Button_GetCheck(chk_auto_dpi_) == BST_CHECKED;
  const bool enable_autostart = Button_GetCheck(chk_autostart_) == BST_CHECKED;
  autostart::SetAutoStart(enable_autostart);

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

  MessageBoxW(hwnd_, L"偏好设置已成功保存并即刻生效！", L"提示", MB_OK | MB_ICONINFORMATION);
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
    case WM_HSCROLL: {
      // 实时更新数值标签
      if (reinterpret_cast<HWND>(lp) == slider_gap_) {
        const int val = static_cast<int>(SendMessageW(slider_gap_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_gap_val_, (std::to_wstring(val) + L" px").c_str());
      } else if (reinterpret_cast<HWND>(lp) == slider_h_width_) {
        const int val = static_cast<int>(SendMessageW(slider_h_width_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_h_width_val_, (std::to_wstring(val) + L" px").c_str());
      } else if (reinterpret_cast<HWND>(lp) == slider_v_width_) {
        const int val = static_cast<int>(SendMessageW(slider_v_width_, TBM_GETPOS, 0, 0));
        SetWindowTextW(lbl_v_width_val_, (std::to_wstring(val) + L" px").c_str());
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

      // 1. 绘制全局背景
      FillRect(hdc, &client, br_bg_);

      // 2. 绘制 macOS 卡片背景与标题
      auto draw_card = [&](int top, int height, const wchar_t* title) {
        const int pad_x = Scale(18, dpi);
        const int card_w = Scale(524, dpi);
        const int card_r = Scale(10, dpi);
        RECT rc = {pad_x, top, pad_x + card_w, top + height};

        HPEN pen = CreatePen(PS_SOLID, 1, cr_card_border_);
        HPEN old_pen = SelectPen(hdc, pen);
        HBRUSH old_br = SelectBrush(hdc, br_card_);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, card_r, card_r);
        SelectBrush(hdc, old_br);
        SelectPen(hdc, old_pen);
        DeleteObject(pen);

        // 卡片标题
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, cr_text_);
        SelectFont(hdc, font_title_);
        TextOutW(hdc, rc.left + Scale(14, dpi), rc.top + Scale(12, dpi), title, static_cast<int>(wcslen(title)));
      };

      // 四大卡片
      draw_card(Scale(14, dpi), Scale(138, dpi), L"🎯 辅助线尺寸与外观");
      draw_card(Scale(162, dpi), Scale(120, dpi), L"🎨 色彩模式与透明度");
      draw_card(Scale(292, dpi), Scale(92, dpi), L"⌨️ 全局快捷键");
      draw_card(Scale(394, dpi), Scale(86, dpi), L"🚀 系统偏好");

      // 绘制各行的静态提示文字
      SelectFont(hdc, font_regular_);
      SetTextColor(hdc, cr_secondary_);

      const int c_x = Scale(18, dpi) + Scale(14, dpi);
      // Card 1
      TextOutW(hdc, c_x, Scale(57, dpi), L"中心间隙:", 5);
      TextOutW(hdc, c_x, Scale(89, dpi), L"横线粗细:", 5);
      TextOutW(hdc, c_x, Scale(121, dpi), L"竖线粗细:", 5);

      // Card 2
      TextOutW(hdc, c_x, Scale(214, dpi), L"浅色模式:", 5);
      TextOutW(hdc, c_x + Scale(242, dpi), Scale(214, dpi), L"不透明度:", 5);
      TextOutW(hdc, c_x, Scale(248, dpi), L"深色模式:", 5);
      TextOutW(hdc, c_x + Scale(242, dpi), Scale(248, dpi), L"不透明度:", 5);

      // Card 3
      TextOutW(hdc, c_x, Scale(316, dpi), L"显隐热键:", 5);
      TextOutW(hdc, c_x, Scale(352, dpi), L"退出热键:", 5);

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
