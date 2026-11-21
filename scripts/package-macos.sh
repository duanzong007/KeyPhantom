#!/bin/bash
set -euo pipefail

# KEYPHANTOM_MACOS_ARCH: arm64 (default) or x86_64.
# Usage: ./scripts/package-macos.sh /path/to/Qt/macos /path/to/output [extra CMake arguments...]
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 QT_PREFIX OUTPUT_DIRECTORY [extra CMake arguments...]" >&2
    exit 1
fi

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
qt_prefix="$(cd "$1" && pwd)"
output_dir="$2"
shift 2
macos_arch="${KEYPHANTOM_MACOS_ARCH:-arm64}"
case "$macos_arch" in
    arm64) archive_arch="arm64"; platform_label="Apple Silicon（M 系列）Mac" ;;
    x86_64) archive_arch="x64"; platform_label="Intel Mac" ;;
    *) echo "Unsupported architecture: $macos_arch" >&2; exit 1 ;;
esac
build_dir="$project_root/build/macos-release-$archive_arch"
version="$(awk '/^project\(KeyPhantom VERSION / { print $3; exit }' "$project_root/CMakeLists.txt")"
if [ -z "$version" ] || [ ! -x "$qt_prefix/bin/macdeployqt" ]; then
    echo "Project version or macdeployqt not found." >&2
    exit 1
fi
mkdir -p "$output_dir" "$project_root/build"
output_dir="$(cd "$output_dir" && pwd)"
filename="KeyPhantom-${version}-macos-${archive_arch}.dmg"
artifact="$output_dir/$filename"
if [ -e "$artifact" ] || [ -e "$artifact.sha256" ]; then
    echo "Release output already exists; choose an empty output directory: $output_dir" >&2
    exit 1
fi

cmake -S "$project_root" -B "$build_dir" \
    -DCMAKE_PREFIX_PATH="$qt_prefix" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="$macos_arch" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DKEYPHANTOM_BUILD_TESTS=OFF "$@"
cmake --build "$build_dir" --target KeyPhantom --parallel 4

staging_dir="$(mktemp -d "$project_root/build/macos-package.XXXXXX")"
image_dir="$staging_dir/image"
app_path="$image_dir/键影.app"
mkdir -p "$image_dir"
ditto "$build_dir/键影.app" "$app_path"
bundle_executable="$(/usr/libexec/PlistBuddy -c 'Print CFBundleExecutable' "$app_path/Contents/Info.plist")"
if [ -z "$bundle_executable" ] || [ ! -x "$app_path/Contents/MacOS/$bundle_executable" ]; then
    echo "Bundle executable metadata is missing or invalid." >&2
    exit 1
fi
"$qt_prefix/bin/macdeployqt" "$app_path" -verbose=1 -codesign=-
codesign --verify --deep --strict "$app_path"
ln -s /Applications "$image_dir/Applications"
cat > "$image_dir/使用说明.txt" <<HELP
键影 KeyPhantom

1. 将“键影.app”拖入 Applications，然后从“应用程序”文件夹打开。
2. 在键影的“设置”中开启辅助功能权限，允许当前安装的键影控制电脑。
3. 在首页填入文本，点击开始输入，并在倒计时内切换到目标输入框。
4. 全局停止快捷键：Control（⌃）+ Option（⌥）+ Q。

本包适用于 ${platform_label}，最低系统版本为 macOS 12.0。
已包含 Qt 运行依赖，不需要额外安装 Qt。
本包采用本地签名，尚未经过 Apple 公证；首次打开可能出现开发者验证提示。
HELP
hdiutil create -volname "键影 v$version ($archive_arch)" -srcfolder "$image_dir" -format UDZO "$artifact"
hdiutil verify "$artifact"
(cd "$output_dir" && shasum -a 256 "$filename" > "$filename.sha256")
if [ -f "$project_root/docs/release-v${version}.md" ]; then
    cp "$project_root/docs/release-v${version}.md" "$output_dir/更新说明.md"
fi
printf '\nRelease image: %s\nPackaged app: %s\n' "$artifact" "$app_path"
