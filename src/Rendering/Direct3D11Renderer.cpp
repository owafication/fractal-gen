#include "Rendering/Direct3D11Renderer.h"

#include "Infrastructure/Logger.h"

#ifdef _WIN32
#include <d3dcompiler.h>
#include <dxgi.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <utility>

namespace mw {

#ifdef _WIN32
namespace {

using Microsoft::WRL::ComPtr;

constexpr const char* kShaderSource = R"hlsl(
cbuffer FractalConstants : register(b0) {
    float4 cCentre;
    float4 cCamera;
    float4 cEquationQuadraticLinear;
    float4 cEquationParameterConstant;
    float4 cIterationReciprocal;
    float4 cInitialJulia;
    float4 cNewtonTargetRelaxation;
    float4 cOrbit;
    float4 cColour;
    float4 cEffects;
    float4 cAnimation;
    float4 cInterior;
    float4 cBackground;
    float4 cIntegers0;
    float4 cFlags0;
    float4 cFlags1;
    float4 cFlags2;
    float4 cFlags3;
};

cbuffer PostConstants : register(b1) { float4 cTexelGlow; };
Texture1D<float4> CustomPalette : register(t0);
Texture1D<float4> ReferenceOrbitReal : register(t1);
Texture1D<float4> ReferenceOrbitImaginary : register(t2);
Texture2D<float4> FrameTexture : register(t3);
SamplerState LinearWrap : register(s0);
SamplerState PointClamp : register(s1);

struct VertexOutput { float4 position : SV_Position; float2 uv : TEXCOORD0; };

VertexOutput VSMain(uint vertexId : SV_VertexID) {
    VertexOutput output;
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    output.uv = uv;
    output.position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return output;
}

float2 cmul(float2 a, float2 b) { return float2(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x); }
float2 cdiv(float2 a, float2 b) { float d=max(dot(b,b),1.0e-30); return float2(a.x*b.x+a.y*b.y,a.y*b.x-a.x*b.y)/d; }
float2 cpowInt(float2 z,int power){float2 r=float2(1.0,0.0);[loop]for(int i=0;i<12;++i){if(i>=power)break;r=cmul(r,z);}return r;}
float2 cexpv(float2 z){float e=exp(clamp(z.x,-40.0,40.0));return e*float2(cos(z.y),sin(z.y));}
float2 clogv(float2 z){return float2(log(max(length(z),1.0e-30)),atan2(z.y,z.x));}
float2 csinv(float2 z){float ep=exp(clamp(z.y,-20.0,20.0));float em=exp(clamp(-z.y,-20.0,20.0));float ch=0.5*(ep+em);float sh=0.5*(ep-em);return float2(sin(z.x)*ch,cos(z.x)*sh);}
float2 ccosv(float2 z){float ep=exp(clamp(z.y,-20.0,20.0));float em=exp(clamp(-z.y,-20.0,20.0));float ch=0.5*(ep+em);float sh=0.5*(ep-em);return float2(cos(z.x)*ch,-sin(z.x)*sh);}
float2 transformZ(float2 z){if(cFlags0.x>0.5)z.x=abs(z.x);if(cFlags0.y>0.5)z.y=abs(z.y);if(cFlags0.w>0.5)z=z.yx;if(cFlags0.z>0.5)z.y=-z.y;int unary=(int)cFlags1.x;if(unary==1)z=csinv(z);else if(unary==2)z=ccosv(z);else if(unary==3)z=cexpv(z);else if(unary==4)z=clogv(z);return z;}
float2 animatedCoeff(float2 base,float phase){if(cAnimation.z<0.5)return base;float w=sin(cCamera.w*cEffects.w*6.2831853+phase)*cAnimation.x;return base+float2(w,w*0.35);}
float2 principalRoot(float2 value,int degree){float mag=pow(max(length(value),1.0e-30),1.0/max((float)degree,1.0));float angle=atan2(value.y,value.x)/max((float)degree,1.0);return mag*float2(cos(angle),sin(angle));}
float2 initialValue(float2 pixel,float2 c){if(cFlags1.z>0.5)return pixel;int mode=(int)cFlags1.y;if(mode==1)return cInitialJulia.xy;if(mode==2)return c;if(mode==3&&(int)cIntegers0.w>0&&length(cIterationReciprocal.zw)>1.0e-8&&length(cEquationQuadraticLinear.xy)>1.0e-8){float2 rhs=cdiv((float)((int)cIntegers0.w)*cIterationReciprocal.zw,(float)((int)cIntegers0.y)*cEquationQuadraticLinear.xy);return principalRoot(rhs,(int)cIntegers0.y+(int)cIntegers0.w);}return float2(0.0,0.0);}
float trapDistance(float2 z){float2 r=z-cOrbit.xy;int trap=(int)cFlags2.z;if(trap==1)return min(abs(r.x),abs(r.y));if(trap==2)return abs(length(r)-cOrbit.z);return length(r);}

float2 twoSum(float a,float b){float s=a+b;float bb=s-a;return float2(s,(a-(s-bb))+(b-bb));}
float2 quickTwoSum(float a,float b){float s=a+b;return float2(s,b-(s-a));}
float2 ddAdd(float2 a,float2 b){float2 s=twoSum(a.x,b.x);float e=a.y+b.y+s.y;return quickTwoSum(s.x,e);}
float2 ddSub(float2 a,float2 b){return ddAdd(a,-b);}
float2 splitFloat(float a){float c=4097.0*a;float hi=c-(c-a);return float2(hi,a-hi);}
float2 ddMul(float2 a,float2 b){float2 as=splitFloat(a.x);float2 bs=splitFloat(b.x);float p=a.x*b.x;float e=((as.x*bs.x-p)+as.x*bs.y+as.y*bs.x)+as.y*bs.y+a.x*b.y+a.y*b.x;return quickTwoSum(p,e);}
float4 cddAdd(float4 a,float4 b){float2 r=ddAdd(a.xz,b.xz);float2 i=ddAdd(a.yw,b.yw);return float4(r.x,i.x,r.y,i.y);}
float4 cddMul(float4 a,float4 b){float2 r=ddSub(ddMul(a.xz,b.xz),ddMul(a.yw,b.yw));float2 i=ddAdd(ddMul(a.xz,b.yw),ddMul(a.yw,b.xz));return float4(r.x,i.x,r.y,i.y);}
float4 cddFromVec2(float2 a){return float4(a.x,a.y,0.0,0.0);}
float4 cddScale(float4 a,float scale){float2 r=ddMul(a.xz,float2(scale,0.0));float2 i=ddMul(a.yw,float2(scale,0.0));return float4(r.x,i.x,r.y,i.y);}
float2 cddValue(float4 a){return float2(a.x+a.z,a.y+a.w);}

float3 hsv2rgb(float3 c){float3 p=abs(frac(c.xxx+float3(0.0,2.0/3.0,1.0/3.0))*6.0-3.0);return c.z*lerp(float3(1.0,1.0,1.0),saturate(p-1.0),c.y);}
float3 palette(float t){t=frac(t);if(cFlags3.x>0.5)return CustomPalette.Sample(LinearWrap,t).rgb;int p=(int)cFlags2.w;if(p==0)return hsv2rgb(float3(t,0.82,1.0));if(p==1)return lerp(float3(0.0,0.015,0.08),float3(0.0,0.65,1.0),pow(t,0.7));if(p==2)return lerp(float3(0.06,0.0,0.0),float3(1.0,0.8,0.05),pow(t,1.5));if(p==3)return lerp(float3(0.02,0.0,0.08),float3(1.0,0.1,0.9),0.5+0.5*sin(t*6.28318));if(p==4)return lerp(float3(0.0,0.0,0.0),float3(0.1,1.0,0.25),pow(t,0.8));if(p==5)return lerp(float3(0.03,0.01,0.0),float3(1.0,0.72,0.12),pow(t,0.65));if(p==6)return lerp(float3(0.01,0.08,0.12),float3(0.75,0.98,1.0),pow(t,0.75));if(p==7)return float3(t,t,t);if(p==8)return 0.62+0.38*cos(6.28318*(float3(0.0,0.33,0.67)+t));return frac(t*8.0)>=0.5?float3(1.0,1.0,1.0):float3(0.01,0.01,0.01);}
float3 adjustColour(float3 colour){float l=dot(colour,float3(0.2126,0.7152,0.0722));colour=lerp(float3(l,l,l),colour,cColour.w);colour=(colour-0.5)*cColour.z+0.5;return saturate(colour*cColour.y);}
float3 finishEscape(int iteration,float magnitudeSquared,float trap,float distanceEstimate){int maxIterations=(int)cIntegers0.x;if(iteration>=maxIterations)return cInterior.rgb;float smoothIteration=(float)iteration;if(cFlags3.y>0.5&&magnitudeSquared>1.0){float lm=0.5*log(max(magnitudeSquared,1.000001));if(lm>0.0){float correction=log(max(lm,0.000001))/log(max((float)((int)cIntegers0.y),2.0));if(correction==correction&&abs(correction)<3.4e38)smoothIteration=(float)iteration+1.0-correction;}}float t=smoothIteration/max((float)maxIterations,1.0)*8.0+cColour.x;float feature=0.0;int method=(int)cFlags2.y;if(method==1){t=-log(max(trap,1.0e-8))*0.32+cColour.x;feature=exp(-trap*18.0);}else if(method==2&&distanceEstimate>0.0){t=-log(max(distanceEstimate,1.0e-10))*0.22+cColour.x;feature=exp(-distanceEstimate*80.0);}float3 colour=lerp(cBackground.rgb,palette(t),0.96);float depth=1.0+cEffects.y*(1.0-smoothIteration/max((float)maxIterations,1.0))*0.45;colour*=depth;colour+=palette(t+0.08)*feature*cEffects.x*0.6;return adjustColour(colour);}
float3 finishNewton(int iteration,float root,float trap){if(root<0.0)return cInterior.rgb;float base=root/max(cFlags2.x,1.0);float shade=1.0-(float)iteration/max(cIntegers0.x,1.0);float3 colour=palette(base+cColour.x+shade*0.08);float glow=exp(-trap*18.0)*cEffects.x;colour=colour*(0.45+0.75*shade)+palette(base+0.12)*glow;return adjustColour(colour);}

float3 newtonFloat(float2 pixel){float2 z=pixel;float root=-1.0;int iteration=0;float trap=1.0e20;int maxIterations=(int)cIntegers0.x;int degree=(int)cFlags2.x;[loop]for(int i=0;i<4096;++i){if(i>=maxIterations)break;trap=min(trap,trapDistance(z));float2 zp=cpowInt(z,degree);float2 residual=zp-cNewtonTargetRelaxation.xy;if(length(residual)<=cEffects.z){float a=atan2(z.y,z.x);if(a<0.0)a+=6.2831853;root=floor(a/6.2831853*(float)degree+0.5);if(root>=(float)degree)root=0.0;iteration=i;break;}float2 derivative=(float)degree*cpowInt(z,degree-1);if(dot(derivative,derivative)<1.0e-24)break;z-=cmul(cNewtonTargetRelaxation.zw,cdiv(residual,derivative));iteration=i+1;}return finishNewton(iteration,root,trap);}

float3 directFloat(float2 p){float2 pixel=cCentre.xy+p*cCamera.x;if(cFlags1.w>0.5)return newtonFloat(pixel);float2 c=cFlags1.z>0.5?cInitialJulia.zw:pixel;float2 z=initialValue(pixel,c);int iteration=0;float magnitudeSquared=dot(z,z);float trap=1.0e20;float2 dz=cFlags1.z>0.5?float2(1.0,0.0):float2(0.0,0.0);bool derivativeSupported=(int)cFlags1.x==0&&cFlags0.x<0.5&&cFlags0.y<0.5&&cFlags0.z<0.5&&cFlags0.w<0.5&&(int)cIntegers0.w==0;float2 aq=animatedCoeff(cEquationQuadraticLinear.xy,0.0),ab=animatedCoeff(cEquationQuadraticLinear.zw,1.7),ac=animatedCoeff(cEquationParameterConstant.xy,3.1),ad=animatedCoeff(cEquationParameterConstant.zw,4.9),ae=cIterationReciprocal.xy,ar=cIterationReciprocal.zw;int maxIterations=(int)cIntegers0.x;[loop]for(int i=0;i<4096;++i){if(i>=maxIterations)break;magnitudeSquared=dot(z,z);if(magnitudeSquared>cOrbit.w)break;trap=min(trap,trapDistance(z));float2 w=transformZ(z);float2 wp=cpowInt(w,(int)cIntegers0.y);float2 next=cmul(aq,wp)+cmul(ab,w)+cmul(ac,cpowInt(c,(int)cIntegers0.z))+ad+ae*(float)i;if((int)cIntegers0.w>0&&length(ar)>1.0e-8){float2 den=cpowInt(w,(int)cIntegers0.w);if(dot(den,den)<1.0e-24){magnitudeSquared=cOrbit.w*2.0;iteration=i+1;break;}next+=cdiv(ar,den);}if(derivativeSupported){float2 local=cmul(aq,(float)((int)cIntegers0.y)*cpowInt(w,(int)cIntegers0.y-1))+ab;dz=cmul(local,dz)+(cFlags1.z>0.5?float2(0.0,0.0):cmul(ac,(float)((int)cIntegers0.z)*cpowInt(c,(int)cIntegers0.z-1)));}z=next;iteration=i+1;}float distanceEstimate=0.0;float mag=sqrt(max(magnitudeSquared,0.0));float derivativeMagnitude=length(dz);if(derivativeSupported&&derivativeMagnitude>1.0e-12&&mag>1.0)distanceEstimate=0.5*log(mag)*mag/derivativeMagnitude;return finishEscape(iteration,magnitudeSquared,trap,distanceEstimate);}

float3 directSplit(float2 p){float2 rx=ddAdd(float2(cCentre.x,cCentre.z),ddMul(float2(cCamera.x,0.0),float2(p.x,0.0)));float2 iy=ddAdd(float2(cCentre.y,cCentre.w),ddMul(float2(cCamera.x,0.0),float2(p.y,0.0)));float4 c=float4(rx.x,iy.x,rx.y,iy.y);float4 z=float4(0.0,0.0,0.0,0.0);int iteration=0;float magnitudeSquared=0.0;float4 aq=cddFromVec2(cEquationQuadraticLinear.xy),ab=cddFromVec2(cEquationQuadraticLinear.zw),ac=cddFromVec2(cEquationParameterConstant.xy),ad=cddFromVec2(cEquationParameterConstant.zw);int maxIterations=(int)cIntegers0.x;[loop]for(int i=0;i<4096;++i){if(i>=maxIterations)break;float2 zv=cddValue(z);magnitudeSquared=dot(zv,zv);if(magnitudeSquared>cOrbit.w)break;z=cddAdd(cddAdd(cddMul(aq,cddMul(z,z)),cddMul(ab,z)),cddAdd(cddMul(ac,c),ad));iteration=i+1;}return finishEscape(iteration,magnitudeSquared,1.0e20,0.0);}
float4 referenceTimesQ(float4 realParts,float4 imaginaryParts,float4 q){float4 result=float4(0.0,0.0,0.0,0.0);result=cddAdd(result,cddMul(cddFromVec2(float2(realParts.x,imaginaryParts.x)),q));result=cddAdd(result,cddMul(cddFromVec2(float2(realParts.y,imaginaryParts.y)),q));result=cddAdd(result,cddMul(cddFromVec2(float2(realParts.z,imaginaryParts.z)),q));result=cddAdd(result,cddMul(cddFromVec2(float2(realParts.w,imaginaryParts.w)),q));return result;}
float3 perturb(float2 p){float4 q=float4(0.0,0.0,0.0,0.0);int iteration=0;float magnitudeSquared=0.0;float4 aq=cddFromVec2(cEquationQuadraticLinear.xy),ab=cddFromVec2(cEquationQuadraticLinear.zw),ac=cddFromVec2(cEquationParameterConstant.xy);float4 local=cddFromVec2(p);int maxIterations=(int)cIntegers0.x;int referenceLength=max((int)cAnimation.y,1);[loop]for(int i=0;i<4096;++i){if(i>=maxIterations||i>=referenceLength)break;float4 zr=ReferenceOrbitReal.Load(int2(i,0));float4 zi=ReferenceOrbitImaginary.Load(int2(i,0));float2 Z=float2(zr.x+zr.y+zr.z+zr.w,zi.x+zi.y+zi.z+zi.w);float2 delta=cddValue(cddScale(q,cCamera.x));float2 approximate=Z+delta;magnitudeSquared=dot(approximate,approximate);if(magnitudeSquared>cOrbit.w)break;float4 twiceZq=cddScale(referenceTimesQ(zr,zi,q),2.0);float4 quadraticDelta=cddAdd(twiceZq,cddScale(cddMul(q,q),cCamera.x));q=cddAdd(cddAdd(cddMul(aq,quadraticDelta),cddMul(ab,q)),cddMul(ac,local));iteration=i+1;}return finishEscape(iteration,magnitudeSquared,1.0e20,0.0);}
float3 sampleFractal(float2 uv){float aspect=cCamera.y/max(cCamera.z,1.0);float2 p=float2((uv.x*2.0-1.0)*aspect,((1.0-uv.y)*2.0-1.0));int mode=(int)cFlags3.w;if(mode==1)return directSplit(p);if(mode==2)return perturb(p);return directFloat(p);}

float4 FractalMain(VertexOutput input) : SV_Target {int samples=clamp((int)cFlags3.z,1,4);float3 colour=float3(0.0,0.0,0.0);float count=0.0;[loop]for(int y=0;y<4;++y){if(y>=samples)break;[loop]for(int x=0;x<4;++x){if(x>=samples)break;float2 offset=(float2((float)x,(float)y)+0.5)/(float)samples-0.5;colour+=sampleFractal(input.uv+offset/cCamera.yz);count+=1.0;}}return float4(colour/max(count,1.0),1.0);}

float4 PostMain(VertexOutput input) : SV_Target {float2 texel=cTexelGlow.xy;float glowStrength=cTexelGlow.z;float3 base=FrameTexture.Sample(PointClamp,input.uv).rgb;float3 blur=float3(0.0,0.0,0.0);blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(-1.0,-1.0)).rgb;blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(0.0,-1.0)).rgb*2.0;blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(1.0,-1.0)).rgb;blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(-1.0,0.0)).rgb*2.0;blur+=base*4.0;blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(1.0,0.0)).rgb*2.0;blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(-1.0,1.0)).rgb;blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(0.0,1.0)).rgb*2.0;blur+=FrameTexture.Sample(PointClamp,input.uv+texel*float2(1.0,1.0)).rgb;blur/=16.0;float bloom=max(max(blur.r,blur.g),blur.b);float3 glow=blur*max(bloom-0.22,0.0)*glowStrength;return float4(saturate(base+glow),1.0);}
)hlsl";

