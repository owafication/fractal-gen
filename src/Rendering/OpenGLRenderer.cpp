#include "Rendering/OpenGLRenderer.h"

#include "Infrastructure/Logger.h"

#ifdef _WIN32
#include <GL/gl.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

namespace mw {

#ifdef _WIN32
namespace {

constexpr unsigned GL_VERTEX_SHADER_VALUE = 0x8B31;
constexpr unsigned GL_FRAGMENT_SHADER_VALUE = 0x8B30;
constexpr unsigned GL_COMPILE_STATUS_VALUE = 0x8B81;
constexpr unsigned GL_LINK_STATUS_VALUE = 0x8B82;
constexpr unsigned GL_INFO_LOG_LENGTH_VALUE = 0x8B84;
constexpr unsigned GL_FRAMEBUFFER_VALUE = 0x8D40;
constexpr unsigned GL_COLOR_ATTACHMENT0_VALUE = 0x8CE0;
constexpr unsigned GL_FRAMEBUFFER_COMPLETE_VALUE = 0x8CD5;
constexpr unsigned GL_RGBA8_VALUE = 0x8058;
constexpr unsigned GL_RGBA32F_VALUE = 0x8814;
constexpr unsigned GL_CLAMP_TO_EDGE_VALUE = 0x812F;
constexpr unsigned GL_TEXTURE0_VALUE = 0x84C0;
constexpr unsigned GL_TEXTURE1_VALUE = 0x84C1;
constexpr unsigned GL_TEXTURE2_VALUE = 0x84C2;

const char* kVertexShader = R"glsl(
#version 120
varying vec2 vUv;
void main() { gl_Position = gl_Vertex; vUv = gl_MultiTexCoord0.xy; }
)glsl";

const char* kCommonFragmentShader = R"glsl(
#version 120
varying vec2 vUv;
uniform vec2 uCentre;
uniform vec2 uCentreLow;
uniform float uScale;
uniform int uMaxIterations;
uniform vec2 uResolution;
uniform vec2 uEquationQuadratic;
uniform vec2 uEquationLinear;
uniform vec2 uEquationParameter;
uniform vec2 uEquationConstant;
uniform vec2 uIterationTerm;
uniform vec2 uReciprocalCoefficient;
uniform int uPower;
uniform int uParameterPower;
uniform int uReciprocalPower;
uniform int uAbsoluteReal;
uniform int uAbsoluteImaginary;
uniform int uConjugate;
uniform int uSwapRealImaginary;
uniform int uUnaryTransform;
uniform int uInitialZMode;
uniform vec2 uInitialZ;
uniform int uJuliaMode;
uniform vec2 uJuliaParameter;
uniform float uBailoutSquared;
uniform int uNewtonMode;
uniform int uNewtonDegree;
uniform vec2 uNewtonTarget;
uniform vec2 uNewtonRelaxation;
uniform float uConvergenceTolerance;
uniform int uColouringMethod;
uniform int uOrbitTrapType;
uniform vec2 uOrbitTrapPoint;
uniform float uOrbitTrapRadius;
uniform float uGlowStrength;
uniform float uDepthStrength;
uniform int uAnimateCoefficients;
uniform float uCoefficientAnimationSpeed;
uniform float uCoefficientAnimationAmplitude;
uniform float uTime;
uniform int uPrecisionMode;
uniform sampler1D uReferenceOrbitReal;
uniform sampler1D uReferenceOrbitImaginary;
uniform float uReferenceLength;
uniform int uPalette;
uniform sampler1D uCustomPalette;
uniform int uUseCustomPalette;
uniform float uColourOffset;
uniform float uBrightness;
uniform float uContrast;
uniform float uSaturation;
uniform vec3 uInterior;
uniform vec3 uBackground;
uniform int uSmooth;
uniform int uAA;

vec2 cmul(vec2 a, vec2 b) { return vec2(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x); }
vec2 cdiv(vec2 a, vec2 b) { float d=max(dot(b,b),1.0e-30); return vec2(a.x*b.x+a.y*b.y,a.y*b.x-a.x*b.y)/d; }
vec2 cpowInt(vec2 z,int power){vec2 r=vec2(1.0,0.0);for(int i=0;i<12;++i){if(i>=power)break;r=cmul(r,z);}return r;}
vec2 cexpv(vec2 z){float e=exp(clamp(z.x,-40.0,40.0));return e*vec2(cos(z.y),sin(z.y));}
vec2 clogv(vec2 z){return vec2(log(max(length(z),1.0e-30)),atan(z.y,z.x));}
vec2 csinv(vec2 z){float ep=exp(clamp(z.y,-20.0,20.0));float em=exp(clamp(-z.y,-20.0,20.0));float ch=0.5*(ep+em);float sh=0.5*(ep-em);return vec2(sin(z.x)*ch,cos(z.x)*sh);}
vec2 ccosv(vec2 z){float ep=exp(clamp(z.y,-20.0,20.0));float em=exp(clamp(-z.y,-20.0,20.0));float ch=0.5*(ep+em);float sh=0.5*(ep-em);return vec2(cos(z.x)*ch,-sin(z.x)*sh);}
vec2 transformZ(vec2 z){if(uAbsoluteReal!=0)z.x=abs(z.x);if(uAbsoluteImaginary!=0)z.y=abs(z.y);if(uSwapRealImaginary!=0)z=z.yx;if(uConjugate!=0)z.y=-z.y;if(uUnaryTransform==1)z=csinv(z);else if(uUnaryTransform==2)z=ccosv(z);else if(uUnaryTransform==3)z=cexpv(z);else if(uUnaryTransform==4)z=clogv(z);return z;}
vec2 animatedCoeff(vec2 base,float phase){if(uAnimateCoefficients==0)return base;float w=sin(uTime*uCoefficientAnimationSpeed*6.2831853+phase)*uCoefficientAnimationAmplitude;return base+vec2(w,w*0.35);}
vec2 principalRoot(vec2 value,int degree){float mag=pow(max(length(value),1.0e-30),1.0/max(float(degree),1.0));float angle=atan(value.y,value.x)/max(float(degree),1.0);return mag*vec2(cos(angle),sin(angle));}
vec2 initialValue(vec2 pixel,vec2 c){if(uJuliaMode!=0)return pixel;if(uInitialZMode==1)return uInitialZ;if(uInitialZMode==2)return c;if(uInitialZMode==3&&uReciprocalPower>0&&length(uReciprocalCoefficient)>1.0e-8&&length(uEquationQuadratic)>1.0e-8){vec2 rhs=cdiv(float(uReciprocalPower)*uReciprocalCoefficient,float(uPower)*uEquationQuadratic);return principalRoot(rhs,uPower+uReciprocalPower);}return vec2(0.0);}
float trapDistance(vec2 z){vec2 r=z-uOrbitTrapPoint;if(uOrbitTrapType==1)return min(abs(r.x),abs(r.y));if(uOrbitTrapType==2)return abs(length(r)-uOrbitTrapRadius);return length(r);}

vec2 twoSum(float a, float b) { float s=a+b; float bb=s-a; return vec2(s,(a-(s-bb))+(b-bb)); }
vec2 quickTwoSum(float a, float b) { float s=a+b; return vec2(s,b-(s-a)); }
vec2 ddAdd(vec2 a, vec2 b) { vec2 s=twoSum(a.x,b.x); float e=a.y+b.y+s.y; return quickTwoSum(s.x,e); }
vec2 ddNeg(vec2 a) { return -a; }
vec2 ddSub(vec2 a, vec2 b) { return ddAdd(a,ddNeg(b)); }
vec2 splitFloat(float a) { float c=4097.0*a; float hi=c-(c-a); return vec2(hi,a-hi); }
vec2 ddMul(vec2 a, vec2 b) { vec2 as=splitFloat(a.x); vec2 bs=splitFloat(b.x); float p=a.x*b.x; float e=((as.x*bs.x-p)+as.x*bs.y+as.y*bs.x)+as.y*bs.y+a.x*b.y+a.y*b.x; return quickTwoSum(p,e); }
vec4 cddAdd(vec4 a, vec4 b) { vec2 r=ddAdd(a.xz,b.xz); vec2 i=ddAdd(a.yw,b.yw); return vec4(r.x,i.x,r.y,i.y); }
vec4 cddMul(vec4 a, vec4 b) { vec2 r=ddSub(ddMul(a.xz,b.xz),ddMul(a.yw,b.yw)); vec2 i=ddAdd(ddMul(a.xz,b.yw),ddMul(a.yw,b.xz)); return vec4(r.x,i.x,r.y,i.y); }
vec4 cddFromVec2(vec2 a) { return vec4(a.x,a.y,0.0,0.0); }
vec4 cddScale(vec4 a, float scale) { vec2 r=ddMul(a.xz,vec2(scale,0.0)); vec2 i=ddMul(a.yw,vec2(scale,0.0)); return vec4(r.x,i.x,r.y,i.y); }
vec2 cddValue(vec4 a) { return vec2(a.x+a.z,a.y+a.w); }

vec3 hsv2rgb(vec3 c) { vec3 p=abs(fract(c.xxx+vec3(0.0,2.0/3.0,1.0/3.0))*6.0-3.0); return c.z*mix(vec3(1.0),clamp(p-1.0,0.0,1.0),c.y); }
vec3 palette(float t) {
    t=fract(t); if(uUseCustomPalette!=0) return texture1D(uCustomPalette,t).rgb;
    if(uPalette==0) return hsv2rgb(vec3(t,0.82,1.0));
    if(uPalette==1) return mix(vec3(0.0,0.015,0.08),vec3(0.0,0.65,1.0),pow(t,0.7));
    if(uPalette==2) return mix(vec3(0.06,0.0,0.0),vec3(1.0,0.8,0.05),pow(t,1.5));
    if(uPalette==3) return mix(vec3(0.02,0.0,0.08),vec3(1.0,0.1,0.9),0.5+0.5*sin(t*6.28318));
    if(uPalette==4) return mix(vec3(0.0),vec3(0.1,1.0,0.25),pow(t,0.8));
    if(uPalette==5) return mix(vec3(0.03,0.01,0.0),vec3(1.0,0.72,0.12),pow(t,0.65));
    if(uPalette==6) return mix(vec3(0.01,0.08,0.12),vec3(0.75,0.98,1.0),pow(t,0.75));
    if(uPalette==7) return vec3(t);
    if(uPalette==8) return 0.62+0.38*cos(6.28318*(vec3(0.0,0.33,0.67)+t));
    return step(0.5,fract(t*8.0))*vec3(1.0)+step(fract(t*8.0),0.5)*vec3(0.01);
}
vec3 adjustColour(vec3 colour) { float l=dot(colour,vec3(0.2126,0.7152,0.0722)); colour=mix(vec3(l),colour,uSaturation); colour=(colour-0.5)*uContrast+0.5; return clamp(colour*uBrightness,0.0,1.0); }
vec3 finishEscape(int iteration,float magnitudeSquared,float trap,float distanceEstimate) {
    if(iteration>=uMaxIterations) return uInterior;
    float smoothIteration=float(iteration);
    if(uSmooth!=0 && magnitudeSquared>1.0) { float lm=0.5*log(max(magnitudeSquared,1.000001)); if(lm>0.0) { float correction=log(max(lm,0.000001))/log(max(float(uPower),2.0)); if(correction==correction) smoothIteration=float(iteration)+1.0-correction; } }
    float t=smoothIteration/max(float(uMaxIterations),1.0)*8.0+uColourOffset;
    float feature=0.0;
    if(uColouringMethod==1){t=-log(max(trap,1.0e-8))*0.32+uColourOffset;feature=exp(-trap*18.0);}
    else if(uColouringMethod==2&&distanceEstimate>0.0){t=-log(max(distanceEstimate,1.0e-10))*0.22+uColourOffset;feature=exp(-distanceEstimate*80.0);}
    vec3 colour=mix(uBackground,palette(t),0.96);
    float depth=1.0+uDepthStrength*(1.0-smoothIteration/max(float(uMaxIterations),1.0))*0.45;
    colour*=depth;
    colour+=palette(t+0.08)*feature*uGlowStrength*0.6;
    return adjustColour(colour);
}
vec3 finishNewton(int iteration,float root,float trap){if(root<0.0)return uInterior;float base=root/max(float(uNewtonDegree),1.0);float shade=1.0-float(iteration)/max(float(uMaxIterations),1.0);vec3 colour=palette(base+uColourOffset+shade*0.08);float glow=exp(-trap*18.0)*uGlowStrength;colour=colour*(0.45+0.75*shade)+palette(base+0.12)*glow;return adjustColour(colour);}

vec3 newtonFloat(vec2 pixel){vec2 z=pixel;float root=-1.0;int iteration=0;float trap=1.0e20;for(int i=0;i<4096;++i){if(i>=uMaxIterations)break;trap=min(trap,trapDistance(z));vec2 zp=cpowInt(z,uNewtonDegree);vec2 residual=zp-uNewtonTarget;if(length(residual)<=uConvergenceTolerance){float a=atan(z.y,z.x);if(a<0.0)a+=6.2831853;root=floor(a/6.2831853*float(uNewtonDegree)+0.5);if(root>=float(uNewtonDegree))root=0.0;iteration=i;break;}vec2 derivative=float(uNewtonDegree)*cpowInt(z,uNewtonDegree-1);if(dot(derivative,derivative)<1.0e-24)break;z-=cmul(uNewtonRelaxation,cdiv(residual,derivative));iteration=i+1;}return finishNewton(iteration,root,trap);}

vec3 directFloat(vec2 p) {
    vec2 pixel=uCentre+p*uScale; if(uNewtonMode!=0)return newtonFloat(pixel); vec2 c=uJuliaMode!=0?uJuliaParameter:pixel; vec2 z=initialValue(pixel,c); int iteration=0; float magnitudeSquared=dot(z,z);float trap=1.0e20;vec2 dz=uJuliaMode!=0?vec2(1.0,0.0):vec2(0.0);bool derivativeSupported=uUnaryTransform==0&&uAbsoluteReal==0&&uAbsoluteImaginary==0&&uConjugate==0&&uSwapRealImaginary==0&&uReciprocalPower==0;
    vec2 aq=animatedCoeff(uEquationQuadratic,0.0),ab=animatedCoeff(uEquationLinear,1.1),ac=animatedCoeff(uEquationParameter,2.2),ad=animatedCoeff(uEquationConstant,3.3),ae=animatedCoeff(uIterationTerm,4.4),ar=animatedCoeff(uReciprocalCoefficient,5.5);
    for(int i=0;i<4096;++i) { if(i>=uMaxIterations) break; magnitudeSquared=dot(z,z); if(magnitudeSquared>uBailoutSquared) break;trap=min(trap,trapDistance(z)); vec2 w=transformZ(z);vec2 wp=cpowInt(w,uPower);vec2 next=cmul(aq,wp)+cmul(ab,w)+cmul(ac,cpowInt(c,uParameterPower))+ad+ae*float(i);if(uReciprocalPower>0&&length(ar)>1.0e-8){vec2 den=cpowInt(w,uReciprocalPower);if(dot(den,den)<1.0e-24){magnitudeSquared=uBailoutSquared*2.0;iteration=i+1;break;}next+=cdiv(ar,den);}if(derivativeSupported){vec2 local=cmul(aq,float(uPower)*cpowInt(w,uPower-1))+ab;dz=cmul(local,dz)+(uJuliaMode!=0?vec2(0.0):cmul(ac,float(uParameterPower)*cpowInt(c,uParameterPower-1)));}z=next;iteration=i+1; }
    float distanceEstimate=0.0;float mag=sqrt(max(magnitudeSquared,0.0));float derivativeMagnitude=length(dz);if(derivativeSupported&&derivativeMagnitude>1.0e-12&&mag>1.0)distanceEstimate=0.5*log(mag)*mag/derivativeMagnitude;
    return finishEscape(iteration,magnitudeSquared,trap,distanceEstimate);
}

vec3 directSplit(vec2 p) {
    vec2 rx=ddAdd(vec2(uCentre.x,uCentreLow.x),ddMul(vec2(uScale,0.0),vec2(p.x,0.0))); vec2 iy=ddAdd(vec2(uCentre.y,uCentreLow.y),ddMul(vec2(uScale,0.0),vec2(p.y,0.0))); vec4 c=vec4(rx.x,iy.x,rx.y,iy.y); vec4 z=vec4(0.0); int iteration=0; float magnitudeSquared=0.0; vec4 aq=cddFromVec2(uEquationQuadratic), ab=cddFromVec2(uEquationLinear), ac=cddFromVec2(uEquationParameter), ad=cddFromVec2(uEquationConstant);
    for(int i=0;i<4096;++i) { if(i>=uMaxIterations) break; vec2 zv=cddValue(z); magnitudeSquared=dot(zv,zv); if(magnitudeSquared>uBailoutSquared) break; z=cddAdd(cddAdd(cddMul(aq,cddMul(z,z)),cddMul(ab,z)),cddAdd(cddMul(ac,c),ad)); iteration=i+1; }
    return finishEscape(iteration,magnitudeSquared,1.0e20,0.0);
}
vec4 referenceTimesQ(vec4 realParts, vec4 imaginaryParts, vec4 q) {vec4 result=vec4(0.0);result=cddAdd(result,cddMul(cddFromVec2(vec2(realParts.x,imaginaryParts.x)),q));result=cddAdd(result,cddMul(cddFromVec2(vec2(realParts.y,imaginaryParts.y)),q));result=cddAdd(result,cddMul(cddFromVec2(vec2(realParts.z,imaginaryParts.z)),q));result=cddAdd(result,cddMul(cddFromVec2(vec2(realParts.w,imaginaryParts.w)),q));return result;}
vec3 perturb(vec2 p) {vec4 q=vec4(0.0); int iteration=0; float magnitudeSquared=0.0;vec4 aq=cddFromVec2(uEquationQuadratic), ab=cddFromVec2(uEquationLinear), ac=cddFromVec2(uEquationParameter);vec4 local=cddFromVec2(p);for(int i=0;i<4096;++i){if(i>=uMaxIterations)break;float orbitX=(float(i)+0.5)/max(uReferenceLength,1.0);vec4 zr=texture1D(uReferenceOrbitReal,orbitX);vec4 zi=texture1D(uReferenceOrbitImaginary,orbitX);vec2 Z=vec2(zr.x+zr.y+zr.z+zr.w,zi.x+zi.y+zi.z+zi.w);vec2 delta=cddValue(cddScale(q,uScale));vec2 approximate=Z+delta;magnitudeSquared=dot(approximate,approximate);if(magnitudeSquared>uBailoutSquared)break;vec4 twiceZq=cddScale(referenceTimesQ(zr,zi,q),2.0);vec4 quadraticDelta=cddAdd(twiceZq,cddScale(cddMul(q,q),uScale));q=cddAdd(cddAdd(cddMul(aq,quadraticDelta),cddMul(ab,q)),cddMul(ac,local));iteration=i+1;}return finishEscape(iteration,magnitudeSquared,1.0e20,0.0);}
vec3 sampleFractal(vec2 uv) { float aspect=uResolution.x/max(uResolution.y,1.0); vec2 p=vec2((uv.x*2.0-1.0)*aspect,uv.y*2.0-1.0); if(uPrecisionMode==1)return directSplit(p); if(uPrecisionMode==2)return perturb(p); return directFloat(p); }
void main() { int samples=uAA; vec3 colour=vec3(0.0); float count=0.0; for(int y=0;y<4;++y){if(y>=samples)break;for(int x=0;x<4;++x){if(x>=samples)break;vec2 o=(vec2(float(x),float(y))+0.5)/float(samples)-0.5;colour+=sampleFractal(vUv+o/uResolution);count+=1.0;}} gl_FragColor=vec4(colour/max(count,1.0),1.0); }
)glsl";

