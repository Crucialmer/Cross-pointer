#include <windows.h>
#include <shellapi.h>
#include <timeapi.h>

#include <exception>
#include <string>

#include "src/autostart.h"
#include "src/config.h"
#include "src/config_file_util.h"
#include "src/crosshair.h"
#include "src/hotkey.h"
#include "src/settings_window.h"
#include "src/tray.h"
#include "src/updater.h"

namespace {
constexpr wchar_t kMutexName[] =
    L"Local\\CrossPointer_F5B6239126A64833BE094D6DC8DC1951";
constexpr wchar_t kControllerClassName[] = L"CrossPointerControllerClass";

enum class PreferredAppMode {
  Default,
  AllowDark,
  ForceDark,
  ForceLight,
  Max
};
using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode);

void EnableDarkModeSupport() noexcept {
  const HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr,
                                         LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (uxtheme != nullptr) {
    const auto set_mode = reinterpret_cast<fnSetPreferredAppMode>(
        GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));
    if (set_mode != nullptr) {
      set_mode(PreferredAppMode::AllowDark);
    }
  }
}

class UniqueHandle {
 public:
  explicit UniqueHandle(HANDLE handle = nullptr) noexcept : handle_(handle) {}
  ~UniqueHandle() {
    if (handle_ != nullptr) CloseHandle(handle_);
  }
  UniqueHandle(const UniqueHandle&) = delete;
  UniqueHandle& operator=(const UniqueHandle&) = delete;
  [[nodiscard]] HANDLE get() const noexcept { return handle_; }

 private:
  HANDLE handle_;
};

// RAII 精度计时器：将 Windows 调度器时钟中断精度提升到 1ms，
// 确保在 120Hz/144Hz 高刷屏及 8K 超高分辨率屏幕下鼠标十字线移动如丝绸般平滑。
class PrecisionTimerScope {
 public:
  PrecisionTimerScope() noexcept { timeBeginPeriod(1); }
  ~PrecisionTimerScope() noexcept { timeEndPeriod(1); }
  PrecisionTimerScope(const PrecisionTimerScope&) = delete;
  PrecisionTimerScope& operator=(const PrecisionTimerScope&) = delete;
};

void EnableBestDpiAwareness() noexcept {
  using SetDpiContext = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
  if (const HMODULE user32 = GetModuleHandleW(L"user32.dll"); user32 != nullptr) {
    const auto set_context = reinterpret_cast<SetDpiContext>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (set_context != nullptr &&
        set_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
      return;
    }
  }
  SetProcessDPIAware();
}

std::wstring FormatHotkeyTooltip(const HotkeyConfig& toggle,
                                 const HotkeyConfig& exit) {
  auto format_single = [](const HotkeyConfig& hotkey) {
    std::wstring s;
    if ((hotkey.mod & MOD_CONTROL) != 0) s += L"Ctrl+";
    if ((hotkey.mod & MOD_ALT) != 0) s += L"Alt+";
    if ((hotkey.mod & MOD_WIN) != 0) s += L"Win+";
    if ((hotkey.mod & MOD_SHIFT) != 0) s += L"Shift+";

    if (hotkey.vk >= L'A' && hotkey.vk <= L'Z') {
      s += static_cast<wchar_t>(hotkey.vk);
    } else if (hotkey.vk >= L'0' && hotkey.vk <= L'9') {
      s += static_cast<wchar_t>(hotkey.vk);
    } else if (hotkey.vk >= VK_F1 && hotkey.vk <= VK_F24) {
      s += L"F" + std::to_wstring(hotkey.vk - VK_F1 + 1);
    } else {
      s += Config::VKToString(hotkey.vk);
    }
    return s;
  };

  if (toggle.mod == exit.mod && toggle.vk == exit.vk) {
    return L"CrossPointer 🎯 (单键复用: " + format_single(toggle) + L" 显示/退出)";
  }
  return L"CrossPointer 🎯 (显隐: " + format_single(toggle) + L", 退出: " + format_single(exit) + L")";
}

struct AppContext {
  HINSTANCE instance = nullptr;
  std::wstring config_path;
  Config raw_config;
  Config active_config;
  CrosshairWindow* crosshair = nullptr;
  TrayManager tray;
  SettingsWindow settings_win;

