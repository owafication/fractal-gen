#include "App/AppWindow.h"

#include "Infrastructure/Logger.h"
#include "Infrastructure/Paths.h"

#ifdef _WIN32
#include <windows.h>
#include <shellscalingapi.h>
#endif

#include <exception>
#include <string>

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    mw::Logger::Instance().Initialise(mw::Paths::LogDirectory());
    mw::LogInfo("Application startup.");

    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\MandelbrotLiveWallpaper.SingleInstance");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND existing = FindWindowW(L"MandelbrotLiveWallpaperControl", nullptr)) {
            ShowWindow(existing, SW_RESTORE);
            SetForegroundWindow(existing);
        }
        CloseHandle(mutex);
        return 0;
    }

    int exitCode = 1;
    try {
        mw::AppWindow app;
        std::string error;
        const bool startHidden = commandLine && wcsstr(commandLine, L"--tray") != nullptr;
        if (!app.Create(instance, showCommand, startHidden, error)) {
            mw::LogError("Application window startup failed: " + error);
            MessageBoxW(nullptr, L"Mandelbrot Live Wallpaper could not start. See the local log for diagnostic information.",
                        L"Startup Error", MB_OK | MB_ICONERROR);
        } else {
            exitCode = app.RunMessageLoop();
        }
    } catch (const std::exception& exception) {
        mw::LogError(std::string("Unhandled exception: ") + exception.what());
        MessageBoxW(nullptr, L"Mandelbrot Live Wallpaper stopped after an unexpected error. The desktop wallpaper window has been released.",
                    L"Unexpected Error", MB_OK | MB_ICONERROR);
    } catch (...) {
        mw::LogError("Unhandled non-standard exception.");
        MessageBoxW(nullptr, L"Mandelbrot Live Wallpaper stopped after an unexpected error. The desktop wallpaper window has been released.",
                    L"Unexpected Error", MB_OK | MB_ICONERROR);
    }

    mw::LogInfo("Application shutdown.");
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return exitCode;
}
#endif
