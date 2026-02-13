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