  void RefreshActiveTheme() {
    const bool is_light = IsSystemInLightTheme();
    active_config = GetThemedConfig(raw_config, is_light);
    if (crosshair != nullptr) {
      (void)crosshair->ApplyConfig(active_config);
    }
  }
};

LRESULT CALLBACK ControllerWndProc(HWND window, UINT message, WPARAM w_param,
                                    LPARAM l_param) noexcept {
  auto* ctx = reinterpret_cast<AppContext*>(
      GetWindowLongPtrW(window, GWLP_USERDATA));

  switch (message) {
    case WM_NCCREATE: {
      const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
      ctx = static_cast<AppContext*>(create->lpCreateParams);
      SetWindowLongPtrW(window, GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(ctx));
      return TRUE;
    }

    // 监听系统深浅主题切换（Windows 10/11 实时广播）
    case WM_SETTINGCHANGE: {
      if (ctx != nullptr && ctx->raw_config.auto_theme) {
        ctx->RefreshActiveTheme();
      }
      break;
    }

    case WM_TRAYICON: {
      if (ctx == nullptr) break;
      if (l_param == WM_RBUTTONUP) {
        POINT pt = {};
        GetCursorPos(&pt);
        ctx->tray.ShowContextMenu(window, pt, ctx->crosshair->IsVisible());
      } else if (l_param == WM_LBUTTONUP) {
        PostMessageW(window, WM_COMMAND, IDM_TRAY_TOGGLE, 0);
      } else if (l_param == WM_LBUTTONDBLCLK) {
        PostMessageW(window, WM_COMMAND, IDM_TRAY_SETTINGS, 0);
      }
      return 0;
    }

    case WM_COMMAND: {
      if (ctx == nullptr) break;
      const UINT command_id = LOWORD(w_param);
      switch (command_id) {
        case IDM_TRAY_TOGGLE: {
          ctx->crosshair->ToggleVisible();
          return 0;
        }

        case IDM_TRAY_SETTINGS: {
          ctx->settings_win.Show(
              ctx->instance, window, ctx->raw_config, ctx->config_path,
              [ctx](const Config& new_config) -> bool {
                if (!HotkeyManager::Register(new_config.hotkey_h_s,
                                             new_config.hotkey_exit)) {
                  HotkeyManager::Register(ctx->raw_config.hotkey_h_s,
                                          ctx->raw_config.hotkey_exit);
                  return false;
                }
                ctx->raw_config = new_config;
                ctx->RefreshActiveTheme();
                ctx->tray.UpdateTooltip(
                    FormatHotkeyTooltip(ctx->raw_config.hotkey_h_s,
                                        ctx->raw_config.hotkey_exit));
                return true;
              });
          return 0;
        }

        case IDM_TRAY_CHECK_UPDATE: {
          updater::CheckUpdateAsync(window, /*silent_if_latest=*/false);
          return 0;
        }

        case IDM_TRAY_OPEN_CONFIG: {
          ShellExecuteW(nullptr, L"open", ctx->config_path.c_str(), nullptr,
                        nullptr, SW_SHOW);
          return 0;
        }

        case IDM_TRAY_RELOAD: {
          Config updated;
          if (!updated.Load(ctx->config_path.c_str())) {
            MessageBoxW(window, L"配置文件读取失败，请检查语法格式。", L"提示",
                        MB_OK | MB_ICONWARNING);
            return 0;
          }
          if (!HotkeyManager::Register(updated.hotkey_h_s, updated.hotkey_exit)) {
            HotkeyManager::Register(ctx->raw_config.hotkey_h_s, ctx->raw_config.hotkey_exit);
            MessageBoxW(window, L"新热键已被占用，已保留原有配置。", L"警告",
                        MB_OK | MB_ICONWARNING);
            return 0;
          }
          ctx->raw_config = updated;
          ctx->RefreshActiveTheme();
          ctx->tray.UpdateTooltip(
              FormatHotkeyTooltip(ctx->raw_config.hotkey_h_s,
                                  ctx->raw_config.hotkey_exit));
          MessageBoxW(window, L"配置已成功重新载入并生效！", L"提示",
                      MB_OK | MB_ICONINFORMATION);
          return 0;
        }

        case IDM_TRAY_ABOUT: {
          MessageBoxW(
              window,
              L"CrossPointer 1.4 (macOS 26 视觉美学重构、无截断宽屏偏好设置与 8K 适配版)\n\n"
              L"• 单键复用：显隐与退出可设为同一热键，未显示时显示，显示时直接退出\n"
              L"• 按键顺序无关：Ctrl/Alt/Win/Shift 任意顺序按下均可毫秒级触发\n"
              L"• macOS 26 偏好设置：分栏胶囊 Tab 切换，消除压缩，极致宽敞美观\n"
              L"• 8K 与高 DPI 适配：物理微米级动态自适应缩放，线宽永不失真\n"
              L"• 极简加粗激光十字图标：中心圆形镂空光孔，极致辨识度\n"
              L"• 一键本地直接更新：支持应用内全自动极速静默热升级，无需前往浏览器手动下载\n"
              L"• 绿色开机自启动：独立 HKCU 便携接管，零系统垃圾\n\n"
              L"100% 纯绿色运行，零系统残留。\n"
              L"Copyright © 2026 Crucialmer",
              L"关于 CrossPointer", MB_OK | MB_ICONINFORMATION);
          return 0;
        }

        case IDM_TRAY_EXIT: {
          PostQuitMessage(0);
          return 0;
        }
      }
      break;
    }

    case WM_DESTROY: {
      if (ctx != nullptr) {
        ctx->settings_win.Close();
        ctx->tray.Remove();
      }
      break;
    }
  }
  return DefWindowProcW(window, message, w_param, l_param);
}

