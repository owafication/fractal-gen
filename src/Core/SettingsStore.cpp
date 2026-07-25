#include "Core/SettingsStore.h"

#include "Core/Json.h"

#include <chrono>
#include <fstream>
#include <sstream>

namespace mw {
namespace {

using json::Value;


Value ComplexCoefficientToJson(const ComplexCoefficient& coefficient) {
    return Value::Object{{"real", coefficient.real}, {"imaginary", coefficient.imaginary}};
}

bool ComplexCoefficientFromJson(const Value* value, ComplexCoefficient& coefficient, std::string& error) {
    if (!value) return true;
    if (!value->IsObject()) {
        error = "Equation coefficients must be JSON objects.";
        return false;
    }
    if (const auto* child = value->Find("real")) {
        if (!child->IsNumber()) { error = "Equation coefficient real parts must be numbers."; return false; }
        coefficient.real = child->AsNumber(coefficient.real);
    }
    if (const auto* child = value->Find("imaginary")) {
        if (!child->IsNumber()) { error = "Equation coefficient imaginary parts must be numbers."; return false; }
        coefficient.imaginary = child->AsNumber(coefficient.imaginary);
    }
    return true;
}

Value EquationToJson(const EquationSettings& equation) {
    return Value::Object{
        {"quadratic", ComplexCoefficientToJson(equation.quadratic)},
        {"linear", ComplexCoefficientToJson(equation.linear)},
        {"parameter", ComplexCoefficientToJson(equation.parameter)},
        {"constant", ComplexCoefficientToJson(equation.constant)},
        {"iterationTerm", ComplexCoefficientToJson(equation.iterationTerm)},
        {"reciprocalCoefficient", ComplexCoefficientToJson(equation.reciprocalCoefficient)},
        {"power", equation.power},
        {"parameterPower", equation.parameterPower},
        {"reciprocalPower", equation.reciprocalPower},
        {"absoluteReal", equation.absoluteReal},
        {"absoluteImaginary", equation.absoluteImaginary},
        {"conjugate", equation.conjugate},
        {"swapRealImaginary", equation.swapRealImaginary},
        {"transform", ToString(equation.unaryTransform)},
        {"initialZMode", ToString(equation.initialZMode)},
        {"initialZ", ComplexCoefficientToJson(equation.initialZ)},
        {"juliaMode", equation.juliaMode},
        {"juliaParameter", ComplexCoefficientToJson(equation.juliaParameter)},
        {"bailoutRadius", equation.bailoutRadius},
        {"renderMode", ToString(equation.renderMode)},
        {"newtonMode", equation.newtonMode},
        {"newtonDegree", equation.newtonDegree},
        {"newtonTarget", ComplexCoefficientToJson(equation.newtonTarget)},
        {"newtonRelaxation", ComplexCoefficientToJson(equation.newtonRelaxation)},
        {"convergenceTolerance", equation.convergenceTolerance},
        {"colouringMethod", ToString(equation.colouringMethod)},
        {"orbitTrap", ToString(equation.orbitTrap)},
        {"orbitTrapPoint", ComplexCoefficientToJson(equation.orbitTrapPoint)},
        {"orbitTrapRadius", equation.orbitTrapRadius},
        {"glowStrength", equation.glowStrength},
        {"depthStrength", equation.depthStrength},
        {"animateCoefficients", equation.animateCoefficients},
        {"coefficientAnimationSpeed", equation.coefficientAnimationSpeed},
        {"coefficientAnimationAmplitude", equation.coefficientAnimationAmplitude},
    };
}

bool EquationFromJson(const Value* value, EquationSettings& equation, std::string& error) {
    if (!value) return true; // Older presets use the classic Mandelbrot recurrence.
    if (!value->IsObject()) {
        error = "Equation settings must be a JSON object.";
        return false;
    }
    if (!ComplexCoefficientFromJson(value->Find("quadratic"), equation.quadratic, error) ||
        !ComplexCoefficientFromJson(value->Find("linear"), equation.linear, error) ||
        !ComplexCoefficientFromJson(value->Find("parameter"), equation.parameter, error) ||
        !ComplexCoefficientFromJson(value->Find("constant"), equation.constant, error) ||
        !ComplexCoefficientFromJson(value->Find("iterationTerm"), equation.iterationTerm, error) ||
        !ComplexCoefficientFromJson(value->Find("reciprocalCoefficient"), equation.reciprocalCoefficient, error) ||
        !ComplexCoefficientFromJson(value->Find("initialZ"), equation.initialZ, error) ||
        !ComplexCoefficientFromJson(value->Find("juliaParameter"), equation.juliaParameter, error) ||
        !ComplexCoefficientFromJson(value->Find("newtonTarget"), equation.newtonTarget, error) ||
        !ComplexCoefficientFromJson(value->Find("newtonRelaxation"), equation.newtonRelaxation, error) ||
        !ComplexCoefficientFromJson(value->Find("orbitTrapPoint"), equation.orbitTrapPoint, error)) {
        return false;
    }
    if (const auto* child = value->Find("absoluteReal")) {
        if (!child->IsBool()) { error = "Equation absolute-value flags must be booleans."; return false; }
        equation.absoluteReal = child->AsBool(equation.absoluteReal);
    }
    if (const auto* child = value->Find("absoluteImaginary")) {
        if (!child->IsBool()) { error = "Equation absolute-value flags must be booleans."; return false; }
        equation.absoluteImaginary = child->AsBool(equation.absoluteImaginary);
    }
    if (const auto* child = value->Find("power")) equation.power = child->AsInt(equation.power);
    if (const auto* child = value->Find("parameterPower")) equation.parameterPower = child->AsInt(equation.parameterPower);
    if (const auto* child = value->Find("reciprocalPower")) equation.reciprocalPower = child->AsInt(equation.reciprocalPower);
    if (const auto* child = value->Find("conjugate")) equation.conjugate = child->AsBool(equation.conjugate);
    if (const auto* child = value->Find("swapRealImaginary")) equation.swapRealImaginary = child->AsBool(equation.swapRealImaginary);
    if (const auto* child = value->Find("transform")) {
        const auto parsed = EquationUnaryTransformFromString(child->AsString());
        if (!parsed) { error = "Equation contains an unsupported transform."; return false; }
        equation.unaryTransform = *parsed;
    }
    if (const auto* child = value->Find("initialZMode")) {
        const auto parsed = InitialZModeFromString(child->AsString());
        if (!parsed) { error = "Equation contains an unsupported initial z mode."; return false; }
        equation.initialZMode = *parsed;
    }
    if (const auto* child = value->Find("juliaMode")) equation.juliaMode = child->AsBool(equation.juliaMode);
    if (const auto* child = value->Find("bailoutRadius")) equation.bailoutRadius = child->AsNumber(equation.bailoutRadius);
    if (const auto* child = value->Find("renderMode")) {
        const auto parsed = FractalRenderModeFromString(child->AsString());
        if (!parsed) { error = "Equation contains an unsupported render mode."; return false; }
        equation.renderMode = *parsed;
    }
    if (const auto* child = value->Find("newtonMode")) equation.newtonMode = child->AsBool(equation.newtonMode);
    if (const auto* child = value->Find("newtonDegree")) equation.newtonDegree = child->AsInt(equation.newtonDegree);
    if (const auto* child = value->Find("convergenceTolerance")) equation.convergenceTolerance = child->AsNumber(equation.convergenceTolerance);
    if (const auto* child = value->Find("colouringMethod")) {
        const auto parsed = ColouringMethodFromString(child->AsString());
        if (!parsed) { error = "Equation contains an unsupported colouring method."; return false; }
        equation.colouringMethod = *parsed;
    }
    if (const auto* child = value->Find("orbitTrap")) {
        const auto parsed = OrbitTrapTypeFromString(child->AsString());
        if (!parsed) { error = "Equation contains an unsupported orbit trap."; return false; }
        equation.orbitTrap = *parsed;
    }
    if (const auto* child = value->Find("orbitTrapRadius")) equation.orbitTrapRadius = child->AsNumber(equation.orbitTrapRadius);
    if (const auto* child = value->Find("glowStrength")) equation.glowStrength = child->AsNumber(equation.glowStrength);
    if (const auto* child = value->Find("depthStrength")) equation.depthStrength = child->AsNumber(equation.depthStrength);
    if (const auto* child = value->Find("animateCoefficients")) equation.animateCoefficients = child->AsBool(equation.animateCoefficients);
    if (const auto* child = value->Find("coefficientAnimationSpeed")) equation.coefficientAnimationSpeed = child->AsNumber(equation.coefficientAnimationSpeed);
    if (const auto* child = value->Find("coefficientAnimationAmplitude")) equation.coefficientAnimationAmplitude = child->AsNumber(equation.coefficientAnimationAmplitude);
    return true;
}

Value ColourToJson(const Colour& colour) {
    return Value::Object{{"r", colour.r}, {"g", colour.g}, {"b", colour.b}, {"a", colour.a}};
}

Colour ColourFromJson(const Value* value, const Colour& fallback) {
    if (!value || !value->IsObject()) return fallback;
    Colour colour = fallback;
    if (const auto* child = value->Find("r")) colour.r = static_cast<float>(child->AsNumber(colour.r));
    if (const auto* child = value->Find("g")) colour.g = static_cast<float>(child->AsNumber(colour.g));
    if (const auto* child = value->Find("b")) colour.b = static_cast<float>(child->AsNumber(colour.b));
    if (const auto* child = value->Find("a")) colour.a = static_cast<float>(child->AsNumber(colour.a));
    return colour;
}

Value PaletteColoursToJson(const std::vector<Colour>& colours) {
    Value::Array values;
    values.reserve(colours.size());
    for (const auto& colour : colours) values.push_back(ColourToJson(colour));
    return values;
}

bool PaletteColoursFromJson(const Value* value, std::vector<Colour>& colours, std::string& error) {
    if (!value) return true;
    if (!value->IsArray()) {
        error = "Custom palette colours must be an array.";
        return false;
    }
    if (value->AsArray().size() > 4096) {
        error = "Custom palettes may contain at most 4096 colour stops.";
        return false;
    }
    colours.clear();
    colours.reserve(value->AsArray().size());
    for (const auto& item : value->AsArray()) {
        if (!item.IsObject()) {
            error = "Every custom palette entry must be a colour object.";
            return false;
        }
        colours.push_back(ColourFromJson(&item, Colour{}));
    }
    return true;
}

Value PalettePresetToJson(const PalettePreset& preset) {
    return Value::Object{
        {"id", preset.id},
        {"name", preset.name},
        {"colours", PaletteColoursToJson(preset.colours)},
    };
}

std::optional<PalettePreset> PalettePresetFromJson(const Value& root, std::string& error) {
    if (!root.IsObject()) {
        error = "Palette preset must be a JSON object.";
        return std::nullopt;
    }
    const auto* id = root.Find("id");
    const auto* name = root.Find("name");
    if (!id || !id->IsString() || !name || !name->IsString()) {
        error = "Palette preset is missing its id or name.";
        return std::nullopt;
    }
    PalettePreset preset;
    preset.id = id->AsString();
    preset.name = name->AsString();
    if (!PaletteColoursFromJson(root.Find("colours"), preset.colours, error)) return std::nullopt;
    const auto validation = ValidateAndNormalise(preset);
    if (!validation.valid) {
        error = "Palette preset values are invalid.";
        return std::nullopt;
    }
    return preset;
}

Value EquationPresetToJson(const EquationPreset& preset) {
    return Value::Object{
        {"id", preset.id},
        {"name", preset.name},
        {"equation", EquationToJson(preset.equation)},
    };
}

std::optional<EquationPreset> EquationPresetFromJson(const Value& root, std::string& error) {
    if (!root.IsObject()) { error = "Equation preset must be a JSON object."; return std::nullopt; }
    const auto* id = root.Find("id");
    const auto* name = root.Find("name");
    if (!id || !id->IsString() || !name || !name->IsString()) {
        error = "Equation preset is missing its id or name.";
        return std::nullopt;
    }
    EquationPreset preset;
    preset.id = id->AsString();
    preset.name = name->AsString();
    if (!EquationFromJson(root.Find("equation"), preset.equation, error)) return std::nullopt;
    const auto validation = ValidateAndNormalise(preset);
    if (!validation.valid) { error = "Equation preset values are invalid."; return std::nullopt; }
    return preset;
}

Value PresetToJson(const Preset& preset) {
    return Value::Object{
        {"schemaVersion", 2},
        {"id", preset.id},
        {"name", preset.name},
        {"builtIn", preset.builtIn},
        {"camera", Value::Object{{"centreX", preset.camera.centreX}, {"centreY", preset.camera.centreY}, {"scale", preset.camera.scale}, {"centreXLow", preset.camera.centreXLow}, {"centreYLow", preset.camera.centreYLow}}},
        {"startingScale", preset.startingScale},
        {"maximumZoom", preset.maximumZoom},
        {"zoomSpeed", preset.zoomSpeed},
        {"zoomRestartBehaviour", ToString(preset.zoomRestartBehaviour)},
        {"maximumIterations", preset.maximumIterations},
        {"equation", EquationToJson(preset.equation)},
        {"palette", ToString(preset.palette)},
        {"customPaletteColours", PaletteColoursToJson(preset.customPaletteColours)},
        {"colourOffset", preset.colourOffset},
        {"colourCycleSpeed", preset.colourCycleSpeed},
        {"brightness", preset.brightness},
        {"contrast", preset.contrast},
        {"saturation", preset.saturation},
        {"interiorColour", ColourToJson(preset.interiorColour)},
        {"backgroundColour", ColourToJson(preset.backgroundColour)},
        {"smoothColouring", preset.smoothColouring},
        {"animationMode", ToString(preset.animationMode)},
        {"automaticJourneyWaypoints", preset.automaticJourneyWaypoints},
        {"frameRateLimit", preset.frameRateLimit},
        {"renderScale", preset.renderScale},
        {"antiAliasingLevel", preset.antiAliasingLevel},
    };
}

std::optional<Preset> PresetFromJson(const Value& root, std::string& error) {
    if (!root.IsObject()) {
        error = "Preset root must be a JSON object.";
        return std::nullopt;
    }
    Preset preset;
    const auto* id = root.Find("id");
    const auto* name = root.Find("name");
    const auto* camera = root.Find("camera");
    const auto* palette = root.Find("palette");
    const auto* mode = root.Find("animationMode");
    if (!id || !id->IsString() || !name || !name->IsString() || !camera || !camera->IsObject() || !palette || !palette->IsString() || !mode || !mode->IsString()) {
        error = "Preset is missing required typed fields.";
        return std::nullopt;
    }
    preset.id = id->AsString();
    preset.name = name->AsString();
    preset.builtIn = false; // Imported presets never gain built-in privileges.
    if (const auto* value = camera->Find("centreX")) preset.camera.centreX = value->AsNumber(preset.camera.centreX);
    if (const auto* value = camera->Find("centreY")) preset.camera.centreY = value->AsNumber(preset.camera.centreY);
    if (const auto* value = camera->Find("scale")) preset.camera.scale = value->AsNumber(preset.camera.scale);
    if (const auto* value = camera->Find("centreXLow")) preset.camera.centreXLow = value->AsNumber(preset.camera.centreXLow);
    if (const auto* value = camera->Find("centreYLow")) preset.camera.centreYLow = value->AsNumber(preset.camera.centreYLow);
    if (const auto* value = root.Find("startingScale")) preset.startingScale = value->AsNumber(preset.camera.scale);
    else preset.startingScale = preset.camera.scale;
    if (const auto* value = root.Find("maximumZoom")) preset.maximumZoom = value->AsNumber(preset.maximumZoom);
    if (const auto* value = root.Find("zoomSpeed")) preset.zoomSpeed = value->AsNumber(preset.zoomSpeed);
    if (const auto* value = root.Find("zoomRestartBehaviour")) {
        const auto behaviour = ZoomRestartBehaviourFromString(value->AsString());
        if (!behaviour) { error = "Preset contains an unsupported zoom restart behaviour."; return std::nullopt; }
        preset.zoomRestartBehaviour = *behaviour;
    }
    if (const auto* value = root.Find("maximumIterations")) preset.maximumIterations = value->AsInt(preset.maximumIterations);
    if (!EquationFromJson(root.Find("equation"), preset.equation, error)) return std::nullopt;
    const auto paletteValue = PaletteFromString(palette->AsString());
    const auto modeValue = AnimationModeFromString(mode->AsString());
    if (!paletteValue || !modeValue) {
        error = "Preset contains an unsupported palette or animation mode.";
        return std::nullopt;
    }
    preset.palette = *paletteValue;
    preset.animationMode = *modeValue;
    if (!PaletteColoursFromJson(root.Find("customPaletteColours"), preset.customPaletteColours, error)) return std::nullopt;
    if (const auto* value = root.Find("automaticJourneyWaypoints")) preset.automaticJourneyWaypoints = value->AsString();
    if (const auto* value = root.Find("colourOffset")) preset.colourOffset = value->AsNumber(preset.colourOffset);
    if (const auto* value = root.Find("colourCycleSpeed")) preset.colourCycleSpeed = value->AsNumber(preset.colourCycleSpeed);
    if (const auto* value = root.Find("brightness")) preset.brightness = value->AsNumber(preset.brightness);
    if (const auto* value = root.Find("contrast")) preset.contrast = value->AsNumber(preset.contrast);
    if (const auto* value = root.Find("saturation")) preset.saturation = value->AsNumber(preset.saturation);
    preset.interiorColour = ColourFromJson(root.Find("interiorColour"), preset.interiorColour);
    preset.backgroundColour = ColourFromJson(root.Find("backgroundColour"), preset.backgroundColour);
    if (const auto* value = root.Find("smoothColouring")) preset.smoothColouring = value->AsBool(preset.smoothColouring);
    if (const auto* value = root.Find("frameRateLimit")) preset.frameRateLimit = value->AsInt(preset.frameRateLimit);
    if (const auto* value = root.Find("renderScale")) preset.renderScale = value->AsNumber(preset.renderScale);
    if (const auto* value = root.Find("antiAliasingLevel")) preset.antiAliasingLevel = value->AsInt(preset.antiAliasingLevel);

    const auto validation = ValidateAndNormalise(preset);
    if (!validation.valid) {
        std::ostringstream stream;
        stream << "Preset values were invalid:";
        for (const auto& issue : validation.issues) stream << " " << issue.field << ";";
        error = stream.str();
        return std::nullopt;
    }
    return preset;
}

std::string ReadFile(const std::filesystem::path& path, std::string& error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "Could not open file.";
        return {};
    }
    stream.seekg(0, std::ios::end);
    const auto size = stream.tellg();
    if (size < 0 || size > static_cast<std::streamoff>(16 * 1024 * 1024)) {
        error = "File is empty or exceeds the 16 MiB limit.";
        return {};
    }
    stream.seekg(0, std::ios::beg);
    std::string text(static_cast<std::size_t>(size), '\0');
    if (!text.empty()) stream.read(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream && !text.empty()) {
        error = "Could not read the complete file.";
        return {};
    }
    return text;
}

bool WriteAtomically(const std::filesystem::path& path, const std::string& text, std::string& error) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        error = "Could not create the settings directory.";
        return false;
    }
    const auto temporary = path.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = "Could not create the temporary settings file.";
            return false;
        }
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.flush();
        if (!stream) {
            error = "Could not write the complete settings file.";
            return false;
        }
    }
    std::filesystem::rename(temporary, path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        ec.clear();
        std::filesystem::rename(temporary, path, ec);
    }
    if (ec) {
        error = "Could not replace the settings file.";
        std::filesystem::remove(temporary, ec);
        return false;
    }
    return true;
}

} // namespace