const char* kDoubleVertexShader = R"glsl(
#version 400 compatibility
out vec2 vUv;
void main(){gl_Position=gl_Vertex;vUv=gl_MultiTexCoord0.xy;}
)glsl";

const char* kDoubleFragmentShader = R"glsl(
#version 400 compatibility
in vec2 vUv;out vec4 outputColour;
uniform dvec2 uCentreD;uniform double uScaleD;uniform vec2 uResolution;uniform int uMaxIterations;
uniform dvec2 uEquationQuadraticD,uEquationLinearD,uEquationParameterD,uEquationConstantD;
uniform vec2 uIterationTerm,uReciprocalCoefficient,uInitialZ,uJuliaParameter,uNewtonTarget,uNewtonRelaxation,uOrbitTrapPoint;
uniform int uPower,uParameterPower,uReciprocalPower,uAbsoluteReal,uAbsoluteImaginary,uConjugate,uSwapRealImaginary,uUnaryTransform,uInitialZMode,uJuliaMode,uNewtonMode,uNewtonDegree,uColouringMethod,uOrbitTrapType;
uniform float uBailoutSquared,uConvergenceTolerance,uOrbitTrapRadius,uGlowStrength,uDepthStrength,uColourOffset,uBrightness,uContrast,uSaturation;uniform int uPalette,uUseCustomPalette,uSmooth,uAA;uniform sampler1D uCustomPalette;
dvec2 cmulD(dvec2 a,dvec2 b){return dvec2(a.x*b.x-a.y*b.y,a.x*b.y+a.y*b.x);}dvec2 cdivD(dvec2 a,dvec2 b){double d=max(dot(b,b),1.0e-300);return dvec2(a.x*b.x+a.y*b.y,a.y*b.x-a.x*b.y)/d;}dvec2 cpowD(dvec2 z,int p){dvec2 r=dvec2(1.0,0.0);for(int i=0;i<12;++i){if(i>=p)break;r=cmulD(r,z);}return r;}
dvec2 transformD(dvec2 z){if(uAbsoluteReal!=0)z.x=abs(z.x);if(uAbsoluteImaginary!=0)z.y=abs(z.y);if(uSwapRealImaginary!=0)z=z.yx;if(uConjugate!=0)z.y=-z.y;if(uUnaryTransform==1){double ep=exp(clamp(z.y,-20.0,20.0)),em=exp(clamp(-z.y,-20.0,20.0));z=dvec2(sin(z.x)*0.5*(ep+em),cos(z.x)*0.5*(ep-em));}else if(uUnaryTransform==2){double ep=exp(clamp(z.y,-20.0,20.0)),em=exp(clamp(-z.y,-20.0,20.0));z=dvec2(cos(z.x)*0.5*(ep+em),-sin(z.x)*0.5*(ep-em));}else if(uUnaryTransform==3){double e=exp(clamp(z.x,-40.0,40.0));z=e*dvec2(cos(z.y),sin(z.y));}else if(uUnaryTransform==4){z=dvec2(log(max(length(z),1.0e-300)),atan(z.y,z.x));}return z;}
vec3 hsv2rgb(vec3 c){vec3 p=abs(fract(c.xxx+vec3(0.0,2.0/3.0,1.0/3.0))*6.0-3.0);return c.z*mix(vec3(1.0),clamp(p-1.0,0.0,1.0),c.y);}vec3 palette(float t){t=fract(t);if(uUseCustomPalette!=0)return texture(uCustomPalette,t).rgb;if(uPalette==0)return hsv2rgb(vec3(t,0.82,1.0));if(uPalette==1)return mix(vec3(0.0,0.015,0.08),vec3(0.0,0.65,1.0),pow(t,0.7));if(uPalette==2)return mix(vec3(0.06,0.0,0.0),vec3(1.0,0.8,0.05),pow(t,1.5));if(uPalette==3)return mix(vec3(0.02,0.0,0.08),vec3(1.0,0.1,0.9),0.5+0.5*sin(t*6.28318));if(uPalette==4)return mix(vec3(0.0),vec3(0.1,1.0,0.25),pow(t,0.8));if(uPalette==5)return mix(vec3(0.03,0.01,0.0),vec3(1.0,0.72,0.12),pow(t,0.65));if(uPalette==6)return mix(vec3(0.01,0.08,0.12),vec3(0.75,0.98,1.0),pow(t,0.75));if(uPalette==7)return vec3(t);if(uPalette==8)return 0.62+0.38*cos(6.28318*(vec3(0.0,0.33,0.67)+t));return step(0.5,fract(t*8.0))*vec3(1.0)+step(fract(t*8.0),0.5)*vec3(0.01);}vec3 adjust(vec3 c){float l=dot(c,vec3(0.2126,0.7152,0.0722));c=mix(vec3(l),c,uSaturation);c=(c-0.5)*uContrast+0.5;return clamp(c*uBrightness,0.0,1.0);}
vec3 sampleFractal(vec2 uv){double aspect=double(uResolution.x/max(uResolution.y,1.0));dvec2 pixel=uCentreD+dvec2((double(uv.x)*2.0-1.0)*aspect,double(uv.y)*2.0-1.0)*uScaleD;if(uNewtonMode!=0){dvec2 z=pixel;int root=-1;int iteration=0;for(int i=0;i<4096;++i){if(i>=uMaxIterations)break;dvec2 residual=cpowD(z,uNewtonDegree)-dvec2(uNewtonTarget);if(length(residual)<=double(uConvergenceTolerance)){double a=atan(z.y,z.x);if(a<0.0)a+=6.283185307;root=int(floor(a/6.283185307*double(uNewtonDegree)+0.5));if(root>=uNewtonDegree)root=0;iteration=i;break;}dvec2 derivative=double(uNewtonDegree)*cpowD(z,uNewtonDegree-1);if(dot(derivative,derivative)<1.0e-300)break;z-=cmulD(dvec2(uNewtonRelaxation),cdivD(residual,derivative));iteration=i+1;}if(root<0)return vec3(0.0);float shade=1.0-float(iteration)/max(float(uMaxIterations),1.0);return adjust(palette(float(root)/float(uNewtonDegree)+uColourOffset)*(0.45+0.75*shade));}
dvec2 c=uJuliaMode!=0?dvec2(uJuliaParameter):pixel;dvec2 z=uJuliaMode!=0?pixel:(uInitialZMode==1?dvec2(uInitialZ):(uInitialZMode==2?c:dvec2(0.0)));int iteration=0;double magnitudeSquared=dot(z,z);double trap=1.0e300;for(int i=0;i<4096;++i){if(i>=uMaxIterations)break;magnitudeSquared=dot(z,z);if(magnitudeSquared>double(uBailoutSquared))break;trap=min(trap,length(z-dvec2(uOrbitTrapPoint)));dvec2 w=transformD(z);z=cmulD(uEquationQuadraticD,cpowD(w,uPower))+cmulD(uEquationLinearD,w)+cmulD(uEquationParameterD,cpowD(c,uParameterPower))+uEquationConstantD+dvec2(uIterationTerm)*double(i);if(uReciprocalPower>0&&length(uReciprocalCoefficient)>1.0e-8)z+=cdivD(dvec2(uReciprocalCoefficient),cpowD(w,uReciprocalPower));iteration=i+1;}if(iteration>=uMaxIterations)return vec3(0.0);double smooth=double(iteration);if(uSmooth!=0&&magnitudeSquared>1.0){double lm=0.5*log(max(magnitudeSquared,1.000001));if(lm>0.0)smooth=double(iteration)+1.0-log(max(lm,0.000001))/log(max(double(uPower),2.0));}float t=float(smooth/double(max(uMaxIterations,1))*8.0)+uColourOffset;if(uColouringMethod==1)t=float(-log(max(trap,1.0e-12))*0.32)+uColourOffset;vec3 colour=palette(t);colour*=1.0+uDepthStrength*(1.0-float(smooth)/max(float(uMaxIterations),1.0))*0.45;colour+=palette(t+0.08)*exp(float(-trap*18.0))*uGlowStrength*0.6;return adjust(colour);}
void main(){int samples=uAA;vec3 colour=vec3(0.0);float count=0.0;for(int y=0;y<4;++y){if(y>=samples)break;for(int x=0;x<4;++x){if(x>=samples)break;vec2 o=(vec2(float(x),float(y))+0.5)/float(samples)-0.5;colour+=sampleFractal(vUv+o/uResolution);count+=1.0;}}outputColour=vec4(colour/max(count,1.0),1.0);}
)glsl";