int Run(HINSTANCE instance) {
  updater::CleanupOldExecutable();

  const bool is_update_restart =
      (wcsstr(GetCommandLineW(), L"--update-restart") != nullptr);
  UniqueHandle mutex;
  const int max_tries = is_update_restart ? 30 : 1;
  for (int i = 0; i < max_tries; ++i) {
    HANDLE h = CreateMutexW(nullptr, FALSE, kMutexName);
    if (h != nullptr && GetLastError() != ERROR_ALREADY_EXISTS) {
      mutex = UniqueHandle(h);
      break;
    }
    if (h != nullptr) CloseHandle(h);
    if (!is_update_restart) break;
    Sleep(100);
  }

  if (mutex.get() == nullptr) {
    if (!is_update_restart) {
      MessageBoxW(nullptr, L"CrossPointer 已经在运行中（可在右下角系统托盘找到）。", L"提示",
                  MB_OK | MB_ICONINFORMATION);
    }
    return 0;
  }

  EnableDarkModeSupport();
  const PrecisionTimerScope precision_timer;
  EnableBestDpiAwareness();
  const std::wstring config_path = GetConfigPath();
  if (!EnsureConfigExists(config_path)) return 1;

  Config raw_config;
  if (!raw_config.Load(config_path.c_str())) {
    MessageBoxW(nullptr, L"配置文件无效，请修正后重新启动程序。", L"错误",
                MB_OK | MB_ICONERROR);
    return 1;
  }

  Config active_config = GetThemedConfig(raw_config, IsSystemInLightTheme());

  CrosshairWindow crosshair(instance, active_config);
  if (!crosshair.Create()) {
    MessageBoxW(nullptr, L"创建或初始化准星窗口失败。", L"错误",
                MB_OK | MB_ICONERROR);
    return 1;
  }
  // 检查启动时是否默认显示十字线，若用户配置为隐藏则启动后静默常驻托盘
  if (!raw_config.show_on_startup) {
    crosshair.SetVisible(false);
  }
  if (!HotkeyManager::Register(raw_config.hotkey_h_s, raw_config.hotkey_exit)) {
    MessageBoxW(nullptr, L"注册全局热键失败；热键可能已被其他程序占用。",
                L"错误", MB_OK | MB_ICONERROR);
    return 1;
  }

  AppContext context;
  context.instance = instance;
  context.config_path = config_path;
  context.raw_config = raw_config;
  context.active_config = active_config;
  context.crosshair = &crosshair;

  WNDCLASSEXW wc = {sizeof(wc)};
  wc.lpfnWndProc = ControllerWndProc;
  wc.hInstance = instance;
  wc.lpszClassName = kControllerClassName;
  if (RegisterClassExW(&wc) == 0) return 1;

  HWND controller = CreateWindowExW(
      0, kControllerClassName, L"CrossPointerController", WS_OVERLAPPED, 0, 0,
      0, 0, nullptr, nullptr, instance, &context);
  if (controller == nullptr) {
    UnregisterClassW(kControllerClassName, instance);
    return 1;
  }

  context.tray.Initialize(instance, controller);
  if (is_update_restart) {
    context.tray.UpdateTooltip(L"CrossPointer 🎯 (已成功直接更新至最新版本！)");
  } else {
    context.tray.UpdateTooltip(
        FormatHotkeyTooltip(context.raw_config.hotkey_h_s,
                            context.raw_config.hotkey_exit));
  }

  if (context.raw_config.auto_check_update) {
    updater::CheckUpdateAsync(controller, /*silent_if_latest=*/true);
  }

  int result = 0;
  MSG message = {};
  while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
    if (message.message == WM_HOTKEY) {
      if (message.wParam == kExitHotkeyId) {
        PostQuitMessage(0);
        continue;
      }
      if (message.wParam == kToggleHotkeyId) {
        const bool is_same_hotkey =
            (context.raw_config.hotkey_h_s.mod == context.raw_config.hotkey_exit.mod &&
             context.raw_config.hotkey_h_s.vk == context.raw_config.hotkey_exit.vk);

        // 当显隐与退出设为同一全局快捷键时：
        // 1. 若当前十字线正在显示 -> 触发安全退出程序
        // 2. 若当前十字线处于隐藏 -> 触发热重载并显示十字线
        if (is_same_hotkey) {
          if (crosshair.IsVisible()) {
            PostQuitMessage(0);
            continue;
          }

          // 重新显示时热重载配置
          Config updated;
          if (updated.Load(config_path.c_str())) {
            if (HotkeyManager::Register(updated.hotkey_h_s, updated.hotkey_exit)) {
              context.raw_config = updated;
              context.RefreshActiveTheme();
              context.tray.UpdateTooltip(
                  FormatHotkeyTooltip(context.raw_config.hotkey_h_s,
                                      context.raw_config.hotkey_exit));
            }
          }
          crosshair.ToggleVisible();
          continue;
        }

        // 当显隐与退出为不同热键时：按显隐热键切换显隐状态
        if (crosshair.IsVisible()) {
          crosshair.ToggleVisible();
          continue;
        }

        // 仅在重新显示时热重载
        Config updated;
        if (!updated.Load(config_path.c_str())) {
          crosshair.ToggleVisible();
          continue;
        }
        if (HotkeyManager::Register(updated.hotkey_h_s, updated.hotkey_exit)) {
          context.raw_config = updated;
          context.RefreshActiveTheme();
          context.tray.UpdateTooltip(
              FormatHotkeyTooltip(context.raw_config.hotkey_h_s,
                                  context.raw_config.hotkey_exit));
        }
        crosshair.ToggleVisible();
        continue;
      }
    }

    TranslateMessage(&message);
    DispatchMessageW(&message);
  }

  HotkeyManager::UnregisterAll();
  context.settings_win.Close();
  context.tray.Remove();
  DestroyWindow(controller);
  UnregisterClassW(kControllerClassName, instance);

  if (result == -1) {
    MessageBoxW(nullptr, L"Windows 消息循环异常终止。", L"错误",
                MB_OK | MB_ICONERROR);
    return 1;
  }
  return static_cast<int>(message.wParam);
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  try {
    return Run(instance);
  } catch (const std::exception&) {
    MessageBoxW(nullptr, L"程序发生未处理的 C++ 异常。", L"严重错误",
                MB_OK | MB_ICONERROR);
  } catch (...) {
    MessageBoxW(nullptr, L"程序发生未知异常。", L"严重错误",
                MB_OK | MB_ICONERROR);
  }
  return 1;
}
