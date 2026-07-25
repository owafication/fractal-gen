#include "Core/Models.h"

#include <array>
#include <limits>
#include <iomanip>
#include <sstream>

namespace mw {
namespace {

template <typename T>
T ClampFinite(T value, T minimum, T maximum, T fallback) {
    if (!std::isfinite(static_cast<double>(value))) {
        return fallback;
    }
    return std::clamp(value, minimum, maximum);
}


std::string FormatCoefficient(const ComplexCoefficient& value) {
    std::ostringstream stream;
    stream << std::setprecision(5);
    if (std::abs(value.imaginary) < 1.0e-12) {
        stream << value.real;
    } else {
        stream << "(" << value.real;
        if (value.imaginary >= 0.0) stream << "+";
        stream << value.imaginary << "i)";
    }
    return stream.str();
}

bool IsZero(const ComplexCoefficient& value) {
    return std::abs(value.real) < 1.0e-12 && std::abs(value.imaginary) < 1.0e-12;
}

void AddIssue(ValidationResult& result, std::string field, std::string message) {
    result.valid = false;
    result.issues.push_back({std::move(field), std::move(message)});
}

} // namespace

std::string ToString(AnimationMode value) {
    switch (value) {
    case AnimationMode::AutomaticJourney: return "automatic-journey";
    case AnimationMode::ContinuousZoom: return "continuous-zoom";
    case AnimationMode::StaticAnimatedColour: return "static-animated-colour";
    case AnimationMode::ManualView: return "manual-view";
    }
    return "automatic-journey";
}

std::string ToString(Palette value) {
    switch (value) {
    case Palette::ClassicSpectrum: return "classic-spectrum";
    case Palette::DeepOcean: return "deep-ocean";
    case Palette::Fire: return "fire";
    case Palette::PurpleNeon: return "purple-neon";
    case Palette::GreenMatrix: return "green-matrix";
    case Palette::Gold: return "gold";
    case Palette::Ice: return "ice";
    case Palette::Greyscale: return "greyscale";
    case Palette::Pastel: return "pastel";
    case Palette::HighContrast: return "high-contrast";
    }
    return "classic-spectrum";
}


std::string ToString(ZoomRestartBehaviour value) {
    switch (value) {
    case ZoomRestartBehaviour::Restart: return "restart";
    case ZoomRestartBehaviour::PingPong: return "ping-pong";
    }
    return "restart";
}

std::string ToString(PerformanceProfile value) {
    switch (value) {
    case PerformanceProfile::BatterySaver: return "battery-saver";
    case PerformanceProfile::Balanced: return "balanced";
    case PerformanceProfile::HighQuality: return "high-quality";
    case PerformanceProfile::Custom: return "custom";
    }
    return "balanced";
}

std::string ToString(MonitorMode value) {
    switch (value) {
    case MonitorMode::Mirror: return "mirror";
    case MonitorMode::Span: return "span";
    case MonitorMode::Independent: return "independent";
    }
    return "mirror";
}

std::string ToString(PrecisionMode value) {
    switch (value) {
    case PrecisionMode::Automatic: return "automatic";
    case PrecisionMode::Float32: return "float32";
    case PrecisionMode::Float64: return "float64";
    case PrecisionMode::SplitFloat: return "split-float";
    case PrecisionMode::Perturbation: return "perturbation";
    case PrecisionMode::ArbitraryPrecisionPerturbation: return "arbitrary-perturbation";
    }
    return "automatic";
}

std::string ToString(StaticSlideshowOrder value) {
    switch (value) {
    case StaticSlideshowOrder::Sequential: return "sequential";
    case StaticSlideshowOrder::Shuffle: return "shuffle";
    }
    return "sequential";
}

std::string ToString(EquationUnaryTransform value) {
    switch (value) {
    case EquationUnaryTransform::None: return "none";
    case EquationUnaryTransform::Sin: return "sin";
    case EquationUnaryTransform::Cos: return "cos";
    case EquationUnaryTransform::Exp: return "exp";
    case EquationUnaryTransform::Log: return "log";
    }
    return "none";
}

std::string ToString(InitialZMode value) {
    switch (value) {
    case InitialZMode::Zero: return "zero";
    case InitialZMode::Fixed: return "fixed";
    case InitialZMode::Parameter: return "parameter";
    case InitialZMode::CriticalPoint: return "critical-point";
    }
    return "zero";
}

std::string ToString(FractalRenderMode value) {
    return value == FractalRenderMode::Newton ? "newton" : "escape-time";
}

std::string ToString(ColouringMethod value) {
    switch (value) {
    case ColouringMethod::SmoothEscape: return "smooth-escape";
    case ColouringMethod::OrbitTrap: return "orbit-trap";
    case ColouringMethod::DistanceEstimation: return "distance-estimation";
    case ColouringMethod::NewtonBasins: return "newton-basins";
    }
    return "smooth-escape";
}

std::string ToString(OrbitTrapType value) {
    switch (value) {
    case OrbitTrapType::Point: return "point";
    case OrbitTrapType::Cross: return "cross";
    case OrbitTrapType::Circle: return "circle";
    }
    return "point";
}

std::optional<AnimationMode> AnimationModeFromString(const std::string& value) {
    if (value == "automatic-journey") return AnimationMode::AutomaticJourney;
    if (value == "continuous-zoom") return AnimationMode::ContinuousZoom;
    if (value == "static-animated-colour") return AnimationMode::StaticAnimatedColour;
    if (value == "manual-view") return AnimationMode::ManualView;
    return std::nullopt;
}

std::optional<Palette> PaletteFromString(const std::string& value) {
    static const std::array<std::pair<const char*, Palette>, 10> values{{
        {"classic-spectrum", Palette::ClassicSpectrum},
        {"deep-ocean", Palette::DeepOcean},
        {"fire", Palette::Fire},
        {"purple-neon", Palette::PurpleNeon},
        {"green-matrix", Palette::GreenMatrix},
        {"gold", Palette::Gold},
        {"ice", Palette::Ice},
        {"greyscale", Palette::Greyscale},
        {"pastel", Palette::Pastel},
        {"high-contrast", Palette::HighContrast},
    }};
    for (const auto& [name, palette] : values) {
        if (value == name) return palette;
    }
    return std::nullopt;
}


std::optional<ZoomRestartBehaviour> ZoomRestartBehaviourFromString(const std::string& value) {
    if (value == "restart") return ZoomRestartBehaviour::Restart;
    if (value == "ping-pong") return ZoomRestartBehaviour::PingPong;
    return std::nullopt;
}

std::optional<PerformanceProfile> PerformanceProfileFromString(const std::string& value) {
    if (value == "battery-saver") return PerformanceProfile::BatterySaver;
    if (value == "balanced") return PerformanceProfile::Balanced;
    if (value == "high-quality") return PerformanceProfile::HighQuality;
    if (value == "custom") return PerformanceProfile::Custom;
    return std::nullopt;
}

std::optional<MonitorMode> MonitorModeFromString(const std::string& value) {
    if (value == "mirror") return MonitorMode::Mirror;
    if (value == "span") return MonitorMode::Span;
    if (value == "independent") return MonitorMode::Independent;
    return std::nullopt;
}

std::optional<PrecisionMode> PrecisionModeFromString(const std::string& value) {
    if (value == "automatic") return PrecisionMode::Automatic;
    if (value == "float32") return PrecisionMode::Float32;
    if (value == "float64") return PrecisionMode::Float64;
    if (value == "split-float") return PrecisionMode::SplitFloat;
    if (value == "perturbation") return PrecisionMode::Perturbation;
    if (value == "arbitrary-perturbation") return PrecisionMode::ArbitraryPrecisionPerturbation;
    return std::nullopt;
}

std::optional<StaticSlideshowOrder> StaticSlideshowOrderFromString(const std::string& value) {
    if (value == "sequential") return StaticSlideshowOrder::Sequential;
    if (value == "shuffle") return StaticSlideshowOrder::Shuffle;
    return std::nullopt;
}

std::optional<EquationUnaryTransform> EquationUnaryTransformFromString(const std::string& value) {
    if (value == "none") return EquationUnaryTransform::None;
    if (value == "sin") return EquationUnaryTransform::Sin;
    if (value == "cos") return EquationUnaryTransform::Cos;
    if (value == "exp") return EquationUnaryTransform::Exp;
    if (value == "log") return EquationUnaryTransform::Log;
    return std::nullopt;
}

std::optional<InitialZMode> InitialZModeFromString(const std::string& value) {
    if (value == "zero") return InitialZMode::Zero;
    if (value == "fixed") return InitialZMode::Fixed;
    if (value == "parameter") return InitialZMode::Parameter;
    if (value == "critical-point") return InitialZMode::CriticalPoint;
    return std::nullopt;
}

std::optional<FractalRenderMode> FractalRenderModeFromString(const std::string& value) {
    if (value == "escape-time") return FractalRenderMode::EscapeTime;
    if (value == "newton") return FractalRenderMode::Newton;
    return std::nullopt;
}

std::optional<ColouringMethod> ColouringMethodFromString(const std::string& value) {
    if (value == "smooth-escape") return ColouringMethod::SmoothEscape;
    if (value == "orbit-trap") return ColouringMethod::OrbitTrap;
    if (value == "distance-estimation") return ColouringMethod::DistanceEstimation;
    if (value == "newton-basins") return ColouringMethod::NewtonBasins;
    return std::nullopt;
}

std::optional<OrbitTrapType> OrbitTrapTypeFromString(const std::string& value) {
    if (value == "point") return OrbitTrapType::Point;
    if (value == "cross") return OrbitTrapType::Cross;
    if (value == "circle") return OrbitTrapType::Circle;
    return std::nullopt;
}

PerformanceSettings SettingsForProfile(PerformanceProfile profile) {
    PerformanceSettings settings;
    settings.profile = profile;
    switch (profile) {
    case PerformanceProfile::BatterySaver:
        settings.maximumFrameRate = 15;
        settings.renderScale = 0.5;
        settings.maximumIterations = 180;
        settings.antiAliasingLevel = 1;
        settings.pauseOnBattery = true;
        settings.reduceQualityOnBattery = true;
        break;
    case PerformanceProfile::Balanced:
        settings.maximumFrameRate = 30;
        settings.renderScale = 0.75;
        settings.maximumIterations = 350;
        settings.antiAliasingLevel = 1;
        settings.pauseOnBattery = false;
        settings.reduceQualityOnBattery = true;
        break;
    case PerformanceProfile::HighQuality:
        settings.maximumFrameRate = 60;
        settings.renderScale = 1.0;
        settings.maximumIterations = 700;
        settings.antiAliasingLevel = 2;
        settings.pauseOnBattery = false;
        settings.reduceQualityOnBattery = false;
        break;
    case PerformanceProfile::Custom:
        break;
    }
    return settings;
}


std::vector<std::string> EquationExampleNames() {
    return {
        "Classic Mandelbrot",
        "Cubic Multibrot",
        "Degree-5 Multibrot",
        "Burning Ship",
        "Conjugate Tricorn",
        "Linear z + 1.2c",
        "Rational z^3 + 0.25/z + c + 1.2",
        "Julia -0.8 + 0.156i",
        "Newton z^3 = 1",
        "Sine fractal",
        "Cosine fractal",
        "Exponential fractal",
        "Orbit-trap glow",
        "Distance-estimated depth",
        "Reference: Scaled c — z^2 + 1.2c",
        "Reference: Constant add — z^2 + c + 0.5",
        "Reference: Scaled z and c — 1.2z^2 + c",
        "Reference: Add to z — z^2 + 0.5z + c",
        "Reference: Swap z and c — z + c^2",
        "Reference: Absolute z — |z|^2 + c",
        "Reference: Minus c — z^2 - c",
        "Quartic Multibrot",
        "Sextic Multibrot",
        "Octic Multibrot",
        "Julia Dragon -0.835 - 0.2321i",
        "Julia Rabbit -0.123 + 0.745i",
        "Julia Dendrite 0 + 1i",
        "Julia Siegel -0.391 - 0.587i",
        "Newton z^4 = 1",
        "Newton z^5 = 1",
        "Newton z^7 = 1",
        "Logarithmic fractal",
        "Reciprocal quadratic — z^2 + c + 0.2/z",
        "Reciprocal cubic — z^3 + c + 0.15/z^2",
        "Cross orbit trap",
        "Point orbit trap",
        "Animated coefficients",
        "Iteration drift",
        "Parameter cubic plus z — z + c^3",
        "Complex scaled c — z^2 + (1+0.25i)c",
        "Complex scaled z — (0.8+0.2i)z^2 + c",
        "Negative linear feedback — z^2 - 0.35z + c",
        "Fixed initial z",
        "Initial z equals c",
        "Quartic critical-point map",
    };
}

EquationSettings EquationExample(std::size_t index) {
    EquationSettings equation;
    switch (index) {
    case 1:
        equation.power = 3;
        equation.initialZMode = InitialZMode::CriticalPoint;
        break;
    case 2:
        equation.power = 5;
        equation.initialZMode = InitialZMode::CriticalPoint;
        break;
    case 3:
        equation.absoluteReal = true;
        equation.absoluteImaginary = true;
        break;
    case 4:
        equation.conjugate = true;
        break;
    case 5:
        equation.quadratic = {0.0, 0.0};
        equation.linear = {1.0, 0.0};
        equation.parameter = {1.2, 0.0};
        break;
    case 6:
        equation.power = 3;
        equation.reciprocalPower = 1;
        equation.reciprocalCoefficient = {0.25, 0.0};
        equation.constant = {1.2, 0.0};
        equation.initialZMode = InitialZMode::CriticalPoint;
        break;
    case 7:
        equation.juliaMode = true;
        equation.juliaParameter = {-0.8, 0.156};
        break;
    case 8:
        equation.renderMode = FractalRenderMode::Newton;
        equation.newtonMode = true;
        equation.newtonDegree = 3;
        equation.newtonTarget = {1.0, 0.0};
        equation.colouringMethod = ColouringMethod::NewtonBasins;
        break;
    case 9:
        equation.quadratic = {0.85, 0.0};
        equation.power = 1;
        equation.unaryTransform = EquationUnaryTransform::Sin;
        break;
    case 10:
        equation.quadratic = {0.85, 0.0};
        equation.power = 1;
        equation.unaryTransform = EquationUnaryTransform::Cos;
        break;
    case 11:
        equation.quadratic = {0.35, 0.0};
        equation.power = 1;
        equation.unaryTransform = EquationUnaryTransform::Exp;
        equation.bailoutRadius = 8.0;
        break;
    case 12:
        equation.colouringMethod = ColouringMethod::OrbitTrap;
        equation.orbitTrap = OrbitTrapType::Circle;
        equation.orbitTrapRadius = 0.35;
        equation.glowStrength = 1.4;
        equation.depthStrength = 0.4;
        break;
    case 13:
        equation.colouringMethod = ColouringMethod::DistanceEstimation;
        equation.glowStrength = 0.65;
        equation.depthStrength = 1.25;
        break;
    case 14:
        equation.parameter = {1.2, 0.0};
        break;
    case 15:
        equation.constant = {0.5, 0.0};
        break;
    case 16:
        equation.quadratic = {1.2, 0.0};
        break;
    case 17:
        equation.linear = {0.5, 0.0};
        break;
    case 18:
        equation.quadratic = {0.0, 0.0};
        equation.linear = {1.0, 0.0};
        equation.parameterPower = 2;
        break;
    case 19:
        equation.absoluteReal = true;
        equation.absoluteImaginary = true;
        break;
    case 20:
        equation.parameter = {-1.0, 0.0};
        break;
    case 21:
        equation.power = 4;
        equation.initialZMode = InitialZMode::CriticalPoint;
        break;
    case 22:
        equation.power = 6;
        equation.initialZMode = InitialZMode::CriticalPoint;
        break;
    case 23:
        equation.power = 8;
        equation.initialZMode = InitialZMode::CriticalPoint;
        break;
    case 24:
        equation.juliaMode = true;
        equation.juliaParameter = {-0.835, -0.2321};
        break;
    case 25:
        equation.juliaMode = true;
        equation.juliaParameter = {-0.123, 0.745};
        break;
    case 26:
        equation.juliaMode = true;
        equation.juliaParameter = {0.0, 1.0};
        break;
    case 27:
        equation.juliaMode = true;
        equation.juliaParameter = {-0.391, -0.587};
        break;
    case 28:
    case 29:
    case 30:
        equation.renderMode = FractalRenderMode::Newton;
        equation.newtonMode = true;
        equation.newtonDegree = index == 28 ? 4 : (index == 29 ? 5 : 7);
        equation.newtonTarget = {1.0, 0.0};
        equation.colouringMethod = ColouringMethod::NewtonBasins;
        equation.glowStrength = 0.55;
        break;
    case 31:
        equation.quadratic = {0.58, 0.0};
        equation.power = 1;
        equation.unaryTransform = EquationUnaryTransform::Log;
        equation.bailoutRadius = 6.0;
        break;
    case 32:
        equation.reciprocalPower = 1;
        equation.reciprocalCoefficient = {0.2, 0.0};
        equation.initialZMode = InitialZMode::Fixed;
        equation.initialZ = {0.1, 0.0};
        break;
    case 33:
        equation.power = 3;
        equation.reciprocalPower = 2;
        equation.reciprocalCoefficient = {0.15, 0.0};
        equation.initialZMode = InitialZMode::Fixed;
        equation.initialZ = {0.2, 0.0};
        break;
    case 34:
        equation.colouringMethod = ColouringMethod::OrbitTrap;
        equation.orbitTrap = OrbitTrapType::Cross;
        equation.glowStrength = 1.15;
        equation.depthStrength = 0.35;
        break;
    case 35:
        equation.colouringMethod = ColouringMethod::OrbitTrap;
        equation.orbitTrap = OrbitTrapType::Point;
        equation.orbitTrapPoint = {-0.25, 0.0};
        equation.glowStrength = 1.0;
        break;
    case 36:
        equation.animateCoefficients = true;
        equation.coefficientAnimationSpeed = 0.16;
        equation.coefficientAnimationAmplitude = 0.12;
        equation.glowStrength = 0.5;
        break;
    case 37:
        equation.iterationTerm = {0.0015, -0.0008};
        equation.bailoutRadius = 8.0;
        break;
    case 38:
        equation.quadratic = {0.0, 0.0};
        equation.linear = {1.0, 0.0};
        equation.parameterPower = 3;
        break;
    case 39:
        equation.parameter = {1.0, 0.25};
        break;
    case 40:
        equation.quadratic = {0.8, 0.2};
        break;
    case 41:
        equation.linear = {-0.35, 0.0};
        break;
    case 42:
        equation.initialZMode = InitialZMode::Fixed;
        equation.initialZ = {0.25, 0.15};
        break;
    case 43:
        equation.initialZMode = InitialZMode::Parameter;
        break;
    case 44:
        equation.power = 4;
        equation.initialZMode = InitialZMode::CriticalPoint;
        equation.quadratic = {0.9, 0.0};
        break;
    default:
        break;
    }
    return equation;
}

std::string EquationSummary(const EquationSettings& equation) {
    std::ostringstream stream;
    if (equation.renderMode == FractalRenderMode::Newton) {
        stream << "Newton: z^" << equation.newtonDegree << " = " << FormatCoefficient(equation.newtonTarget);
        return stream.str();
    }
    stream << "z = ";
    const std::string transformed = [&]() {
        std::string name = "z";
        if (equation.swapRealImaginary) name = "swap(" + name + ")";
        if (equation.conjugate) name = "conj(" + name + ")";
        if (equation.absoluteReal || equation.absoluteImaginary) {
            name = "abs[" + std::string(equation.absoluteReal ? "Re" : "") +
                   std::string(equation.absoluteReal && equation.absoluteImaginary ? "," : "") +
                   std::string(equation.absoluteImaginary ? "Im" : "") + "](" + name + ")";
        }
        if (equation.unaryTransform != EquationUnaryTransform::None) name = ToString(equation.unaryTransform) + "(" + name + ")";
        return name;
    }();
    bool wrote = false;
    auto term = [&](const ComplexCoefficient& coefficient, const std::string& suffix) {
        if (IsZero(coefficient)) return;
        if (wrote) stream << " + ";
        stream << FormatCoefficient(coefficient);
        if (!suffix.empty()) { if (suffix.rfind("1/", 0) == 0) stream << "*"; stream << suffix; }
        wrote = true;
    };
    term(equation.quadratic, transformed + "^" + std::to_string(equation.power));
    term(equation.linear, transformed);
    term(equation.parameter, equation.parameterPower == 1 ? "c" : "c^" + std::to_string(equation.parameterPower));
    term(equation.constant, "");
    term(equation.iterationTerm, "n");
    if (equation.reciprocalPower > 0 && !IsZero(equation.reciprocalCoefficient)) {
        term(equation.reciprocalCoefficient, "1/(" + transformed + "^" + std::to_string(equation.reciprocalPower) + ")");
    }
    if (!wrote) stream << "0";
    if (equation.juliaMode) stream << " [Julia c=" << FormatCoefficient(equation.juliaParameter) << "]";
    return stream.str();
}

std::vector<Colour> PalettePreviewColours(Palette palette) {
    switch (palette) {
    case Palette::ClassicSpectrum:
        return {{1.0F, 0.12F, 0.12F, 1.0F}, {1.0F, 0.85F, 0.12F, 1.0F}, {0.12F, 1.0F, 0.35F, 1.0F},
                {0.10F, 0.65F, 1.0F, 1.0F}, {0.55F, 0.16F, 1.0F, 1.0F}, {1.0F, 0.12F, 0.75F, 1.0F}};
    case Palette::DeepOcean:
        return {{0.0F, 0.015F, 0.08F, 1.0F}, {0.0F, 0.12F, 0.35F, 1.0F}, {0.0F, 0.48F, 0.82F, 1.0F}, {0.25F, 0.88F, 1.0F, 1.0F}};
    case Palette::Fire:
        return {{0.06F, 0.0F, 0.0F, 1.0F}, {0.35F, 0.0F, 0.0F, 1.0F}, {0.95F, 0.18F, 0.0F, 1.0F}, {1.0F, 0.82F, 0.05F, 1.0F}};
    case Palette::PurpleNeon:
        return {{0.02F, 0.0F, 0.08F, 1.0F}, {0.25F, 0.02F, 0.55F, 1.0F}, {1.0F, 0.1F, 0.9F, 1.0F}, {0.25F, 0.75F, 1.0F, 1.0F}};
    case Palette::GreenMatrix:
        return {{0.0F, 0.0F, 0.0F, 1.0F}, {0.0F, 0.18F, 0.04F, 1.0F}, {0.1F, 1.0F, 0.25F, 1.0F}, {0.75F, 1.0F, 0.82F, 1.0F}};
    case Palette::Gold:
        return {{0.03F, 0.01F, 0.0F, 1.0F}, {0.28F, 0.12F, 0.0F, 1.0F}, {0.85F, 0.48F, 0.04F, 1.0F}, {1.0F, 0.88F, 0.35F, 1.0F}};
    case Palette::Ice:
        return {{0.01F, 0.08F, 0.12F, 1.0F}, {0.05F, 0.35F, 0.58F, 1.0F}, {0.45F, 0.88F, 1.0F, 1.0F}, {0.92F, 1.0F, 1.0F, 1.0F}};
    case Palette::Greyscale:
        return {{0.0F, 0.0F, 0.0F, 1.0F}, {0.33F, 0.33F, 0.33F, 1.0F}, {0.7F, 0.7F, 0.7F, 1.0F}, {1.0F, 1.0F, 1.0F, 1.0F}};
    case Palette::Pastel:
        return {{1.0F, 0.62F, 0.72F, 1.0F}, {1.0F, 0.86F, 0.62F, 1.0F}, {0.62F, 0.92F, 0.82F, 1.0F},
                {0.62F, 0.78F, 1.0F, 1.0F}, {0.82F, 0.66F, 1.0F, 1.0F}};
    case Palette::HighContrast:
        return {{0.01F, 0.01F, 0.01F, 1.0F}, {1.0F, 1.0F, 1.0F, 1.0F}, {0.05F, 0.05F, 0.05F, 1.0F}, {1.0F, 0.9F, 0.0F, 1.0F}};
    }
    return {};
}


std::vector<PalettePreset> BuiltInPalettePresets() {
    const auto colour = [](unsigned red, unsigned green, unsigned blue) {
        return Colour{static_cast<float>(red) / 255.0F,
                      static_cast<float>(green) / 255.0F,
                      static_cast<float>(blue) / 255.0F, 1.0F};
    };
    const auto make = [&](const char* id, const char* name,
                          std::initializer_list<std::array<unsigned, 3>> values) {
        PalettePreset preset;
        preset.id = id;
        preset.name = name;
        for (const auto& value : values) preset.colours.push_back(colour(value[0], value[1], value[2]));
        return preset;
    };
    return {
        make("reference-blue-gold", "Reference — Electric Blue and Gold", {{0,3,28},{0,24,92},{0,91,210},{55,185,255},{255,244,142},{255,151,18},{255,255,255}}),
        make("reference-cyan-aurora", "Reference — Cyan Aurora", {{0,7,18},{0,45,72},{0,142,168},{39,245,255},{206,255,228},{255,238,99}}),
        make("reference-magenta-nebula", "Reference — Magenta Nebula", {{10,0,22},{52,0,74},{142,0,173},{255,38,239},{255,175,250},{100,31,168}}),
        make("reference-golden-halo", "Reference — Golden Halo", {{8,4,0},{54,31,0},{154,93,0},{255,196,0},{255,248,135},{255,255,255}}),
        make("reference-deep-cyan", "Reference — Deep Cyan", {{0,7,12},{0,43,55},{0,117,137},{0,222,241},{117,255,244},{8,76,99}}),
        make("reference-crimson-web", "Reference — Crimson Web", {{10,0,2},{72,0,13},{170,0,34},{255,35,67},{255,150,165},{93,4,25}}),
        make("reference-ice-lightning", "Reference — Ice Lightning", {{0,3,23},{0,38,111},{0,116,255},{100,211,255},{235,253,255},{89,83,255}}),
        make("reference-toxic-green", "Reference — Toxic Green", {{1,12,0},{7,48,0},{32,126,0},{116,255,0},{225,255,105},{0,75,28}}),
        make("sunset-inferno", "Sunset Inferno", {{18,0,9},{92,0,33},{214,36,31},{255,111,20},{255,210,70},{255,246,201}}),
        make("abyssal-ocean", "Abyssal Ocean", {{0,2,14},{0,18,52},{0,70,110},{0,161,183},{105,238,225},{226,255,252}}),
        make("royal-amethyst", "Royal Amethyst", {{8,0,25},{48,9,93},{101,35,164},{183,90,255},{246,203,255},{67,36,133}}),
        make("neon-candy", "Neon Candy", {{14,0,35},{255,35,208},{45,242,255},{255,242,65},{93,255,134},{255,84,126}}),
        make("matrix-lime", "Matrix Lime", {{0,0,0},{0,34,7},{0,103,18},{20,220,50},{173,255,185},{7,61,15}}),
        make("amber-copper", "Amber Copper", {{11,4,0},{73,25,2},{151,62,11},{224,120,31},{255,202,91},{255,239,190}}),
        make("glacier", "Glacier", {{0,11,23},{5,48,76},{23,118,155},{91,205,226},{211,249,255},{255,255,255}}),
        make("silver-noir", "Silver Noir", {{0,0,0},{28,28,32},{79,82,91},{157,163,176},{229,232,240},{255,255,255}}),
        make("pastel-dream", "Pastel Dream", {{255,164,190},{255,216,158},{178,244,211},{154,215,255},{209,177,255},{255,190,238}}),
        make("high-contrast-yellow", "High Contrast Yellow", {{0,0,0},{255,255,255},{14,14,14},{255,225,0},{35,35,35},{255,255,255}}),
        make("teal-coral", "Teal and Coral", {{0,19,25},{0,92,100},{0,196,184},{255,220,166},{255,111,97},{159,31,55}}),
        make("vaporwave", "Vaporwave", {{12,0,43},{62,15,126},{193,49,255},{255,65,180},{35,231,255},{255,214,90}}),
        make("midnight-rose", "Midnight Rose", {{3,3,21},{27,9,57},{87,19,87},{176,47,104},{255,130,154},{255,223,218}}),
        make("solar-flare", "Solar Flare", {{0,0,0},{79,0,0},{201,20,0},{255,104,0},{255,226,35},{255,255,235}}),
        make("emerald-gold", "Emerald and Gold", {{0,10,7},{0,63,40},{0,159,91},{91,239,151},{255,221,74},{255,250,196}}),
        make("blue-magenta", "Blue Magenta", {{0,0,24},{0,45,158},{27,153,255},{142,46,255},{255,25,210},{255,185,245}}),
        make("blackbody", "Blackbody", {{0,0,0},{70,0,0},{180,14,0},{255,90,0},{255,214,50},{255,255,255}}),
        make("amethyst-ice", "Amethyst Ice", {{8,0,31},{62,25,117},{132,74,201},{107,187,255},{210,249,255},{255,255,255}}),
        make("arctic-fire", "Arctic Fire", {{0,9,29},{0,118,196},{109,231,255},{255,255,255},{255,149,51},{177,12,0}}),
        make("forest-mist", "Forest Mist", {{2,13,7},{12,55,31},{38,112,62},{111,174,108},{199,225,173},{238,244,220}}),
        make("retro-rainbow", "Retro Rainbow", {{30,5,65},{111,31,146},{240,55,146},{255,134,55},{255,226,77},{72,221,183},{52,135,255}}),
        make("peacock", "Peacock", {{0,13,24},{0,66,91},{0,149,137},{49,226,170},{74,116,255},{144,60,217},{255,190,78}}),
    };
}

ValidationResult ValidateAndNormalise(Preset& preset) {
    ValidationResult result;
    if (preset.id.empty() || preset.id.size() > 80) {
        AddIssue(result, "id", "Preset id must contain 1 to 80 characters.");
        preset.id = "custom-preset";
    }
    if (preset.name.empty() || preset.name.size() > 120) {
        AddIssue(result, "name", "Preset name must contain 1 to 120 characters.");
        preset.name = "Custom Preset";
    }
    const auto oldX = preset.camera.centreX;
    const auto oldY = preset.camera.centreY;
    preset.camera.centreX = ClampFinite(preset.camera.centreX, -4.0, 4.0, -0.5);
    preset.camera.centreY = ClampFinite(preset.camera.centreY, -4.0, 4.0, 0.0);
    if (oldX != preset.camera.centreX || oldY != preset.camera.centreY) {
        AddIssue(result, "camera", "Camera coordinates were outside the supported range.");
    }
    if (!std::isfinite(preset.camera.centreXLow)) {
        preset.camera.centreXLow = 0.0;
        AddIssue(result, "camera.centreXLow", "The compensated X coordinate was not finite.");
    }
    if (!std::isfinite(preset.camera.centreYLow)) {
        preset.camera.centreYLow = 0.0;
        AddIssue(result, "camera.centreYLow", "The compensated Y coordinate was not finite.");
    }
    const auto oldScale = preset.camera.scale;
    preset.camera.scale = ClampFinite(preset.camera.scale, 1.0e-32, 4.0, 1.5);
    if (oldScale != preset.camera.scale) AddIssue(result, "camera.scale", "Scale was outside the supported precision range.");
    preset.startingScale = ClampFinite(preset.startingScale, 1.0e-32, 4.0, preset.camera.scale);
    preset.maximumZoom = ClampFinite(preset.maximumZoom, 1.0, 1.0e30, 1.0e30);
    preset.zoomSpeed = ClampFinite(preset.zoomSpeed, 0.0, 2.0, 0.08);
    preset.maximumIterations = std::clamp(preset.maximumIterations, 32, 4096);
    auto clampCoefficient = [&](ComplexCoefficient& coefficient, const char* field) {
        const double oldReal = coefficient.real;
        const double oldImaginary = coefficient.imaginary;
        coefficient.real = ClampFinite(coefficient.real, -8.0, 8.0, 0.0);
        coefficient.imaginary = ClampFinite(coefficient.imaginary, -8.0, 8.0, 0.0);
        if (oldReal != coefficient.real || oldImaginary != coefficient.imaginary) {
            AddIssue(result, field, "Equation coefficients must be finite values between -8 and 8.");
        }
    };
    clampCoefficient(preset.equation.quadratic, "equation.quadratic");
    clampCoefficient(preset.equation.linear, "equation.linear");
    clampCoefficient(preset.equation.parameter, "equation.parameter");
    clampCoefficient(preset.equation.constant, "equation.constant");
    clampCoefficient(preset.equation.iterationTerm, "equation.iterationTerm");
    clampCoefficient(preset.equation.reciprocalCoefficient, "equation.reciprocalCoefficient");
    clampCoefficient(preset.equation.initialZ, "equation.initialZ");
    clampCoefficient(preset.equation.juliaParameter, "equation.juliaParameter");
    clampCoefficient(preset.equation.newtonTarget, "equation.newtonTarget");
    clampCoefficient(preset.equation.newtonRelaxation, "equation.newtonRelaxation");
    clampCoefficient(preset.equation.orbitTrapPoint, "equation.orbitTrapPoint");
    preset.equation.power = std::clamp(preset.equation.power, 1, 12);
    preset.equation.parameterPower = std::clamp(preset.equation.parameterPower, 1, 12);
    preset.equation.reciprocalPower = std::clamp(preset.equation.reciprocalPower, 0, 12);
    if (preset.equation.newtonMode) preset.equation.renderMode = FractalRenderMode::Newton;
    preset.equation.newtonMode = preset.equation.renderMode == FractalRenderMode::Newton;
    preset.equation.newtonDegree = std::clamp(preset.equation.newtonDegree, 2, 12);
    preset.equation.bailoutRadius = ClampFinite(preset.equation.bailoutRadius, 1.01, 1.0e6, 2.0);
    preset.equation.convergenceTolerance = ClampFinite(preset.equation.convergenceTolerance, 1.0e-12, 0.1, 1.0e-6);
    preset.equation.orbitTrapRadius = ClampFinite(preset.equation.orbitTrapRadius, 1.0e-6, 8.0, 0.5);
    preset.equation.glowStrength = ClampFinite(preset.equation.glowStrength, 0.0, 4.0, 0.0);
    preset.equation.depthStrength = ClampFinite(preset.equation.depthStrength, 0.0, 4.0, 0.0);
    preset.equation.coefficientAnimationSpeed = ClampFinite(preset.equation.coefficientAnimationSpeed, 0.0, 8.0, 0.25);
    preset.equation.coefficientAnimationAmplitude = ClampFinite(preset.equation.coefficientAnimationAmplitude, 0.0, 2.0, 0.0);
    preset.colourOffset = ClampFinite(preset.colourOffset, -1000.0, 1000.0, 0.0);
    preset.colourCycleSpeed = ClampFinite(preset.colourCycleSpeed, -0.25, 0.25, 0.02);
    preset.brightness = ClampFinite(preset.brightness, 0.1, 2.5, 1.0);
    preset.contrast = ClampFinite(preset.contrast, 0.1, 3.0, 1.0);
    preset.saturation = ClampFinite(preset.saturation, 0.0, 2.0, 1.0);
    preset.frameRateLimit = std::clamp(preset.frameRateLimit, 5, 240);
    preset.renderScale = ClampFinite(preset.renderScale, 0.25, 1.0, 0.75);
    preset.antiAliasingLevel = std::clamp(preset.antiAliasingLevel, 1, 4);
    if (preset.automaticJourneyWaypoints.size() > 32768U ||
        preset.automaticJourneyWaypoints.find('\0') != std::string::npos) {
        AddIssue(result, "automaticJourneyWaypoints", "Automatic Journey waypoint text was invalid or exceeded 32768 bytes.");
        preset.automaticJourneyWaypoints.resize(std::min<std::size_t>(preset.automaticJourneyWaypoints.size(), 32768U));
        preset.automaticJourneyWaypoints.erase(
            std::remove(preset.automaticJourneyWaypoints.begin(), preset.automaticJourneyWaypoints.end(), '\0'),
            preset.automaticJourneyWaypoints.end());
    }
    auto clampColour = [](Colour& colour) {
        colour.r = std::clamp(colour.r, 0.0F, 1.0F);
        colour.g = std::clamp(colour.g, 0.0F, 1.0F);
        colour.b = std::clamp(colour.b, 0.0F, 1.0F);
        colour.a = std::clamp(colour.a, 0.0F, 1.0F);
    };
    if (preset.customPaletteColours.size() > 4096) {
        AddIssue(result, "customPaletteColours", "Custom palettes are limited to 4096 colour stops for safe local storage and rendering.");
        preset.customPaletteColours.resize(4096);
    }
    for (auto& colour : preset.customPaletteColours) clampColour(colour);
    if (preset.customPaletteColours.size() == 1) {
        preset.customPaletteColours.push_back(preset.customPaletteColours.front());
    }
    clampColour(preset.interiorColour);
    clampColour(preset.backgroundColour);
    return result;
}

ValidationResult ValidateAndNormalise(PalettePreset& preset) {
    ValidationResult result;
    if (preset.id.empty() || preset.id.size() > 80) {
        AddIssue(result, "id", "Palette preset id must contain 1 to 80 characters.");
        preset.id = "custom-palette";
    }
    if (preset.name.empty() || preset.name.size() > 120) {
        AddIssue(result, "name", "Palette preset name must contain 1 to 120 characters.");
        preset.name = "Custom Palette";
    }
    if (preset.colours.size() > 4096) {
        AddIssue(result, "colours", "Palette presets are limited to 4096 colour stops.");
        preset.colours.resize(4096);
    }
    auto clampColour = [](Colour& colour) {
        colour.r = std::clamp(colour.r, 0.0F, 1.0F);
        colour.g = std::clamp(colour.g, 0.0F, 1.0F);
        colour.b = std::clamp(colour.b, 0.0F, 1.0F);
        colour.a = std::clamp(colour.a, 0.0F, 1.0F);
    };
    for (auto& colour : preset.colours) clampColour(colour);
    if (preset.colours.size() < 2) {
        AddIssue(result, "colours", "A saved palette requires at least two colour stops.");
        if (preset.colours.empty()) preset.colours = PalettePreviewColours(Palette::ClassicSpectrum);
        else preset.colours.push_back(preset.colours.front());
    }
    return result;
}

ValidationResult ValidateAndNormalise(EquationPreset& preset) {
    ValidationResult result;
    if (preset.id.empty() || preset.id.size() > 80) {
        AddIssue(result, "id", "Equation preset id must contain 1 to 80 characters.");
        preset.id = "custom-equation";
    }
    if (preset.name.empty() || preset.name.size() > 120) {
        AddIssue(result, "name", "Equation preset name must contain 1 to 120 characters.");
        preset.name = "Custom Equation";
    }
    Preset wrapper;
    wrapper.id = "equation-validation";
    wrapper.name = "Equation Validation";
    wrapper.equation = preset.equation;
    const auto wrapped = ValidateAndNormalise(wrapper);
    preset.equation = wrapper.equation;
    for (const auto& issue : wrapped.issues) {
        if (issue.field.rfind("equation.", 0) == 0) AddIssue(result, issue.field, issue.message);
    }
    return result;
}

ValidationResult ValidateAndNormalise(AppSettings& settings) {
    ValidationResult result;
    if (settings.schemaVersion < 1 || settings.schemaVersion > 8) {
        AddIssue(result, "schemaVersion", "Unsupported settings version; safe defaults were applied where necessary.");
    }
    settings.schemaVersion = 8;
    settings.performance.maximumFrameRate = std::clamp(settings.performance.maximumFrameRate, 5, 240);
    settings.performance.renderScale = ClampFinite(settings.performance.renderScale, 0.25, 1.0, 0.75);
    settings.performance.maximumIterations = std::clamp(settings.performance.maximumIterations, 32, 4096);
    settings.performance.antiAliasingLevel = std::clamp(settings.performance.antiAliasingLevel, 1, 4);
    settings.performance.resumeDelayMs = std::clamp(settings.performance.resumeDelayMs, 0, 30000);
    settings.performance.precision.arbitraryPrecisionBits =
        std::clamp(settings.performance.precision.arbitraryPrecisionBits, 128, 512);
    settings.performance.precision.arbitraryPrecisionBits =
        ((settings.performance.precision.arbitraryPrecisionBits + 15) / 16) * 16;
    settings.performance.adaptive.minimumFramesPerSecond =
        ClampFinite(settings.performance.adaptive.minimumFramesPerSecond, 1.0, 240.0, 8.0);
    settings.performance.adaptive.lowFpsSustainMs =
        std::clamp(settings.performance.adaptive.lowFpsSustainMs, 1000, 60000);
    settings.performance.adaptive.maximumProcessCpuPercent =
        ClampFinite(settings.performance.adaptive.maximumProcessCpuPercent, 1.0, 100.0, 65.0);
    settings.performance.adaptive.highCpuSustainMs =
        std::clamp(settings.performance.adaptive.highCpuSustainMs, 1000, 60000);
    settings.performance.adaptive.maximumWorkingSetMb =
        std::clamp(settings.performance.adaptive.maximumWorkingSetMb, 128, 32768);
    settings.performance.adaptive.highMemorySustainMs =
        std::clamp(settings.performance.adaptive.highMemorySustainMs, 1000, 60000);
    settings.performance.adaptive.resumeStableMs =
        std::clamp(settings.performance.adaptive.resumeStableMs, 1000, 120000);
    settings.performance.adaptive.minimumVisiblePixelChange =
        ClampFinite(settings.performance.adaptive.minimumVisiblePixelChange, 0.01, 4.0, 0.25);
    settings.performance.adaptive.minimumVisibleColourChange =
        ClampFinite(settings.performance.adaptive.minimumVisibleColourChange, 0.00001, 0.25, 0.001);
    settings.staticWallpaper.cycleSeconds = std::clamp(settings.staticWallpaper.cycleSeconds, 10, 86400);
    if (settings.staticWallpaper.storageDirectory.size() > 32768 ||
        settings.staticWallpaper.storageDirectory.find('\0') != std::string::npos) {
        AddIssue(result, "staticWallpaper.storageDirectory", "The slideshow storage folder was invalid and has been reset.");
        settings.staticWallpaper.storageDirectory.clear();
    }
    if (settings.staticWallpaper.imagePaths.size() > 512) {
        AddIssue(result, "staticWallpaper.imagePaths", "The slideshow was limited to 512 images.");
        settings.staticWallpaper.imagePaths.resize(512);
    }
    settings.staticWallpaper.imagePaths.erase(
        std::remove_if(settings.staticWallpaper.imagePaths.begin(), settings.staticWallpaper.imagePaths.end(),
            [](const std::string& path) {
                return path.empty() || path.size() > 32768 || path.find('\0') != std::string::npos;
            }),
        settings.staticWallpaper.imagePaths.end());
    std::vector<std::string> uniqueImages;
    uniqueImages.reserve(settings.staticWallpaper.imagePaths.size());
    for (const auto& path : settings.staticWallpaper.imagePaths) {
        if (std::find(uniqueImages.begin(), uniqueImages.end(), path) == uniqueImages.end()) {
            uniqueImages.push_back(path);
        }
    }
    settings.staticWallpaper.imagePaths = std::move(uniqueImages);
    if (settings.staticWallpaper.imagePaths.empty()) {
        settings.staticWallpaper.enabled = false;
        settings.staticWallpaper.currentIndex = 0;
    } else {
        settings.staticWallpaper.currentIndex = std::clamp(
            settings.staticWallpaper.currentIndex, 0,
            static_cast<int>(settings.staticWallpaper.imagePaths.size()) - 1);
    }
    if (settings.selectedPresetId.empty()) settings.selectedPresetId = "full-view";
    for (auto& preset : settings.customPresets) {
        const auto presetResult = ValidateAndNormalise(preset);
        if (!presetResult.valid) {
            result.valid = false;
            result.issues.insert(result.issues.end(), presetResult.issues.begin(), presetResult.issues.end());
        }
        preset.builtIn = false;
    }
    if (settings.customPalettePresets.size() > 256) {
        AddIssue(result, "customPalettePresets", "Saved palette presets were limited to 256 entries.");
        settings.customPalettePresets.resize(256);
    }
    for (auto& palettePreset : settings.customPalettePresets) {
        const auto paletteResult = ValidateAndNormalise(palettePreset);
        if (!paletteResult.valid) {
            result.valid = false;
            result.issues.insert(result.issues.end(), paletteResult.issues.begin(), paletteResult.issues.end());
        }
    }
    if (settings.customEquationPresets.size() > 256) {
        AddIssue(result, "customEquationPresets", "Saved equation presets were limited to 256 entries.");
        settings.customEquationPresets.resize(256);
    }
    for (auto& equationPreset : settings.customEquationPresets) {
        const auto equationResult = ValidateAndNormalise(equationPreset);
        if (!equationResult.valid) {
            result.valid = false;
            result.issues.insert(result.issues.end(), equationResult.issues.begin(), equationResult.issues.end());
        }
    }
    return result;
}

std::vector<Preset> BuiltInPresets() {
    auto make = [](std::string id, std::string name, double x, double y, double scale,
                   int iterations, Palette palette, AnimationMode mode, double zoomSpeed,
                   double colourSpeed, int fps, double renderScale) {
        Preset preset;
        preset.id = std::move(id);
        preset.name = std::move(name);
        preset.builtIn = true;
        preset.camera = {x, y, scale};
        preset.startingScale = scale;
        preset.maximumIterations = iterations;
        preset.palette = palette;
        preset.animationMode = mode;
        preset.zoomSpeed = zoomSpeed;
        preset.colourCycleSpeed = colourSpeed;
        preset.frameRateLimit = fps;
        preset.renderScale = renderScale;
        return preset;
    };

    std::vector<Preset> presets{
        make("full-view", "Full Mandelbrot View", -0.5, 0.0, 1.5, 280, Palette::ClassicSpectrum, AnimationMode::AutomaticJourney, 0.055, 0.018, 30, 0.75),
        make("seahorse-valley", "Seahorse Valley", -0.743643887037151, 0.131825904205330, 0.0065, 650, Palette::DeepOcean, AnimationMode::ContinuousZoom, 0.045, 0.010, 30, 0.75),
        make("elephant-valley", "Elephant Valley", 0.285, 0.01, 0.045, 520, Palette::Gold, AnimationMode::ContinuousZoom, 0.035, 0.012, 30, 0.75),
        make("double-spiral", "Double Spiral", -0.777807810193171, 0.131645108003206, 0.00085, 820, Palette::PurpleNeon, AnimationMode::StaticAnimatedColour, 0.0, 0.022, 30, 0.75),
        make("mini-mandelbrot", "Mini Mandelbrot", -1.25066, 0.02012, 0.012, 650, Palette::ClassicSpectrum, AnimationMode::ContinuousZoom, 0.035, 0.013, 30, 0.75),
        make("deep-blue", "Deep Blue", -0.7453, 0.1127, 0.025, 560, Palette::DeepOcean, AnimationMode::StaticAnimatedColour, 0.0, 0.010, 30, 0.75),
        make("fire", "Fire", -0.16, 1.0405, 0.015, 640, Palette::Fire, AnimationMode::StaticAnimatedColour, 0.0, 0.016, 30, 0.75),
        make("monochrome", "Monochrome", -0.5, 0.0, 1.45, 350, Palette::Greyscale, AnimationMode::AutomaticJourney, 0.04, 0.004, 24, 0.75),
        make("neon", "Neon", -0.743, 0.131, 0.018, 620, Palette::PurpleNeon, AnimationMode::ContinuousZoom, 0.05, 0.024, 30, 0.75),
        make("slow-ambient", "Slow Ambient Zoom", -0.75, 0.0, 0.9, 420, Palette::Pastel, AnimationMode::AutomaticJourney, 0.018, 0.004, 24, 0.75),
    };

    const auto palettes = BuiltInPalettePresets();
    const auto palette = [&](const std::string& id) -> std::vector<Colour> {
        const auto found = std::find_if(palettes.begin(), palettes.end(),
                                        [&](const PalettePreset& item) { return item.id == id; });
        return found == palettes.end() ? PalettePreviewColours(Palette::ClassicSpectrum) : found->colours;
    };
    const auto addScene = [&](const char* id, const char* name, std::size_t equationIndex,
                              const char* paletteId, double centreX, double centreY,
                              double scale, int iterations, AnimationMode mode,
                              double zoomSpeed, double colourSpeed, double glow = -1.0) {
        Preset scene = make(id, name, centreX, centreY, scale, iterations,
                            Palette::ClassicSpectrum, mode, zoomSpeed, colourSpeed, 30, 0.85);
        scene.equation = EquationExample(equationIndex);
        if (glow >= 0.0) scene.equation.glowStrength = glow;
        scene.customPaletteColours = palette(paletteId);
        scene.brightness = 1.05;
        scene.contrast = 1.08;
        scene.saturation = 1.12;
        ValidateAndNormalise(scene);
        presets.push_back(std::move(scene));
    };

    // Reference-image equation and palette combinations.
    addScene("reference-classic-blue-gold", "Reference — Classic Blue Gold", 0, "reference-blue-gold", -0.5, 0.0, 1.5, 420, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.65);
    addScene("reference-scaled-c-cyan", "Reference — Scaled c Cyan", 14, "reference-cyan-aurora", -0.5, 0.0, 1.5, 460, AnimationMode::StaticAnimatedColour, 0.0, 0.014, 0.75);
    addScene("reference-constant-magenta", "Reference — Constant Add Magenta", 15, "reference-magenta-nebula", -0.35, 0.0, 1.65, 520, AnimationMode::StaticAnimatedColour, 0.0, 0.016, 0.8);
    addScene("reference-scaled-z-gold", "Reference — Scaled z Gold", 16, "reference-golden-halo", -0.3, 0.0, 1.45, 520, AnimationMode::StaticAnimatedColour, 0.0, 0.010, 0.95);
    addScene("reference-linear-cyan", "Reference — Add to z Cyan", 17, "reference-deep-cyan", -0.5, 0.0, 1.7, 520, AnimationMode::StaticAnimatedColour, 0.0, 0.013, 0.85);
    addScene("reference-swap-crimson", "Reference — Swap z and c Crimson", 18, "reference-crimson-web", 0.0, 0.0, 1.7, 520, AnimationMode::StaticAnimatedColour, 0.0, 0.014, 0.65);
    addScene("reference-absolute-ice", "Reference — Absolute z Ice", 19, "reference-ice-lightning", -0.5, -0.45, 1.65, 520, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.95);
    addScene("reference-minus-c-green", "Reference — Minus c Toxic Green", 20, "reference-toxic-green", 0.5, 0.0, 1.5, 520, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.8);

    // Additional equation-library scenes.
    addScene("cubic-aurora", "Cubic Aurora", 1, "reference-cyan-aurora", 0.0, 0.0, 1.45, 540, AnimationMode::AutomaticJourney, 0.035, 0.012, 0.55);
    addScene("quartic-amethyst", "Quartic Amethyst", 21, "royal-amethyst", 0.0, 0.0, 1.35, 580, AnimationMode::ContinuousZoom, 0.03, 0.014, 0.7);
    addScene("quintic-nebula", "Quintic Nebula", 2, "blue-magenta", 0.0, 0.0, 1.25, 620, AnimationMode::ContinuousZoom, 0.028, 0.016, 0.75);
    addScene("octic-rainbow", "Octic Retro Rainbow", 23, "retro-rainbow", 0.0, 0.0, 1.12, 720, AnimationMode::StaticAnimatedColour, 0.0, 0.018, 0.55);
    addScene("burning-ship-inferno", "Burning Ship Inferno", 3, "sunset-inferno", -0.45, -0.55, 1.45, 620, AnimationMode::ContinuousZoom, 0.03, 0.012, 0.9);
    addScene("tricorn-arctic", "Tricorn Arctic Fire", 4, "arctic-fire", 0.0, 0.0, 1.7, 580, AnimationMode::AutomaticJourney, 0.03, 0.01, 0.65);
    addScene("julia-dragon-vaporwave", "Julia Dragon Vaporwave", 24, "vaporwave", 0.0, 0.0, 1.45, 620, AnimationMode::StaticAnimatedColour, 0.0, 0.018, 0.7);
    addScene("julia-rabbit-candy", "Julia Rabbit Neon Candy", 25, "neon-candy", 0.0, 0.0, 1.35, 620, AnimationMode::StaticAnimatedColour, 0.0, 0.02, 0.65);
    addScene("julia-dendrite-glacier", "Julia Dendrite Glacier", 26, "glacier", 0.0, 0.0, 1.35, 680, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.75);
    addScene("newton-three-royal", "Newton Three Royal", 8, "royal-amethyst", 0.0, 0.0, 1.65, 160, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.8);
    addScene("newton-five-solar", "Newton Five Solar", 29, "solar-flare", 0.0, 0.0, 1.65, 180, AnimationMode::StaticAnimatedColour, 0.0, 0.014, 0.8);
    addScene("rational-copper", "Rational Copper", 6, "amber-copper", 0.0, 0.0, 2.0, 480, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.65);
    addScene("sine-abyss", "Sine Abyss", 9, "abyssal-ocean", 0.0, 0.0, 2.2, 420, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.55);
    addScene("cosine-midnight", "Cosine Midnight Rose", 10, "midnight-rose", 0.0, 0.0, 2.2, 420, AnimationMode::StaticAnimatedColour, 0.0, 0.014, 0.55);
    addScene("exponential-emerald", "Exponential Emerald", 11, "emerald-gold", 0.0, 0.0, 2.0, 360, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.65);
    addScene("orbit-cross-peacock", "Orbit Cross Peacock", 34, "peacock", -0.5, 0.0, 1.5, 520, AnimationMode::StaticAnimatedColour, 0.0, 0.016, 1.15);
    addScene("distance-glacier", "Distance Depth Glacier", 13, "glacier", -0.743, 0.131, 0.03, 720, AnimationMode::ContinuousZoom, 0.025, 0.008, 0.65);
    addScene("animated-rainbow", "Animated Coefficient Rainbow", 36, "retro-rainbow", -0.5, 0.0, 1.5, 480, AnimationMode::StaticAnimatedColour, 0.0, 0.012, 0.75);

    return presets;
}

} // namespace mw
