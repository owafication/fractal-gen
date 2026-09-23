#include "Core/AdaptivePerformance.h"
#include "Core/Animation.h"
#include "Core/DeepZoom.h"
#include "Core/FractalScout.h"
#include "Core/FrameSequenceExport.h"
#include "Core/ExternalVideoExport.h"
#include "Core/GeneralAnimation.h"
#include "Core/MandelbrotMath.h"
#include "Core/Models.h"
#include "Core/ProjectState.h"
#include "Core/ProjectHistory.h"
#include "Core/Precision/ExactCameraAdapter.h"
#include "Core/Precision/ExactDecimal.h"
#include "Core/Precision/HighPrecisionBackend.h"
#include "Core/Precision/PrecisionPlanner.h"
#include "Core/Precision/ReferenceOrbitService.h"
#include "Core/SettingsStore.h"
#include "Core/StillImageRenderer.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <complex>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool SameColour(const mw::Colour& left, const mw::Colour& right) {
    return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
}

bool SamePaletteSnapshot(const mw::PaletteParameterSnapshot& left,
                         const mw::PaletteParameterSnapshot& right) {
    if (left.palette != right.palette ||
        left.customPaletteColours.size() != right.customPaletteColours.size() ||
        left.colourOffset != right.colourOffset ||
        left.paletteFrequency != right.paletteFrequency ||
        left.paletteGamma != right.paletteGamma ||
        left.paletteInterpolation != right.paletteInterpolation ||
        left.brightness != right.brightness || left.contrast != right.contrast ||
        left.saturation != right.saturation ||
        !SameColour(left.interiorColour, right.interiorColour) ||
        !SameColour(left.backgroundColour, right.backgroundColour) ||
        left.smoothColouring != right.smoothColouring ||
        left.stripeStrength != right.stripeStrength ||
        left.bloomStrength != right.bloomStrength ||
        left.bloomRadius != right.bloomRadius ||
        left.edgeLightingStrength != right.edgeLightingStrength) {
        return false;
    }
    for (std::size_t index = 0U; index < left.customPaletteColours.size(); ++index) {
        if (!SameColour(left.customPaletteColours[index], right.customPaletteColours[index])) {
            return false;
        }
    }
    return true;
}

