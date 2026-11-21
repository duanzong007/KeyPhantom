# 构建与打包

## 开发环境

- CMake 3.21.1 或更新版本。
- Qt 6.2 或更新版本，包含 Qt Widgets；当前 Mac 发布构建使用 Qt 6.9.2。
- 支持 C++17 的编译器。Windows 编译器必须与 Qt Kit 匹配。
- Windows / macOS 各自构建和验证对应平台的发布包。

版本号统一定义在根目录 `CMakeLists.txt` 的 `project(... VERSION 1.2.0 ...)`，程序运行版本和 Mac Bundle 版本均从这里生成。

## macOS

Qt Creator 可直接打开根目录的 `CMakeLists.txt`。命令行构建示例：

```sh
cmake -S . -B build/macos \
  -DCMAKE_PREFIX_PATH="/path/to/Qt/6.9.2/macos" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build/macos --parallel
```

应用生成在 `build/macos/键影.app`。CMake 内部目标名称仍为 `KeyPhantom`，Mac 应用文件名、系统显示名和主窗口标题为“键影”。

如果 Apple Silicon 上的 Qt 6.9.2 与较新的 Apple Clang 组合出现 Qt 头文件中 `__yield` 未声明的错误，在配置命令中额外传入：

```sh
-DCMAKE_CXX_FLAGS="-include arm_acle.h"
```

### 生成 Mac 发布包

从仓库根目录执行：

```sh
./scripts/package-macos.sh \
  "/path/to/Qt/6.9.2/macos" \
  "$PWD/build/dist/v1.2.0"
```

需要上述本地兼容参数时：

```sh
./scripts/package-macos.sh \
  "/path/to/Qt/6.9.2/macos" \
  "$PWD/build/dist/v1.2.0" \
  '-DCMAKE_CXX_FLAGS=-include arm_acle.h'
```

默认构建 Apple Silicon（arm64）版本。构建 Intel（x64）版本时，在命令前设置架构：

```sh
KEYPHANTOM_MACOS_ARCH=x86_64 ./scripts/package-macos.sh \
  "/path/to/Qt/6.9.2/macos" \
  "$PWD/build/dist/v1.2.0"
```

两种架构使用不同的构建目录；Intel 构建不要带 ARM 专用的 `arm_acle.h` 参数。Qt 安装需包含所选架构的库。

脚本会按架构构建 Release，使用 `macdeployqt` 收集 Qt 框架及插件，进行本地签名并验证，然后生成：

- `KeyPhantom-1.2.0-macos-arm64.dmg`
- `KeyPhantom-1.2.0-macos-arm64.dmg.sha256`
- `KeyPhantom-1.2.0-macos-x64.dmg`（Intel 构建）
- `KeyPhantom-1.2.0-macos-x64.dmg.sha256`（Intel 构建）
- `更新说明.md`

镜像包含“键影.app”、Applications 快捷入口和使用说明。目标目录中若已存在同名镜像或校验文件，脚本会停止，避免覆盖既有发布包。当前脚本不会进行 Apple 公证。

校验下载文件：

```sh
shasum -a 256 -c KeyPhantom-1.2.0-macos-arm64.dmg.sha256
```

## Windows x86 / x64

发布文件分别命名为 `KeyPhantom-1.2.0-windows-x86.zip` 和 `KeyPhantom-1.2.0-windows-x64.zip`，不构建 Windows ARM 版本。

### x64 构建

以下以 **Qt 6.9.2 MSVC 2022 64 位**为例。在对应的 Visual Studio x64 开发环境中打开 PowerShell，并确认 CMake 和 Qt 工具可用：

```powershell
cmake -S . -B build/windows -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.9.2/msvc2022_64"
cmake --build build/windows --config Release --parallel
```

程序生成在 `build/windows/Release/KeyPhantom.exe`。若使用其他生成器，输出目录可能不同，请以实际构建结果为准。

### x86（32 位）构建

32 位构建需要匹配的 **32 位 Qt 6 库和 MSVC Win32 编译环境**。Qt 6.9 的官方 Windows 支持列表提供 x64 和 ARM64，未列出 x86；32 位 Qt Kit 需在 Windows 环境另行准备，不能直接使用上述 `msvc2022_64` Kit。[Qt 6.9 Windows 配置](https://doc.qt.io/archives/qt-6.9/windows.html)

准备好 32 位 Qt 后，使用独立的构建目录（路径替换为实际安装位置）：

```powershell
cmake -S . -B build/windows-x86 -G "Visual Studio 17 2022" -A Win32 `
  -DCMAKE_PREFIX_PATH="C:/Qt/custom-qt6-msvc-x86"
cmake --build build/windows-x86 --config Release --parallel
```

使用该 32 位 Qt Kit 对应的 `windeployqt.exe` 收集依赖，发布时命名为 `KeyPhantom-1.2.0-windows-x86.zip`。

### 打包 ZIP

下面的命令以新的空目录收集 EXE、Qt DLL 和插件：

```powershell
$packageDir = "build/dist/KeyPhantom-1.2.0-windows-x64"
New-Item -ItemType Directory -Path $packageDir
Copy-Item "build/windows/Release/KeyPhantom.exe" $packageDir
& "C:/Qt/6.9.2/msvc2022_64/bin/windeployqt.exe" --release --compiler-runtime `
  --dir $packageDir "$packageDir/KeyPhantom.exe"
Compress-Archive -Path $packageDir `
  -DestinationPath "build/dist/KeyPhantom-1.2.0-windows-x64.zip"
```

请检查 `windeployqt` 的输出与退出状态，确认收集成功后再压缩。压缩包应保留完整的目录结构。MSVC 构建在部分机器上还需要安装微软 Visual C++ 运行库，发布前应在没有开发环境的 Windows 机器上验证。

如果使用 MinGW，请换用匹配的 Qt MinGW Kit 和生成器，并通过同一个 Kit 的 `windeployqt` 部署依赖，不要混用 MSVC 与 MinGW 的 Qt 文件。

Qt 官方部署说明：[Windows](https://doc.qt.io/qt-6/windows-deployment.html)、[macOS](https://doc.qt.io/qt-6/macos-deployment.html)。

## 回归测试

```sh
cmake -S . -B build/macos -DKEYPHANTOM_BUILD_TESTS=ON
cmake --build build/macos --parallel
ctest --test-dir build/macos --output-on-failure
```

首次配置需提供 Qt 路径及本机需要的兼容参数。Windows 使用对应构建目录；多配置生成器运行测试时增加 `-C Release`。

发送线程测试使用替代后端，不会向其他应用输入文本。测试覆盖 Unicode/Emoji、换行与 Tab、进度、暂停继续、停止、错误及权限分支；窗口测试覆盖倒计时取消、页面切换、紧凑布局和偏好保存。

自动化测试无法替代系统级实测。发布前应确认：在目标应用中输入中文和 Emoji、换行和 Tab 行为、全局停止、暂停继续、首次授权及更新后的授权流程。

## 发布到 GitHub

建议 Tag 为 `v1.2.0`，标题为 **键影 KeyPhantom v1.2.0**。可直接使用 [Release 文案](release-v1.2.0.md)，按实际构建结果上传两个 Mac DMG、Windows x86 / x64 ZIP 和对应校验文件。仅上传已经构建并验证的包，文案中的下载列表按实际附件保留。
