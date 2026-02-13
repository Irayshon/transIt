#pragma once

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QKeySequence>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class HotkeyManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    bool registerHotkey(const QKeySequence &keySequence);
    void unregisterHotkey();

    bool nativeEventFilter(const QByteArray &eventType,
                           void *message, qintptr *result) override;

signals:
    void hotkeyTriggered();

private:
    static constexpr int HOTKEY_ID = 1;
    bool m_registered = false;

#ifdef Q_OS_WIN
    static UINT qtKeyToWinMod(Qt::KeyboardModifiers mods);
    static UINT qtKeyToWinVk(int key);
#endif
};
