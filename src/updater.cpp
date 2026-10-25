#include "./updater.h"

#include <shellapi.h>
#include <wininet.h>

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

}  // namespace

UpdateResult CheckUpdateSync() noexcept {
  UpdateResult res;
  HINTERNET net = InternetOpenW(L"CrossPointer-Updater",
                                INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (net == nullptr) {
    res.error_msg = L"无法初始化网络组件 (InternetOpen 失败)";
    return res;
  }

  // 设置 8 秒超时，避免网络慢时等待过久
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
      net, kUrl, kHeaders, -1,
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

  if (res.latest_version.empty()) {
    res.error_msg = L"解析 GitHub 版本响应失败。";
    return res;
  }

  res.success = true;
  // 版本号比对：若云端 tag 与当前版本不相同则视作有新版本
  res.has_update = (res.latest_version != kCurrentVersion);
  return res;
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
        if (preview.size() > 400) {
          preview = preview.substr(0, 400) + L"\n...（更多更新详情见 Release 页面）";
        }
        msg += L"【更新日志】\n" + preview + L"\n\n";
      }

      msg += L"是否立即前往 GitHub 下载最新版便携压缩包？";

      if (MessageBoxW(parent_hwnd, msg.c_str(), L"软件更新 - CrossPointer",
                      MB_YESNO | MB_ICONINFORMATION) == IDYES) {
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
              L"设计美学、系统托盘、8K 高分屏缩放与开机自启等全部特性。";
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
