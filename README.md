# CrossPointer 🎯

<p align="center">
  <strong>参考 macOS 26 设计美学的极简、纯绿色 Windows 屏幕十字辅助线工具</strong>
  <br />
  <em>A minimalist, pure-green mouse pointer crosshair overlay for Windows, inspired by macOS design aesthetics.</em>
</p>

<p align="center">
  <a href="https://github.com/Crucialmer/CrossPointer/releases/latest"><img src="https://img.shields.io/github/v/release/Crucialmer/CrossPointer?color=007AFF&label=Release" alt="Release"></a>
  <img src="https://img.shields.io/badge/Language-C%2B%2B20-blue" alt="C++20">
  <img src="https://img.shields.io/badge/Platform-Windows%2010%2F11-0078D4" alt="Platform">
  <img src="https://img.shields.io/badge/Direct2D-Hardware%20Accelerated-green" alt="Direct2D">
  <img src="https://img.shields.io/badge/License-MIT-brightgreen" alt="License">
  <img src="https://img.shields.io/badge/Portable-100%25%20Pure%20Green-success" alt="Pure Green">
</p>

---

## ✨ 为什么选择 CrossPointer？

在日常浏览超长 Excel 表格、审阅复杂数据、代码审校、绘图或对齐设计时，我们经常需要快速准确定位鼠标所在的行与列。

微软 PowerToys 虽然内置了“鼠标十字线”，但必须捆绑安装数十个无用模块，体积动辄数百兆。**CrossPointer** 专为解决这一痛点而生：
- **极简小巧**：单一可执行文件仅 **~290 KB**，内存占用低于 10 MB。
- **100% 纯绿色**：零安装、零注册表、零外部文件外溢，所有配置严格保存在程序同目录下。
- **macOS 美学**：融合 macOS 极简悬浮质感与深浅色模式实时自适应。

---

## 🌟 核心特性

- 🎨 **macOS 26 设计美学**：
  - 专属设计的黑曜石磨砂玻璃 Squircle（超椭圆圆角）高清图标。
  - 默认采用 Apple Azure 蔚蓝半透明高质感线条，视野通透、低侵入感，保护底层阅读体验。
- 🌓 **跟随系统深浅色自适应 (Auto Light/Dark Mode)**：
  - 毫秒级响应 Windows 10/11 主题切换广播（`WM_SETTINGCHANGE`），无需重启程序。
  - ☀️ **浅色模式**：高对比沉稳蔚蓝（`#0066E6`），白底文档表格清晰锐利。
  - 🌙 **深色模式**：极具通透感的激光荧光青（`#00D2FF`），暗色界面微光聚焦。
  - 沉浸式暗黑托盘菜单（系统级 `SetPreferredAppMode` 适配）。
- 🖥️ **原生系统托盘管理 (System Tray Manager)**：
  - **单击左键**：一键切换十字线显示 / 隐藏。
  - **双击左键**：自动用默认文本编辑器打开 `CrossPointer.ini`，直观修改快捷键、颜色与线宽。
  - **右键菜单**：提供快捷设置、显隐切换、配置重载、关于与退出。
  - **悬停提示**：鼠标划过托盘图标即时显示当前快捷键（如 `CrossPointer (快捷键: Ctrl+Alt+Win+H)`）。
- ⚡ **1ms 硬件级高刷调度**：
  - 引入 `timeBeginPeriod(1)` 调度，突破 Windows 默认 15.6ms 时钟限制，在 120Hz/144Hz 电竞高刷屏上呈现如 Apple ProMotion 般丝滑平顺的跟随效果。
- 🛡️ **穿透与稳定性**：
  - 窗口扩展样式包含 `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE`，鼠标点击 100% 穿透，不抢占任何窗口焦点。

---

## 🚀 快速上手

1. 前往 **[Releases 页面](https://github.com/Crucialmer/CrossPointer/releases/latest)** 下载单一绿色文件 `CrossPointer.exe`。
2. 双击运行即可立即使用。
3. 首次运行会在同目录下自动生成 `CrossPointer.ini` 配置文件。
4. **常用交互**：
   - **默认切换显隐快捷键**：`Ctrl + Alt + Win + H`（可自定义）
   - **默认退出程序快捷键**：`Ctrl + Alt + Win + E`（可自定义）
   - **系统托盘**：双击右下角托盘图标即可打开配置，修改后在右键菜单点击“重新载入配置”即刻生效。

---

## ⚙️ 配置文件说明 (`CrossPointer.ini`)

```ini
[Theme]
# 1 = 跟随系统自动切换黑白模式 (推荐)；0 = 固定使用下方颜色
AutoFollowSystem = 1

[Light_Mode]
# 浅色模式专属色彩（推荐高对比沉稳蔚蓝）
R = 0
G = 102
B = 230
Alpha = 200

[Dark_Mode]
# 深色模式专属色彩（推荐极光电青）
R = 0
G = 210
B = 255
Alpha = 180

[Crosshair]
# 鼠标光标中心的留白间隙 (0-400)，默认 16px
Gap = 16

[Horizontal]
# 横线宽度 (1-200)
Width = 2
# 当 AutoFollowSystem = 0 时使用的固定颜色
R = 0
G = 122
B = 255
Alpha = 180

[Vertical]
# 竖线宽度 (1-200)
Width = 2
R = 0
G = 122
B = 255
Alpha = 180

[Hide_Show_Hotkey]
# 修饰键支持: Ctrl, Alt, Win, Shift (可逗号分隔多选；若单按 F1-F24 可留空)
Mod = Ctrl,Alt,Win
# 主按键支持: A-Z, 0-9, F1-F24, Space, Tab, XButton1/2 等
VK = H

[Exit_Hotkey]
Mod = Ctrl,Alt,Win
VK = E
```

---

## 🛠️ 本地编译构建

**环境要求**：
- Windows 10 / 11
- MSVC (Visual Studio 2022 或 C++ Build Tools)
- CMake >= 3.24
- Ninja

```powershell
# 1. 生成构建目录
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2. 编译为纯绿色单文件 (/MT 静态链接)
cmake --build build -j 8
```
编译完成后将在根目录输出 `CrossPointer.exe`。

---

## 📄 开源许可证

本项目基于 [MIT License](LICENSE) 开源。
