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
    src/Settings.cpp
    src/HotkeyManager.cpp
    src/RegionSelector.cpp
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
