#include "HotkeyManager.h"
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
    QCoreApplication::instance()->installNativeEventFilter(this);
}

HotkeyManager::~HotkeyManager() {
    unregisterHotkey();
    QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool HotkeyManager::registerHotkey(const QKeySequence &keySequence) {
#ifdef Q_OS_WIN
    unregisterHotkey();

    if (keySequence.isEmpty())
        return false;

    auto combo = keySequence[0];
    Qt::KeyboardModifiers mods = combo.keyboardModifiers();
    int key = combo.key();

    UINT winMod = qtKeyToWinMod(mods);
    UINT winVk = qtKeyToWinVk(key);

    if (RegisterHotKey(nullptr, HOTKEY_ID, winMod | MOD_NOREPEAT, winVk)) {
        m_registered = true;
        return true;
    }
    return false;
#else
    Q_UNUSED(keySequence)
    return false;
#endif
}

void HotkeyManager::unregisterHotkey() {
#ifdef Q_OS_WIN
    if (m_registered) {
        UnregisterHotKey(nullptr, HOTKEY_ID);
        m_registered = false;
    }
#endif
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType,
                                        void *message, qintptr *result) {
    Q_UNUSED(result)
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == HOTKEY_ID) {
            emit hotkeyTriggered();
            return true;
        }
    }
#else
    Q_UNUSED(eventType)
    Q_UNUSED(message)
#endif
    return false;
}

#ifdef Q_OS_WIN
UINT HotkeyManager::qtKeyToWinMod(Qt::KeyboardModifiers mods) {
    UINT result = 0;
    if (mods & Qt::ControlModifier) result |= MOD_CONTROL;
    if (mods & Qt::AltModifier) result |= MOD_ALT;
    if (mods & Qt::ShiftModifier) result |= MOD_SHIFT;
    if (mods & Qt::MetaModifier) result |= MOD_WIN;
    return result;
}

UINT HotkeyManager::qtKeyToWinVk(int key) {
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return key;
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return key;
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        return VK_F1 + (key - Qt::Key_F1);
    return 0;
}
#endif
