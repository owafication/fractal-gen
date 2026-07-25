#pragma once

#include "Core/DeepZoom.h"
#include "Rendering/RendererTypes.h"

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
#endif

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace mw {

class Direct3D11Renderer {
public:
    Direct3D11Renderer() = default;
    ~Direct3D11Renderer();
    Direct3D11Renderer(const Direct3D11Renderer&) = delete;
    Direct3D11Renderer& operator=(const Direct3D11Renderer&) = delete;

#ifdef _WIN32
    bool Initialise(HWND window, std::string& error);
    bool Render(const std::vector<RenderRegion>& regions, const RenderOptions& options,
                std::string& error);
    bool CapturePixels(std::vector<std::uint32_t>& pixels, int& width, int& height,
                       std::string& error);
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
    struct Float4 { float x{}, y{}, z{}, w{}; };
    struct FractalConstants {
        Float4 centre;
        Float4 camera;
        Float4 equationQuadraticLinear;
        Float4 equationParameterConstant;
        Float4 iterationReciprocal;
        Float4 initialJulia;
        Float4 newtonTargetRelaxation;
        Float4 orbit;
        Float4 colour;
        Float4 effects;
        Float4 animation;
        Float4 interior;
        Float4 background;
        Float4 integers0;
        Float4 flags0;
        Float4 flags1;
        Float4 flags2;
        Float4 flags3;
    };
    struct PostConstants { Float4 texelGlow; };

    bool CreateDeviceAndSwapChain(std::string& error);
    bool BuildShaders(std::string& error);
    bool CreateFixedState(std::string& error);
    bool EnsureBackBuffer(std::string& error);
    bool EnsureRenderTarget(int width, int height, double scale, std::string& error);
    void DestroyBackBuffer();
    void DestroyRenderTarget();
    bool UploadCustomPalette(const std::vector<Colour>& colours, std::string& error);
    bool UploadReferenceOrbit(const RenderRegion& region, PrecisionMode mode, int bits,
                              std::string& error);
    PrecisionMode ResolvePrecision(const RenderRegion& region,
                                   const PrecisionSettings& settings,
                                   std::string& error) const;
    bool DrawFractalRegion(const RenderRegion& region, const RECT& targetRect,
                           int targetWidth, int targetHeight, int aaLevel,
                           const PrecisionSettings& precisionSettings,
                           double timeSeconds, std::string& error);
    bool UpdateBuffer(ID3D11Buffer* buffer, const void* data, std::size_t size,
                      std::string& error);
    std::string DeviceRemovedError(HRESULT result) const;

    HWND window_{nullptr};
    int width_{1};
    int height_{1};
    int backBufferWidth_{0};
    int backBufferHeight_{0};
    int targetWidth_{0};
    int targetHeight_{0};
    D3D_FEATURE_LEVEL featureLevel_{D3D_FEATURE_LEVEL_11_0};

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBufferTexture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferView_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> outputTexture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> outputTargetView_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTexture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> renderShaderView_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> fullScreenVertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> fractalPixelShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> postPixelShader_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> fractalConstantBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> postConstantBuffer_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> linearWrapSampler_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> pointClampSampler_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
    Microsoft::WRL::ComPtr<ID3D11Texture1D> customPaletteTexture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> customPaletteView_;
    Microsoft::WRL::ComPtr<ID3D11Texture1D> referenceOrbitRealTexture_;
    Microsoft::WRL::ComPtr<ID3D11Texture1D> referenceOrbitImaginaryTexture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> referenceOrbitRealView_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> referenceOrbitImaginaryView_;
    std::string referenceOrbitKey_;
    int referenceOrbitLength_{0};
    float postProcessGlowStrength_{0.0F};
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
