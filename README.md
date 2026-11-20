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
├── README.md
└── 源码/
    ├── zhantie.pro       # qmake 项目配置
    ├── main.cpp          # 程序入口
    ├── mainwindow.h      # 主窗口和发送线程声明
    ├── mainwindow.cpp    # 共用输入流程及 Windows/macOS 平台实现
    ├── mainwindow.ui     # Qt Designer 界面
    ├── logo.qrc          # 图标资源配置
    ├── keyphantom_logo.icns
    └── keyphantom_logo.ico
```

原 Mac 专版与双平台版已合并，后续统一维护 `源码/`。Windows 使用 `SendInput`，macOS 使用 `CGEvent`。

## 构建

安装支持 C++17 的编译器及包含 Qt Widgets 的 Qt 开发环境。可在 Qt Creator 中打开 `源码/zhantie.pro`，选择对应平台的 Qt Kit 后构建。

也可以在已配置 Qt 的终端中，从仓库根目录执行：

```sh
mkdir -p build
cd build
qmake ../源码/zhantie.pro
make -j4
```

以上命令适用于 macOS；Windows 根据所选 Kit 使用 `mingw32-make` 或 `nmake`。Windows 配置自动链接 `user32`，macOS 配置自动链接 ApplicationServices 和 Carbon 框架。
