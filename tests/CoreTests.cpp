#include "Core/AdaptivePerformance.h"
#include "Core/Animation.h"
#include "Core/DeepZoom.h"
#include "Core/MandelbrotMath.h"
#include "Core/Models.h"
#include "Core/SettingsStore.h"
#include "Core/StillImageRenderer.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void TestKnownPoints() {
    Check(!mw::CalculateEscape(0.0, 0.0, 500).escaped, "0 + 0i should remain inside.");
    Check(!mw::CalculateEscape(-1.0, 0.0, 500).escaped, "-1 + 0i should remain inside.");
    Check(mw::CalculateEscape(2.0, 2.0, 500).escaped, "2 + 2i should escape.");
    Check(mw::CalculateEscape(0.5, 0.5, 500).escaped, "0.5 + 0.5i should escape.");
    const auto smooth = mw::CalculateEscape(0.5, 0.5, 500);
    Check(std::isfinite(smooth.smoothValue), "Smooth escape value must be finite.");
}

void TestPresetValidation() {
    mw::Preset preset;
    preset.id.clear();
    preset.name.clear();
    preset.camera.scale = 0.0;
    preset.maximumIterations = 50000;
    preset.renderScale = 9.0;
    preset.equation.quadratic.real = 99.0;
    preset.automaticJourneyWaypoints.assign(40000, '1');
    const auto result = mw::ValidateAndNormalise(preset);
    Check(!result.valid, "Invalid preset should report validation issues.");
    Check(preset.camera.scale >= 1.0e-32, "Scale should be clamped to the deep-zoom precision floor.");
    Check(preset.maximumIterations == 4096, "Iterations should be capped.");
    Check(preset.renderScale == 1.0, "Render scale should be capped.");
    Check(preset.equation.quadratic.real == 8.0, "Equation coefficients should be safely capped.");
    Check(preset.automaticJourneyWaypoints.size() == 32768U,
          "Automatic Journey waypoint text should be safety-bounded.");

    mw::PalettePreset palettePreset;
    palettePreset.id = "palette-validation";
    palettePreset.name = "Palette Validation";
    palettePreset.colours = {{2.0F, -1.0F, 0.5F, 1.0F}};
    const auto paletteResult = mw::ValidateAndNormalise(palettePreset);
    Check(!paletteResult.valid, "A one-stop palette preset should report validation issues.");
    Check(palettePreset.colours.size() == 2, "A one-stop palette should be normalised to two stops.");
    Check(palettePreset.colours.front().r == 1.0F && palettePreset.colours.front().g == 0.0F,
          "Saved palette colours should be clamped.");
}

