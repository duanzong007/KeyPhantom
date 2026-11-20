# 键影 KeyPhantom

基于 C++17 和 Qt Widgets 的模拟键盘输入工具，使用同一套源码支持 Windows 和 macOS。将文本填入窗口后，程序通过系统键盘事件逐字输入到目标应用。

## 使用

1. 在首页文本框中输入或粘贴内容。首页保持简洁，只保留文本框、简要统计与当前操作按钮。
2. 点击右上角“设置”，在独立设置页调整速度、倒计时（1–30 秒）、换行、Tab、完成提醒与权限，然后返回首页。
3. 点击“开始输入”，在倒计时内切换到目标应用并聚焦输入框。倒计时可随时取消；若结束时仍停留在键影，程序不会开始发送。
4. 返回键影时自动暂停，输入中主按钮变为“暂停输入”。点击“继续输入”会重新倒计时，再切回目标窗口继续。
5. 在任意应用中按 **Ctrl + Alt + F8** 停止输入；也可使用窗口中的“停止”。在键影窗口内按 Esc 同样可以取消或停止。

macOS 上快捷键为 **Control + Option + F8**。如果 F8 被设置为媒体键，可能需要同时按 Fn。快捷键注册失败时，窗口会明确显示不可用。

输入进度以完整 Unicode 字素（例如一个 Emoji）计数，CRLF 和 CR 会统一为单个换行；Tab 转为空格时，展开后的空格分别计数。进度表示已向系统发送的内容，不代表目标应用已接收。

### 输入策略与权限

- 换行可选择 **Enter** 或 **Shift+Enter**。Enter 在某些应用中会提交消息，请按目标应用选择。
- Tab 可选择发送 **Tab 按键** 或替换为 **1–8 个空格**。Tab 按键可能使焦点切换到其他控件。
- macOS 必须在“系统设置 → 隐私与安全性 → 辅助功能”中授权应用。缺少权限时无法开始，设置页提供授权入口；运行中检测到权限撤销会停止并显示原因。
- Windows 检查系统的 `SendInput` 返回值；系统拒绝或只接受部分事件时会停止并报告错误。向高于本程序权限级别的应用输入可能被系统阻止。
- 完成、停止和错误均在窗口中提示，不弹出完成对话框。可勾选“完成时提醒”，通过 Dock 或任务栏提醒完成。
- 自动记住速度、倒计时、输入策略、提醒偏好及窗口位置；不会保存输入文本。

程序继续使用 Unicode 键盘事件。部分游戏、串流客户端（包括 Moonlight 的普通键盘转发）和自定义输入控件可能忽略事件中的文本；这些场景需要单独适配。

## 输入速度

| 模式 | 每批字符数 | 字符间隔 | 批次间隔 |
| --- | ---: | ---: | ---: |
| 慢速 | 1 | 20 ms | 300 ms |
| 正常（默认） | 5 | 10 ms | 50 ms |
| 快速 | 20 | 2 ms | 20 ms |
| 超高速 | 50 | 0 ms | 10 ms |

字符间隔与批次间隔会累加。不同应用对模拟输入的支持不同，请按实际效果选择速度。

## 项目结构

```text
KeyPhantom/
├── CMakeLists.txt
├── cmake/                    # 平台构建配置
├── src/
│   ├── main.cpp              # 程序入口
│   ├── mainwindow.*          # 窗口、倒计时与设置
│   ├── appearance.*          # 应用主题
│   ├── inputplan.*           # Unicode 分段、换行和 Tab 处理
│   ├── keysenderthread.*     # 可暂停、可中止的发送线程
│   ├── platforminput.*       # Windows/macOS 键盘接口
│   └── globalstophotkey.*    # 系统级停止快捷键
├── ui/mainwindow.ui          # Qt Designer 界面
├── resources/                # 样式、资源配置与图标
└── tests/                    # 输入逻辑及窗口回归测试
```

Windows 和 macOS 共用一套源码。Windows 使用 `SendInput`，macOS 使用 `CGEvent`。生成文件统一放在 `build/` 等独立构建目录中。

## 构建

需要 CMake 3.21 或更新版本、Qt 6.2 或更新版本（含 Qt Widgets），以及支持 C++17 的编译器。Windows 编译器需与 Qt Kit 匹配。

在 Qt Creator 中打开仓库根目录的 `CMakeLists.txt`，选择对应平台的 Qt Kit 后构建；也可以从仓库根目录执行：

```sh
cmake -S . -B build/macos -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/macos" -DCMAKE_BUILD_TYPE=Release
cmake --build build/macos --parallel
```

macOS 的程序生成在 `build/macos/KeyPhantom.app`。Windows 将 Qt 路径替换为所选 Kit 的安装目录，并使用独立构建目录，例如：

```powershell
cmake -S . -B build/windows -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
cmake --build build/windows --config Release --parallel
```

Windows 配置自动链接 `user32`，macOS 配置自动链接 ApplicationServices 和 Carbon 框架，并设置应用图标。

如果使用 Qt 6.9.2 与较新的 Apple Clang，在 Apple Silicon 上遇到 Qt 头文件中 `__yield` 未声明的错误，可在 CMake 配置命令中额外传入 `-DCMAKE_CXX_FLAGS="-include arm_acle.h"`。这是该工具链组合的本地兼容参数。

## 验证

启用回归测试后构建并运行：

```sh
cmake -S . -B build/macos -DKEYPHANTOM_BUILD_TESTS=ON
cmake --build build/macos --parallel
ctest --test-dir build/macos --output-on-failure
```

首次配置仍需提供 Qt 安装路径，以及本机需要的工具链兼容参数。测试覆盖 Unicode/Emoji、换行归一化、Tab 转换、进度、暂停继续、停止、错误与权限分支，以及倒计时取消和偏好保存。发送测试使用替代后端，不会向其他应用输入内容。窗口测试使用离屏渲染。
