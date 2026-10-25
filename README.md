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
  <img src="https://img.shields.io/badge/High%20DPI-Up%20to%208K%20Ready-orange" alt="8K Ready">
  <img src="https://img.shields.io/badge/License-MIT-brightgreen" alt="License">
  <img src="https://img.shields.io/badge/Portable-100%25%20Pure%20Green-success" alt="Pure Green">
</p>

---

## ✨ 为什么选择 CrossPointer？

在日常浏览超长 Excel 表格、审阅复杂数据、代码审校、绘图或对齐设计时，我们经常需要快速准确定位鼠标所在的行与列。

微软 PowerToys 虽然内置了“鼠标十字线”，但必须捆绑安装数十个无用模块，体积动辄数百兆。**CrossPointer** 专为解决这一痛点而生：
- **极简小巧**：单一可执行文件仅 **~290 KB**，内存占用低于 10 MB。
- **100% 纯绿色**：零安装、零注册表垃圾、零外部文件外溢，所有配置严格保存在程序同目录下。
- **macOS 美学**：融合 macOS 极简悬浮质感、全新原生可视化设置窗口与深浅色模式实时自适应。

---

## 🌟 核心特性 (v1.3 最新版)

- 🖥️ **macOS 26 分栏胶囊偏好设置面板 (Segmented Capsule Tabs)**：
  - **告别界面挤压与高度不足**：采用 macOS 26 分栏式设计，将设置划分为「🎯 外观尺寸」、「🎨 色彩模式」、「⌨️ 全局快捷键」与「🚀 系统与更新」四大专属 Tab，空间极其宽敞舒朗。
  - **实时十字线微缩预览**：调整中心空隙留白（Gap）、水平线宽与垂直线宽时，卡片内即时呈现 Direct2D 仿真准星预览。
  - **沉浸式深浅色自适应**：设置窗口自身完美融入 Windows 10/11 深色/浅色模式与 DWM 原生圆角。
- ⚡ **单快捷键智能复用（Show/Hide + Exit Toggle）**：
  - **单键兼顾显示与退出**：显隐热键与退出热键支持设为**完全相同的按键组合**！
  - **智能分发**：当十字线未显示时，按下热键立即显示十字线；当十字线已处于显示状态时，按下同一热键立即安全退出程序。无需记忆两个独立快捷键。
- 🎹 **按键按下顺序完全无关（Order-Independent Modifiers）**：
  - 无论您先按下 `Shift` 还是先按下 `Alt` / `Ctrl` / `Win`，只要修饰键处于按下状态并敲击触发键（如 E 键），系统均能毫秒级精准触发，彻底告别按键死板顺序限制。
  - 配置文件中同时完美兼容 `+` 与 `,` 分隔符（如 `Ctrl+Alt` 与 `Ctrl,Alt` 等价解析）。
- 🎨 **极简加粗激光十字标与中心圆孔镂空图标**：
  - 钛金磨砂渐变微弧底座，4个圆角外保持 **100% 数学 Alpha=0 纯透明**（绝无黑边与黑晕）。
  - 十字线大幅加粗至 16px，中心加入精致的圆形镂空孔径，在浅色/深色壁纸及任务栏中均清晰可辨。
- 📐 **最大 8K 分辨率与高 DPI 屏幕动态自适应 (High-DPI & 8K Ready)**：
  - 内置 `PerMonitorV2` 顶级 DPI 感知清单。
  - 针对 4K/8K 超高 PPI 屏幕引入微米级动态缩放，线条宽度与间距自适应当前显示器物理 DPI，拒绝变成微细发丝。
- 🚀 **启动显隐状态自定义 (Show on Startup Toggle)**：
  - **默认静默启动不显示**：打开软件后静默常驻系统右下角托盘，不遮挡当前桌面工作流；随时按下快捷键 `Shift + Alt + E` 或单击托盘图标即可毫秒级呼出。
  - 在偏好设置「系统与更新」中可自由勾选「启动软件时显示十字辅助线」以恢复打开即显示的经典模式。
- 🚀 **绿色开机自启动支持 (Auto-Start)**：
  - 偏好设置窗口中一键勾选“开机自动启动”，由当前用户的 HKCU 注册表独立接管。
  - 绿色纯便携，无需管理员权限，关闭选项即刻彻底清除，不留任何残留。
- 🔄 **原生在线更新检测 (Auto & Manual Update Checker)**：
  - 在偏好设置与托盘菜单中均提供“检查新版本”入口，调用原生 WinINet 异步比对 GitHub 官方最新发布。
  - 支持启动时静默检查更新，发现新版本即弹窗展示更新日志与一键直达下载。
- 🌓 **跟随系统深浅色自适应 (Auto Light/Dark Mode)**：
  - 毫秒级响应 Windows 10/11 主题切换广播（`WM_SETTINGCHANGE`），无需重启程序。
  - ☀️ **浅色模式**：高对比沉稳蔚蓝（`#0066E6`），白底文档表格清晰锐利。
  - 🌙 **深色模式**：极具通透感的激光荧光青（`#00D2FF`），暗色界面微光聚焦。
- ⚡ **1ms 硬件级高刷调度**：
  - 引入 `timeBeginPeriod(1)` 调度，突破 Windows 默认 15.6ms 时钟限制，在 120Hz/144Hz/240Hz 高刷屏上呈现丝滑跟随。

---

## 🚀 快速上手

1. 前往 **[Releases 页面](https://github.com/Crucialmer/CrossPointer/releases/latest)** 下载绿色压缩包 `CrossPointer.zip` 并解压。
2. 解压后自带独立的 `CrossPointer` 文件夹，双击运行 `CrossPointer.exe`（默认静默常驻托盘，按下 `Shift + Alt + E` 即可呼出十字线）。
3. **常用交互**：
   - **默认切换显隐快捷键**：`Shift + Alt + E`（可在窗口设置中修改，支持单键复用）
   - **默认退出程序快捷键**：`Shift + Alt + Q`（可在窗口设置中修改，支持与显隐设为同一键）
   - **系统托盘**：双击右下角托盘图标即可打开可视化设置面板，保存后立即生效。

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