void TestSettingsRoundTrip() {
    mw::AppSettings settings;
    Check(!settings.general.colourCyclingEnabled,
          "New settings should start with colour cycling stopped.");
    settings.general.reducedMotion = true;
    settings.general.colourCyclingEnabled = false;
    settings.monitorMode = mw::MonitorMode::Independent;
    settings.performance.precision.mode = mw::PrecisionMode::ArbitraryPrecisionPerturbation;
    settings.performance.precision.arbitraryPrecisionBits = 512;
    settings.performance.precision.allowFloat64 = false;
    settings.performance.adaptive.minimumFramesPerSecond = 12.0;
    settings.performance.adaptive.maximumProcessCpuPercent = 55.0;
    settings.performance.adaptive.maximumWorkingSetMb = 1024;
    settings.performance.adaptive.minimumVisiblePixelChange = 0.4;
    auto custom = mw::BuiltInPresets().at(1);
    custom.id = "custom-test";
    custom.name = "Custom Test";
    custom.builtIn = false;
    custom.customPaletteColours = {{1.0F, 0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F, 1.0F}, {0.0F, 1.0F, 0.0F, 1.0F}};
    custom.equation = mw::EquationExample(7);
    custom.equation.power = 3;
    custom.equation.parameterPower = 2;
    custom.equation.reciprocalPower = 1;
    custom.equation.reciprocalCoefficient = {0.2, 0.0};
    custom.equation.initialZ = {0.1, -0.1};
    custom.equation.unaryTransform = mw::EquationUnaryTransform::Sin;
    custom.automaticJourneyWaypoints = "-0.743643887037151,0.131825904205330,0.004,12,1;0.285,0.01,0.028,14,2";
    settings.customPresets.push_back(custom);
    settings.customPalettePresets.push_back({
        "palette-test", "Palette Test",
        {{0.9F, 0.1F, 0.2F, 1.0F}, {0.1F, 0.3F, 0.9F, 1.0F}, {0.2F, 0.9F, 0.4F, 1.0F}}
    });
    settings.customEquationPresets.push_back({"equation-test", "Equation Test", custom.equation});
    settings.staticWallpaper.enabled = true;
    settings.staticWallpaper.cycleEnabled = true;
    settings.staticWallpaper.cycleSeconds = 45;
    settings.staticWallpaper.order = mw::StaticSlideshowOrder::Shuffle;
    settings.staticWallpaper.storageDirectory = "C:/renders";
    settings.staticWallpaper.imagePaths = {"C:/renders/one.bmp", "C:/renders/two.bmp"};
    settings.staticWallpaper.currentIndex = 1;
    settings.monitorPresetAssignments["DISPLAY1"] = custom.id;

    const auto serialised = mw::SettingsStore::SerialiseSettings(settings);
    std::string error;
    const auto parsed = mw::SettingsStore::DeserialiseSettings(serialised, error);
    Check(parsed.has_value(), "Settings round trip should parse: " + error);
    if (parsed) {
        Check(parsed->schemaVersion == 8, "Settings should migrate to schema version 8.");
        Check(parsed->general.reducedMotion, "Reduced-motion setting should persist.");
        Check(!parsed->general.colourCyclingEnabled, "Colour-cycling play/pause state should persist.");
        Check(parsed->monitorMode == mw::MonitorMode::Independent, "Monitor mode should persist.");
        Check(parsed->performance.precision.mode == mw::PrecisionMode::ArbitraryPrecisionPerturbation, "Precision strategy should persist.");
        Check(parsed->performance.precision.arbitraryPrecisionBits == 512, "Arbitrary precision bit count should persist.");
        Check(!parsed->performance.precision.allowFloat64, "Precision candidate toggles should persist.");
        Check(parsed->performance.adaptive.minimumFramesPerSecond == 12.0,
              "Adaptive minimum FPS should persist.");
        Check(parsed->performance.adaptive.maximumProcessCpuPercent == 55.0,
              "Adaptive CPU limit should persist.");
        Check(parsed->performance.adaptive.maximumWorkingSetMb == 1024,
              "Adaptive memory limit should persist.");
        Check(parsed->performance.adaptive.minimumVisiblePixelChange == 0.4,
              "Invisible-frame pixel threshold should persist.");
        Check(parsed->customPresets.size() == 1, "Custom preset should persist.");
        Check(parsed->customPresets.front().customPaletteColours.size() == 3, "Custom palette stops should persist.");
        Check(parsed->customPresets.front().equation.power == 3 &&
              parsed->customPresets.front().equation.parameterPower == 2 &&
              parsed->customPresets.front().equation.reciprocalPower == 1 &&
              parsed->customPresets.front().equation.juliaMode,
              "Advanced equation options, including c powers, should persist.");
        Check(parsed->customPresets.front().automaticJourneyWaypoints == custom.automaticJourneyWaypoints,
              "Custom Automatic Journey waypoint text should persist.");
        Check(parsed->customPalettePresets.size() == 1, "Saved palette presets should persist.");
        Check(parsed->customEquationPresets.size() == 1, "Saved equation presets should persist independently.");
        if (!parsed->customEquationPresets.empty()) {
            Check(parsed->customEquationPresets.front().equation.power == 3,
                  "Saved equation preset power should persist.");
            Check(parsed->customEquationPresets.front().equation.unaryTransform == mw::EquationUnaryTransform::Sin,
                  "Saved equation preset transform should persist.");
        }
        if (!parsed->customPalettePresets.empty()) {
            Check(parsed->customPalettePresets.front().name == "Palette Test", "Saved palette name should persist.");
            Check(parsed->customPalettePresets.front().colours.size() == 3, "Saved palette colours should persist.");
        }
        Check(parsed->staticWallpaper.enabled, "Static wallpaper mode should persist.");
        Check(parsed->staticWallpaper.cycleEnabled, "Static wallpaper cycling should persist.");
        Check(parsed->staticWallpaper.cycleSeconds == 45, "Static wallpaper interval should persist.");
        Check(parsed->staticWallpaper.order == mw::StaticSlideshowOrder::Shuffle,
              "Static wallpaper playback order should persist.");
        Check(parsed->staticWallpaper.storageDirectory == "C:/renders",
              "Static wallpaper capture directory should persist.");
        Check(parsed->staticWallpaper.imagePaths.size() == 2, "Static wallpaper image history should persist.");
        Check(parsed->staticWallpaper.currentIndex == 1, "Static wallpaper index should persist.");
        Check(parsed->monitorPresetAssignments.at("DISPLAY1") == "custom-test", "Monitor assignment should persist.");
    }
}

void TestPresetImportSecurity() {
    std::string error;
    const auto executable = mw::SettingsStore::DeserialisePreset(R"json({"id":"x","name":"x","camera":{"centreX":0,"centreY":0,"scale":1},"palette":"classic-spectrum","animationMode":"manual-view","script":"DeleteEverything()"})json", error);
    Check(executable.has_value(), "Unknown data-only fields should be ignored safely.");
    const auto invalid = mw::SettingsStore::DeserialisePreset(R"json({"id":"x","name":"x","camera":{"centreX":0,"centreY":0,"scale":1},"palette":"remote-shader","animationMode":"manual-view"})json", error);
    Check(!invalid.has_value(), "Unknown palette must be rejected.");
    const auto invalidEquation = mw::SettingsStore::DeserialisePreset(R"json({"id":"x","name":"x","camera":{"centreX":0,"centreY":0,"scale":1},"palette":"classic-spectrum","animationMode":"manual-view","equation":"run shader code"})json", error);
    Check(!invalidEquation.has_value(), "Equation input must be bounded structured data, not executable text.");
    const auto malformed = mw::SettingsStore::DeserialisePreset("{not-json", error);
    Check(!malformed.has_value(), "Malformed JSON must be rejected.");
}