std::string BlobText(ID3DBlob* blob) {
    if (!blob || !blob->GetBufferPointer() || blob->GetBufferSize() == 0U) return {};
    return std::string(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
}

bool CompileShader(const char* entry, const char* target, ComPtr<ID3DBlob>& bytecode,
                   std::string& error) {
    ComPtr<ID3DBlob> diagnostics;
    const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
    const HRESULT result = D3DCompile(kShaderSource, std::strlen(kShaderSource),
                                      "MandelbrotD3D11.hlsl", nullptr, nullptr,
                                      entry, target, flags, 0,
                                      bytecode.ReleaseAndGetAddressOf(),
                                      diagnostics.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "Direct3D 11 shader compilation failed";
        const std::string detail = BlobText(diagnostics.Get());
        if (!detail.empty()) error += ": " + detail;
        return false;
    }
    return true;
}

std::pair<float, float> SplitForFloat(double value) {
    const float high = static_cast<float>(value);
    return {high, static_cast<float>(value - static_cast<double>(high))};
}

std::string FeatureLevelName(D3D_FEATURE_LEVEL level) {
    switch (level) {
    case D3D_FEATURE_LEVEL_11_1: return "11.1";
    case D3D_FEATURE_LEVEL_11_0: return "11.0";
    default: return "unsupported";
    }
}

} // namespace
#endif

