#include "Rendering/Direct3D11Renderer.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
LRESULT CALLBACK FixtureWindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(window, message, wParam, lParam);
}

bool RenderExact(mw::Direct3D11Renderer& renderer, const mw::RenderRegion& region,
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
        std::cerr << "FAIL: " << name << ": " << error
                  << " dimensions=" << capturedWidth << 'x' << capturedHeight
                  << " pixels=" << first.size()
                  << " repeat=" << (repeat == first ? "equal" : "different") << '\n';
        return false;
    }
    return true;
}

bool VerifyOrbitTextureTransport(mw::Direct3D11Renderer& renderer,
                                 const mw::RenderRegion& region, std::string& error) {
    const mw::ReferenceOrbit expected = mw::BuildReferenceOrbitDouble(
        region.camera, region.equation, region.maximumIterations);
    mw::ReferenceOrbit captured;
    if (expected.points.empty() || !renderer.CaptureReferenceOrbitTexture(captured, error) ||
        captured.points.size() != expected.points.size()) {
        std::cerr << "FAIL: perturbation WARP reference-orbit texture transport: " << error
                  << " expected=" << expected.points.size()
                  << " captured=" << captured.points.size() << '\n';
        return false;
    }
    for (std::size_t point = 0; point < expected.points.size(); ++point) {
        for (std::size_t component = 0; component < expected.points[point].real.size(); ++component) {
            if (std::bit_cast<std::uint32_t>(captured.points[point].real[component]) !=
                    std::bit_cast<std::uint32_t>(expected.points[point].real[component]) ||
                std::bit_cast<std::uint32_t>(captured.points[point].imaginary[component]) !=
                    std::bit_cast<std::uint32_t>(expected.points[point].imaginary[component])) {
                std::cerr << "FAIL: perturbation WARP reference-orbit texture transport: "
                          << "float4 component changed at point=" << point
                          << " component=" << component << '\n';
                return false;
            }
        }
    }
    return true;
}

bool VerifyAutomaticPreviewPrecisionSwitches(mw::Direct3D11Renderer& renderer,
                                             const mw::RenderRegion& baseRegion,
                                             const mw::RenderOptions& baseOptions,
                                             std::string& error) {
    mw::RenderOptions options = baseOptions;
    options.precision.mode = mw::PrecisionMode::Automatic;
    mw::RenderRegion region = baseRegion;
    region.maximumIterations = 96;
    const auto renderAt = [&](double scale, const char* expected) {
        region.camera.scale = scale;
        error.clear();
        if (!renderer.Render({region}, options, error)) return false;
        const std::string actual = renderer.PrecisionDescription();
        if (actual == expected) return true;
        error = "Expected " + std::string(expected) + " but selected " + actual + '.';
        return false;
    };
    if (!renderAt(1.5, "GPU float32") ||
        !renderAt(1.0e-7, "Split high/low float") ||
        !renderAt(1.0e-14, "GPU perturbation / arbitrary-precision reference")) {
        if (error.empty()) {
            error = "Automatic preview precision did not move through float32, split-float and arbitrary-reference modes.";
        }
        return false;
    }
    return true;
}
}

int main(int argc, char** argv) {
    bool useWarp = true;
    if (argc == 2 && std::string(argv[1]) == "--hardware") useWarp = false;
    else if (argc > 1) {
        std::cerr << "Usage: MandelbrotD3D11WarpFixtures [--hardware]\n";
        return 2;
    }
    constexpr int width = 256;
    constexpr int height = 144;
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t className[] = L"MandelbrotD3D11WarpFixtureHost";
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = FixtureWindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.style = CS_OWNDC;
    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;
    const HWND host = CreateWindowExW(0, className, L"", WS_POPUP, 0, 0, width, height,
                                      nullptr, nullptr, instance, nullptr);
    if (!host) return 1;
    mw::Direct3D11Renderer renderer;
    std::string error;
    if (!renderer.Initialise(host, error, useWarp)) {
        std::cerr << "FAIL: " << (useWarp ? "WARP" : "hardware")
                  << " initialise: " << error << '\n';
        DestroyWindow(host);
        return 1;
    }
    mw::RenderRegion region;
    region.pixels = RECT{0, 0, width, height};
    mw::RenderOptions options;
    options.renderScale = 1.0;
    options.timeSeconds = 0.0;
    const bool standardOk = RenderExact(renderer, region, options, width, height,
                                        "standard WARP render/readback", error);

    mw::RenderRegion bloom = region;
    bloom.equation.glowStrength = 1.0;
    bloom.equation.bloomThreshold = 0.12;
    bloom.equation.bloomSoftKnee = 0.5;
    bloom.equation.bloomRadius = 4;
    const bool bloomOk = standardOk && RenderExact(renderer, bloom, options, width, height,
                                                    "bloom WARP render/readback", error);

    mw::RenderRegion perturbation = region;
    perturbation.camera.centreX = -0.743643887037151;
    perturbation.camera.centreY = 0.13182590420533;
    perturbation.camera.scale = 1.0e-5;
    perturbation.maximumIterations = 1000;
    mw::RenderOptions perturbationOptions = options;
    perturbationOptions.precision.mode = mw::PrecisionMode::Perturbation;
    const bool perturbationOk = bloomOk && RenderExact(renderer, perturbation,
                                                        perturbationOptions, width, height,
                                                        "perturbation WARP render/readback", error);
    const bool orbitTransportOk = perturbationOk &&
        VerifyOrbitTextureTransport(renderer, perturbation, error);
    const bool automaticPrecisionOk = orbitTransportOk &&
        VerifyAutomaticPreviewPrecisionSwitches(renderer, perturbation, options, error);
    const std::string graphicsDescription = renderer.GraphicsDescription();
    renderer.Shutdown();
    DestroyWindow(host);
    if (!automaticPrecisionOk) {
        std::cerr << "FAIL: automatic preview precision switching: " << error << '\n';
        return 1;
    }
    std::cout << "PASS: D3D11 " << (useWarp ? "WARP" : "hardware")
              << " standard, bloom and perturbation production readbacks, float4 orbit "
                 "texture transport, and automatic preview precision switching pass at "
              << width << 'x' << height << "; " << graphicsDescription << '\n';
    return 0;
}