void TestCustomEquations() {
    const auto classic = mw::EquationExample(0);
    Check(mw::CalculateEscape(0.5, 0.5, 500, classic).escaped,
          "The classic equation example should match Mandelbrot escape behaviour.");

    const auto cubic = mw::EquationExample(1);
    Check(cubic.power == 3 && cubic.initialZMode == mw::InitialZMode::CriticalPoint,
          "Cubic parameter sets should use the degree-three critical point configuration.");
    Check(!mw::CalculateEscape(0.0, 0.0, 100, cubic).escaped,
          "The cubic parameter-set critical orbit should remain bounded at c=0.");

    const auto scaledC = mw::EquationExample(5);
    Check(!mw::CalculateEscape(0.0, 0.0, 100, scaledC).escaped,
          "z = z + 1.2c should remain at zero when c is zero.");
    Check(mw::CalculateEscape(2.0, 0.0, 100, scaledC).escaped,
          "z = z + 1.2c should escape for a large positive c.");

    const auto rational = mw::EquationExample(6);
    Check(rational.reciprocalPower == 1 && rational.reciprocalCoefficient.real == 0.25,
          "Rational-map examples should configure bounded reciprocal powers.");
    Check(mw::CalculateEscape(0.0, 0.0, 100, rational).escaped,
          "The rational map should safely handle a critical orbit near a pole.");

    const auto julia = mw::EquationExample(7);
    Check(julia.juliaMode && std::abs(julia.juliaParameter.real + 0.8) < 1.0e-9,
          "Julia examples should configure a fixed complex parameter.");
    Check(mw::CalculateEscape(2.0, 2.0, 100, julia).escaped,
          "Julia mode should use the pixel as z0 and escape far-away points.");

    auto transformed = mw::EquationExample(9);
    Check(transformed.unaryTransform == mw::EquationUnaryTransform::Sin,
          "Sine examples should enable the bounded complex sine transform.");
    Check(std::isfinite(mw::CalculateEscape(0.2, 0.1, 100, transformed).smoothValue),
          "Trigonometric recurrence results should remain finite.");
    transformed.unaryTransform = mw::EquationUnaryTransform::Log;
    Check(std::isfinite(mw::CalculateEscape(0.0, 0.0, 50, transformed).smoothValue),
          "The logarithm transform should guard the zero singularity.");

    auto componentTransforms = classic;
    componentTransforms.conjugate = true;
    componentTransforms.swapRealImaginary = true;
    componentTransforms.absoluteReal = true;
    Check(std::isfinite(mw::CalculateEscape(-0.2, 0.6, 100, componentTransforms).smoothValue),
          "Conjugation, component swapping and independent absolute values should compose safely.");

    auto iterationEquation = classic;
    iterationEquation.iterationTerm = {0.02, -0.01};
    Check(mw::CalculateEscape(0.0, 0.0, 200, iterationEquation).escaped,
          "Iteration-dependent terms should change an otherwise bounded orbit.");

    const auto newton = mw::EquationExample(8);
    const auto newtonResult = mw::CalculateEscape(0.5, 0.5, 80, newton);
    Check(newton.newtonMode && newtonResult.converged && newtonResult.rootIndex >= 0,
          "Newton mode should converge and identify a root basin.");

    const auto orbitTrap = mw::EquationExample(12);
    const auto trapResult = mw::CalculateEscape(0.5, 0.5, 200, orbitTrap);
    Check(std::isfinite(trapResult.orbitTrapDistance),
          "Orbit-trap rendering should collect a finite trap distance.");

    const auto distance = mw::EquationExample(13);
    const auto distanceResult = mw::CalculateEscape(0.5, 0.5, 200, distance);
    Check(distanceResult.distanceEstimate >= 0.0 && std::isfinite(distanceResult.distanceEstimate),
          "Distance-estimation rendering should return a safe non-negative estimate.");

    auto animated = classic;
    animated.animateCoefficients = true;
    animated.coefficientAnimationAmplitude = 0.2;
    const auto animatedA = mw::CalculateEscape(0.4, 0.2, 100, animated, 0.0);
    const auto animatedB = mw::CalculateEscape(0.4, 0.2, 100, animated, 1.0);
    Check(animatedA.iterations != animatedB.iterations || animatedA.smoothValue != animatedB.smoothValue,
          "Animated coefficients should vary the recurrence over time.");

    Check(mw::EquationSummary(scaledC).find("1.2c") != std::string::npos,
          "Equation summary should describe the selected coefficients.");
}