Direct3D11Renderer::~Direct3D11Renderer() {
#ifdef _WIN32
    Shutdown();
#endif
}

#ifdef _WIN32
bool Direct3D11Renderer::Initialise(HWND window, std::string& error) {
    Shutdown();
    if (!window || !IsWindow(window)) {
        error = "Direct3D 11 requires a valid render window.";
        return false;
    }
    window_ = window;
    RECT client{};
    GetClientRect(window_, &client);
    Resize(client.right - client.left, client.bottom - client.top);
    if (!CreateDeviceAndSwapChain(error) || !BuildShaders(error) || !CreateFixedState(error) ||
        !EnsureBackBuffer(error)) {
        Shutdown();
        return false;
    }
    capabilities_.nativeFloat64 = false;
    capabilities_.splitFloat = true;
    capabilities_.perturbation = true;
    capabilities_.arbitraryReference = true;
    fpsWindowStart_ = std::chrono::steady_clock::now();
    ready_ = true;
    LogInfo("Direct3D 11 renderer started: " + graphicsDescription_);
    return true;
}

bool Direct3D11Renderer::CreateDeviceAndSwapChain(std::string& error) {
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Width = static_cast<UINT>(std::max(width_, 1));
    description.BufferDesc.Height = static_cast<UINT>(std::max(height_, 1));
    description.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.OutputWindow = window_;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levelsWith11_1[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0
    };
    const D3D_FEATURE_LEVEL levels11_0[] = {D3D_FEATURE_LEVEL_11_0};
    const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levelsWith11_1, static_cast<UINT>(std::size(levelsWith11_1)), D3D11_SDK_VERSION,
        &description, swapChain_.ReleaseAndGetAddressOf(), device_.ReleaseAndGetAddressOf(),
        &featureLevel_, context_.ReleaseAndGetAddressOf());
    if (result == E_INVALIDARG) {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
            levels11_0, static_cast<UINT>(std::size(levels11_0)), D3D11_SDK_VERSION,
            &description, swapChain_.ReleaseAndGetAddressOf(), device_.ReleaseAndGetAddressOf(),
            &featureLevel_, context_.ReleaseAndGetAddressOf());
    }
    if (FAILED(result)) {
        error = "A Direct3D 11 hardware device and swap chain could not be created.";
        return false;
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC adapterDescription{};
    if (SUCCEEDED(device_.As(&dxgiDevice)) &&
        SUCCEEDED(dxgiDevice->GetAdapter(adapter.ReleaseAndGetAddressOf())) &&
        SUCCEEDED(adapter->GetDesc(&adapterDescription))) {
        char adapterName[256]{};
        WideCharToMultiByte(CP_UTF8, 0, adapterDescription.Description, -1,
                            adapterName, static_cast<int>(std::size(adapterName)), nullptr, nullptr);
        graphicsDescription_ = std::string(adapterName) + " / Direct3D " +
                               FeatureLevelName(featureLevel_);
    } else {
        graphicsDescription_ = "Unknown adapter / Direct3D " + FeatureLevelName(featureLevel_);
    }
    return true;
}

