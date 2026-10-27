cmake_minimum_required(VERSION 3.21)

project(NoctaliaWin VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 6.5+ recommended on Windows. (6.4 also builds; used for the Linux preview build.)
set(NOCTALIA_MIN_QT "6.5" CACHE STRING "Minimum Qt version")
find_package(Qt6 ${NOCTALIA_MIN_QT} REQUIRED COMPONENTS Core Gui Qml Quick Network)
qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/core/Config.cpp src/core/Config.h
    src/core/Theme.cpp src/core/Theme.h
    src/core/Glyphs.cpp src/core/Glyphs.h
    src/services/AudioService.cpp src/services/AudioService.h
    src/services/SystemServices.cpp src/services/SystemServices.h
    src/services/ShellService.cpp src/services/ShellService.h
    src/services/LauncherService.cpp src/services/LauncherService.h
    src/services/PanelService.cpp src/services/PanelService.h
    src/services/IconProvider.cpp src/services/IconProvider.h
    src/core/MathEval.cpp src/core/MathEval.h
    src/core/Worker.h
    src/services/DeviceServices.cpp src/services/DeviceServices.h
    src/services/WeatherService.cpp src/services/WeatherService.h
    src/services/WallpaperService.cpp src/services/WallpaperService.h
    src/services/ClipboardService.cpp src/services/ClipboardService.h
    src/services/DockService.cpp src/services/DockService.h
    src/platform/Platform.h
)

if(WIN32)
  list(APPEND SOURCES
      src/platform/win/WinCommon.cpp
      src/platform/win/WinAudio.cpp
      src/platform/win/WinDesktops.cpp
      src/platform/win/WinAppBar.cpp
      src/platform/win/WinApps.cpp
      src/platform/win/WinWindows.cpp
      src/platform/win/WinAudioDevices.cpp
      src/platform/win/WinSystem.cpp
      src/platform/win/WinWifi.cpp
      src/platform/win/WinDisplays.cpp
      src/platform/win/WinWallpaper.cpp
      src/platform/win/WinWindowUtil.h
      src/platform/win/WinInput.h
  )
  if(MSVC)
    # Media controls, radios and Bluetooth use C++/WinRT (part of the Windows SDK).
    list(APPEND SOURCES src/platform/win/WinMedia.cpp src/platform/win/WinRadios.cpp)
  else()
    list(APPEND SOURCES src/platform/win/WinRtStub.cpp)
  endif()
else()
  # Fake data so the UI can be developed and previewed on Linux/macOS.
  list(APPEND SOURCES src/platform/stub/PlatformStub.cpp)
endif()

# Noctalia's palette generator (Material You from the wallpaper + fixed palettes), unchanged.
file(GLOB NOCTALIA_THEME_SOURCES CONFIGURE_DEPENDS
    third_party/noctalia/theme/*.cpp
    third_party/noctalia/render/core/color.cpp
    third_party/material_color_utilities/cpp/*/*.cc)
add_library(noctalia_theme STATIC ${NOCTALIA_THEME_SOURCES})
target_compile_features(noctalia_theme PUBLIC cxx_std_23)
target_include_directories(noctalia_theme PUBLIC third_party/noctalia third_party/material_color_utilities)
if(MSVC)
  target_compile_options(noctalia_theme PRIVATE /W0 /utf-8 /permissive-)
else()
  target_compile_options(noctalia_theme PRIVATE -w)
endif()

qt_add_executable(noctalia-win WIN32 ${SOURCES})

target_include_directories(noctalia-win PRIVATE src)
target_compile_definitions(noctalia-win PRIVATE NOCTALIA_WIN_VERSION="${PROJECT_VERSION}")

file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS qml/*.qml qml/*/qmldir)
qt_add_resources(noctalia-win "qml"
    PREFIX "/"
    FILES ${QML_FILES}
)
qt_add_resources(noctalia-win "assets"
    PREFIX "/"
    BIG_RESOURCES
    FILES
        assets/palettes.json
        assets/glyphs.json
        assets/emoji.json
        assets/noctalia.svg
        assets/fonts/noctalia-tabler.ttf
)

target_link_libraries(noctalia-win PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Network noctalia_theme)

if(WIN32)
  target_compile_definitions(noctalia-win PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(noctalia-win PRIVATE ole32 oleaut32 user32 shell32 advapi32 dwmapi powrprof uuid gdi32 propsys wlanapi dxva2 wbemuuid)
  if(MSVC)
    target_compile_options(noctalia-win PRIVATE /W4 /permissive- /utf-8 /bigobj)
    target_link_libraries(noctalia-win PRIVATE windowsapp)
  endif()
  set_target_properties(noctalia-win PROPERTIES OUTPUT_NAME "noctalia")
else()
  target_compile_options(noctalia-win PRIVATE -Wall -Wextra)
endif()

install(TARGETS noctalia-win RUNTIME DESTINATION .)