void TestProjectStateAdaptersAndFingerprint() {
    Check(mw::Sha256Hex("") ==
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
          "SHA-256 should match the empty-string reference vector.");
    Check(mw::Sha256Hex("abc") ==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA-256 should match the abc reference vector.");
    Check(mw::Sha256Hex(
              "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
          "SHA-256 should match the multi-block reference vector.");

    const auto descriptors = mw::ProjectParameterDescriptors();
    Check(descriptors.size() == 17U,
          "PH-03 should expose the bounded camera, palette and post-processing key set.");
    std::vector<std::string_view> stableNames;
    for (const auto& descriptor : descriptors) {
        Check(!descriptor.stableName.empty(), "Every project parameter needs a stable name.");
        Check(std::find(stableNames.begin(), stableNames.end(), descriptor.stableName) ==
                  stableNames.end(),
              "Project parameter stable names must be unique.");
        Check(mw::ParameterKeyFromStableName(descriptor.stableName) == descriptor.key,
              "Stable parameter names should resolve to their declared key.");
        Check(mw::StableParameterName(descriptor.key) == descriptor.stableName,
              "Project parameter keys should resolve to their stable name.");
        stableNames.push_back(descriptor.stableName);
    }
    Check(!mw::ParameterKeyFromStableName("camera.unknown").has_value(),
          "Unknown project parameter names should fail explicitly.");
    const auto paletteSelection = std::find_if(
        descriptors.begin(), descriptors.end(), [](const mw::ParameterDescriptor& descriptor) {
            return descriptor.key == mw::ParameterKey::PaletteSelection;
        });
    Check(paletteSelection != descriptors.end() &&
              paletteSelection->valueType == mw::ParameterValueType::Integer &&
              !paletteSelection->interpolationSupported && paletteSelection->historyEligible,
          "Palette selection should have stable discrete identity and remain history eligible.");

    mw::Preset source = mw::BuiltInPresets().at(2);
    source.camera.centreX = -0.743643887037151;
    source.camera.centreXLow = 2.6e-17;
    source.camera.centreY = 0.131825904205330;
    source.camera.centreYLow = -1.7e-17;
    source.camera.scale = 1.0e-18;
    source.rotationDegrees = 31.5;
    source.palette = mw::Palette::Ice;
    source.customPaletteColours = {
        {0.1F, 0.2F, 0.3F, 1.0F},
        {0.8F, 0.7F, 0.6F, 1.0F},
    };
    source.colourOffset = 0.375;
    source.paletteFrequency = 19.25;
    source.paletteGamma = 0.85;
    source.paletteInterpolation = mw::PaletteInterpolation::Smoothstep;
    source.brightness = 1.1;
    source.contrast = 1.2;
    source.saturation = 0.9;
    source.interiorColour = {0.05F, 0.04F, 0.03F, 1.0F};
    source.backgroundColour = {0.01F, 0.02F, 0.03F, 1.0F};
    source.smoothColouring = false;
    source.equation.stripeStrength = 0.42;
    source.equation.glowStrength = 0.33;
    source.equation.bloomRadius = 7;
    source.equation.edgeLightingStrength = 0.61;

    const auto camera = mw::CaptureCameraParameters(source);
    const auto palette = mw::CapturePaletteParameters(source);
    mw::Preset restored;
    restored.id = "authority-retained";
    restored.name = "Authority Retained";
    restored.equation.power = 5;
    mw::ApplyCameraParameters(restored, camera);
    mw::ApplyPaletteParameters(restored, palette);
    const auto restoredCamera = mw::CaptureCameraParameters(restored);
    const auto restoredPalette = mw::CapturePaletteParameters(restored);
    Check(restoredCamera.centreX == camera.centreX &&
              restoredCamera.centreXLow == camera.centreXLow &&
              restoredCamera.centreY == camera.centreY &&
              restoredCamera.centreYLow == camera.centreYLow &&
              restoredCamera.scale == camera.scale &&
              restoredCamera.rotationDegrees == camera.rotationDegrees,
          "Camera adapter round trip should preserve compensated high/low components exactly.");
    Check(SamePaletteSnapshot(restoredPalette, palette),
          "Palette adapter round trip should preserve the selected palette and post fields exactly.");
    Check(restored.id == "authority-retained" && restored.equation.power == 5,
          "Applying bounded adapters should not replace unrelated authoritative Preset state.");

    mw::RenderFingerprintContext context;
    context.rendererId = "cpu-production-still";
    context.width = 192U;
    context.height = 108U;
    context.precision.mode = mw::PrecisionMode::Float64;
    context.timeSeconds = 1.25;
    context.seed = 42U;
    mw::RenderFingerprint first;
    std::string error;
    Check(mw::BuildRenderFingerprint(source, context, first, error),
          "Canonical render fingerprint should build for a valid preset: " + error);
    Check(first.algorithm == "SHA-256" && first.canonicalVersion == "mw-render-state-v1" &&
              first.digest.size() == 64U && !first.canonicalState.empty(),
          "Canonical render fingerprint should identify its algorithm, schema and digest.");

    source.exactCamera.reset();
    Check(mw::EnsureExactCamera(source, error),
          "Exact fingerprint fixture should reconstruct canonical exact camera state: " + error);
    mw::RenderFingerprint exactFirst;
    Check(mw::BuildExactRenderFingerprint(source, context, exactFirst, error) &&
              exactFirst.canonicalVersion == "mw-render-state-v2-exact-camera" &&
              exactFirst.digest != first.digest &&
              exactFirst.canonicalState.find("camera.exact-centre-x") != std::string::npos,
          "Exact render identity should use a distinct v2 contract and canonical exact camera text.");
    mw::Preset changedExact = source;
    Check(mw::ExactDecimal::Parse("1e-40", changedExact.exactCamera->halfHeight, error),
          "Exact fingerprint mutation fixture should parse an exact half-height: " + error);
    mw::RenderFingerprint exactChanged;
    Check(mw::BuildExactRenderFingerprint(changedExact, context, exactChanged, error) &&
              exactChanged.digest != exactFirst.digest,
          "A canonical exact camera change should alter the v2 render identity.");
    mw::Preset missingExact = source;
    missingExact.exactCamera.reset();
    Check(!mw::BuildExactRenderFingerprint(missingExact, context, exactChanged, error),
          "Exact render identity should reject missing exact camera state rather than falling back to v1 values.");

    mw::Preset metadataOnly = source;
    metadataOnly.id = "different-id";
    metadataOnly.name = "Different Name";
    metadataOnly.builtIn = !metadataOnly.builtIn;
    metadataOnly.frameRateLimit = 144;
    metadataOnly.zoomSpeed = 0.5;
    metadataOnly.automaticJourneyWaypoints = "metadata-only";
    mw::RenderFingerprint metadataFingerprint;
    Check(mw::BuildRenderFingerprint(metadataOnly, context, metadataFingerprint, error) &&
              metadataFingerprint.canonicalState == first.canonicalState &&
              metadataFingerprint.digest == first.digest,
          "Non-render metadata should not alter the canonical render fingerprint.");

    mw::Preset changedLow = source;
    changedLow.camera.centreXLow += 1.0e-22;
    mw::RenderFingerprint lowFingerprint;
    Check(mw::BuildRenderFingerprint(changedLow, context, lowFingerprint, error) &&
              lowFingerprint.digest != first.digest,
          "A compensated low-component change should alter the render fingerprint.");

    mw::Preset changedPalette = source;
    changedPalette.paletteGamma += 0.01;
    mw::RenderFingerprint paletteFingerprint;
    Check(mw::BuildRenderFingerprint(changedPalette, context, paletteFingerprint, error) &&
              paletteFingerprint.digest != first.digest,
          "A render-affecting palette change should alter the render fingerprint.");

    mw::RenderFingerprintContext changedContext = context;
    changedContext.width += 1U;
    mw::RenderFingerprint contextFingerprint;
    Check(mw::BuildRenderFingerprint(source, changedContext, contextFingerprint, error) &&
              contextFingerprint.digest != first.digest,
          "Output dimensions should be part of the render fingerprint context.");

    mw::Preset negativeZero = source;
    negativeZero.camera.centreXLow = -0.0;
    mw::Preset positiveZero = negativeZero;
    positiveZero.camera.centreXLow = 0.0;
    mw::RenderFingerprint negativeZeroFingerprint;
    mw::RenderFingerprint positiveZeroFingerprint;
    Check(mw::BuildRenderFingerprint(negativeZero, context, negativeZeroFingerprint, error) &&
              mw::BuildRenderFingerprint(positiveZero, context, positiveZeroFingerprint, error) &&
              negativeZeroFingerprint.digest == positiveZeroFingerprint.digest,
          "Equivalent positive and negative zero values should canonicalise identically.");

    mw::Preset invalid = source;
    invalid.paletteGamma = std::numeric_limits<double>::infinity();
    mw::RenderFingerprint rejected;
    Check(!mw::BuildRenderFingerprint(invalid, context, rejected, error) && !error.empty(),
          "Non-finite render state should be rejected rather than fingerprinted.");

    mw::Preset invalidEnum = source;
    invalidEnum.palette = static_cast<mw::Palette>(999);
    Check(!mw::BuildRenderFingerprint(invalidEnum, context, rejected, error) && !error.empty(),
          "Unsupported enum values should be rejected rather than canonicalised as defaults.");

    mw::Preset largestSupportedPalette = source;
    largestSupportedPalette.customPaletteColours.assign(4096U, mw::Colour{});
    Check(mw::BuildRenderFingerprint(largestSupportedPalette, context, rejected, error),
          "Fingerprint limits should preserve the authoritative 4096-stop palette contract.");
    largestSupportedPalette.customPaletteColours.push_back(mw::Colour{});
    Check(!mw::BuildRenderFingerprint(largestSupportedPalette, context, rejected, error),
          "Fingerprinting should reject palettes beyond the authoritative safety limit.");
}

void TestProjectParameterMutationCoordinator() {
    mw::Preset authority = mw::BuiltInPresets().front();
    authority.id = "mutation-authority";
    authority.name = "Mutation Authority";
    authority.camera.centreXLow = 2.5e-20;
    authority.equation.power = 7;
    const mw::CameraState originalCamera = authority.camera;
    const auto originalPalette = authority.palette;

    const std::array<mw::ParameterMutation, 4> mutations{{
        {mw::ParameterKey::Brightness, 2.9},
        {mw::ParameterKey::Contrast, 1.4},
        {mw::ParameterKey::Saturation, 0.7},
        {mw::ParameterKey::PaletteOffset, 0.25},
    }};
    mw::ParameterMutationResult result;
    std::string error;
    Check(mw::ApplyProjectParameterMutations(authority, mutations, result, error),
          "A valid parameter batch should be applied: " + error);
    Check(result.changed && result.normalised && result.changedKeys.size() == 4U,
          "The coordinator should report changed and model-normalised fields.");
    Check(result.origin == mw::ParameterMutationOrigin::UserControl && result.historyEligible,
          "Default user-control mutations should report an eligible origin for future history.");
    Check(mw::HasParameterInvalidation(result.invalidationMask,
                                       mw::ParameterInvalidation::PaletteColouring) &&
              mw::HasParameterInvalidation(result.invalidationMask,
                                           mw::ParameterInvalidation::PostProcessing) &&
              !mw::HasParameterInvalidation(result.invalidationMask,
                                            mw::ParameterInvalidation::CameraViewport),
          "The coordinator should aggregate only the affected invalidation classes.");
    Check(authority.brightness == 2.5 && authority.contrast == 1.4 &&
              authority.saturation == 0.7 && authority.colourOffset == 0.25,
          "The coordinator should use authoritative model bounds for committed values.");
    Check(authority.camera.centreX == originalCamera.centreX &&
              authority.camera.centreXLow == originalCamera.centreXLow &&
              authority.camera.centreY == originalCamera.centreY &&
              authority.camera.centreYLow == originalCamera.centreYLow &&
              authority.camera.scale == originalCamera.scale &&
              authority.palette == originalPalette && authority.equation.power == 7 &&
              authority.id == "mutation-authority" && authority.name == "Mutation Authority",
          "A bounded parameter transaction must preserve unrelated authoritative state.");

    mw::Preset cameraAuthority = authority;
    const auto cameraPalette = cameraAuthority.palette;
    const double cameraBrightness = cameraAuthority.brightness;
    const std::array<mw::ParameterMutation, 5> cameraMutations{{
        {mw::ParameterKey::CameraCentreXHigh, 8.0},
        {mw::ParameterKey::CameraCentreXLow, 4.5e-20},
        {mw::ParameterKey::CameraCentreYHigh, -9.0},
        {mw::ParameterKey::CameraCentreYLow, -3.5e-20},
        {mw::ParameterKey::CameraScale, 1.0e-40},
    }};
    Check(mw::ApplyProjectParameterMutations(cameraAuthority, cameraMutations, result, error),
          "A valid compensated camera batch should be applied: " + error);
    Check(result.changed && result.normalised && result.changedKeys.size() == 5U,
          "The camera transaction should report all requested changed fields and normalisation.");
    Check(cameraAuthority.camera.centreX == 4.0 &&
              cameraAuthority.camera.centreXLow == 4.5e-20 &&
              cameraAuthority.camera.centreY == -4.0 &&
              cameraAuthority.camera.centreYLow == -3.5e-20 &&
              cameraAuthority.camera.scale == 1.0e-32,
          "Camera mutations should preserve compensated low components and use model bounds.");
    mw::ExactDecimal expectedCameraScale;
    Check(mw::ExactDecimal::FromFiniteDouble(cameraAuthority.camera.scale, expectedCameraScale, error) &&
              cameraAuthority.exactCamera.has_value() &&
              cameraAuthority.exactCamera->halfHeight == expectedCameraScale,
          "Committed camera mutations should synchronise retained exact camera state.");
    Check(mw::HasParameterInvalidation(result.invalidationMask,
                                       mw::ParameterInvalidation::CameraViewport) &&
              !mw::HasParameterInvalidation(result.invalidationMask,
                                            mw::ParameterInvalidation::PaletteColouring) &&
              !mw::HasParameterInvalidation(result.invalidationMask,
                                            mw::ParameterInvalidation::PostProcessing),
          "A camera-only transaction should emit only camera viewport invalidation.");
    Check(cameraAuthority.palette == cameraPalette &&
              cameraAuthority.brightness == cameraBrightness,
          "A camera transaction must preserve unrelated palette and post-processing state.");

    const mw::Preset exactCameraBefore = cameraAuthority;
    mw::ExactCamera exactCameraMutation;
    Check(mw::ExactDecimal::Parse("-7.4364388703715100000000000001e-1",
                                  exactCameraMutation.centreX, error) &&
              mw::ExactDecimal::Parse("1.3182590420533000000000000001e-1",
                                      exactCameraMutation.centreY, error) &&
              mw::ExactDecimal::Parse("1e-1000", exactCameraMutation.halfHeight, error) &&
              mw::ApplyProjectExactCameraMutation(cameraAuthority, exactCameraMutation, result, error) &&
              result.changed && result.historyEligible &&
              cameraAuthority.exactCamera == exactCameraMutation &&
              cameraAuthority.camera.scale > 0.0 &&
              mw::HasParameterInvalidation(result.invalidationMask,
                                           mw::ParameterInvalidation::CameraViewport),
          "A parsed exact-camera transaction must preserve ultra-deep text without a legacy round trip: " + error);
    const mw::Preset exactCameraAfter = cameraAuthority;
    mw::ProjectHistory exactCameraHistory;
    mw::ProjectHistoryApplyResult exactCameraHistoryResult;
    Check(exactCameraHistory.RecordParameterMutation(
              exactCameraBefore, exactCameraAfter, result, "Set Exact Camera", error) &&
              exactCameraHistory.EntryCount() == 1U &&
              exactCameraHistory.Undo(cameraAuthority, exactCameraHistoryResult, error) &&
              exactCameraHistoryResult.requiresFullRender &&
              cameraAuthority == exactCameraBefore &&
              exactCameraHistory.Redo(cameraAuthority, exactCameraHistoryResult, error) &&
              exactCameraHistoryResult.requiresFullRender &&
              cameraAuthority == exactCameraAfter &&
              cameraAuthority.exactCamera == exactCameraMutation,
          "An exact-camera edit must undo and redo through an exact structural snapshot: " + error);
    const mw::Preset beforeRejectedExactCamera = cameraAuthority;
    mw::ExactCamera invalidExactCamera = exactCameraMutation;
    Check(mw::ExactDecimal::Parse("-1e-3", invalidExactCamera.halfHeight, error) &&
              !mw::ApplyProjectExactCameraMutation(cameraAuthority, invalidExactCamera, result, error) &&
              cameraAuthority == beforeRejectedExactCamera,
          "An invalid exact-camera transaction must fail without changing authoritative state.");

    const mw::Preset cameraBeforeRejected = cameraAuthority;
    const std::array<mw::ParameterMutation, 2> rejectedCamera{{
        {mw::ParameterKey::CameraCentreXHigh, -0.75},
        {mw::ParameterKey::CameraCentreXLow, std::numeric_limits<double>::infinity()},
    }};
    Check(!mw::ApplyProjectParameterMutations(cameraAuthority, rejectedCamera, result, error) &&
              cameraAuthority.camera.centreX == cameraBeforeRejected.camera.centreX &&
              cameraAuthority.camera.centreXLow == cameraBeforeRejected.camera.centreXLow,
          "A rejected compensated camera batch must roll back every requested field.");

    const std::array<mw::ParameterMutation, 4> noOp{{
        {mw::ParameterKey::Brightness, authority.brightness},
        {mw::ParameterKey::Contrast, authority.contrast},
        {mw::ParameterKey::Saturation, authority.saturation},
        {mw::ParameterKey::PaletteOffset, authority.colourOffset},
    }};
    Check(mw::ApplyProjectParameterMutations(authority, noOp, result, error) &&
              !result.changed && result.invalidationMask == 0U,
          "An equivalent parameter batch should not emit invalidation.");

    const mw::Preset beforeRejected = authority;
    const std::array<mw::ParameterMutation, 2> rejected{{
        {mw::ParameterKey::Contrast, 2.0},
        {mw::ParameterKey::Saturation, std::numeric_limits<double>::infinity()},
    }};
    Check(!mw::ApplyProjectParameterMutations(authority, rejected, result, error) &&
              authority.contrast == beforeRejected.contrast &&
              authority.saturation == beforeRejected.saturation,
          "A rejected batch must leave the authoritative preset unchanged.");

    const std::array<mw::ParameterMutation, 1> wrongType{{
        {mw::ParameterKey::Brightness, std::int64_t{1}},
    }};
    Check(!mw::ApplyProjectParameterMutations(authority, wrongType, result, error),
          "A value whose variant type conflicts with the descriptor must be rejected.");

    const std::array<mw::ParameterMutation, 2> duplicate{{
        {mw::ParameterKey::PaletteOffset, 0.1},
        {mw::ParameterKey::PaletteOffset, 0.2},
    }};
    Check(!mw::ApplyProjectParameterMutations(authority, duplicate, result, error),
          "Duplicate keys in one parameter transaction must be rejected.");

    mw::Preset paletteAuthority = authority;
    paletteAuthority.palette = mw::Palette::DeepOcean;
    paletteAuthority.customPaletteColours = {
        {0.1F, 0.2F, 0.3F, 1.0F},
        {0.8F, 0.7F, 0.6F, 1.0F},
    };
    const auto paletteCamera = paletteAuthority.camera;
    Check(mw::ApplyProjectPaletteSelection(
              paletteAuthority, mw::Palette::Fire, true, result, error,
              {mw::ParameterMutationOrigin::UserControl}),
          "A supported palette selection should be applied transactionally: " + error);
    Check(result.changed && result.changedKeys.size() == 1U &&
              result.changedKeys.front() == mw::ParameterKey::PaletteSelection &&
              result.historyEligible &&
              mw::HasParameterInvalidation(result.invalidationMask,
                                           mw::ParameterInvalidation::PaletteColouring),
          "Palette selection that removes custom stops should remain eligible for PH-05 atomic history.");
    Check(paletteAuthority.palette == mw::Palette::Fire &&
              paletteAuthority.customPaletteColours.empty() &&
              paletteAuthority.camera.centreX == paletteCamera.centreX &&
              paletteAuthority.camera.centreXLow == paletteCamera.centreXLow,
          "Changing the built-in palette should clear custom stops and preserve unrelated camera state.");

    mw::Preset simplePaletteAuthority = authority;
    simplePaletteAuthority.palette = mw::Palette::DeepOcean;
    simplePaletteAuthority.customPaletteColours.clear();
    Check(mw::ApplyProjectPaletteSelection(
              simplePaletteAuthority, mw::Palette::Fire, true, result, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              result.changed && result.historyEligible,
          "A simple discrete palette change should remain eligible for future parameter history.");

    paletteAuthority.customPaletteColours = {
        {0.2F, 0.3F, 0.4F, 1.0F},
        {0.6F, 0.7F, 0.8F, 1.0F},
    };
    Check(mw::ApplyProjectPaletteSelection(
              paletteAuthority, mw::Palette::Fire, true, result, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              !result.changed && !result.historyEligible &&
              paletteAuthority.customPaletteColours.size() == 2U,
          "Re-selecting the current palette should be a no-op and preserve custom stops.");

    const mw::Preset beforeInvalidPalette = paletteAuthority;
    Check(!mw::ApplyProjectPaletteSelection(
              paletteAuthority, static_cast<mw::Palette>(999), true, result, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              paletteAuthority.palette == beforeInvalidPalette.palette &&
              paletteAuthority.customPaletteColours.size() ==
                  beforeInvalidPalette.customPaletteColours.size(),
          "Unsupported palette values should be rejected without partial mutation.");

    const std::array<mw::ParameterMutation, 1> runtimeMutation{{
        {mw::ParameterKey::Contrast, 1.6},
    }};
    Check(mw::ApplyProjectParameterMutations(
              authority, runtimeMutation, result, error,
              {mw::ParameterMutationOrigin::SystemRuntime}) &&
              result.changed && result.origin == mw::ParameterMutationOrigin::SystemRuntime &&
              !result.historyEligible,
          "Runtime-origin mutations should remain explicitly excluded from future project history.");
    Check(mw::IsParameterMutationOriginHistoryEligible(
              mw::ParameterMutationOrigin::UserGesture) &&
              mw::IsParameterMutationOriginHistoryEligible(
                  mw::ParameterMutationOrigin::ScoutApply) &&
              !mw::IsParameterMutationOriginHistoryEligible(
                  mw::ParameterMutationOrigin::UndoRedo) &&
              !mw::IsParameterMutationOriginHistoryEligible(
                  mw::ParameterMutationOrigin::AnimationEvaluation),
          "Mutation-origin eligibility should separate user edits from replay and runtime evaluation.");

    mw::ParameterGestureCoalescer coalescer;
    const auto firstPanToken = coalescer.Begin(mw::ParameterGestureKind::PreviewPan);
    Check(firstPanToken != 0U &&
              coalescer.ActiveToken(mw::ParameterGestureKind::PreviewPan) == firstPanToken,
          "A preview drag should begin one explicit non-zero coalescing sequence.");
    coalescer.End(mw::ParameterGestureKind::PreviewPan);
    const auto secondPanToken = coalescer.Begin(mw::ParameterGestureKind::PreviewPan);
    Check(secondPanToken != firstPanToken,
          "Separate preview drags should never share a coalescing token.");
    coalescer.Reset();
    const auto firstWheelToken = coalescer.ContinueOrBegin(
        mw::ParameterGestureKind::PreviewWheelZoom, 1000U, 250U);
    const auto continuedWheelToken = coalescer.ContinueOrBegin(
        mw::ParameterGestureKind::PreviewWheelZoom, 1240U, 250U);
    const auto expiredWheelToken = coalescer.ContinueOrBegin(
        mw::ParameterGestureKind::PreviewWheelZoom, 1491U, 250U);
    Check(firstWheelToken == continuedWheelToken && expiredWheelToken != firstWheelToken,
          "Wheel zoom events should coalesce only while their monotonic gap stays within policy.");
    const auto rewoundWheelToken = coalescer.ContinueOrBegin(
        mw::ParameterGestureKind::PreviewWheelZoom, 1200U, 250U);
    Check(rewoundWheelToken != expiredWheelToken,
          "A non-monotonic wheel timestamp should start a new coalescing sequence.");

    mw::Preset gestureAuthority = authority;
    const std::array<mw::ParameterMutation, 1> gestureMutation{{
        {mw::ParameterKey::CameraScale, gestureAuthority.camera.scale * 0.5},
    }};
    Check(mw::ApplyProjectParameterMutations(
              gestureAuthority, gestureMutation, result, error,
              {mw::ParameterMutationOrigin::UserGesture,
               mw::ParameterGestureKind::PreviewWheelZoom,
               rewoundWheelToken}) &&
              result.changed && result.historyEligible && result.coalescingEligible &&
              result.coalescingToken == rewoundWheelToken,
          "A bounded preview gesture should propagate its kind and token for future history coalescing.");
    const mw::Preset beforeInvalidGesture = gestureAuthority;
    Check(!mw::ApplyProjectParameterMutations(
              gestureAuthority, gestureMutation, result, error,
              {mw::ParameterMutationOrigin::UserGesture,
               mw::ParameterGestureKind::PreviewPan, 0U}) &&
              gestureAuthority == beforeInvalidGesture,
          "Gesture metadata without a non-zero coalescing token should be rejected transactionally.");

    mw::Preset replacementAuthority = mw::BuiltInPresets().front();
    mw::Preset loadedCandidate = mw::BuiltInPresets().at(1);
    loadedCandidate.camera.scale = 99.0;
    mw::ProjectPresetReplacementResult replacementResult;
    Check(mw::ApplyProjectPresetReplacement(
              replacementAuthority, loadedCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::PresetLoad,
               mw::ProjectPresetReplacementKind::PresetLoad}) &&
              replacementResult.changed && replacementResult.normalised &&
              replacementResult.historyEligible && replacementResult.requiresFullRender &&
              replacementAuthority.camera.scale == 4.0,
          "Preset loads should be classified, normalised and committed as undoable broad replacements.");
    const mw::Preset replacementAfterLoad = replacementAuthority;
    Check(mw::ApplyProjectPresetReplacement(
              replacementAuthority, replacementAfterLoad, replacementResult, error,
              {mw::ParameterMutationOrigin::PresetLoad,
               mw::ProjectPresetReplacementKind::PresetLoad}) &&
              !replacementResult.changed && !replacementResult.requiresFullRender,
          "An equivalent whole-preset replacement should be a no-op.");
    mw::Preset importedCandidate = replacementAuthority;
    importedCandidate.id = "imported-replacement";
    importedCandidate.name = "Imported Replacement";
    Check(mw::ApplyProjectPresetReplacement(
              replacementAuthority, importedCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::Import,
               mw::ProjectPresetReplacementKind::ImportedPreset}) &&
              replacementResult.changed && replacementResult.historyEligible,
          "Imported presets should retain an explicit import replacement origin and enter PH-05 broad history.");
    mw::Preset dialogCandidate = replacementAuthority;
    dialogCandidate.equation.power = 7;
    Check(mw::ApplyProjectPresetReplacement(
              replacementAuthority, dialogCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::UserControl,
               mw::ProjectPresetReplacementKind::EquationDialog}) &&
              replacementAuthority.equation.power == 7,
          "Accepted project dialogs should use an explicit user-control broad replacement boundary.");
    const mw::Preset beforeInvalidReplacement = replacementAuthority;
    Check(!mw::ApplyProjectPresetReplacement(
              replacementAuthority, loadedCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::UserControl,
               mw::ProjectPresetReplacementKind::ImportedPreset}) &&
              replacementAuthority == beforeInvalidReplacement,
          "Mismatched replacement kind and origin should be rejected transactionally.");

    const std::array<mw::ParameterMutation, 1> bloom{{
        {mw::ParameterKey::BloomRadius, std::int64_t{99}},
    }};
    Check(mw::ApplyProjectParameterMutations(authority, bloom, result, error) &&
              authority.equation.bloomRadius == 16 && result.normalised &&
              mw::HasParameterInvalidation(result.invalidationMask,
                                           mw::ParameterInvalidation::PostProcessing),
          "Integer parameters should use the authoritative normaliser and invalidation mapping.");
}


void TestProjectHistory() {
    mw::Preset authority = mw::BuiltInPresets().front();
    authority.camera.centreX = -0.5;
    authority.camera.centreXLow = 1.0e-20;
    authority.camera.centreY = 0.0;
    authority.camera.centreYLow = -2.0e-20;
    authority.camera.scale = 1.5;
    authority.startingScale = 1.5;
    authority.animationMode = mw::AnimationMode::AutomaticJourney;
    authority.exactCamera.reset();
    std::string initialExactError;
    Check(mw::EnsureExactCamera(authority, initialExactError),
          "Test camera setup should reconstruct retained exact state: " + initialExactError);

    mw::ProjectHistory history({3U, 4096U});
    std::string error;
    mw::ParameterMutationResult mutationResult;

    const mw::Preset cameraBefore = authority;
    const std::array<mw::ParameterMutation, 3> firstPan{{
        {mw::ParameterKey::CameraCentreXHigh, -0.6},
        {mw::ParameterKey::CameraCentreYHigh, 0.1},
        {mw::ParameterKey::CameraScale, 1.2},
    }};
    Check(mw::ApplyProjectParameterMutations(
              authority, firstPan, mutationResult, error,
              {mw::ParameterMutationOrigin::UserGesture,
               mw::ParameterGestureKind::PreviewPan, 17U}),
          "A history-bound camera gesture should apply before recording: " + error);
    authority.animationMode = mw::AnimationMode::ManualView;
    Check(history.RecordParameterMutation(cameraBefore, authority, mutationResult,
                                          "Pan Preview", error),
          "A valid camera gesture should be recorded: " + error);
    Check(history.EntryCount() == 1U && history.Cursor() == 1U && history.CanUndo() &&
              !history.CanRedo() && history.UndoLabel() == "Pan Preview",
          "The first camera gesture should create one labelled undo entry.");

    const mw::Preset secondPanBefore = authority;
    const std::array<mw::ParameterMutation, 3> secondPan{{
        {mw::ParameterKey::CameraCentreXHigh, -0.7},
        {mw::ParameterKey::CameraCentreYHigh, 0.2},
        {mw::ParameterKey::CameraScale, 1.0},
    }};
    Check(mw::ApplyProjectParameterMutations(
              authority, secondPan, mutationResult, error,
              {mw::ParameterMutationOrigin::UserGesture,
               mw::ParameterGestureKind::PreviewPan, 17U}) &&
              history.RecordParameterMutation(secondPanBefore, authority, mutationResult,
                                              "Pan Preview", error),
          "A continued camera gesture should apply and record: " + error);
    Check(history.EntryCount() == 1U,
          "Matching gesture kind, token and targets should coalesce into one history entry.");
    const mw::Preset coalescedCameraAfter = authority;

    mw::ProjectHistoryApplyResult applyResult;
    Check(history.Undo(authority, applyResult, error) && applyResult.changed &&
              authority == cameraBefore && history.CanRedo() &&
              history.RedoLabel() == "Pan Preview",
          "Undo should restore exact full state for the bounded camera operation.");
    Check(history.Redo(authority, applyResult, error) && authority == coalescedCameraAfter,
          "Redo should restore exact full state for the final coalesced camera operation.");

    mw::ProjectHistory navigationHistory;
    mw::Preset navigationAuthority = mw::BuiltInPresets().front();
    const mw::Preset navigationBefore = navigationAuthority;
    const std::array<mw::ParameterMutation, 1> navigationFirst{{
        {mw::ParameterKey::CameraCentreXHigh, -0.625},
    }};
    Check(mw::ApplyProjectParameterMutations(
              navigationAuthority, navigationFirst, mutationResult, error,
              {mw::ParameterMutationOrigin::UserGesture,
               mw::ParameterGestureKind::PreviewNavigation, 73U}) &&
              navigationHistory.RecordParameterMutation(
                  navigationBefore, navigationAuthority, mutationResult,
                  "Navigate Preview", error),
          "The first preview-navigation component should be recorded: " + error);
    const mw::Preset navigationSecondBefore = navigationAuthority;
    const std::array<mw::ParameterMutation, 2> navigationSecond{{
        {mw::ParameterKey::CameraCentreYLow, 2.0e-20},
        {mw::ParameterKey::CameraScale, 1.1},
    }};
    Check(mw::ApplyProjectParameterMutations(
              navigationAuthority, navigationSecond, mutationResult, error,
              {mw::ParameterMutationOrigin::UserGesture,
               mw::ParameterGestureKind::PreviewNavigation, 73U}) &&
              navigationHistory.RecordParameterMutation(
                  navigationSecondBefore, navigationAuthority, mutationResult,
                  "Navigate Preview", error) &&
              navigationHistory.EntryCount() == 1U,
          "One preview-navigation token should coalesce changing camera-field subsets.");
    const mw::Preset navigationAfter = navigationAuthority;
    Check(navigationHistory.Undo(navigationAuthority, applyResult, error) &&
              navigationAuthority == navigationBefore &&
              navigationHistory.Redo(navigationAuthority, applyResult, error) &&
              navigationAuthority == navigationAfter,
          "Merged preview navigation should preserve exact undo and redo endpoints.");

    const mw::Preset paletteBefore = authority;
    const std::array<mw::ParameterMutation, 1> paletteOffset{{
        {mw::ParameterKey::PaletteOffset, 0.35},
    }};
    Check(mw::ApplyProjectParameterMutations(
              authority, paletteOffset, mutationResult, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              history.RecordParameterMutation(paletteBefore, authority, mutationResult,
                                              "Palette Offset", error),
          "A palette scalar edit should be recorded: " + error);
    Check(history.EntryCount() == 2U && history.UndoLabel() == "Palette Offset",
          "A discrete palette edit should create a separate history entry.");
    Check(history.Undo(authority, applyResult, error) &&
              authority.colourOffset == paletteBefore.colourOffset,
          "Palette undo should restore the prior scalar value.");

    const mw::Preset branchBefore = authority;
    const std::array<mw::ParameterMutation, 1> branchEdit{{
        {mw::ParameterKey::PaletteOffset, -0.25},
    }};
    Check(mw::ApplyProjectParameterMutations(
              authority, branchEdit, mutationResult, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              history.RecordParameterMutation(branchBefore, authority, mutationResult,
                                              "Palette Offset", error),
          "A new edit after undo should be recordable: " + error);
    Check(!history.CanRedo(), "A new edit after undo must truncate the redo branch.");

    const std::size_t beforeRuntime = history.EntryCount();
    const mw::Preset runtimeBefore = authority;
    const std::array<mw::ParameterMutation, 1> runtimeEdit{{
        {mw::ParameterKey::CameraScale, 0.8},
    }};
    Check(mw::ApplyProjectParameterMutations(
              authority, runtimeEdit, mutationResult, error,
              {mw::ParameterMutationOrigin::AnimationEvaluation}) &&
              history.RecordParameterMutation(runtimeBefore, authority, mutationResult,
                                              "Runtime Camera", error) &&
              history.EntryCount() == beforeRuntime,
          "Runtime and background origins must not enter project history.");

    for (int index = 0; index < 5; ++index) {
        const mw::Preset boundedBefore = authority;
        const std::array<mw::ParameterMutation, 1> boundedEdit{{
            {mw::ParameterKey::PaletteOffset, static_cast<double>(index) / 10.0},
        }};
        Check(mw::ApplyProjectParameterMutations(
                  authority, boundedEdit, mutationResult, error,
                  {mw::ParameterMutationOrigin::UserControl}) &&
                  history.RecordParameterMutation(boundedBefore, authority, mutationResult,
                                                  "Bounded Edit", error),
              "A bounded history edit should record: " + error);
    }
    Check(history.EntryCount() <= 3U && history.Cursor() == history.EntryCount() &&
              history.EstimatedBytes() <= 4096U,
          "History should enforce both configured entry and estimated-memory bounds.");

    mw::ProjectHistory paletteGestureHistory;
    mw::Preset paletteGestureAuthority = mw::BuiltInPresets().front();
    for (const double offset : {0.1, 0.2, 0.3}) {
        const mw::Preset beforeOffset = paletteGestureAuthority;
        const std::array<mw::ParameterMutation, 1> offsetMutation{{
            {mw::ParameterKey::PaletteOffset, offset},
        }};
        Check(mw::ApplyProjectParameterMutations(
                  paletteGestureAuthority, offsetMutation, mutationResult, error,
                  {mw::ParameterMutationOrigin::UserGesture,
                   mw::ParameterGestureKind::PaletteControl, 41U}) &&
                  paletteGestureHistory.RecordParameterMutation(
                      beforeOffset, paletteGestureAuthority, mutationResult,
                      "Adjust Palette Offset", error),
              "A palette-control gesture should be recordable: " + error);
    }
    Check(paletteGestureHistory.EntryCount() == 1U,
          "One palette slider gesture should coalesce to one history entry.");

    mw::ProjectHistory selectionHistory;
    mw::Preset customPaletteAuthority = mw::BuiltInPresets().front();
    customPaletteAuthority.customPaletteColours = {
        {0.1F, 0.2F, 0.3F, 1.0F}, {0.8F, 0.7F, 0.6F, 1.0F},
    };
    const mw::Preset beforeCustomSelection = customPaletteAuthority;
    Check(mw::ApplyProjectPaletteSelection(
              customPaletteAuthority, mw::Palette::Ice, true,
              mutationResult, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              mutationResult.historyEligible &&
              selectionHistory.RecordParameterMutation(
                  beforeCustomSelection, customPaletteAuthority, mutationResult,
                  "Select Palette", error) &&
              selectionHistory.EntryCount() == 1U &&
              selectionHistory.Undo(customPaletteAuthority, applyResult, error) &&
              customPaletteAuthority == beforeCustomSelection &&
              applyResult.requiresFullRender &&
              selectionHistory.Redo(customPaletteAuthority, applyResult, error) &&
              customPaletteAuthority.customPaletteColours.empty() &&
              customPaletteAuthority.palette == mw::Palette::Ice,
          "A palette selection that deletes custom stops should round-trip as one atomic PH-05 entry.");

    mw::ProjectHistory mixedDomainHistory;
    mw::Preset mixedDomainAuthority = mw::BuiltInPresets().front();
    const mw::Preset mixedDomainBefore = mixedDomainAuthority;
    const std::array<mw::ParameterMutation, 2> mixedDomainMutation{{
        {mw::ParameterKey::PaletteOffset, 0.45},
        {mw::ParameterKey::Brightness, 1.25},
    }};
    Check(mw::ApplyProjectParameterMutations(
              mixedDomainAuthority, mixedDomainMutation, mutationResult, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              mixedDomainHistory.RecordParameterMutation(
                  mixedDomainBefore, mixedDomainAuthority, mutationResult,
                  "Mixed Palette/Post Edit", error) &&
              mixedDomainHistory.EntryCount() == 1U &&
              mixedDomainHistory.Undo(mixedDomainAuthority, applyResult, error) &&
              mixedDomainAuthority == mixedDomainBefore &&
              mixedDomainHistory.Redo(mixedDomainAuthority, applyResult, error) &&
              mixedDomainAuthority.colourOffset == 0.45 &&
              mixedDomainAuthority.brightness == 1.25,
          "A mixed palette/post action should be one complete reversible PH-05 entry.");

    mw::ProjectHistory rotationHistory;
    mw::Preset rotationAuthority = mw::BuiltInPresets().front();
    const mw::Preset rotationBefore = rotationAuthority;
    const std::array<mw::ParameterMutation, 1> rotationMutation{{
        {mw::ParameterKey::RotationDegrees, 37.5},
    }};
    Check(mw::ApplyProjectParameterMutations(
              rotationAuthority, rotationMutation, mutationResult, error,
              {mw::ParameterMutationOrigin::UserControl}) &&
              rotationHistory.RecordParameterMutation(
                  rotationBefore, rotationAuthority, mutationResult,
                  "Rotate View", error) &&
              rotationHistory.Undo(rotationAuthority, applyResult, error) &&
              rotationAuthority.rotationDegrees == rotationBefore.rotationDegrees &&
              rotationHistory.Redo(rotationAuthority, applyResult, error) &&
              rotationAuthority.rotationDegrees == 37.5,
          "Registered rotation changes should round-trip through PH-04 history.");

    mw::ProjectHistory scoutHistory;
    mw::Preset scoutAuthority = mw::BuiltInPresets().front();
    const mw::Preset scoutBefore = scoutAuthority;
    const std::array<mw::ParameterMutation, 5> scoutCameraMutations{{
        {mw::ParameterKey::CameraCentreXHigh, -0.743643887037151},
        {mw::ParameterKey::CameraCentreXLow, 2.5e-17},
        {mw::ParameterKey::CameraCentreYHigh, 0.131825904205330},
        {mw::ParameterKey::CameraCentreYLow, -1.5e-17},
        {mw::ParameterKey::CameraScale, 0.000004},
    }};
    Check(mw::ApplyProjectParameterMutations(
              scoutAuthority, scoutCameraMutations, mutationResult, error,
              {mw::ParameterMutationOrigin::ScoutApply}) &&
              mutationResult.historyEligible,
          "A Scout camera choice should apply through the history-eligible coordinator: " + error);
    scoutAuthority.startingScale = scoutAuthority.camera.scale;
    scoutAuthority.animationMode = mw::AnimationMode::ManualView;
    Check(scoutHistory.RecordParameterMutation(
              scoutBefore, scoutAuthority, mutationResult,
              "Apply Scout Camera", error) &&
              scoutHistory.EntryCount() == 1U &&
              scoutHistory.UndoLabel() == "Apply Scout Camera",
          "Applying one Scout result should create exactly one labelled history entry: " + error);
    const mw::Preset scoutAfter = scoutAuthority;
    Check(scoutHistory.Undo(scoutAuthority, applyResult, error) &&
              scoutAuthority == scoutBefore &&
              scoutHistory.Redo(scoutAuthority, applyResult, error) &&
              scoutAuthority == scoutAfter,
          "One Scout result should undo and redo as one exact camera transaction.");

    mw::ProjectHistory structuralHistory;
    mw::Preset structuralAuthority = mw::BuiltInPresets().front();
    structuralAuthority.customPaletteColours = {
        {0.1F, 0.2F, 0.3F, 1.0F},
        {0.4F, 0.5F, 0.6F, 1.0F},
        {0.7F, 0.8F, 0.9F, 1.0F},
    };
    const mw::Preset paletteStructureBefore = structuralAuthority;
    mw::Preset paletteStructureCandidate = structuralAuthority;
    paletteStructureCandidate.customPaletteColours = {
        paletteStructureBefore.customPaletteColours[2],
        {0.9F, 0.2F, 0.1F, 1.0F},
        paletteStructureBefore.customPaletteColours[0],
    };
    mw::ProjectPresetReplacementResult replacementResult;
    Check(mw::ApplyProjectPresetReplacement(
              structuralAuthority, paletteStructureCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::UserControl,
               mw::ProjectPresetReplacementKind::PaletteDialog}) &&
              structuralHistory.RecordPresetReplacement(
                  paletteStructureBefore, structuralAuthority, replacementResult,
                  "Reorder Palette Stops", error) &&
              structuralHistory.EntryCount() == 1U,
          "Palette stop insert/remove/reorder should record as one structural entry: " + error);
    const mw::Preset paletteStructureAfter = structuralAuthority;
    Check(structuralHistory.Undo(structuralAuthority, applyResult, error) &&
              structuralAuthority == paletteStructureBefore && applyResult.requiresFullRender &&
              structuralHistory.Redo(structuralAuthority, applyResult, error) &&
              structuralAuthority == paletteStructureAfter,
          "Palette structural replacement should restore exact ordered stops in both directions.");

    const mw::Preset equationBefore = structuralAuthority;
    mw::Preset equationCandidate = structuralAuthority;
    equationCandidate.equation.power = 9;
    equationCandidate.equation.parameterPower = 3;
    equationCandidate.equation.unaryTransform = mw::EquationUnaryTransform::Cos;
    equationCandidate.equation.glowStrength = 0.62;
    equationCandidate.equation.bloomRadius = 7;
    Check(mw::ApplyProjectPresetReplacement(
              structuralAuthority, equationCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::UserControl,
               mw::ProjectPresetReplacementKind::EquationDialog}) &&
              structuralHistory.RecordPresetReplacement(
                  equationBefore, structuralAuthority, replacementResult,
                  "Edit Equation", error) &&
              structuralHistory.EntryCount() == 2U,
          "A multi-field equation edit should be one cross-dialog transaction: " + error);
    const mw::Preset equationAfter = structuralAuthority;
    Check(structuralHistory.Undo(structuralAuthority, applyResult, error) &&
              structuralAuthority == equationBefore &&
              structuralHistory.Redo(structuralAuthority, applyResult, error) &&
              structuralAuthority == equationAfter,
          "Equation subtree replacement should undo and redo without partial state.");

    const mw::Preset journeyBefore = structuralAuthority;
    mw::Preset journeyCandidate = structuralAuthority;
    journeyCandidate.animationMode = mw::AnimationMode::AutomaticJourney;
    journeyCandidate.automaticJourneyWaypoints =
        "-0.75,0.1,0.02,4,1\n-0.1,0.65,0.001,7,2\n-1.2,0.0,0.08,3,0";
    Check(mw::ApplyProjectPresetReplacement(
              structuralAuthority, journeyCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::UserControl,
               mw::ProjectPresetReplacementKind::JourneyDialog}) &&
              structuralHistory.RecordPresetReplacement(
                  journeyBefore, structuralAuthority, replacementResult,
                  "Edit Journey", error) &&
              structuralHistory.EntryCount() == 3U,
          "Journey row insertion and reorder should record as one structural entry: " + error);
    const mw::Preset journeyAfter = structuralAuthority;
    Check(structuralHistory.Undo(structuralAuthority, applyResult, error) &&
              structuralAuthority == journeyBefore &&
              structuralHistory.Redo(structuralAuthority, applyResult, error) &&
              structuralAuthority == journeyAfter,
          "Journey subtree replacement should preserve exact row order and timing.");

    const mw::Preset loadBefore = structuralAuthority;
    mw::Preset loadCandidate = mw::BuiltInPresets().at(2);
    Check(mw::ApplyProjectPresetReplacement(
              structuralAuthority, loadCandidate, replacementResult, error,
              {mw::ParameterMutationOrigin::PresetLoad,
               mw::ProjectPresetReplacementKind::PresetLoad}) &&
              structuralHistory.RecordPresetReplacement(
                  loadBefore, structuralAuthority, replacementResult, {}, error) &&
              structuralHistory.UndoLabel() == "Load Preset" &&
              structuralHistory.Undo(structuralAuthority, applyResult, error) &&
              structuralAuthority == loadBefore &&
              structuralHistory.Redo(structuralAuthority, applyResult, error) &&
              structuralAuthority == loadCandidate,
          "Preset application should be one labelled reversible project action.");

    mw::ProjectHistory boundedStructuralHistory({8U, 512U});
    mw::Preset oversizedBefore = mw::BuiltInPresets().front();
    mw::Preset oversizedAfter = oversizedBefore;
    oversizedAfter.customPaletteColours.resize(128U);
    replacementResult = {};
    replacementResult.origin = mw::ParameterMutationOrigin::UserControl;
    replacementResult.kind = mw::ProjectPresetReplacementKind::PaletteDialog;
    replacementResult.changed = true;
    replacementResult.historyEligible = true;
    replacementResult.requiresFullRender = true;
    Check(!boundedStructuralHistory.RecordPresetReplacement(
              oversizedBefore, oversizedAfter, replacementResult,
              "Oversized Palette", error) &&
              boundedStructuralHistory.EntryCount() == 0U,
          "One structural snapshot larger than the configured bound should be rejected.");

    mw::ProjectHistory runtimeStructuralHistory;
    replacementResult.origin = mw::ParameterMutationOrigin::SystemRuntime;
    Check(runtimeStructuralHistory.RecordPresetReplacement(
              oversizedBefore, oversizedAfter, replacementResult,
              "Runtime Replacement", error) &&
              runtimeStructuralHistory.EntryCount() == 0U,
          "Runtime structural replacements must remain excluded from history.");

    history.Clear();
    Check(!history.CanUndo() && !history.CanRedo() && history.EntryCount() == 0U &&
              history.EstimatedBytes() == 0U,
          "Clearing history should reset entries, cursor and estimated memory.");
}

void TestModelessEditorCandidateMerges() {
    mw::Preset authoritative = mw::BuiltInPresets().front();
    authoritative.camera = {-0.743643887037151, 0.131825904205330, 1.0e-9,
                            2.5e-17, -1.5e-17};
    authoritative.rotationDegrees = 23.5;
    authoritative.brightness = 1.35;
    authoritative.maximumIterations = 777;
    authoritative.exactCamera.reset();
    std::string error;
    Check(mw::EnsureExactCamera(authoritative, error),
          "The modeless-editor merge fixture should establish exact authoritative camera state: " + error);

    mw::Preset paletteCandidate = mw::BuiltInPresets().at(1);
    paletteCandidate.customPaletteColours = {
        {0.1F, 0.2F, 0.3F, 1.0F}, {0.8F, 0.7F, 0.6F, 1.0F},
    };
    paletteCandidate.paletteFrequency = 17.0;
    paletteCandidate.paletteGamma = 0.75;
    paletteCandidate.paletteInterpolation = mw::PaletteInterpolation::Smoothstep;
    paletteCandidate.equation.stripeAverageEnabled = true;
    paletteCandidate.equation.stripeDensity = 13.0;
    paletteCandidate.equation.stripePhase = 0.4;
    paletteCandidate.equation.stripeStrength = 0.8;
    paletteCandidate.equation.stripeStartIteration = 9;
    const mw::Preset paletteMerged = mw::MergePaletteEditorCandidate(
        authoritative, paletteCandidate);
    Check(paletteMerged.camera == authoritative.camera &&
              paletteMerged.exactCamera == authoritative.exactCamera &&
              paletteMerged.rotationDegrees == authoritative.rotationDegrees &&
              paletteMerged.brightness == authoritative.brightness &&
              paletteMerged.maximumIterations == authoritative.maximumIterations,
          "A palette-editor change must preserve camera, exact camera and unrelated live project fields.");
    Check(paletteMerged.customPaletteColours == paletteCandidate.customPaletteColours &&
              paletteMerged.paletteFrequency == paletteCandidate.paletteFrequency &&
              paletteMerged.paletteGamma == paletteCandidate.paletteGamma &&
              paletteMerged.paletteInterpolation == paletteCandidate.paletteInterpolation &&
              paletteMerged.equation.stripeAverageEnabled &&
              paletteMerged.equation.stripeDensity == paletteCandidate.equation.stripeDensity &&
              paletteMerged.equation.stripePhase == paletteCandidate.equation.stripePhase &&
              paletteMerged.equation.stripeStrength == paletteCandidate.equation.stripeStrength &&
              paletteMerged.equation.stripeStartIteration ==
                  paletteCandidate.equation.stripeStartIteration,
          "A palette-editor merge must apply every field owned by that editor.");

    mw::Preset equationCandidate = mw::BuiltInPresets().at(2);
    equationCandidate.equation.power = 7;
    equationCandidate.equation.glowStrength = 0.9;
    equationCandidate.equation.animateCoefficients = true;
    const mw::Preset equationMerged = mw::MergeEquationEditorCandidate(
        authoritative, equationCandidate);
    Check(equationMerged.camera == authoritative.camera &&
              equationMerged.exactCamera == authoritative.exactCamera &&
              equationMerged.rotationDegrees == authoritative.rotationDegrees &&
              equationMerged.customPaletteColours == authoritative.customPaletteColours &&
              equationMerged.maximumIterations == authoritative.maximumIterations,
          "An equation-editor change must preserve camera, exact camera and unrelated live project fields.");
    Check(equationMerged.equation == equationCandidate.equation,
          "An equation-editor merge must apply the complete equation subtree.");
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
    settings.monitorMode = mw::MonitorMode::Span;
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
    custom.equation = mw::EquationExample(9);
    custom.equation.power = 3;
    custom.equation.parameterPower = 2;
    custom.equation.reciprocalPower = 1;
    custom.equation.reciprocalCoefficient = {0.2, 0.0};
    custom.equation.initialZ = {0.1, -0.1};
    custom.equation.unaryTransform = mw::EquationUnaryTransform::Sin;
    custom.equation.stripeAverageEnabled = true;
    custom.equation.stripeDensity = 9.0;
    custom.equation.stripePhase = 0.3;
    custom.equation.stripeStrength = 0.18;
    custom.equation.stripeStartIteration = 11;
    custom.equation.glowStrength = 0.31;
    custom.equation.edgeLightingStrength = 0.47;
    custom.equation.bloomThreshold = 0.64;
    custom.equation.bloomSoftKnee = 0.12;
    custom.equation.bloomRadius = 5;
    custom.rotationDegrees = 37.5;
    custom.paletteFrequency = 28.0;
    custom.paletteGamma = 0.85;
    custom.paletteInterpolation = mw::PaletteInterpolation::Smoothstep;
    custom.automaticJourneyWaypoints = "-0.743643887037151,0.131825904205330,0.004,12,1;0.285,0.01,0.028,14,2";
    settings.customPresets.push_back(custom);
    settings.customPalettePresets.push_back({
        "palette-test", "Palette Test",
        {{0.9F, 0.1F, 0.2F, 1.0F}, {0.1F, 0.3F, 0.9F, 1.0F}, {0.2F, 0.9F, 0.4F, 1.0F}}
    });
    settings.customEquationPresets.push_back({"equation-test", "Equation Test", custom.equation});
    settings.general.defaultDesktopMode = mw::DesktopMode::Video;
    settings.videoWallpaper.filePath = "C:/renders/animation.mp4";
    settings.staticWallpaper.enabled = true;
    settings.staticWallpaper.cycleEnabled = true;
    settings.staticWallpaper.cycleSeconds = 45;
    settings.staticWallpaper.order = mw::StaticSlideshowOrder::Shuffle;
    settings.staticWallpaper.storageDirectory = "C:/renders";
    settings.staticWallpaper.savedImageFormat = mw::SavedImageFormat::Jpeg;
    settings.staticWallpaper.compressionQuality = 73;
    settings.staticWallpaper.imagePaths = {"C:/renders/one.bmp", "C:/renders/two.bmp"};
    settings.staticWallpaper.currentIndex = 1;

    mw::ExactCamera persistedExact;
    std::string exactCameraError;
    Check(mw::ExactDecimal::Parse("-7.4364388703715100000000000001e-1",
                                  persistedExact.centreX, exactCameraError) &&
              mw::ExactDecimal::Parse("1.3182590420533000000000000001e-1",
                                      persistedExact.centreY, exactCameraError) &&
              mw::ExactDecimal::Parse("6.5e-3", persistedExact.halfHeight,
                                      exactCameraError),
          "Schema-3 round-trip fixture should construct exact camera text: " + exactCameraError);
    custom.exactCamera = persistedExact;
    settings.customPresets.front().exactCamera = persistedExact;

    std::string legacyPreset = mw::SettingsStore::SerialisePreset(custom);
    Check(legacyPreset.find("\"schemaVersion\": 3") != std::string::npos &&
              legacyPreset.find("\"exactCamera\"") != std::string::npos,
          "New preset writes must use schema 3 with canonical exact camera text.");
    mw::Preset ultraDeepPreset = custom;
    mw::ExactCamera ultraDeepExact = persistedExact;
    Check(mw::ExactDecimal::Parse("1e-1000", ultraDeepExact.halfHeight, exactCameraError),
          "Ultra-deep schema fixture should parse exact half-height text: " + exactCameraError);
    ultraDeepPreset.exactCamera = ultraDeepExact;
    const std::string ultraDeepPresetText = mw::SettingsStore::SerialisePreset(ultraDeepPreset);
    const auto reloadedUltraDeepPreset =
        mw::SettingsStore::DeserialisePreset(ultraDeepPresetText, exactCameraError);
    Check(reloadedUltraDeepPreset.has_value(),
          "A schema-3 preset below double scale range must parse: " + exactCameraError);
    if (reloadedUltraDeepPreset) {
        Check(reloadedUltraDeepPreset->exactCamera == ultraDeepExact &&
                  std::isfinite(reloadedUltraDeepPreset->camera.scale) &&
                  reloadedUltraDeepPreset->camera.scale > 0.0,
              "A schema-3 preset below double scale range must reload with its exact camera intact.");
    }
    const std::size_t edgeKey = legacyPreset.find("\"edgeLightingStrength\"");
    if (edgeKey != std::string::npos) {
        const std::size_t lineStart = legacyPreset.rfind('\n', edgeKey);
        const std::size_t lineEnd = legacyPreset.find('\n', edgeKey);
        if (lineStart != std::string::npos && lineEnd != std::string::npos) {
            legacyPreset.erase(lineStart + 1U, lineEnd - lineStart);
        }
    }
    std::string legacyError;
    const auto migratedLegacyPreset =
        mw::SettingsStore::DeserialisePreset(legacyPreset, legacyError);
    Check(migratedLegacyPreset.has_value(),
          "A pre-1.12.1 preset without edgeLightingStrength should migrate: " + legacyError);
    if (migratedLegacyPreset) {
        Check(migratedLegacyPreset->equation.edgeLightingStrength ==
                  migratedLegacyPreset->equation.glowStrength,
              "Legacy glow should migrate once to the independent edge-light field.");
    }

    std::string migrationError;
    const auto migratedLaunchMode = mw::SettingsStore::DeserialiseSettings(
        R"json({"schemaVersion":8,"general":{"startWallpaperOnLaunch":true}})json",
        migrationError);
    Check(migratedLaunchMode.has_value(),
          "Legacy startWallpaperOnLaunch settings should migrate: " + migrationError);
    if (migratedLaunchMode) {
        Check(migratedLaunchMode->general.defaultDesktopMode == mw::DesktopMode::None,
              "The legacy live launch checkbox should migrate fail-closed to no desktop mode.");
    }
    const auto migratedRemovedMode = mw::SettingsStore::DeserialiseSettings(
        R"json({"schemaVersion":10,"general":{"defaultDesktopMode":"journey"}})json",
        migrationError);
    Check(migratedRemovedMode.has_value() &&
              migratedRemovedMode->general.defaultDesktopMode == mw::DesktopMode::None,
          "Removed continuously rendered desktop modes should migrate to None: " + migrationError);
    const auto migratedIndependentMonitor = mw::SettingsStore::DeserialiseSettings(
        R"json({"schemaVersion":11,"monitorMode":"independent","monitorPresetAssignments":{"DISPLAY1":"custom-test"}})json",
        migrationError);
    Check(migratedIndependentMonitor.has_value() &&
              migratedIndependentMonitor->schemaVersion == 12 &&
              migratedIndependentMonitor->monitorMode == mw::MonitorMode::Mirror,
          "Schema-11 independent monitor assignments should migrate to Mirror: " + migrationError);
    if (migratedIndependentMonitor) {
        Check(mw::SettingsStore::SerialiseSettings(*migratedIndependentMonitor)
                  .find("monitorPresetAssignments") == std::string::npos,
              "Schema-12 settings must discard removed monitor assignment data.");
    }

    const auto serialised = mw::SettingsStore::SerialiseSettings(settings);
    std::string error;
    const auto parsed = mw::SettingsStore::DeserialiseSettings(serialised, error);
    Check(parsed.has_value(), "Settings round trip should parse: " + error);
    if (parsed) {
        Check(parsed->schemaVersion == 12, "Settings should migrate to schema version 12.");
        Check(parsed->general.reducedMotion, "Reduced-motion setting should persist.");
        Check(!parsed->general.colourCyclingEnabled, "Colour-cycling play/pause state should persist.");
        Check(parsed->general.defaultDesktopMode == mw::DesktopMode::Video,
              "The selected default desktop mode should persist.");
        Check(parsed->videoWallpaper.filePath == settings.videoWallpaper.filePath,
              "The selected exported-video wallpaper path should persist.");
        Check(parsed->monitorMode == mw::MonitorMode::Span, "Monitor mode should persist.");
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
        Check(parsed->customPresets.front().exactCamera.has_value() &&
                  parsed->customPresets.front().exactCamera == persistedExact,
              "Schema-3 exact camera text should survive a settings round trip unchanged.");
        Check(parsed->customPresets.front().equation.power == 3 &&
              parsed->customPresets.front().equation.parameterPower == 2 &&
              parsed->customPresets.front().equation.reciprocalPower == 1 &&
              parsed->customPresets.front().equation.juliaMode,
              "Advanced equation options, including c powers, should persist.");
        Check(parsed->customPresets.front().automaticJourneyWaypoints == custom.automaticJourneyWaypoints,
              "Custom Automatic Journey waypoint text should persist.");
        Check(parsed->customPresets.front().paletteFrequency == 28.0 &&
              parsed->customPresets.front().paletteGamma == 0.85 &&
              parsed->customPresets.front().paletteInterpolation == mw::PaletteInterpolation::Smoothstep,
              "Palette frequency, gamma and interpolation should persist.");
        Check(parsed->customPresets.front().equation.stripeAverageEnabled &&
              parsed->customPresets.front().equation.stripeDensity == 9.0 &&
              parsed->customPresets.front().equation.stripePhase == 0.3 &&
              parsed->customPresets.front().equation.stripeStrength == 0.18 &&
              parsed->customPresets.front().equation.stripeStartIteration == 11,
              "Stripe-average colouring controls should persist.");
        Check(parsed->customPresets.front().equation.glowStrength == 0.31 &&
              parsed->customPresets.front().equation.edgeLightingStrength == 0.47,
              "Bloom and mathematical edge-light strengths should persist independently.");
        Check(parsed->customPresets.front().equation.bloomThreshold == 0.64 &&
              parsed->customPresets.front().equation.bloomSoftKnee == 0.12 &&
              parsed->customPresets.front().equation.bloomRadius == 5,
              "Bloom threshold, soft knee and blur radius should persist.");
        Check(parsed->customPresets.front().rotationDegrees == 37.5,
              "Camera rotation should persist with a preset.");
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
        Check(parsed->staticWallpaper.savedImageFormat == mw::SavedImageFormat::Jpeg,
              "The saved-image format should persist.");
        Check(parsed->staticWallpaper.compressionQuality == 73,
              "The saved-image compression/quality setting should persist.");
        Check(parsed->staticWallpaper.imagePaths.size() == 2, "Static wallpaper image history should persist.");
        Check(parsed->staticWallpaper.currentIndex == 1, "Static wallpaper index should persist.");
    }

    const std::filesystem::path migrationPath =
        std::filesystem::temp_directory_path() / "mw-schema12-forward-migration-test.json";
    std::error_code migrationCleanupError;
    std::filesystem::remove(migrationPath, migrationCleanupError);
    mw::AppSettings legacySettings = settings;
    legacySettings.schemaVersion = 9;
    {
        std::ofstream legacyFile(migrationPath, std::ios::binary | std::ios::trunc);
        legacyFile << mw::SettingsStore::SerialiseSettings(legacySettings);
    }
    mw::SettingsStore migrationStore(migrationPath);
    const auto migration = migrationStore.Load();
    Check(migration.migrated && migration.settings.schemaVersion == 12 &&
              !migration.migrationBackupPath.empty() &&
              std::filesystem::exists(migration.migrationBackupPath),
          "Schema-9 settings should preserve an original backup before forward schema-12 promotion.");
    std::ifstream promotedFile(migrationPath, std::ios::binary);
    std::stringstream promotedText;
    promotedText << promotedFile.rdbuf();
    const auto promotedSettings = mw::SettingsStore::DeserialiseSettings(promotedText.str(), error);
    Check(promotedSettings.has_value() && promotedSettings->schemaVersion == 12 &&
              promotedSettings->customPresets.front().exactCamera == persistedExact,
          "Forward-promoted settings should re-read as schema 12 without losing exact camera text.");
    std::filesystem::remove(migrationPath, migrationCleanupError);
    if (!migration.migrationBackupPath.empty()) {
        std::filesystem::remove(migration.migrationBackupPath, migrationCleanupError);
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

    const auto scaledC = mw::EquationExample(7);
    Check(!mw::CalculateEscape(0.0, 0.0, 100, scaledC).escaped,
          "z = z + 1.2c should remain at zero when c is zero.");
    Check(mw::CalculateEscape(2.0, 0.0, 100, scaledC).escaped,
          "z = z + 1.2c should escape for a large positive c.");

    const auto rational = mw::EquationExample(8);
    Check(rational.reciprocalPower == 1 && rational.reciprocalCoefficient.real == 0.25,
          "Rational-map examples should configure bounded reciprocal powers.");
    Check(mw::CalculateEscape(0.0, 0.0, 100, rational).escaped,
          "The rational map should safely handle a critical orbit near a pole.");

    const auto julia = mw::EquationExample(9);
    Check(julia.juliaMode && std::abs(julia.juliaParameter.real + 0.8) < 1.0e-9,
          "Julia examples should configure a fixed complex parameter.");
    Check(mw::CalculateEscape(2.0, 2.0, 100, julia).escaped,
          "Julia mode should use the pixel as z0 and escape far-away points.");

    auto transformed = mw::EquationExample(11);
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

    auto stripedTricorn = mw::EquationExample(4);
    stripedTricorn.stripeAverageEnabled = true;
    stripedTricorn.stripeDensity = 9.0;
    stripedTricorn.stripePhase = 0.25;
    stripedTricorn.stripeStartIteration = 2;
    const auto stripeResult = mw::CalculateEscape(0.5, 0.5, 300, stripedTricorn);
    Check(stripeResult.stripeAverage >= 0.0 && stripeResult.stripeAverage <= 1.0 &&
              std::isfinite(stripeResult.stripeAverage),
          "Stripe-average colouring should return a finite normalised orbit texture value.");

    auto iterationEquation = classic;
    iterationEquation.iterationTerm = {0.02, -0.01};
    Check(mw::CalculateEscape(0.0, 0.0, 200, iterationEquation).escaped,
          "Iteration-dependent terms should change an otherwise bounded orbit.");

    const auto newton = mw::EquationExample(10);
    const auto newtonResult = mw::CalculateEscape(0.5, 0.5, 80, newton);
    Check(newton.newtonMode && newtonResult.converged && newtonResult.rootIndex >= 0,
          "Newton mode should converge and identify a root basin.");

    const auto orbitTrap = mw::EquationExample(14);
    const auto trapResult = mw::CalculateEscape(0.5, 0.5, 200, orbitTrap);
    Check(std::isfinite(trapResult.orbitTrapDistance),
          "Orbit-trap rendering should collect a finite trap distance.");

    const auto distance = mw::EquationExample(15);
    const auto distanceResult = mw::CalculateEscape(0.5, 0.5, 200, distance);
    Check(distanceResult.distanceEstimate >= 0.0 && std::isfinite(distanceResult.distanceEstimate),
          "Distance-estimation rendering should return a safe non-negative estimate.");
    Check(std::abs(distanceResult.distanceEstimate - 0.044521402776881977) < 1.0e-14,
          "Existing analytic Mandelbrot distance-estimation output should remain unchanged.");

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


std::complex<double> IterateTricorn(std::complex<double> c, int iterations) {
    std::complex<double> z{};
    for (int i = 0; i < iterations; ++i) z = std::conj(z) * std::conj(z) + c;
    return z;
}

double FiniteDifferenceTricornDistance(std::complex<double> c, int iterations) {
    constexpr double epsilon = 1.0e-6;
    const std::complex<double> z = IterateTricorn(c, iterations);
    const std::complex<double> derivativeX =
        (IterateTricorn(c + epsilon, iterations) -
         IterateTricorn(c - epsilon, iterations)) / (2.0 * epsilon);
    const std::complex<double> derivativeY =
        (IterateTricorn(c + std::complex<double>{0.0, epsilon}, iterations) -
         IterateTricorn(c - std::complex<double>{0.0, epsilon}, iterations)) /
        (2.0 * epsilon);
    const double trace = std::norm(derivativeX) + std::norm(derivativeY);
    const double determinant = derivativeX.real() * derivativeY.imag() -
                               derivativeY.real() * derivativeX.imag();
    const double discriminant = std::max(
        0.0, trace * trace - 4.0 * determinant * determinant);
    const double stretch = std::sqrt(
        0.5 * (trace + std::sqrt(discriminant)));
    const double magnitude = std::abs(z);
    return 0.5 * std::log(magnitude) * magnitude / stretch;
}

void TestConjugateDistanceEstimation() {
    auto tricorn = mw::EquationExample(4);
    tricorn.colouringMethod = mw::ColouringMethod::DistanceEstimation;
    Check(mw::SupportsConjugateDistanceEstimation(tricorn),
          "The exact power-2 Tricorn profile should enable conjugate distance estimation.");

    constexpr double real = 1.0;
    constexpr double imaginary = 0.2;
    const auto result = mw::CalculateEscape(real, imaginary, 500, tricorn);
    Check(result.escaped && result.distanceEstimate > 0.0 &&
              std::isfinite(result.distanceEstimate),
          "Power-2 Tricorn distance estimation should return a finite positive value outside the set.");
    const double finiteDifference = FiniteDifferenceTricornDistance(
        {real, imaginary}, result.iterations);
    const double relativeError = std::abs(result.distanceEstimate - finiteDifference) /
                                 std::max(finiteDifference, 1.0e-12);
    Check(relativeError < 2.0e-5,
          "The Tricorn Jacobian distance should match a finite-difference reference sample.");

    auto unsupported = tricorn;
    unsupported.power = 3;
    Check(!mw::SupportsConjugateDistanceEstimation(unsupported),
          "Higher Multicorn powers should remain outside the validated Phase 2 support profile.");
    const auto unsupportedResult = mw::CalculateEscape(real, imaginary, 500, unsupported);
    Check(unsupportedResult.distanceEstimate == 0.0,
          "Unsupported conjugate formulas should use the safe direct path without a fabricated distance estimate.");

    const auto renderEdgePixel = [&](double edgeStrength) {
        mw::StillRenderRequest request;
        request.width = 1;
        request.height = 1;
        request.tileWidth = 1;
        request.previewMaximumWidth = 0;
        request.previewMaximumHeight = 0;
        request.scaleQualityToResolution = false;
        request.preset.id = "tricorn-edge-test";
        request.preset.name = "Tricorn Edge Test";
        request.preset.camera = {0.5, 0.5, 1.0e-6};
        request.preset.maximumIterations = 500;
        request.preset.antiAliasingLevel = 1;
        request.preset.equation = tricorn;
        request.preset.equation.edgeLightingStrength = edgeStrength;
        request.preset.equation.glowStrength = 0.0;
        request.preset.customPaletteColours = {
            {0.1F, 0.2F, 0.3F, 1.0F}, {0.2F, 0.3F, 0.4F, 1.0F}};
        mw::StillRenderResult renderResult;
        std::string renderError;
        std::uint32_t pixel = 0;
        const bool rendered = mw::RenderStillImageTiled(
            request,
            [&](std::uint32_t, std::span<const std::uint32_t> row,
                std::string&) {
                pixel = row.front();
                return true;
            },
            {}, {}, renderResult, renderError);
        Check(rendered, "The CPU Tricorn edge-light fixture should render: " + renderError);
        return pixel;
    };
    Check(renderEdgePixel(0.0) != renderEdgePixel(1.0),
          "Tricorn mathematical edge lighting should change CPU output while bloom remains disabled.");
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

    const auto tricorn = mw::EquationExample(4);
    const auto cubicMulticorn = mw::EquationExample(5);
    const auto quarticMulticorn = mw::EquationExample(6);
    Check(tricorn.conjugate && tricorn.power == 2 &&
              cubicMulticorn.conjugate && cubicMulticorn.power == 3 &&
              quarticMulticorn.conjugate && quarticMulticorn.power == 4,
          "The equation template library should expose Tricorn and higher-order Multicorn families.");

    const auto scaledC = mw::EquationExample(16);
    Check(std::abs(scaledC.parameter.real - 1.2) < 1.0e-12,
          "The reference scaled-c equation should use 1.2c.");
    const auto constantAdd = mw::EquationExample(17);
    Check(std::abs(constantAdd.constant.real - 0.5) < 1.0e-12,
          "The reference constant-add equation should add 0.5.");
    const auto scaledZ = mw::EquationExample(18);
    Check(std::abs(scaledZ.quadratic.real - 1.2) < 1.0e-12,
          "The reference scaled-z equation should use 1.2z squared.");
    const auto addToZ = mw::EquationExample(19);
    Check(std::abs(addToZ.linear.real - 0.5) < 1.0e-12,
          "The reference add-to-z equation should use 0.5z.");
    const auto swapped = mw::EquationExample(20);
    Check(swapped.parameterPower == 2 && std::abs(swapped.linear.real - 1.0) < 1.0e-12 &&
              std::abs(swapped.quadratic.real) < 1.0e-12,
          "The reference swap equation should be represented exactly as z + c squared.");
    Check(mw::EquationSummary(swapped).find("c^2") != std::string::npos,
          "Equation summaries should expose powered c terms.");
    Check(mw::CalculateEscape(2.0, 0.0, 20, swapped).iterations <
              mw::CalculateEscape(2.0, 0.0, 20, mw::EquationExample(0)).iterations,
          "The c-squared recurrence should be evaluated rather than treated as a linear c term.");
    const auto absoluteZ = mw::EquationExample(21);
    Check(absoluteZ.absoluteReal && absoluteZ.absoluteImaginary,
          "The reference absolute-z equation should apply absolute values to both components.");
    const auto minusC = mw::EquationExample(22);
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
    const auto cyanFireScene = std::find_if(scenes.begin(), scenes.end(), [](const mw::Preset& scene) {
        return scene.id == "tricorn-cyan-fire-ring";
    });
    Check(cyanFireScene != scenes.end() && cyanFireScene->equation.conjugate &&
              cyanFireScene->paletteFrequency == 28.0 &&
              cyanFireScene->paletteInterpolation == mw::PaletteInterpolation::Smoothstep &&
              cyanFireScene->equation.stripeAverageEnabled &&
              cyanFireScene->equation.colouringMethod == mw::ColouringMethod::DistanceEstimation &&
              cyanFireScene->equation.edgeLightingStrength == 0.65 &&
              cyanFireScene->equation.glowStrength == 0.30 &&
              cyanFireScene->antiAliasingLevel == 4,
          "The tuned Tricorn Cyan Fire Ring scene should include independent distance edge lighting and bloom.");
}


void TestGeneralAnimationEvaluator() {
    mw::Preset base = mw::BuiltInPresets().front();
    base.camera.centreX = -0.743643887037151;
    base.camera.centreXLow = 1.0e-20;
    base.camera.centreY = 0.131825904205330;
    base.camera.centreYLow = -2.0e-20;
    base.camera.scale = 1.0;
    base.rotationDegrees = 170.0;
    base.colourOffset = 0.0;
    base.equation.quadratic.real = -1.0;
    base.equation.power = 2;
    mw::NormaliseCamera(base.camera);
    const mw::Preset originalBase = base;

    mw::AnimationTimeline timeline;
    timeline.id = "timeline-main";
    timeline.durationSeconds = 10.0;
    timeline.loopMode = mw::AnimationLoopMode::Clamp;

    mw::AnimationTrack centreTrack;
    centreTrack.id = "track-centre-x";
    centreTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::CameraCentreX));
    centreTrack.keyframes = {
        {"key-centre-start", 0.0,
         mw::CompensatedAnimationValue{-0.743643887037151, 1.0e-20},
         mw::AnimationInterpolation::Linear},
        {"key-centre-end", 10.0,
         mw::CompensatedAnimationValue{-0.743643887037151, 9.0e-20},
         mw::AnimationInterpolation::Linear},
    };

    mw::AnimationTrack scaleTrack;
    scaleTrack.id = "track-scale";
    scaleTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::CameraScale));
    scaleTrack.keyframes = {
        {"key-scale-start", 0.0, 1.0, mw::AnimationInterpolation::Linear},
        {"key-scale-end", 10.0, 1.0e-8, mw::AnimationInterpolation::Linear},
    };

    mw::AnimationTrack rotationTrack;
    rotationTrack.id = "track-rotation";
    rotationTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::RotationDegrees));
    rotationTrack.keyframes = {
        {"key-rotation-start", 0.0, 170.0, mw::AnimationInterpolation::Linear},
        {"key-rotation-end", 10.0, -170.0, mw::AnimationInterpolation::Linear},
    };

    mw::AnimationTrack paletteTrack;
    paletteTrack.id = "track-palette-offset";
    paletteTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::PaletteOffset));
    paletteTrack.keyframes = {
        {"key-palette-start", 0.0, 0.0, mw::AnimationInterpolation::Smoothstep},
        {"key-palette-end", 10.0, 1.0, mw::AnimationInterpolation::Smoothstep},
    };

    mw::AnimationTrack equationRealTrack;
    equationRealTrack.id = "track-equation-real";
    equationRealTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::EquationQuadraticReal));
    equationRealTrack.keyframes = {
        {"key-equation-real-start", 0.0, -1.0, mw::AnimationInterpolation::Linear},
        {"key-equation-real-end", 10.0, 1.0, mw::AnimationInterpolation::Linear},
    };

    mw::AnimationTrack equationPowerTrack;
    equationPowerTrack.id = "track-equation-power";
    equationPowerTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::EquationPower));
    equationPowerTrack.keyframes = {
        {"key-equation-power-start", 0.0, std::int64_t{2},
         mw::AnimationInterpolation::Step},
        {"key-equation-power-end", 8.0, std::int64_t{5},
         mw::AnimationInterpolation::Step},
    };

    mw::AnimationTrack unknownDisabledTrack;
    unknownDisabledTrack.id = "track-future-disabled";
    unknownDisabledTrack.target = "future.unsupported-target";
    unknownDisabledTrack.enabled = false;

    timeline.tracks = {
        scaleTrack,
        unknownDisabledTrack,
        paletteTrack,
        centreTrack,
        equationPowerTrack,
        rotationTrack,
        equationRealTrack,
    };

    const auto validation = mw::ValidateGeneralAnimationTimeline(timeline);
    Check(validation.valid && validation.warnings.size() == 1U,
          "A valid timeline should retain one disabled unknown target warning.");

    mw::AnimationEvaluationResult first;
    mw::AnimationEvaluationResult second;
    std::string error;
    Check(mw::EvaluateGeneralAnimation(base, timeline, 5.0, 42U, first, error),
          "The deterministic PH-06 evaluator should accept a valid timeline: " + error);
    Check(mw::EvaluateGeneralAnimation(base, timeline, 5.0, 42U, second, error),
          "Repeated PH-06 evaluation should succeed: " + error);
    Check(first.framePreset == second.framePreset &&
              first.resolvedTimeSeconds == second.resolvedTimeSeconds &&
              first.seed == second.seed &&
              first.appliedTrackIds == second.appliedTrackIds,
          "Same snapshot, timeline, time and seed should yield identical frame-local state.");
    Check(base == originalBase,
          "General animation evaluation must not mutate the immutable base snapshot.");
    mw::Preset deepExactAnimationBase = base;
    Check(mw::EnsureExactCamera(deepExactAnimationBase, error) &&
              mw::ExactDecimal::Parse("1e-1000", deepExactAnimationBase.exactCamera->halfHeight,
                                      error),
          "Deep animation fixture should retain authoritative exact camera text: " + error);
    mw::AnimationEvaluationResult deepExactAnimationResult;
    Check(!mw::EvaluateGeneralAnimation(deepExactAnimationBase, timeline, 5.0, 42U,
                                        deepExactAnimationResult, error) &&
              error.find("exact keyframes") != std::string::npos,
          "Double-valued camera tracks must refuse a lossy exact camera rather than rebuild it.");
    Check(first.appliedTrackIds.size() == 6U &&
              std::is_sorted(first.appliedTrackIds.begin(), first.appliedTrackIds.end()),
          "Enabled tracks should be evaluated in stable track-ID order.");
    Check(std::abs(first.framePreset.camera.scale - 1.0e-4) < 1.0e-15,
          "Camera scale should use logarithmic interpolation.");
    Check(std::abs(std::abs(first.framePreset.rotationDegrees) - 180.0) < 1.0e-12,
          "Rotation should interpolate over the shortest wrapped angle.");
    Check(std::abs(first.framePreset.colourOffset - 0.5) < 1.0e-12,
          "Smoothstep palette interpolation should evaluate deterministically at the midpoint.");
    Check(std::abs(first.framePreset.equation.quadratic.real) < 1.0e-12 &&
              first.framePreset.equation.power == 2,
          "Continuous and step equation targets should use their declared interpolation modes.");
    Check(first.framePreset.camera.centreXLow != 0.0 &&
              first.framePreset.camera.centreXLow > 1.0e-20 &&
              first.framePreset.camera.centreXLow < 9.0e-20,
          "Deep camera interpolation should retain a compensated low component.");
    mw::ExactCamera expectedAnimatedExact;
    std::string animatedExactError;
    Check(mw::BuildExactCameraFromLegacy(first.framePreset.camera, expectedAnimatedExact,
                                         animatedExactError) &&
              first.framePreset.exactCamera.has_value() &&
              *first.framePreset.exactCamera == expectedAnimatedExact,
          "Camera animation tracks must rebuild exact camera authority for the frame snapshot.");
    Check(mw::HasParameterInvalidation(first.invalidationMask,
                                       mw::ParameterInvalidation::CameraViewport) &&
              mw::HasParameterInvalidation(first.invalidationMask,
                                           mw::ParameterInvalidation::PaletteColouring) &&
              mw::HasParameterInvalidation(first.invalidationMask,
                                           mw::ParameterInvalidation::EquationPrecision),
          "Timeline evaluation should aggregate the affected render invalidation classes.");

    mw::AnimationTimeline loopTimeline = timeline;
    loopTimeline.loopMode = mw::AnimationLoopMode::Loop;
    mw::AnimationEvaluationResult loopResult;
    Check(mw::EvaluateGeneralAnimation(base, loopTimeline, 12.0, 42U, loopResult, error) &&
              std::abs(loopResult.resolvedTimeSeconds - 2.0) < 1.0e-12,
          "Loop mode should wrap evaluation time deterministically.");
    mw::AnimationTimeline pingPongTimeline = timeline;
    pingPongTimeline.loopMode = mw::AnimationLoopMode::PingPong;
    mw::AnimationEvaluationResult pingPongResult;
    Check(mw::EvaluateGeneralAnimation(base, pingPongTimeline, 12.0, 42U,
                                       pingPongResult, error) &&
              std::abs(pingPongResult.resolvedTimeSeconds - 8.0) < 1.0e-12 &&
              pingPongResult.framePreset.equation.power == 5,
          "Ping-pong mode should reflect time and reach the exact step keyframe.");
    mw::AnimationEvaluationResult clampedResult;
    Check(mw::EvaluateGeneralAnimation(base, timeline, 12.0, 42U, clampedResult, error) &&
              std::abs(clampedResult.resolvedTimeSeconds - 10.0) < 1.0e-12,
          "Clamp mode should stop at the timeline duration.");

    mw::AnimationTimeline duplicateTarget = timeline;
    duplicateTarget.tracks.push_back(scaleTrack);
    duplicateTarget.tracks.back().id = "track-scale-duplicate";
    duplicateTarget.tracks.back().keyframes[0].id = "key-scale-duplicate-start";
    duplicateTarget.tracks.back().keyframes[1].id = "key-scale-duplicate-end";
    Check(!mw::ValidateGeneralAnimationTimeline(duplicateTarget).valid,
          "Duplicate enabled Replace targets should be rejected.");

    mw::AnimationTimeline duplicateTime = timeline;
    duplicateTime.tracks.front().keyframes[1].timeSeconds = 0.0;
    Check(!mw::ValidateGeneralAnimationTimeline(duplicateTime).valid,
          "Duplicate enabled-track keyframe times should be rejected.");

    mw::AnimationTimeline invalidIntegerInterpolation = timeline;
    for (auto& track : invalidIntegerInterpolation.tracks) {
        if (track.target == mw::StableAnimationTargetName(mw::AnimationTargetKey::EquationPower)) {
            track.keyframes.front().interpolation = mw::AnimationInterpolation::Linear;
        }
    }
    Check(!mw::ValidateGeneralAnimationTimeline(invalidIntegerInterpolation).valid,
          "Integer equation targets should reject continuous interpolation.");

    mw::ProjectHistory history;
    mw::AnimationEvaluationResult historyResult;
    Check(mw::EvaluateGeneralAnimation(base, timeline, 1.0, 9U, historyResult, error) &&
              history.EntryCount() == 0U,
          "Per-frame animation evaluation must not enter project undo history.");

    mw::AnimationClockBank clocks;
    Check(clocks.SetTime(mw::AnimationClockDomain::Preview, 1.0, error) &&
              clocks.SetTime(mw::AnimationClockDomain::Wallpaper, 2.0, error) &&
              clocks.SetTime(mw::AnimationClockDomain::Export, 3.0, error) &&
              clocks.Advance(mw::AnimationClockDomain::Preview, 0.5, error),
          "Independent animation clocks should accept valid time updates.");
    Check(clocks.Time(mw::AnimationClockDomain::Preview) == 1.5 &&
              clocks.Time(mw::AnimationClockDomain::Wallpaper) == 2.0 &&
              clocks.Time(mw::AnimationClockDomain::Export) == 3.0,
          "Preview, wallpaper and export clocks must remain independent.");
    const double previewBeforeInvalidAdvance =
        clocks.Time(mw::AnimationClockDomain::Preview);
    Check(!clocks.Advance(mw::AnimationClockDomain::Preview, -5.0, error) &&
              clocks.Time(mw::AnimationClockDomain::Preview) == previewBeforeInvalidAdvance,
          "An invalid clock advance should fail without mutating the selected clock.");
}