bool Direct3D11Renderer::BuildShaders(std::string& error) {
    ComPtr<ID3DBlob> vertexBytecode;
    ComPtr<ID3DBlob> fractalBytecode;
    ComPtr<ID3DBlob> postBytecode;
    if (!CompileShader("VSMain", "vs_5_0", vertexBytecode, error) ||
        !CompileShader("FractalMain", "ps_5_0", fractalBytecode, error) ||
        !CompileShader("PostMain", "ps_5_0", postBytecode, error)) {
        return false;
    }
    HRESULT result = device_->CreateVertexShader(vertexBytecode->GetBufferPointer(),
                                                   vertexBytecode->GetBufferSize(), nullptr,
                                                   fullScreenVertexShader_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 full-screen vertex shader could not be created.";
        return false;
    }
    result = device_->CreatePixelShader(fractalBytecode->GetBufferPointer(),
                                         fractalBytecode->GetBufferSize(), nullptr,
                                         fractalPixelShader_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 fractal pixel shader could not be created.";
        return false;
    }
    result = device_->CreatePixelShader(postBytecode->GetBufferPointer(),
                                         postBytecode->GetBufferSize(), nullptr,
                                         postPixelShader_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 post-process pixel shader could not be created.";
        return false;
    }
    return true;
}

bool Direct3D11Renderer::CreateFixedState(std::string& error) {
    D3D11_BUFFER_DESC bufferDescription{};
    bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
    bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDescription.ByteWidth = static_cast<UINT>(sizeof(FractalConstants));
    HRESULT result = device_->CreateBuffer(&bufferDescription, nullptr,
                                            fractalConstantBuffer_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 fractal constant buffer could not be created.";
        return false;
    }
    bufferDescription.ByteWidth = static_cast<UINT>(sizeof(PostConstants));
    result = device_->CreateBuffer(&bufferDescription, nullptr,
                                    postConstantBuffer_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 post-process constant buffer could not be created.";
        return false;
    }

    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    result = device_->CreateSamplerState(&sampler, linearWrapSampler_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 palette sampler could not be created.";
        return false;
    }
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    result = device_->CreateSamplerState(&sampler, pointClampSampler_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 frame sampler could not be created.";
        return false;
    }

    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.ScissorEnable = TRUE;
    rasterizer.DepthClipEnable = TRUE;
    result = device_->CreateRasterizerState(&rasterizer, rasterizerState_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 rasterizer state could not be created.";
        return false;
    }
    return true;
}

void Direct3D11Renderer::Resize(int width, int height) {
    width_ = std::max(width, 1);
    height_ = std::max(height, 1);
}

void Direct3D11Renderer::DestroyBackBuffer() {
    outputTargetView_.Reset();
    outputTexture_.Reset();
    backBufferView_.Reset();
    backBufferTexture_.Reset();
    backBufferWidth_ = 0;
    backBufferHeight_ = 0;
}

