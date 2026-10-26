#pragma once

#include <windows.h>
#include <string>

namespace updater {

inline constexpr wchar_t kCurrentVersion[] = L"v1.4";

struct UpdateResult {
  bool success = false;
  bool has_update = false;
  std::wstring latest_version;
  std::wstring release_url;
  std::wstring changelog;
  std::wstring exe_download_url;
  std::wstring zip_download_url;
  std::wstring error_msg;
};

// 同步检查更新（供工作线程调用）
[[nodiscard]] UpdateResult CheckUpdateSync() noexcept;

// 同步执行一键直接下载并重启更新（无需跳转浏览器）
[[nodiscard]] bool PerformDirectUpdate(const UpdateResult& update_info,
                                      std::wstring* error_out = nullptr) noexcept;

// 异步检查更新并显示 UI 提示与一键直接升级
// silent_if_latest: 若为 true 且已是最新版或出错，不弹窗打扰用户
void CheckUpdateAsync(HWND parent_hwnd, bool silent_if_latest);

// 启动时清理残留的旧版本临时备份文件 (.old.exe 等)
void CleanupOldExecutable() noexcept;

}  // namespace updater
