#include "./tray.h"

#include <shellapi.h>

namespace {
constexpr UINT kTrayId = 1001;
}  // namespace

TrayManager::~TrayManager() {
  Remove();
}

bool TrayManager::Initialize(HINSTANCE instance, HWND window) {
  window_ = window;
  const int icon_size = GetSystemMetrics(SM_CXSMICON);
  icon_ = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101),
                                        IMAGE_ICON, icon_size, icon_size,
                                        LR_DEFAULTCOLOR));
  if (icon_ == nullptr) {
    icon_ = LoadIconW(nullptr, IDI_APPLICATION);
  }

  NOTIFYICONDATAW nid = {sizeof(nid)};
  nid.hWnd = window_;
  nid.uID = kTrayId;
  nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  nid.uCallbackMessage = WM_TRAYICON;
  nid.hIcon = icon_;
  wcscpy_s(nid.szTip, L"CrossPointer");

  added_ = Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
  return added_;
}

void TrayManager::Remove() noexcept {
  if (added_ && window_ != nullptr) {
    NOTIFYICONDATAW nid = {sizeof(nid)};
    nid.hWnd = window_;
    nid.uID = kTrayId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    added_ = false;
  }
  if (icon_ != nullptr) {
    DestroyIcon(icon_);
    icon_ = nullptr;
  }
}

void TrayManager::UpdateTooltip(const std::wstring& text) noexcept {
  if (!added_ || window_ == nullptr) return;
  NOTIFYICONDATAW nid = {sizeof(nid)};
  nid.hWnd = window_;
  nid.uID = kTrayId;
  nid.uFlags = NIF_TIP;
  wcsncpy_s(nid.szTip, text.c_str(), _countof(nid.szTip) - 1);
  Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayManager::ShowContextMenu(HWND window, POINT pt,
                                  bool is_visible) const noexcept {
  HMENU menu = CreatePopupMenu();
  if (menu == nullptr) return;

  const wchar_t* toggle_text =
      is_visible ? L"隐藏十字线 (&H)" : L"显示十字线 (&S)";
  AppendMenuW(menu, MF_STRING, IDM_TRAY_TOGGLE, toggle_text);
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(menu, MF_STRING, IDM_TRAY_SETTINGS, L"⚙️ 偏好设置... (&P)");
  AppendMenuW(menu, MF_STRING, IDM_TRAY_RELOAD, L"🔄 重新载入配置 (&R)");
  AppendMenuW(menu, MF_STRING, IDM_TRAY_OPEN_CONFIG,
              L"📝 高级：打开配置文件 (&O)");
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(menu, MF_STRING, IDM_TRAY_ABOUT, L"ℹ️ 关于 CrossPointer (&A)");
  AppendMenuW(menu, MF_STRING, IDM_TRAY_EXIT, L"🚪 退出 (&X)");

  SetForegroundWindow(window);
  TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN, pt.x,
                 pt.y, 0, window, nullptr);
  PostMessageW(window, WM_NULL, 0, 0);
  DestroyMenu(menu);
}
