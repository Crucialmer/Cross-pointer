#include "./hotkey.h"

bool HotkeyManager::Register(const HotkeyConfig& toggle,
                             const HotkeyConfig& exit) {
  UnregisterAll();
  if (!RegisterHotKey(nullptr, kToggleHotkeyId, toggle.mod | MOD_NOREPEAT,
                      toggle.vk)) {
    return false;
  }
  // 当显隐热键与退出热键设为相同时，只需注册一个全局热键 ID (kToggleHotkeyId)
  // 消息循环将根据十字线当前显隐状态智能分发：未显示时触发显示，已显示时触发退出
  if (toggle.mod == exit.mod && toggle.vk == exit.vk) {
    return true;
  }

  if (!RegisterHotKey(nullptr, kExitHotkeyId, exit.mod | MOD_NOREPEAT,
                      exit.vk)) {
    UnregisterHotKey(nullptr, kToggleHotkeyId);
    return false;
  }
  return true;
}

void HotkeyManager::UnregisterAll() noexcept {
  UnregisterHotKey(nullptr, kToggleHotkeyId);
  UnregisterHotKey(nullptr, kExitHotkeyId);
}