const char* kPostProcessFragmentShader = R"glsl(
#version 120
varying vec2 vUv;
uniform sampler2D uFrame;
uniform vec2 uTexel;
uniform float uGlow;
void main(){
    vec3 base=texture2D(uFrame,vUv).rgb;
    vec3 blur=vec3(0.0);
    blur+=texture2D(uFrame,vUv+uTexel*vec2(-1.0,-1.0)).rgb;
    blur+=texture2D(uFrame,vUv+uTexel*vec2( 0.0,-1.0)).rgb*2.0;
    blur+=texture2D(uFrame,vUv+uTexel*vec2( 1.0,-1.0)).rgb;
    blur+=texture2D(uFrame,vUv+uTexel*vec2(-1.0, 0.0)).rgb*2.0;
    blur+=texture2D(uFrame,vUv).rgb*4.0;
    blur+=texture2D(uFrame,vUv+uTexel*vec2( 1.0, 0.0)).rgb*2.0;
    blur+=texture2D(uFrame,vUv+uTexel*vec2(-1.0, 1.0)).rgb;
    blur+=texture2D(uFrame,vUv+uTexel*vec2( 0.0, 1.0)).rgb*2.0;
    blur+=texture2D(uFrame,vUv+uTexel*vec2( 1.0, 1.0)).rgb;
    blur/=16.0;
    float bloom=max(max(blur.r,blur.g),blur.b);
    vec3 glow=blur*max(bloom-0.22,0.0)*uGlow;
    gl_FragColor=vec4(clamp(base+glow,0.0,1.0),1.0);
}
)glsl";