void TestBuiltInEquationAndPaletteLibraries() {
    const auto names = mw::EquationExampleNames();
    Check(names.size() >= 45U, "The built-in equation library should contain the expanded preset set.");
    for (std::size_t index = 0; index < names.size(); ++index) {
        mw::EquationPreset preset{"equation-" + std::to_string(index), names[index], mw::EquationExample(index)};
        const auto validation = mw::ValidateAndNormalise(preset);
        Check(validation.valid, "Built-in equation preset should validate: " + names[index]);
        const auto sample = mw::CalculateEscape(0.37, -0.21, 80, preset.equation, 0.25);
        Check(std::isfinite(sample.smoothValue), "Built-in equation should produce a finite sample: " + names[index]);
    }

    const auto scaledC = mw::EquationExample(14);
    Check(std::abs(scaledC.parameter.real - 1.2) < 1.0e-12,
          "The reference scaled-c equation should use 1.2c.");
    const auto constantAdd = mw::EquationExample(15);
    Check(std::abs(constantAdd.constant.real - 0.5) < 1.0e-12,
          "The reference constant-add equation should add 0.5.");
    const auto scaledZ = mw::EquationExample(16);
    Check(std::abs(scaledZ.quadratic.real - 1.2) < 1.0e-12,
          "The reference scaled-z equation should use 1.2z squared.");
    const auto addToZ = mw::EquationExample(17);
    Check(std::abs(addToZ.linear.real - 0.5) < 1.0e-12,
          "The reference add-to-z equation should use 0.5z.");
    const auto swapped = mw::EquationExample(18);
    Check(swapped.parameterPower == 2 && std::abs(swapped.linear.real - 1.0) < 1.0e-12 &&
              std::abs(swapped.quadratic.real) < 1.0e-12,
          "The reference swap equation should be represented exactly as z + c squared.");
    Check(mw::EquationSummary(swapped).find("c^2") != std::string::npos,
          "Equation summaries should expose powered c terms.");
    Check(mw::CalculateEscape(2.0, 0.0, 20, swapped).iterations <
              mw::CalculateEscape(2.0, 0.0, 20, mw::EquationExample(0)).iterations,
          "The c-squared recurrence should be evaluated rather than treated as a linear c term.");
    const auto absoluteZ = mw::EquationExample(19);
    Check(absoluteZ.absoluteReal && absoluteZ.absoluteImaginary,
          "The reference absolute-z equation should apply absolute values to both components.");
    const auto minusC = mw::EquationExample(20);
    Check(std::abs(minusC.parameter.real + 1.0) < 1.0e-12,
          "The reference minus-c equation should use a negative parameter coefficient.");

    const auto palettes = mw::BuiltInPalettePresets();
    Check(palettes.size() >= 30U, "The built-in palette library should contain at least 30 palettes.");
    std::vector<std::string> paletteIds;
    for (auto palette : palettes) {
        const auto validation = mw::ValidateAndNormalise(palette);
        Check(validation.valid, "Built-in palette should validate: " + palette.name);
        Check(palette.colours.size() >= 2U, "Built-in palette should contain at least two stops: " + palette.name);
        Check(std::find(paletteIds.begin(), paletteIds.end(), palette.id) == paletteIds.end(),
              "Built-in palette ids should be unique: " + palette.id);
        paletteIds.push_back(palette.id);
    }

    const auto scenes = mw::BuiltInPresets();
    Check(scenes.size() >= 35U, "The complete preset library should include the expanded equation/colour scenes.");
    std::vector<std::string> sceneIds;
    for (auto scene : scenes) {
        const auto validation = mw::ValidateAndNormalise(scene);
        Check(validation.valid, "Built-in scene should validate: " + scene.name);
        Check(std::find(sceneIds.begin(), sceneIds.end(), scene.id) == sceneIds.end(),
              "Built-in scene ids should be unique: " + scene.id);
        sceneIds.push_back(scene.id);
    }
    const auto referenceScene = std::find_if(scenes.begin(), scenes.end(), [](const mw::Preset& scene) {
        return scene.id == "reference-swap-crimson";
    });
    Check(referenceScene != scenes.end() && referenceScene->equation.parameterPower == 2 &&
              !referenceScene->customPaletteColours.empty(),
          "The supplied swap-z/c equation and crimson palette should be available as a complete scene preset.");
}

