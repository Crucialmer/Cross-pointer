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
  const std::filesystem::path primary_config = exe_dir / L"Cross-pointer.ini";
  const std::filesystem::path legacy_config = exe_dir / L"crosshair.ini";

  // 绾豢鑹叉ā寮忥細浠呭湪绋嬪簭鍚岀洰褰曚笅妫€绱㈡垨鍒涘缓閰嶇疆鏂囦欢锛屾潨缁濆啓鍏ヤ换浣曞閮ㄧ郴缁熺洰褰?  if (IsRegularFile(primary_config)) {
    return primary_config.wstring();
  }
  if (IsRegularFile(legacy_config)) {
    return legacy_config.wstring();
  }
  return primary_config.wstring();
}

bool EnsureConfigExists(const std::wstring& path) {
  if (path.empty()) {
    MessageBoxW(nullptr, L"鏃犳硶纭畾绋嬪簭鎵€鍦ㄧ洰褰曘€?, L"閿欒",
                MB_OK | MB_ICONERROR);
    return false;
  }

  const std::filesystem::path config_path(path);
  if (IsRegularFile(config_path)) return true;

  // 鐩存帴鍦ㄧ▼搴忓綋鍓嶇洰褰曞垱寤洪厤缃枃浠讹紝涓嶄骇鐢熶换浣曞閮ㄦ垨涓存椂鎺㈡祴鏂囦欢
  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_EXISTS) return true;
    MessageBoxW(
        nullptr,
        L"鏃犳硶鍦ㄧ▼搴忓綋鍓嶇洰褰曞垱寤洪厤缃枃浠讹紝璇风‘璁ゆ湰鏂囦欢澶瑰叿澶囧啓鏉冮檺锛堢函缁胯壊妯″紡涓嶅悜绯荤粺鍏朵粬鐩綍鍐欏叆浠讳綍鏂囦欢锛夈€?,
        L"閿欒", MB_OK | MB_ICONERROR);
    return false;
  }

  DWORD written = 0;
  const DWORD size = static_cast<DWORD>(sizeof(kDefaultIni) - 1);
  const bool success = WriteFile(file, kDefaultIni, size, &written, nullptr) &&
                       written == size && FlushFileBuffers(file);
  CloseHandle(file);

  if (!success) {
    DeleteFileW(path.c_str());
    MessageBoxW(nullptr, L"鍐欏叆榛樿閰嶇疆鏂囦欢澶辫触銆?, L"閿欒",
                MB_OK | MB_ICONERROR);
    return false;
  }

  return true;
}