template <typename T> T LoadGlFunction(const char* name) {
    auto address=reinterpret_cast<T>(wglGetProcAddress(name));
    if(!address||address==reinterpret_cast<T>(1)||address==reinterpret_cast<T>(2)||address==reinterpret_cast<T>(3)||address==reinterpret_cast<T>(-1)){
        const HMODULE module=GetModuleHandleW(L"opengl32.dll"); address=reinterpret_cast<T>(GetProcAddress(module,name));
    }
    return address;
}

int PaletteIndex(Palette palette){return static_cast<int>(palette);}
void DrawUnitQuad(){glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();}

bool HasExtension(const char* extensions,const char* name){if(!extensions||!name)return false;const std::string all(extensions);const std::string token(name);std::size_t pos=0;while((pos=all.find(token,pos))!=std::string::npos){const bool left=pos==0||all[pos-1]==' ';const std::size_t end=pos+token.size();const bool right=end==all.size()||all[end]==' ';if(left&&right)return true;pos=end;}return false;}

std::pair<float,float> SplitForFloat(double value){const float high=static_cast<float>(value);return {high,static_cast<float>(value-static_cast<double>(high))};}

} // namespace
#endif

OpenGLRenderer::~OpenGLRenderer(){
#ifdef _WIN32
    Shutdown();
#endif
}

