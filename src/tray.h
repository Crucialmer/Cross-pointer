#pragma once

#include <windows.h>
#include <string>

constexpr UINT WM_TRAYICON = WM_APP + 100;

constexpr UINT IDM_TRAY_TOGGLE = 2001;
constexpr UINT IDM_TRAY_OPEN_CONFIG = 2002;
constexpr UINT IDM_TRAY_RELOAD = 2003;
constexpr UINT IDM_TRAY_ABOUT = 2004;
constexpr UINT IDM_TRAY_EXIT = 2005;
constexpr UINT IDM_TRAY_SETTINGS = 2006;
constexpr UINT IDM_TRAY_CHECK_UPDATE = 2007;

class TrayManager {
 public:
  TrayManager() noexcept = default;
  ~TrayManager();

  TrayManager(const TrayManager&) = delete;
  TrayManager& operator=(const TrayManager&) = delete;

  [[nodiscard]] bool Initialize(HINSTANCE instance, HWND window);
  void Remove() noexcept;
  void UpdateTooltip(const std::wstring& text) noexcept;
  void ShowContextMenu(HWND window, POINT pt, bool is_visible) const noexcept;

 private:
  HWND window_ = nullptr;
  HICON icon_ = nullptr;
  bool added_ = false;
};
