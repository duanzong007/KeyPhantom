# 键影 KeyPhantom v1.2.0

本次更新重做了界面和输入控制：简洁首页搭配独立设置页，长文本输入支持进度查看、暂停继续，以及随时全局停止。

## 新增功能

- **全局停止**：Mac 按 `Control + Option + Q`，Windows 按 `Ctrl + Alt + Q`。
- **可取消倒计时**：支持 1–30 秒，方便切换到目标输入框。
- **输入进度**：显示已发送字符数和预计剩余时间。
- **暂停与继续**：返回键影自动暂停，继续前重新倒计时。
- **输入策略**：换行可选 Enter / Shift+Enter，Tab 可选按键或 1–8 个空格。
- **偏好记忆**：自动保存速度、等待时间、输入策略、提醒与窗口位置。
- **完成提醒**：窗口内显示结果，可选 Dock / 任务栏提醒。

## 界面与体验

- 首页保留文本框与主操作按钮，进度和停止控制按任务状态显示。
- 新增独立设置页，集中管理输入选项与权限。
- 完成、停止和错误均在窗口内提示，不再弹出完成对话框。
- Mac 应用名称改为 **键影.app**，快捷键提示使用 Mac 对应键名。

## 修复与改进

- 修复 Emoji、生僻字和组合字符可能被拆开发送的问题。
- 统一 CRLF / CR 换行，避免重复回车。
- 完善 macOS 权限检查与 Windows 输入失败提示。
- 改善停止响应，避免误向键影自身输入。
- 合并双平台代码并迁移至标准 CMake 工程，补充回归测试。

## 下载与使用

- **Mac · Apple Silicon**：`KeyPhantom-1.2.0-macos-arm64.dmg`，打开后将“键影.app”拖入 Applications。最低系统版本为 macOS 12.0。
- **Mac · Intel**：`KeyPhantom-1.2.0-macos-x64.dmg`，适用于 Intel 芯片 Mac，最低系统版本为 macOS 12.0。
- **Windows · 32 位（x86）**：`KeyPhantom-1.2.0-windows-x86.zip`，完整解压后运行 `KeyPhantom.exe`。
- **Windows · 64 位（x64）**：`KeyPhantom-1.2.0-windows-x64.zip`，完整解压后运行 `KeyPhantom.exe`，请保留附带的 DLL 和插件目录。

Mac 按 Apple Silicon / Intel 选择对应包，Windows 按 x86 / x64 选择对应包，本次不提供 Windows ARM 专用版本。

Mac 首次输入前，请在系统的辅助功能设置中授权“键影”。当前 Mac 包采用本地签名，尚未经过 Apple 公证，首次打开可能出现开发者验证提示。

Enter 可能提交内容，Tab 可能切换焦点，请按目标应用选择设置。部分游戏或串流客户端可能不接受 Unicode 模拟输入。