#ifdef _WIN32
bool OpenGLRenderer::Initialise(HWND window,std::string& error){
    Shutdown();window_=window;deviceContext_=GetDC(window_);if(!deviceContext_){error="Could not acquire a rendering device context.";return false;}
    PIXELFORMATDESCRIPTOR format{};format.nSize=sizeof(format);format.nVersion=1;format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;format.cAlphaBits=8;format.iLayerType=PFD_MAIN_PLANE;
    const int pixelFormat=ChoosePixelFormat(deviceContext_,&format);if(pixelFormat==0||!SetPixelFormat(deviceContext_,pixelFormat,&format)){error="The OpenGL pixel format could not be configured.";Shutdown();return false;}
    renderContext_=wglCreateContext(deviceContext_);if(!renderContext_||!wglMakeCurrent(deviceContext_,renderContext_)){error="The OpenGL rendering context could not be created.";Shutdown();return false;}
    if(!LoadFunctions(error)||!BuildPrograms(error)){Shutdown();return false;}
    GLint maximumTextureSize=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maximumTextureSize);maximumPaletteTextureWidth_=std::clamp(static_cast<int>(maximumTextureSize),256,16384);
    glGenTextures(1,&customPaletteTexture_);glBindTexture(GL_TEXTURE_1D,customPaletteTexture_);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glGenTextures(1,&referenceOrbitRealTexture_);glBindTexture(GL_TEXTURE_1D,referenceOrbitRealTexture_);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE_VALUE);
    glGenTextures(1,&referenceOrbitImaginaryTexture_);glBindTexture(GL_TEXTURE_1D,referenceOrbitImaginaryTexture_);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE_VALUE);glBindTexture(GL_TEXTURE_1D,0);
    wglSwapInterval_=LoadGlFunction<WglSwapInterval>("wglSwapIntervalEXT");if(wglSwapInterval_)wglSwapInterval_(0);
    const auto* vendor=reinterpret_cast<const char*>(glGetString(GL_VENDOR));const auto* renderer=reinterpret_cast<const char*>(glGetString(GL_RENDERER));const auto* version=reinterpret_cast<const char*>(glGetString(GL_VERSION));std::ostringstream description;description<<(vendor?vendor:"Unknown vendor")<<" / "<<(renderer?renderer:"Unknown renderer")<<" / OpenGL "<<(version?version:"unknown");graphicsDescription_=description.str();
    RECT client{};GetClientRect(window_,&client);Resize(client.right-client.left,client.bottom-client.top);fpsWindowStart_=std::chrono::steady_clock::now();ready_=true;LogInfo("OpenGL renderer started: "+graphicsDescription_);return true;
}

bool OpenGLRenderer::LoadFunctions(std::string& error){
#define LOAD_GL(member,type,name) member=LoadGlFunction<type>(name);if(!member){error=std::string("Required OpenGL function is unavailable: ")+name;return false;}
    LOAD_GL(glCreateShader_,GLCreateShader,"glCreateShader");LOAD_GL(glShaderSource_,GLShaderSource,"glShaderSource");LOAD_GL(glCompileShader_,GLCompileShader,"glCompileShader");LOAD_GL(glGetShaderiv_,GLGetShaderiv,"glGetShaderiv");LOAD_GL(glGetShaderInfoLog_,GLGetShaderInfoLog,"glGetShaderInfoLog");LOAD_GL(glDeleteShader_,GLDeleteShader,"glDeleteShader");LOAD_GL(glCreateProgram_,GLCreateProgram,"glCreateProgram");LOAD_GL(glAttachShader_,GLAttachShader,"glAttachShader");LOAD_GL(glLinkProgram_,GLLinkProgram,"glLinkProgram");LOAD_GL(glGetProgramiv_,GLGetProgramiv,"glGetProgramiv");LOAD_GL(glGetProgramInfoLog_,GLGetProgramInfoLog,"glGetProgramInfoLog");LOAD_GL(glDeleteProgram_,GLDeleteProgram,"glDeleteProgram");LOAD_GL(glUseProgram_,GLUseProgram,"glUseProgram");LOAD_GL(glGetUniformLocation_,GLGetUniformLocation,"glGetUniformLocation");LOAD_GL(glUniform1i_,GLUniform1i,"glUniform1i");LOAD_GL(glUniform1f_,GLUniform1f,"glUniform1f");LOAD_GL(glUniform2f_,GLUniform2f,"glUniform2f");LOAD_GL(glUniform3f_,GLUniform3f,"glUniform3f");LOAD_GL(glGenFramebuffers_,GLGenFramebuffers,"glGenFramebuffers");LOAD_GL(glBindFramebuffer_,GLBindFramebuffer,"glBindFramebuffer");LOAD_GL(glFramebufferTexture2D_,GLFramebufferTexture2D,"glFramebufferTexture2D");LOAD_GL(glCheckFramebufferStatus_,GLCheckFramebufferStatus,"glCheckFramebufferStatus");LOAD_GL(glDeleteFramebuffers_,GLDeleteFramebuffers,"glDeleteFramebuffers");LOAD_GL(glActiveTexture_,GLActiveTexture,"glActiveTexture");
#undef LOAD_GL
    glUniform1d_=LoadGlFunction<GLUniform1d>("glUniform1d");glUniform2d_=LoadGlFunction<GLUniform2d>("glUniform2d");return true;
}

unsigned OpenGLRenderer::CompileShader(unsigned type,const char* source,std::string& error){const unsigned shader=glCreateShader_(type);glShaderSource_(shader,1,&source,nullptr);glCompileShader_(shader);int compiled=0;glGetShaderiv_(shader,GL_COMPILE_STATUS_VALUE,&compiled);if(!compiled){int length=0;glGetShaderiv_(shader,GL_INFO_LOG_LENGTH_VALUE,&length);std::string log(static_cast<std::size_t>(std::max(length,1)), '\0');glGetShaderInfoLog_(shader,length,nullptr,log.data());error="Shader compilation failed: "+log;glDeleteShader_(shader);return 0;}return shader;}

unsigned OpenGLRenderer::LinkProgram(const char* vertexSource,const char* fragmentSource,std::string& error){const unsigned vertex=CompileShader(GL_VERTEX_SHADER_VALUE,vertexSource,error);if(!vertex)return 0;const unsigned fragment=CompileShader(GL_FRAGMENT_SHADER_VALUE,fragmentSource,error);if(!fragment){glDeleteShader_(vertex);return 0;}const unsigned program=glCreateProgram_();glAttachShader_(program,vertex);glAttachShader_(program,fragment);glLinkProgram_(program);glDeleteShader_(vertex);glDeleteShader_(fragment);int linked=0;glGetProgramiv_(program,GL_LINK_STATUS_VALUE,&linked);if(!linked){int length=0;glGetProgramiv_(program,GL_INFO_LOG_LENGTH_VALUE,&length);std::string log(static_cast<std::size_t>(std::max(length,1)), '\0');glGetProgramInfoLog_(program,length,nullptr,log.data());error="Shader link failed: "+log;glDeleteProgram_(program);return 0;}return program;}

