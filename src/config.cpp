#include "./config.h"

#include <cwctype>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
std::wstring TrimAndLower(std::wstring value) {
  const auto whitespace = [](wchar_t character) { return iswspace(character); };
  while (!value.empty() && whitespace(value.front())) value.erase(value.begin());
  while (!value.empty() && whitespace(value.back())) value.pop_back();
  for (wchar_t& character : value) {
    character = static_cast<wchar_t>(towlower(character));
  }
  return value;
}

std::optional<std::wstring> ReadString(const wchar_t* section,
                                       const wchar_t* key,
                                       const wchar_t* filename,
                                       bool required = true) {
  constexpr wchar_t kMissing[] = L"{92D52289-BC0B-4E11-9FC6-55B21A223BA7}";
  std::wstring buffer(256, L'\0');
  const DWORD length = GetPrivateProfileStringW(
      section, key, kMissing, buffer.data(), static_cast<DWORD>(buffer.size()),
      filename);
  if (length >= buffer.size() - 1) return std::nullopt;
  buffer.resize(length);
  if (buffer == kMissing) {
    return required ? std::nullopt : std::optional<std::wstring>{L""};
  }
  return TrimAndLower(std::move(buffer));
}

std::optional<unsigned int> ParseUnsigned(std::wstring_view text,
                                          unsigned int minimum,
                                          unsigned int maximum) {
  if (text.empty()) return std::nullopt;
  unsigned long long value = 0;
  for (const wchar_t character : text) {
    if (character < L'0' || character > L'9') return std::nullopt;
    const unsigned int digit = static_cast<unsigned int>(character - L'0');
    if (value > (std::numeric_limits<unsigned int>::max() - digit) / 10ULL) {
      return std::nullopt;
    }
    value = value * 10ULL + digit;
  }
  if (value < minimum || value > maximum) return std::nullopt;
  return static_cast<unsigned int>(value);
}

std::optional<unsigned int> ReadUnsigned(const wchar_t* section,
                                         const wchar_t* key,
                                         const wchar_t* filename,
                                         unsigned int minimum,
                                         unsigned int maximum) {
  const auto value = ReadString(section, key, filename);
  return value ? ParseUnsigned(*value, minimum, maximum) : std::nullopt;
}
}  // namespace

bool IsSystemInLightTheme() noexcept {
  DWORD light_theme = 1;
  DWORD size = sizeof(light_theme);
  const LSTATUS status = RegGetValueW(
      HKEY_CURRENT_USER,
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
      L"AppsUseLightTheme",
      RRF_RT_REG_DWORD,
      nullptr,
      &light_theme,
      &size);
  if (status == ERROR_SUCCESS) {
    return light_theme != 0;
  }
  return true;
}

Config GetThemedConfig(const Config& base, bool is_light) noexcept {
  Config themed = base;
  if (base.auto_theme) {
    const LineColor& active = is_light ? base.light_theme : base.dark_theme;
    themed.horizontal.r = active.r;
    themed.horizontal.g = active.g;
    themed.horizontal.b = active.b;
    themed.horizontal.alpha = active.alpha;

    themed.vertical.r = active.r;
    themed.vertical.g = active.g;
    themed.vertical.b = active.b;
    themed.vertical.alpha = active.alpha;
  }
  return themed;
}