void TestAnimation() {
    mw::AnimationController controller;
    auto preset = mw::BuiltInPresets().at(1);
    preset.animationMode = mw::AnimationMode::ContinuousZoom;
    preset.zoomSpeed = 0.2;
    controller.SetPreset(preset, false);
    const double initialScale = controller.Camera().scale;
    for (int i = 0; i < 60; ++i) controller.Update(1.0 / 60.0);
    Check(controller.Camera().scale < initialScale, "Continuous zoom should reduce scale.");
    controller.ZoomAt(0.0, 0.0, 1.0, 16.0 / 9.0);
    Check(controller.Camera().scale > 0.0, "Manual zoom should retain a positive scale.");

    controller.SetPreset(preset, false);
    const auto beforePause = controller.Update(0.1);
    controller.SetColourCyclingEnabled(false);
    const auto whilePaused = controller.Update(0.2);
    Check(whilePaused.colourOffset == beforePause.colourOffset,
          "Paused colour cycling should retain the current colour offset.");
    controller.SetColourCyclingEnabled(true);
    const auto beforeMotionPause = controller.Update(0.1);
    controller.SetMotionEnabled(false);
    const auto whileMotionPaused = controller.Update(0.5);
    Check(whileMotionPaused.camera.scale == beforeMotionPause.camera.scale,
          "Paused zoom motion should retain the current camera scale.");
    Check(whileMotionPaused.colourOffset > beforeMotionPause.colourOffset,
          "Stopping zoom motion should not stop colour cycling.");
    controller.SetMotionEnabled(true);
    const auto afterResume = controller.Update(0.2);
    Check(afterResume.colourOffset > whileMotionPaused.colourOffset,
          "Resumed colour cycling should continue from the retained offset.");

    auto journeyPreset = mw::BuiltInPresets().front();
    journeyPreset.animationMode = mw::AnimationMode::AutomaticJourney;
    controller.SetPreset(journeyPreset, false);
    Check(controller.JourneyTargetCount() >= 8,
          "Automatic journey should retain a varied set of safe destinations.");
    auto previous = controller.Update(0.0).camera;
    bool pannedDuringTransition = false;
    bool sawScaleDecrease = false;
    bool sawScaleIncrease = false;
    for (int i = 0; i < 500; ++i) {
        const auto camera = controller.Update(0.1).camera;
        const bool panned = std::abs(camera.centreX - previous.centreX) > 1.0e-10 ||
                            std::abs(camera.centreY - previous.centreY) > 1.0e-10;
        if (camera.scale < previous.scale - 1.0e-10) sawScaleDecrease = true;
        if (camera.scale > previous.scale + 1.0e-10) sawScaleIncrease = true;
        if (panned) pannedDuringTransition = true;
        previous = camera;
    }
    Check(sawScaleDecrease || sawScaleIncrease, "Automatic journey should change scale between destinations.");
    Check(pannedDuringTransition, "Automatic journey should pan during destination transitions.");

    auto scriptedJourney = mw::BuiltInPresets().front();
    scriptedJourney.animationMode = mw::AnimationMode::AutomaticJourney;
    scriptedJourney.automaticJourneyWaypoints =
        "-0.743643887037151,0.131825904205330,0.004,11,1;"
        "0.285,0.01,0.028,12,1;"
        "-1.25066,0.02012,0.009,13,1;"
        "-0.16,1.0405,0.018,14,1;"
        "-0.77654,-0.13664,0.006,15,1;"
        "-0.10109636384562,0.95628651080914,0.0065,16,1;"
        "-0.088,0.654,0.012,17,1;"
        "-1.768778833,0.001738996,0.0035,18,1";
    controller.SetPreset(scriptedJourney, false);
    Check(controller.JourneyTargetCount() == 8,
          "Eight valid scripted waypoints should replace the built-in journey candidate list.");

    auto exactJourney = mw::BuiltInPresets().front();
    exactJourney.animationMode = mw::AnimationMode::AutomaticJourney;
    exactJourney.camera = {-0.5, 0.0, 1.5};
    exactJourney.automaticJourneyWaypoints =
        "-0.75,0.10,0.05,1,0.5;0.25,-0.20,0.20,1,0";
    controller.SetPreset(exactJourney, false);
    Check(controller.JourneyTargetCount() == 2,
          "A structured custom journey should contain exactly the supplied destinations.");
    mw::CameraState firstDestination{};
    for (int step = 0; step < 4; ++step) firstDestination = controller.Update(0.25).camera;
    Check(std::abs(firstDestination.centreX - (-0.75)) < 1.0e-12 &&
          std::abs(firstDestination.centreY - 0.10) < 1.0e-12 &&
          std::abs(firstDestination.scale - 0.05) < 1.0e-12,
          "The first custom destination should be reached exactly after its transition time.");
    controller.Update(0.25);
    controller.Update(0.25);
    mw::CameraState secondDestination{};
    for (int step = 0; step < 4; ++step) secondDestination = controller.Update(0.25).camera;
    Check(std::abs(secondDestination.centreX - 0.25) < 1.0e-12 &&
          std::abs(secondDestination.centreY - (-0.20)) < 1.0e-12 &&
          std::abs(secondDestination.scale - 0.20) < 1.0e-12,
          "After the hold, the journey should transition directly to the next exact coordinate.");
}


void TestSafeZoomTarget() {
    Check(!mw::IsInterestingMandelbrotTarget(0.0, 0.0, 500), "The black interior centre should not be accepted as a zoom target.");
    const auto target = mw::FindInterestingMandelbrotTarget(0.0, 0.0, 1.0, 500);
    Check(mw::IsInterestingMandelbrotTarget(target.first, target.second, 500),
          "Automatic target correction should find an escaping boundary-rich point.");
    Check(mw::IsBoundaryRichFractalTarget(target.first, target.second, 1.0, 500, mw::EquationSettings{}),
          "Automatic target correction should reject mostly black or featureless destinations.");

    mw::AnimationController controller;
    auto preset = mw::BuiltInPresets().front();
    preset.animationMode = mw::AnimationMode::ContinuousZoom;
    preset.camera = {0.0, 0.0, 1.0};
    controller.SetPreset(preset, false);
    Check(mw::IsInterestingMandelbrotTarget(controller.Camera().centreX, controller.Camera().centreY, preset.maximumIterations),
          "Continuous zoom should redirect an interior target before zooming.");
}


