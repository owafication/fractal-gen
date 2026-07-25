#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mw {

enum class AnimationMode { AutomaticJourney, ContinuousZoom, StaticAnimatedColour, ManualView };
enum class ZoomRestartBehaviour { Restart, PingPong };
enum class Palette { ClassicSpectrum, DeepOcean, Fire, PurpleNeon, GreenMatrix, Gold, Ice, Greyscale, Pastel, HighContrast };
enum class PerformanceProfile { BatterySaver, Balanced, HighQuality, Custom };
enum class MonitorMode { Mirror, Span, Independent };
enum class PrecisionMode { Automatic, Float32, Float64, SplitFloat, Perturbation, ArbitraryPrecisionPerturbation };
enum class StaticSlideshowOrder { Sequential, Shuffle };
enum class EquationUnaryTransform { None, Sin, Cos, Exp, Log };
enum class InitialZMode { Zero, Fixed, Parameter, CriticalPoint };
enum class FractalRenderMode { EscapeTime, Newton };
enum class ColouringMethod { SmoothEscape, OrbitTrap, DistanceEstimation, NewtonBasins };
enum class OrbitTrapType { Point, Cross, Circle };

struct Colour {
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{1.0F};
};

struct ComplexCoefficient {
    double real{0.0};
    double imaginary{0.0};
};

struct EquationSettings {
    // Bounded, data-only recurrence. No shader source or executable content is accepted.
    // Escape-time mode evaluates:
    // z(n+1) = A*T(z)^power + B*T(z) + C*c + D + E*n + lambda/T(z)^reciprocalPower
    // where T can apply component transforms, conjugation, swapping and one analytic function.
    ComplexCoefficient quadratic{1.0, 0.0}; // A; retained name for preset compatibility.
    ComplexCoefficient linear{0.0, 0.0};
    ComplexCoefficient parameter{1.0, 0.0};
    ComplexCoefficient constant{0.0, 0.0};
    ComplexCoefficient iterationTerm{0.0, 0.0};
    ComplexCoefficient reciprocalCoefficient{0.0, 0.0};
    int power{2};
    int parameterPower{1};
    int reciprocalPower{0};
    bool absoluteReal{false};
    bool absoluteImaginary{false};
    bool conjugate{false};
    bool swapRealImaginary{false};
    EquationUnaryTransform unaryTransform{EquationUnaryTransform::None};

    InitialZMode initialZMode{InitialZMode::Zero};
    ComplexCoefficient initialZ{0.0, 0.0};
    bool juliaMode{false};
    ComplexCoefficient juliaParameter{-0.8, 0.156};
    double bailoutRadius{2.0};

    FractalRenderMode renderMode{FractalRenderMode::EscapeTime};
    bool newtonMode{false}; // Compatibility flag; normalisation keeps it aligned with renderMode.
    int newtonDegree{3};
    ComplexCoefficient newtonTarget{1.0, 0.0};
    ComplexCoefficient newtonRelaxation{1.0, 0.0};
    double convergenceTolerance{1.0e-6};

    ColouringMethod colouringMethod{ColouringMethod::SmoothEscape};
    OrbitTrapType orbitTrap{OrbitTrapType::Point};
    ComplexCoefficient orbitTrapPoint{0.0, 0.0};
    double orbitTrapRadius{0.5};
    double glowStrength{0.0};
    double depthStrength{0.0};

    bool animateCoefficients{false};
    double coefficientAnimationSpeed{0.25};
    double coefficientAnimationAmplitude{0.0};
};

struct EquationPreset {
    std::string id;
    std::string name;
    EquationSettings equation;
};

struct PalettePreset {
    std::string id;
    std::string name;
    std::vector<Colour> colours;
};

struct CameraState {
    double centreX{-0.5};
    double centreY{0.0};
    double scale{1.5}; // half-height in complex-plane units
    // Compensated low components preserve pan offsets below one double ULP.
    double centreXLow{0.0};
    double centreYLow{0.0};
};

struct Preset {
    std::string id;
    std::string name;
    bool builtIn{false};
    CameraState camera;
    double startingScale{1.5};
    double maximumZoom{1.0e30};
    double zoomSpeed{0.08};
    ZoomRestartBehaviour zoomRestartBehaviour{ZoomRestartBehaviour::Restart};
    int maximumIterations{300};
    EquationSettings equation;
    Palette palette{Palette::ClassicSpectrum};
    // Empty means use the selected built-in palette. Otherwise colours are
    // interpolated in list order and uploaded as a GPU palette texture.
    std::vector<Colour> customPaletteColours;
    double colourOffset{0.0};
    double colourCycleSpeed{0.02};
    double brightness{1.0};
    double contrast{1.0};
    double saturation{1.0};
    Colour interiorColour{0.0F, 0.0F, 0.0F, 1.0F};
    Colour backgroundColour{0.0F, 0.0F, 0.0F, 1.0F};
    bool smoothColouring{true};
    AnimationMode animationMode{AnimationMode::AutomaticJourney};
    // Optional exact ordered journey. Each row transitions from the current
    // view to the row coordinate, holds there, then continues to the next row:
    // centreX,centreY,scale,transitionSeconds,holdSeconds
    std::string automaticJourneyWaypoints;
    int frameRateLimit{30};
    double renderScale{0.75};
    int antiAliasingLevel{1};
};