bool Config::Load(const wchar_t* filename) {
  if (filename == nullptr || *filename == L'\0') return false;
  const DWORD attributes = GetFileAttributesW(filename);
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return false;
  }

  Config loaded;
  const auto gap_value = ReadUnsigned(L"Crosshair", L"Gap", filename, 0, 400);
  const auto horizontal_width =
      ReadUnsigned(L"Horizontal", L"Width", filename, 1, 200);
  const auto horizontal_r =
      ReadUnsigned(L"Horizontal", L"R", filename, 0, 255);
  const auto horizontal_g =
      ReadUnsigned(L"Horizontal", L"G", filename, 0, 255);
  const auto horizontal_b =
      ReadUnsigned(L"Horizontal", L"B", filename, 0, 255);
  const auto horizontal_alpha =
      ReadUnsigned(L"Horizontal", L"Alpha", filename, 0, 255);
  const auto vertical_width =
      ReadUnsigned(L"Vertical", L"Width", filename, 1, 200);
  const auto vertical_r = ReadUnsigned(L"Vertical", L"R", filename, 0, 255);
  const auto vertical_g = ReadUnsigned(L"Vertical", L"G", filename, 0, 255);
  const auto vertical_b = ReadUnsigned(L"Vertical", L"B", filename, 0, 255);
  const auto vertical_alpha =
      ReadUnsigned(L"Vertical", L"Alpha", filename, 0, 255);
  const auto hide_key = ReadString(L"Hide_Show_Hotkey", L"VK", filename);
  const auto hide_mod =
      ReadString(L"Hide_Show_Hotkey", L"Mod", filename, false);
  const auto exit_key = ReadString(L"Exit_Hotkey", L"VK", filename);
  const auto exit_mod = ReadString(L"Exit_Hotkey", L"Mod", filename, false);

  if (!gap_value || !horizontal_width || !horizontal_r || !horizontal_g ||
      !horizontal_b || !horizontal_alpha || !vertical_width || !vertical_r ||
      !vertical_g || !vertical_b || !vertical_alpha || !hide_key || !hide_mod ||
      !exit_key || !exit_mod) {
    return false;
  }

  // 主题配置项 (默认为跟随系统)
  const auto auto_follow =
      ReadUnsigned(L"Theme", L"AutoFollowSystem", filename, 0, 1);
  loaded.auto_theme = auto_follow.value_or(1) != 0;

  // 高 DPI / 8K 分辨率自适应配置项 (默认为开启)
  const auto auto_dpi_val =
      ReadUnsigned(L"Display", L"AutoDpi", filename, 0, 1);
  loaded.auto_dpi = auto_dpi_val.value_or(1) != 0;

  // 自动检查更新配置项 (默认为开启)
  const auto auto_update_val =
      ReadUnsigned(L"Update", L"AutoCheckUpdate", filename, 0, 1);
  loaded.auto_check_update = auto_update_val.value_or(1) != 0;

  // 启动时是否默认显示十字线配置项 (1 = 默认显示, 0 = 启动后静默常驻托盘)
  const auto show_startup_val =
      ReadUnsigned(L"General", L"ShowOnStartup", filename, 0, 1);
  loaded.show_on_startup = show_startup_val.value_or(1) != 0;

  // 浅色主题预设 (可选)
  const auto light_r = ReadUnsigned(L"Light_Mode", L"R", filename, 0, 255);
  const auto light_g = ReadUnsigned(L"Light_Mode", L"G", filename, 0, 255);
  const auto light_b = ReadUnsigned(L"Light_Mode", L"B", filename, 0, 255);
  const auto light_alpha =
      ReadUnsigned(L"Light_Mode", L"Alpha", filename, 0, 255);
  if (light_r && light_g && light_b && light_alpha) {
    loaded.light_theme = {ColorChannel(*light_r), ColorChannel(*light_g),
                          ColorChannel(*light_b), ColorChannel(*light_alpha)};
  }

  // 深色主题预设 (可选)
  const auto dark_r = ReadUnsigned(L"Dark_Mode", L"R", filename, 0, 255);
  const auto dark_g = ReadUnsigned(L"Dark_Mode", L"G", filename, 0, 255);
  const auto dark_b = ReadUnsigned(L"Dark_Mode", L"B", filename, 0, 255);
  const auto dark_alpha =
      ReadUnsigned(L"Dark_Mode", L"Alpha", filename, 0, 255);
  if (dark_r && dark_g && dark_b && dark_alpha) {
    loaded.dark_theme = {ColorChannel(*dark_r), ColorChannel(*dark_g),
                         ColorChannel(*dark_b), ColorChannel(*dark_alpha)};
  }

  loaded.gap = *gap_value;
  loaded.horizontal.width = *horizontal_width;
  loaded.horizontal.r = ColorChannel(static_cast<int>(*horizontal_r));
  loaded.horizontal.g = ColorChannel(static_cast<int>(*horizontal_g));
  loaded.horizontal.b = ColorChannel(static_cast<int>(*horizontal_b));
  loaded.horizontal.alpha = ColorChannel(static_cast<int>(*horizontal_alpha));
  loaded.vertical.width = *vertical_width;
  loaded.vertical.r = ColorChannel(static_cast<int>(*vertical_r));
  loaded.vertical.g = ColorChannel(static_cast<int>(*vertical_g));
  loaded.vertical.b = ColorChannel(static_cast<int>(*vertical_b));
  loaded.vertical.alpha = ColorChannel(static_cast<int>(*vertical_alpha));

  const auto parsed_hide_key = ParseVK(hide_key->c_str(), 'h');
  const auto parsed_exit_key = ParseVK(exit_key->c_str(), 'e');
  loaded.hotkey_h_s.vk = parsed_hide_key.first;
  loaded.hotkey_h_s.mod = ParseMod(hide_mod->c_str(), parsed_hide_key.second);
  loaded.hotkey_exit.vk = parsed_exit_key.first;
  loaded.hotkey_exit.mod = ParseMod(exit_mod->c_str(), parsed_exit_key.second);
  loaded.ClampAll();

  // 支持同一快捷键复用兼具显示与退出，不再拒绝相同热键
  *this = loaded;
  return true;
}