bool Direct3D11Renderer::EnsureBackBuffer(std::string& error) {
    if (!swapChain_ || !device_ || !context_) {
        error = "The Direct3D 11 swap chain is unavailable.";
        return false;
    }
    if (backBufferView_ && backBufferWidth_ == width_ && backBufferHeight_ == height_) return true;
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    DestroyBackBuffer();
    HRESULT result = swapChain_->ResizeBuffers(0, static_cast<UINT>(width_),
                                                static_cast<UINT>(height_),
                                                DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(result)) {
        error = DeviceRemovedError(result);
        return false;
    }
    result = swapChain_->GetBuffer(0, IID_PPV_ARGS(backBufferTexture_.ReleaseAndGetAddressOf()));
    if (FAILED(result) || !backBufferTexture_) {
        error = "The Direct3D 11 back buffer could not be acquired.";
        return false;
    }
    result = device_->CreateRenderTargetView(backBufferTexture_.Get(), nullptr,
                                              backBufferView_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 back-buffer view could not be created.";
        return false;
    }
    D3D11_TEXTURE2D_DESC outputDescription{};
    backBufferTexture_->GetDesc(&outputDescription);
    outputDescription.Usage = D3D11_USAGE_DEFAULT;
    outputDescription.BindFlags = D3D11_BIND_RENDER_TARGET;
    outputDescription.CPUAccessFlags = 0;
    outputDescription.MiscFlags = 0;
    result = device_->CreateTexture2D(&outputDescription, nullptr,
                                      outputTexture_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 final-frame texture could not be created.";
        return false;
    }
    result = device_->CreateRenderTargetView(outputTexture_.Get(), nullptr,
                                              outputTargetView_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 final-frame render-target view could not be created.";
        return false;
    }
    backBufferWidth_ = width_;
    backBufferHeight_ = height_;
    return true;
}

void Direct3D11Renderer::DestroyRenderTarget() {
    renderShaderView_.Reset();
    renderTargetView_.Reset();
    renderTexture_.Reset();
    targetWidth_ = 0;
    targetHeight_ = 0;
}

bool Direct3D11Renderer::EnsureRenderTarget(int width, int height, double scale,
                                             std::string& error) {
    const int requestedWidth = std::max(1, static_cast<int>(std::lround(
        static_cast<double>(width) * std::clamp(scale, 0.25, 1.0))));
    const int requestedHeight = std::max(1, static_cast<int>(std::lround(
        static_cast<double>(height) * std::clamp(scale, 0.25, 1.0))));
    if (renderTexture_ && targetWidth_ == requestedWidth && targetHeight_ == requestedHeight) {
        return true;
    }
    ID3D11ShaderResourceView* nullViews[4]{nullptr, nullptr, nullptr, nullptr};
    context_->PSSetShaderResources(0, 4, nullViews);
    DestroyRenderTarget();

    D3D11_TEXTURE2D_DESC texture{};
    texture.Width = static_cast<UINT>(requestedWidth);
    texture.Height = static_cast<UINT>(requestedHeight);
    texture.MipLevels = 1;
    texture.ArraySize = 1;
    texture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture.SampleDesc.Count = 1;
    texture.Usage = D3D11_USAGE_DEFAULT;
    texture.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    HRESULT result = device_->CreateTexture2D(&texture, nullptr,
                                               renderTexture_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 off-screen render texture could not be created.";
        return false;
    }
    result = device_->CreateRenderTargetView(renderTexture_.Get(), nullptr,
                                              renderTargetView_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 off-screen render-target view could not be created.";
        return false;
    }
    result = device_->CreateShaderResourceView(renderTexture_.Get(), nullptr,
                                                renderShaderView_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 off-screen shader view could not be created.";
        return false;
    }
    targetWidth_ = requestedWidth;
    targetHeight_ = requestedHeight;
    return true;
}

bool Direct3D11Renderer::UpdateBuffer(ID3D11Buffer* buffer, const void* data,
                                       std::size_t size, std::string& error) {
    if (!buffer || !data) {
        error = "A Direct3D 11 constant buffer update was invalid.";
        return false;
    }
    D3D11_MAPPED_SUBRESOURCE mapped{};
    const HRESULT result = context_->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result) || !mapped.pData) {
        error = "A Direct3D 11 constant buffer could not be mapped.";
        return false;
    }
    std::memcpy(mapped.pData, data, size);
    context_->Unmap(buffer, 0);
    return true;
}

bool Direct3D11Renderer::UploadCustomPalette(const std::vector<Colour>& colours,
                                              std::string& error) {
    if (colours.size() < 2U) {
        customPaletteView_.Reset();
        customPaletteTexture_.Reset();
        return true;
    }
    const UINT textureSize = static_cast<UINT>(std::clamp<std::size_t>(colours.size(), 2U, 4096U));
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(textureSize) * 4U);
    for (UINT x = 0; x < textureSize; ++x) {
        const double position = static_cast<double>(x) / textureSize * colours.size();
        const auto first = static_cast<std::size_t>(std::floor(position)) % colours.size();
        const auto second = (first + 1U) % colours.size();
        const float f = static_cast<float>(position - std::floor(position));
        const auto lerp = [f](float a, float b) { return std::clamp(a + (b - a) * f, 0.0F, 1.0F); };
        const Colour colour{lerp(colours[first].r, colours[second].r),
                            lerp(colours[first].g, colours[second].g),
                            lerp(colours[first].b, colours[second].b), 1.0F};
        const std::size_t offset = static_cast<std::size_t>(x) * 4U;
        pixels[offset] = static_cast<std::uint8_t>(std::lround(colour.r * 255.0F));
        pixels[offset + 1U] = static_cast<std::uint8_t>(std::lround(colour.g * 255.0F));
        pixels[offset + 2U] = static_cast<std::uint8_t>(std::lround(colour.b * 255.0F));
        pixels[offset + 3U] = 255U;
    }

    customPaletteView_.Reset();
    customPaletteTexture_.Reset();
    D3D11_TEXTURE1D_DESC texture{};
    texture.Width = textureSize;
    texture.MipLevels = 1;
    texture.ArraySize = 1;
    texture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture.Usage = D3D11_USAGE_IMMUTABLE;
    texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{};
    initial.pSysMem = pixels.data();
    initial.SysMemPitch = textureSize * 4U;
    HRESULT result = device_->CreateTexture1D(&texture, &initial,
                                               customPaletteTexture_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 custom palette texture could not be created.";
        return false;
    }
    result = device_->CreateShaderResourceView(customPaletteTexture_.Get(), nullptr,
                                                customPaletteView_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 custom palette view could not be created.";
        return false;
    }
    return true;
}

PrecisionMode Direct3D11Renderer::ResolvePrecision(const RenderRegion& region,
                                                    const PrecisionSettings& settings,
                                                    std::string& error) const {
    const bool legacyDeepZoomEquation = EquationSupportsPerturbation(region.equation);
    const auto available = [&](PrecisionMode mode) {
        switch (mode) {
        case PrecisionMode::Float32: return true;
        case PrecisionMode::Float64: return capabilities_.nativeFloat64;
        case PrecisionMode::SplitFloat: return capabilities_.splitFloat && legacyDeepZoomEquation;
        case PrecisionMode::Perturbation: return capabilities_.perturbation && legacyDeepZoomEquation;
        case PrecisionMode::ArbitraryPrecisionPerturbation:
            return capabilities_.arbitraryReference && legacyDeepZoomEquation;
        case PrecisionMode::Automatic: return true;
        }
        return false;
    };
    const auto fallback = [&]() {
        const double zoom = CameraZoom(region.camera);
        if (!legacyDeepZoomEquation) return PrecisionMode::Float32;
        if (zoom < 1.0e6) return PrecisionMode::Float32;
        if (settings.allowSplitFloat && available(PrecisionMode::SplitFloat) && zoom < 1.0e13) {
            return PrecisionMode::SplitFloat;
        }
        if (settings.allowArbitraryPrecision &&
            available(PrecisionMode::ArbitraryPrecisionPerturbation)) {
            return PrecisionMode::ArbitraryPrecisionPerturbation;
        }
        if (settings.allowPerturbation && available(PrecisionMode::Perturbation)) {
            return PrecisionMode::Perturbation;
        }
        if (settings.allowSplitFloat && available(PrecisionMode::SplitFloat)) {
            return PrecisionMode::SplitFloat;
        }
        return PrecisionMode::Float32;
    };
    if (settings.mode == PrecisionMode::Automatic) return fallback();
    if (available(settings.mode)) return settings.mode;
    if (settings.automaticFallback) return fallback();
    error = "The selected precision strategy is unavailable in Direct3D 11 or incompatible with the selected equation operations.";
    return settings.mode;
}

