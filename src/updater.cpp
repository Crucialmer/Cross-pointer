#include "./updater.h"

#include <shellapi.h>
#include <wininet.h>

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

namespace updater {
namespace {

std::wstring Utf8ToWide(const std::string& str) {
  if (str.empty()) return L"";
  const int len = MultiByteToWideChar(CP_UTF8, 0, str.data(),
                                      static_cast<int>(str.size()), nullptr, 0);
  if (len <= 0) return L"";
  std::wstring res(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                      res.data(), len);
  return res;
}

std::wstring ExtractJsonField(const std::wstring& json,
                              const std::wstring& key) {
  const std::wstring target = L"\"" + key + L"\"";
  const size_t pos = json.find(target);
  if (pos == std::wstring::npos) return L"";

  const size_t colon = json.find(L':', pos + target.size());
  if (colon == std::wstring::npos) return L"";

  const size_t start_quote = json.find(L'\"', colon);
  if (start_quote == std::wstring::npos) return L"";

  std::wstring result;
  bool escape = false;
  for (size_t i = start_quote + 1; i < json.size(); ++i) {
    const wchar_t c = json[i];
    if (escape) {
      if (c == L'n') {
        result += L'\n';
      } else if (c == L'r') {
        result += L'\r';
      } else if (c == L't') {
        result += L'\t';
      } else if (c == L'\"') {
        result += L'\"';
      } else if (c == L'\\') {
        result += L'\\';
      } else {
        result += c;
      }
      escape = false;
    } else {
      if (c == L'\\') {
        escape = true;
      } else if (c == L'\"') {
        break;
      } else {
        result += c;
      }
    }
  }
  return result;
}

std::wstring ExtractAssetUrl(const std::wstring& json,
                             const std::wstring& asset_suffix) {
  size_t search_pos = 0;
  while ((search_pos = json.find(L"\"name\"", search_pos)) != std::wstring::npos) {
    const size_t colon = json.find(L':', search_pos);
    if (colon == std::wstring::npos) break;
    const size_t name_start = json.find(L'\"', colon);
    if (name_start == std::wstring::npos) break;
    const size_t name_end = json.find(L'\"', name_start + 1);
    if (name_end == std::wstring::npos) break;

    const std::wstring name = json.substr(name_start + 1, name_end - name_start - 1);
    if (name.size() >= asset_suffix.size()) {
      std::wstring name_lower = name;
      std::wstring suffix_lower = asset_suffix;
      std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::towlower);
      std::transform(suffix_lower.begin(), suffix_lower.end(), suffix_lower.begin(), ::towlower);

      if (name_lower.compare(name_lower.size() - suffix_lower.size(), suffix_lower.size(), suffix_lower) == 0) {
        const size_t url_key = json.find(L"\"browser_download_url\"", name_end);
        if (url_key != std::wstring::npos) {
          const size_t url_colon = json.find(L':', url_key);
          if (url_colon != std::wstring::npos) {
            const size_t url_start = json.find(L'\"', url_colon);
            if (url_start != std::wstring::npos) {
              const size_t url_end = json.find(L'\"', url_start + 1);
              if (url_end != std::wstring::npos) {
                return json.substr(url_start + 1, url_end - url_start - 1);
              }
            }
          }
        }
      }
    }
    search_pos = name_end + 1;
  }
  return L"";
}

bool DownloadFileToDisk(const std::wstring& url_str, const std::wstring& dest_path) noexcept {
  HINTERNET net = InternetOpenW(L"CrossPointer-Updater",
                                INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (net == nullptr) return false;

  DWORD timeout_ms = 30000;
  InternetSetOptionW(net, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout_ms, sizeof(timeout_ms));
  InternetSetOptionW(net, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout_ms, sizeof(timeout_ms));

  constexpr wchar_t kHeaders[] = L"User-Agent: CrossPointer-App\r\n";
  HINTERNET url = InternetOpenUrlW(
      net, url_str.c_str(), kHeaders, static_cast<DWORD>(-1),
      INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);

  if (url == nullptr) {
    InternetCloseHandle(net);
    return false;
  }

  HANDLE file = CreateFileW(dest_path.c_str(), GENERIC_WRITE, 0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    InternetCloseHandle(url);
    InternetCloseHandle(net);
    return false;
  }

  char buffer[8192];
  DWORD bytes_read = 0;
  DWORD bytes_written = 0;
  bool success = true;

  while (InternetReadFile(url, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
    if (!WriteFile(file, buffer, bytes_read, &bytes_written, nullptr) ||
        bytes_written != bytes_read) {
      success = false;
      break;
    }
  }

  CloseHandle(file);
  InternetCloseHandle(url);
  InternetCloseHandle(net);

  if (!success) {
    DeleteFileW(dest_path.c_str());
  }
  return success;
}

}  // namespace

void CleanupOldExecutable() noexcept {
  std::wstring module_path(MAX_PATH, L'\0');
  DWORD len = GetModuleFileNameW(nullptr, module_path.data(),
                                 static_cast<DWORD>(module_path.size()));
  if (len == 0) return;
  module_path.resize(len);

  const size_t last_slash = module_path.find_last_of(L"\\/");
  if (last_slash == std::wstring::npos) return;
  const std::wstring dir = module_path.substr(0, last_slash + 1);

  DeleteFileW((dir + L"CrossPointer.old.exe").c_str());
  DeleteFileW((dir + L"CrossPointer.new.exe").c_str());
  DeleteFileW((dir + L"CrossPointer.update.zip").c_str());
  DeleteFileW((dir + L"CrossPointer\\CrossPointer.exe").c_str());
  DeleteFileW((dir + L"CrossPointer\\README.md").c_str());
  DeleteFileW((dir + L"CrossPointer\\QuickStart_Guide.txt").c_str());
  RemoveDirectoryW((dir + L"CrossPointer").c_str());
}

UpdateResult CheckUpdateSync() noexcept {
  UpdateResult res;
  HINTERNET net = InternetOpenW(L"CrossPointer-Updater",
                                INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (net == nullptr) {
    res.error_msg = L"无法初始化网络组件 (InternetOpen 失败)";
    return res;
  }

  DWORD timeout_ms = 8000;
  InternetSetOptionW(net, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout_ms,
                     sizeof(timeout_ms));
  InternetSetOptionW(net, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout_ms,
                     sizeof(timeout_ms));

  constexpr wchar_t kHeaders[] =
      L"User-Agent: CrossPointer-App\r\nAccept: "
      L"application/vnd.github+json\r\n";
  constexpr wchar_t kUrl[] =
      L"https://api.github.com/repos/Crucialmer/CrossPointer/releases/latest";

  HINTERNET url = InternetOpenUrlW(
      net, kUrl, kHeaders, static_cast<DWORD>(-1),
      INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
      0);

  if (url == nullptr) {
    InternetCloseHandle(net);
    res.error_msg = L"无法连接到 GitHub 检查更新，请检查网络或代理设置。";
    return res;
  }

  std::string response;
  char buffer[4096];
  DWORD bytes_read = 0;
  while (InternetReadFile(url, buffer, sizeof(buffer), &bytes_read) &&
         bytes_read > 0) {
    response.append(buffer, bytes_read);
  }

  InternetCloseHandle(url);
  InternetCloseHandle(net);

  if (response.empty()) {
    res.error_msg = L"GitHub 接口返回空数据。";
    return res;
  }

  const std::wstring json = Utf8ToWide(response);
  res.latest_version = ExtractJsonField(json, L"tag_name");
  res.release_url = ExtractJsonField(json, L"html_url");
  res.changelog = ExtractJsonField(json, L"body");
  res.exe_download_url = ExtractAssetUrl(json, L".exe");
  res.zip_download_url = ExtractAssetUrl(json, L".zip");

  if (res.latest_version.empty()) {
    res.error_msg = L"解析 GitHub 版本响应失败。";
    return res;
  }

  res.success = true;
  res.has_update = (res.latest_version != kCurrentVersion);
  return res;
}

bool PerformDirectUpdate(const UpdateResult& update_info,
                         std::wstring* error_out) noexcept {
  std::wstring module_path(MAX_PATH, L'\0');
  DWORD len = GetModuleFileNameW(nullptr, module_path.data(),
                                 static_cast<DWORD>(module_path.size()));
  if (len == 0) {
    if (error_out) *error_out = L"无法获取当前程序路径。";
    return false;
  }
  module_path.resize(len);

  const size_t last_slash = module_path.find_last_of(L"\\/");
  if (last_slash == std::wstring::npos) {
    if (error_out) *error_out = L"解析程序目录失败。";
    return false;
  }
  const std::wstring dir = module_path.substr(0, last_slash + 1);
  const std::wstring current_exe = module_path;
  const std::wstring old_exe = dir + L"CrossPointer.old.exe";
  const std::wstring new_exe = dir + L"CrossPointer.new.exe";
  const std::wstring zip_file = dir + L"CrossPointer.update.zip";

  // 1. 尝试直接下载 .exe 文件
  bool download_ok = false;
  if (!update_info.exe_download_url.empty()) {
    download_ok = DownloadFileToDisk(update_info.exe_download_url, new_exe);
  }

  // 2. 若直接下载 exe 失败或未提供，回退到下载 zip 并自动调用系统内置 tar.exe 解压
  if (!download_ok && !update_info.zip_download_url.empty()) {
    if (DownloadFileToDisk(update_info.zip_download_url, zip_file)) {
      std::wstring extract_cmd = L"tar.exe -xf \"" + zip_file + L"\" -C \"" + dir + L"\"";
      STARTUPINFOW si = {sizeof(si)};
      si.dwFlags = STARTF_USESHOWWINDOW;
      si.wShowWindow = SW_HIDE;
      PROCESS_INFORMATION pi = {};
      if (CreateProcessW(nullptr, extract_cmd.data(), nullptr, nullptr, FALSE,
                         CREATE_NO_WINDOW, nullptr, dir.c_str(), &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 15000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
      }
      DeleteFileW(zip_file.c_str());

      const std::wstring extracted_sub = dir + L"CrossPointer\\CrossPointer.exe";
      if (GetFileAttributesW(extracted_sub.c_str()) != INVALID_FILE_ATTRIBUTES) {
        MoveFileExW(extracted_sub.c_str(), new_exe.c_str(), MOVEFILE_REPLACE_EXISTING);
        MoveFileExW((dir + L"CrossPointer\\README.md").c_str(), (dir + L"README.md").c_str(), MOVEFILE_REPLACE_EXISTING);
        MoveFileExW((dir + L"CrossPointer\\QuickStart_Guide.txt").c_str(), (dir + L"QuickStart_Guide.txt").c_str(), MOVEFILE_REPLACE_EXISTING);
        RemoveDirectoryW((dir + L"CrossPointer").c_str());
        download_ok = true;
      }
    }
  }

  // 3. 校验下载的二进制文件完整性与 PE 签名 (MZ 头)
  if (!download_ok || GetFileAttributesW(new_exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
    if (error_out) *error_out = L"下载更新文件失败，请检查网络连接或代理设置。";
    return false;
  }

  HANDLE h_check = CreateFileW(new_exe.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, 0, nullptr);
  if (h_check == INVALID_HANDLE_VALUE) {
    if (error_out) *error_out = L"无法读取下载的新版本可执行文件。";
    return false;
  }
  char mz[2] = {0};
  DWORD read_bytes = 0;
  ReadFile(h_check, mz, 2, &read_bytes, nullptr);
  const DWORD file_size = GetFileSize(h_check, nullptr);
  CloseHandle(h_check);

  if (read_bytes < 2 || mz[0] != 'M' || mz[1] != 'Z' || file_size < 100 * 1024) {
    DeleteFileW(new_exe.c_str());
    if (error_out) *error_out = L"下载的新版本文件校验失败（非合法的 Windows 可执行程序）。";
    return false;
  }

  // 4. 原地热替换：重命名当前正在运行的 EXE 为 .old.exe，再将 .new.exe 替换为当前 EXE
  DeleteFileW(old_exe.c_str());
  if (!MoveFileExW(current_exe.c_str(), old_exe.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    DeleteFileW(new_exe.c_str());
    if (error_out) *error_out = L"无法重命名当前运行程序，可能被杀毒软件或文件系统锁定。";
    return false;
  }

  if (!MoveFileExW(new_exe.c_str(), current_exe.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    MoveFileExW(old_exe.c_str(), current_exe.c_str(), MOVEFILE_REPLACE_EXISTING);
    if (error_out) *error_out = L"无法将新版本写入目标路径。";
    return false;
  }

  // 5. 启动新版本（附带 --update-restart 参数）并安全退出当前进程
  const HINSTANCE h_inst = ShellExecuteW(nullptr, L"open", current_exe.c_str(),
                                         L"--update-restart", dir.c_str(), SW_SHOW);
  if (reinterpret_cast<INT_PTR>(h_inst) <= 32) {
    if (error_out) *error_out = L"新版本程序启动失败。";
    return false;
  }

  ExitProcess(0);
  return true;
}

void CheckUpdateAsync(HWND parent_hwnd, bool silent_if_latest) {
  std::thread([parent_hwnd, silent_if_latest]() {
    const UpdateResult res = CheckUpdateSync();

    if (res.has_update) {
      std::wstring msg =
          L"🎉 发现 CrossPointer 新版本 " + res.latest_version + L"！\n\n" +
          L"当前运行版本：" + std::wstring(kCurrentVersion) + L"\n\n";

      if (!res.changelog.empty()) {
        std::wstring preview = res.changelog;
        if (preview.size() > 300) {
          preview = preview.substr(0, 300) + L"\n...（更多更新详情见 Release 页面）";
        }
        msg += L"【更新日志】\n" + preview + L"\n\n";
      }

      msg +=
          L"💡 请选择更新方式：\n\n"
          L"• 点击【是 (Yes)】：🚀 一键直接在本地自动更新并重启（推荐，无需前往浏览器，静默无感升级）\n\n"
          L"• 点击【否 (No)】：🌐 前往 GitHub Releases 网页手动下载\n\n"
          L"• 点击【取消 (Cancel)】：暂不更新";

      const int choice = MessageBoxW(parent_hwnd, msg.c_str(), L"发现新版本 - CrossPointer",
                                     MB_YESNOCANCEL | MB_ICONINFORMATION);
      if (choice == IDYES) {
        MessageBoxW(parent_hwnd,
                    L"正在后台高速下载最新版本...\n\n"
                    L"下载完成后软件将自动重启并无缝生效，点击「确定」立即开始更新。",
                    L"准备更新 - CrossPointer", MB_OK | MB_ICONINFORMATION);

        std::wstring err;
        if (!PerformDirectUpdate(res, &err)) {
          std::wstring fail_msg = L"自动更新失败：" + err + L"\n\n是否转为前往 GitHub 页面手动下载？";
          if (MessageBoxW(parent_hwnd, fail_msg.c_str(), L"更新失败 - CrossPointer",
                          MB_YESNO | MB_ICONERROR) == IDYES) {
            ShellExecuteW(nullptr, L"open", res.release_url.c_str(), nullptr, nullptr, SW_SHOW);
          }
        }
      } else if (choice == IDNO) {
        const wchar_t* open_url =
            res.release_url.empty()
                ? L"https://github.com/Crucialmer/CrossPointer/releases/latest"
                : res.release_url.c_str();
        ShellExecuteW(nullptr, L"open", open_url, nullptr, nullptr, SW_SHOW);
      }
    } else {
      if (!silent_if_latest) {
        if (res.success) {
          std::wstring msg =
              L"您当前使用的 CrossPointer (" + std::wstring(kCurrentVersion) +
              L") 已是最新版本！\n\n已具备最新的 macOS 26 "
              L"设计美学、本地一键直接更新、8K 高分屏缩放与开机自启等全部特性。";
          MessageBoxW(parent_hwnd, msg.c_str(), L"检查更新 - CrossPointer",
                      MB_OK | MB_ICONINFORMATION);
        } else {
          MessageBoxW(parent_hwnd, res.error_msg.c_str(),
                      L"检查更新 - CrossPointer", MB_OK | MB_ICONWARNING);
        }
      }
    }
  }).detach();
}

}  // namespace updater