bool Config::Save(const wchar_t* filename) const {
  if (filename == nullptr || *filename == L'\0') return false;

  std::wostringstream ss;
  ss << L"[General]\n";
  ss << L"# 软件启动时是否立即显示十字辅助线 (1 = 默认显示, 0 = 启动后静默常驻托盘，需按热键呼出)\n";
  ss << L"ShowOnStartup = " << (show_on_startup ? 1 : 0) << L"\n\n";

  ss << L"[Theme]\n";
  ss << L"# 1 = 跟随系统自动切换黑白模式 (推荐); 0 = 关闭自动切换\n";
  ss << L"AutoFollowSystem = " << (auto_theme ? 1 : 0) << L"\n\n";

  ss << L"[Display]\n";
  ss << L"# 是否开启高 DPI / 4K / 8K 分辨率自适应缩放 (1 = 开启, 0 = 关闭)\n";
  ss << L"AutoDpi = " << (auto_dpi ? 1 : 0) << L"\n\n";

  ss << L"[Update]\n";
  ss << L"# 启动时自动检查更新 (1 = 开启, 0 = 关闭)\n";
  ss << L"AutoCheckUpdate = " << (auto_check_update ? 1 : 0) << L"\n\n";

  ss << L"[Light_Mode]\n";
  ss << L"# 浅色模式专属色彩（白底文档/表格）\n";
  ss << L"R = " << static_cast<int>(light_theme.r.value) << L"\n";
  ss << L"G = " << static_cast<int>(light_theme.g.value) << L"\n";
  ss << L"B = " << static_cast<int>(light_theme.b.value) << L"\n";
  ss << L"Alpha = " << static_cast<int>(light_theme.alpha.value) << L"\n\n";

  ss << L"[Dark_Mode]\n";
  ss << L"# 深色模式专属色彩（深色代码/夜间背景）\n";
  ss << L"R = " << static_cast<int>(dark_theme.r.value) << L"\n";
  ss << L"G = " << static_cast<int>(dark_theme.g.value) << L"\n";
  ss << L"B = " << static_cast<int>(dark_theme.b.value) << L"\n";
  ss << L"Alpha = " << static_cast<int>(dark_theme.alpha.value) << L"\n\n";

  ss << L"[Crosshair]\n";
  ss << L"Gap = " << gap << L"\n\n";

  ss << L"[Horizontal]\n";
  ss << L"Width = " << horizontal.width << L"\n";
  ss << L"R = " << static_cast<int>(horizontal.r.value) << L"\n";
  ss << L"G = " << static_cast<int>(horizontal.g.value) << L"\n";
  ss << L"B = " << static_cast<int>(horizontal.b.value) << L"\n";
  ss << L"Alpha = " << static_cast<int>(horizontal.alpha.value) << L"\n\n";

  ss << L"[Vertical]\n";
  ss << L"Width = " << vertical.width << L"\n";
  ss << L"R = " << static_cast<int>(vertical.r.value) << L"\n";
  ss << L"G = " << static_cast<int>(vertical.g.value) << L"\n";
  ss << L"B = " << static_cast<int>(vertical.b.value) << L"\n";
  ss << L"Alpha = " << static_cast<int>(vertical.alpha.value) << L"\n\n";

  ss << L"[Hide_Show_Hotkey]\n";
  ss << L"Mod = " << ModToString(hotkey_h_s.mod) << L"\n";
  ss << L"VK = " << VKToString(hotkey_h_s.vk) << L"\n\n";

  ss << L"[Exit_Hotkey]\n";
  ss << L"Mod = " << ModToString(hotkey_exit.mod) << L"\n";
  ss << L"VK = " << VKToString(hotkey_exit.vk) << L"\n";

  const std::wstring content = ss.str();
  // 以 UTF-8 编码写入文件
  HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;

  const int utf8_len = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1,
                                          nullptr, 0, nullptr, nullptr);
  if (utf8_len <= 1) {
    CloseHandle(file);
    return false;
  }

  std::string utf8_str(utf8_len - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, utf8_str.data(),
                      utf8_len - 1, nullptr, nullptr);

  DWORD written = 0;
  const BOOL ok = WriteFile(file, utf8_str.data(),
                            static_cast<DWORD>(utf8_str.size()), &written,
                            nullptr);
  CloseHandle(file);
  return ok != FALSE;
}