bool Direct3D11Renderer::UploadReferenceOrbit(const RenderRegion& region, PrecisionMode mode,
                                               int bits, std::string& error) {
    std::ostringstream key;
    key << std::setprecision(17) << static_cast<int>(mode) << ':'
        << region.camera.centreX << ':' << region.camera.centreXLow << ':'
        << region.camera.centreY << ':' << region.camera.centreYLow << ':'
        << region.maximumIterations << ':' << bits << ':' << EquationSummary(region.equation);
    if (key.str() == referenceOrbitKey_) return true;

    ReferenceOrbit orbit = mode == PrecisionMode::ArbitraryPrecisionPerturbation
        ? BuildReferenceOrbitArbitrary(region.camera, region.equation,
                                       region.maximumIterations, bits)
        : BuildReferenceOrbitDouble(region.camera, region.equation, region.maximumIterations);
    if (orbit.points.empty()) {
        error = "The Direct3D 11 deep-zoom reference orbit could not be generated.";
        return false;
    }
    std::vector<Float4> realData(orbit.points.size());
    std::vector<Float4> imaginaryData(orbit.points.size());
    for (std::size_t i = 0; i < orbit.points.size(); ++i) {
        realData[i] = Float4{orbit.points[i].real[0], orbit.points[i].real[1],
                            orbit.points[i].real[2], orbit.points[i].real[3]};
        imaginaryData[i] = Float4{orbit.points[i].imaginary[0], orbit.points[i].imaginary[1],
                                 orbit.points[i].imaginary[2], orbit.points[i].imaginary[3]};
    }

    referenceOrbitRealView_.Reset();
    referenceOrbitImaginaryView_.Reset();
    referenceOrbitRealTexture_.Reset();
    referenceOrbitImaginaryTexture_.Reset();
    D3D11_TEXTURE1D_DESC texture{};
    texture.Width = static_cast<UINT>(orbit.points.size());
    texture.MipLevels = 1;
    texture.ArraySize = 1;
    texture.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture.Usage = D3D11_USAGE_IMMUTABLE;
    texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA realInitial{};
    realInitial.pSysMem = realData.data();
    realInitial.SysMemPitch = static_cast<UINT>(realData.size() * sizeof(Float4));
    HRESULT result = device_->CreateTexture1D(&texture, &realInitial,
                                               referenceOrbitRealTexture_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 real reference-orbit texture could not be created.";
        return false;
    }
    D3D11_SUBRESOURCE_DATA imaginaryInitial{};
    imaginaryInitial.pSysMem = imaginaryData.data();
    imaginaryInitial.SysMemPitch = static_cast<UINT>(imaginaryData.size() * sizeof(Float4));
    result = device_->CreateTexture1D(&texture, &imaginaryInitial,
                                      referenceOrbitImaginaryTexture_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 imaginary reference-orbit texture could not be created.";
        return false;
    }
    result = device_->CreateShaderResourceView(referenceOrbitRealTexture_.Get(), nullptr,
                                                referenceOrbitRealView_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 real reference-orbit view could not be created.";
        return false;
    }
    result = device_->CreateShaderResourceView(referenceOrbitImaginaryTexture_.Get(), nullptr,
                                                referenceOrbitImaginaryView_.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The Direct3D 11 imaginary reference-orbit view could not be created.";
        return false;
    }
    referenceOrbitLength_ = static_cast<int>(orbit.points.size());
    referenceOrbitKey_ = key.str();
    return true;
}