void TestGeneralAnimationEditorAndJourneyAdapter() {
    mw::Preset base = mw::BuiltInPresets().front();
    base.camera = {-0.5, 0.0, 1.5, 0.0, 0.0};
    base.automaticJourneyWaypoints =
        "-0.743643887037151,0.13182590420533,0.004,12,2\n"
        "0.285,0.01,0.028,8,0\n";

    mw::AnimationTimeline timeline;
    std::string error;
    Check(mw::ConvertJourneyToGeneralAnimation(base, timeline, error),
          "A strict structured Journey should convert to camera tracks: " + error);
    Check(timeline.loopMode == mw::AnimationLoopMode::Clamp &&
              timeline.durationSeconds == 22.0 && timeline.tracks.size() == 3U,
          "Journey conversion should create a bounded one-pass camera timeline.");
    Check(mw::ValidateGeneralAnimationTimeline(timeline).valid,
          "Journey-generated tracks should satisfy the PH-06 timeline contract.");

    mw::AnimationEvaluationResult arrival;
    Check(mw::EvaluateGeneralAnimation(base, timeline, 12.0, 0U, arrival, error) &&
              std::abs(mw::CameraCentreX(arrival.framePreset.camera) + 0.743643887037151) < 1.0e-15 &&
              std::abs(arrival.framePreset.camera.scale - 0.004) < 1.0e-15,
          "Journey transition timing should evaluate to the first exact destination.");
    mw::AnimationEvaluationResult hold;
    Check(mw::EvaluateGeneralAnimation(base, timeline, 13.0, 0U, hold, error) &&
              hold.framePreset.camera == arrival.framePreset.camera,
          "Journey holds should remain exact Step segments in the converted timeline.");

    std::string roundTripScript;
    Check(mw::ConvertGeneralAnimationToJourney(base, timeline, roundTripScript, error),
          "The supported camera-only timeline should convert back to Journey text: " + error);
    mw::Preset roundTripPreset = base;
    roundTripPreset.automaticJourneyWaypoints = roundTripScript;
    mw::AnimationTimeline roundTripTimeline;
    Check(mw::ConvertJourneyToGeneralAnimation(roundTripPreset, roundTripTimeline, error) &&
              roundTripTimeline == timeline,
          "Supported Journey and camera tracks should round-trip without timing or value loss.");

    mw::Preset malformed = base;
    malformed.automaticJourneyWaypoints += "not-a-coordinate,row\n";
    mw::AnimationTimeline unchanged = timeline;
    Check(!mw::ConvertJourneyToGeneralAnimation(malformed, unchanged, error) &&
              unchanged == timeline,
          "Journey conversion must reject malformed rows instead of silently dropping them.");

    mw::AnimationTimeline unsupported = timeline;
    mw::AnimationTrack paletteTrack;
    paletteTrack.id = "palette-extra";
    paletteTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::PaletteOffset));
    paletteTrack.keyframes = {
        {"palette-extra-0", 0.0, base.colourOffset, mw::AnimationInterpolation::Linear},
        {"palette-extra-1", unsupported.durationSeconds, 1.0, mw::AnimationInterpolation::Linear},
    };
    unsupported.tracks.push_back(paletteTrack);
    std::string unsupportedScript = "must-clear";
    Check(!mw::ConvertGeneralAnimationToJourney(base, unsupported, unsupportedScript, error) &&
              unsupportedScript.empty(),
          "Tracks-to-Journey must explicitly refuse unsupported enabled targets without dropping them.");

    mw::AnimationTimeline lossy = timeline;
    auto xTrack = std::find_if(lossy.tracks.begin(), lossy.tracks.end(), [](const mw::AnimationTrack& track) {
        return track.target == mw::StableAnimationTargetName(mw::AnimationTargetKey::CameraCentreX);
    });
    auto compensated = std::get<mw::CompensatedAnimationValue>(xTrack->keyframes[1].value);
    compensated.low = 1.0e-30;
    xTrack->keyframes[1].value = compensated;
    Check(!mw::ConvertGeneralAnimationToJourney(base, lossy, unsupportedScript, error),
          "Tracks-to-Journey must refuse compensated values that Journey text cannot preserve.");

    mw::AnimationTimeline authored;
    authored.id = "editor-authoring";
    authored.durationSeconds = 5.0;
    authored.loopMode = mw::AnimationLoopMode::Clamp;
    authored.tracks.push_back({"editor-brightness",
                               std::string(mw::StableAnimationTargetName(
                                   mw::AnimationTargetKey::Brightness)),
                               true, {}});
    base.brightness = 1.25;
    Check(mw::AddGeneralAnimationKeyframeFromPreset(
              authored, "editor-brightness", "editor-brightness-key-0", 2.0,
              base, mw::AnimationInterpolation::Smoothstep, error) &&
              std::get<double>(authored.tracks.front().keyframes.front().value) == 1.25,
          "Add Current Value should read the authoritative project snapshot through the target registry.");
    const mw::AnimationTimeline beforeDuplicate = authored;
    Check(!mw::AddGeneralAnimationKeyframeFromPreset(
              authored, "editor-brightness", "editor-brightness-key-1", 2.0,
              base, mw::AnimationInterpolation::Smoothstep, error) &&
              authored == beforeDuplicate,
          "Editor authoring should reject duplicate track times without partially changing the timeline.");
}


