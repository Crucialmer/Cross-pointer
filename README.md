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

## 🌟 核心特性 (v1.1)

- 🎨 **全新 macOS 26 纯矢量设计美学图标**：
  - 采用钛金磨砂灰蓝渐变底座，微弧高光倒角，内嵌加粗苹果 Azure 激光刻度荧光十字标与纯白高光聚焦点。
  - 四个圆角外区域为**绝对数学 Alpha=0 纯透明**，彻底杜绝黑边与黑晕，在任何桌面背景下均极其醒目精致。
- 🖥️ **macOS 26 风格原生现代化偏好设置窗口 (GUI Settings)**：
  - **告别手动编辑配置文件**：双击托盘图标或在右键菜单中选择“偏好设置”，即可打开精美的卡片式设置面板。
  - **实时滑块交互**：实时调整中心留白间隙（Gap）、横竖线粗细（Width）以及不透明度（Alpha）。
  - **调色盘取色**：支持分别设置浅色模式与深色模式下的十字线 RGB 色彩。
  - **快捷键组合器**：勾选修饰键（Ctrl / Alt / Win / Shift）并从下拉列表自由选择触发键。
  - **沉浸式深浅色适配**：设置窗口自身完美融入 Windows 10/11 深色/浅色模式与 DWM 原生圆角。
- 🚀 **绿色开机自启动支持 (Auto-Start)**：
  - 在偏好设置窗口中一键勾选“开机自动启动”，由当前用户的 HKCU 注册表独立接管。
  - 绿色纯便携，无需管理员权限，关闭选项即刻彻底清除，不留任何残留。
- 📐 **最大 8K 分辨率与高 DPI 屏幕动态自适应 (High-DPI & 8K Ready)**：
  - 内置 `PerMonitorV2` 顶级 DPI 感知清单。
  - 十字线粗细与间隙自动根据当前显示器的物理微米级 DPI 进行动态比例缩放。即使在 4K/8K 超高清电竞屏上，线条依然保持一致的视觉比例与清晰度，拒绝变成肉眼难辨的单像素细丝。
- 🌓 **跟随系统深浅色自适应 (Auto Light/Dark Mode)**：
  - 毫秒级响应 Windows 10/11 主题切换广播（`WM_SETTINGCHANGE`），无需重启程序。
  - ☀️ **浅色模式**：高对比沉稳蔚蓝（`#0066E6`），白底文档表格清晰锐利。
  - 🌙 **深色模式**：极具通透感的激光荧光青（`#00D2FF`），暗色界面微光聚焦。
- 🖥️ **原生系统托盘管理 (System Tray Manager)**：
  - **单击左键**：一键切换十字线显示 / 隐藏。
  - **双击左键**：打开现代化 macOS 卡片风格偏好设置窗口。
  - **右键菜单**：提供快捷设置、显隐切换、配置重载、关于与退出。
  - **悬停提示**：鼠标划过托盘图标即时显示当前快捷键（如 `CrossPointer (快捷键: Ctrl+Alt+Win+H)`）。
- ⚡ **1ms 硬件级高刷调度**：
  - 引入 `timeBeginPeriod(1)` 调度，突破 Windows 默认 15.6ms 时钟限制，在 120Hz/144Hz/240Hz 高刷屏上呈现如丝绸般的跟随效果。

---

## 🚀 快速上手

1. 前往 **[Releases 页面](https://github.com/Crucialmer/CrossPointer/releases/latest)** 下载绿色压缩包 `CrossPointer.zip` 并解压。
2. 解压后自带独立的 `CrossPointer` 文件夹，双击运行 `CrossPointer.exe` 即可立即使用。
3. **常用交互**：
   - **默认切换显隐快捷键**：`Ctrl + Alt + Win + H`（可在窗口设置中修改）
   - **默认退出程序快捷键**：`Ctrl + Alt + Win + E`（可在窗口设置中修改）
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
