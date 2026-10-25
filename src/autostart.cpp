#include "./autostart.h"

#include <string>
#include <vector>

namespace autostart {
namespace {
constexpr wchar_t kRunSubKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kAppName[] = L"CrossPointer";
}  // namespace

bool IsAutoStartEnabled() noexcept {
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubKey, 0, KEY_QUERY_VALUE, &key) !=
      ERROR_SUCCESS) {
    return false;
  }

  DWORD type = 0;
  DWORD size = 0;
  const LSTATUS status =
      RegQueryValueExW(key, kAppName, nullptr, &type, nullptr, &size);
  RegCloseKey(key);
  return (status == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ));
}

bool SetAutoStart(bool enable) noexcept {
  if (enable) {
    std::wstring module_path(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(nullptr, module_path.data(),
                                   static_cast<DWORD>(module_path.size()));
    if (len == 0) return false;
    while (len == module_path.size()) {
      module_path.resize(module_path.size() * 2);
      len = GetModuleFileNameW(nullptr, module_path.data(),
                               static_cast<DWORD>(module_path.size()));
    }
    module_path.resize(len);

    // 格式化为双引号包裹的标准自启动命令行
    const std::wstring cmd = L"\"" + module_path + L"\"";

    const LSTATUS status = RegSetKeyValueW(
        HKEY_CURRENT_USER, kRunSubKey, kAppName, REG_SZ, cmd.c_str(),
        static_cast<DWORD>((cmd.length() + 1) * sizeof(wchar_t)));
    return status == ERROR_SUCCESS;
  } else {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubKey, 0, KEY_SET_VALUE, &key) !=
        ERROR_SUCCESS) {
      return true;  // 如果键不存在，视同已删除成功
    }
    const LSTATUS status = RegDeleteValueW(key, kAppName);
    RegCloseKey(key);
    return (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND);
  }
}

}  // namespace autostart
