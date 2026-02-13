# TransIt Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a Windows 11 system-tray utility that captures screen regions, sends them to AI for OCR+translation, and overlays the result.

**Architecture:** Qt6 C++ app with 5 core components (TrayApp, HotkeyManager, RegionSelector, OverlayWindow, AIService) communicating via Qt signals/slots. AI backends are pluggable via abstract interface. Built on Linux via MinGW-w64 cross-compilation.

**Tech Stack:** C++17, Qt6, vcpkg (manifest mode), nlohmann-json, cpr, MinGW-w64

---

### Task 1: Project Scaffold — CMake + vcpkg + Toolchain

**Files:**
- Create: `CMakeLists.txt`
- Create: `vcpkg.json`
- Create: `cmake/mingw-w64-toolchain.cmake`
- Create: `src/main.cpp`
- Create: `.gitignore`

**Step 1: Create vcpkg.json manifest**

```json
{
  "name": "transit",
  "version": "0.1.0",
  "description": "Screen region OCR + translation overlay for Windows 11",
  "dependencies": [
    {
      "name": "qtbase",
      "version>=": "6.6.0"
    },
    "nlohmann-json",
    "cpr"
  ]
}
```

**Step 2: Create CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(transIt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

add_executable(transIt WIN32
    src/main.cpp
)

target_link_libraries(transIt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    nlohmann_json::nlohmann_json
    cpr::cpr
)

if(WIN32)
    target_link_libraries(transIt PRIVATE user32 gdi32)
endif()
```

**Step 3: Create cross-compilation toolchain file**

Create `cmake/mingw-w64-toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

**Step 4: Create minimal main.cpp**

```cpp
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

    // Placeholder — TrayApp will be added in Task 3
    QSystemTrayIcon trayIcon;
    trayIcon.setToolTip("TransIt - Screen Translator");
    trayIcon.show();

    return app.exec();
}
```

**Step 5: Create .gitignore**

```
build/
.cache/
CMakeUserPresets.json
*.exe
*.dll
*.obj
*.o
```

**Step 6: Verify the project configures**

Run (native Linux build first to verify CMake logic — cross-compile test later):
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```
Expected: CMake configures without errors (or note missing vcpkg packages to install).

**Step 7: Commit**

```bash
git init && git add -A && git commit -m "feat: project scaffold with CMake, vcpkg manifest, and cross-compile toolchain"
```

---

### Task 2: Settings — Configuration Persistence

**Files:**
- Create: `src/Settings.h`
- Create: `src/Settings.cpp`

**Step 1: Create Settings.h**

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QKeySequence>

class Settings : public QObject {
    Q_OBJECT
public:
    enum class Backend {
        OpenAI,
        Gemini
    };
    Q_ENUM(Backend)

    explicit Settings(QObject *parent = nullptr);

    // API Keys
    QString apiKey(Backend backend) const;
    void setApiKey(Backend backend, const QString &key);

    // Target language
    QString targetLanguage() const;
    void setTargetLanguage(const QString &lang);

    // Active backend
    Backend activeBackend() const;
    void setActiveBackend(Backend backend);

    // Hotkey
    QKeySequence hotkey() const;
    void setHotkey(const QKeySequence &key);

    // Overlay font size
    int overlayFontSize() const;
    void setOverlayFontSize(int size);

signals:
    void settingsChanged();

private:
    static QString backendKey(Backend backend);
};
```

**Step 2: Create Settings.cpp**

```cpp
#include "Settings.h"
#include <QSettings>

Settings::Settings(QObject *parent)
    : QObject(parent) {}

QString Settings::apiKey(Backend backend) const {
    QSettings s;
    return s.value("api_keys/" + backendKey(backend)).toString();
}

void Settings::setApiKey(Backend backend, const QString &key) {
    QSettings s;
    s.setValue("api_keys/" + backendKey(backend), key);
    emit settingsChanged();
}

QString Settings::targetLanguage() const {
    QSettings s;
    return s.value("target_language", "English").toString();
}

void Settings::setTargetLanguage(const QString &lang) {
    QSettings s;
    s.setValue("target_language", lang);
    emit settingsChanged();
}

Settings::Backend Settings::activeBackend() const {
    QSettings s;
    return static_cast<Backend>(s.value("active_backend", 0).toInt());
}

void Settings::setActiveBackend(Backend backend) {
    QSettings s;
    s.setValue("active_backend", static_cast<int>(backend));
    emit settingsChanged();
}

QKeySequence Settings::hotkey() const {
    QSettings s;
    return QKeySequence(s.value("hotkey", "Ctrl+Alt+T").toString());
}

void Settings::setHotkey(const QKeySequence &key) {
    QSettings s;
    s.setValue("hotkey", key.toString());
    emit settingsChanged();
}

int Settings::overlayFontSize() const {
    QSettings s;
    return s.value("overlay_font_size", 14).toInt();
}

void Settings::setOverlayFontSize(int size) {
    QSettings s;
    s.setValue("overlay_font_size", size);
    emit settingsChanged();
}

QString Settings::backendKey(Backend backend) {
    switch (backend) {
        case Backend::OpenAI: return "openai";
        case Backend::Gemini: return "gemini";
    }
    return "unknown";
}
```

**Step 3: Add to CMakeLists.txt**

Add `src/Settings.cpp` to the `add_executable` sources list.

**Step 4: Verify it compiles**

```bash
cmake --build build
```

**Step 5: Commit**

```bash
git add -A && git commit -m "feat: add Settings class for configuration persistence"
```

---

### Task 3: HotkeyManager — Global Hotkey Registration

**Files:**
- Create: `src/HotkeyManager.h`
- Create: `src/HotkeyManager.cpp`

**Step 1: Create HotkeyManager.h**

```cpp
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
```

**Step 2: Create HotkeyManager.cpp**

```cpp
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
    // On non-Windows, hotkey registration is a no-op (for development)
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
    // Qt key codes map directly to Windows VK codes for A-Z, 0-9
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return key; // Qt::Key_A == 0x41 == VK_A
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return key;
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        return VK_F1 + (key - Qt::Key_F1);
    // Add more mappings as needed
    return 0;
}
#endif
```

**Step 3: Add to CMakeLists.txt**

Add `src/HotkeyManager.cpp` to the sources list.

**Step 4: Verify it compiles**

```bash
cmake --build build
```

**Step 5: Commit**

```bash
git add -A && git commit -m "feat: add HotkeyManager with Win32 global hotkey support"
```

---

### Task 4: RegionSelector — Screen Region Selection + Capture

**Files:**
- Create: `src/RegionSelector.h`
- Create: `src/RegionSelector.cpp`

**Step 1: Create RegionSelector.h**

```cpp
#pragma once

#include <QWidget>
#include <QPixmap>
#include <QRect>
#include <QPoint>

class RegionSelector : public QWidget {
    Q_OBJECT
public:
    explicit RegionSelector(QWidget *parent = nullptr);

    void start();

signals:
    void regionSelected(const QRect &region, const QPixmap &screenshot);
    void selectionCancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPixmap captureRegion(const QRect &region);

    QPoint m_startPos;
    QPoint m_currentPos;
    bool m_selecting = false;
};
```

**Step 2: Create RegionSelector.cpp**

```cpp
#include "RegionSelector.h"

#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QKeyEvent>

RegionSelector::RegionSelector(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setCursor(Qt::CrossCursor);
}

void RegionSelector::start() {
    // Cover the entire virtual desktop (all monitors)
    QRect fullGeometry;
    for (QScreen *screen : QGuiApplication::screens()) {
        fullGeometry = fullGeometry.united(screen->geometry());
    }
    setGeometry(fullGeometry);

    m_selecting = false;
    m_startPos = QPoint();
    m_currentPos = QPoint();

    showFullScreen();
    activateWindow();
    grabMouse();
    grabKeyboard();
}

void RegionSelector::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    // Semi-transparent dark overlay
    painter.fillRect(rect(), QColor(0, 0, 0, 100));

    if (m_selecting) {
        QRect selection = QRect(m_startPos, m_currentPos).normalized();

        // Clear the selected region (make it transparent)
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(selection, Qt::transparent);

        // Draw border around selection
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        QPen pen(QColor(0, 120, 215), 2); // Windows 11 accent blue
        painter.setPen(pen);
        painter.drawRect(selection);
    }
}

void RegionSelector::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_startPos = event->globalPosition().toPoint();
        m_currentPos = m_startPos;
        m_selecting = true;
        update();
    }
}

void RegionSelector::mouseMoveEvent(QMouseEvent *event) {
    if (m_selecting) {
        m_currentPos = event->globalPosition().toPoint();
        update();
    }
}

void RegionSelector::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        releaseMouse();
        releaseKeyboard();
        hide();

        QRect region = QRect(m_startPos, event->globalPosition().toPoint()).normalized();

        // Minimum selection size
        if (region.width() < 10 || region.height() < 10) {
            emit selectionCancelled();
            return;
        }

        QPixmap screenshot = captureRegion(region);
        emit regionSelected(region, screenshot);
    }
}

void RegionSelector::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        m_selecting = false;
        releaseMouse();
        releaseKeyboard();
        hide();
        emit selectionCancelled();
    }
}

QPixmap RegionSelector::captureRegion(const QRect &region) {
    QScreen *screen = QGuiApplication::screenAt(region.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    return screen->grabWindow(0,
        region.x() - screen->geometry().x(),
        region.y() - screen->geometry().y(),
        region.width(),
        region.height());
}
```

**Step 3: Add to CMakeLists.txt**

Add `src/RegionSelector.cpp` to the sources list.

**Step 4: Verify it compiles**

```bash
cmake --build build
```

**Step 5: Commit**

```bash
git add -A && git commit -m "feat: add RegionSelector for screen area capture"
```

---

### Task 5: AIService — Abstract Interface + OpenAI Backend

**Files:**
- Create: `src/AIService.h`
- Create: `src/OpenAIBackend.h`
- Create: `src/OpenAIBackend.cpp`

**Step 1: Create AIService.h**

```cpp
#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

class AIService : public QObject {
    Q_OBJECT
public:
    explicit AIService(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~AIService() = default;

    virtual QString name() const = 0;
    virtual void translate(const QByteArray &pngImageData,
                           const QString &targetLanguage) = 0;
    virtual void cancel() {}

signals:
    void translationReady(const QString &translatedText);
    void translationFailed(const QString &errorMessage);
};
```

**Step 2: Create OpenAIBackend.h**

```cpp
#pragma once

#include "AIService.h"
#include <QThread>

class OpenAIBackend : public AIService {
    Q_OBJECT
public:
    explicit OpenAIBackend(const QString &apiKey, QObject *parent = nullptr);

    QString name() const override { return "OpenAI"; }
    void translate(const QByteArray &pngImageData,
                   const QString &targetLanguage) override;
    void cancel() override;

private:
    QString m_apiKey;
    bool m_cancelled = false;
};
```

**Step 3: Create OpenAIBackend.cpp**

```cpp
#include "OpenAIBackend.h"

#include <QBuffer>
#include <QtConcurrent>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

OpenAIBackend::OpenAIBackend(const QString &apiKey, QObject *parent)
    : AIService(parent), m_apiKey(apiKey) {}

void OpenAIBackend::translate(const QByteArray &pngImageData,
                               const QString &targetLanguage) {
    m_cancelled = false;

    QString apiKey = m_apiKey;
    QString lang = targetLanguage;
    QByteArray imageData = pngImageData;

    QtConcurrent::run([this, apiKey, lang, imageData]() {
        try {
            QString base64Image = QString::fromLatin1(imageData.toBase64());
            QString dataUrl = "data:image/png;base64," + base64Image;

            QString prompt = QString(
                "OCR the text in this image and translate it to %1. "
                "Return ONLY the translated text, nothing else. "
                "If no text is found, respond with '[No text detected]'."
            ).arg(lang);

            json payload = {
                {"model", "gpt-4o"},
                {"messages", {{
                    {"role", "user"},
                    {"content", {
                        {{"type", "text"}, {"text", prompt.toStdString()}},
                        {{"type", "image_url"}, {"image_url", {{"url", dataUrl.toStdString()}}}}
                    }}
                }}},
                {"max_tokens", 4096}
            };

            cpr::Response response = cpr::Post(
                cpr::Url{"https://api.openai.com/v1/chat/completions"},
                cpr::Header{
                    {"Content-Type", "application/json"},
                    {"Authorization", "Bearer " + apiKey.toStdString()}
                },
                cpr::Body{payload.dump()},
                cpr::Timeout{30000}
            );

            if (m_cancelled) return;

            if (response.status_code != 200) {
                QString error = QString("API error (HTTP %1): %2")
                    .arg(response.status_code)
                    .arg(QString::fromStdString(response.text).left(200));
                QMetaObject::invokeMethod(this, [this, error]() {
                    emit translationFailed(error);
                }, Qt::QueuedConnection);
                return;
            }

            json result = json::parse(response.text);
            std::string text = result["choices"][0]["message"]["content"];
            QString translated = QString::fromStdString(text).trimmed();

            QMetaObject::invokeMethod(this, [this, translated]() {
                emit translationReady(translated);
            }, Qt::QueuedConnection);

        } catch (const std::exception &e) {
            if (m_cancelled) return;
            QString error = QString("Request failed: %1").arg(e.what());
            QMetaObject::invokeMethod(this, [this, error]() {
                emit translationFailed(error);
            }, Qt::QueuedConnection);
        }
    });
}

void OpenAIBackend::cancel() {
    m_cancelled = true;
}
```

**Step 4: Add to CMakeLists.txt**

Add `src/OpenAIBackend.cpp` to the sources list. Note: `AIService.h` is header-only, no .cpp needed.

**Step 5: Verify it compiles**

```bash
cmake --build build
```

**Step 6: Commit**

```bash
git add -A && git commit -m "feat: add AIService interface and OpenAI vision backend"
```

---

### Task 6: GeminiBackend — Google Gemini Vision Backend

**Files:**
- Create: `src/GeminiBackend.h`
- Create: `src/GeminiBackend.cpp`

**Step 1: Create GeminiBackend.h**

```cpp
#pragma once

#include "AIService.h"

class GeminiBackend : public AIService {
    Q_OBJECT
public:
    explicit GeminiBackend(const QString &apiKey, QObject *parent = nullptr);

    QString name() const override { return "Gemini"; }
    void translate(const QByteArray &pngImageData,
                   const QString &targetLanguage) override;
    void cancel() override;

private:
    QString m_apiKey;
    bool m_cancelled = false;
};
```

**Step 2: Create GeminiBackend.cpp**

```cpp
#include "GeminiBackend.h"

#include <QtConcurrent>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

GeminiBackend::GeminiBackend(const QString &apiKey, QObject *parent)
    : AIService(parent), m_apiKey(apiKey) {}

void GeminiBackend::translate(const QByteArray &pngImageData,
                               const QString &targetLanguage) {
    m_cancelled = false;

    QString apiKey = m_apiKey;
    QString lang = targetLanguage;
    QByteArray imageData = pngImageData;

    QtConcurrent::run([this, apiKey, lang, imageData]() {
        try {
            QString base64Image = QString::fromLatin1(imageData.toBase64());

            QString prompt = QString(
                "OCR the text in this image and translate it to %1. "
                "Return ONLY the translated text, nothing else. "
                "If no text is found, respond with '[No text detected]'."
            ).arg(lang);

            json payload = {
                {"contents", {{
                    {"parts", {
                        {{"text", prompt.toStdString()}},
                        {{"inline_data", {
                            {"mime_type", "image/png"},
                            {"data", base64Image.toStdString()}
                        }}}
                    }}
                }}},
                {"generationConfig", {
                    {"maxOutputTokens", 4096}
                }}
            };

            QString url = QString(
                "https://generativelanguage.googleapis.com/v1beta/models/"
                "gemini-2.0-flash:generateContent?key=%1"
            ).arg(apiKey);

            cpr::Response response = cpr::Post(
                cpr::Url{url.toStdString()},
                cpr::Header{{"Content-Type", "application/json"}},
                cpr::Body{payload.dump()},
                cpr::Timeout{30000}
            );

            if (m_cancelled) return;

            if (response.status_code != 200) {
                QString error = QString("Gemini API error (HTTP %1): %2")
                    .arg(response.status_code)
                    .arg(QString::fromStdString(response.text).left(200));
                QMetaObject::invokeMethod(this, [this, error]() {
                    emit translationFailed(error);
                }, Qt::QueuedConnection);
                return;
            }

            json result = json::parse(response.text);
            std::string text = result["candidates"][0]["content"]["parts"][0]["text"];
            QString translated = QString::fromStdString(text).trimmed();

            QMetaObject::invokeMethod(this, [this, translated]() {
                emit translationReady(translated);
            }, Qt::QueuedConnection);

        } catch (const std::exception &e) {
            if (m_cancelled) return;
            QString error = QString("Request failed: %1").arg(e.what());
            QMetaObject::invokeMethod(this, [this, error]() {
                emit translationFailed(error);
            }, Qt::QueuedConnection);
        }
    });
}

void GeminiBackend::cancel() {
    m_cancelled = true;
}
```

**Step 3: Add to CMakeLists.txt**

Add `src/GeminiBackend.cpp` to the sources list.

**Step 4: Verify it compiles**

```bash
cmake --build build
```

**Step 5: Commit**

```bash
git add -A && git commit -m "feat: add Gemini vision backend for OCR+translation"
```

---

### Task 7: OverlayWindow — Result Display with Auto-resize

**Files:**
- Create: `src/OverlayWindow.h`
- Create: `src/OverlayWindow.cpp`

**Step 1: Create OverlayWindow.h**

```cpp
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QRect>
#include <QPropertyAnimation>

class OverlayWindow : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWindow(QWidget *parent = nullptr);

    void showLoading(const QRect &selectionRect);
    void showResult(const QString &text);
    void showError(const QString &error);
    void dismiss();

    void setFontSize(int size);

signals:
    void dismissed();
    void copyRequested();
    void saveRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    void setupUi();
    void adjustSizeToContent();

    QLabel *m_textLabel = nullptr;
    QLabel *m_loadingLabel = nullptr;
    QPushButton *m_copyBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_buttonBar = nullptr;

    QRect m_selectionRect;
    QString m_currentText;
    int m_fontSize = 14;

    static constexpr int PADDING = 12;
    static constexpr int BUTTON_BAR_HEIGHT = 36;
};
```

**Step 2: Create OverlayWindow.cpp**

```cpp
#include "OverlayWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPainter>
#include <QClipboard>
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QFileDialog>
#include <QTextStream>
#include <QFontMetrics>

OverlayWindow::OverlayWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::Tool | Qt::BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating, false);

    setupUi();
}

void OverlayWindow::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(PADDING, PADDING, PADDING, PADDING);
    mainLayout->setSpacing(8);

    // Loading label
    m_loadingLabel = new QLabel("Translating...", this);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("color: #cccccc; font-size: 14px;");

    // Result text label
    m_textLabel = new QLabel(this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textLabel->setStyleSheet(
        QString("color: #ffffff; font-size: %1px; line-height: 1.4;")
            .arg(m_fontSize));
    m_textLabel->hide();

    // Button bar
    m_buttonBar = new QWidget(this);
    auto *btnLayout = new QHBoxLayout(m_buttonBar);
    btnLayout->setContentsMargins(0, 4, 0, 0);
    btnLayout->setSpacing(8);

    m_copyBtn = new QPushButton("Copy", m_buttonBar);
    m_saveBtn = new QPushButton("Save", m_buttonBar);
    m_closeBtn = new QPushButton("Close", m_buttonBar);

    QString btnStyle =
        "QPushButton {"
        "  background: rgba(255,255,255,0.15);"
        "  color: #ffffff;"
        "  border: 1px solid rgba(255,255,255,0.3);"
        "  border-radius: 4px;"
        "  padding: 4px 12px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255,255,255,0.25);"
        "}";
    m_copyBtn->setStyleSheet(btnStyle);
    m_saveBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);

    btnLayout->addStretch();
    btnLayout->addWidget(m_copyBtn);
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_closeBtn);
    m_buttonBar->hide();

    mainLayout->addWidget(m_loadingLabel);
    mainLayout->addWidget(m_textLabel, 1);
    mainLayout->addWidget(m_buttonBar);

    // Connections
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_currentText);
        m_copyBtn->setText("Copied!");
        QTimer::singleShot(1500, this, [this]() {
            m_copyBtn->setText("Copy");
        });
    });

    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, "Save Translation", "translation.txt",
            "Text Files (*.txt);;All Files (*)");
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << m_currentText;
            }
        }
    });

    connect(m_closeBtn, &QPushButton::clicked, this, &OverlayWindow::dismiss);
}

void OverlayWindow::showLoading(const QRect &selectionRect) {
    m_selectionRect = selectionRect;
    m_loadingLabel->show();
    m_textLabel->hide();
    m_buttonBar->hide();

    setGeometry(selectionRect);
    show();
    activateWindow();
}

void OverlayWindow::showResult(const QString &text) {
    m_currentText = text;
    m_loadingLabel->hide();
    m_textLabel->setText(text);
    m_textLabel->show();
    m_buttonBar->show();

    adjustSizeToContent();
}

void OverlayWindow::showError(const QString &error) {
    m_loadingLabel->hide();
    m_textLabel->setText("Error: " + error);
    m_textLabel->setStyleSheet(
        QString("color: #ff6b6b; font-size: %1px;").arg(m_fontSize));
    m_textLabel->show();
    m_buttonBar->show();
    m_copyBtn->hide();
    m_saveBtn->hide();

    adjustSizeToContent();
}

void OverlayWindow::dismiss() {
    hide();
    emit dismissed();
}

void OverlayWindow::setFontSize(int size) {
    m_fontSize = size;
    m_textLabel->setStyleSheet(
        QString("color: #ffffff; font-size: %1px; line-height: 1.4;")
            .arg(m_fontSize));
}

void OverlayWindow::adjustSizeToContent() {
    QFontMetrics fm(m_textLabel->font());
    int availableWidth = m_selectionRect.width() - 2 * PADDING;
    QRect textRect = fm.boundingRect(
        QRect(0, 0, availableWidth, 99999),
        Qt::TextWordWrap, m_currentText);

    int neededHeight = textRect.height() + BUTTON_BAR_HEIGHT + 3 * PADDING;
    int newHeight = qMax(m_selectionRect.height(), neededHeight);

    // Clamp to screen bottom
    QScreen *screen = QGuiApplication::screenAt(m_selectionRect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    int maxHeight = screen->availableGeometry().bottom() - m_selectionRect.y();
    newHeight = qMin(newHeight, maxHeight);

    setGeometry(m_selectionRect.x(), m_selectionRect.y(),
                m_selectionRect.width(), newHeight);
}

void OverlayWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        dismiss();
    } else if (event->matches(QKeySequence::Copy)) {
        QApplication::clipboard()->setText(m_currentText);
    }
    QWidget::keyPressEvent(event);
}

void OverlayWindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Dark semi-transparent background with rounded corners
    painter.setBrush(QColor(30, 30, 30, 220));
    painter.setPen(QColor(0, 120, 215)); // Windows 11 accent blue border
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
}

bool OverlayWindow::event(QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate) {
        // Dismiss when clicking outside
        dismiss();
        return true;
    }
    return QWidget::event(event);
}
```

**Step 3: Add to CMakeLists.txt**

Add `src/OverlayWindow.cpp` to the sources list. Also add `find_package(Qt6 REQUIRED COMPONENTS ... Concurrent)` and link `Qt6::Concurrent`.

**Step 4: Verify it compiles**

```bash
cmake --build build
```

**Step 5: Commit**

```bash
git add -A && git commit -m "feat: add OverlayWindow with auto-resize and copy/save"
```

---

### Task 8: TrayApp — Orchestration + System Tray + Settings Dialog

**Files:**
- Create: `src/TrayApp.h`
- Create: `src/TrayApp.cpp`
- Modify: `src/main.cpp` — replace placeholder with TrayApp

**Step 1: Create TrayApp.h**

```cpp
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
```

**Step 2: Create TrayApp.cpp**

```cpp
#include "TrayApp.h"
#include "OpenAIBackend.h"
#include "GeminiBackend.h"

#include <QApplication>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QKeySequenceEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QBuffer>
#include <QMessageBox>

TrayApp::TrayApp(QObject *parent)
    : QObject(parent)
{
    m_settings = new Settings(this);
    m_hotkeyManager = new HotkeyManager(this);
    m_regionSelector = new RegionSelector();
    m_overlayWindow = new OverlayWindow();
}

TrayApp::~TrayApp() {
    delete m_regionSelector;
    delete m_overlayWindow;
}

void TrayApp::initialize() {
    createTrayIcon();
    createAIService();
    registerHotkey();

    // Connections
    connect(m_hotkeyManager, &HotkeyManager::hotkeyTriggered,
            this, &TrayApp::onHotkeyTriggered);
    connect(m_regionSelector, &RegionSelector::regionSelected,
            this, &TrayApp::onRegionSelected);

    m_overlayWindow->setFontSize(m_settings->overlayFontSize());
}

void TrayApp::onHotkeyTriggered() {
    m_overlayWindow->dismiss();
    m_regionSelector->start();
}

void TrayApp::onRegionSelected(const QRect &region, const QPixmap &screenshot) {
    m_overlayWindow->showLoading(region);

    // Encode screenshot to PNG bytes
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, "PNG");
    buffer.close();

    // Ensure AI service is current
    createAIService();

    if (!m_aiService) {
        m_overlayWindow->showError("No API key configured. Right-click tray icon → Settings.");
        return;
    }

    m_aiService->translate(imageData, m_settings->targetLanguage());
}

void TrayApp::onTranslationReady(const QString &text) {
    m_overlayWindow->showResult(text);
}

void TrayApp::onTranslationFailed(const QString &error) {
    m_overlayWindow->showError(error);
}

void TrayApp::createTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip("TransIt - Screen Translator");

    // Use a default icon (can be replaced with custom .ico later)
    m_trayIcon->setIcon(QApplication::style()->standardIcon(
        QStyle::SP_ComputerIcon));

    m_trayMenu = new QMenu();

    QAction *settingsAction = m_trayMenu->addAction("Settings...");
    connect(settingsAction, &QAction::triggered, this, &TrayApp::showSettingsDialog);

    m_trayMenu->addSeparator();

    QAction *quitAction = m_trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    m_trayIcon->showMessage("TransIt", "Running in background. Press "
        + m_settings->hotkey().toString() + " to translate.",
        QSystemTrayIcon::Information, 3000);
}

void TrayApp::createAIService() {
    if (m_aiService) {
        m_aiService->cancel();
        disconnect(m_aiService, nullptr, this, nullptr);
        m_aiService->deleteLater();
        m_aiService = nullptr;
    }

    Settings::Backend backend = m_settings->activeBackend();
    QString apiKey = m_settings->apiKey(backend);

    if (apiKey.isEmpty())
        return;

    switch (backend) {
        case Settings::Backend::OpenAI:
            m_aiService = new OpenAIBackend(apiKey, this);
            break;
        case Settings::Backend::Gemini:
            m_aiService = new GeminiBackend(apiKey, this);
            break;
    }

    if (m_aiService) {
        connect(m_aiService, &AIService::translationReady,
                this, &TrayApp::onTranslationReady);
        connect(m_aiService, &AIService::translationFailed,
                this, &TrayApp::onTranslationFailed);
    }
}

void TrayApp::registerHotkey() {
    if (!m_hotkeyManager->registerHotkey(m_settings->hotkey())) {
        // Hotkey registration may fail on Linux dev environment — that's OK
        qWarning("Failed to register hotkey. This is expected on non-Windows.");
    }
}

void TrayApp::showSettingsDialog() {
    QDialog dialog;
    dialog.setWindowTitle("TransIt Settings");
    dialog.setMinimumWidth(400);

    auto *layout = new QFormLayout(&dialog);

    // Backend selection
    auto *backendCombo = new QComboBox();
    backendCombo->addItem("OpenAI (GPT-4o)", static_cast<int>(Settings::Backend::OpenAI));
    backendCombo->addItem("Google Gemini", static_cast<int>(Settings::Backend::Gemini));
    backendCombo->setCurrentIndex(static_cast<int>(m_settings->activeBackend()));
    layout->addRow("AI Backend:", backendCombo);

    // API Keys
    auto *openaiKeyEdit = new QLineEdit(m_settings->apiKey(Settings::Backend::OpenAI));
    openaiKeyEdit->setEchoMode(QLineEdit::Password);
    openaiKeyEdit->setPlaceholderText("sk-...");
    layout->addRow("OpenAI API Key:", openaiKeyEdit);

    auto *geminiKeyEdit = new QLineEdit(m_settings->apiKey(Settings::Backend::Gemini));
    geminiKeyEdit->setEchoMode(QLineEdit::Password);
    geminiKeyEdit->setPlaceholderText("AI...");
    layout->addRow("Gemini API Key:", geminiKeyEdit);

    // Target language
    auto *langCombo = new QComboBox();
    langCombo->setEditable(true);
    QStringList languages = {
        "English", "Chinese (Simplified)", "Chinese (Traditional)",
        "Japanese", "Korean", "Spanish", "French", "German",
        "Russian", "Portuguese", "Arabic", "Hindi"
    };
    langCombo->addItems(languages);
    langCombo->setCurrentText(m_settings->targetLanguage());
    layout->addRow("Target Language:", langCombo);

    // Hotkey
    auto *hotkeyEdit = new QKeySequenceEdit(m_settings->hotkey());
    layout->addRow("Hotkey:", hotkeyEdit);

    // Font size
    auto *fontSizeSpin = new QSpinBox();
    fontSizeSpin->setRange(8, 32);
    fontSizeSpin->setValue(m_settings->overlayFontSize());
    layout->addRow("Overlay Font Size:", fontSizeSpin);

    // OK / Cancel
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        m_settings->setActiveBackend(
            static_cast<Settings::Backend>(backendCombo->currentData().toInt()));
        m_settings->setApiKey(Settings::Backend::OpenAI, openaiKeyEdit->text());
        m_settings->setApiKey(Settings::Backend::Gemini, geminiKeyEdit->text());
        m_settings->setTargetLanguage(langCombo->currentText());
        m_settings->setOverlayFontSize(fontSizeSpin->value());

        // Re-register hotkey if changed
        QKeySequence newHotkey = hotkeyEdit->keySequence();
        if (newHotkey != m_settings->hotkey()) {
            m_settings->setHotkey(newHotkey);
            registerHotkey();
        }

        m_overlayWindow->setFontSize(m_settings->overlayFontSize());
        createAIService();
    }
}
```

**Step 3: Update main.cpp**

```cpp
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
```

**Step 4: Update CMakeLists.txt — final sources list**

```cmake
add_executable(transIt WIN32
    src/main.cpp
    src/TrayApp.cpp
    src/Settings.cpp
    src/HotkeyManager.cpp
    src/RegionSelector.cpp
    src/OverlayWindow.cpp
    src/AIService.h
    src/OpenAIBackend.cpp
    src/GeminiBackend.cpp
)
```

Also add Qt6::Concurrent to find_package and target_link_libraries.

**Step 5: Verify it compiles**

```bash
cmake --build build
```

**Step 6: Commit**

```bash
git add -A && git commit -m "feat: add TrayApp orchestration and settings dialog"
```

---

### Task 9: Resource Files + Windows Metadata

**Files:**
- Create: `resources/transIt.qrc`
- Create: `resources/transIt.rc`

**Step 1: Create Qt resource file**

`resources/transIt.qrc`:
```xml
<RCC>
    <qresource prefix="/">
        <file>icons/tray.png</file>
    </qresource>
</RCC>
```

Note: Create a simple placeholder tray icon (16x16 or 32x32 PNG). Can replace later with a proper icon.

**Step 2: Create Windows resource file**

`resources/transIt.rc`:
```rc
#include <windows.h>

IDI_ICON1 ICON "icons/tray.ico"

VS_VERSION_INFO VERSIONINFO
FILEVERSION     0,1,0,0
PRODUCTVERSION  0,1,0,0
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904E4"
        BEGIN
            VALUE "CompanyName",      "TransIt"
            VALUE "FileDescription",  "Screen OCR + Translation Utility"
            VALUE "FileVersion",      "0.1.0"
            VALUE "ProductName",      "TransIt"
            VALUE "ProductVersion",   "0.1.0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1252
    END
END
```

**Step 3: Add resource file to CMakeLists.txt**

Add to CMakeLists.txt:
```cmake
if(WIN32)
    target_sources(transIt PRIVATE resources/transIt.rc)
endif()

# Qt resources
qt6_add_resources(transIt "resources" PREFIX "/" FILES
    resources/icons/tray.png
)
```

**Step 4: Commit**

```bash
git add -A && git commit -m "feat: add Windows resource file and Qt resources"
```

---

### Task 10: Final CMakeLists.txt + Cross-Compile Verification

**Files:**
- Modify: `CMakeLists.txt` — ensure all pieces fit

**Step 1: Finalize CMakeLists.txt**

Ensure the complete CMakeLists.txt looks like:

```cmake
cmake_minimum_required(VERSION 3.20)
project(transIt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

add_executable(transIt WIN32
    src/main.cpp
    src/TrayApp.cpp
    src/Settings.cpp
    src/HotkeyManager.cpp
    src/RegionSelector.cpp
    src/OverlayWindow.cpp
    src/OpenAIBackend.cpp
    src/GeminiBackend.cpp
    resources/transIt.qrc
)

target_include_directories(transIt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(transIt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    nlohmann_json::nlohmann_json
    cpr::cpr
)

if(WIN32)
    target_sources(transIt PRIVATE resources/transIt.rc)
    target_link_libraries(transIt PRIVATE user32 gdi32)
endif()

# Install
install(TARGETS transIt RUNTIME DESTINATION bin)
```

**Step 2: Test cross-compile**

```bash
cmake -B build-win \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic
cmake --build build-win
```

**Step 3: Commit**

```bash
git add -A && git commit -m "feat: finalize CMakeLists.txt and verify cross-compilation"
```

---

## Summary

| Task | Component | Description |
|------|-----------|-------------|
| 1 | Scaffold | CMake + vcpkg + toolchain + main.cpp |
| 2 | Settings | QSettings persistence for all configuration |
| 3 | HotkeyManager | Win32 global hotkey registration |
| 4 | RegionSelector | Fullscreen overlay + rubber-band + screenshot |
| 5 | AIService + OpenAI | Abstract interface + GPT-4o vision backend |
| 6 | GeminiBackend | Google Gemini vision backend |
| 7 | OverlayWindow | Result display with auto-resize, copy, save |
| 8 | TrayApp | System tray orchestration + settings dialog |
| 9 | Resources | Icons, .rc file, Qt resources |
| 10 | Final Build | Complete CMakeLists.txt + cross-compile verify |

Tasks 2-7 are largely independent and can be parallelized. Tasks 8-10 depend on all prior tasks.
