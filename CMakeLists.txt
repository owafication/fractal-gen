cmake_minimum_required(VERSION 3.24)
project(MandelbrotLiveWallpaper VERSION 1.11.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MW_BUILD_TESTS "Build core tests" ON)
option(MW_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_library(MandelbrotCore STATIC
    src/Core/AdaptivePerformance.cpp
    src/Core/Animation.cpp
    src/Core/DeepZoom.cpp
    src/Core/Json.cpp
    src/Core/MandelbrotMath.cpp
    src/Core/Models.cpp
    src/Core/SettingsStore.cpp
    src/Core/StillImageRenderer.cpp
)
target_include_directories(MandelbrotCore PUBLIC src)

if(MSVC)
    target_compile_options(MandelbrotCore PRIVATE /W4 /permissive- /EHsc /utf-8)
    if(MW_WARNINGS_AS_ERRORS)
        target_compile_options(MandelbrotCore PRIVATE /WX)
    endif()
else()
    target_compile_options(MandelbrotCore PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(MW_WARNINGS_AS_ERRORS)
        target_compile_options(MandelbrotCore PRIVATE -Werror)
    endif()
endif()

if(WIN32)
    add_executable(MandelbrotWallpaper WIN32
        src/App/AdaptivePerformanceDialog.cpp
        src/App/DialogSupport.cpp
        src/App/AppWindow.cpp
        src/App/HighResRenderDialog.cpp
        src/App/EquationEditorDialog.cpp
        src/App/PaletteEditorDialog.cpp
        src/App/PrecisionDialog.cpp
        src/App/PresetManagerDialog.cpp
        src/App/QuickControllerWindow.cpp
        src/App/SettingsDialog.cpp
        src/App/SlideshowDialog.cpp
        src/App/main.cpp
        src/App/resources.rc
        src/Infrastructure/Logger.cpp
        src/Infrastructure/Paths.cpp
        src/Rendering/Direct3D11Renderer.cpp
        src/Rendering/GpuRenderer.cpp
        src/Rendering/OpenGLRenderer.cpp
        src/WindowsIntegration/DesktopHost.cpp
        src/WindowsIntegration/DisplayManager.cpp
        src/WindowsIntegration/StartupManager.cpp
        src/WindowsIntegration/SystemStateMonitor.cpp
        src/WindowsIntegration/TrayIcon.cpp
        src/WindowsIntegration/WallpaperController.cpp
    )
    target_include_directories(MandelbrotWallpaper PRIVATE src)
    target_link_libraries(MandelbrotWallpaper PRIVATE MandelbrotCore d3d11 dxgi d3dcompiler opengl32 comctl32 comdlg32 shell32 shlwapi user32 gdi32 advapi32 dwmapi wtsapi32 psapi windowscodecs ole32)
    target_compile_definitions(MandelbrotWallpaper PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    if(MSVC)
        target_compile_options(MandelbrotWallpaper PRIVATE /W4 /permissive- /EHsc /utf-8)
        if(MW_WARNINGS_AS_ERRORS)
            target_compile_options(MandelbrotWallpaper PRIVATE /WX)
        endif()
        # resources.rc embeds src/App/app.manifest as RT_MANIFEST resource 1.
        # Disable link.exe manifest generation to avoid CVT1100 duplicate MANIFEST.
        target_link_options(MandelbrotWallpaper PRIVATE /MANIFEST:NO)
    else()
        target_compile_options(MandelbrotWallpaper PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(MW_BUILD_TESTS)
    enable_testing()
    add_executable(MandelbrotCoreTests tests/CoreTests.cpp)
    target_link_libraries(MandelbrotCoreTests PRIVATE MandelbrotCore)
    add_test(NAME MandelbrotCoreTests COMMAND MandelbrotCoreTests)
endif()

if(TARGET MandelbrotWallpaper)
    install(TARGETS MandelbrotWallpaper RUNTIME DESTINATION .)
endif()
install(DIRECTORY assets/presets DESTINATION assets OPTIONAL)
install(FILES README.md LICENSE DESTINATION . OPTIONAL)
