#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>

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

    // Placeholder — TrayApp will be added in Task 8
    QSystemTrayIcon trayIcon;
    trayIcon.setToolTip("TransIt - Screen Translator");
    trayIcon.show();

    return app.exec();
}
