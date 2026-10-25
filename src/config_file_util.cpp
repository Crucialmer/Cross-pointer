#include "./config_file_util.h"

#include <windows.h>

#include <filesystem>
#include <system_error>
#include <vector>

namespace {
std::wstring GetExecutablePath() {
  std::vector<wchar_t> buffer(512);
  for (;;) {
    SetLastError(ERROR_SUCCESS);
    const DWORD length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) return {};
    if (length < buffer.size() - 1 ||
        (length < buffer.size() && GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
      return std::wstring(buffer.data(), length);
    }
    if (buffer.size() >= 32768) return {};
    buffer.resize(std::min<size_t>(buffer.size() * 2, 32768));
  }
}

bool IsRegularFile(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::is_regular_file(path, error) && !error;
}
}  // namespace

std::wstring GetConfigPath() {
  const std::wstring executable = GetExecutablePath();
  if (executable.empty()) return {};

  const std::filesystem::path exe_dir =
      std::filesystem::path(executable).parent_path();
  const std::filesystem::path primary_config = exe_dir / L"CrossPointer.ini";
  const std::filesystem::path legacy_config1 = exe_dir / L"Cross-pointer.ini";
  const std::filesystem::path legacy_config2 = exe_dir / L"crosshair.ini";

  if (IsRegularFile(primary_config)) {
    return primary_config.wstring();
  }
  if (IsRegularFile(legacy_config1)) {
    return legacy_config1.wstring();
  }
  if (IsRegularFile(legacy_config2)) {
    return legacy_config2.wstring();
  }
  return primary_config.wstring();
}

bool EnsureConfigExists(const std::wstring& path) {
  if (path.empty()) {
    MessageBoxW(nullptr, L"无法确定程序所在目录。", L"错误",
                MB_OK | MB_ICONERROR);
    return false;
  }

  const std::filesystem::path config_path(path);
  if (IsRegularFile(config_path)) return true;

  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_EXISTS) return true;
    MessageBoxW(
        nullptr,
        L"无法在程序当前目录创建配置文件，请确认本文件夹具备写权限（纯绿色模式不向系统其他目录写入任何文件）。",
        L"错误", MB_OK | MB_ICONERROR);
    return false;
  }

  DWORD written = 0;
  const DWORD size = static_cast<DWORD>(sizeof(kDefaultIni) - 1);
  const bool success = WriteFile(file, kDefaultIni, size, &written, nullptr) &&
                       written == size && FlushFileBuffers(file);
  CloseHandle(file);

  if (!success) {
    DeleteFileW(path.c_str());
    MessageBoxW(nullptr, L"写入默认配置文件失败。", L"错误",
                MB_OK | MB_ICONERROR);
    return false;
  }

  return true;
}