void TestDeepZoomMath() {
    mw::CameraState camera{-0.743643887037151, 0.131825904205330, 1.0e-24};
    mw::OffsetCamera(camera, 1.0e-24, -2.0e-24);
    Check(camera.centreXLow != 0.0 || camera.centreYLow != 0.0,
          "Compensated camera coordinates should retain sub-ULP pan offsets.");
    const auto doubleOrbit = mw::BuildReferenceOrbitDouble(camera, mw::EquationSettings{}, 128);
    const auto arbitraryOrbit = mw::BuildReferenceOrbitArbitrary(camera, mw::EquationSettings{}, 128, 256);
    Check(doubleOrbit.points.size() == 128, "Double reference orbit should contain every requested iteration.");
    Check(arbitraryOrbit.points.size() == 128, "Arbitrary reference orbit should contain every requested iteration.");
    Check(arbitraryOrbit.precisionBits == 256, "Arbitrary reference orbit should report its precision.");
    Check(std::isfinite(arbitraryOrbit.points.at(20).real[0]) && std::isfinite(arbitraryOrbit.points.at(20).imaginary[0]),
          "Arbitrary reference orbit values should remain finite before escape.");
    Check(mw::EquationSupportsPerturbation(mw::EquationSettings{}),
          "The analytic Mandelbrot equation should support perturbation.");
    auto burningShip = mw::EquationExample(3);
    Check(!mw::EquationSupportsPerturbation(burningShip),
          "Absolute-value recurrences should reject analytic perturbation and use a safe fallback.");
}


void TestStaticSlideshowValidation() {
    mw::AppSettings settings;
    settings.staticWallpaper.enabled = true;
    settings.staticWallpaper.cycleSeconds = 1;
    settings.staticWallpaper.currentIndex = 99;
    settings.staticWallpaper.order = mw::StaticSlideshowOrder::Shuffle;
    settings.staticWallpaper.storageDirectory = "C:/captures";
    settings.staticWallpaper.imagePaths = {
        "C:/captures/one.bmp",
        "C:/captures/one.bmp",
        "C:/captures/two.bmp",
    };
    const auto result = mw::ValidateAndNormalise(settings);
    Check(result.valid, "A valid slideshow should normalise without a fatal validation issue.");
    Check(settings.schemaVersion == 8, "Slideshow settings should use schema version 8.");
    Check(settings.staticWallpaper.cycleSeconds == 10, "Slideshow interval should clamp to ten seconds.");
    Check(settings.staticWallpaper.imagePaths.size() == 2, "Duplicate slideshow paths should be removed.");
    Check(settings.staticWallpaper.currentIndex == 1, "Slideshow current index should clamp to the available list.");
    Check(settings.staticWallpaper.order == mw::StaticSlideshowOrder::Shuffle,
          "Slideshow playback order should survive validation.");
}


void TestAdaptivePerformance() {
    mw::AdaptivePerformanceSettings settings;
    settings.minimumFramesPerSecond = 20.0;
    settings.lowFpsSustainMs = 2000;
    settings.maximumProcessCpuPercent = 50.0;
    settings.highCpuSustainMs = 2000;
    settings.resumeStableMs = 2000;

    mw::AdaptivePerformanceController controller;
    mw::AdaptivePerformanceSample sample;
    sample.rendererActive = true;
    sample.framesPerSecondMeaningful = true;
    sample.framesPerSecond = 10.0;
    for (int i = 0; i < 3; ++i) controller.Update(settings, sample, 1.0);
    const auto lowFpsDecision = controller.Update(settings, sample, 1.0);
    Check(lowFpsDecision.paused, "Sustained low FPS should trigger an adaptive pause after warmup.");

    sample.rendererActive = false;
    sample.framesPerSecondMeaningful = false;
    sample.processCpuPercent = 5.0;
    auto stable = controller.Update(settings, sample, 1.0);
    Check(stable.paused, "Adaptive pause should remain active until the stable cooldown completes.");
    stable = controller.Update(settings, sample, 1.1);
    Check(stable.resumeNow && !stable.paused, "Stable resources should permit one bounded resume probe.");

    controller.Reset();
    sample.rendererActive = true;
    sample.processCpuPercent = 80.0;
    sample.framesPerSecondMeaningful = false;
    controller.Update(settings, sample, 1.0);
    const auto cpuDecision = controller.Update(settings, sample, 1.1);
    Check(cpuDecision.paused, "Sustained high process CPU should trigger an adaptive pause.");
}

void TestInvisibleFrameSuppression() {
    mw::AdaptivePerformanceSettings settings;
    settings.stopWhenVisuallyUnchanged = true;
    settings.minimumVisiblePixelChange = 0.25;
    settings.minimumVisibleColourChange = 0.01;

    mw::VisibleChangeDetector detector;
    mw::VisualFrameDescriptor frame;
    frame.camera = {-0.5, 0.0, 1.5};
    frame.pixelWidth = 1920;
    frame.pixelHeight = 1080;
    frame.contentRevision = 42;
    Check(detector.ShouldRender({frame}, settings), "The first visual frame should render.");
    Check(!detector.ShouldRender({frame}, settings), "An identical frame should skip equation rendering.");
    Check(detector.IsVisuallyIdle(), "Skipped identical frames should report visual idle state.");

    auto subPixel = frame;
    subPixel.camera.centreX += 0.0001;
    Check(!detector.ShouldRender({subPixel}, settings),
          "Sub-threshold movement should remain skipped until it accumulates.");

    auto visible = frame;
    visible.camera.centreX += 0.001;
    Check(detector.ShouldRender({visible}, settings),
          "Accumulated camera movement above the pixel threshold should render.");

    auto colour = visible;
    colour.colourOffset += 0.02;
    Check(detector.ShouldRender({colour}, settings),
          "A visible palette offset change should render.");
}


