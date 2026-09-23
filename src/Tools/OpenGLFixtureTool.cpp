#include "Rendering/GpuRenderer.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
LRESULT CALLBACK FixtureWindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(window, message, wParam, lParam);
}

bool RenderExact(mw::GpuRenderer& renderer, const mw::RenderRegion& region,
                 const mw::RenderOptions& options, int width, int height,
                 const char* name, std::string& error) {
    const std::vector<mw::RenderRegion> regions{region};
    std::vector<std::uint32_t> first;
    std::vector<std::uint32_t> repeat;
    int capturedWidth = 0;
    int capturedHeight = 0;
    const bool firstOk = renderer.Render(regions, options, error) &&
        renderer.CapturePixels(first, capturedWidth, capturedHeight, error);
    const bool repeatOk = firstOk && renderer.Render(regions, options, error) &&
        renderer.CapturePixels(repeat, capturedWidth, capturedHeight, error);
    if (!repeatOk || capturedWidth != width || capturedHeight != height || first.empty() ||
        *std::min_element(first.begin(), first.end()) == *std::max_element(first.begin(), first.end()) ||
        repeat != first) {
        std::cerr << "FAIL: " << name << ": " << error << '\n';
        return false;
    }
    return true;
}
}

int main() {
    constexpr int width = 256;
    constexpr int height = 144;
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t className[] = L"MandelbrotOpenGLFixtureHost";
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = FixtureWindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.style = CS_OWNDC;
    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;
    const HWND host = CreateWindowExW(0, className, L"", WS_POPUP, 0, 0, width, height,
                                      nullptr, nullptr, instance, nullptr);
    if (!host) return 1;
    mw::GpuRenderer renderer;
    std::string error;
    if (!renderer.Initialise(host, error, mw::GpuBackendPreference::OpenGL)) {
        std::cerr << "FAIL: OpenGL initialise: " << error << '\n';
        DestroyWindow(host);
        return 1;
    }
    mw::RenderRegion region;
    region.pixels = RECT{0, 0, width, height};
    mw::RenderOptions options;
    options.renderScale = 1.0;
    options.timeSeconds = 0.0;
    const bool standardOk = RenderExact(renderer, region, options, width, height, "OpenGL standard", error);
    mw::RenderRegion bloom = region;
    bloom.equation.glowStrength = 1.0;
    bloom.equation.bloomThreshold = 0.12;
    bloom.equation.bloomSoftKnee = 0.5;
    bloom.equation.bloomRadius = 4;
    const bool bloomOk = standardOk && RenderExact(renderer, bloom, options, width, height, "OpenGL bloom", error);
    mw::RenderRegion perturbation = region;
    perturbation.camera.centreX = -0.743643887037151;
    perturbation.camera.centreY = 0.13182590420533;
    perturbation.camera.scale = 1.0e-5;
    perturbation.maximumIterations = 1000;
    mw::RenderOptions perturbationOptions = options;
    perturbationOptions.precision.mode = mw::PrecisionMode::Perturbation;
    const bool perturbationOk = bloomOk && RenderExact(renderer, perturbation, perturbationOptions,
                                                        width, height, "OpenGL perturbation", error);
    bool float64Ok = perturbationOk;
    const bool nativeFloat64 = renderer.Capabilities().nativeFloat64;
    if (float64Ok && nativeFloat64) {
        mw::RenderOptions float64Options = options;
        float64Options.precision.mode = mw::PrecisionMode::Float64;
        float64Options.precision.automaticFallback = false;
        float64Ok = RenderExact(renderer, region, float64Options, width, height,
                                "OpenGL native float64", error) &&
                    renderer.PrecisionDescription() == "GPU float64";
    }
    const std::string graphicsDescription = renderer.GraphicsDescription();
    renderer.Shutdown();
    DestroyWindow(host);
    if (!float64Ok) return 1;
    std::cout << "PASS: OpenGL standard, bloom and perturbation production readbacks"
              << (nativeFloat64 ? ", including native float64," : ", without native float64,") << " are exact at "
              << width << 'x' << height << "; " << graphicsDescription << '\n';
    return 0;
}