struct PrecisionSettings {
    PrecisionMode mode{PrecisionMode::Automatic};
    bool allowFloat64{true};
    bool allowSplitFloat{true};
    bool allowPerturbation{true};
    bool allowArbitraryPrecision{true};
    bool automaticFallback{true};
    int arbitraryPrecisionBits{256};
};


struct AdaptivePerformanceSettings {
    bool enabled{true};
    bool pauseOnLowFps{true};
    double minimumFramesPerSecond{8.0};
    int lowFpsSustainMs{5000};
    bool pauseOnHighCpu{true};
    double maximumProcessCpuPercent{65.0};
    int highCpuSustainMs{5000};
    bool pauseOnHighMemory{true};
    int maximumWorkingSetMb{2048};
    int highMemorySustainMs{5000};
    int resumeStableMs{8000};
    bool stopWhenVisuallyUnchanged{true};
    double minimumVisiblePixelChange{0.25};
    double minimumVisibleColourChange{0.001};
};

struct PerformanceSettings {
    PerformanceProfile profile{PerformanceProfile::Balanced};
    int maximumFrameRate{30};
    double renderScale{0.75};
    int maximumIterations{350};
    int antiAliasingLevel{1};
    bool pauseOnBattery{false};
    bool reduceQualityOnBattery{true};
    bool pauseWhenFullscreen{true};
    bool pauseWhenDesktopHidden{true};
    bool pauseDuringRemoteDesktop{true};
    bool pauseWhenLocked{true};
    int resumeDelayMs{1250};
    PrecisionSettings precision;
    AdaptivePerformanceSettings adaptive;
};

struct GeneralSettings {
    bool startWithWindows{false};
    bool minimiseToTray{true};
    bool reducedMotion{false};
    bool colourCyclingEnabled{false};
    bool restoreOnExit{true};
    bool startWallpaperOnLaunch{false};
};

struct StaticWallpaperSettings {
    bool enabled{false};
    bool cycleEnabled{false};
    int cycleSeconds{300};
    int currentIndex{0};
    StaticSlideshowOrder order{StaticSlideshowOrder::Sequential};
    // UTF-8 directory used for new captures. Existing slideshow entries may
    // still point elsewhere and are never executed or interpreted as code.
    std::string storageDirectory;
    std::vector<std::string> imagePaths;
};

struct AppSettings {
    int schemaVersion{8};
    std::string selectedPresetId{"full-view"};
    PerformanceSettings performance;
    GeneralSettings general;
    StaticWallpaperSettings staticWallpaper;
    MonitorMode monitorMode{MonitorMode::Mirror};
    std::map<std::string, std::string> monitorPresetAssignments;
    std::vector<Preset> customPresets;
    std::vector<PalettePreset> customPalettePresets;
    std::vector<EquationPreset> customEquationPresets;
    bool lastWallpaperRunning{false};
};

struct ValidationIssue {
    std::string field;
    std::string message;
};

struct ValidationResult {
    bool valid{true};
    std::vector<ValidationIssue> issues;
};

std::string ToString(AnimationMode value);
std::string ToString(Palette value);
std::string ToString(ZoomRestartBehaviour value);
std::string ToString(PerformanceProfile value);
std::string ToString(MonitorMode value);
std::string ToString(PrecisionMode value);
std::string ToString(StaticSlideshowOrder value);
std::string ToString(EquationUnaryTransform value);
std::string ToString(InitialZMode value);
std::string ToString(FractalRenderMode value);
std::string ToString(ColouringMethod value);
std::string ToString(OrbitTrapType value);

std::optional<AnimationMode> AnimationModeFromString(const std::string& value);
std::optional<Palette> PaletteFromString(const std::string& value);
std::optional<ZoomRestartBehaviour> ZoomRestartBehaviourFromString(const std::string& value);
std::optional<PerformanceProfile> PerformanceProfileFromString(const std::string& value);
std::optional<MonitorMode> MonitorModeFromString(const std::string& value);
std::optional<PrecisionMode> PrecisionModeFromString(const std::string& value);
std::optional<StaticSlideshowOrder> StaticSlideshowOrderFromString(const std::string& value);
std::optional<EquationUnaryTransform> EquationUnaryTransformFromString(const std::string& value);
std::optional<InitialZMode> InitialZModeFromString(const std::string& value);
std::optional<FractalRenderMode> FractalRenderModeFromString(const std::string& value);
std::optional<ColouringMethod> ColouringMethodFromString(const std::string& value);
std::optional<OrbitTrapType> OrbitTrapTypeFromString(const std::string& value);

PerformanceSettings SettingsForProfile(PerformanceProfile profile);
std::vector<Colour> PalettePreviewColours(Palette palette);
std::vector<PalettePreset> BuiltInPalettePresets();
std::string EquationSummary(const EquationSettings& equation);
EquationSettings EquationExample(std::size_t index);
std::vector<std::string> EquationExampleNames();
ValidationResult ValidateAndNormalise(Preset& preset);
ValidationResult ValidateAndNormalise(PalettePreset& preset);
ValidationResult ValidateAndNormalise(EquationPreset& preset);
ValidationResult ValidateAndNormalise(AppSettings& settings);
std::vector<Preset> BuiltInPresets();

} // namespace mw