void TestStillRenderQualityAndTileCamera() {
    mw::StillRenderRequest qualityRequest;
    qualityRequest.preset = mw::BuiltInPresets().front();
    qualityRequest.preset.maximumIterations = 300;
    qualityRequest.preset.camera.scale = 1.0e-12;
    qualityRequest.width = 8192;
    qualityRequest.height = 8192;
    const auto quality = mw::ResolveStillRenderQuality(qualityRequest);
    Check(quality.maximumIterations > qualityRequest.preset.maximumIterations,
          "Deep high-resolution stills should receive a larger automatic iteration budget.");
    Check(quality.maximumIterations <= 4096,
          "Automatic still quality should preserve the renderer iteration safety cap.");
    Check(quality.outputPixelSpan > 0.0 && quality.detailStopsBeyond1080p > 0.0,
          "Automatic still quality should expose the resolved output pixel scale.");

    mw::StillRenderRequest resolutionRequest;
    resolutionRequest.preset = mw::BuiltInPresets().front();
    resolutionRequest.preset.maximumIterations = 300;
    resolutionRequest.preset.camera.scale = 1.5;
    resolutionRequest.width = 1920;
    resolutionRequest.height = 1080;
    const auto normalResolution = mw::ResolveStillRenderQuality(resolutionRequest);
    resolutionRequest.width = 58254;
    resolutionRequest.height = 32768;
    const auto hugeResolution = mw::ResolveStillRenderQuality(resolutionRequest);
    Check(normalResolution.maximumIterations == 300 &&
              hugeResolution.maximumIterations > normalResolution.maximumIterations,
          "Huge output resolution should raise detail depth even at the same camera scale.");

    qualityRequest.scaleQualityToResolution = false;
    const auto fixedQuality = mw::ResolveStillRenderQuality(qualityRequest);
    Check(fixedQuality.maximumIterations == qualityRequest.preset.maximumIterations,
          "Automatic iteration scaling should be explicitly bypassable.");

    mw::CameraState fullCamera;
    fullCamera.centreX = 0.0;
    fullCamera.centreY = 0.0;
    fullCamera.scale = 1.5;
    const auto fullTile = mw::CameraForStillRenderTile(fullCamera, 400, 200, 0, 0, 400, 200);
    Check(std::abs(mw::CameraCentreX(fullTile) - mw::CameraCentreX(fullCamera)) < 1.0e-15 &&
              std::abs(mw::CameraCentreY(fullTile) - mw::CameraCentreY(fullCamera)) < 1.0e-15 &&
              std::abs(fullTile.scale - fullCamera.scale) < 1.0e-15,
          "A full-frame GPU tile should preserve the original camera exactly.");
    const auto topLeft = mw::CameraForStillRenderTile(fullCamera, 400, 200, 0, 0, 200, 100);
    Check(std::abs(mw::CameraCentreX(topLeft) + 1.5) < 1.0e-12,
          "GPU tile camera should preserve the full-frame horizontal viewport.");
    Check(std::abs(mw::CameraCentreY(topLeft) - 0.75) < 1.0e-12,
          "Top-down GPU tiles should map the top half to positive imaginary coordinates.");
    Check(std::abs(topLeft.scale - 0.75) < 1.0e-12,
          "GPU tile camera scale should match its fraction of the full output height.");

    const auto bottomRight = mw::CameraForStillRenderTile(fullCamera, 400, 200, 200, 100, 200, 100);
    Check(std::abs(mw::CameraCentreX(bottomRight) - 1.5) < 1.0e-12 &&
              std::abs(mw::CameraCentreY(bottomRight) + 0.75) < 1.0e-12,
          "Adjacent GPU tile cameras should cover the opposite full-frame quadrant.");

    mw::CameraState deepCamera;
    deepCamera.centreX = 1.0;
    deepCamera.centreY = -0.5;
    deepCamera.scale = 1.0e-18;
    const auto deepLeft = mw::CameraForStillRenderTile(deepCamera, 400, 200, 0, 0, 200, 200);
    Check(deepLeft.centreX == deepCamera.centreX && deepLeft.centreXLow != 0.0,
          "Deep GPU tile offsets should be retained in the compensated centre component.");
}