bool Direct3D11Renderer::DrawFractalRegion(const RenderRegion& region,
                                            const RECT& targetRect, int, int,
                                            int aaLevel,
                                            const PrecisionSettings& precisionSettings,
                                            double timeSeconds, std::string& error) {
    const int regionWidth = std::max(1, static_cast<int>(targetRect.right - targetRect.left));
    const int regionHeight = std::max(1, static_cast<int>(targetRect.bottom - targetRect.top));
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = static_cast<float>(targetRect.left);
    viewport.TopLeftY = static_cast<float>(targetRect.top);
    viewport.Width = static_cast<float>(regionWidth);
    viewport.Height = static_cast<float>(regionHeight);
    viewport.MinDepth = 0.0F;
    viewport.MaxDepth = 1.0F;
    context_->RSSetViewports(1, &viewport);
    D3D11_RECT scissor{targetRect.left, targetRect.top, targetRect.right, targetRect.bottom};
    context_->RSSetScissorRects(1, &scissor);

    const PrecisionMode mode = ResolvePrecision(region, precisionSettings, error);
    if (!error.empty() && !precisionSettings.automaticFallback) return false;
    const bool useCustom = region.customPaletteColours.size() >= 2U;
    if (!UploadCustomPalette(region.customPaletteColours, error)) return false;
    const bool useReference = mode == PrecisionMode::Perturbation ||
                              mode == PrecisionMode::ArbitraryPrecisionPerturbation;
    if (useReference && !UploadReferenceOrbit(region, mode,
                                               precisionSettings.arbitraryPrecisionBits,
                                               error)) {
        return false;
    }

    const auto centreX = SplitForFloat(region.camera.centreX);
    const auto centreY = SplitForFloat(region.camera.centreY);
    const double renderTime = timeSeconds >= 0.0 && std::isfinite(timeSeconds)
        ? timeSeconds
        : std::chrono::duration<double>(
              std::chrono::steady_clock::now().time_since_epoch()).count();
    FractalConstants constants{};
    constants.centre = Float4{centreX.first, centreY.first,
                              centreX.second + static_cast<float>(region.camera.centreXLow),
                              centreY.second + static_cast<float>(region.camera.centreYLow)};
    constants.camera = Float4{static_cast<float>(region.camera.scale),
                              static_cast<float>(regionWidth), static_cast<float>(regionHeight),
                              static_cast<float>(std::fmod(renderTime, 100000.0))};
    constants.equationQuadraticLinear = Float4{
        static_cast<float>(region.equation.quadratic.real),
        static_cast<float>(region.equation.quadratic.imaginary),
        static_cast<float>(region.equation.linear.real),
        static_cast<float>(region.equation.linear.imaginary)};
    constants.equationParameterConstant = Float4{
        static_cast<float>(region.equation.parameter.real),
        static_cast<float>(region.equation.parameter.imaginary),
        static_cast<float>(region.equation.constant.real),
        static_cast<float>(region.equation.constant.imaginary)};
    constants.iterationReciprocal = Float4{
        static_cast<float>(region.equation.iterationTerm.real),
        static_cast<float>(region.equation.iterationTerm.imaginary),
        static_cast<float>(region.equation.reciprocalCoefficient.real),
        static_cast<float>(region.equation.reciprocalCoefficient.imaginary)};
    constants.initialJulia = Float4{
        static_cast<float>(region.equation.initialZ.real),
        static_cast<float>(region.equation.initialZ.imaginary),
        static_cast<float>(region.equation.juliaParameter.real),
        static_cast<float>(region.equation.juliaParameter.imaginary)};
    constants.newtonTargetRelaxation = Float4{
        static_cast<float>(region.equation.newtonTarget.real),
        static_cast<float>(region.equation.newtonTarget.imaginary),
        static_cast<float>(region.equation.newtonRelaxation.real),
        static_cast<float>(region.equation.newtonRelaxation.imaginary)};
    constants.orbit = Float4{
        static_cast<float>(region.equation.orbitTrapPoint.real),
        static_cast<float>(region.equation.orbitTrapPoint.imaginary),
        static_cast<float>(region.equation.orbitTrapRadius),
        static_cast<float>(region.equation.bailoutRadius * region.equation.bailoutRadius)};
    constants.colour = Float4{static_cast<float>(region.colourOffset),
                              static_cast<float>(region.brightness),
                              static_cast<float>(region.contrast),
                              static_cast<float>(region.saturation)};
    constants.effects = Float4{static_cast<float>(region.equation.glowStrength),
                               static_cast<float>(region.equation.depthStrength),
                               static_cast<float>(region.equation.convergenceTolerance),
                               static_cast<float>(region.equation.coefficientAnimationSpeed)};
    constants.animation = Float4{
        static_cast<float>(region.equation.coefficientAnimationAmplitude),
        static_cast<float>(referenceOrbitLength_),
        region.equation.animateCoefficients ? 1.0F : 0.0F, 0.0F};
    constants.interior = Float4{region.interiorColour.r, region.interiorColour.g,
                                region.interiorColour.b, 0.0F};
    constants.background = Float4{region.backgroundColour.r, region.backgroundColour.g,
                                  region.backgroundColour.b, 0.0F};
    constants.integers0 = Float4{static_cast<float>(std::clamp(region.maximumIterations, 32, 4096)),
                                 static_cast<float>(region.equation.power),
                                 static_cast<float>(region.equation.parameterPower),
                                 static_cast<float>(region.equation.reciprocalPower)};
    constants.flags0 = Float4{region.equation.absoluteReal ? 1.0F : 0.0F,
                              region.equation.absoluteImaginary ? 1.0F : 0.0F,
                              region.equation.conjugate ? 1.0F : 0.0F,
                              region.equation.swapRealImaginary ? 1.0F : 0.0F};
    constants.flags1 = Float4{static_cast<float>(region.equation.unaryTransform),
                              static_cast<float>(region.equation.initialZMode),
                              region.equation.juliaMode ? 1.0F : 0.0F,
                              (region.equation.newtonMode ||
                               region.equation.renderMode == FractalRenderMode::Newton) ? 1.0F : 0.0F};
    constants.flags2 = Float4{static_cast<float>(region.equation.newtonDegree),
                              static_cast<float>(region.equation.colouringMethod),
                              static_cast<float>(region.equation.orbitTrap),
                              static_cast<float>(region.palette)};
    const float shaderPrecision = mode == PrecisionMode::SplitFloat ? 1.0F :
        (useReference ? 2.0F : 0.0F);
    constants.flags3 = Float4{useCustom ? 1.0F : 0.0F,
                              region.smoothColouring ? 1.0F : 0.0F,
                              static_cast<float>(std::clamp(aaLevel, 1, 4)), shaderPrecision};
    if (!UpdateBuffer(fractalConstantBuffer_.Get(), &constants, sizeof(constants), error)) {
        return false;
    }

    ID3D11ShaderResourceView* views[3]{customPaletteView_.Get(),
                                       useReference ? referenceOrbitRealView_.Get() : nullptr,
                                       useReference ? referenceOrbitImaginaryView_.Get() : nullptr};
    ID3D11SamplerState* samplers[2]{linearWrapSampler_.Get(), pointClampSampler_.Get()};
    context_->PSSetShaderResources(0, 3, views);
    context_->PSSetSamplers(0, 2, samplers);
    ID3D11Buffer* constant = fractalConstantBuffer_.Get();
    context_->PSSetConstantBuffers(0, 1, &constant);
    context_->PSSetShader(fractalPixelShader_.Get(), nullptr, 0);
    context_->Draw(3, 0);
    ID3D11ShaderResourceView* nullViews[3]{nullptr, nullptr, nullptr};
    context_->PSSetShaderResources(0, 3, nullViews);
    precisionDescription_ = PrecisionModeDisplayName(mode);
    return true;
}

