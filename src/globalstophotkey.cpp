#include "globalstophotkey.h"

#include <QCoreApplication>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <Carbon/Carbon.h>
#endif

namespace {
constexpr int HotkeyId = 0x4B50;
}

struct GlobalStopHotkey::Native
{
    bool registered = false;
#ifdef Q_OS_MACOS
    EventHotKeyRef hotkey = nullptr;
    EventHandlerRef handler = nullptr;
#endif
};

GlobalStopHotkey::GlobalStopHotkey(QObject *parent)
    : QObject(parent), native_(std::make_unique<Native>())
{
#ifdef Q_OS_WIN
    native_->registered = RegisterHotKey(nullptr, HotkeyId, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'Q');
    if (native_->registered)
        QCoreApplication::instance()->installNativeEventFilter(this);
#elif defined(Q_OS_MACOS)
    const EventTypeSpec eventType = {kEventClassKeyboard, kEventHotKeyPressed};
    const OSStatus handlerStatus = InstallApplicationEventHandler(
        [](EventHandlerCallRef, EventRef event, void *context) -> OSStatus {
            EventHotKeyID id{};
            if (GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID,
                                  nullptr, sizeof(id), nullptr, &id) != noErr || id.id != HotkeyId
                || id.signature != 0x4B50484D)
                return eventNotHandledErr;
            auto *self = static_cast<GlobalStopHotkey *>(context);
            QMetaObject::invokeMethod(self, [self] { emit self->activated(); }, Qt::QueuedConnection);
            return noErr;
        }, 1, &eventType, this, &native_->handler);
    if (handlerStatus == noErr) {
        const EventHotKeyID id = {0x4B50484D, HotkeyId};
        native_->registered = RegisterEventHotKey(kVK_ANSI_Q, controlKey | optionKey, id,
                                                  GetApplicationEventTarget(), 0, &native_->hotkey) == noErr;
    }
#endif
}

GlobalStopHotkey::~GlobalStopHotkey()
{
#ifdef Q_OS_WIN
    if (native_->registered) {
        UnregisterHotKey(nullptr, HotkeyId);
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
#elif defined(Q_OS_MACOS)
    if (native_->hotkey)
        UnregisterEventHotKey(native_->hotkey);
    if (native_->handler)
        RemoveEventHandler(native_->handler);
#endif
}

bool GlobalStopHotkey::isRegistered() const
{
    return native_->registered;
}

QString GlobalStopHotkey::shortcutText()
{
#ifdef Q_OS_MACOS
    return QStringLiteral("⌃ Control + ⌥ Option + Q");
#else
    return QStringLiteral("Ctrl + Alt + Q");
#endif
}

bool GlobalStopHotkey::nativeEventFilter(const QByteArray &type, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (type == "windows_generic_MSG" || type == "windows_dispatcher_MSG") {
        const auto *event = static_cast<MSG *>(message);
        if (event->message == WM_HOTKEY && event->wParam == HotkeyId) {
            emit activated();
            *result = 0;
            return true;
        }
    }
#else
    Q_UNUSED(type)
    Q_UNUSED(message)
    Q_UNUSED(result)
#endif
    return false;
}