void TestTiledStillRenderer() {
    mw::StillRenderRequest request;
    request.preset = mw::BuiltInPresets().front();
    request.preset.maximumIterations = 48;
    request.preset.antiAliasingLevel = 2;
    request.preset.equation.animateCoefficients = true;
    request.preset.equation.coefficientAnimationAmplitude = 0.05;
    request.timeSeconds = 1.25;
    request.width = 64;
    request.height = 24;
    request.tileWidth = 11;
    request.previewMaximumWidth = 20;
    request.previewMaximumHeight = 12;

    std::uint32_t writtenRows = 0;
    std::uint64_t writtenPixels = 0;
    std::uint32_t lastProgress = 0;
    mw::StillRenderResult result;
    std::string error;
    const bool rendered = mw::RenderStillImageTiled(
        request,
        [&](std::uint32_t rowIndex, std::span<const std::uint32_t> row,
            std::string&) {
            Check(rowIndex == writtenRows, "Still renderer should write rows in top-down order.");
            Check(row.size() == request.width, "Every still-render scanline should match the requested width.");
            ++writtenRows;
            writtenPixels += row.size();
            return true;
        },
        [&](const mw::StillRenderProgress& progress) {
            Check(progress.completedRows >= lastProgress,
                  "Still-render progress should be monotonic.");
            Check(progress.totalRows == request.height,
                  "Still-render progress should expose the requested row count.");
            lastProgress = progress.completedRows;
        },
        [] { return false; },
        result,
        error);
    Check(rendered, "Tiled still render should complete: " + error);
    Check(writtenRows == request.height, "Tiled still renderer should emit every output row.");
    Check(writtenPixels == static_cast<std::uint64_t>(request.width) * request.height,
          "Tiled still renderer should emit the requested pixel count.");
    Check(lastProgress == request.height, "Still-render progress should finish at the total row count.");
    Check(result.statistics.renderedPixels == writtenPixels,
          "Still-render statistics should report the emitted pixel count.");
    Check(result.statistics.tileWidth == request.tileWidth,
          "Still-render statistics should preserve the bounded tile width.");
    Check(result.statistics.tileHeight == 1U,
          "CPU scanline tiling should report a one-row tile band.");
    Check(result.statistics.maximumIterations == request.preset.maximumIterations &&
              result.statistics.antiAliasingLevel == request.preset.antiAliasingLevel,
          "Ordinary still renders should report their resolved quality budget.");
    Check(result.statistics.peakWorkingPixels <= request.width + request.tileWidth,
          "Tiled rendering should keep full-resolution working memory to one row and one tile.");
    Check(result.statistics.peakWorkingPixels <
              static_cast<std::size_t>(request.width) * request.height,
          "Tiled rendering should not allocate a full-resolution frame.");
    Check(result.preview.width <= request.previewMaximumWidth &&
              result.preview.height <= request.previewMaximumHeight,
          "Still-render preview should remain within its bounded dimensions.");
    Check(result.preview.pixels.size() ==
              static_cast<std::size_t>(result.preview.width) * result.preview.height,
          "Still-render preview should contain a complete bounded image.");

    std::uint32_t cancellationChecks = 0;
    mw::StillRenderResult cancelledResult;
    error.clear();
    const bool cancelled = mw::RenderStillImageTiled(
        request,
        [](std::uint32_t, std::span<const std::uint32_t>, std::string&) { return true; },
        {},
        [&] { return ++cancellationChecks > 5U; },
        cancelledResult,
        error);
    Check(!cancelled, "A cancellation request should stop a still render.");
    Check(error == "Still render cancelled.",
          "A cancelled still render should report a deterministic cancellation result.");

    mw::StillRenderResult failedWriteResult;
    error.clear();
    const bool wroteAllRows = mw::RenderStillImageTiled(
        request,
        [](std::uint32_t rowIndex, std::span<const std::uint32_t>, std::string& writerError) {
            if (rowIndex == 2U) {
                writerError = "Synthetic row writer failure.";
                return false;
            }
            return true;
        },
        {},
        [] { return false; },
        failedWriteResult,
        error);
    Check(!wroteAllRows, "A row-writer failure should stop a still render.");
    Check(error == "Synthetic row writer failure.",
          "A still render should preserve the encoder row-writer error.");
}

void TestDefaults() {
    const auto presets = mw::BuiltInPresets();
    Check(presets.size() >= 10, "At least ten built-in presets should exist.");
    Check(mw::SettingsForProfile(mw::PerformanceProfile::BatterySaver).maximumFrameRate == 15, "Battery Saver FPS should be 15.");
    Check(mw::SettingsForProfile(mw::PerformanceProfile::Balanced).maximumFrameRate == 30, "Balanced FPS should be 30.");
    Check(mw::SettingsForProfile(mw::PerformanceProfile::HighQuality).maximumFrameRate == 60, "High Quality FPS should be 60.");
}

} // namespace

int main() {
    TestKnownPoints();
    TestPresetValidation();
    TestSettingsRoundTrip();
    TestPresetImportSecurity();
    TestCustomEquations();
    TestBuiltInEquationAndPaletteLibraries();
    TestAnimation();
    TestSafeZoomTarget();
    TestDeepZoomMath();
    TestStaticSlideshowValidation();
    TestAdaptivePerformance();
    TestInvisibleFrameSuppression();
    TestStillRenderQualityAndTileCamera();
    TestTiledStillRenderer();
    TestDefaults();
    if (failures == 0) {
        std::cout << "All Mandelbrot core tests passed.\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
