cmake_minimum_required(VERSION 3.21)

project(NoctaliaWin VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
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
    src/platform/Platform.h
)

if(WIN32)
  list(APPEND SOURCES
      src/platform/win/WinCommon.cpp
      src/platform/win/WinAudio.cpp
      src/platform/win/WinDesktops.cpp
      src/platform/win/WinAppBar.cpp
      src/platform/win/WinInput.h
  )
else()
  # Fake data so the UI can be developed and previewed on Linux/macOS.
  list(APPEND SOURCES src/platform/stub/PlatformStub.cpp)
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
        assets/noctalia.svg
        assets/fonts/noctalia-tabler.ttf
)

target_link_libraries(noctalia-win PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Network)

if(WIN32)
  target_compile_definitions(noctalia-win PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(noctalia-win PRIVATE ole32 user32 shell32 advapi32 dwmapi powrprof uuid)
  if(MSVC)
    target_compile_options(noctalia-win PRIVATE /W4 /permissive- /utf-8)
  endif()
  set_target_properties(noctalia-win PROPERTIES OUTPUT_NAME "noctalia")
else()
  target_compile_options(noctalia-win PRIVATE -Wall -Wextra)
endif()

install(TARGETS noctalia-win RUNTIME DESTINATION .)
