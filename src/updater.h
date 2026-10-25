#pragma once

#include <windows.h>
#include <string>

namespace updater {

inline constexpr wchar_t kCurrentVersion[] = L"v1.2";

struct UpdateResult {
  bool success = false;
  bool has_update = false;
  std::wstring latest_version;
  std::wstring release_url;
  std::wstring changelog;
  std::wstring error_msg;
};

// 同步检查更新（供工作线程调用）
[[nodiscard]] UpdateResult CheckUpdateSync() noexcept;

// 异步检查更新并显示 UI 提示
// silent_if_latest: 若为 true 且已是最新版或出错，不弹窗打扰用户
void CheckUpdateAsync(HWND parent_hwnd, bool silent_if_latest);

}  // namespace updater
