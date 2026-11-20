# 键影 KeyPhantom

基于 C++17 和 Qt Widgets 的模拟键盘输入工具，使用同一套源码支持 Windows 和 macOS。将文本填入窗口后，程序通过系统键盘事件逐字输入到目标应用。

## 使用

1. 在文本框中填入需要输入的内容。
2. 选择输入速度，默认为“正常”。
3. 点击“粘贴”，在 3 秒等待时间内切换到目标窗口并聚焦输入框。
4. 开始输入后，可返回工具窗口点击“停止”中止发送。

macOS 需要在“系统设置 → 隐私与安全性 → 辅助功能”中授权本应用。当前实现会把文本中的换行符发送为 Return 按键，制表符发送为 Tab 按键；实际效果取决于目标应用，例如提交内容或切换焦点。

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
├── CMakeLists.txt            # Qt 6 / CMake 构建入口
├── README.md
├── cmake/
│   └── KeyPhantom.rc.in      # Windows 应用图标配置
├── src/
│   ├── main.cpp             # 程序入口
│   ├── mainwindow.h         # 主窗口和发送线程声明
│   └── mainwindow.cpp       # 输入流程与平台实现
├── ui/
│   └── mainwindow.ui        # Qt Designer 界面
└── resources/
    ├── resources.qrc        # Qt 资源配置
    └── icons/
        ├── keyphantom_logo.icns
        └── keyphantom_logo.ico
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