std::wstring Config::ModToString(unsigned int mod) {
  std::wstring res;
  if ((mod & MOD_CONTROL) != 0) res += L"Ctrl,";
  if ((mod & MOD_ALT) != 0) res += L"Alt,";
  if ((mod & MOD_WIN) != 0) res += L"Win,";
  if ((mod & MOD_SHIFT) != 0) res += L"Shift,";
  if (!res.empty() && res.back() == L',') res.pop_back();
  return res;
}

std::wstring Config::VKToString(unsigned int vk) {
  if (vk >= L'A' && vk <= L'Z') return std::wstring(1, static_cast<wchar_t>(vk));
  if (vk >= L'0' && vk <= L'9') return std::wstring(1, static_cast<wchar_t>(vk));
  if (vk >= VK_F1 && vk <= VK_F24) return L"F" + std::to_wstring(vk - VK_F1 + 1);
  if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return L"Numpad" + std::to_wstring(vk - VK_NUMPAD0);

  switch (vk) {
    case VK_SPACE: return L"Space";
    case VK_TAB: return L"Tab";
    case VK_RETURN: return L"Enter";
    case VK_ESCAPE: return L"Escape";
    case VK_BACK: return L"Backspace";
    case VK_DELETE: return L"Delete";
    case VK_INSERT: return L"Insert";
    case VK_XBUTTON1: return L"XButton1";
    case VK_XBUTTON2: return L"XButton2";
    case VK_OEM_PLUS: return L"Plus";
    case VK_OEM_MINUS: return L"Minus";
    case VK_OEM_COMMA: return L"Comma";
    case VK_OEM_PERIOD: return L"Period";
    default: return std::wstring(1, static_cast<wchar_t>(vk));
  }
}

void Config::ClampAll() noexcept {
  gap = std::min(gap, 400U);
  horizontal.Clamp();
  vertical.Clamp();
  hotkey_h_s.Clamp('h');
  hotkey_exit.Clamp('e');
}

void HotkeyConfig::Clamp(char mode) noexcept {
  constexpr unsigned int kAllowedModifiers =
      MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN;
  if ((mod & ~kAllowedModifiers) != 0) mod = 0;

  const bool function_key = vk >= VK_F1 && vk <= VK_F24;
  const bool valid_key = function_key || (vk >= L'0' && vk <= L'9') ||
                         (vk >= L'A' && vk <= L'Z') ||
                         (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) ||
                         vk == VK_XBUTTON1 || vk == VK_XBUTTON2 ||
                         vk == VK_BACK || vk == VK_TAB || vk == VK_SPACE ||
                         vk == VK_RETURN || vk == VK_DELETE || vk == VK_INSERT ||
                         vk == VK_ESCAPE || (vk >= VK_OEM_1 && vk <= VK_OEM_3) ||
                         (vk >= VK_OEM_4 && vk <= VK_OEM_8) ||
                         vk == VK_OEM_PLUS || vk == VK_OEM_COMMA ||
                         vk == VK_OEM_MINUS || vk == VK_OEM_PERIOD;
  if (!valid_key) vk = (mode == 'e') ? L'Q' : L'E';
  if (!function_key && mod == 0) mod = MOD_SHIFT | MOD_ALT;
}