SettingsStore::SettingsStore(std::filesystem::path settingsPath)
    : settingsPath_(std::move(settingsPath)) {}

LoadSettingsResult SettingsStore::Load() const {
    LoadSettingsResult result;
    if (!std::filesystem::exists(settingsPath_)) {
        result.usedDefaults = true;
        return result;
    }
    std::string readError;
    const auto text = ReadFile(settingsPath_, readError);
    if (!readError.empty()) {
        result.usedDefaults = true;
        result.warning = readError;
        return result;
    }
    std::string parseError;
    auto settings = DeserialiseSettings(text, parseError);
    if (settings) {
        result.settings = std::move(*settings);
        return result;
    }

    std::error_code ec;
    const auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto backup = settingsPath_.string() + ".corrupt-" + std::to_string(timestamp) + ".json";
    std::filesystem::copy_file(settingsPath_, backup, std::filesystem::copy_options::overwrite_existing, ec);
    result.usedDefaults = true;
    result.warning = "Settings were invalid and safe defaults were loaded. " + parseError;
    return result;
}

bool SettingsStore::Save(const AppSettings& settings, std::string& error) const {
    AppSettings copy = settings;
    ValidateAndNormalise(copy);
    return WriteAtomically(settingsPath_, SerialiseSettings(copy), error);
}

