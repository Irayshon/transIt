# TransIt

A lightweight Windows 11 background utility that captures any screen region, performs OCR, translates the text, and overlays the result directly on the original location.

## Features

- **Hotkey-triggered screen capture** — press `Ctrl+Shift+T` to select any region
- **AI-powered OCR + Translation** — a single API call recognizes text and translates it
- **Smart overlay** — translated text is positioned where the original text was; falls back to a readable word-wrapped view when the content overflows
- **Pluggable AI backends** — OpenAI-compatible and Google Gemini, with configurable base URL / model / API key
- **Auto-detect source language** — only set your target language
- **Copy & Save** — right-click the overlay to copy text or save the image
- **System tray** — runs quietly in the background

## Quick Start

1. Download `TransIt-Windows-x64.zip` from the [latest release](https://github.com/Irayshon/transIt/releases/latest).
2. Extract and run `transIt.exe`.
3. Right-click the tray icon → **Settings**.
4. Choose a backend and enter your credentials:

| Provider | Backend | Base URL | Model (example) |
|----------|---------|----------|-----------------|
| Alibaba Qwen | OpenAI | `https://dashscope.aliyuncs.com/compatible-mode` | `qwen3-vl-flash` |
| OpenAI | OpenAI | `https://api.openai.com` | `gpt-4o` |
| DeepSeek | OpenAI | `https://api.deepseek.com` | `deepseek-chat` |
| Google | Gemini | *(built-in)* | `gemini-2.0-flash` |

5. Press `Ctrl+Shift+T`, drag to select a region, and the translation appears.

## Supported AI Providers

Any provider that exposes an **OpenAI-compatible** `/v1/chat/completions` endpoint works out of the box. Set the **Base URL**, **Model Name**, and **API Key** in Settings.

## Uninstall

TransIt stores settings in the Windows Registry at `HKCU\Software\TransIt`.
To clean up after deleting the executable, run the included `uninstall.bat` (or manually delete the registry key).

## Build from Source

**Requirements**: CMake 3.20+, MSVC 2022, Qt 6.7+, vcpkg

```powershell
git clone https://github.com/Irayshon/transIt.git
cd transIt
# set VCPKG_ROOT to your vcpkg installation
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_PREFIX_PATH="<path-to-Qt>/lib/cmake"
cmake --build build --config Release
```

Or let GitHub Actions build it — every push to `main` and every tag triggers a CI build.

## Tech Stack

| Component | Technology |
|-----------|------------|
| Language | C++17 |
| Framework | Qt 6 (Widgets, Network, Concurrent) |
| Build | CMake + vcpkg |
| AI | OpenAI-compatible API / Google Gemini API |
| CI | GitHub Actions (Windows) |

## License

[Apache License 2.0](LICENSE)