void TestFrameSequenceExport() {
    mw::FrameRate ntsc{30000U, 1001U};
    std::string error;
    Check(mw::IsValidFrameRate(ntsc),
          "A bounded rational NTSC-style frame rate should be accepted.");
    const std::uint32_t exactCount = mw::FrameCountForDuration(10.0, {30U, 1U}, true, error);
    Check(exactCount == 301U,
          "Inclusive 10-second export at 30 fps should contain frames 0 through 300.");
    Check(mw::FrameCountForDuration(10.0, {30U, 1U}, false, error) == 300U,
          "Exclusive-end 10-second export at 30 fps should contain 300 frames.");
    Check(mw::FrameCountForDuration(10.05, {30U, 1U}, true, error) == 302U &&
              mw::FrameCountForDuration(10.05, {30U, 1U}, false, error) == 302U,
          "A non-frame-aligned duration should keep the rational grid and not invent a partial interval.");

    mw::Preset base = mw::BuiltInPresets().front();
    base.maximumIterations = 64;
    base.antiAliasingLevel = 1;
    mw::AnimationTimeline timeline;
    timeline.id = "timeline-export-test";
    timeline.durationSeconds = 2.0;
    timeline.loopMode = mw::AnimationLoopMode::Clamp;
    mw::AnimationTrack offset;
    offset.id = "track-offset";
    offset.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::PaletteOffset));
    offset.keyframes = {
        {"key-offset-0", 0.0, 0.0, mw::AnimationInterpolation::Linear},
        {"key-offset-1", 2.0, 0.75, mw::AnimationInterpolation::Linear},
    };
    timeline.tracks.push_back(offset);

    mw::FrameSequenceExportSettings settings;
    settings.width = 16U;
    settings.height = 12U;
    settings.dpi = 96U;
    settings.tileWidth = 5U;
    settings.frameRate = {1U, 1U};
    settings.startTimeSeconds = 0.0;
    settings.frameCount = 3U;
    settings.seed = 77U;
    settings.applicationVersion = "test";
    settings.filePrefix = "fixture";
    settings.frameNumberDigits = 4U;
    settings.scaleQualityToResolution = false;
    Check(std::abs(mw::FrameTimeForIndex(settings, 2U) - 2.0) < 1.0e-15,
          "Frame time should be derived directly from index and rational rate.");

    const auto unique = std::to_string(
        static_cast<unsigned long long>(std::filesystem::file_time_type::clock::now()
            .time_since_epoch().count()));
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / ("mw-ph08-export-" + unique);
    std::error_code filesystemError;
    std::filesystem::remove_all(output, filesystemError);

    mw::FrameSequenceExportSettings unimplementedRendererSettings = settings;
    unimplementedRendererSettings.rendererId = "gpu-d3d11-validated-v1";
    mw::FrameSequenceExportJob unimplementedRendererJob;
    Check(!mw::BuildFrameSequenceExportJob(base, timeline, unimplementedRendererSettings, output,
                                           unimplementedRendererJob, error) &&
              error.find("not an implemented deterministic route") != std::string::npos,
          "An unimplemented GPU renderer identifier must be rejected instead of falling back to CPU.");

    mw::FrameSequenceExportJob job;
    Check(mw::BuildFrameSequenceExportJob(base, timeline, settings, output, job, error),
          "A valid immutable frame-sequence job should build: " + error);
    Check(job.projectFingerprintVersion == "mw-render-state-v3-exact-precision-plan",
          "New frame-sequence jobs should bind to the exact-camera plus precision-plan contract.");
    const std::string originalFingerprint = job.jobFingerprint;
    mw::FrameSequenceExportJob repeated;
    Check(mw::BuildFrameSequenceExportJob(base, timeline, settings, output, repeated, error) &&
              repeated.jobFingerprint == originalFingerprint,
          "Identical export snapshots should produce the same job fingerprint.");
    mw::Preset exactChanged = base;
    Check(mw::ExactDecimal::Parse("1e-10", exactChanged.exactCamera->halfHeight, error),
          "Exact export identity fixture should parse an exact half-height: " + error);
    mw::FrameSequenceExportJob changedExactJob;
    Check(mw::BuildFrameSequenceExportJob(exactChanged, timeline, settings, output,
                                          changedExactJob, error) &&
              changedExactJob.projectFingerprint != job.projectFingerprint &&
              changedExactJob.jobFingerprint != job.jobFingerprint,
          "A textual exact-camera change must produce a distinct, non-resumable export job.");
    mw::Preset unsupportedDeep = base;
    Check(mw::ExactDecimal::Parse("1e-200", unsupportedDeep.exactCamera->halfHeight, error),
          "Deep frame-export rejection fixture should parse authoritative exact camera text: " + error);
    mw::FrameSequenceExportJob rejectedDeepJob;
    Check(!mw::BuildFrameSequenceExportJob(unsupportedDeep, timeline, settings, output,
                                            rejectedDeepJob, error) &&
              error.find("cannot execute the base exact camera safely") != std::string::npos,
          "The Float64 frame exporter must reject deep exact cameras before creating a job.");
    mw::FrameSequenceExportSettings exactFrameSettings = settings;
    exactFrameSettings.rendererId = "cpu-exact-boost512-v1";
    mw::FrameSequenceExportJob exactDeepJob;
    Check(mw::BuildFrameSequenceExportJob(exactChanged, timeline, exactFrameSettings, output,
                                          exactDeepJob, error) &&
              exactDeepJob.projectFingerprint != changedExactJob.projectFingerprint,
          "The explicit Boost-512 frame renderer must create a distinct immutable exact job identity.");
    mw::Preset rotatedExactFrame = exactChanged;
    rotatedExactFrame.rotationDegrees = 1.0;
    mw::FrameSequenceExportJob rotatedExactFrameJob;
    Check(!mw::BuildFrameSequenceExportJob(rotatedExactFrame, timeline, exactFrameSettings,
                                           output, rotatedExactFrameJob, error) &&
              error.find("does not support rotation") != std::string::npos,
          "Exact frame export must reject unsupported rotation before creating an immutable job.");
    mw::Preset animatedCoefficientExactFrame = exactChanged;
    animatedCoefficientExactFrame.equation.animateCoefficients = true;
    mw::FrameSequenceExportJob animatedCoefficientExactFrameJob;
    Check(!mw::BuildFrameSequenceExportJob(animatedCoefficientExactFrame, timeline,
                                           exactFrameSettings, output,
                                           animatedCoefficientExactFrameJob, error) &&
              error.find("animated equation coefficients") != std::string::npos,
          "Exact frame export must reject coefficient animation before creating an immutable job.");
    mw::AnimationTimeline rotatingExactTimeline = timeline;
    mw::AnimationTrack rotationTrack;
    rotationTrack.id = "track-exact-export-rotation";
    rotationTrack.target = std::string(mw::StableAnimationTargetName(
        mw::AnimationTargetKey::RotationDegrees));
    rotationTrack.keyframes = {
        {"key-exact-export-rotation-0", 0.0, 0.0, mw::AnimationInterpolation::Linear},
        {"key-exact-export-rotation-1", 1.0, 1.0, mw::AnimationInterpolation::Linear},
    };
    rotatingExactTimeline.tracks.push_back(rotationTrack);
    mw::FrameSequenceExportSettings animatedExactFrameSettings = exactFrameSettings;
    animatedExactFrameSettings.startTimeSeconds = 1.0;
    animatedExactFrameSettings.frameCount = 1U;
    animatedExactFrameSettings.filePrefix = "rotated";
    const std::filesystem::path rotatingOutput = output.parent_path() /
        (output.filename().string() + "-rotating");
    std::filesystem::remove_all(rotatingOutput, filesystemError);
    mw::FrameSequenceExportJob animatedExactFrameJob;
    Check(mw::BuildFrameSequenceExportJob(exactChanged, rotatingExactTimeline,
                                          animatedExactFrameSettings, rotatingOutput,
                                          animatedExactFrameJob, error),
          "A base-valid exact frame job with a later rotating timeline frame should build: " + error);
    std::uint32_t unexpectedExactRenderCalls = 0U;
    mw::FrameSequenceExportResult animatedExactFrameResult;
    Check(!mw::RunFrameSequenceExport(
              animatedExactFrameJob, false,
              [&unexpectedExactRenderCalls](const mw::FrameSequenceFrameRequest&,
                                            const std::filesystem::path&, std::string&) {
                  ++unexpectedExactRenderCalls;
                  return false;
              },
              [](const std::filesystem::path&, std::uint32_t, std::uint32_t, std::string&) {
                  return false;
              }, {}, [] { return false; }, animatedExactFrameResult, error) &&
              unexpectedExactRenderCalls == 0U &&
              error.find("frame 0 safely") != std::string::npos &&
              error.find("does not support rotation") != std::string::npos,
          "An animated unsupported exact frame must be rejected before its renderer callback.");
    std::filesystem::remove_all(rotatingOutput, filesystemError);
    mw::FrameSequenceExportSettings exact2048FrameSettings = settings;
    exact2048FrameSettings.rendererId = "cpu-exact-boost2048-v1";
    mw::FrameSequenceExportJob exact2048DeepJob;
    Check(mw::BuildFrameSequenceExportJob(unsupportedDeep, timeline, exact2048FrameSettings,
                                          output, exact2048DeepJob, error) &&
              exact2048DeepJob.projectFingerprint != job.projectFingerprint,
          "The explicit Boost-2048 frame renderer must create a distinct immutable deep job identity.");
    mw::Preset nearInfiniteDeep = base;
    Check(mw::ExactDecimal::Parse("1e-1000", nearInfiniteDeep.exactCamera->halfHeight, error),
          "Near-unlimited frame-export fixture should parse authoritative exact camera text: " + error);
    mw::FrameSequenceExportSettings exact8192FrameSettings = settings;
    exact8192FrameSettings.rendererId = "cpu-exact-boost8192-v1";
    mw::FrameSequenceExportJob exact8192DeepJob;
    Check(mw::BuildFrameSequenceExportJob(nearInfiniteDeep, timeline, exact8192FrameSettings,
                                          output, exact8192DeepJob, error) &&
              exact8192DeepJob.projectFingerprint != exact2048DeepJob.projectFingerprint,
          "The explicit Boost-8192 frame renderer must create a distinct immutable near-unlimited job identity.");
    mw::FrameSequenceExportJob sameCamera8192Job;
    Check(mw::BuildFrameSequenceExportJob(unsupportedDeep, timeline, exact8192FrameSettings,
                                          output, sameCamera8192Job, error) &&
              sameCamera8192Job.projectFingerprint != exact2048DeepJob.projectFingerprint,
          "Planner backend/tier trace must make the same exact camera non-resumable across 2048 and 8192 routes.");

    const std::filesystem::path exactPlanOutput = output.parent_path() /
        (output.filename().string() + "-exact-plan");
    std::filesystem::remove_all(exactPlanOutput, filesystemError);
    mw::FrameSequenceExportSettings exactPlanSettings = exact8192FrameSettings;
    exactPlanSettings.frameCount = 1U;
    exactPlanSettings.filePrefix = "exact-plan";
    mw::FrameSequenceExportJob exactPlanJob;
    Check(mw::BuildFrameSequenceExportJob(nearInfiniteDeep, timeline, exactPlanSettings,
                                          exactPlanOutput, exactPlanJob, error),
          "A bounded exact-plan export job should be constructible: " + error);
    mw::PrecisionPlan callbackPrecisionPlan;
    mw::FrameSequenceExportResult exactPlanResult;
    Check(mw::RunFrameSequenceExport(
              exactPlanJob, false,
              [&callbackPrecisionPlan](const mw::FrameSequenceFrameRequest& request,
                                        const std::filesystem::path& temporaryPath,
                                        std::string& renderError) {
                  callbackPrecisionPlan = request.precisionPlan;
                  std::ofstream stream(temporaryPath, std::ios::binary | std::ios::trunc);
                  if (!stream) {
                      renderError = "exact-plan fixture could not create its temporary frame";
                      return false;
                  }
                  stream << "exact-plan";
                  return static_cast<bool>(stream);
              },
              [](const std::filesystem::path&, std::uint32_t, std::uint32_t, std::string&) {
                  return true;
              }, {}, [] { return false; }, exactPlanResult, error) &&
              exactPlanResult.renderedFrames == 1U &&
              callbackPrecisionPlan.backend == mw::PrecisionExecutionBackend::CpuBoost8192Direct &&
              callbackPrecisionPlan.selectedBits == 8192 &&
              callbackPrecisionPlan.version == mw::kPrecisionPlanVersion,
          "A direct exact frame callback must receive the immutable planner-selected 8192-bit execution plan: " + error);
    std::filesystem::remove_all(exactPlanOutput, filesystemError);

    mw::Preset ultraDeep = base;
    Check(mw::ExactDecimal::Parse("1e-3000", ultraDeep.exactCamera->halfHeight, error),
          "The 16384-bit frame-export fixture should parse a deeper exact camera scale: " + error);
    const std::filesystem::path exact16384PlanOutput = output.parent_path() /
        (output.filename().string() + "-exact-16384-plan");
    std::filesystem::remove_all(exact16384PlanOutput, filesystemError);
    mw::FrameSequenceExportSettings exact16384PlanSettings = settings;
    exact16384PlanSettings.rendererId = "cpu-exact-boost16384-v1";
    exact16384PlanSettings.frameCount = 1U;
    exact16384PlanSettings.filePrefix = "exact-16384-plan";
    mw::FrameSequenceExportJob exact16384PlanJob;
    Check(mw::BuildFrameSequenceExportJob(ultraDeep, timeline, exact16384PlanSettings,
                                          exact16384PlanOutput, exact16384PlanJob, error),
          "A bounded 16384-bit exact-plan export job should be constructible: " + error);
    callbackPrecisionPlan = {};
    exactPlanResult = {};
    Check(mw::RunFrameSequenceExport(
              exact16384PlanJob, false,
              [&callbackPrecisionPlan](const mw::FrameSequenceFrameRequest& request,
                                        const std::filesystem::path& temporaryPath,
                                        std::string& renderError) {
                  callbackPrecisionPlan = request.precisionPlan;
                  std::ofstream stream(temporaryPath, std::ios::binary | std::ios::trunc);
                  if (!stream) {
                      renderError = "exact-16384-plan fixture could not create its temporary frame";
                      return false;
                  }
                  stream << "exact-16384-plan";
                  return static_cast<bool>(stream);
              },
              [](const std::filesystem::path&, std::uint32_t, std::uint32_t, std::string&) {
                  return true;
              }, {}, [] { return false; }, exactPlanResult, error) &&
              exactPlanResult.renderedFrames == 1U &&
              callbackPrecisionPlan.backend == mw::PrecisionExecutionBackend::CpuBoost16384Direct &&
              callbackPrecisionPlan.selectedBits == 16384 &&
              callbackPrecisionPlan.version == mw::kPrecisionPlanVersion,
          "A direct exact frame callback must receive the immutable planner-selected 16384-bit execution plan: " + error);
    std::filesystem::remove_all(exact16384PlanOutput, filesystemError);

    std::uint32_t renderedCallbackCount = 0U;
    std::uint32_t completedProgress = 0U;
    auto render = [&](const mw::FrameSequenceFrameRequest& request,
                      const std::filesystem::path& path,
                      std::string& renderError) {
        ++renderedCallbackCount;
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream) {
            renderError = "fake frame could not open";
            return false;
        }
        stream << "frame=" << request.frameIndex << "\ntime="
               << std::setprecision(17) << request.timeSeconds << "\nseed="
               << request.seed << "\noffset=" << request.framePreset.colourOffset << "\n";
        stream.flush();
        if (!stream) {
            renderError = "fake frame could not write";
            return false;
        }
        return true;
    };
    auto validate = [](const std::filesystem::path& path, std::uint32_t width,
                       std::uint32_t height, std::string& validationError) {
        std::ifstream stream(path, std::ios::binary);
        std::string firstLine;
        std::getline(stream, firstLine);
        if (!stream || firstLine.rfind("frame=", 0U) != 0U || width == 0U || height == 0U) {
            validationError = "fake frame validation failed";
            return false;
        }
        return true;
    };
    mw::FrameSequenceExportResult cancelled;
    const bool cancelledRun = mw::RunFrameSequenceExport(
        job, false, render, validate,
        [&](const mw::FrameSequenceExportProgress& progress) {
            completedProgress = progress.completedFrames;
        },
        [&] { return completedProgress >= 2U; }, cancelled, error);
    Check(cancelledRun && cancelled.cancelled && cancelled.renderedFrames == 2U &&
              renderedCallbackCount == 2U,
          "Cancellation should stop new frames while preserving two verified outputs: " + error);
    Check(std::filesystem::exists(mw::FrameSequenceFinalFramePath(job, 0U)) &&
              std::filesystem::exists(mw::FrameSequenceFinalFramePath(job, 1U)) &&
              !std::filesystem::exists(mw::FrameSequenceFinalFramePath(job, 2U)),
          "Cancelled export should preserve only promoted complete frames.");

    completedProgress = 0U;
    mw::FrameSequenceExportResult resumed;
    Check(mw::RunFrameSequenceExport(job, true, render, validate,
              [&](const mw::FrameSequenceExportProgress& progress) {
                  completedProgress = progress.completedFrames;
              }, [] { return false; }, resumed, error),
          "A matching manifest should resume the remaining frame: " + error);
    Check(!resumed.cancelled && resumed.resumedFrames == 2U && resumed.renderedFrames == 1U &&
              renderedCallbackCount == 3U && completedProgress == 3U,
          "Resume should verify two frames and render only the missing frame.");

    mw::FrameSequenceManifest manifest;
    Check(mw::LoadFrameSequenceManifest(mw::FrameSequenceManifestPath(job), manifest, error) &&
              manifest.completedFrames.size() == 3U &&
              manifest.jobFingerprint == originalFingerprint &&
              manifest.projectFingerprintVersion == "mw-render-state-v3-exact-precision-plan",
          "Completed export should persist a complete matching manifest: " + error);

    mw::FrameSequenceExportSettings mismatchSettings = settings;
    mismatchSettings.width = 17U;
    mw::FrameSequenceExportJob mismatch;
    Check(mw::BuildFrameSequenceExportJob(base, timeline, mismatchSettings, output,
                                          mismatch, error),
          "A distinct mismatch job should still be constructible.");
    mw::FrameSequenceExportResult mismatchResult;
    Check(!mw::RunFrameSequenceExport(mismatch, true, render, validate, {},
                                      [] { return false; }, mismatchResult, error) &&
              error.find("does not match") != std::string::npos,
          "Resume must refuse a project/output fingerprint mismatch.");

    mw::FrameSequenceManifest tamperedManifest = manifest;
    tamperedManifest.width += 1U;
    Check(mw::SaveFrameSequenceManifest(mw::FrameSequenceManifestPath(job),
                                        tamperedManifest, error),
          "A syntactically valid tampered manifest fixture should save: " + error);
    mw::FrameSequenceExportResult tamperedResult;
    Check(!mw::RunFrameSequenceExport(job, true, render, validate, {},
                                      [] { return false; }, tamperedResult, error) &&
              error.find("does not match") != std::string::npos,
          "Resume must compare typed manifest settings as well as its job fingerprint.");
    mw::FrameSequenceManifest unsupportedRendererManifest = manifest;
    unsupportedRendererManifest.rendererId = "gpu-d3d11-validated-v1";
    Check(!mw::SaveFrameSequenceManifest(mw::FrameSequenceManifestPath(job),
                                         unsupportedRendererManifest, error) &&
              error.find("internally inconsistent") != std::string::npos,
          "A manifest cannot be rewritten with an unimplemented renderer identifier.");
    Check(mw::SaveFrameSequenceManifest(mw::FrameSequenceManifestPath(job), manifest, error),
          "The valid manifest should be restorable after the tamper fixture: " + error);

    const std::filesystem::path malformedPath = output / "malformed-manifest.json";
    {
        std::ofstream malformed(malformedPath, std::ios::binary | std::ios::trunc);
        malformed << R"({"schema":1,"width":16,"height":12,"dpi":96,"frameRateNumerator":1,"frameRateDenominator":1,"frameCount":1,"firstFrameNumber":0,"frameNumberDigits":4,"evaluatorVersion":"general-animation-v1","applicationVersion":"test","jobFingerprint":"x","projectFingerprint":"x","timelineFingerprint":"x","rendererId":"cpu-production-still","filePrefix":"fixture","scaleQualityToResolution":false,"seed":"0","startTimeSeconds":"0x0p+0","completedFrames":[{"frameIndex":0,"timeSeconds":"0x0p+0","fileName":"fixture-0000.png","contentDigest":"fnv1a64:0"}]})";
    }
    mw::FrameSequenceManifest malformedManifest;
    Check(!mw::LoadFrameSequenceManifest(malformedPath, malformedManifest, error),
          "A manifest entry missing fileSizeBytes should be rejected safely.");

    // Selected-frame visual evidence: render start, middle and end twice through
    // the production CPU still path and require deterministic equality plus change.
    std::vector<std::vector<std::uint32_t>> selectedFrames;
    for (std::uint32_t frameIndex = 0U; frameIndex < 3U; ++frameIndex) {
        mw::AnimationEvaluationResult evaluated;
        const double time = mw::FrameTimeForIndex(settings, frameIndex);
        Check(mw::EvaluateGeneralAnimation(base, timeline, time, 77U, evaluated, error),
              "Selected export frame should evaluate: " + error);
        auto renderPixels = [&](std::vector<std::uint32_t>& pixels) {
            pixels.assign(static_cast<std::size_t>(settings.width) * settings.height, 0U);
            mw::StillRenderRequest request;
            request.preset = evaluated.framePreset;
            request.width = settings.width;
            request.height = settings.height;
            request.tileWidth = settings.tileWidth;
            request.previewMaximumWidth = 0U;
            request.previewMaximumHeight = 0U;
            request.timeSeconds = time;
            request.scaleQualityToResolution = false;
            mw::StillRenderResult result;
            return mw::RenderStillImageTiled(
                request,
                [&](std::uint32_t row, std::span<const std::uint32_t> values,
                    std::string&) {
                    std::copy(values.begin(), values.end(),
                              pixels.begin() + static_cast<std::ptrdiff_t>(row * settings.width));
                    return true;
                }, {}, [] { return false; }, result, error);
        };
        std::vector<std::uint32_t> first;
        std::vector<std::uint32_t> second;
        Check(renderPixels(first) && renderPixels(second) && first == second,
              "Selected production export frame should repeat exactly: " + error);
        selectedFrames.push_back(std::move(first));
    }
    Check(selectedFrames[0] != selectedFrames[1] && selectedFrames[1] != selectedFrames[2],
          "Selected timeline frames should show the intended animated palette change.");

    std::filesystem::remove_all(output, filesystemError);
}