bool OpenGLRenderer::BuildPrograms(std::string& error){
    fractalProgram_=LinkProgram(kVertexShader,kCommonFragmentShader,error);if(!fractalProgram_)return false;
    postProcessProgram_=LinkProgram(kVertexShader,kPostProcessFragmentShader,error);if(!postProcessProgram_)return false;
    const auto* extensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));capabilities_.perturbation=HasExtension(extensions,"GL_ARB_texture_float")||HasExtension(extensions,"GL_ATI_texture_float");capabilities_.arbitraryReference=capabilities_.perturbation;capabilities_.splitFloat=true;
    capabilities_.nativeFloat64=(HasExtension(extensions,"GL_ARB_gpu_shader_fp64")||HasExtension(extensions,"GL_NV_gpu_shader5"))&&glUniform1d_&&glUniform2d_;
    if(capabilities_.nativeFloat64){std::string doubleError;doubleProgram_=LinkProgram(kDoubleVertexShader,kDoubleFragmentShader,doubleError);if(!doubleProgram_){capabilities_.nativeFloat64=false;LogWarning("Native GPU double precision was detected but its shader could not start: "+doubleError);}}
    return true;
}

void OpenGLRenderer::Resize(int width,int height){width_=std::max(width,1);height_=std::max(height,1);}

bool OpenGLRenderer::EnsureRenderTarget(int width,int height,double scale,std::string& error){const int requestedWidth=std::max(1,static_cast<int>(std::lround(width*std::clamp(scale,0.25,1.0))));const int requestedHeight=std::max(1,static_cast<int>(std::lround(height*std::clamp(scale,0.25,1.0))));if(framebuffer_&&targetWidth_==requestedWidth&&targetHeight_==requestedHeight)return true;DestroyRenderTarget();targetWidth_=requestedWidth;targetHeight_=requestedHeight;glGenTextures(1,&renderTexture_);glBindTexture(GL_TEXTURE_2D,renderTexture_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE_VALUE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE_VALUE);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8_VALUE,targetWidth_,targetHeight_,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glGenFramebuffers_(1,&framebuffer_);glBindFramebuffer_(GL_FRAMEBUFFER_VALUE,framebuffer_);glFramebufferTexture2D_(GL_FRAMEBUFFER_VALUE,GL_COLOR_ATTACHMENT0_VALUE,GL_TEXTURE_2D,renderTexture_,0);const auto status=glCheckFramebufferStatus_(GL_FRAMEBUFFER_VALUE);glBindFramebuffer_(GL_FRAMEBUFFER_VALUE,0);if(status!=GL_FRAMEBUFFER_COMPLETE_VALUE){error="The GPU render target could not be created.";DestroyRenderTarget();return false;}return true;}
void OpenGLRenderer::DestroyRenderTarget(){if(framebuffer_&&glDeleteFramebuffers_)glDeleteFramebuffers_(1,&framebuffer_);if(renderTexture_)glDeleteTextures(1,&renderTexture_);framebuffer_=0;renderTexture_=0;targetWidth_=0;targetHeight_=0;}

void OpenGLRenderer::UploadCustomPalette(const std::vector<Colour>& colours){if(!customPaletteTexture_||colours.size()<2)return;const int textureSize=std::clamp(static_cast<int>(std::min<std::size_t>(colours.size(),static_cast<std::size_t>(maximumPaletteTextureWidth_))),2,maximumPaletteTextureWidth_);std::vector<unsigned char> pixels(static_cast<std::size_t>(textureSize)*4U);for(int x=0;x<textureSize;++x){const double position=static_cast<double>(x)/textureSize*colours.size();const auto first=static_cast<std::size_t>(std::floor(position))%colours.size();const auto second=(first+1U)%colours.size();const float f=static_cast<float>(position-std::floor(position));const auto lerp=[f](float a,float b){return std::clamp(a+(b-a)*f,0.0F,1.0F);};Colour c{lerp(colours[first].r,colours[second].r),lerp(colours[first].g,colours[second].g),lerp(colours[first].b,colours[second].b),1.0F};const auto o=static_cast<std::size_t>(x)*4U;pixels[o]=static_cast<unsigned char>(std::lround(c.r*255));pixels[o+1]=static_cast<unsigned char>(std::lround(c.g*255));pixels[o+2]=static_cast<unsigned char>(std::lround(c.b*255));pixels[o+3]=255;}glActiveTexture_(GL_TEXTURE0_VALUE);glBindTexture(GL_TEXTURE_1D,customPaletteTexture_);glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,textureSize,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());}

PrecisionMode OpenGLRenderer::ResolvePrecision(const RenderRegion& region, const PrecisionSettings& settings,
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
        if (!legacyDeepZoomEquation) {
            if (settings.allowFloat64 && available(PrecisionMode::Float64)) return PrecisionMode::Float64;
            return PrecisionMode::Float32;
        }
        if (zoom < 1.0e6) return PrecisionMode::Float32;
        if (settings.allowFloat64 && available(PrecisionMode::Float64) && zoom < 1.0e14) return PrecisionMode::Float64;
        if (settings.allowSplitFloat && available(PrecisionMode::SplitFloat) && zoom < 1.0e13) return PrecisionMode::SplitFloat;
        if (settings.allowArbitraryPrecision && available(PrecisionMode::ArbitraryPrecisionPerturbation)) return PrecisionMode::ArbitraryPrecisionPerturbation;
        if (settings.allowPerturbation && available(PrecisionMode::Perturbation)) return PrecisionMode::Perturbation;
        if (settings.allowFloat64 && available(PrecisionMode::Float64)) return PrecisionMode::Float64;
        if (settings.allowSplitFloat && available(PrecisionMode::SplitFloat)) return PrecisionMode::SplitFloat;
        return PrecisionMode::Float32;
    };
    if (settings.mode == PrecisionMode::Automatic) return fallback();
    if (available(settings.mode)) return settings.mode;
    if (settings.automaticFallback) return fallback();
    error = "The selected precision strategy is unavailable on this GPU or incompatible with the selected equation operations.";
    return settings.mode;
}

bool OpenGLRenderer::UploadReferenceOrbit(const RenderRegion& region,PrecisionMode mode,int bits,std::string& error){
    std::ostringstream key;key<<std::setprecision(17)<<static_cast<int>(mode)<<':'<<region.camera.centreX<<':'<<region.camera.centreXLow<<':'<<region.camera.centreY<<':'<<region.camera.centreYLow<<':'<<region.maximumIterations<<':'<<bits<<':'<<EquationSummary(region.equation);
    if(key.str()==referenceOrbitKey_)return true;
    ReferenceOrbit orbit=mode==PrecisionMode::ArbitraryPrecisionPerturbation?BuildReferenceOrbitArbitrary(region.camera,region.equation,region.maximumIterations,bits):BuildReferenceOrbitDouble(region.camera,region.equation,region.maximumIterations);
    if(orbit.points.empty()){error="The deep-zoom reference orbit could not be generated.";return false;}
    std::vector<float> realData(orbit.points.size()*4U,0.0F),imaginaryData(orbit.points.size()*4U,0.0F);
    for(std::size_t i=0;i<orbit.points.size();++i){for(std::size_t part=0;part<4U;++part){realData[i*4U+part]=orbit.points[i].real[part];imaginaryData[i*4U+part]=orbit.points[i].imaginary[part];}}
    glActiveTexture_(GL_TEXTURE1_VALUE);glBindTexture(GL_TEXTURE_1D,referenceOrbitRealTexture_);glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA32F_VALUE,static_cast<GLsizei>(orbit.points.size()),0,GL_RGBA,GL_FLOAT,realData.data());
    glActiveTexture_(GL_TEXTURE2_VALUE);glBindTexture(GL_TEXTURE_1D,referenceOrbitImaginaryTexture_);glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA32F_VALUE,static_cast<GLsizei>(orbit.points.size()),0,GL_RGBA,GL_FLOAT,imaginaryData.data());
    if(glGetError()!=GL_NO_ERROR){error="The GPU could not upload a floating-point perturbation reference orbit.";return false;}
    referenceOrbitLength_=static_cast<int>(orbit.points.size());referenceOrbitKey_=key.str();return true;
}