bool SettingsStore::Reset(std::string& error) const {
    std::error_code ec;
    std::filesystem::remove(settingsPath_, ec);
    if (ec) {
        error = "Could not remove the settings file.";
        return false;
    }
    return true;
}

std::string SettingsStore::SerialisePreset(const Preset& preset) {
    Preset copy = preset;
    ValidateAndNormalise(copy);
    copy.builtIn = false;
    return json::Stringify(PresetToJson(copy), true);
}

std::optional<Preset> SettingsStore::DeserialisePreset(const std::string& text, std::string& error) {
    const auto parsed = json::Parse(text, 256 * 1024);
    if (!parsed.value) {
        error = parsed.error + " Offset: " + std::to_string(parsed.errorOffset) + ".";
        return std::nullopt;
    }
    return PresetFromJson(*parsed.value, error);
}

std::string SettingsStore::SerialiseSettings(const AppSettings& settings) {
    Value::Array customPresets;
    for (const auto& preset : settings.customPresets) customPresets.push_back(PresetToJson(preset));
    Value::Array customPalettePresets;
    for (const auto& preset : settings.customPalettePresets) customPalettePresets.push_back(PalettePresetToJson(preset));
    Value::Array customEquationPresets;
    for (const auto& preset : settings.customEquationPresets) customEquationPresets.push_back(EquationPresetToJson(preset));
    Value::Object assignments;
    for (const auto& [monitor, preset] : settings.monitorPresetAssignments) assignments[monitor] = preset;
    Value::Array staticImages;
    for (const auto& path : settings.staticWallpaper.imagePaths) staticImages.push_back(path);

    Value root = Value::Object{
        {"schemaVersion", settings.schemaVersion},
        {"selectedPresetId", settings.selectedPresetId},
        {"monitorMode", ToString(settings.monitorMode)},
        {"lastWallpaperRunning", settings.lastWallpaperRunning},
        {"general", Value::Object{
            {"startWithWindows", settings.general.startWithWindows},
            {"minimiseToTray", settings.general.minimiseToTray},
            {"reducedMotion", settings.general.reducedMotion},
            {"colourCyclingEnabled", settings.general.colourCyclingEnabled},
            {"restoreOnExit", settings.general.restoreOnExit},
            {"startWallpaperOnLaunch", settings.general.startWallpaperOnLaunch},
        }},
        {"staticWallpaper", Value::Object{
            {"enabled", settings.staticWallpaper.enabled},
            {"cycleEnabled", settings.staticWallpaper.cycleEnabled},
            {"cycleSeconds", settings.staticWallpaper.cycleSeconds},
            {"currentIndex", settings.staticWallpaper.currentIndex},
            {"order", ToString(settings.staticWallpaper.order)},
            {"storageDirectory", settings.staticWallpaper.storageDirectory},
            {"imagePaths", std::move(staticImages)},
        }},
        {"performance", Value::Object{
            {"profile", ToString(settings.performance.profile)},
            {"maximumFrameRate", settings.performance.maximumFrameRate},
            {"renderScale", settings.performance.renderScale},
            {"maximumIterations", settings.performance.maximumIterations},
            {"antiAliasingLevel", settings.performance.antiAliasingLevel},
            {"pauseOnBattery", settings.performance.pauseOnBattery},
            {"reduceQualityOnBattery", settings.performance.reduceQualityOnBattery},
            {"pauseWhenFullscreen", settings.performance.pauseWhenFullscreen},
            {"pauseWhenDesktopHidden", settings.performance.pauseWhenDesktopHidden},
            {"pauseDuringRemoteDesktop", settings.performance.pauseDuringRemoteDesktop},
            {"pauseWhenLocked", settings.performance.pauseWhenLocked},
            {"resumeDelayMs", settings.performance.resumeDelayMs},
            {"precision", Value::Object{
                {"mode", ToString(settings.performance.precision.mode)},
                {"allowFloat64", settings.performance.precision.allowFloat64},
                {"allowSplitFloat", settings.performance.precision.allowSplitFloat},
                {"allowPerturbation", settings.performance.precision.allowPerturbation},
                {"allowArbitraryPrecision", settings.performance.precision.allowArbitraryPrecision},
                {"automaticFallback", settings.performance.precision.automaticFallback},
                {"arbitraryPrecisionBits", settings.performance.precision.arbitraryPrecisionBits},
            }},
            {"adaptive", Value::Object{
                {"enabled", settings.performance.adaptive.enabled},
                {"pauseOnLowFps", settings.performance.adaptive.pauseOnLowFps},
                {"minimumFramesPerSecond", settings.performance.adaptive.minimumFramesPerSecond},
                {"lowFpsSustainMs", settings.performance.adaptive.lowFpsSustainMs},
                {"pauseOnHighCpu", settings.performance.adaptive.pauseOnHighCpu},
                {"maximumProcessCpuPercent", settings.performance.adaptive.maximumProcessCpuPercent},
                {"highCpuSustainMs", settings.performance.adaptive.highCpuSustainMs},
                {"pauseOnHighMemory", settings.performance.adaptive.pauseOnHighMemory},
                {"maximumWorkingSetMb", settings.performance.adaptive.maximumWorkingSetMb},
                {"highMemorySustainMs", settings.performance.adaptive.highMemorySustainMs},
                {"resumeStableMs", settings.performance.adaptive.resumeStableMs},
                {"stopWhenVisuallyUnchanged", settings.performance.adaptive.stopWhenVisuallyUnchanged},
                {"minimumVisiblePixelChange", settings.performance.adaptive.minimumVisiblePixelChange},
                {"minimumVisibleColourChange", settings.performance.adaptive.minimumVisibleColourChange},
            }},
        }},
        {"monitorPresetAssignments", std::move(assignments)},
        {"customPresets", std::move(customPresets)},
        {"customPalettePresets", std::move(customPalettePresets)},
        {"customEquationPresets", std::move(customEquationPresets)},
    };
    return json::Stringify(root, true);
}

