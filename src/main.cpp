#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include "TrayApp.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("TransIt");
    app.setOrganizationName("TransIt");
    app.setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "TransIt",
            "System tray is not available on this system.");
        return 1;
    }

    TrayApp trayApp;
    trayApp.initialize();

    return app.exec();
}