void OpenGLRenderer::DrawFractalRegion(const RenderRegion& region, const RECT& targetRect,
                                        int, int targetHeight, int aaLevel,
                                        const PrecisionSettings& precisionSettings, double timeSeconds,
                                        std::string& error) {
    const int x = targetRect.left;
    const int y = targetHeight - targetRect.bottom;
    const int width = std::max(1, static_cast<int>(targetRect.right - targetRect.left));
    const int height = std::max(1, static_cast<int>(targetRect.bottom - targetRect.top));
    glViewport(x, y, width, height);
    glScissor(x, y, width, height);
    glEnable(GL_SCISSOR_TEST);

    const PrecisionMode mode = ResolvePrecision(region, precisionSettings, error);
    if (!error.empty() && !precisionSettings.automaticFallback) return;
    const bool useDouble = mode == PrecisionMode::Float64 && doubleProgram_ != 0;
    const unsigned program = useDouble ? doubleProgram_ : fractalProgram_;
    glUseProgram_(program);

    const bool useCustom = region.customPaletteColours.size() >= 2;
    if (useCustom) {
        UploadCustomPalette(region.customPaletteColours);
        glActiveTexture_(GL_TEXTURE0_VALUE);
        glEnable(GL_TEXTURE_1D);
        glBindTexture(GL_TEXTURE_1D, customPaletteTexture_);
    }
    const auto uniform1i = [&](const char* name, int value) {
        glUniform1i_(glGetUniformLocation_(program, name), value);
    };
    const auto uniform1f = [&](const char* name, float value) {
        glUniform1f_(glGetUniformLocation_(program, name), value);
    };
    const auto uniform2f = [&](const char* name, double real, double imaginary) {
        glUniform2f_(glGetUniformLocation_(program, name), static_cast<float>(real), static_cast<float>(imaginary));
    };

    uniform1i("uCustomPalette", 0);
    uniform1i("uUseCustomPalette", useCustom ? 1 : 0);
    uniform2f("uResolution", width, height);
    uniform1i("uMaxIterations", std::clamp(region.maximumIterations, 32, 4096));
    uniform1i("uAbsoluteReal", region.equation.absoluteReal ? 1 : 0);
    uniform1i("uAbsoluteImaginary", region.equation.absoluteImaginary ? 1 : 0);
    uniform1i("uConjugate", region.equation.conjugate ? 1 : 0);
    uniform1i("uSwapRealImaginary", region.equation.swapRealImaginary ? 1 : 0);
    uniform1i("uUnaryTransform", static_cast<int>(region.equation.unaryTransform));
    uniform1i("uPower", region.equation.power);
    uniform1i("uParameterPower", region.equation.parameterPower);
    uniform1i("uReciprocalPower", region.equation.reciprocalPower);
    uniform2f("uIterationTerm", region.equation.iterationTerm.real, region.equation.iterationTerm.imaginary);
    uniform2f("uReciprocalCoefficient", region.equation.reciprocalCoefficient.real,
              region.equation.reciprocalCoefficient.imaginary);
    uniform1i("uInitialZMode", static_cast<int>(region.equation.initialZMode));
    uniform2f("uInitialZ", region.equation.initialZ.real, region.equation.initialZ.imaginary);
    uniform1i("uJuliaMode", region.equation.juliaMode ? 1 : 0);
    uniform2f("uJuliaParameter", region.equation.juliaParameter.real, region.equation.juliaParameter.imaginary);
    uniform1f("uBailoutSquared", static_cast<float>(region.equation.bailoutRadius * region.equation.bailoutRadius));
    uniform1i("uNewtonMode", (region.equation.newtonMode ||
                              region.equation.renderMode == FractalRenderMode::Newton) ? 1 : 0);
    uniform1i("uNewtonDegree", region.equation.newtonDegree);
    uniform2f("uNewtonTarget", region.equation.newtonTarget.real, region.equation.newtonTarget.imaginary);
    uniform2f("uNewtonRelaxation", region.equation.newtonRelaxation.real,
              region.equation.newtonRelaxation.imaginary);
    uniform1f("uConvergenceTolerance", static_cast<float>(region.equation.convergenceTolerance));
    uniform1i("uColouringMethod", static_cast<int>(region.equation.colouringMethod));
    uniform1i("uOrbitTrapType", static_cast<int>(region.equation.orbitTrap));
    uniform2f("uOrbitTrapPoint", region.equation.orbitTrapPoint.real, region.equation.orbitTrapPoint.imaginary);
    uniform1f("uOrbitTrapRadius", static_cast<float>(region.equation.orbitTrapRadius));
    uniform1f("uGlowStrength", static_cast<float>(region.equation.glowStrength));
    uniform1f("uDepthStrength", static_cast<float>(region.equation.depthStrength));
    uniform1i("uAnimateCoefficients", region.equation.animateCoefficients ? 1 : 0);
    uniform1f("uCoefficientAnimationSpeed", static_cast<float>(region.equation.coefficientAnimationSpeed));
    uniform1f("uCoefficientAnimationAmplitude", static_cast<float>(region.equation.coefficientAnimationAmplitude));
    const double renderTime = timeSeconds >= 0.0 && std::isfinite(timeSeconds)
        ? timeSeconds
        : std::chrono::duration<double>(
              std::chrono::steady_clock::now().time_since_epoch()).count();
    uniform1f("uTime", static_cast<float>(std::fmod(renderTime, 100000.0)));

    uniform1i("uPalette", PaletteIndex(region.palette));
    uniform1f("uColourOffset", static_cast<float>(region.colourOffset));
    uniform1f("uBrightness", static_cast<float>(region.brightness));
    uniform1f("uContrast", static_cast<float>(region.contrast));
    uniform1f("uSaturation", static_cast<float>(region.saturation));
    glUniform3f_(glGetUniformLocation_(program, "uInterior"), region.interiorColour.r,
                 region.interiorColour.g, region.interiorColour.b);
    glUniform3f_(glGetUniformLocation_(program, "uBackground"), region.backgroundColour.r,
                 region.backgroundColour.g, region.backgroundColour.b);
    uniform1i("uSmooth", region.smoothColouring ? 1 : 0);
    uniform1i("uAA", std::clamp(aaLevel, 1, 4));

    if (useDouble) {
        glUniform2d_(glGetUniformLocation_(program, "uCentreD"), CameraCentreX(region.camera),
                     CameraCentreY(region.camera));
        glUniform1d_(glGetUniformLocation_(program, "uScaleD"), region.camera.scale);
        glUniform2d_(glGetUniformLocation_(program, "uEquationQuadraticD"),
                     region.equation.quadratic.real, region.equation.quadratic.imaginary);
        glUniform2d_(glGetUniformLocation_(program, "uEquationLinearD"),
                     region.equation.linear.real, region.equation.linear.imaginary);
        glUniform2d_(glGetUniformLocation_(program, "uEquationParameterD"),
                     region.equation.parameter.real, region.equation.parameter.imaginary);
        glUniform2d_(glGetUniformLocation_(program, "uEquationConstantD"),
                     region.equation.constant.real, region.equation.constant.imaginary);
    } else {
        const auto cx = SplitForFloat(region.camera.centreX);
        const auto cy = SplitForFloat(region.camera.centreY);
        uniform2f("uCentre", cx.first, cy.first);
        uniform2f("uCentreLow", cx.second + static_cast<float>(region.camera.centreXLow),
                  cy.second + static_cast<float>(region.camera.centreYLow));
        uniform1f("uScale", static_cast<float>(region.camera.scale));
        uniform2f("uEquationQuadratic", region.equation.quadratic.real, region.equation.quadratic.imaginary);
        uniform2f("uEquationLinear", region.equation.linear.real, region.equation.linear.imaginary);
        uniform2f("uEquationParameter", region.equation.parameter.real, region.equation.parameter.imaginary);
        uniform2f("uEquationConstant", region.equation.constant.real, region.equation.constant.imaginary);
        const int shaderMode = mode == PrecisionMode::SplitFloat ? 1 :
            ((mode == PrecisionMode::Perturbation || mode == PrecisionMode::ArbitraryPrecisionPerturbation) ? 2 : 0);
        uniform1i("uPrecisionMode", shaderMode);
        if (shaderMode == 2) {
            if (!UploadReferenceOrbit(region, mode, precisionSettings.arbitraryPrecisionBits, error)) {
                glUseProgram_(0);
                return;
            }
            glActiveTexture_(GL_TEXTURE1_VALUE);
            glEnable(GL_TEXTURE_1D);
            glBindTexture(GL_TEXTURE_1D, referenceOrbitRealTexture_);
            uniform1i("uReferenceOrbitReal", 1);
            glActiveTexture_(GL_TEXTURE2_VALUE);
            glEnable(GL_TEXTURE_1D);
            glBindTexture(GL_TEXTURE_1D, referenceOrbitImaginaryTexture_);
            uniform1i("uReferenceOrbitImaginary", 2);
            uniform1f("uReferenceLength", static_cast<float>(referenceOrbitLength_));
        }
    }
    precisionDescription_ = PrecisionModeDisplayName(mode);
    DrawUnitQuad();
    glActiveTexture_(GL_TEXTURE2_VALUE); glBindTexture(GL_TEXTURE_1D, 0); glDisable(GL_TEXTURE_1D);
    glActiveTexture_(GL_TEXTURE1_VALUE); glBindTexture(GL_TEXTURE_1D, 0); glDisable(GL_TEXTURE_1D);
    glActiveTexture_(GL_TEXTURE0_VALUE); glBindTexture(GL_TEXTURE_1D, 0); glDisable(GL_TEXTURE_1D);
    glDisable(GL_SCISSOR_TEST);
    glUseProgram_(0);
}

