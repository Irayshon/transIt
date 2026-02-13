#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

#include "Settings.h"
#include "HotkeyManager.h"
#include "RegionSelector.h"
#include "OverlayWindow.h"
#include "AIService.h"

class TrayApp : public QObject {
    Q_OBJECT
public:
    explicit TrayApp(QObject *parent = nullptr);
    ~TrayApp() override;

    void initialize();

private slots:
    void onHotkeyTriggered();
    void onRegionSelected(const QRect &region, const QPixmap &screenshot);
    void onTranslationReady(const QString &text);
    void onTranslationFailed(const QString &error);
    void showSettingsDialog();

private:
    void createTrayIcon();
    void createAIService();
    void registerHotkey();

    Settings *m_settings = nullptr;
    HotkeyManager *m_hotkeyManager = nullptr;
    RegionSelector *m_regionSelector = nullptr;
    OverlayWindow *m_overlayWindow = nullptr;
    AIService *m_aiService = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
};