bool Direct3D11Renderer::Render(const std::vector<RenderRegion>& regions,
                                const RenderOptions& options, std::string& error) {
    if (!ready_ || !window_ || !IsWindow(window_)) {
        error = "The Direct3D 11 renderer is not ready.";
        return false;
    }
    RECT client{};
    GetClientRect(window_, &client);
    Resize(client.right - client.left, client.bottom - client.top);
    if (!EnsureBackBuffer(error) || !EnsureRenderTarget(width_, height_, options.renderScale, error)) {
        return false;
    }

    ID3D11ShaderResourceView* nullViews[4]{nullptr, nullptr, nullptr, nullptr};
    context_->PSSetShaderResources(0, 4, nullViews);
    ID3D11RenderTargetView* target = renderTargetView_.Get();
    context_->OMSetRenderTargets(1, &target, nullptr);
    const float clear[4]{0.0F, 0.0F, 0.0F, 1.0F};
    context_->ClearRenderTargetView(renderTargetView_.Get(), clear);
    context_->IASetInputLayout(nullptr);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(fullScreenVertexShader_.Get(), nullptr, 0);
    context_->RSSetState(rasterizerState_.Get());
    postProcessGlowStrength_ = 0.0F;

    for (const auto& region : regions) {
        postProcessGlowStrength_ = std::max(
            postProcessGlowStrength_, static_cast<float>(region.equation.glowStrength));
        RECT scaled{
            static_cast<LONG>(std::lround(static_cast<double>(region.pixels.left) * targetWidth_ / width_)),
            static_cast<LONG>(std::lround(static_cast<double>(region.pixels.top) * targetHeight_ / height_)),
            static_cast<LONG>(std::lround(static_cast<double>(region.pixels.right) * targetWidth_ / width_)),
            static_cast<LONG>(std::lround(static_cast<double>(region.pixels.bottom) * targetHeight_ / height_))};
        scaled.left = std::clamp<LONG>(scaled.left, 0, targetWidth_);
        scaled.right = std::clamp<LONG>(scaled.right, 0, targetWidth_);
        scaled.top = std::clamp<LONG>(scaled.top, 0, targetHeight_);
        scaled.bottom = std::clamp<LONG>(scaled.bottom, 0, targetHeight_);
        if (!DrawFractalRegion(region, scaled, targetWidth_, targetHeight_,
                               options.antiAliasingLevel, options.precision,
                               options.timeSeconds, error)) {
            return false;
        }
    }

    target = outputTargetView_.Get();
    context_->OMSetRenderTargets(1, &target, nullptr);
    context_->ClearRenderTargetView(outputTargetView_.Get(), clear);
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MaxDepth = 1.0F;
    context_->RSSetViewports(1, &viewport);
    D3D11_RECT scissor{0, 0, width_, height_};
    context_->RSSetScissorRects(1, &scissor);
    PostConstants post{};
    post.texelGlow = Float4{1.0F / static_cast<float>(std::max(targetWidth_, 1)),
                            1.0F / static_cast<float>(std::max(targetHeight_, 1)),
                            postProcessGlowStrength_, 0.0F};
    if (!UpdateBuffer(postConstantBuffer_.Get(), &post, sizeof(post), error)) return false;
    ID3D11ShaderResourceView* frameView = renderShaderView_.Get();
    context_->PSSetShaderResources(3, 1, &frameView);
    ID3D11SamplerState* sampler = pointClampSampler_.Get();
    context_->PSSetSamplers(1, 1, &sampler);
    ID3D11Buffer* postBuffer = postConstantBuffer_.Get();
    context_->PSSetConstantBuffers(1, 1, &postBuffer);
    context_->PSSetShader(postPixelShader_.Get(), nullptr, 0);
    context_->Draw(3, 0);
    context_->PSSetShaderResources(3, 1, nullViews);
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    context_->CopyResource(backBufferTexture_.Get(), outputTexture_.Get());

    const HRESULT present = swapChain_->Present(0, 0);
    if (FAILED(present)) {
        error = DeviceRemovedError(present);
        return false;
    }
    ++fpsFrameCount_;
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - fpsWindowStart_).count();
    if (elapsed >= 1.0) {
        framesPerSecond_ = static_cast<double>(fpsFrameCount_) / elapsed;
        fpsFrameCount_ = 0;
        fpsWindowStart_ = now;
    }
    return true;
}

int Direct3D11Renderer::MaximumRenderDimension() const noexcept {
    return ready_ ? static_cast<int>(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) : 0;
}

bool Direct3D11Renderer::CapturePixels(std::vector<std::uint32_t>& pixels, int& width,
                                       int& height, std::string& error) {
    if (!ready_ || !swapChain_ || !device_ || !context_) {
        error = "The Direct3D 11 renderer is not ready to capture a static image.";
        return false;
    }
    if (!outputTexture_) {
        error = "No completed Direct3D 11 frame is available for capture.";
        return false;
    }
    HRESULT result = S_OK;
    D3D11_TEXTURE2D_DESC description{};
    outputTexture_->GetDesc(&description);
    D3D11_TEXTURE2D_DESC stagingDescription = description;
    stagingDescription.Usage = D3D11_USAGE_STAGING;
    stagingDescription.BindFlags = 0;
    stagingDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDescription.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    result = device_->CreateTexture2D(&stagingDescription, nullptr,
                                      staging.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "A Direct3D 11 staging texture could not be created for capture.";
        return false;
    }
    context_->CopyResource(staging.Get(), outputTexture_.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    result = context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(result) || !mapped.pData) {
        error = "The Direct3D 11 captured frame could not be mapped.";
        return false;
    }
    width = static_cast<int>(description.Width);
    height = static_cast<int>(description.Height);
    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    pixels.resize(pixelCount);
    const auto* source = static_cast<const std::uint8_t*>(mapped.pData);
    for (int topY = 0; topY < height; ++topY) {
        const auto* row = source + static_cast<std::size_t>(topY) * mapped.RowPitch;
        const int bottomY = height - 1 - topY;
        auto* destination = pixels.data() + static_cast<std::size_t>(bottomY) * width;
        for (int x = 0; x < width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(x) * 4U;
            const std::uint32_t blue = row[offset];
            const std::uint32_t green = row[offset + 1U];
            const std::uint32_t red = row[offset + 2U];
            destination[x] = 0xFF000000U | (red << 16U) | (green << 8U) | blue;
        }
    }
    context_->Unmap(staging.Get(), 0);
    return true;
}

std::string Direct3D11Renderer::DeviceRemovedError(HRESULT result) const {
    std::ostringstream message;
    message << "Direct3D 11 operation failed (HRESULT 0x" << std::hex
            << static_cast<unsigned long>(result) << ')';
    if (device_) {
        const HRESULT removed = device_->GetDeviceRemovedReason();
        if (FAILED(removed)) {
            message << "; device removed reason 0x"
                    << static_cast<unsigned long>(removed);
        }
    }
    return message.str();
}

void Direct3D11Renderer::Shutdown() {
    ready_ = false;
    if (context_) {
        context_->ClearState();
        context_->Flush();
    }
    referenceOrbitRealView_.Reset();
    referenceOrbitImaginaryView_.Reset();
    referenceOrbitRealTexture_.Reset();
    referenceOrbitImaginaryTexture_.Reset();
    customPaletteView_.Reset();
    customPaletteTexture_.Reset();
    DestroyRenderTarget();
    DestroyBackBuffer();
    rasterizerState_.Reset();
    pointClampSampler_.Reset();
    linearWrapSampler_.Reset();
    postConstantBuffer_.Reset();
    fractalConstantBuffer_.Reset();
    postPixelShader_.Reset();
    fractalPixelShader_.Reset();
    fullScreenVertexShader_.Reset();
    swapChain_.Reset();
    context_.Reset();
    device_.Reset();
    window_ = nullptr;
    width_ = height_ = 1;
    graphicsDescription_.clear();
    precisionDescription_ = "Not rendered yet";
    capabilities_ = {};
    referenceOrbitKey_.clear();
    referenceOrbitLength_ = 0;
    postProcessGlowStrength_ = 0.0F;
    framesPerSecond_ = 0.0;
    fpsFrameCount_ = 0;
}
#endif

} // namespace mw
