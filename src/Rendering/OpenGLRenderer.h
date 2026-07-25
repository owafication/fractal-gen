#pragma once

#include "Core/DeepZoom.h"
#include "Rendering/RendererTypes.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <chrono>
#include <string>
#include <vector>

namespace mw {

class OpenGLRenderer {
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer();
    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

#ifdef _WIN32
    bool Initialise(HWND window, std::string& error);
    bool Render(const std::vector<RenderRegion>& regions, const RenderOptions& options, std::string& error);
    bool CapturePixels(std::vector<std::uint32_t>& pixels, int& width, int& height, std::string& error);
    [[nodiscard]] int MaximumRenderDimension() const noexcept;
    void Resize(int width, int height);
    void Shutdown();
#endif

    [[nodiscard]] bool IsReady() const noexcept { return ready_; }
    [[nodiscard]] double FramesPerSecond() const noexcept { return framesPerSecond_; }
    [[nodiscard]] std::string GraphicsDescription() const { return graphicsDescription_; }
    [[nodiscard]] std::string PrecisionDescription() const { return precisionDescription_; }
    [[nodiscard]] PrecisionCapabilities Capabilities() const noexcept { return capabilities_; }

private:
#ifdef _WIN32
    bool LoadFunctions(std::string& error);
    bool BuildPrograms(std::string& error);
    bool EnsureRenderTarget(int width, int height, double scale, std::string& error);
    void DestroyRenderTarget();
    unsigned CompileShader(unsigned type, const char* source, std::string& error);
    unsigned LinkProgram(const char* vertexSource, const char* fragmentSource, std::string& error);
    PrecisionMode ResolvePrecision(const RenderRegion& region, const PrecisionSettings& settings, std::string& error) const;
    void DrawFractalRegion(const RenderRegion& region, const RECT& targetRect, int targetWidth, int targetHeight,
                           int aaLevel, const PrecisionSettings& precisionSettings, double timeSeconds,
                           std::string& error);
    void UploadCustomPalette(const std::vector<Colour>& colours);
    bool UploadReferenceOrbit(const RenderRegion& region, PrecisionMode mode, int bits, std::string& error);
    void DrawTextureToBackbuffer();

    HWND window_{nullptr};
    HDC deviceContext_{nullptr};
    HGLRC renderContext_{nullptr};
    int width_{1};
    int height_{1};
    int targetWidth_{0};
    int targetHeight_{0};
    unsigned fractalProgram_{0};
    unsigned doubleProgram_{0};
    unsigned postProcessProgram_{0};
    unsigned framebuffer_{0};
    unsigned renderTexture_{0};
    unsigned customPaletteTexture_{0};
    unsigned referenceOrbitRealTexture_{0};
    unsigned referenceOrbitImaginaryTexture_{0};
    int maximumPaletteTextureWidth_{1024};
    std::string referenceOrbitKey_;
    int referenceOrbitLength_{0};
    float postProcessGlowStrength_{0.0F};

    using GLCreateShader = unsigned(APIENTRY*)(unsigned);
    using GLShaderSource = void(APIENTRY*)(unsigned, int, const char* const*, const int*);
    using GLCompileShader = void(APIENTRY*)(unsigned);
    using GLGetShaderiv = void(APIENTRY*)(unsigned, unsigned, int*);
    using GLGetShaderInfoLog = void(APIENTRY*)(unsigned, int, int*, char*);
    using GLDeleteShader = void(APIENTRY*)(unsigned);
    using GLCreateProgram = unsigned(APIENTRY*)();
    using GLAttachShader = void(APIENTRY*)(unsigned, unsigned);
    using GLLinkProgram = void(APIENTRY*)(unsigned);
    using GLGetProgramiv = void(APIENTRY*)(unsigned, unsigned, int*);
    using GLGetProgramInfoLog = void(APIENTRY*)(unsigned, int, int*, char*);
    using GLDeleteProgram = void(APIENTRY*)(unsigned);
    using GLUseProgram = void(APIENTRY*)(unsigned);
    using GLGetUniformLocation = int(APIENTRY*)(unsigned, const char*);
    using GLUniform1i = void(APIENTRY*)(int, int);
    using GLUniform1f = void(APIENTRY*)(int, float);
    using GLUniform2f = void(APIENTRY*)(int, float, float);
    using GLUniform3f = void(APIENTRY*)(int, float, float, float);
    using GLUniform1d = void(APIENTRY*)(int, double);
    using GLUniform2d = void(APIENTRY*)(int, double, double);
    using GLActiveTexture = void(APIENTRY*)(unsigned);
    using GLGenFramebuffers = void(APIENTRY*)(int, unsigned*);
    using GLBindFramebuffer = void(APIENTRY*)(unsigned, unsigned);
    using GLFramebufferTexture2D = void(APIENTRY*)(unsigned, unsigned, unsigned, unsigned, int);
    using GLCheckFramebufferStatus = unsigned(APIENTRY*)(unsigned);
    using GLDeleteFramebuffers = void(APIENTRY*)(int, const unsigned*);
    using WglSwapInterval = BOOL(WINAPI*)(int);

    GLCreateShader glCreateShader_{nullptr}; GLShaderSource glShaderSource_{nullptr}; GLCompileShader glCompileShader_{nullptr};
    GLGetShaderiv glGetShaderiv_{nullptr}; GLGetShaderInfoLog glGetShaderInfoLog_{nullptr}; GLDeleteShader glDeleteShader_{nullptr};
    GLCreateProgram glCreateProgram_{nullptr}; GLAttachShader glAttachShader_{nullptr}; GLLinkProgram glLinkProgram_{nullptr};
    GLGetProgramiv glGetProgramiv_{nullptr}; GLGetProgramInfoLog glGetProgramInfoLog_{nullptr}; GLDeleteProgram glDeleteProgram_{nullptr};
    GLUseProgram glUseProgram_{nullptr}; GLGetUniformLocation glGetUniformLocation_{nullptr}; GLUniform1i glUniform1i_{nullptr};
    GLUniform1f glUniform1f_{nullptr}; GLUniform2f glUniform2f_{nullptr}; GLUniform3f glUniform3f_{nullptr};
    GLUniform1d glUniform1d_{nullptr}; GLUniform2d glUniform2d_{nullptr}; GLActiveTexture glActiveTexture_{nullptr};
    GLGenFramebuffers glGenFramebuffers_{nullptr}; GLBindFramebuffer glBindFramebuffer_{nullptr};
    GLFramebufferTexture2D glFramebufferTexture2D_{nullptr}; GLCheckFramebufferStatus glCheckFramebufferStatus_{nullptr};
    GLDeleteFramebuffers glDeleteFramebuffers_{nullptr}; WglSwapInterval wglSwapInterval_{nullptr};
#endif

    bool ready_{false};
    double framesPerSecond_{0.0};
    std::string graphicsDescription_;
    std::string precisionDescription_{"Not rendered yet"};
    PrecisionCapabilities capabilities_;
    std::chrono::steady_clock::time_point fpsWindowStart_{};
    int fpsFrameCount_{0};
};

} // namespace mw