void TestExternalVideoExport() {
    mw::ExternalEncoderCapabilities capabilities;
    std::string error;
    Check(mw::ParseFfmpegCapabilities(
              "ffmpeg version 8.1 test\n", "Encoder libx264 [test]\n",
              "Muxer mp4 [test]\n", capabilities, error) &&
              capabilities.hasLibx264 && capabilities.hasMp4Muxer,
          "FFmpeg capability checks should require a version, libx264, and MP4 muxing: " + error);
    Check(!mw::ParseFfmpegCapabilities(
              "ffmpeg version 8.1 test\n", "Encoder png\n", "Muxer mp4\n",
              capabilities, error) && error.find("libx264") != std::string::npos,
          "An FFmpeg build without libx264 should be rejected explicitly.");

    const std::wstring unsafe = L"C:\\Frames & Tools\\quote\"name.png";
    const std::wstring quoted = mw::QuoteWindowsProcessArgument(unsafe);
    Check(!quoted.empty() && quoted.front() == L'\"' && quoted.back() == L'\"' &&
              quoted.find(L"&") != std::wstring::npos,
          "Windows process arguments should be quoted as one argument without shell interpretation.");
    const std::wstring command = mw::BuildWindowsCommandLine(
        {L"ffmpeg.exe", L"-i", unsafe, L"output.mp4"});
    Check(command.find(L"cmd.exe") == std::wstring::npos &&
              command.find(L" /c ") == std::wstring::npos,
          "The fixed FFmpeg command line must not introduce a command shell.");

    mw::Preset base = mw::BuiltInPresets().front();
    mw::AnimationTimeline timeline;
    timeline.id = "video-export-timeline";
    timeline.durationSeconds = 2.0;
    mw::FrameSequenceExportSettings sequenceSettings;
    sequenceSettings.width = 8U;
    sequenceSettings.height = 6U;
    sequenceSettings.frameRate = {1U, 1U};
    sequenceSettings.frameCount = 3U;
    sequenceSettings.applicationVersion = "test";
    sequenceSettings.filePrefix = "video";
    sequenceSettings.frameNumberDigits = 4U;

    const auto unique = std::to_string(
        static_cast<unsigned long long>(std::filesystem::file_time_type::clock::now()
            .time_since_epoch().count()));
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / ("mw-ph09-video-" + unique);
    std::error_code filesystemError;
    std::filesystem::remove_all(directory, filesystemError);

    mw::FrameSequenceExportJob sequenceJob;
    Check(mw::BuildFrameSequenceExportJob(base, timeline, sequenceSettings, directory,
                                          sequenceJob, error),
          "The PH09 fixture sequence job should build: " + error);
    auto render = [](const mw::FrameSequenceFrameRequest& request,
                     const std::filesystem::path& path, std::string& renderError) {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream << "png-fixture-" << request.frameIndex;
        if (!stream) {
            renderError = "fixture write failed";
            return false;
        }
        return true;
    };
    auto validate = [](const std::filesystem::path& path, std::uint32_t,
                       std::uint32_t, std::string& validationError) {
        std::ifstream stream(path, std::ios::binary);
        std::string text;
        std::getline(stream, text);
        if (text.rfind("png-fixture-", 0U) != 0U) {
            validationError = "fixture validation failed";
            return false;
        }
        return true;
    };
    mw::FrameSequenceExportResult sequenceResult;
    Check(mw::RunFrameSequenceExport(sequenceJob, false, render, validate, {},
                                      [] { return false; }, sequenceResult, error),
          "The complete verified PH09 source sequence should render: " + error);

    mw::FrameSequenceManifest manifest;
    Check(mw::LoadVerifiedFrameSequence(directory, manifest, error) &&
              manifest.completedFrames.size() == 3U,
          "PH09 should load only a complete digest-verified frame sequence: " + error);
    {
        std::ofstream ffmpeg(directory / "ffmpeg.exe", std::ios::binary | std::ios::trunc);
        ffmpeg << "external fixture";
    }

    mw::ExternalVideoExportSettings settings;
    settings.ffmpegExecutable = directory / "ffmpeg.exe";
    settings.frameSequenceDirectory = directory;
    settings.outputPath = directory / "video & safe.mp4";
    settings.encoderPreset = "medium";
    settings.encoderVersionLine = "ffmpeg version 8.1 test";
    settings.crf = 18U;
    mw::ExternalVideoExportJob job;
    Check(mw::BuildExternalVideoExportJob(manifest, settings, job, error),
          "A fixed-vector external video job should build: " + error);
    Check(std::find(job.encodeArguments.begin(), job.encodeArguments.end(), L"cmd.exe") ==
              job.encodeArguments.end() &&
              std::find(job.encodeArguments.begin(), job.encodeArguments.end(), L"/c") ==
              job.encodeArguments.end() &&
              job.encodeArguments.back() == job.temporaryOutputPath.wstring(),
          "The encoder job should keep paths as vector elements and never use shell tokens.");

    mw::FrameSequenceManifest oddManifest = manifest;
    oddManifest.width = 7U;
    mw::ExternalVideoExportJob oddJob;
    Check(!mw::BuildExternalVideoExportJob(oddManifest, settings, oddJob, error) &&
              error.find("even source width and height") != std::string::npos,
          "H.264 yuv420p video jobs should reject odd source dimensions before FFmpeg starts.");

    int processCalls = 0;
    auto successRunner = [&](const std::filesystem::path&,
                             const std::vector<std::wstring>& arguments,
                             const std::function<bool()>&,
                             const std::function<void(std::string_view)>& stdoutChunk,
                             mw::ExternalProcessResult& processResult,
                             std::string&) {
        ++processCalls;
        processResult = {};
        processResult.exitCode = 0;
        if (processCalls == 1) {
            std::ofstream output(std::filesystem::path(arguments.back()),
                                 std::ios::binary | std::ios::trunc);
            output << "verified-mp4-fixture";
            if (stdoutChunk) stdoutChunk("frame=1\nframe=3\nprogress=end\n");
            processResult.standardOutput = "frame=3\nprogress=end\n";
        }
        return true;
    };
    std::uint32_t reportedFrames = 0U;
    mw::ExternalVideoExportResult result;
    Check(mw::RunExternalVideoExport(
              job, successRunner, [] { return false; },
              [&](const mw::ExternalVideoExportProgress& progress) {
                  reportedFrames = progress.encodedFrames;
              }, result, error) &&
              processCalls == 2 && reportedFrames == 3U &&
              std::filesystem::exists(settings.outputPath),
          "Successful encoding should verify then atomically promote the MP4: " + error);
    Check(std::filesystem::exists(directory / manifest.completedFrames.front().fileName),
          "Source PNG frames should remain by default after successful encoding.");

    mw::ExternalVideoExportSettings failedSettings = settings;
    failedSettings.outputPath = directory / "failed.mp4";
    mw::ExternalVideoExportJob failedJob;
    Check(mw::BuildExternalVideoExportJob(manifest, failedSettings, failedJob, error),
          "A failure-path video job should build: " + error);
    auto failedRunner = [](const std::filesystem::path&,
                           const std::vector<std::wstring>& arguments,
                           const std::function<bool()>&,
                           const std::function<void(std::string_view)>&,
                           mw::ExternalProcessResult& processResult,
                           std::string&) {
        std::ofstream output(std::filesystem::path(arguments.back()),
                                 std::ios::binary | std::ios::trunc);
        output << "partial";
        processResult.exitCode = 1;
        processResult.standardError = "intentional failure";
        return true;
    };
    mw::ExternalVideoExportResult failedResult;
    Check(!mw::RunExternalVideoExport(failedJob, failedRunner, [] { return false; }, {},
                                      failedResult, error) &&
              !std::filesystem::exists(failedSettings.outputPath) &&
              std::filesystem::exists(directory / manifest.completedFrames.front().fileName),
          "Encoder failure must preserve verified source frames and avoid a final MP4.");

    mw::ExternalVideoExportSettings cancelledSettings = settings;
    cancelledSettings.outputPath = directory / "cancelled.mp4";
    mw::ExternalVideoExportJob cancelledJob;
    Check(mw::BuildExternalVideoExportJob(manifest, cancelledSettings, cancelledJob, error),
          "A cancellation-path video job should build: " + error);
    auto cancelledRunner = [](const std::filesystem::path&,
                              const std::vector<std::wstring>& arguments,
                              const std::function<bool()>&,
                              const std::function<void(std::string_view)>&,
                              mw::ExternalProcessResult& processResult,
                              std::string&) {
        std::ofstream output(std::filesystem::path(arguments.back()),
                                 std::ios::binary | std::ios::trunc);
        output << "partial";
        processResult.exitCode = 1;
        processResult.cancelled = true;
        return true;
    };
    mw::ExternalVideoExportResult cancelledResult;
    Check(mw::RunExternalVideoExport(cancelledJob, cancelledRunner, [] { return false; }, {},
                                     cancelledResult, error) &&
              cancelledResult.cancelled &&
              !std::filesystem::exists(cancelledSettings.outputPath) &&
              std::filesystem::exists(directory / manifest.completedFrames.front().fileName),
          "Encoder cancellation should remove only the owned temporary MP4 and preserve frames.");

    const std::filesystem::path unrelated = directory / "keep-me.txt";
    {
        std::ofstream stream(unrelated, std::ios::binary | std::ios::trunc);
        stream << "untracked";
    }
    mw::ExternalVideoExportSettings cleanupSettings = settings;
    cleanupSettings.outputPath = directory / "cleanup.mp4";
    cleanupSettings.cleanupFramesAfterSuccess = true;
    mw::ExternalVideoExportJob cleanupJob;
    Check(mw::BuildExternalVideoExportJob(manifest, cleanupSettings, cleanupJob, error),
          "A cleanup-enabled job should build: " + error);
    processCalls = 0;
    mw::ExternalVideoExportResult cleanupResult;
    Check(mw::RunExternalVideoExport(cleanupJob, successRunner, [] { return false; }, {},
                                     cleanupResult, error) &&
              cleanupResult.sourceFramesCleaned &&
              std::filesystem::exists(cleanupSettings.outputPath) &&
              std::filesystem::exists(unrelated) &&
              !std::filesystem::exists(directory / manifest.completedFrames.front().fileName),
          "Optional cleanup should occur only after verified success and remove only tracked sequence files: " + error);

    std::filesystem::remove_all(directory, filesystemError);
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

    preset.camera = {0.0, 0.0, 1.0};
    preset.animationMode = mw::AnimationMode::ManualView;
    controller.SetPreset(preset, false);
    controller.Pan(0.25, 0.0, 1.0, 90.0);
    Check(std::abs(mw::CameraCentreX(controller.Camera())) < 1.0e-12 &&
              std::abs(mw::CameraCentreY(controller.Camera()) + 0.5) < 1.0e-12,
          "Rotated preview panning should move along the rotated complex-plane axis.");
    controller.SetPreset(preset, false);
    controller.ZoomAt(0.5, 0.0, 1.0, 1.0, 90.0);
    Check(std::abs(mw::CameraCentreX(controller.Camera())) < 1.0e-12 &&
              mw::CameraCentreY(controller.Camera()) > 0.0,
          "Rotated wheel zoom should preserve its anchor along the rotated complex-plane axis.");

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

    const auto tricorn = mw::EquationExample(4);
    Check(mw::ResolvePerturbationProfile(tricorn) ==
              mw::PerturbationProfile::TricornQuadratic,
          "The exact power-2 Tricorn profile should enable conjugate perturbation.");
    Check(mw::EquationSupportsPerturbation(tricorn),
          "The exact power-2 Tricorn profile should be accepted by the deep-zoom selector.");
    const auto multicorn = mw::EquationExample(5);
    Check(!mw::EquationSupportsPerturbation(multicorn),
          "Higher Multicorn powers should remain on the safe direct precision path.");

    mw::CameraState tricornCamera{0.35, 0.42, 1.0e-8};
    const auto tricornDoubleOrbit =
        mw::BuildReferenceOrbitDouble(tricornCamera, tricorn, 96);
    const auto tricornArbitraryOrbit =
        mw::BuildReferenceOrbitArbitrary(tricornCamera, tricorn, 96, 256);
    const auto expansionValue = [](const mw::ReferenceOrbitPoint& point) {
        std::complex<double> value{};
        for (float component : point.real) value.real(value.real() + component);
        for (float component : point.imaginary) value.imag(value.imag() + component);
        return value;
    };
    const std::complex<double> c{mw::CameraCentreX(tricornCamera),
                                 mw::CameraCentreY(tricornCamera)};
    const std::complex<double> expectedSecond = std::conj(c) * std::conj(c) + c;
    Check(std::abs(expansionValue(tricornDoubleOrbit.points.at(2)) - expectedSecond) < 1.0e-12,
          "The double Tricorn reference orbit should apply conjugation before squaring.");
    Check(std::abs(expansionValue(tricornArbitraryOrbit.points.at(2)) - expectedSecond) < 1.0e-12,
          "The arbitrary-precision Tricorn reference orbit should apply conjugation before squaring.");

    for (double scale : {1.0e-4, 1.0e-8, 1.0e-12}) {
        tricornCamera.scale = scale;
        constexpr double offsetX = 0.37;
        constexpr double offsetY = -0.23;
        const double targetX = mw::CameraCentreX(tricornCamera) + scale * offsetX;
        const double targetY = mw::CameraCentreY(tricornCamera) + scale * offsetY;
        const auto direct = mw::CalculateEscape(targetX, targetY, 512, tricorn);
        const auto perturbed = mw::EvaluatePerturbationSample(
            tricornCamera, tricorn, 512, offsetX, offsetY, 256, true);
        Check(perturbed.stable && !perturbed.referenceRefreshed &&
                  perturbed.validity == mw::PerturbationSampleValidity::Stable,
              "Deep Tricorn perturbation should remain stable for an in-frame sample.");
        Check(perturbed.iterations == direct.iterations &&
                  perturbed.escaped == direct.escaped,
              "Deep Tricorn perturbation should match direct iteration results at progressively deeper scales.");
    }

    tricornCamera.scale = 1.0e-3;
    const auto guarded = mw::EvaluatePerturbationSample(
        tricornCamera, tricorn, 128, 1000000.0, -1000000.0, 256, true);
    Check(guarded.referenceRefreshed && guarded.stable &&
              guarded.validity == mw::PerturbationSampleValidity::Rebased,
          "An excessive perturbation delta should refresh the reference orbit instead of continuing unstably.");
    const auto unresolved = mw::EvaluatePerturbationSample(
        tricornCamera, tricorn, 128, 1000000.0, -1000000.0, 256, false);
    Check(!unresolved.stable && !unresolved.referenceRefreshed &&
              unresolved.validity == mw::PerturbationSampleValidity::Unresolved,
          "An unstable perturbation sample without an allowed rebase must be explicitly unresolved.");
}


