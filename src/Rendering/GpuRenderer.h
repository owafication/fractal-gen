#pragma once

#include "Rendering/RendererTypes.h"

#include <string>
#include <vector>

namespace mw {

class Direct3D11Renderer;
class OpenGLRenderer;

class GpuRenderer {
public:
    GpuRenderer();
    ~GpuRenderer();
    GpuRenderer(const GpuRenderer&) = delete;
    GpuRenderer& operator=(const GpuRenderer&) = delete;

#ifdef _WIN32
    bool Initialise(HWND window, std::string& error,
                    GpuBackendPreference preference = GpuBackendPreference::Automatic);
    bool Render(const std::vector<RenderRegion>& regions, const RenderOptions& options,
                std::string& error);
    bool CapturePixels(std::vector<std::uint32_t>& pixels, int& width, int& height,
                       std::string& error);
    [[nodiscard]] int MaximumRenderDimension() const noexcept;
    void Resize(int width, int height);
    void Shutdown();
#endif

    [[nodiscard]] bool IsReady() const noexcept;
    [[nodiscard]] double FramesPerSecond() const noexcept;
    [[nodiscard]] std::string GraphicsDescription() const;
    [[nodiscard]] std::string PrecisionDescription() const;
    [[nodiscard]] PrecisionCapabilities Capabilities() const noexcept;
    [[nodiscard]] ActiveGpuBackend ActiveBackend() const noexcept { return activeBackend_; }
    [[nodiscard]] std::string BackendName() const;

private:
#ifdef _WIN32
    HWND window_{nullptr};
    GpuBackendPreference preference_{GpuBackendPreference::Automatic};
#endif
    Direct3D11Renderer* direct3D11_{nullptr};
    OpenGLRenderer* openGL_{nullptr};
    ActiveGpuBackend activeBackend_{ActiveGpuBackend::None};
};

} // namespace mw