unsigned int Config::ParseMod(const wchar_t* text, bool function_key) {
  constexpr unsigned int kDefault = MOD_SHIFT | MOD_ALT;
  std::wstring input = TrimAndLower(text != nullptr ? text : L"");
  if (input.empty()) return function_key ? 0U : kDefault;

  // 同时支持 '+' 与 ',' 作为修饰键分隔符，任何按键书写顺序均可等价解析
  for (wchar_t& ch : input) {
    if (ch == L'+') ch = L',';
  }

  unsigned int modifiers = 0;
  size_t start = 0;
  while (start <= input.size()) {
    const size_t separator = input.find(L',', start);
    const size_t end = separator == std::wstring::npos ? input.size() : separator;
    const std::wstring token = TrimAndLower(input.substr(start, end - start));
    if (!token.empty()) {
      if (token == L"ctrl" || token == L"control") {
        modifiers |= MOD_CONTROL;
      } else if (token == L"alt") {
        modifiers |= MOD_ALT;
      } else if (token == L"win" || token == L"windows") {
        modifiers |= MOD_WIN;
      } else if (token == L"shift") {
        modifiers |= MOD_SHIFT;
      } else {
        return kDefault;
      }
    }
    if (separator == std::wstring::npos) break;
    start = separator + 1;
  }
  return modifiers == 0 ? (function_key ? 0U : kDefault) : modifiers;
}

std::pair<unsigned int, bool> Config::ParseVK(const wchar_t* text, char mode) {
  static const std::unordered_map<std::wstring, unsigned int> key_map = {
      {L"xbutton1", VK_XBUTTON1}, {L"xbutton2", VK_XBUTTON2},
      {L"backspace", VK_BACK},    {L"tab", VK_TAB},
      {L"space", VK_SPACE},       {L"enter", VK_RETURN},
      {L"delete", VK_DELETE},     {L"insert", VK_INSERT},
      {L"esc", VK_ESCAPE},        {L"escape", VK_ESCAPE},
      {L"plus", VK_OEM_PLUS},     {L"+", VK_OEM_PLUS},
      {L"=", VK_OEM_PLUS},        {L"comma", VK_OEM_COMMA},
      {L",", VK_OEM_COMMA},       {L"minus", VK_OEM_MINUS},
      {L"-", VK_OEM_MINUS},       {L"_", VK_OEM_MINUS},
      {L"period", VK_OEM_PERIOD}, {L".", VK_OEM_PERIOD},
      {L"slash", VK_OEM_2},       {L"/", VK_OEM_2},
      {L"semicolon", VK_OEM_1},   {L";", VK_OEM_1},
      {L"apostrophe", VK_OEM_7},  {L"'", VK_OEM_7},
      {L"bracketleft", VK_OEM_4}, {L"[", VK_OEM_4},
      {L"backslash", VK_OEM_5},   {L"\\", VK_OEM_5},
      {L"bracketright", VK_OEM_6},{L"]", VK_OEM_6},
      {L"grave", VK_OEM_3},       {L"`", VK_OEM_3}};

  std::wstring key = TrimAndLower(text != nullptr ? text : L"");
  if (key.starts_with(L"vk_")) key.erase(0, 3);
  if (key.size() == 1) {
    const wchar_t character = key.front();
    if ((character >= L'a' && character <= L'z') ||
        (character >= L'0' && character <= L'9')) {
      return {static_cast<unsigned int>(towupper(character)), false};
    }
  }
  if (key.size() >= 2 && key.front() == L'f') {
    if (const auto number = ParseUnsigned(std::wstring_view(key).substr(1), 1, 24)) {
      return {VK_F1 + *number - 1, true};
    }
  }
  if (key.starts_with(L"numpad")) {
    if (const auto number = ParseUnsigned(std::wstring_view(key).substr(6), 0, 9)) {
      return {VK_NUMPAD0 + *number, false};
    }
  }
  if (const auto item = key_map.find(key); item != key_map.end()) {
    return {item->second, false};
  }
  return {static_cast<unsigned int>(mode == 'e' ? L'Q' : L'E'), false};
}
