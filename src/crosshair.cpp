#include "./crosshair.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr wchar_t kClassName[] = L"MouseCrosshairWindow";
constexpr DWORD kExtendedWindowStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                       WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
                                       WS_EX_NOACTIVATE;

template <typename T>
void SafeRelease(T*& object) noexcept {
  if (object != nullptr) {
    object->Release();
    object = nullptr;
  }
}

D2D1_COLOR_F ToD2DColor(const LineConfig& line) noexcept {
  return D2D1::ColorF(static_cast<float>(line.r.value) / 255.0F,
                      static_cast<float>(line.g.value) / 255.0F,
                      static_cast<float>(line.b.value) / 255.0F, 1.0F);
}

float GetMonitorDpiScale(HMONITOR monitor) noexcept {
  using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
  static const auto get_dpi_fn = []() -> GetDpiForMonitorFn {
    const HMODULE shcore = LoadLibraryExW(L"shcore.dll", nullptr,
                                         LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (shcore != nullptr) {
      return reinterpret_cast<GetDpiForMonitorFn>(
          GetProcAddress(shcore, "GetDpiForMonitor"));
    }
    return nullptr;
  }();

  if (get_dpi_fn != nullptr && monitor != nullptr) {
    UINT dpi_x = 96;
    UINT dpi_y = 96;
    if (SUCCEEDED(get_dpi_fn(monitor, 0 /* MDT_EFFECTIVE_DPI */, &dpi_x,
                            &dpi_y))) {
      return static_cast<float>(dpi_x) / 96.0f;
    }
  }
  return 1.0f;
}
}  // namespace

CrosshairWindow::CrosshairWindow(HINSTANCE instance,
                                 const Config& config) noexcept
    : instance_(instance), config_(config) {}

CrosshairWindow::~CrosshairWindow() {
  if (left_window_ != nullptr) {
    KillTimer(left_window_, kUpdateTimerId);
  }
  SetWindowsVisible(false);
  CleanupDirect2D();
  DestroyWindows();
  if (class_registered_) {
    UnregisterClassW(kClassName, instance_);
  }
}

void CrosshairWindow::DestroyWindows() noexcept {
  const std::array<HWND*, 4> windows = {&left_window_, &right_window_,
                                        &top_window_, &bottom_window_};
  for (HWND* window : windows) {
    if (*window != nullptr) {
      SetWindowLongPtrW(*window, GWLP_USERDATA, 0);
      DestroyWindow(*window);
      *window = nullptr;
    }
  }
}

bool CrosshairWindow::Create() {
  WNDCLASSEXW window_class = {sizeof(window_class)};
  window_class.lpfnWndProc = WndProc;
  window_class.hInstance = instance_;
  window_class.lpszClassName = kClassName;
  window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);

  if (RegisterClassExW(&window_class) == 0) {
    return false;
  }
  class_registered_ = true;

  auto create_window = [this]() {
    return CreateWindowExW(kExtendedWindowStyle, kClassName, L"", WS_POPUP, 0,
                           0, 1, 1, nullptr, nullptr, instance_, this);
  };
  left_window_ = create_window();
  right_window_ = create_window();
  top_window_ = create_window();
  bottom_window_ = create_window();
  if (left_window_ == nullptr || right_window_ == nullptr ||
      top_window_ == nullptr || bottom_window_ == nullptr) {
    DestroyWindows();
    UnregisterClassW(kClassName, instance_);
    class_registered_ = false;
    return false;
  }

  visible_ = false;
  const bool config_applied = ApplyConfig(config_);
  visible_ = true;
  if (!config_applied || !InitializeDirect2D()) {
    CleanupDirect2D();
    DestroyWindows();
    UnregisterClassW(kClassName, instance_);
    class_registered_ = false;
    return false;
  }

  if (SetTimer(left_window_, kUpdateTimerId, kUpdateIntervalMs, nullptr) == 0) {
    CleanupDirect2D();
    DestroyWindows();
    UnregisterClassW(kClassName, instance_);
    class_registered_ = false;
    return false;
  }

  UpdatePosition();
  return true;
}

