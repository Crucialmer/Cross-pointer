#pragma once

#include <windows.h>

namespace autostart {

// 检查当前 CrossPointer 是否已加入开机自启动
[[nodiscard]] bool IsAutoStartEnabled() noexcept;

// 设置或取消开机自启动
[[nodiscard]] bool SetAutoStart(bool enable) noexcept;

}  // namespace autostart