std::optional<AppSettings> SettingsStore::DeserialiseSettings(const std::string& text, std::string& error) {
    const auto parsed = json::Parse(text, 1024 * 1024);
    if (!parsed.value) {
        error = parsed.error + " Offset: " + std::to_string(parsed.errorOffset) + ".";
        return std::nullopt;
    }
    const auto& root = *parsed.value;
    if (!root.IsObject()) {
        error = "Settings root must be a JSON object.";
        return std::nullopt;
    }
    AppSettings settings;
    if (const auto* value = root.Find("schemaVersion")) settings.schemaVersion = value->AsInt(settings.schemaVersion);
    if (const auto* value = root.Find("selectedPresetId")) settings.selectedPresetId = value->AsString(settings.selectedPresetId);
    if (const auto* value = root.Find("lastWallpaperRunning")) settings.lastWallpaperRunning = value->AsBool(settings.lastWallpaperRunning);
    if (const auto* value = root.Find("monitorMode")) {
        const auto mode = MonitorModeFromString(value->AsString());
        if (!mode) {
            error = "Settings contain an unsupported monitor mode.";
            return std::nullopt;
        }
        settings.monitorMode = *mode;
    }
    if (const auto* general = root.Find("general"); general && general->IsObject()) {
        if (const auto* value = general->Find("startWithWindows")) settings.general.startWithWindows = value->AsBool(settings.general.startWithWindows);
        if (const auto* value = general->Find("minimiseToTray")) settings.general.minimiseToTray = value->AsBool(settings.general.minimiseToTray);
        if (const auto* value = general->Find("reducedMotion")) settings.general.reducedMotion = value->AsBool(settings.general.reducedMotion);
        if (const auto* value = general->Find("colourCyclingEnabled")) settings.general.colourCyclingEnabled = value->AsBool(settings.general.colourCyclingEnabled);
        if (const auto* value = general->Find("restoreOnExit")) settings.general.restoreOnExit = value->AsBool(settings.general.restoreOnExit);
        if (const auto* value = general->Find("startWallpaperOnLaunch")) settings.general.startWallpaperOnLaunch = value->AsBool(settings.general.startWallpaperOnLaunch);
    }
    if (const auto* staticWallpaper = root.Find("staticWallpaper"); staticWallpaper && staticWallpaper->IsObject()) {
        if (const auto* value = staticWallpaper->Find("enabled")) settings.staticWallpaper.enabled = value->AsBool(settings.staticWallpaper.enabled);
        if (const auto* value = staticWallpaper->Find("cycleEnabled")) settings.staticWallpaper.cycleEnabled = value->AsBool(settings.staticWallpaper.cycleEnabled);
        if (const auto* value = staticWallpaper->Find("cycleSeconds")) settings.staticWallpaper.cycleSeconds = value->AsInt(settings.staticWallpaper.cycleSeconds);
        if (const auto* value = staticWallpaper->Find("currentIndex")) settings.staticWallpaper.currentIndex = value->AsInt(settings.staticWallpaper.currentIndex);
        if (const auto* value = staticWallpaper->Find("order")) {
            const auto order = StaticSlideshowOrderFromString(value->AsString());
            if (!order) {
                error = "Settings contain an unsupported static slideshow order.";
                return std::nullopt;
            }
            settings.staticWallpaper.order = *order;
        }
        if (const auto* value = staticWallpaper->Find("storageDirectory")) {
            if (!value->IsString() || value->AsString().size() > 32768 ||
                value->AsString().find('\0') != std::string::npos) {
                error = "The static slideshow storage directory is invalid.";
                return std::nullopt;
            }
            settings.staticWallpaper.storageDirectory = value->AsString();
        }
        if (const auto* images = staticWallpaper->Find("imagePaths")) {
            if (!images->IsArray() || images->AsArray().size() > 512) {
                error = "Static wallpaper image history is invalid or too large.";
                return std::nullopt;
            }
            for (const auto& image : images->AsArray()) {
                if (!image.IsString() || image.AsString().empty() || image.AsString().size() > 32768 ||
                    image.AsString().find('\0') != std::string::npos) {
                    error = "A static wallpaper image path is invalid.";
                    return std::nullopt;
                }
                settings.staticWallpaper.imagePaths.push_back(image.AsString());
            }
        }
    }
    if (const auto* performance = root.Find("performance"); performance && performance->IsObject()) {
        if (const auto* value = performance->Find("profile")) {
            const auto profile = PerformanceProfileFromString(value->AsString());
            if (!profile) {
                error = "Settings contain an unsupported performance profile.";
                return std::nullopt;
            }
            settings.performance.profile = *profile;
        }
        if (const auto* value = performance->Find("maximumFrameRate")) settings.performance.maximumFrameRate = value->AsInt(settings.performance.maximumFrameRate);
        if (const auto* value = performance->Find("renderScale")) settings.performance.renderScale = value->AsNumber(settings.performance.renderScale);
        if (const auto* value = performance->Find("maximumIterations")) settings.performance.maximumIterations = value->AsInt(settings.performance.maximumIterations);
        if (const auto* value = performance->Find("antiAliasingLevel")) settings.performance.antiAliasingLevel = value->AsInt(settings.performance.antiAliasingLevel);
        if (const auto* value = performance->Find("pauseOnBattery")) settings.performance.pauseOnBattery = value->AsBool(settings.performance.pauseOnBattery);
        if (const auto* value = performance->Find("reduceQualityOnBattery")) settings.performance.reduceQualityOnBattery = value->AsBool(settings.performance.reduceQualityOnBattery);
        if (const auto* value = performance->Find("pauseWhenFullscreen")) settings.performance.pauseWhenFullscreen = value->AsBool(settings.performance.pauseWhenFullscreen);
        if (const auto* value = performance->Find("pauseWhenDesktopHidden")) settings.performance.pauseWhenDesktopHidden = value->AsBool(settings.performance.pauseWhenDesktopHidden);
        if (const auto* value = performance->Find("pauseDuringRemoteDesktop")) settings.performance.pauseDuringRemoteDesktop = value->AsBool(settings.performance.pauseDuringRemoteDesktop);
        if (const auto* value = performance->Find("pauseWhenLocked")) settings.performance.pauseWhenLocked = value->AsBool(settings.performance.pauseWhenLocked);
        if (const auto* value = performance->Find("resumeDelayMs")) settings.performance.resumeDelayMs = value->AsInt(settings.performance.resumeDelayMs);
        if (const auto* precision = performance->Find("precision"); precision && precision->IsObject()) {
            if (const auto* value = precision->Find("mode")) {
                const auto mode = PrecisionModeFromString(value->AsString());
                if (!mode) { error = "Settings contain an unsupported precision mode."; return std::nullopt; }
                settings.performance.precision.mode = *mode;
            }
            if (const auto* value = precision->Find("allowFloat64")) settings.performance.precision.allowFloat64 = value->AsBool(settings.performance.precision.allowFloat64);
            if (const auto* value = precision->Find("allowSplitFloat")) settings.performance.precision.allowSplitFloat = value->AsBool(settings.performance.precision.allowSplitFloat);
            if (const auto* value = precision->Find("allowPerturbation")) settings.performance.precision.allowPerturbation = value->AsBool(settings.performance.precision.allowPerturbation);
            if (const auto* value = precision->Find("allowArbitraryPrecision")) settings.performance.precision.allowArbitraryPrecision = value->AsBool(settings.performance.precision.allowArbitraryPrecision);
            if (const auto* value = precision->Find("automaticFallback")) settings.performance.precision.automaticFallback = value->AsBool(settings.performance.precision.automaticFallback);
            if (const auto* value = precision->Find("arbitraryPrecisionBits")) settings.performance.precision.arbitraryPrecisionBits = value->AsInt(settings.performance.precision.arbitraryPrecisionBits);
        }
        if (const auto* adaptive = performance->Find("adaptive"); adaptive && adaptive->IsObject()) {
            if (const auto* value = adaptive->Find("enabled")) settings.performance.adaptive.enabled = value->AsBool(settings.performance.adaptive.enabled);
            if (const auto* value = adaptive->Find("pauseOnLowFps")) settings.performance.adaptive.pauseOnLowFps = value->AsBool(settings.performance.adaptive.pauseOnLowFps);
            if (const auto* value = adaptive->Find("minimumFramesPerSecond")) settings.performance.adaptive.minimumFramesPerSecond = value->AsNumber(settings.performance.adaptive.minimumFramesPerSecond);
            if (const auto* value = adaptive->Find("lowFpsSustainMs")) settings.performance.adaptive.lowFpsSustainMs = value->AsInt(settings.performance.adaptive.lowFpsSustainMs);
            if (const auto* value = adaptive->Find("pauseOnHighCpu")) settings.performance.adaptive.pauseOnHighCpu = value->AsBool(settings.performance.adaptive.pauseOnHighCpu);
            if (const auto* value = adaptive->Find("maximumProcessCpuPercent")) settings.performance.adaptive.maximumProcessCpuPercent = value->AsNumber(settings.performance.adaptive.maximumProcessCpuPercent);
            if (const auto* value = adaptive->Find("highCpuSustainMs")) settings.performance.adaptive.highCpuSustainMs = value->AsInt(settings.performance.adaptive.highCpuSustainMs);
            if (const auto* value = adaptive->Find("pauseOnHighMemory")) settings.performance.adaptive.pauseOnHighMemory = value->AsBool(settings.performance.adaptive.pauseOnHighMemory);
            if (const auto* value = adaptive->Find("maximumWorkingSetMb")) settings.performance.adaptive.maximumWorkingSetMb = value->AsInt(settings.performance.adaptive.maximumWorkingSetMb);
            if (const auto* value = adaptive->Find("highMemorySustainMs")) settings.performance.adaptive.highMemorySustainMs = value->AsInt(settings.performance.adaptive.highMemorySustainMs);
            if (const auto* value = adaptive->Find("resumeStableMs")) settings.performance.adaptive.resumeStableMs = value->AsInt(settings.performance.adaptive.resumeStableMs);
            if (const auto* value = adaptive->Find("stopWhenVisuallyUnchanged")) settings.performance.adaptive.stopWhenVisuallyUnchanged = value->AsBool(settings.performance.adaptive.stopWhenVisuallyUnchanged);
            if (const auto* value = adaptive->Find("minimumVisiblePixelChange")) settings.performance.adaptive.minimumVisiblePixelChange = value->AsNumber(settings.performance.adaptive.minimumVisiblePixelChange);
            if (const auto* value = adaptive->Find("minimumVisibleColourChange")) settings.performance.adaptive.minimumVisibleColourChange = value->AsNumber(settings.performance.adaptive.minimumVisibleColourChange);
        }
    }
    if (const auto* assignments = root.Find("monitorPresetAssignments"); assignments && assignments->IsObject()) {
        for (const auto& [monitor, value] : assignments->AsObject()) {
            if (value.IsString() && monitor.size() <= 256 && value.AsString().size() <= 80) settings.monitorPresetAssignments[monitor] = value.AsString();
        }
    }
    if (const auto* customPresets = root.Find("customPresets"); customPresets && customPresets->IsArray()) {
        if (customPresets->AsArray().size() > 256) {
            error = "Settings contain too many custom presets.";
            return std::nullopt;
        }
        for (const auto& value : customPresets->AsArray()) {
            std::string presetError;
            auto preset = PresetFromJson(value, presetError);
            if (!preset) {
                error = "A custom preset is invalid. " + presetError;
                return std::nullopt;
            }
            settings.customPresets.push_back(std::move(*preset));
        }
    }
    if (const auto* customPalettePresets = root.Find("customPalettePresets"); customPalettePresets) {
        if (!customPalettePresets->IsArray() || customPalettePresets->AsArray().size() > 256) {
            error = "Settings contain an invalid number of saved palette presets.";
            return std::nullopt;
        }
        for (const auto& value : customPalettePresets->AsArray()) {
            std::string paletteError;
            auto palettePreset = PalettePresetFromJson(value, paletteError);
            if (!palettePreset) {
                error = "A saved palette preset is invalid. " + paletteError;
                return std::nullopt;
            }
            settings.customPalettePresets.push_back(std::move(*palettePreset));
        }
    }
    if (const auto* customEquationPresets = root.Find("customEquationPresets"); customEquationPresets) {
        if (!customEquationPresets->IsArray() || customEquationPresets->AsArray().size() > 256) {
            error = "Saved equation presets must be an array with at most 256 entries.";
            return std::nullopt;
        }
        for (const auto& value : customEquationPresets->AsArray()) {
            auto equationPreset = EquationPresetFromJson(value, error);
            if (!equationPreset) return std::nullopt;
            settings.customEquationPresets.push_back(std::move(*equationPreset));
        }
    }
    const int sourceSchemaVersion = settings.schemaVersion;
    const auto validation = ValidateAndNormalise(settings);
    if (!validation.valid && sourceSchemaVersion != 1 && sourceSchemaVersion != 2 && sourceSchemaVersion != 3 && sourceSchemaVersion != 4 && sourceSchemaVersion != 5 && sourceSchemaVersion != 6 && sourceSchemaVersion != 7) {
        error = "Settings schema is unsupported.";
        return std::nullopt;
    }
    return settings;
}

} // namespace mw