bool CrosshairWindow::InitializeDirect2D() {
  CleanupDirect2D();
  if (left_window_ == nullptr || right_window_ == nullptr ||
      top_window_ == nullptr || bottom_window_ == nullptr) {
    return false;
  }

  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                               &d2d_factory_))) {
    return false;
  }

  const D2D1_RENDER_TARGET_PROPERTIES target_properties =
      D2D1::RenderTargetProperties(
          D2D1_RENDER_TARGET_TYPE_DEFAULT,
          D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                            D2D1_ALPHA_MODE_PREMULTIPLIED));

  auto create_target = [&](HWND window, ID2D1HwndRenderTarget** target) {
    RECT client = {};
    if (!GetClientRect(window, &client)) {
      return false;
    }
    const UINT width = std::max(1L, client.right - client.left);
    const UINT height = std::max(1L, client.bottom - client.top);
    const auto properties = D2D1::HwndRenderTargetProperties(
        window, D2D1::SizeU(width, height));
    return SUCCEEDED(d2d_factory_->CreateHwndRenderTarget(
        target_properties, properties, target));
  };

  if (!create_target(left_window_, &left_target_) ||
      !create_target(right_window_, &right_target_) ||
      !create_target(top_window_, &top_target_) ||
      !create_target(bottom_window_, &bottom_target_) ||
      FAILED(left_target_->CreateSolidColorBrush(ToD2DColor(config_.horizontal),
                                                 &left_brush_)) ||
      FAILED(right_target_->CreateSolidColorBrush(
          ToD2DColor(config_.horizontal), &right_brush_)) ||
      FAILED(top_target_->CreateSolidColorBrush(ToD2DColor(config_.vertical),
                                                &top_brush_)) ||
      FAILED(bottom_target_->CreateSolidColorBrush(
          ToD2DColor(config_.vertical), &bottom_brush_))) {
    CleanupDirect2D();
    return false;
  }
  return true;
}

void CrosshairWindow::CleanupDirect2D() noexcept {
  SafeRelease(left_brush_);
  SafeRelease(right_brush_);
  SafeRelease(top_brush_);
  SafeRelease(bottom_brush_);
  SafeRelease(left_target_);
  SafeRelease(right_target_);
  SafeRelease(top_target_);
  SafeRelease(bottom_target_);
  SafeRelease(d2d_factory_);
}

bool CrosshairWindow::ApplyConfig(const Config& config) {
  config_ = config;
  if (left_window_ == nullptr) {
    return true;
  }

  const BYTE horizontal_alpha = config_.horizontal.alpha.value;
  const BYTE vertical_alpha = config_.vertical.alpha.value;
  if (!SetLayeredWindowAttributes(left_window_, 0, horizontal_alpha,
                                  LWA_ALPHA) ||
      !SetLayeredWindowAttributes(right_window_, 0, horizontal_alpha,
                                  LWA_ALPHA) ||
      !SetLayeredWindowAttributes(top_window_, 0, vertical_alpha, LWA_ALPHA) ||
      !SetLayeredWindowAttributes(bottom_window_, 0, vertical_alpha,
                                  LWA_ALPHA)) {
    return false;
  }

  if (left_brush_ != nullptr) {
    left_brush_->SetColor(ToD2DColor(config_.horizontal));
    right_brush_->SetColor(ToD2DColor(config_.horizontal));
    top_brush_->SetColor(ToD2DColor(config_.vertical));
    bottom_brush_->SetColor(ToD2DColor(config_.vertical));
  }

  last_cursor_pos_ = {LONG_MIN, LONG_MIN};
  UpdatePosition();
  for (HWND window : {left_window_, right_window_, top_window_, bottom_window_}) {
    InvalidateRect(window, nullptr, FALSE);
  }
  return true;
}

void CrosshairWindow::ToggleVisible() {
  SetVisible(!visible_);
}

void CrosshairWindow::SetVisible(bool visible) noexcept {
  if (visible_ == visible) return;
  visible_ = visible;
  last_cursor_pos_ = {LONG_MIN, LONG_MIN};
  if (visible_) {
    UpdatePosition();
  } else {
    SetWindowsVisible(false);
  }
}