void TestStaticSlideshowValidation() {
    mw::AppSettings settings;
    settings.staticWallpaper.enabled = true;
    settings.staticWallpaper.cycleSeconds = 1;
    settings.staticWallpaper.currentIndex = 99;
    settings.staticWallpaper.compressionQuality = 500;
    settings.staticWallpaper.order = mw::StaticSlideshowOrder::Shuffle;
    settings.staticWallpaper.storageDirectory = "C:/captures";
    settings.staticWallpaper.imagePaths = {
        "C:/captures/one.bmp",
        "C:/captures/one.bmp",
        "C:/captures/two.bmp",
    };
    const auto result = mw::ValidateAndNormalise(settings);
    Check(result.valid, "A valid slideshow should normalise without a fatal validation issue.");
    Check(settings.schemaVersion == 12, "Slideshow settings should use schema version 12.");
    Check(settings.staticWallpaper.cycleSeconds == 10, "Slideshow interval should clamp to ten seconds.");
    Check(settings.staticWallpaper.compressionQuality == 100,
          "Saved-image compression/quality should clamp to 100.");
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

    std::string exactSampleError;
    mw::ExactCamera exactSampleCamera;
    Check(mw::ExactDecimal::Parse("-7.4364388703715100000000000001e-1",
                                  exactSampleCamera.centreX, exactSampleError) &&
              mw::ExactDecimal::Parse("1.3182590420533000000000000001e-1",
                                      exactSampleCamera.centreY, exactSampleError) &&
              mw::ExactDecimal::Parse("6.5e-3", exactSampleCamera.halfHeight, exactSampleError),
          "Exact global-sample fixture should construct retained camera text.");
    mw::ExactStillRenderSample exactSample;
    Check(mw::BuildExactStillRenderPixelSample(exactSampleCamera, 0.0, 400U, 200U,
                                                199U, 99U, exactSample, exactSampleError) &&
              exactSample.camera == exactSampleCamera &&
              exactSample.horizontalHalfHeightFactor == mw::ExactRationalOffset{-1, 200U} &&
              exactSample.verticalHalfHeightFactor == mw::ExactRationalOffset{1, 200U} &&
              !exactSample.requiresRotationAdapter,
          "Exact global samples should retain camera text and use rational centre-pixel factors.");
    Check(mw::BuildExactStillRenderSubpixelSample(exactSampleCamera, 0.0, 400U, 200U,
                                                   199U, 99U, 2U, 0U, 0U,
                                                   exactSample, exactSampleError) &&
              exactSample.horizontalHalfHeightFactor == mw::ExactRationalOffset{-3, 400U} &&
              exactSample.verticalHalfHeightFactor == mw::ExactRationalOffset{3, 400U},
          "Exact subpixel samples should retain rational AA offsets without a binary coordinate conversion.");
    Check(mw::BuildExactStillRenderPixelSample(exactSampleCamera, 90.0, 400U, 200U,
                                                200U, 100U, exactSample, exactSampleError) &&
              exactSample.requiresRotationAdapter,
          "Rotated exact samples must explicitly retain the unresolved rotation adapter boundary.");

    const auto rotatedRight = mw::MapStillRenderSample(
        fullCamera, 90.0, 400, 200, 400.0, 100.0);
    Check(std::abs(rotatedRight.real) < 1.0e-12 &&
              std::abs(rotatedRight.imaginary - 3.0) < 1.0e-12,
          "A positive 90-degree view rotation should rotate the right edge toward positive imaginary coordinates.");
    const auto rotatedTile = mw::CameraForStillRenderTile(
        fullCamera, 400, 200, 0, 0, 200, 100, 90.0);
    const auto rotatedTileCentre = mw::MapStillRenderSample(
        fullCamera, 90.0, 400, 200, 100.0, 50.0);
    Check(std::abs(mw::CameraCentreX(rotatedTile) - rotatedTileCentre.real) < 1.0e-12 &&
              std::abs(mw::CameraCentreY(rotatedTile) - rotatedTileCentre.imaginary) < 1.0e-12,
          "A rotated GPU tile camera should share the exact global mapping of its tile centre.");

    mw::Preset overlapPreset = mw::BuiltInPresets().front();
    overlapPreset.equation.glowStrength = 0.0;
    overlapPreset.equation.bloomRadius = 6;
    overlapPreset.antiAliasingLevel = 1;
    Check(mw::StillRenderTileOverlapPixels(overlapPreset) == 0U,
          "Disabled bloom with single-sample rendering should require no tile overlap.");
    overlapPreset.equation.glowStrength = 0.5;
    overlapPreset.equation.bloomRadius = 3;
    Check(mw::StillRenderTileOverlapPixels(overlapPreset) == 3U,
          "GPU tile overlap should cover the configured bloom radius.");
    overlapPreset.antiAliasingLevel = 4;
    Check(mw::StillRenderTileOverlapPixels(overlapPreset) == 4U,
          "GPU tile overlap should include the bloom and reconstruction radii.");

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


void TestFractalScout() {
    mw::FractalScoutRequest request;
    request.preset = mw::BuiltInPresets().front();
    request.searchCamera = request.preset.camera;
    request.candidatePoolSize = 12;
    request.resultCount = 5;
    request.sampleGridWidth = 7;
    request.sampleGridHeight = 5;
    request.thumbnailWidth = 48;
    request.thumbnailHeight = 32;
    request.maximumIterations = 48;
    request.searchRadius = 0.8;
    request.refinementScale = 0.4;
    request.scaleBandCount = 3;
    request.scaleBandSpread = 0.5;
    request.minimumResultSeparation = 0.45;
    request.goal = mw::FractalScoutGoal::Filaments;

    mw::FractalScoutRequest deepExactRequest = request;
    std::string deepExactError;
    Check(mw::EnsureExactCamera(deepExactRequest.preset, deepExactError) &&
              mw::ExactDecimal::Parse("1e-1000", deepExactRequest.preset.exactCamera->halfHeight,
                                      deepExactError),
          "Deep Scout fixture should retain authoritative exact camera text: " + deepExactError);
    mw::FractalScoutResult deepExactResult;
    Check(!mw::RunFractalScout(deepExactRequest, {}, {}, deepExactResult, deepExactError) &&
              deepExactError.find("exact Scout coordinates") != std::string::npos,
          "Double-coordinate Scout must refuse a lossy exact source instead of producing approximate candidates.");

    const auto limits = mw::ResolveFractalScoutLimits(request);
    Check(limits.candidatePoolSize == 12U && limits.resultCount == 5U,
          "Fractal Scout should preserve bounded request sizes.");
    Check(limits.thumbnailWidth == 48U && limits.thumbnailHeight == 32U,
          "Fractal Scout should preserve bounded thumbnail dimensions.");
    Check(limits.scaleBandCount == 3U &&
              std::abs(limits.scaleBandSpread - 0.5) < 1.0e-12 &&
              std::abs(limits.minimumResultSeparation - 0.45) < 1.0e-12,
          "Fractal Scout should preserve bounded multi-scale diversity settings.");

    mw::FractalScoutMetrics boundaryMetrics;
    boundaryMetrics.boundaryMix = 1.0;
    boundaryMetrics.edgeDensity = 0.6;
    mw::FractalScoutMetrics filamentMetrics;
    filamentMetrics.iterationVariance = 0.8;
    filamentMetrics.edgeDensity = 1.0;
    filamentMetrics.detail = 1.0;
    mw::FractalScoutMetrics symmetryMetrics;
    symmetryMetrics.boundaryMix = 0.4;
    symmetryMetrics.edgeDensity = 0.4;
    symmetryMetrics.symmetry = 1.0;
    Check(mw::CalculateFractalScoutScore(boundaryMetrics, mw::FractalScoutGoal::Boundary) >
              mw::CalculateFractalScoutScore(filamentMetrics, mw::FractalScoutGoal::Boundary),
          "Boundary-targeted Scout scoring should prefer mixed boundary structures.");
    Check(mw::CalculateFractalScoutScore(filamentMetrics, mw::FractalScoutGoal::Filaments) >
              mw::CalculateFractalScoutScore(boundaryMetrics, mw::FractalScoutGoal::Filaments),
          "Filament-targeted Scout scoring should prefer edge-rich detailed structures.");
    Check(mw::CalculateFractalScoutScore(symmetryMetrics, mw::FractalScoutGoal::Symmetry) >
              mw::CalculateFractalScoutScore(boundaryMetrics, mw::FractalScoutGoal::Symmetry),
          "Symmetry-targeted Scout scoring should prefer symmetric structures.");

    mw::FractalScoutResult first;
    mw::FractalScoutResult second;
    std::string error;
    std::uint32_t lastProgress = 0U;
    const bool firstSucceeded = mw::RunFractalScout(
        request,
        [&](const mw::FractalScoutProgress& progress) {
            lastProgress = progress.completedCandidates;
        },
        [] { return false; }, first, error);
    Check(firstSucceeded, "Fractal Scout should complete a bounded search: " + error);
    Check(!first.cancelled && first.evaluatedCandidates == request.candidatePoolSize,
          "Fractal Scout should evaluate the deterministic candidate pool.");
    Check(first.candidates.size() == request.resultCount,
          "Fractal Scout should return the requested bounded result count.");
    Check(lastProgress == request.resultCount,
          "Fractal Scout progress should finish after thumbnail generation.");
    Check(first.suppressedNearDuplicates > 0U,
          "Fractal Scout should suppress near-duplicate ranked candidates before thumbnail rendering.");
    std::vector<double> resultScales;
    for (std::size_t index = 0; index < first.candidates.size(); ++index) {
        const auto& candidate = first.candidates[index];
        Check(candidate.score >= 0.0 && candidate.score <= 1.0,
              "Fractal Scout scores should remain normalised.");
        Check(candidate.thumbnail.width == request.thumbnailWidth &&
                  candidate.thumbnail.height == request.thumbnailHeight,
              "Fractal Scout should render complete bounded thumbnails.");
        Check(candidate.thumbnail.pixels.size() ==
                  static_cast<std::size_t>(request.thumbnailWidth) * request.thumbnailHeight,
              "Fractal Scout thumbnails should contain all pixels.");
        Check(candidate.preset.id.empty() && !candidate.preset.builtIn,
              "Fractal Scout candidates should remain unsaved preview values.");
        mw::ExactCamera expectedCandidateCamera;
        std::string candidateExactError;
        Check(mw::BuildExactCameraFromLegacy(candidate.preset.camera, expectedCandidateCamera,
                                             candidateExactError) &&
                  candidate.preset.exactCamera.has_value() &&
                  *candidate.preset.exactCamera == expectedCandidateCamera,
              "Fractal Scout candidates must rebuild exact camera authority after changing legacy coordinates.");
        Check(candidate.identity.size() == 64U &&
                  candidate.identity.find_first_not_of("0123456789abcdef") == std::string::npos,
              "Fractal Scout candidates should carry a stable SHA-256 identity.");
        resultScales.push_back(candidate.preset.camera.scale);
        if (index > 0U) {
            Check(first.candidates[index - 1U].score >= candidate.score,
                  "Fractal Scout results should be sorted by descending score.");
        }
    }
    std::sort(resultScales.begin(), resultScales.end());
    const auto uniqueScaleEnd = std::unique(resultScales.begin(), resultScales.end(),
        [](double left, double right) {
            return std::abs(std::log(left / right)) < 1.0e-9;
        });
    Check(std::distance(resultScales.begin(), uniqueScaleEnd) >= 2,
          "Fractal Scout should retain candidates from multiple deterministic depth bands.");

    error.clear();
    const bool secondSucceeded = mw::RunFractalScout(
        request, {}, [] { return false; }, second, error);
    Check(secondSucceeded, "A repeated Fractal Scout search should complete: " + error);
    Check(second.candidates.size() == first.candidates.size(),
          "Repeated Fractal Scout searches should return the same result count.");
    Check(second.suppressedNearDuplicates == first.suppressedNearDuplicates,
          "Repeated Fractal Scout searches should preserve the diversity suppression count.");
    if (second.candidates.size() == first.candidates.size()) {
        Check(first.suppressedNearDuplicates > 0U,
          "Fractal Scout should suppress near-duplicate ranked candidates before thumbnail rendering.");
    std::vector<double> resultScales;
    for (std::size_t index = 0; index < first.candidates.size(); ++index) {
            Check(std::abs(first.candidates[index].score - second.candidates[index].score) < 1.0e-12 &&
                      std::abs(mw::CameraCentreX(first.candidates[index].preset.camera) -
                               mw::CameraCentreX(second.candidates[index].preset.camera)) < 1.0e-15 &&
                      std::abs(mw::CameraCentreY(first.candidates[index].preset.camera) -
                               mw::CameraCentreY(second.candidates[index].preset.camera)) < 1.0e-15 &&
                      std::abs(std::log(first.candidates[index].preset.camera.scale /
                                       second.candidates[index].preset.camera.scale)) < 1.0e-15 &&
                      first.candidates[index].identity == second.candidates[index].identity &&
                      first.candidates[index].thumbnail.pixels == second.candidates[index].thumbnail.pixels,
                  "Fractal Scout scoring, identity and thumbnails should be deterministic for identical inputs.");
        }
    }

    std::uint32_t cancellationChecks = 0U;
    mw::FractalScoutResult cancelled;
    error.clear();
    const bool cancellationSucceeded = mw::RunFractalScout(
        request, {}, [&] { return ++cancellationChecks > 2U; }, cancelled, error);
    Check(cancellationSucceeded && cancelled.cancelled,
          "Fractal Scout should report bounded cancellation without treating it as an error.");
    Check(cancelled.candidates.empty(),
          "A cancelled Fractal Scout search should not expose partial candidates.");

    mw::FractalScoutRequest excessive = request;
    excessive.candidatePoolSize = 10000U;
    excessive.resultCount = 10000U;
    excessive.thumbnailWidth = 10000U;
    excessive.thumbnailHeight = 10000U;
    excessive.maximumIterations = 100000;
    excessive.searchRadius = std::numeric_limits<double>::infinity();
    excessive.scaleBandCount = 1000U;
    excessive.scaleBandSpread = std::numeric_limits<double>::infinity();
    excessive.minimumResultSeparation = 1000.0;
    const auto clamped = mw::ResolveFractalScoutLimits(excessive);
    Check(clamped.candidatePoolSize == 96U && clamped.resultCount <= 24U,
          "Fractal Scout should cap candidate work and retained results.");
    Check(clamped.thumbnailWidth == 256U && clamped.thumbnailHeight == 180U &&
              clamped.maximumIterations == 768,
          "Fractal Scout should cap thumbnail and iteration work.");
    Check(std::abs(clamped.searchRadius - 0.9) < 1.0e-12,
          "Fractal Scout should replace non-finite search radii with a deterministic fallback.");
    Check(clamped.scaleBandCount == 3U &&
              std::abs(clamped.scaleBandSpread - 0.38) < 1.0e-12 &&
              std::abs(clamped.minimumResultSeparation - 0.75) < 1.0e-12,
          "Fractal Scout should cap depth bands, spread and result separation.");
}

void TestIndependentHighPrecisionBackend() {
    const auto& orbitEncoding = mw::CurrentOrbitEncoding();
    Check(orbitEncoding.identifier == "mw-orbit-float4-expansion/v1" &&
              orbitEncoding.version == 1 && orbitEncoding.componentsPerCoordinate == 4 &&
              orbitEncoding.componentBits == 32 && !orbitEncoding.validatedCeilingAvailable &&
              orbitEncoding.validatedBits == 0,
          "The existing four-float orbit transport must have a named unmeasured v1 contract.");
    const auto& backend = mw::IndependentHighPrecisionBackend();
    Check(backend.packageName == "Boost.Multiprecision standalone source subset" &&
              backend.packageVersion == "1.83.0" && backend.licence == "BSL-1.0",
          "The independent high-precision backend should expose the reviewed package identity.");
    Check(backend.precisionBits == 512 && backend.headerOnly &&
              !backend.requiresRuntimeArtifact && backend.fixedStorage,
          "The reviewed high-precision backend should remain fixed, header-only and runtime-artifact free.");

    mw::CameraState camera{-0.743643887037151, 0.131825904205330, 1.0e-24};
    mw::OffsetCamera(camera, 1.0e-24, -2.0e-24);
    const auto independent = mw::BuildIndependentHighPrecisionReferenceOrbit(
        camera, mw::EquationSettings{}, 128);
    const auto current = mw::BuildReferenceOrbitArbitrary(
        camera, mw::EquationSettings{}, 128, 512);
    Check(independent.precisionBits == 512 && independent.points.size() == 128U,
          "The independent backend should return a bounded 512-bit reference orbit.");
    const mw::OrbitEncodingMeasurement encodingMeasurement =
        mw::MeasureIndependentOrbitEncoding(camera, mw::EquationSettings{}, 128);
    Check(encodingMeasurement.sourcePrecisionBits == 512 &&
              encodingMeasurement.requestedIterations == 128 &&
              encodingMeasurement.comparedCoordinates == 256 &&
              std::isfinite(encodingMeasurement.maximumAbsoluteError) &&
              std::isfinite(encodingMeasurement.maximumRelativeError) &&
              encodingMeasurement.maximumAbsoluteError > 0.0 &&
              encodingMeasurement.maximumRelativeError > 0.0,
          "The orbit encoding measurement should report finite non-zero bounded fixture error.");
    Check(independent.escaped == current.escaped &&
              independent.escapeIteration == current.escapeIteration,
          "The independent backend should agree with the existing arbitrary path on escape metadata.");

    const auto expansionValue = [](const mw::ReferenceOrbitPoint& point) {
        std::complex<double> value{};
        for (float component : point.real) value.real(value.real() + component);
        for (float component : point.imaginary) value.imag(value.imag() + component);
        return value;
    };
    const auto sameFloat4Payload = [](const mw::ReferenceOrbitPoint& expected,
                                      const mw::ReferenceOrbitPoint& actual) {
        for (std::size_t component = 0; component < expected.real.size(); ++component) {
            if (std::bit_cast<std::uint32_t>(expected.real[component]) !=
                    std::bit_cast<std::uint32_t>(actual.real[component]) ||
                std::bit_cast<std::uint32_t>(expected.imaginary[component]) !=
                    std::bit_cast<std::uint32_t>(actual.imaginary[component])) {
                return false;
            }
        }
        return true;
    };
    for (std::size_t index : {2U, 8U, 20U, 40U}) {
        const auto expected = expansionValue(current.points.at(index));
        const auto actual = expansionValue(independent.points.at(index));
        Check(std::abs(expected - actual) < 2.0e-12,
              "The independent Boost reference should agree with the existing bounded orbit expansion.");
        Check(sameFloat4Payload(current.points.at(index), independent.points.at(index)),
              "The independent Boost reference should preserve every selected float4 orbit payload component.");
    }
    const mw::EquationSettings independentTricornEquation = mw::EquationExample(4);
    const auto tricornCurrent = mw::BuildReferenceOrbitArbitrary(
        camera, independentTricornEquation, 128, 512);
    const auto tricornIndependent = mw::BuildIndependentHighPrecisionReferenceOrbit(
        camera, independentTricornEquation, 128);
    Check(tricornCurrent.escaped == tricornIndependent.escaped &&
              tricornCurrent.escapeIteration == tricornIndependent.escapeIteration,
          "The independent Boost Tricorn reference should agree with the existing arbitrary path on escape metadata.");
    for (std::size_t index : {2U, 8U, 20U, 40U}) {
        Check(sameFloat4Payload(tricornCurrent.points.at(index), tricornIndependent.points.at(index)),
              "The independent Boost Tricorn reference should preserve every selected float4 orbit payload component.");
    }
    mw::ExactCamera exactCamera;
    std::string exactCameraError;
    Check(mw::BuildExactCameraFromLegacy(camera, exactCamera, exactCameraError),
          "The exact-backend fixture should reconstruct canonical camera text: " + exactCameraError);
    const auto exactIndependent = mw::BuildIndependentHighPrecisionReferenceOrbit(
        exactCamera, mw::EquationSettings{}, 128);
    Check(exactIndependent.points.size() == independent.points.size() &&
              exactIndependent.escaped == independent.escaped &&
              std::abs(expansionValue(exactIndependent.points.at(20U)) -
                       expansionValue(independent.points.at(20U))) < 2.0e-12,
          "The exact-camera backend entry should agree with the equivalent legacy reconstruction.");
    mw::ExactCamera ordinaryExactCamera;
    Check(mw::ExactDecimal::Parse("0", ordinaryExactCamera.centreX, exactCameraError) &&
              mw::ExactDecimal::Parse("0", ordinaryExactCamera.centreY, exactCameraError) &&
              mw::ExactDecimal::Parse("1", ordinaryExactCamera.halfHeight, exactCameraError),
          "The exact-backend ordinary-decimal fixture should parse canonical values without an exponent marker: " +
              exactCameraError);
    const auto ordinaryExactOrbit = mw::BuildIndependentHighPrecisionReferenceOrbit(
        ordinaryExactCamera, mw::EquationSettings{}, 64);
    Check(ordinaryExactOrbit.points.size() == 64U && !ordinaryExactOrbit.escaped,
          "The independent backend must safely accept canonical exact values without an exponent marker.");
    mw::ExactStillRenderSample exactSample;
    Check(mw::BuildExactStillRenderPixelSample(exactCamera, 0.0, 8U, 6U, 3U, 2U,
                                               exactSample, exactCameraError),
          "The direct high-precision fixture should build an unrotated exact pixel sample: " +
              exactCameraError);
    mw::HighPrecisionEscapeResult exactEscape;
    Check(mw::EvaluateIndependentHighPrecisionSample(exactSample, mw::EquationSettings{}, 128,
                                                      {}, exactEscape, exactCameraError),
          "The direct high-precision evaluator should accept an exact unrotated pixel: " +
              exactCameraError);
    const mw::ComplexPlanePoint mappedSample = mw::MapStillRenderSample(
        camera, 0.0, 8U, 6U, 3.5, 2.5);
    const mw::EscapeResult doubleEscape = mw::CalculateEscape(
        mappedSample.real, mappedSample.imaginary, 128, mw::EquationSettings{});
    Check(exactEscape.precisionBits == 512 && exactEscape.escaped == doubleEscape.escaped &&
              exactEscape.iterations == doubleEscape.iterations &&
              std::abs(exactEscape.smoothValue - doubleEscape.smoothValue) < 1.0e-10,
          "The exact direct evaluator should agree with ordinary CPU escape classification and smoothing at a representable sample.");
    const mw::Preset colourPreset = mw::BuiltInPresets().front();
    const std::vector<mw::Colour> colourPalette = mw::PalettePreviewColours(colourPreset.palette);
    const auto directColour = mw::ColourForEscape(colourPreset, colourPalette,
                                                   mw::ToEscapeResult(exactEscape), 128);
    const auto ordinaryColour = mw::ColourForEscape(colourPreset, colourPalette, doubleEscape, 128);
    Check(std::abs(directColour[0] - ordinaryColour[0]) < 1.0e-12 &&
              std::abs(directColour[1] - ordinaryColour[1]) < 1.0e-12 &&
              std::abs(directColour[2] - ordinaryColour[2]) < 1.0e-12,
          "Direct exact samples should reuse the canonical CPU colour mapping for supported smooth escape output.");
    mw::Preset comparisonPreset = colourPreset;
    comparisonPreset.camera = camera;
    comparisonPreset.rotationDegrees = 0.0;
    comparisonPreset.antiAliasingLevel = 1;
    std::vector<std::uint32_t> exactPixels;
    mw::StillRenderResult exactStillResult;
    mw::ExactDirectStillRenderRequest exactStillRequest{comparisonPreset, exactCamera, 8U, 6U, 128};
    Check(mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&exactPixels](std::uint32_t, std::span<const std::uint32_t> row,
                             std::string&) {
                  exactPixels.insert(exactPixels.end(), row.begin(), row.end());
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) &&
              exactPixels.size() == 48U && exactStillResult.statistics.renderedPixels == 48U,
          "The bounded exact direct still renderer should stream every global sample: " +
              exactCameraError);
    std::vector<std::uint32_t> ordinaryPixels;
    mw::StillRenderRequest ordinaryStillRequest;
    ordinaryStillRequest.preset = comparisonPreset;
    ordinaryStillRequest.width = 8U;
    ordinaryStillRequest.height = 6U;
    ordinaryStillRequest.tileWidth = 8U;
    ordinaryStillRequest.previewMaximumWidth = 0U;
    ordinaryStillRequest.previewMaximumHeight = 0U;
    ordinaryStillRequest.scaleQualityToResolution = false;
    mw::StillRenderResult ordinaryStillResult;
    Check(mw::RenderStillImageTiled(
              ordinaryStillRequest,
              [&ordinaryPixels](std::uint32_t, std::span<const std::uint32_t> row,
                                  std::string&) {
                  ordinaryPixels.insert(ordinaryPixels.end(), row.begin(), row.end());
                  return true;
              }, {}, {}, ordinaryStillResult, exactCameraError) && exactPixels == ordinaryPixels,
          "Exact direct still output should agree pixel-for-pixel with ordinary CPU output at representable coordinates.");
    mw::ExactDirectStillRenderRequest exactTileRequest = exactStillRequest;
    exactTileRequest.width = 4U;
    exactTileRequest.height = 3U;
    exactTileRequest.fullWidth = 8U;
    exactTileRequest.fullHeight = 6U;
    exactTileRequest.tileOriginX = 2U;
    exactTileRequest.tileOriginY = 1U;
    std::vector<std::uint32_t> exactTilePixels;
    Check(mw::RenderExactDirectStillImage(
              exactTileRequest,
              [&exactTilePixels](std::uint32_t, std::span<const std::uint32_t> row,
                                  std::string&) {
                  exactTilePixels.insert(exactTilePixels.end(), row.begin(), row.end());
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) &&
              exactTilePixels.size() == 12U &&
              std::equal(exactTilePixels.begin(), exactTilePixels.begin() + 4,
                         exactPixels.begin() + 10U) &&
              std::equal(exactTilePixels.begin() + 4, exactTilePixels.begin() + 8,
                         exactPixels.begin() + 18U) &&
              std::equal(exactTilePixels.begin() + 8, exactTilePixels.end(),
                         exactPixels.begin() + 26U),
          "An exact direct tile must use the full-frame global mapping rather than a local crop camera.");
    exactTileRequest.tileOriginX = 6U;
    Check(!mw::RenderExactDirectStillImage(
              exactTileRequest,
              [](std::uint32_t, std::span<const std::uint32_t>, std::string&) { return true; },
              {}, {}, exactStillResult, exactCameraError) &&
              exactCameraError.find("tile origin") != std::string::npos,
          "An exact direct tile that exceeds its declared full frame must fail before rendering.");
    mw::ExactDirectStillRenderRequest oversizedExactStillRequest = exactStillRequest;
    oversizedExactStillRequest.width = 16U * 1024U * 1024U + 1U;
    Check(!mw::RenderExactDirectStillImage(
              oversizedExactStillRequest,
              [](std::uint32_t, std::span<const std::uint32_t>, std::string&) { return true; },
              {}, {}, exactStillResult, exactCameraError) &&
              exactCameraError.find("64 MiB working-memory bound") != std::string::npos,
          "Exact direct still rendering must reject an oversized row before allocating it.");
    exactStillRequest.preset.antiAliasingLevel = 2;
    std::vector<std::uint32_t> exactAaPixels;
    std::vector<std::uint32_t> repeatedExactAaPixels;
    Check(mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&exactAaPixels](std::uint32_t, std::span<const std::uint32_t> row,
                               std::string&) {
                  exactAaPixels.insert(exactAaPixels.end(), row.begin(), row.end());
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) &&
              exactStillResult.statistics.antiAliasingLevel == 2 && exactAaPixels.size() == 48U &&
              mw::RenderExactDirectStillImage(
                  exactStillRequest,
                  [&repeatedExactAaPixels](std::uint32_t, std::span<const std::uint32_t> row,
                                            std::string&) {
                      repeatedExactAaPixels.insert(repeatedExactAaPixels.end(), row.begin(), row.end());
                      return true;
                  }, {}, {}, exactStillResult, exactCameraError) &&
              repeatedExactAaPixels == exactAaPixels,
          "Exact direct still rendering should support deterministic rational AA-2 samples.");
    exactStillRequest.preset.antiAliasingLevel = 4;
    std::vector<std::uint32_t> exactAa4Pixels;
    Check(mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&exactAa4Pixels](std::uint32_t, std::span<const std::uint32_t> row,
                                 std::string&) {
                  exactAa4Pixels.insert(exactAa4Pixels.end(), row.begin(), row.end());
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) &&
              exactStillResult.statistics.antiAliasingLevel == 4 && exactAa4Pixels.size() == 48U,
          "Exact direct still rendering should support the bounded rational AA-4 upper limit.");
    exactStillRequest.preset.antiAliasingLevel = 1;
    exactStillRequest.preset.equation.animateCoefficients = true;
    Check(!mw::RenderExactDirectStillImage(
              exactStillRequest,
              [](std::uint32_t, std::span<const std::uint32_t>, std::string&) { return true; },
              {}, {}, exactStillResult, exactCameraError) &&
              exactCameraError.find("animated equation coefficients") != std::string::npos,
          "Exact direct still rendering must refuse coefficient animation it cannot evaluate by time phase.");
    Check(!mw::EvaluateIndependentHighPrecisionSample(exactSample, exactStillRequest.preset.equation,
                                                      128, {}, exactEscape, exactCameraError) &&
              exactCameraError.find("animated equation coefficients") != std::string::npos,
          "The public exact evaluator must refuse coefficient animation rather than silently ignore it.");
    exactStillRequest.preset.equation.animateCoefficients = false;
    exactStillRequest.preset.rotationDegrees = 1.0;
    Check(!mw::RenderExactDirectStillImage(exactStillRequest, {}, {}, {}, exactStillResult,
                                            exactCameraError) &&
              exactCameraError.find("dimensions and a row writer") != std::string::npos,
          "Exact direct still requests must require a row writer before execution.");
    mw::ExactCamera deepStillCamera = exactCamera;
    Check(mw::ExactDecimal::Parse("1e-40", deepStillCamera.halfHeight, exactCameraError),
          "The deep exact direct-still fixture should parse canonical camera scale: " +
              exactCameraError);
    exactStillRequest.camera = deepStillCamera;
    exactStillRequest.preset.exactCamera = deepStillCamera;
    exactStillRequest.preset.rotationDegrees = 0.0;
    std::uint32_t deepRows = 0U;
    Check(mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&deepRows](std::uint32_t, std::span<const std::uint32_t>, std::string&) {
                  ++deepRows;
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) && deepRows == 6U &&
              exactStillResult.statistics.renderedPixels == 48U,
          "The Boost-512 direct still route must render a 1e-40 exact camera without legacy adaptation: " +
              exactCameraError);
    Check(mw::ExactDecimal::Parse("1e-200", deepStillCamera.halfHeight, exactCameraError),
          "The 2048-bit direct-still fixture should parse a deeper exact camera scale: " +
              exactCameraError);
    exactStillRequest.camera = deepStillCamera;
    exactStillRequest.preset.exactCamera = deepStillCamera;
    exactStillRequest.precisionBits = 2048;
    deepRows = 0U;
    Check(mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&deepRows](std::uint32_t, std::span<const std::uint32_t>, std::string&) {
                  ++deepRows;
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) && deepRows == 6U &&
              exactStillResult.statistics.renderedPixels == 48U,
          "The Boost-2048 direct still route must render a 1e-200 exact camera without legacy adaptation: " +
              exactCameraError);
    Check(mw::ExactDecimal::Parse("1e-1000", deepStillCamera.halfHeight, exactCameraError),
          "The 8192-bit direct-still fixture should parse a near-unlimited exact camera scale: " +
          exactCameraError);
    exactStillRequest.camera = deepStillCamera;
    exactStillRequest.preset.exactCamera = deepStillCamera;
    exactStillRequest.precisionBits = 8192;
    deepRows = 0U;
    Check(mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&deepRows](std::uint32_t, std::span<const std::uint32_t>, std::string&) {
                  ++deepRows;
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) && deepRows == 6U &&
              exactStillResult.statistics.renderedPixels == 48U,
          "The Boost-8192 direct still route must render a 1e-1000 exact camera without legacy adaptation: " +
          exactCameraError);
    Check(mw::ExactDecimal::Parse("1e-3000", deepStillCamera.halfHeight, exactCameraError),
          "The 16384-bit direct-still fixture should parse a deeper exact camera scale: " +
          exactCameraError);
    exactStillRequest.camera = deepStillCamera;
    exactStillRequest.preset.exactCamera = deepStillCamera;
    exactStillRequest.precisionBits = 16384;
    deepRows = 0U;
    Check(mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&deepRows](std::uint32_t, std::span<const std::uint32_t>, std::string&) {
                  ++deepRows;
                  return true;
              }, {}, {}, exactStillResult, exactCameraError) && deepRows == 6U &&
              exactStillResult.statistics.renderedPixels == 48U,
          "The Boost-16384 direct still route must render a 1e-3000 exact camera without legacy adaptation: " +
          exactCameraError);
    std::uint32_t cancelledDeepRows = 0U;
    int deepCancellationPolls = 0;
    Check(!mw::RenderExactDirectStillImage(
              exactStillRequest,
              [&cancelledDeepRows](std::uint32_t, std::span<const std::uint32_t>, std::string&) {
                  ++cancelledDeepRows;
                  return true;
              }, {}, [&deepCancellationPolls] { return ++deepCancellationPolls >= 2; },
              exactStillResult, exactCameraError) &&
              cancelledDeepRows == 0U &&
              exactCameraError.find("cancelled") != std::string::npos,
          "A cancelled 8192-bit direct exact render must publish no partial output row.");
    exactStillRequest.precisionBits = 512;
    exactSample.rotationDegrees = 1.0;
    exactSample.requiresRotationAdapter = true;
    Check(!mw::EvaluateIndependentHighPrecisionSample(exactSample, mw::EquationSettings{}, 128,
                                                       {}, exactEscape, exactCameraError) &&
              exactCameraError.find("rotated") != std::string::npos,
          "The direct high-precision evaluator must fail closed until exact rotation is implemented.");
    exactSample.rotationDegrees = 0.0;
    exactSample.requiresRotationAdapter = false;
    Check(!mw::EvaluateIndependentHighPrecisionSample(exactSample, mw::EquationSettings{}, 128,
                                                       [] { return true; }, exactEscape,
                                                       exactCameraError) &&
              exactCameraError.find("cancelled") != std::string::npos,
          "The direct high-precision evaluator must stop before publishing a cancelled sample.");
    mw::PrecisionBackendCapabilities plannerCapabilities;
    const mw::PrecisionPlan referencePlan = mw::BuildPrecisionPlan(
        exactCamera, mw::EquationSettings{}, mw::PrecisionMode::ArbitraryPrecisionPerturbation,
        plannerCapabilities);
    mw::ReferenceOrbitService service;
    mw::ReferenceOrbitRequest serviceRequest{exactCamera, mw::EquationSettings{}, referencePlan, 64, 7U};
    mw::ReferenceOrbitResult serviceResult;
    Check(service.Build(serviceRequest, {}, serviceResult, exactCameraError) &&
              !serviceResult.cacheHit && serviceResult.generation == 7U &&
              serviceResult.orbit.points.size() == 64U && service.CachedEntries() == 1U,
          "The bounded reference service should publish a matching-generation Boost orbit: " +
              exactCameraError);
    Check(serviceResult.byteSize > 0U,
          "The bounded reference service should report its cached orbit byte size.");
    Check(serviceResult.precisionPlanVersion == mw::kPrecisionPlanVersion &&
              serviceResult.formulaCapabilityId == "analytic-quadratic-mandelbrot" &&
              serviceResult.formulaCapabilityVersion == 1 &&
              serviceResult.orbitEncodingIdentifier == "mw-orbit-float4-expansion/v1" &&
              serviceResult.orbitEncodingVersion == 1,
          "Reference-orbit results must preserve immutable plan, formula and orbit-encoding identity.");
    mw::ReferenceOrbitRequest deepServiceRequest = serviceRequest;
    Check(mw::ExactDecimal::Parse("1e-3000", deepServiceRequest.camera.halfHeight,
                                  exactCameraError),
          "The 16384-bit reference-orbit fixture should parse deeper exact camera text: " +
              exactCameraError);
    mw::PrecisionBackendCapabilities deepReferenceCapabilities;
    deepReferenceCapabilities.cpuBoost512Reference = false;
    deepReferenceCapabilities.cpuBoost16384Direct = true;
    deepServiceRequest.plan = mw::BuildPrecisionPlan(
        deepServiceRequest.camera, deepServiceRequest.equation,
        mw::PrecisionMode::Automatic, deepReferenceCapabilities);
    deepServiceRequest.generation = 11U;
    mw::ReferenceOrbitService deepService;
    mw::ReferenceOrbitResult deepServiceResult;
    Check(deepService.Build(deepServiceRequest, {}, deepServiceResult, exactCameraError) &&
              deepServiceResult.orbit.precisionBits == 16384 &&
              deepServiceResult.plan.selectedBits == 16384 &&
              deepServiceResult.orbit.points.size() == 64U,
          "The reference-orbit service must preserve the planner-selected 16384-bit exact tier: " +
              exactCameraError);
    const std::string_view acceptedPlanVersion = serviceRequest.plan.version;
    serviceRequest.plan.version = "mw-precision-plan-untrusted-v99";
    mw::ReferenceOrbitResult substitutedPlanResult;
    Check(!service.Build(serviceRequest, {}, substitutedPlanResult, exactCameraError) &&
              exactCameraError.find("precision-plan version") != std::string::npos &&
              service.CachedEntries() == 1U,
          "Reference-orbit service must reject an unregistered precision-plan version without cache mutation.");
    serviceRequest.plan.version = acceptedPlanVersion;
    const int acceptedEncodingVersion = serviceRequest.orbitEncodingVersion;
    serviceRequest.orbitEncodingVersion = acceptedEncodingVersion + 1;
    mw::ReferenceOrbitResult substitutedEncodingResult;
    Check(!service.Build(serviceRequest, {}, substitutedEncodingResult, exactCameraError) &&
              exactCameraError.find("orbit-encoding contract") != std::string::npos &&
              service.CachedEntries() == 1U,
          "Reference-orbit service must reject a substituted orbit-encoding version without cache mutation.");
    serviceRequest.orbitEncodingVersion = acceptedEncodingVersion;
    serviceRequest.generation = 8U;
    mw::ReferenceOrbitResult cachedServiceResult;
    Check(service.Build(serviceRequest, {}, cachedServiceResult, exactCameraError) &&
              cachedServiceResult.cacheHit && cachedServiceResult.generation == 8U,
          "Reference cache reuse should retain numerical data but stamp the caller generation.");
    mw::ReferenceOrbitResult cancelledServiceResult;
    Check(!service.Build(serviceRequest, [] { return true; }, cancelledServiceResult, exactCameraError) &&
              exactCameraError.find("cancelled") != std::string::npos,
          "Reference-orbit service cancellation must prevent cache publication or delivery.");
    mw::ReferenceOrbitService singleEntryService({serviceResult.byteSize, 1U});
    mw::ReferenceOrbitResult singleEntryFirst;
    Check(singleEntryService.Build(serviceRequest, {}, singleEntryFirst, exactCameraError) &&
              !singleEntryFirst.cacheHit && singleEntryService.CachedEntries() == 1U &&
              singleEntryService.CachedBytes() == serviceResult.byteSize,
          "A single-entry reference cache should account for its first bounded orbit.");
    mw::ReferenceOrbitRequest alternateRequest = serviceRequest;
    Check(mw::ExactDecimal::Parse("-7.43643887037150e-1", alternateRequest.camera.centreX,
                                  exactCameraError),
          "The cache-eviction fixture should parse an alternate exact camera: " + exactCameraError);
    alternateRequest.generation = 9U;
    mw::ReferenceOrbitResult singleEntryAlternate;
    Check(singleEntryService.Build(alternateRequest, {}, singleEntryAlternate, exactCameraError) &&
              !singleEntryAlternate.cacheHit && singleEntryService.CachedEntries() == 1U &&
              singleEntryService.CachedBytes() <= serviceResult.byteSize,
          "A distinct orbit should evict the least-recent single-entry cache member within its byte bound.");
    serviceRequest.generation = 10U;
    mw::ReferenceOrbitResult reloadedSingleEntry;
    Check(singleEntryService.Build(serviceRequest, {}, reloadedSingleEntry, exactCameraError) &&
              !reloadedSingleEntry.cacheHit && singleEntryService.CachedEntries() == 1U,
          "An evicted reference orbit must be rebuilt rather than returned as a stale cache hit.");
    mw::ReferenceOrbitService overBudgetService({serviceResult.byteSize - 1U, 1U});
    mw::ReferenceOrbitResult overBudgetResult;
    Check(!overBudgetService.Build(serviceRequest, {}, overBudgetResult, exactCameraError) &&
              exactCameraError.find("exceeds the configured cache budget") != std::string::npos &&
              overBudgetService.CachedEntries() == 0U && overBudgetService.CachedBytes() == 0U,
          "A reference orbit larger than the byte budget must be refused without cache publication.");
    mw::ReferenceOrbitWorker worker;
    std::mutex workerMutex;
    std::condition_variable workerCondition;
    std::vector<mw::ReferenceOrbitWorkCompletion> workerCompletions;
    auto collectCompletion = [&](mw::ReferenceOrbitWorkCompletion completion) {
        {
            std::lock_guard lock(workerMutex);
            workerCompletions.push_back(std::move(completion));
        }
        workerCondition.notify_all();
    };
    serviceRequest.generation = 9U;
    serviceRequest.plan.requiresDirectCorrection = false;
    const std::uint64_t firstWorkId = worker.Enqueue(serviceRequest, collectCompletion,
                                                     exactCameraError);
    serviceRequest.generation = 10U;
    serviceRequest.plan.requiresDirectCorrection = true;
    const std::uint64_t secondWorkId = worker.Enqueue(serviceRequest, collectCompletion,
                                                      exactCameraError);
    {
        std::unique_lock lock(workerMutex);
        workerCondition.wait_for(lock, std::chrono::seconds(5), [&] {
            return workerCompletions.size() == 2U;
        });
    }
    Check(firstWorkId != 0U && secondWorkId != 0U && workerCompletions.size() == 2U &&
              ((workerCompletions[0].generation == 9U && workerCompletions[1].generation == 10U) ||
               (workerCompletions[0].generation == 10U && workerCompletions[1].generation == 9U)) &&
              workerCompletions[0].result.orbit.points.size() == 64U &&
              workerCompletions[1].result.orbit.points.size() == 64U,
          "Reference-orbit worker should coalesce numerical work while stamping each subscriber generation.");
    bool generationNinePreserved = false;
    bool generationTenPreserved = false;
    for (const mw::ReferenceOrbitWorkCompletion& completion : workerCompletions) {
        generationNinePreserved = generationNinePreserved ||
            (completion.generation == 9U && !completion.result.plan.requiresDirectCorrection);
        generationTenPreserved = generationTenPreserved ||
            (completion.generation == 10U && completion.result.plan.requiresDirectCorrection);
    }
    Check(generationNinePreserved && generationTenPreserved,
          "Coalesced numerical work must deliver each subscriber's immutable correction policy.");
    worker.Shutdown();
    Check(worker.PendingKeys() == 0U,
          "Reference-orbit worker shutdown must join and release active and queued request state.");
    std::size_t shutdownCallbackCount = 0U;
    Check(worker.Enqueue(serviceRequest,
                         [&shutdownCallbackCount](mw::ReferenceOrbitWorkCompletion) {
                             ++shutdownCallbackCount;
                         },
                         exactCameraError) == 0U &&
              exactCameraError.find("shutting down") != std::string::npos &&
              shutdownCallbackCount == 0U,
          "A stopped reference-orbit worker must refuse new work without invoking its callback.");
    mw::ReferenceOrbitWorkerLimits zeroQueueLimits;
    zeroQueueLimits.maximumQueuedRequests = 0U;
    mw::ReferenceOrbitWorker zeroQueueWorker(zeroQueueLimits);
    Check(zeroQueueWorker.Enqueue(serviceRequest,
                                  [](mw::ReferenceOrbitWorkCompletion) {},
                                  exactCameraError) == 0U &&
              exactCameraError.find("queue is full") != std::string::npos &&
              zeroQueueWorker.PendingKeys() == 0U,
          "A zero-capacity reference-orbit worker must reject an independent key without retaining it.");
    zeroQueueWorker.Shutdown();
    Check(zeroQueueWorker.PendingKeys() == 0U,
          "A zero-capacity worker shutdown must leave no hidden queued or active request state.");
    mw::ReferenceOrbitWorkerLimits zeroSubscriberLimits;
    zeroSubscriberLimits.maximumSubscribersPerKey = 0U;
    mw::ReferenceOrbitWorker zeroSubscriberWorker(zeroSubscriberLimits);
    Check(zeroSubscriberWorker.Enqueue(serviceRequest,
                                       [](mw::ReferenceOrbitWorkCompletion) {},
                                       exactCameraError) == 0U &&
              exactCameraError.find("subscriber limit is zero") != std::string::npos &&
              zeroSubscriberWorker.PendingKeys() == 0U,
          "A zero-capacity reference-orbit subscriber list must reject work without retaining a key.");
    zeroSubscriberWorker.Shutdown();
    Check(zeroSubscriberWorker.PendingKeys() == 0U,
          "A zero-capacity subscriber worker shutdown must leave no retained request state.");
    std::string tooPrecise(155U, '1');
    Check(mw::ExactDecimal::Parse("1." + tooPrecise + "e-1", exactCamera.centreX,
                                  exactCameraError),
          "The fixed-backend envelope fixture should parse valid exact source text.");
    bool precisionEnvelopeRejected = false;
    try {
        (void)mw::BuildIndependentHighPrecisionReferenceOrbit(
            exactCamera, mw::EquationSettings{}, 64);
    } catch (const std::invalid_argument&) {
        precisionEnvelopeRejected = true;
    }
    Check(precisionEnvelopeRejected,
          "The fixed 512-bit backend must reject exact values it cannot preserve without rounding.");

    const auto tricorn = mw::EquationExample(4);
    const auto quadraticCapability = mw::DescribePerturbationProfile(
        mw::ResolvePerturbationProfile(mw::EquationSettings{}));
    const auto tricornCapability = mw::DescribePerturbationProfile(
        mw::ResolvePerturbationProfile(tricorn));
    Check(quadraticCapability.identifier == "analytic-quadratic-mandelbrot" &&
              quadraticCapability.version == 1 && quadraticCapability.supported &&
              tricornCapability.identifier == "tricorn-power2" &&
              tricornCapability.version == 1 && tricornCapability.supported,
          "Registered deep formula profiles must expose stable versioned capability identities.");
    Check(serviceResult.formulaCapabilityId == quadraticCapability.identifier &&
              serviceResult.formulaCapabilityVersion == quadraticCapability.version,
          "Reference-orbit results must report the immutable formula capability identity.");
    mw::CameraState tricornCamera{0.35, 0.42, 1.0e-8};
    const auto independentTricorn = mw::BuildIndependentHighPrecisionReferenceOrbit(
        tricornCamera, tricorn, 96);
    const std::complex<double> c{mw::CameraCentreX(tricornCamera),
                                 mw::CameraCentreY(tricornCamera)};
    const std::complex<double> expectedSecond = std::conj(c) * std::conj(c) + c;
    Check(std::abs(expansionValue(independentTricorn.points.at(2)) - expectedSecond) < 1.0e-12,
          "The independent backend should preserve the Tricorn conjugation recurrence.");

    auto unsupported = mw::EquationSettings{};
    unsupported.power = 3;
    bool unsupportedRejected = false;
    try {
        (void)mw::BuildIndependentHighPrecisionReferenceOrbit(camera, unsupported, 64);
    } catch (const std::invalid_argument&) {
        unsupportedRejected = true;
    }
    Check(unsupportedRejected,
          "The independent backend must fail closed for an unregistered perturbation profile.");

    auto nonFiniteCamera = camera;
    nonFiniteCamera.centreXLow = std::numeric_limits<double>::quiet_NaN();
    bool nonFiniteRejected = false;
    try {
        (void)mw::BuildIndependentHighPrecisionReferenceOrbit(
            nonFiniteCamera, mw::EquationSettings{}, 64);
    } catch (const std::invalid_argument&) {
        nonFiniteRejected = true;
    }
    Check(nonFiniteRejected,
          "The independent backend must fail closed for non-finite camera input.");
}

