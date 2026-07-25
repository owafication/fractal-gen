#include "Rendering/GpuRenderer.h"

#include "Infrastructure/Logger.h"
#include "Rendering/Direct3D11Renderer.h"
#include "Rendering/OpenGLRenderer.h"

#include <new>
#include <sstream>

namespace mw {

GpuRenderer::GpuRenderer()
    : direct3D11_(new Direct3D11Renderer()), openGL_(new OpenGLRenderer()) {}

GpuRenderer::~GpuRenderer() {
#ifdef _WIN32
    Shutdown();
#endif
    delete direct3D11_;
    delete openGL_;
}

#ifdef _WIN32
bool GpuRenderer::Initialise(HWND window, std::string& error, GpuBackendPreference preference) {
    Shutdown();
    window_ = window;
    preference_ = preference;
    error.clear();

    std::string d3dError;
    if (preference != GpuBackendPreference::OpenGL) {
        if (direct3D11_->Initialise(window, d3dError)) {
            activeBackend_ = ActiveGpuBackend::Direct3D11;
            return true;
        }
        if (preference == GpuBackendPreference::Direct3D11) {
            error = d3dError.empty() ? "Direct3D 11 could not initialise." : d3dError;
            return false;
        }
        LogWarning("Direct3D 11 renderer unavailable; trying OpenGL: " + d3dError);
    }

    std::string glError;
    if (openGL_->Initialise(window, glError)) {
        activeBackend_ = ActiveGpuBackend::OpenGL;
        return true;
    }

    std::ostringstream combined;
    if (!d3dError.empty()) combined << "Direct3D 11: " << d3dError;
    if (!glError.empty()) {
        if (combined.tellp() > 0) combined << " | ";
        combined << "OpenGL: " << glError;
    }
    error = combined.str().empty() ? "No supported GPU renderer could initialise." : combined.str();
    return false;
}

bool GpuRenderer::Render(const std::vector<RenderRegion>& regions, const RenderOptions& options,
                         std::string& error) {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: {
        if (direct3D11_->Render(regions, options, error)) return true;
        if (preference_ != GpuBackendPreference::Automatic || !window_) return false;
        const std::string direct3DError = error;
        LogWarning("Direct3D 11 rendering failed; switching this surface to OpenGL: " + direct3DError);
        direct3D11_->Shutdown();
        std::string openGlError;
        if (!openGL_->Initialise(window_, openGlError)) {
            error = direct3DError + " | OpenGL fallback: " + openGlError;
            activeBackend_ = ActiveGpuBackend::None;
            return false;
        }
        activeBackend_ = ActiveGpuBackend::OpenGL;
        error.clear();
        return openGL_->Render(regions, options, error);
    }
    case ActiveGpuBackend::OpenGL: return openGL_->Render(regions, options, error);
    case ActiveGpuBackend::None: break;
    }
    error = "The GPU renderer is not ready.";
    return false;
}

bool GpuRenderer::CapturePixels(std::vector<std::uint32_t>& pixels, int& width, int& height,
                                std::string& error) {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return direct3D11_->CapturePixels(pixels, width, height, error);
    case ActiveGpuBackend::OpenGL: return openGL_->CapturePixels(pixels, width, height, error);
    case ActiveGpuBackend::None: break;
    }
    error = "The GPU renderer is not ready to capture an image.";
    return false;
}

int GpuRenderer::MaximumRenderDimension() const noexcept {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return direct3D11_->MaximumRenderDimension();
    case ActiveGpuBackend::OpenGL: return openGL_->MaximumRenderDimension();
    case ActiveGpuBackend::None: return 0;
    }
    return 0;
}

void GpuRenderer::Resize(int width, int height) {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: direct3D11_->Resize(width, height); break;
    case ActiveGpuBackend::OpenGL: openGL_->Resize(width, height); break;
    case ActiveGpuBackend::None: break;
    }
}

void GpuRenderer::Shutdown() {
    if (direct3D11_) direct3D11_->Shutdown();
    if (openGL_) openGL_->Shutdown();
    activeBackend_ = ActiveGpuBackend::None;
    window_ = nullptr;
    preference_ = GpuBackendPreference::Automatic;
}
#endif

bool GpuRenderer::IsReady() const noexcept {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return direct3D11_ && direct3D11_->IsReady();
    case ActiveGpuBackend::OpenGL: return openGL_ && openGL_->IsReady();
    case ActiveGpuBackend::None: return false;
    }
    return false;
}

double GpuRenderer::FramesPerSecond() const noexcept {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return direct3D11_->FramesPerSecond();
    case ActiveGpuBackend::OpenGL: return openGL_->FramesPerSecond();
    case ActiveGpuBackend::None: return 0.0;
    }
    return 0.0;
}

std::string GpuRenderer::GraphicsDescription() const {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return direct3D11_->GraphicsDescription();
    case ActiveGpuBackend::OpenGL: return openGL_->GraphicsDescription();
    case ActiveGpuBackend::None: return {};
    }
    return {};
}

std::string GpuRenderer::PrecisionDescription() const {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return direct3D11_->PrecisionDescription();
    case ActiveGpuBackend::OpenGL: return openGL_->PrecisionDescription();
    case ActiveGpuBackend::None: return "Not rendered yet";
    }
    return "Not rendered yet";
}

PrecisionCapabilities GpuRenderer::Capabilities() const noexcept {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return direct3D11_->Capabilities();
    case ActiveGpuBackend::OpenGL: return openGL_->Capabilities();
    case ActiveGpuBackend::None: return {};
    }
    return {};
}

std::string GpuRenderer::BackendName() const {
    switch (activeBackend_) {
    case ActiveGpuBackend::Direct3D11: return "Direct3D 11";
    case ActiveGpuBackend::OpenGL: return "OpenGL";
    case ActiveGpuBackend::None: return "None";
    }
    return "None";
}

} // namespace mw
