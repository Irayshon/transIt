# TransIt — Design Document

## Overview

TransIt is a lightweight Windows 11 background utility for instant screen-region OCR and translation. The user presses a global hotkey, selects any screen region with the mouse, and the tool captures the area, sends it to a multimodal AI API for combined OCR + translation, then displays the result as a direct overlay on top of the original text.

**Stack:** C++17, Qt6, vcpkg, MinGW-w64 cross-compilation (developed on Linux, targets Windows)

## User Experience Flow

1. TransIt runs silently in the system tray
2. User presses `Ctrl+Alt+T` (configurable)
3. Screen dims slightly, crosshair cursor appears
4. User drags to select a region — screenshot is captured
5. A loading spinner appears at the selection location
6. AI returns translation — overlay replaces spinner with translated text
7. User can: copy text, save to file, or dismiss (Esc / click outside)
8. Overlay auto-expands downward if translated text is longer than the selected area

## Architecture

### Components

| Component | File | Responsibility |
|-----------|------|----------------|
| **TrayApp** | `TrayApp.h/cpp` | System tray icon, settings menu, orchestrates the full flow |
| **HotkeyManager** | `HotkeyManager.h/cpp` | Register/unregister global hotkeys via Win32 `RegisterHotKey` |
| **RegionSelector** | `RegionSelector.h/cpp` | Fullscreen transparent overlay, rubber-band mouse selection, screenshot capture |
| **OverlayWindow** | `OverlayWindow.h/cpp` | Frameless always-on-top result window, auto-resize, copy/save buttons |
| **AIService** | `AIService.h/cpp` | Abstract interface for AI backends |
| **OpenAIBackend** | `OpenAIBackend.h/cpp` | GPT-4o vision implementation |
| **GeminiBackend** | `GeminiBackend.h/cpp` | Gemini vision implementation |
| **Settings** | `Settings.h/cpp` | QSettings wrapper for API keys, language, hotkey, backend selection |

### Data Flow

```
TrayApp::onHotkeyTriggered()
  |
  +-> RegionSelector::start()
  |     - Fullscreen transparent widget (always on top)
  |     - Mouse press: record start point
  |     - Mouse move: draw rubber-band rectangle
  |     - Mouse release: QScreen::grabWindow(rect) -> QPixmap
  |     - Emit: regionSelected(QRect, QPixmap)
  |     - ESC: cancel
  |
  +-> TrayApp::onRegionSelected(rect, screenshot)
  |     - Show OverlayWindow at rect with spinner
  |     - Encode screenshot to base64 PNG
  |     - Call AIService::translate() async (QtConcurrent)
  |
  +-> AIService (worker thread)
  |     - POST image + prompt to backend API
  |     - Prompt: "OCR the text in this image and translate to {lang}. Return ONLY the translated text."
  |     - Parse response -> translated text
  |     - Emit: translationReady(QString) or translationFailed(QString)
  |
  +-> OverlayWindow::showResult(text)
        - Replace spinner with text (QLabel, word-wrap)
        - Auto-resize via QFontMetrics
        - If text fits in selection rect: use original size
        - If overflow: expand downward, clamp to screen bottom
        - Show copy + save buttons
        - Click outside or ESC: dismiss
```

## AI Integration

### Single-call OCR + Translation

The screenshot is sent directly to a multimodal AI model (GPT-4o, Gemini) which performs OCR and translation in one API call. No local OCR engine needed.

**Prompt template:**
```
OCR the text in this image and translate it to {target_language}.
Return ONLY the translated text, nothing else.
```

**OpenAI API payload:**
```json
{
  "model": "gpt-4o",
  "messages": [{
    "role": "user",
    "content": [
      {"type": "text", "text": "OCR the text in this image and translate to English. Return ONLY the translated text."},
      {"type": "image_url", "url": {"url": "data:image/png;base64,{screenshot_b64}"}}
    ]
  }],
  "max_tokens": 4096
}
```

### Pluggable Backend Design

`AIService` is an abstract base class:
```cpp
class AIService : public QObject {
    Q_OBJECT
public:
    virtual void translate(const QByteArray& imageData, const QString& targetLang) = 0;
signals:
    void translationReady(const QString& text);
    void translationFailed(const QString& error);
};
```

Each backend (OpenAI, Gemini, future additions) implements this interface. The active backend is selected in settings.

## Overlay Window Behavior

### Auto-sizing Logic

```cpp
void OverlayWindow::showResult(const QString& text) {
    QFontMetrics fm(label->font());
    QRect textRect = fm.boundingRect(
        QRect(0, 0, selectionRect.width(), INT_MAX),
        Qt::TextWordWrap, text
    );
    int newHeight = qMax(selectionRect.height(),
                         textRect.height() + buttonBarHeight + padding);
    int maxHeight = screen->availableGeometry().bottom() - selectionRect.y();
    resize(selectionRect.width(), qMin(newHeight, maxHeight));
}
```

### Overlay Properties
- Frameless (`Qt::FramelessWindowHint`)
- Always on top (`Qt::WindowStaysOnTopHint`)
- Semi-transparent background (dark with ~85% opacity)
- White text, readable font
- Copy button: copies translated text to clipboard
- Save button: saves to file (text or screenshot+text)
- Dismissed via ESC key or clicking outside the overlay

## Project Structure

```
transIt/
├── CMakeLists.txt
├── vcpkg.json
├── cmake/
│   └── mingw-w64-toolchain.cmake
├── src/
│   ├── main.cpp
│   ├── TrayApp.h / TrayApp.cpp
│   ├── HotkeyManager.h / HotkeyManager.cpp
│   ├── RegionSelector.h / RegionSelector.cpp
│   ├── OverlayWindow.h / OverlayWindow.cpp
│   ├── AIService.h / AIService.cpp
│   ├── OpenAIBackend.h / OpenAIBackend.cpp
│   ├── GeminiBackend.h / GeminiBackend.cpp
│   └── Settings.h / Settings.cpp
├── resources/
│   ├── transIt.qrc
│   ├── icons/
│   │   └── tray.ico
│   └── transIt.rc
└── docs/
    └── plans/
```

## Dependencies (vcpkg)

| Package | Purpose |
|---------|---------|
| `qt6` | GUI framework, system tray, image handling |
| `nlohmann-json` | JSON parsing for API requests/responses |
| `cpr` | HTTP client (libcurl wrapper) for AI API calls |
| `openssl` | HTTPS support for API communication |

## Cross-Compilation (Linux -> Windows)

**Toolchain:** MinGW-w64 (`x86_64-w64-mingw32`)
**vcpkg triplet:** `x64-mingw-dynamic` (or `x64-mingw-static` for single .exe)

### Toolchain file (`cmake/mingw-w64-toolchain.cmake`)
```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

### Build commands
```bash
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Testing
- Smoke tests: Wine on Linux
- Full testing: Windows VM or physical machine

## Settings

Stored via `QSettings("TransIt", "TransIt")`:
- **API key** (per backend, encrypted)
- **Target language** (default: English)
- **Hotkey** (default: Ctrl+Alt+T)
- **Active backend** (OpenAI / Gemini)
- **Font size** for overlay text

## Future Considerations (out of scope for v1)

- Translation history/log
- Auto-start with Windows
- Multiple language quick-switch
- Streaming translation (show partial results as they arrive)
- Custom prompts for specialized domains