void TestExactDecimalAndCameraAdapter() {
    mw::ExactDecimal decimal;
    std::string error;
    Check(mw::ExactDecimal::Parse("+001.2300E+02", decimal, error) &&
              decimal.CanonicalText() == "1.23e2",
          "Exact decimals should canonicalise sign, zeroes and exponent spelling.");
    Check(mw::ExactDecimal::Parse("-.00500", decimal, error) &&
              decimal.CanonicalText() == "-5e-3",
          "Exact decimals should canonicalise fractional input without passing through double.");
    Check(mw::ExactDecimal::Parse("-0.000e999999", decimal, error) && decimal.IsZero() &&
              decimal.CanonicalText() == "0",
          "All zero exact decimal spellings should canonicalise to one value.");
    Check(!mw::ExactDecimal::Parse(" 1", decimal, error) &&
              !mw::ExactDecimal::Parse("NaN", decimal, error) &&
              !mw::ExactDecimal::Parse("1_000", decimal, error) &&
              !mw::ExactDecimal::Parse("1e1000001", decimal, error),
          "Exact decimals should reject whitespace, non-finite text, separators and excessive exponents.");
    const std::string tooManyDigits(mw::ExactDecimal::kMaximumSignificantDigits + 1U, '1');
    Check(!mw::ExactDecimal::Parse(tooManyDigits, decimal, error),
          "Exact decimals should enforce the server-class significant-digit resource bound.");
    mw::ExactDecimal addend;
    mw::ExactDecimal sum;
    Check(mw::ExactDecimal::Parse("1.2", decimal, error) &&
              mw::ExactDecimal::Parse("0.003", addend, error) &&
              mw::ExactDecimal::Add(decimal, addend, sum, error) &&
              sum.CanonicalText() == "1.203e0",
          "Exact decimal addition should reconstruct compensated values without double rounding.");
    Check(mw::ExactDecimal::Parse("1.25", decimal, error) &&
              mw::ExactDecimal::Parse("-1.25", addend, error) &&
              mw::ExactDecimal::Add(decimal, addend, sum, error) && sum.IsZero(),
          "Exact decimal addition should canonicalise cancellation to zero.");

    mw::ExactCamera exact;
    Check(mw::ExactDecimal::Parse("-0.5", exact.centreX, error) &&
              mw::ExactDecimal::Parse("0", exact.centreY, error) &&
              mw::ExactDecimal::Parse("1.5", exact.halfHeight, error),
          "Exact camera construction should accept canonical finite values.");
    mw::LegacyCameraAdaptation legacy;
    Check(mw::AdaptExactCameraToLegacy(exact, legacy, error) &&
              legacy.camera.centreX == -0.5 && legacy.camera.centreY == 0.0 &&
              legacy.camera.scale == 1.5 && !legacy.centreXLoss &&
              !legacy.centreYLoss && !legacy.halfHeightLoss,
          "The exact-to-legacy adapter should preserve exactly representable camera values.");
    Check(mw::ExactDecimal::Parse("1e-1000", exact.halfHeight, error) &&
              mw::AdaptExactCameraToLegacy(exact, legacy, error) && legacy.halfHeightLoss &&
              legacy.camera.scale == std::numeric_limits<double>::denorm_min(),
          "The exact-to-legacy adapter should preserve ultra-deep authority through explicit positive underflow loss.");

    Check(mw::ExactDecimal::Parse("1.0000000000000000000000000000000000001", exact.centreX, error) &&
              mw::AdaptExactCameraToLegacy(exact, legacy, error) && legacy.centreXLoss,
          "The one-way legacy adapter should explicitly report precision loss.");
    Check(mw::ExactDecimal::Parse("0", exact.halfHeight, error) &&
              !mw::AdaptExactCameraToLegacy(exact, legacy, error),
          "The legacy adapter should reject a non-positive exact half-height.");
    Check(mw::ExactDecimal::Parse("1e1000000", exact.halfHeight, error) &&
              !mw::AdaptExactCameraToLegacy(exact, legacy, error),
          "The legacy adapter should fail closed when exact camera values are not finite doubles.");

    Check(mw::ExactDecimal::Parse("-7.4364388703715100000000000001e-1", exact.centreX, error) &&
              mw::ExactDecimal::Parse("1.3182590420533000000000000001e-1", exact.centreY, error) &&
              mw::ExactDecimal::Parse("6.5e-3", exact.halfHeight, error),
          "Precision planner fixture should restore valid exact camera text.");
    const auto plan = mw::BuildPrecisionPlan(exact, mw::EquationSettings{},
                                              mw::PrecisionMode::Automatic, {});
    Check(plan.backend == mw::PrecisionExecutionBackend::CpuBoost512Reference &&
              plan.requiredBits > 53 && plan.selectedBits == 512,
          "The planner should choose an equal-or-higher 512-bit route for deep exact text.");
    mw::PrecisionBackendCapabilities unvalidatedGpuCapabilities;
    unvalidatedGpuCapabilities.gpuOrbitFloat4 = true;
    unvalidatedGpuCapabilities.gpuOrbitFloat4ValidatedBits = 0;
    const auto unvalidatedGpuPlan = mw::BuildPrecisionPlan(
        exact, mw::EquationSettings{}, mw::PrecisionMode::Automatic, unvalidatedGpuCapabilities);
    Check(unvalidatedGpuPlan.backend == mw::PrecisionExecutionBackend::CpuBoost512Reference &&
              unvalidatedGpuPlan.requiresDirectCorrection &&
              unvalidatedGpuPlan.reason.find("no validated precision envelope") != std::string::npos,
          "An unvalidated float4 GPU orbit transport must require direct correction rather than imply safe deep execution.");
    exact.halfHeight = mw::ExactDecimal{};
    Check(mw::ExactDecimal::Parse("1.5", exact.halfHeight, error),
          "Explicit-precision planner fixture should restore an ordinary exact scale.");
    const auto forcedReference = mw::BuildPrecisionPlan(
        exact, mw::EquationSettings{}, mw::PrecisionMode::ArbitraryPrecisionPerturbation, {});
    Check(forcedReference.backend == mw::PrecisionExecutionBackend::CpuBoost512Reference,
          "Explicit arbitrary-precision intent must not silently downgrade to Float64.");
    const auto splitRefused = mw::BuildPrecisionPlan(
        exact, mw::EquationSettings{}, mw::PrecisionMode::SplitFloat, {});
    Check(splitRefused.backend == mw::PrecisionExecutionBackend::Refused &&
              splitRefused.reason.find("Split-float") != std::string::npos,
          "Explicit split-float intent must refuse until a validated planner backend exists.");
    mw::ExactDecimal veryDeep;
    Check(mw::ExactDecimal::Parse("1." + std::string(200U, '1') + "e-1", veryDeep, error),
          "The planner refusal fixture should parse valid deep camera text.");
    exact.halfHeight = veryDeep;
    const auto refused = mw::BuildPrecisionPlan(exact, mw::EquationSettings{},
                                                 mw::PrecisionMode::Automatic, {});
    Check(refused.backend == mw::PrecisionExecutionBackend::Refused &&
              refused.reason.find("without rounding") != std::string::npos,
          "The planner must refuse depth beyond configured backends rather than silently downshifting.");
    Check(mw::ExactDecimal::Parse("1e-200", exact.halfHeight, error),
          "The exponent-depth planner fixture should parse valid exact source text.");
    const auto exponentRefused = mw::BuildPrecisionPlan(exact, mw::EquationSettings{},
                                                         mw::PrecisionMode::Automatic, {});
    Check(exponentRefused.backend == mw::PrecisionExecutionBackend::Refused &&
              exponentRefused.requiredBits > 512,
          "The planner must account for exact half-height exponent depth, not only significand digits.");
    mw::PrecisionBackendCapabilities boost2048Capabilities;
    boost2048Capabilities.cpuBoost512Reference = false;
    boost2048Capabilities.cpuBoost2048Direct = true;
    const auto boost2048Plan = mw::BuildPrecisionPlan(
        exact, mw::EquationSettings{}, mw::PrecisionMode::Automatic, boost2048Capabilities);
    Check(boost2048Plan.backend == mw::PrecisionExecutionBackend::CpuBoost2048Direct &&
              boost2048Plan.selectedBits == 2048 && boost2048Plan.requiredBits > 512,
          "The planner must select the 2048-bit direct CPU tier for a camera beyond 512 bits.");
    Check(mw::ExactDecimal::Parse("1e-1000", exact.halfHeight, error),
          "The 8192-bit planner fixture should parse near-unlimited exact camera text.");
    mw::PrecisionBackendCapabilities boost8192Capabilities;
    boost8192Capabilities.cpuBoost512Reference = false;
    boost8192Capabilities.cpuBoost8192Direct = true;
    const auto boost8192Plan = mw::BuildPrecisionPlan(
        exact, mw::EquationSettings{}, mw::PrecisionMode::Automatic, boost8192Capabilities);
    Check(boost8192Plan.backend == mw::PrecisionExecutionBackend::CpuBoost8192Direct &&
              boost8192Plan.selectedBits == 8192 && boost8192Plan.requiredBits > 2048,
          "The planner must select the 8192-bit direct CPU tier for a camera beyond 2048 bits.");
    Check(mw::ExactDecimal::Parse("1e-3000", exact.halfHeight, error),
          "The 16384-bit planner fixture should parse a deeper exact camera scale.");
    mw::PrecisionBackendCapabilities boost16384Capabilities;
    boost16384Capabilities.cpuBoost512Reference = false;
    boost16384Capabilities.cpuBoost16384Direct = true;
    const auto boost16384Plan = mw::BuildPrecisionPlan(
        exact, mw::EquationSettings{}, mw::PrecisionMode::Automatic, boost16384Capabilities);
    Check(boost16384Plan.backend == mw::PrecisionExecutionBackend::CpuBoost16384Direct &&
              boost16384Plan.selectedBits == 16384 && boost16384Plan.requiredBits > 8192,
          "The planner must select the 16384-bit direct CPU tier for a camera beyond 8192 bits.");

    mw::CameraState legacyGpuCamera{-0.743643887037151, 0.131825904205330, 1.0e-7};
    mw::PrecisionSettings legacyGpuSettings;
    mw::LegacyGpuPrecisionCapabilities legacyGpuCapabilities;
    legacyGpuCapabilities.splitFloat = true;
    legacyGpuCapabilities.perturbation = true;
    legacyGpuCapabilities.arbitraryReference = true;
    mw::PrecisionMode legacyGpuMode = mw::PrecisionMode::Automatic;
    legacyGpuCamera.scale = 1.5;
    Check(mw::ResolveLegacyGpuPrecision(legacyGpuCamera, mw::EquationSettings{},
                                        legacyGpuSettings, legacyGpuCapabilities,
                                        legacyGpuMode, error) &&
              legacyGpuMode == mw::PrecisionMode::Float32,
          "The central legacy GPU policy should begin an automatic preview in float32.");
    legacyGpuCamera.scale = 1.0e-7;
    Check(mw::ResolveLegacyGpuPrecision(legacyGpuCamera, mw::EquationSettings{},
                                        legacyGpuSettings, legacyGpuCapabilities,
                                        legacyGpuMode, error) &&
              legacyGpuMode == mw::PrecisionMode::SplitFloat,
          "The central legacy GPU policy should choose split float when Float64 is unavailable at intermediate zoom.");
    legacyGpuCapabilities.nativeFloat64 = true;
    Check(mw::ResolveLegacyGpuPrecision(legacyGpuCamera, mw::EquationSettings{},
                                        legacyGpuSettings, legacyGpuCapabilities,
                                        legacyGpuMode, error) &&
              legacyGpuMode == mw::PrecisionMode::Float64,
          "The central legacy GPU policy should prefer reported Float64 capability at the compatible zoom range.");
    legacyGpuCamera.scale = 1.0e-14;
    legacyGpuCapabilities.nativeFloat64 = false;
    Check(mw::ResolveLegacyGpuPrecision(legacyGpuCamera, mw::EquationSettings{},
                                        legacyGpuSettings, legacyGpuCapabilities,
                                        legacyGpuMode, error) &&
              legacyGpuMode == mw::PrecisionMode::ArbitraryPrecisionPerturbation,
          "The central legacy GPU policy should select the reported arbitrary-reference path beyond split-float range.");
    auto unsupportedGpuEquation = mw::EquationSettings{};
    unsupportedGpuEquation.power = 3;
    legacyGpuCapabilities.nativeFloat64 = true;
    Check(mw::ResolveLegacyGpuPrecision(legacyGpuCamera, unsupportedGpuEquation,
                                        legacyGpuSettings, legacyGpuCapabilities,
                                        legacyGpuMode, error) &&
              legacyGpuMode == mw::PrecisionMode::Float64,
          "The central legacy GPU policy should keep unsupported formulas out of perturbation while using available Float64.");
    legacyGpuSettings.mode = mw::PrecisionMode::Perturbation;
    legacyGpuSettings.automaticFallback = false;
    Check(!mw::ResolveLegacyGpuPrecision(legacyGpuCamera, unsupportedGpuEquation,
                                         legacyGpuSettings, legacyGpuCapabilities,
                                         legacyGpuMode, error) &&
              error.find("unavailable") != std::string::npos,
          "The central legacy GPU policy should refuse explicitly requested incompatible perturbation.");

    mw::CameraState compensated{-0.5, 0.0, 1.5, 2.5e-20, -1.25e-20};
    Check(mw::BuildExactCameraFromLegacy(compensated, exact, error) &&
              exact.centreX.CanonicalText() != "-5e-1" &&
              exact.centreY.CanonicalText() != "0" &&
              exact.halfHeight.CanonicalText() == "1.5e0",
          "Legacy migration should reconstruct compensated centre sums without binary rounding.");
}

