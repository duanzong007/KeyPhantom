#include "platforminput.h"

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>
#elif defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

namespace PlatformInput {

bool hasPermission()
{
#ifdef Q_OS_MACOS
    return AXIsProcessTrusted();
#else
    return true;
#endif
}

void requestPermission()
{
#ifdef Q_OS_MACOS
    const void *keys[] = {kAXTrustedCheckOptionPrompt};
    const void *values[] = {kCFBooleanTrue};
    CFDictionaryRef options = CFDictionaryCreate(nullptr, keys, values, 1,
                                                 &kCFTypeDictionaryKeyCallBacks,
                                                 &kCFTypeDictionaryValueCallBacks);
    if (options) {
        AXIsProcessTrustedWithOptions(options);
        CFRelease(options);
    }
#endif
}

#ifdef Q_OS_WIN
static INPUT key(WORD code, bool unicode, bool up)
{
    INPUT event{};
    event.type = INPUT_KEYBOARD;
    if (unicode) {
        event.ki.wScan = code;
        event.ki.dwFlags = KEYEVENTF_UNICODE;
    } else {
        event.ki.wVk = code;
    }
    if (up)
        event.ki.dwFlags |= KEYEVENTF_KEYUP;
    return event;
}
#endif

QString send(const QString &unit, bool shiftEnter)
{
#ifdef Q_OS_WIN
    std::vector<INPUT> events;
    const bool newline = unit == QStringLiteral("\n");
    const bool tab = unit == QStringLiteral("\t");
    if (newline && shiftEnter)
        events.push_back(key(VK_SHIFT, false, false));
    if (newline || tab) {
        const WORD vk = newline ? VK_RETURN : VK_TAB;
        events.push_back(key(vk, false, false));
        events.push_back(key(vk, false, true));
    } else {
        // Keep each UTF-16 surrogate pair (and each grapheme) in one SendInput call.
        for (QChar ch : unit) {
            events.push_back(key(ch.unicode(), true, false));
            events.push_back(key(ch.unicode(), true, true));
        }
    }
    if (newline && shiftEnter)
        events.push_back(key(VK_SHIFT, false, true));
    if (events.empty())
        return {};
    const UINT count = static_cast<UINT>(events.size());
    SetLastError(ERROR_SUCCESS);
    if (SendInput(count, events.data(), sizeof(INPUT)) == count)
        return {};

    const DWORD error = GetLastError();
    // A partial insertion must not leave Return, Tab, or our Shift held down.
    std::vector<INPUT> releases;
    for (auto event : events) {
        if (event.ki.dwFlags & KEYEVENTF_KEYUP)
            releases.push_back(event);
    }
    SendInput(static_cast<UINT>(releases.size()), releases.data(), sizeof(INPUT));
    return QStringLiteral("系统未能完整发送按键（错误码 %1）。请检查目标窗口及其权限级别；Windows 可能阻止向更高权限的应用输入。")
        .arg(error);
#elif defined(Q_OS_MACOS)
    const bool newline = unit == QStringLiteral("\n");
    const bool tab = unit == QStringLiteral("\t");
    const CGKeyCode code = newline ? kVK_Return : tab ? kVK_Tab : 0;
    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, code, true);
    CGEventRef up = CGEventCreateKeyboardEvent(nullptr, code, false);
    if (!down || !up) {
        if (down) CFRelease(down);
        if (up) CFRelease(up);
        return QStringLiteral("无法创建键盘事件，请检查系统资源与辅助功能权限。");
    }
    const CGEventFlags flags = newline && shiftEnter ? kCGEventFlagMaskShift : 0;
    CGEventSetFlags(down, flags);
    CGEventSetFlags(up, flags);
    if (!newline && !tab) {
        const auto *characters = reinterpret_cast<const UniChar *>(unit.utf16());
        CGEventKeyboardSetUnicodeString(down, unit.size(), characters);
        CGEventKeyboardSetUnicodeString(up, unit.size(), characters);
    }
    CGEventPost(kCGSessionEventTap, down);
    CGEventPost(kCGSessionEventTap, up);
    CFRelease(down);
    CFRelease(up);
    return {};
#else
    Q_UNUSED(unit)
    Q_UNUSED(shiftEnter)
    return QStringLiteral("当前系统不支持模拟输入。");
#endif
}

}
