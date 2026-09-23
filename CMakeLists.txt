cmake_minimum_required(VERSION 3.24)
project(MandelbrotLiveWallpaper VERSION 1.13.1 LANGUAGES CXX)

include(cmake/VerifyVersionConsistency.cmake)
mw_verify_version_consistency("${CMAKE_SOURCE_DIR}" "${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MW_BUILD_TESTS "Build core tests" ON)
option(MW_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(MW_BUILD_VISUAL_FIXTURES "Build the PH-02 production-renderer visual fixture tool" ON)

set(MW_BOOST_MULTIPRECISION_ROOT "${CMAKE_SOURCE_DIR}/third_party/boost-multiprecision-1.83.0")
if(NOT EXISTS "${MW_BOOST_MULTIPRECISION_ROOT}/include/boost/multiprecision/cpp_bin_float.hpp"
   OR NOT EXISTS "${MW_BOOST_MULTIPRECISION_ROOT}/include/boost/config.hpp"
   OR NOT EXISTS "${MW_BOOST_MULTIPRECISION_ROOT}/LICENSE_1_0.txt")
    message(FATAL_ERROR "Pinned Boost.Multiprecision source package is incomplete.")
endif()
file(READ "${MW_BOOST_MULTIPRECISION_ROOT}/include/boost/version.hpp" MW_BOOST_VERSION_HEADER)
if(NOT MW_BOOST_VERSION_HEADER MATCHES "#define BOOST_VERSION 108300")
    message(FATAL_ERROR "Expected Boost 1.83.0 in the pinned Multiprecision source package.")
endif()
add_library(MandelbrotBoostMultiprecision INTERFACE)
target_include_directories(MandelbrotBoostMultiprecision SYSTEM INTERFACE
    "${MW_BOOST_MULTIPRECISION_ROOT}/include")
target_compile_definitions(MandelbrotBoostMultiprecision INTERFACE BOOST_MP_STANDALONE)

add_library(MandelbrotCore STATIC
    src/Core/AdaptivePerformance.cpp
    src/Core/Animation.cpp
    src/Core/DeepZoom.cpp
    src/Core/FractalScout.cpp
    src/Core/FrameSequenceExport.cpp
    src/Core/ExternalVideoExport.cpp
    src/Core/GeneralAnimation.cpp
    src/Core/Json.cpp
    src/Core/MandelbrotMath.cpp
    src/Core/Models.cpp
    src/Core/ProjectState.cpp
    src/Core/ProjectHistory.cpp
    src/Core/Precision/ExactCamera.cpp
    src/Core/Precision/ExactDecimal.cpp
    src/Core/Precision/HighPrecisionBackend.cpp
    src/Core/Precision/PrecisionPlanner.cpp
    src/Core/Precision/ReferenceOrbitService.cpp
    src/Core/SettingsStore.cpp
    src/Core/StillImageRenderer.cpp
    src/Core/VisualRegression.cpp
)
target_include_directories(MandelbrotCore PUBLIC src)
target_link_libraries(MandelbrotCore PRIVATE MandelbrotBoostMultiprecision)

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


if(MW_BUILD_VISUAL_FIXTURES)
    set(MW_SOURCE_REVISION "unavailable")
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE MW_GIT_RESULT
            OUTPUT_VARIABLE MW_GIT_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(MW_GIT_RESULT EQUAL 0 AND NOT MW_GIT_REVISION STREQUAL "")
            set(MW_SOURCE_REVISION "${MW_GIT_REVISION}")
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" status --porcelain
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                RESULT_VARIABLE MW_GIT_STATUS_RESULT
                OUTPUT_VARIABLE MW_GIT_STATUS
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(MW_GIT_STATUS_RESULT EQUAL 0 AND NOT MW_GIT_STATUS STREQUAL "")
                string(APPEND MW_SOURCE_REVISION "-dirty")
            endif()
        endif()
    endif()

    add_executable(MandelbrotVisualFixtures src/Tools/VisualFixtureTool.cpp)
    target_link_libraries(MandelbrotVisualFixtures PRIVATE MandelbrotCore)
    target_include_directories(MandelbrotVisualFixtures PRIVATE src)
    target_compile_definitions(MandelbrotVisualFixtures PRIVATE
        MW_PROJECT_VERSION="${PROJECT_VERSION}"
        MW_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}"
        MW_COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}"
        MW_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
        MW_SOURCE_REVISION="${MW_SOURCE_REVISION}"
    )
    if(MSVC)
        target_compile_options(MandelbrotVisualFixtures PRIVATE /W4 /permissive- /EHsc /utf-8)
        if(MW_WARNINGS_AS_ERRORS)
            target_compile_options(MandelbrotVisualFixtures PRIVATE /WX)
        endif()
    else()
        target_compile_options(MandelbrotVisualFixtures PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(MW_WARNINGS_AS_ERRORS)
            target_compile_options(MandelbrotVisualFixtures PRIVATE -Werror)
        endif()
    endif()
endif()

if(WIN32)
    add_executable(MandelbrotWallpaper WIN32
        src/App/AdaptivePerformanceDialog.cpp
        src/App/DialogSupport.cpp
        src/App/AppWindow.cpp
        src/App/HighResRenderDialog.cpp
        src/App/JourneySettingsDialog.cpp
        src/App/EquationEditorDialog.cpp
        src/App/FractalScoutDialog.cpp
        src/App/FrameSequenceExportDialog.cpp
        src/App/VideoExportDialog.cpp
        src/App/GeneralAnimationEditorDialog.cpp
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
        src/WindowsIntegration/ExternalProcess.cpp
        src/WindowsIntegration/DisplayManager.cpp
        src/WindowsIntegration/ImageCodec.cpp
        src/WindowsIntegration/StartupManager.cpp
        src/WindowsIntegration/SystemStateMonitor.cpp
        src/WindowsIntegration/TrayIcon.cpp
        src/WindowsIntegration/WallpaperController.cpp
    )
    target_include_directories(MandelbrotWallpaper PRIVATE src)
    target_link_libraries(MandelbrotWallpaper PRIVATE MandelbrotCore d3d11 dxgi d3dcompiler opengl32 comctl32 comdlg32 shell32 shlwapi user32 gdi32 advapi32 dwmapi wtsapi32 psapi windowscodecs ole32 mfplay propsys)
    target_compile_definitions(MandelbrotWallpaper PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN MW_PROJECT_VERSION="${PROJECT_VERSION}")
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

    if(MW_BUILD_TESTS)
        add_executable(MandelbrotD3D11WarpFixtures
            src/Tools/D3D11WarpFixtureTool.cpp
            src/Infrastructure/Logger.cpp
            src/Rendering/Direct3D11Renderer.cpp
        )
        target_include_directories(MandelbrotD3D11WarpFixtures PRIVATE src)
        target_link_libraries(MandelbrotD3D11WarpFixtures PRIVATE MandelbrotCore d3d11 dxgi d3dcompiler user32 gdi32)
        target_compile_definitions(MandelbrotD3D11WarpFixtures PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
        if(MSVC)
            target_compile_options(MandelbrotD3D11WarpFixtures PRIVATE /W4 /permissive- /EHsc /utf-8)
            if(MW_WARNINGS_AS_ERRORS)
                target_compile_options(MandelbrotD3D11WarpFixtures PRIVATE /WX)
            endif()
        endif()

        add_executable(MandelbrotOpenGLFixtures
            src/Tools/OpenGLFixtureTool.cpp
            src/Infrastructure/Logger.cpp
            src/Rendering/Direct3D11Renderer.cpp
            src/Rendering/GpuRenderer.cpp
            src/Rendering/OpenGLRenderer.cpp
        )
        target_include_directories(MandelbrotOpenGLFixtures PRIVATE src)
        target_link_libraries(MandelbrotOpenGLFixtures PRIVATE MandelbrotCore d3d11 dxgi d3dcompiler opengl32 user32 gdi32)
        target_compile_definitions(MandelbrotOpenGLFixtures PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
        if(MSVC)
            target_compile_options(MandelbrotOpenGLFixtures PRIVATE /W4 /permissive- /EHsc /utf-8)
            if(MW_WARNINGS_AS_ERRORS)
                target_compile_options(MandelbrotOpenGLFixtures PRIVATE /WX)
            endif()
        endif()

        add_executable(MandelbrotExternalVideoFixture
            src/Tools/ExternalVideoFixtureTool.cpp
            src/WindowsIntegration/ExternalProcess.cpp
            src/WindowsIntegration/ImageCodec.cpp
        )
        target_include_directories(MandelbrotExternalVideoFixture PRIVATE src)
        target_link_libraries(MandelbrotExternalVideoFixture PRIVATE MandelbrotCore windowscodecs ole32)
        target_compile_definitions(MandelbrotExternalVideoFixture PRIVATE
            UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN MW_PROJECT_VERSION="${PROJECT_VERSION}")
        if(MSVC)
            target_compile_options(MandelbrotExternalVideoFixture PRIVATE /W4 /permissive- /EHsc /utf-8)
            if(MW_WARNINGS_AS_ERRORS)
                target_compile_options(MandelbrotExternalVideoFixture PRIVATE /WX)
            endif()
        endif()

        add_executable(MandelbrotWindowsInteractionFixture
            src/Tools/WindowsInteractionFixtureTool.cpp
        )
        target_include_directories(MandelbrotWindowsInteractionFixture PRIVATE src)
        target_link_libraries(MandelbrotWindowsInteractionFixture PRIVATE MandelbrotCore user32 psapi)
        target_compile_definitions(MandelbrotWindowsInteractionFixture PRIVATE
            UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
        add_dependencies(MandelbrotWindowsInteractionFixture MandelbrotWallpaper)
        if(MSVC)
            target_compile_options(MandelbrotWindowsInteractionFixture PRIVATE /W4 /permissive- /EHsc /utf-8)
            if(MW_WARNINGS_AS_ERRORS)
                target_compile_options(MandelbrotWindowsInteractionFixture PRIVATE /WX)
            endif()
        endif()
    endif()
endif()

if(MW_BUILD_TESTS)
    enable_testing()
    add_executable(MandelbrotCoreTests tests/CoreTests.cpp)
    target_link_libraries(MandelbrotCoreTests PRIVATE MandelbrotCore)
    add_test(NAME MandelbrotCoreTests COMMAND MandelbrotCoreTests)

    add_executable(MandelbrotOrbitEncodingMeasurement
        src/Tools/OrbitEncodingMeasurementTool.cpp
    )
    target_include_directories(MandelbrotOrbitEncodingMeasurement PRIVATE src)
    target_link_libraries(MandelbrotOrbitEncodingMeasurement PRIVATE MandelbrotCore)
    if(MSVC)
        target_compile_options(MandelbrotOrbitEncodingMeasurement PRIVATE /W4 /permissive- /EHsc /utf-8)
        if(MW_WARNINGS_AS_ERRORS)
            target_compile_options(MandelbrotOrbitEncodingMeasurement PRIVATE /WX)
        endif()
    else()
        target_compile_options(MandelbrotOrbitEncodingMeasurement PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(MW_WARNINGS_AS_ERRORS)
            target_compile_options(MandelbrotOrbitEncodingMeasurement PRIVATE -Werror)
        endif()
    endif()
    add_test(NAME OrbitEncodingMeasurementSelfCheck COMMAND MandelbrotOrbitEncodingMeasurement)

    add_executable(MandelbrotPathTests
        tests/PathsTests.cpp
        src/Infrastructure/Paths.cpp
    )
    target_include_directories(MandelbrotPathTests PRIVATE src)
    if(MSVC)
        target_compile_options(MandelbrotPathTests PRIVATE /W4 /permissive- /EHsc /utf-8)
        if(MW_WARNINGS_AS_ERRORS)
            target_compile_options(MandelbrotPathTests PRIVATE /WX)
        endif()
    else()
        target_compile_options(MandelbrotPathTests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(MW_WARNINGS_AS_ERRORS)
            target_compile_options(MandelbrotPathTests PRIVATE -Werror)
        endif()
    endif()
    add_test(NAME MandelbrotPathTests COMMAND MandelbrotPathTests)

    if(TARGET MandelbrotVisualFixtures)
        add_test(
            NAME VisualFixtureSelfCheck
            COMMAND MandelbrotVisualFixtures
                --output-dir ${CMAKE_BINARY_DIR}/test_artifacts/visual
        )
        set_tests_properties(VisualFixtureSelfCheck PROPERTIES TIMEOUT 180)
    endif()
    if(TARGET MandelbrotD3D11WarpFixtures)
        add_test(NAME D3D11WarpFixtureSelfCheck COMMAND MandelbrotD3D11WarpFixtures)
        set_tests_properties(D3D11WarpFixtureSelfCheck PROPERTIES TIMEOUT 60)
    endif()
    if(TARGET MandelbrotOpenGLFixtures)
        add_test(NAME OpenGLFixtureSelfCheck COMMAND MandelbrotOpenGLFixtures)
        set_tests_properties(OpenGLFixtureSelfCheck PROPERTIES TIMEOUT 60)
    endif()
    if(TARGET MandelbrotWindowsInteractionFixture)
        add_test(
            NAME WindowsInteractionFixture
            COMMAND MandelbrotWindowsInteractionFixture
                $<TARGET_FILE:MandelbrotWallpaper>
                ${CMAKE_BINARY_DIR}/test_artifacts/windows-interaction
        )
        set_tests_properties(WindowsInteractionFixture PROPERTIES TIMEOUT 150 RUN_SERIAL TRUE)
    endif()

    add_test(
        NAME VersionConsistency
        COMMAND ${CMAKE_COMMAND}
            -DROOT_DIR=${CMAKE_SOURCE_DIR}
            -DEXPECTED_VERSION=${PROJECT_VERSION}
            -P ${CMAKE_SOURCE_DIR}/cmake/VerifyVersionConsistency.cmake
    )
endif()

if(TARGET MandelbrotWallpaper)
    install(TARGETS MandelbrotWallpaper RUNTIME DESTINATION .)
endif()
install(DIRECTORY assets/presets DESTINATION assets OPTIONAL)
install(FILES README.md LICENSE THIRD_PARTY_NOTICES.md DESTINATION . OPTIONAL)