void TestJourneyScriptValidationContract() {
    Check(!mw::AnimationController::HasValidJourneyScriptTargets(""),
          "An empty journey script should not contain valid targets.");
    Check(mw::AnimationController::HasValidJourneyScriptTargets(
              "-0.743643887037151,0.131825904205330,0.004,12,1"),
          "A valid structured journey row should be accepted through the public validation contract.");
    Check(!mw::AnimationController::HasValidJourneyScriptTargets(
              "not-a-coordinate,row"),
          "Malformed journey rows should be rejected through the public validation contract.");
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
    TestProjectStateAdaptersAndFingerprint();
    TestProjectParameterMutationCoordinator();
    TestProjectHistory();
    TestModelessEditorCandidateMerges();
    TestKnownPoints();
    TestPresetValidation();
    TestSettingsRoundTrip();
    TestPresetImportSecurity();
    TestCustomEquations();
    TestConjugateDistanceEstimation();
    TestBuiltInEquationAndPaletteLibraries();
    TestGeneralAnimationEvaluator();
    TestGeneralAnimationEditorAndJourneyAdapter();
    TestFrameSequenceExport();
    TestExternalVideoExport();
    TestAnimation();
    TestSafeZoomTarget();
    TestDeepZoomMath();
    TestIndependentHighPrecisionBackend();
    TestExactDecimalAndCameraAdapter();
    TestStaticSlideshowValidation();
    TestAdaptivePerformance();
    TestInvisibleFrameSuppression();
    TestStillRenderQualityAndTileCamera();
    TestTiledStillRenderer();
    TestFractalScout();
    TestJourneyScriptValidationContract();
    TestDefaults();
    if (failures == 0) {
        std::cout << "All Mandelbrot core tests passed.\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