void OpenGLRenderer::DrawTextureToBackbuffer(){
    glBindFramebuffer_(GL_FRAMEBUFFER_VALUE,0);glViewport(0,0,width_,height_);glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,renderTexture_);
    glUseProgram_(postProcessProgram_);glUniform1i_(glGetUniformLocation_(postProcessProgram_,"uFrame"),0);
    glUniform2f_(glGetUniformLocation_(postProcessProgram_,"uTexel"),1.0F/static_cast<float>(std::max(targetWidth_,1)),1.0F/static_cast<float>(std::max(targetHeight_,1)));
    glUniform1f_(glGetUniformLocation_(postProcessProgram_,"uGlow"),postProcessGlowStrength_);
    DrawUnitQuad();glUseProgram_(0);glBindTexture(GL_TEXTURE_2D,0);glDisable(GL_TEXTURE_2D);
}

bool OpenGLRenderer::Render(const std::vector<RenderRegion>& regions,const RenderOptions& options,std::string& error){if(!ready_||!window_||!IsWindow(window_)){error="The renderer is not ready.";return false;}if(!wglMakeCurrent(deviceContext_,renderContext_)){error="The GPU context could not be activated.";return false;}RECT client{};GetClientRect(window_,&client);Resize(client.right-client.left,client.bottom-client.top);if(!EnsureRenderTarget(width_,height_,options.renderScale,error))return false;glBindFramebuffer_(GL_FRAMEBUFFER_VALUE,framebuffer_);glViewport(0,0,targetWidth_,targetHeight_);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);postProcessGlowStrength_=0.0F;for(const auto& region:regions){postProcessGlowStrength_=std::max(postProcessGlowStrength_,static_cast<float>(region.equation.glowStrength));RECT scaled{static_cast<LONG>(std::lround(static_cast<double>(region.pixels.left)*targetWidth_/width_)),static_cast<LONG>(std::lround(static_cast<double>(region.pixels.top)*targetHeight_/height_)),static_cast<LONG>(std::lround(static_cast<double>(region.pixels.right)*targetWidth_/width_)),static_cast<LONG>(std::lround(static_cast<double>(region.pixels.bottom)*targetHeight_/height_))};scaled.left=std::clamp<LONG>(scaled.left,0,targetWidth_);scaled.right=std::clamp<LONG>(scaled.right,0,targetWidth_);scaled.top=std::clamp<LONG>(scaled.top,0,targetHeight_);scaled.bottom=std::clamp<LONG>(scaled.bottom,0,targetHeight_);DrawFractalRegion(region,scaled,targetWidth_,targetHeight_,options.antiAliasingLevel,options.precision,options.timeSeconds,error);if(!error.empty())return false;}DrawTextureToBackbuffer();if(!SwapBuffers(deviceContext_)){error="The rendered frame could not be presented.";return false;}++fpsFrameCount_;const auto now=std::chrono::steady_clock::now();const double elapsed=std::chrono::duration<double>(now-fpsWindowStart_).count();if(elapsed>=1.0){framesPerSecond_=fpsFrameCount_/elapsed;fpsFrameCount_=0;fpsWindowStart_=now;}return true;}

int OpenGLRenderer::MaximumRenderDimension() const noexcept {
    if (!ready_ || !deviceContext_ || !renderContext_) return 0;
    if (!wglMakeCurrent(deviceContext_, renderContext_)) return 0;
    GLint maximumTexture = 0;
    GLint maximumViewport[2]{0, 0};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTexture);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maximumViewport);
    const int limit = std::min({maximumTexture, maximumViewport[0], maximumViewport[1]});
    return std::max(limit, 0);
}

bool OpenGLRenderer::CapturePixels(std::vector<std::uint32_t>& pixels,int& width,int& height,std::string& error){if(!ready_||!deviceContext_||!renderContext_){error="The renderer is not ready to capture a static image.";return false;}if(!wglMakeCurrent(deviceContext_,renderContext_)){error="The GPU context could not be activated for static capture.";return false;}width=std::max(width_,1);height=std::max(height_,1);std::vector<unsigned char> rgba(static_cast<std::size_t>(width)*height*4U);glReadBuffer(GL_FRONT);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());glReadBuffer(GL_BACK);if(glGetError()!=GL_NO_ERROR){error="The rendered frame could not be read back from the GPU.";return false;}pixels.resize(static_cast<std::size_t>(width)*height);for(std::size_t i=0;i<pixels.size();++i){const auto b=i*4U;pixels[i]=0xFF000000U|(static_cast<std::uint32_t>(rgba[b])<<16U)|(static_cast<std::uint32_t>(rgba[b+1])<<8U)|rgba[b+2];}return true;}

void OpenGLRenderer::Shutdown(){ready_=false;if(renderContext_&&deviceContext_)wglMakeCurrent(deviceContext_,renderContext_);DestroyRenderTarget();if(customPaletteTexture_)glDeleteTextures(1,&customPaletteTexture_);if(referenceOrbitRealTexture_)glDeleteTextures(1,&referenceOrbitRealTexture_);if(referenceOrbitImaginaryTexture_)glDeleteTextures(1,&referenceOrbitImaginaryTexture_);customPaletteTexture_=referenceOrbitRealTexture_=referenceOrbitImaginaryTexture_=0;if(fractalProgram_&&glDeleteProgram_)glDeleteProgram_(fractalProgram_);if(doubleProgram_&&glDeleteProgram_)glDeleteProgram_(doubleProgram_);if(postProcessProgram_&&glDeleteProgram_)glDeleteProgram_(postProcessProgram_);fractalProgram_=doubleProgram_=postProcessProgram_=0;if(renderContext_){wglMakeCurrent(nullptr,nullptr);wglDeleteContext(renderContext_);renderContext_=nullptr;}if(window_&&deviceContext_){ReleaseDC(window_,deviceContext_);deviceContext_=nullptr;}window_=nullptr;graphicsDescription_.clear();precisionDescription_="Not rendered yet";capabilities_={};referenceOrbitKey_.clear();referenceOrbitLength_=0;}
#endif

} // namespace mw