void CrosshairWindow::SetWindowsVisible(bool visible) noexcept {
  for (HWND window : {left_window_, right_window_, top_window_, bottom_window_}) {
    if (window != nullptr) {
      ShowWindow(window, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
  }
}

void CrosshairWindow::UpdatePosition() {
  if (!visible_ || left_window_ == nullptr || right_window_ == nullptr ||
      top_window_ == nullptr || bottom_window_ == nullptr) {
    return;
  }

  POINT cursor = {};
  if (!GetCursorPos(&cursor)) {
    return;
  }
  if (cursor.x == last_cursor_pos_.x && cursor.y == last_cursor_pos_.y) {
    return;
  }
  last_cursor_pos_ = cursor;

  const HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONULL);
  MONITORINFO monitor_info = {sizeof(monitor_info)};
  if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitor_info)) {
    SetWindowsVisible(false);
    return;
  }

  // 8K 与高 DPI 屏幕动态无损适配：根据当前显示器物理 DPI 自适应缩放
  const float dpi_scale = config_.auto_dpi ? GetMonitorDpiScale(monitor) : 1.0f;
  const RECT bounds = monitor_info.rcMonitor;

  const int gap = static_cast<int>(std::round(config_.gap * dpi_scale));
  const int horizontal_width = std::max(
      1, static_cast<int>(std::round(config_.horizontal.width * dpi_scale)));
  const int vertical_width = std::max(
      1, static_cast<int>(std::round(config_.vertical.width * dpi_scale)));

  const int horizontal_y = std::clamp(cursor.y - horizontal_width / 2,
                                      bounds.top, bounds.bottom);
  const int vertical_x = std::clamp(cursor.x - vertical_width / 2, bounds.left,
                                    bounds.right);
  const int horizontal_height = std::min(
      horizontal_width, static_cast<int>(bounds.bottom - horizontal_y));
  const int vertical_size =
      std::min(vertical_width, static_cast<int>(bounds.right - vertical_x));
  const int left_end = std::clamp(cursor.x - gap, bounds.left, bounds.right);
  const int right_start = std::clamp(cursor.x + gap, bounds.left, bounds.right);
  const int top_end = std::clamp(cursor.y - gap, bounds.top, bounds.bottom);
  const int bottom_start =
      std::clamp(cursor.y + gap, bounds.top, bounds.bottom);

  HDWP positions = BeginDeferWindowPos(4);
  if (positions == nullptr) {
    return;
  }
  const UINT flags = SWP_NOACTIVATE | SWP_SHOWWINDOW;
  positions = DeferWindowPos(positions, left_window_, HWND_TOPMOST, bounds.left,
                             horizontal_y, left_end - bounds.left,
                             horizontal_height, flags);
  if (positions != nullptr) {
    positions = DeferWindowPos(positions, right_window_, HWND_TOPMOST,
                               right_start, horizontal_y,
                               bounds.right - right_start, horizontal_height,
                               flags);
  }
  if (positions != nullptr) {
    positions = DeferWindowPos(positions, top_window_, HWND_TOPMOST, vertical_x,
                               bounds.top, vertical_size, top_end - bounds.top,
                               flags);
  }
  if (positions != nullptr) {
    positions = DeferWindowPos(positions, bottom_window_, HWND_TOPMOST,
                               vertical_x, bottom_start, vertical_size,
                               bounds.bottom - bottom_start, flags);
  }
  if (positions == nullptr || !EndDeferWindowPos(positions)) {
    SetWindowsVisible(false);
    last_cursor_pos_ = {LONG_MIN, LONG_MIN};
    return;
  }

  for (HWND window : {left_window_, right_window_, top_window_, bottom_window_}) {
    InvalidateRect(window, nullptr, FALSE);
  }
}

HRESULT CrosshairWindow::RenderWindow(
    ID2D1HwndRenderTarget* target,
    ID2D1SolidColorBrush* brush) const noexcept {
  if (target == nullptr || brush == nullptr) {
    return E_INVALIDARG;
  }

  target->BeginDraw();
  target->Clear(D2D1::ColorF(0, 0.0F));
  const D2D1_SIZE_F size = target->GetSize();
  target->FillRectangle(D2D1::RectF(0.0F, 0.0F, size.width, size.height), brush);
  return target->EndDraw();
}

LRESULT CALLBACK CrosshairWindow::WndProc(HWND window, UINT message,
                                          WPARAM w_param,
                                          LPARAM l_param) noexcept {
  CrosshairWindow* self = reinterpret_cast<CrosshairWindow*>(
      GetWindowLongPtrW(window, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
    self = static_cast<CrosshairWindow*>(create->lpCreateParams);
    SetLastError(ERROR_SUCCESS);
    if (SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(self)) == 0 &&
        GetLastError() != ERROR_SUCCESS) {
      return FALSE;
    }
  }

  switch (message) {
    case WM_TIMER:
      if (self != nullptr && w_param == kUpdateTimerId &&
          window == self->left_window_) {
        self->UpdatePosition();
        return 0;
      }
      break;
    case WM_DISPLAYCHANGE:
    case WM_DPICHANGED:
      if (self != nullptr) {
        self->last_cursor_pos_ = {LONG_MIN, LONG_MIN};
        self->UpdatePosition();
        return 0;
      }
      break;
    case WM_PAINT: {
      PAINTSTRUCT paint = {};
      if (BeginPaint(window, &paint) == nullptr) {
        return 0;
      }
      if (self != nullptr) {
        ID2D1HwndRenderTarget* target = nullptr;
        ID2D1SolidColorBrush* brush = nullptr;
        if (window == self->left_window_) {
          target = self->left_target_;
          brush = self->left_brush_;
        } else if (window == self->right_window_) {
          target = self->right_target_;
          brush = self->right_brush_;
        } else if (window == self->top_window_) {
          target = self->top_target_;
          brush = self->top_brush_;
        } else if (window == self->bottom_window_) {
          target = self->bottom_target_;
          brush = self->bottom_brush_;
        }

        if (target != nullptr && brush != nullptr) {
          RECT client = {};
          HRESULT result = E_FAIL;
          if (GetClientRect(window, &client)) {
            const UINT width = std::max(1L, client.right - client.left);
            const UINT height = std::max(1L, client.bottom - client.top);
            result = target->Resize(D2D1::SizeU(width, height));
            if (SUCCEEDED(result)) {
              result = self->RenderWindow(target, brush);
            }
          }
          if (result == D2DERR_RECREATE_TARGET &&
              !self->InitializeDirect2D()) {
            self->SetWindowsVisible(false);
          }
        }
      }
      EndPaint(window, &paint);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_NCDESTROY:
      SetWindowLongPtrW(window, GWLP_USERDATA, 0);
      break;
    default:
      break;
  }
  return DefWindowProcW(window, message, w_param, l_param);
}
