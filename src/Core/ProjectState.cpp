#include "Core/ProjectState.h"
#include "Core/Precision/ExactCameraAdapter.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>
#include <utility>

namespace mw {
namespace {

constexpr std::array<ParameterDescriptor, 17> kParameterDescriptors{{
    {ParameterDomain::Camera, ParameterKey::CameraCentreXHigh, "camera.centre-x-high",
     ParameterValueType::Real, ParameterInvalidation::CameraViewport, true, true},
    {ParameterDomain::Camera, ParameterKey::CameraCentreXLow, "camera.centre-x-low",
     ParameterValueType::Real, ParameterInvalidation::CameraViewport, true, true},
    {ParameterDomain::Camera, ParameterKey::CameraCentreYHigh, "camera.centre-y-high",
     ParameterValueType::Real, ParameterInvalidation::CameraViewport, true, true},
    {ParameterDomain::Camera, ParameterKey::CameraCentreYLow, "camera.centre-y-low",
     ParameterValueType::Real, ParameterInvalidation::CameraViewport, true, true},
    {ParameterDomain::Camera, ParameterKey::CameraScale, "camera.scale",
     ParameterValueType::Real, ParameterInvalidation::CameraViewport, true, true},
    {ParameterDomain::Camera, ParameterKey::RotationDegrees, "camera.rotation-degrees",
     ParameterValueType::Real, ParameterInvalidation::CameraViewport, true, true},
    {ParameterDomain::Palette, ParameterKey::PaletteSelection, "palette.selection",
     ParameterValueType::Integer, ParameterInvalidation::PaletteColouring, false, true},
    {ParameterDomain::Palette, ParameterKey::PaletteOffset, "palette.offset",
     ParameterValueType::Real, ParameterInvalidation::PaletteColouring, true, true},
    {ParameterDomain::Palette, ParameterKey::PaletteFrequency, "palette.frequency",
     ParameterValueType::Real, ParameterInvalidation::PaletteColouring, true, true},
    {ParameterDomain::Palette, ParameterKey::PaletteGamma, "palette.gamma",
     ParameterValueType::Real, ParameterInvalidation::PaletteColouring, true, true},
    {ParameterDomain::PostProcessing, ParameterKey::Brightness, "post.brightness",
     ParameterValueType::Real, ParameterInvalidation::PostProcessing, true, true},
    {ParameterDomain::PostProcessing, ParameterKey::Contrast, "post.contrast",
     ParameterValueType::Real, ParameterInvalidation::PostProcessing, true, true},
    {ParameterDomain::PostProcessing, ParameterKey::Saturation, "post.saturation",
     ParameterValueType::Real, ParameterInvalidation::PostProcessing, true, true},
    {ParameterDomain::Palette, ParameterKey::StripeStrength, "palette.stripe-strength",
     ParameterValueType::Real, ParameterInvalidation::PaletteColouring, true, true},
    {ParameterDomain::PostProcessing, ParameterKey::BloomStrength, "post.bloom-strength",
     ParameterValueType::Real, ParameterInvalidation::PostProcessing, true, true},
    {ParameterDomain::PostProcessing, ParameterKey::BloomRadius, "post.bloom-radius",
     ParameterValueType::Integer, ParameterInvalidation::PostProcessing, true, true},
    {ParameterDomain::PostProcessing, ParameterKey::EdgeLightingStrength,
     "post.edge-lighting-strength", ParameterValueType::Real,
     ParameterInvalidation::PaletteColouring, true, true},
}};

class CanonicalWriter {
public:
    explicit CanonicalWriter(std::string_view version) { output_ = std::string(version) + "\n"; }

    void AddBool(std::string_view name, bool value) {
        AddName(name);
        output_ += value ? "b:1\n" : "b:0\n";
    }

    void AddInt(std::string_view name, std::int64_t value) {
        AddName(name);
        output_ += "i:";
        AppendInteger(value);
        output_ += '\n';
    }

    void AddUInt(std::string_view name, std::uint64_t value) {
        AddName(name);
        output_ += "u:";
        AppendInteger(value);
        output_ += '\n';
    }

    bool AddDouble(std::string_view name, double value, std::string& error) {
        if (!std::isfinite(value)) {
            error = "Render fingerprint field is not finite: " + std::string(name);
            return false;
        }
        if (value == 0.0) value = 0.0; // Canonicalise negative zero.
        AddName(name);
        output_ += "f64:";
        AppendHex(std::bit_cast<std::uint64_t>(value), 16U);
        output_ += '\n';
        return true;
    }

    bool AddFloat(std::string_view name, float value, std::string& error) {
        if (!std::isfinite(value)) {
            error = "Render fingerprint field is not finite: " + std::string(name);
            return false;
        }
        if (value == 0.0F) value = 0.0F;
        AddName(name);
        output_ += "f32:";
        AppendHex(static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(value)), 8U);
        output_ += '\n';
        return true;
    }

    void AddString(std::string_view name, std::string_view value) {
        AddName(name);
        output_ += "s:";
        AppendInteger(static_cast<std::uint64_t>(value.size()));
        output_ += ':';
        output_.append(value.data(), value.size());
        output_ += '\n';
    }

    [[nodiscard]] std::string Take() { return std::move(output_); }

private:
    template <typename Integer>
    void AppendInteger(Integer value) {
        std::array<char, 32> buffer{};
        const auto conversion = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
        if (conversion.ec == std::errc{}) {
            output_.append(buffer.data(), static_cast<std::size_t>(conversion.ptr - buffer.data()));
        }
    }

    void AddName(std::string_view name) {
        output_.append(name.data(), name.size());
        output_ += '=';
    }

    void AppendHex(std::uint64_t value, unsigned digits) {
        constexpr std::string_view hex = "0123456789abcdef";
        for (unsigned digit = digits; digit > 0U; --digit) {
            const unsigned shift = (digit - 1U) * 4U;
            const auto nibble = static_cast<std::size_t>((value >> shift) & 0x0FULL);
            output_ += hex[nibble];
        }
    }

    std::string output_;
};

bool AddCoefficient(CanonicalWriter& writer, std::string_view prefix,
                    const ComplexCoefficient& value, std::string& error) {
    return writer.AddDouble(std::string(prefix) + ".real", value.real, error) &&
           writer.AddDouble(std::string(prefix) + ".imaginary", value.imaginary, error);
}

bool AddColour(CanonicalWriter& writer, std::string_view prefix,
               const Colour& value, std::string& error) {
    return writer.AddFloat(std::string(prefix) + ".r", value.r, error) &&
           writer.AddFloat(std::string(prefix) + ".g", value.g, error) &&
           writer.AddFloat(std::string(prefix) + ".b", value.b, error) &&
           writer.AddFloat(std::string(prefix) + ".a", value.a, error);
}

template <typename Enum>
bool AddEnum(CanonicalWriter& writer, std::string_view name, Enum value,
             std::optional<Enum> (*parse)(const std::string&),
             std::string& error) {
    const std::string stableValue = ToString(value);
    const auto roundTrip = parse(stableValue);
    if (!roundTrip || *roundTrip != value) {
        error = "Render fingerprint enum is unsupported: " + std::string(name);
        return false;
    }
    writer.AddString(name, stableValue);
    return true;
}

const ParameterDescriptor* DescriptorForKey(ParameterKey key) noexcept {
    for (const auto& descriptor : kParameterDescriptors) {
        if (descriptor.key == key) return &descriptor;
    }
    return nullptr;
}

bool ReadParameterValue(const Preset& preset, ParameterKey key,
                        ParameterValue& value, std::string& error) {
    switch (key) {
    case ParameterKey::CameraCentreXHigh: value = preset.camera.centreX; return true;
    case ParameterKey::CameraCentreXLow: value = preset.camera.centreXLow; return true;
    case ParameterKey::CameraCentreYHigh: value = preset.camera.centreY; return true;
    case ParameterKey::CameraCentreYLow: value = preset.camera.centreYLow; return true;
    case ParameterKey::CameraScale: value = preset.camera.scale; return true;
    case ParameterKey::RotationDegrees: value = preset.rotationDegrees; return true;
    case ParameterKey::PaletteSelection:
        value = static_cast<std::int64_t>(preset.palette);
        return true;
    case ParameterKey::PaletteOffset: value = preset.colourOffset; return true;
    case ParameterKey::PaletteFrequency: value = preset.paletteFrequency; return true;
    case ParameterKey::PaletteGamma: value = preset.paletteGamma; return true;
    case ParameterKey::Brightness: value = preset.brightness; return true;
    case ParameterKey::Contrast: value = preset.contrast; return true;
    case ParameterKey::Saturation: value = preset.saturation; return true;
    case ParameterKey::StripeStrength: value = preset.equation.stripeStrength; return true;
    case ParameterKey::BloomStrength: value = preset.equation.glowStrength; return true;
    case ParameterKey::BloomRadius:
        value = static_cast<std::int64_t>(preset.equation.bloomRadius);
        return true;
    case ParameterKey::EdgeLightingStrength:
        value = preset.equation.edgeLightingStrength;
        return true;
    }
    error = "Unsupported project parameter key.";
    return false;
}

bool WriteParameterValue(Preset& preset, ParameterKey key,
                         const ParameterValue& value, std::string& error) {
    if (const auto* real = std::get_if<double>(&value)) {
        if (!std::isfinite(*real)) {
            error = "Project parameter real value must be finite: " +
                    std::string(StableParameterName(key));
            return false;
        }
        switch (key) {
        case ParameterKey::CameraCentreXHigh: preset.camera.centreX = *real; return true;
        case ParameterKey::CameraCentreXLow: preset.camera.centreXLow = *real; return true;
        case ParameterKey::CameraCentreYHigh: preset.camera.centreY = *real; return true;
        case ParameterKey::CameraCentreYLow: preset.camera.centreYLow = *real; return true;
        case ParameterKey::CameraScale: preset.camera.scale = *real; return true;
        case ParameterKey::RotationDegrees: preset.rotationDegrees = *real; return true;
        case ParameterKey::PaletteSelection:
            break;
        case ParameterKey::PaletteOffset: preset.colourOffset = *real; return true;
        case ParameterKey::PaletteFrequency: preset.paletteFrequency = *real; return true;
        case ParameterKey::PaletteGamma: preset.paletteGamma = *real; return true;
        case ParameterKey::Brightness: preset.brightness = *real; return true;
        case ParameterKey::Contrast: preset.contrast = *real; return true;
        case ParameterKey::Saturation: preset.saturation = *real; return true;
        case ParameterKey::StripeStrength: preset.equation.stripeStrength = *real; return true;
        case ParameterKey::BloomStrength: preset.equation.glowStrength = *real; return true;
        case ParameterKey::EdgeLightingStrength:
            preset.equation.edgeLightingStrength = *real;
            return true;
        case ParameterKey::BloomRadius:
            break;
        }
    } else if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        if (key == ParameterKey::PaletteSelection) {
            const auto first = static_cast<std::int64_t>(Palette::ClassicSpectrum);
            const auto last = static_cast<std::int64_t>(Palette::HighContrast);
            if (*integer < first || *integer > last) {
                error = "Project palette selection is unsupported: " +
                        std::to_string(*integer);
                return false;
            }
            preset.palette = static_cast<Palette>(*integer);
            return true;
        }
        if (key == ParameterKey::BloomRadius) {
            if (*integer < static_cast<std::int64_t>(std::numeric_limits<int>::min()) ||
                *integer > static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
                error = "Project parameter integer value exceeds the platform int range: " +
                        std::string(StableParameterName(key));
                return false;
            }
            preset.equation.bloomRadius = static_cast<int>(*integer);
            return true;
        }
    }
    error = "Project parameter value type does not match its descriptor: " +
            std::string(StableParameterName(key));
    return false;
}

constexpr std::array<std::uint32_t, 64> kSha256RoundConstants{{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U,
    0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U,
    0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
    0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU,
    0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
}};

constexpr std::uint32_t RotateRight(std::uint32_t value, unsigned count) noexcept {
    return (value >> count) | (value << (32U - count));
}

} // namespace

std::span<const ParameterDescriptor> ProjectParameterDescriptors() noexcept {
    return kParameterDescriptors;
}

std::optional<ParameterKey> ParameterKeyFromStableName(std::string_view stableName) noexcept {
    for (const auto& descriptor : kParameterDescriptors) {
        if (descriptor.stableName == stableName) return descriptor.key;
    }
    return std::nullopt;
}

std::string_view StableParameterName(ParameterKey key) noexcept {
    for (const auto& descriptor : kParameterDescriptors) {
        if (descriptor.key == key) return descriptor.stableName;
    }
    return {};
}

bool ReadProjectParameterValue(const Preset& preset, ParameterKey key,
                               ParameterValue& value, std::string& error) {
    error.clear();
    return ReadParameterValue(preset, key, value, error);
}

std::uint64_t ParameterGestureCoalescer::NextToken() noexcept {
    const std::uint64_t token = nextToken_++;
    if (nextToken_ == 0U) nextToken_ = 1U;
    return token == 0U ? nextToken_++ : token;
}

std::uint64_t ParameterGestureCoalescer::Begin(ParameterGestureKind gesture) noexcept {
    if (gesture == ParameterGestureKind::None) {
        Reset();
        return 0U;
    }
    activeGesture_ = gesture;
    activeToken_ = NextToken();
    lastEventTimestampMilliseconds_ = 0U;
    return activeToken_;
}

std::uint64_t ParameterGestureCoalescer::ContinueOrBegin(
    ParameterGestureKind gesture,
    std::uint64_t eventTimestampMilliseconds,
    std::uint64_t maximumGapMilliseconds) noexcept {
    const bool canContinue =
        gesture != ParameterGestureKind::None && activeGesture_ == gesture &&
        activeToken_ != 0U && eventTimestampMilliseconds >= lastEventTimestampMilliseconds_ &&
        eventTimestampMilliseconds - lastEventTimestampMilliseconds_ <= maximumGapMilliseconds;
    if (!canContinue) (void)Begin(gesture);
    lastEventTimestampMilliseconds_ = eventTimestampMilliseconds;
    return activeToken_;
}

void ParameterGestureCoalescer::End(ParameterGestureKind gesture) noexcept {
    if (activeGesture_ == gesture) Reset();
}

void ParameterGestureCoalescer::Reset() noexcept {
    activeGesture_ = ParameterGestureKind::None;
    activeToken_ = 0U;
    lastEventTimestampMilliseconds_ = 0U;
}

std::uint64_t ParameterGestureCoalescer::ActiveToken(
    ParameterGestureKind gesture) const noexcept {
    return activeGesture_ == gesture ? activeToken_ : 0U;
}

namespace {

bool ReplacementContextMatches(ProjectPresetReplacementContext context) noexcept {
    switch (context.kind) {
    case ProjectPresetReplacementKind::PresetLoad:
        return context.origin == ParameterMutationOrigin::PresetLoad;
    case ProjectPresetReplacementKind::ImportedPreset:
        return context.origin == ParameterMutationOrigin::Import;
    case ProjectPresetReplacementKind::DirectProjectEdit:
    case ProjectPresetReplacementKind::PaletteDialog:
    case ProjectPresetReplacementKind::EquationDialog:
    case ProjectPresetReplacementKind::SettingsDialog:
    case ProjectPresetReplacementKind::JourneyDialog:
        return context.origin == ParameterMutationOrigin::UserControl;
    }
    return false;
}

} // namespace

bool ApplyProjectParameterMutations(Preset& authoritativePreset,
                                    std::span<const ParameterMutation> mutations,
                                    ParameterMutationResult& result,
                                    std::string& error,
                                    ParameterMutationContext context) {
    result = {};
    result.origin = context.origin;
    result.gestureKind = context.gestureKind;
    result.coalescingToken = context.coalescingToken;
    error.clear();
    const bool hasGestureKind = context.gestureKind != ParameterGestureKind::None;
    const bool hasCoalescingToken = context.coalescingToken != 0U;
    if (hasGestureKind != hasCoalescingToken) {
        error = "Gesture mutation metadata requires both a gesture kind and a non-zero coalescing token.";
        return false;
    }
    if (hasGestureKind && context.origin != ParameterMutationOrigin::UserGesture) {
        error = "Gesture coalescing metadata is only valid for user-gesture mutations.";
        return false;
    }
    if (mutations.empty()) return true;

    std::vector<ParameterKey> seenKeys;
    seenKeys.reserve(mutations.size());
    Preset candidate = authoritativePreset;
    for (const auto& mutation : mutations) {
        const ParameterDescriptor* descriptor = DescriptorForKey(mutation.key);
        if (!descriptor) {
            error = "Unsupported project parameter key.";
            return false;
        }
        if (std::find(seenKeys.begin(), seenKeys.end(), mutation.key) != seenKeys.end()) {
            error = "Duplicate project parameter in one mutation batch: " +
                    std::string(descriptor->stableName);
            return false;
        }
        seenKeys.push_back(mutation.key);
        const bool typeMatches =
            (descriptor->valueType == ParameterValueType::Real &&
             std::holds_alternative<double>(mutation.value)) ||
            (descriptor->valueType == ParameterValueType::Integer &&
             std::holds_alternative<std::int64_t>(mutation.value));
        if (!typeMatches || !WriteParameterValue(candidate, mutation.key, mutation.value, error)) {
            if (error.empty()) {
                error = "Project parameter value type does not match its descriptor: " +
                        std::string(descriptor->stableName);
            }
            return false;
        }
    }

    Preset normalised = candidate;
    (void)ValidateAndNormalise(normalised);
    Preset committed = authoritativePreset;
    for (const auto& mutation : mutations) {
        ParameterValue normalisedValue;
        ParameterValue originalValue;
        if (!ReadParameterValue(normalised, mutation.key, normalisedValue, error) ||
            !ReadParameterValue(authoritativePreset, mutation.key, originalValue, error)) {
            return false;
        }
        if (normalisedValue != mutation.value) result.normalised = true;
        if (normalisedValue == originalValue) continue;
        if (!WriteParameterValue(committed, mutation.key, normalisedValue, error)) return false;
        const ParameterDescriptor* descriptor = DescriptorForKey(mutation.key);
        result.changedKeys.push_back(mutation.key);
        result.invalidationMask = static_cast<ParameterInvalidationMask>(
            result.invalidationMask | ParameterInvalidationBit(descriptor->invalidation));
    }

    result.changed = !result.changedKeys.empty();
    if (result.changed) {
        const bool cameraChanged = std::any_of(
            result.changedKeys.begin(), result.changedKeys.end(), [](ParameterKey key) {
                return key == ParameterKey::CameraCentreXHigh ||
                       key == ParameterKey::CameraCentreXLow ||
                       key == ParameterKey::CameraCentreYHigh ||
                       key == ParameterKey::CameraCentreYLow ||
                       key == ParameterKey::CameraScale;
            });
        if (cameraChanged) {
            ExactCamera exact;
            if (!BuildExactCameraFromLegacy(committed.camera, exact, error)) return false;
            committed.exactCamera = std::move(exact);
        }
        result.historyEligible = IsParameterMutationOriginHistoryEligible(result.origin);
        for (const ParameterKey key : result.changedKeys) {
            const ParameterDescriptor* descriptor = DescriptorForKey(key);
            result.historyEligible = result.historyEligible && descriptor && descriptor->historyEligible;
        }
        result.coalescingEligible = result.historyEligible && hasGestureKind;
        authoritativePreset = std::move(committed);
    }
    return true;
}

bool ApplyProjectExactCameraMutation(Preset& authoritativePreset,
                                     const ExactCamera& exactCamera,
                                     ParameterMutationResult& result,
                                     std::string& error,
                                     ParameterMutationContext context) {
    result = {};
    result.origin = context.origin;
    result.gestureKind = context.gestureKind;
    result.coalescingToken = context.coalescingToken;
    error.clear();
    const bool hasGestureKind = context.gestureKind != ParameterGestureKind::None;
    const bool hasCoalescingToken = context.coalescingToken != 0U;
    if (hasGestureKind != hasCoalescingToken ||
        (hasGestureKind && context.origin != ParameterMutationOrigin::UserGesture)) {
        error = "Exact-camera mutation gesture metadata is invalid.";
        return false;
    }
    if (exactCamera.halfHeight.IsZero()) {
        error = "Exact-camera half-height must be positive.";
        return false;
    }
    LegacyCameraAdaptation adaptation;
    if (!AdaptExactCameraToLegacy(exactCamera, adaptation, error) ||
        adaptation.camera.scale <= 0.0) {
        if (error.empty()) error = "Exact-camera half-height must adapt to a positive legacy scale.";
        return false;
    }
    Preset candidate = authoritativePreset;
    candidate.exactCamera = exactCamera;
    candidate.camera = adaptation.camera;
    const ValidationResult validation = ValidateAndNormalise(candidate);
    if (!validation.valid) {
        error = "Exact-camera mutation violates the current preset camera bounds.";
        return false;
    }
    result.normalised = candidate.camera != adaptation.camera;
    result.changed = candidate != authoritativePreset;
    if (!result.changed) return true;
    result.changedKeys = {
        ParameterKey::CameraCentreXHigh,
        ParameterKey::CameraCentreXLow,
        ParameterKey::CameraCentreYHigh,
        ParameterKey::CameraCentreYLow,
        ParameterKey::CameraScale,
    };
    result.invalidationMask = ParameterInvalidationBit(ParameterInvalidation::CameraViewport);
    result.historyEligible = IsParameterMutationOriginHistoryEligible(result.origin);
    result.coalescingEligible = result.historyEligible && hasGestureKind;
    authoritativePreset = std::move(candidate);
    return true;
}

bool ApplyProjectPaletteSelection(Preset& authoritativePreset,
                                  Palette palette,
                                  bool clearCustomPaletteOnChange,
                                  ParameterMutationResult& result,
                                  std::string& error,
                                  ParameterMutationContext context) {
    Preset candidate = authoritativePreset;
    const std::array<ParameterMutation, 1> mutations{{
        {ParameterKey::PaletteSelection, static_cast<std::int64_t>(palette)},
    }};
    if (!ApplyProjectParameterMutations(candidate, mutations, result, error, context)) {
        return false;
    }
    if (!result.changed) return true;
    if (clearCustomPaletteOnChange && !candidate.customPaletteColours.empty()) {
        candidate.customPaletteColours.clear();
        // PH-05 history detects that the scalar palette key cannot fully
        // represent this action and records one atomic project snapshot.
        result.historyEligible = IsParameterMutationOriginHistoryEligible(context.origin);
    }
    authoritativePreset = std::move(candidate);
    return true;
}

bool ApplyProjectPresetReplacement(Preset& authoritativePreset,
                                   Preset candidate,
                                   ProjectPresetReplacementResult& result,
                                   std::string& error,
                                   ProjectPresetReplacementContext context) {
    result = {};
    result.origin = context.origin;
    result.kind = context.kind;
    error.clear();
    if (!ReplacementContextMatches(context)) {
        error = "Whole-preset replacement kind does not match its mutation origin.";
        return false;
    }

    const Preset unnormalised = candidate;
    (void)ValidateAndNormalise(candidate);
    result.normalised = candidate != unnormalised;
    result.changed = candidate != authoritativePreset;
    result.historyEligible = result.changed && context.historyEligible;
    if (!result.changed) return true;

    authoritativePreset = std::move(candidate);
    result.requiresFullRender = true;
    return true;
}

Preset MergePaletteEditorCandidate(const Preset& authoritativePreset,
                                   const Preset& editorCandidate) {
    Preset merged = authoritativePreset;
    merged.customPaletteColours = editorCandidate.customPaletteColours;
    merged.paletteFrequency = editorCandidate.paletteFrequency;
    merged.paletteGamma = editorCandidate.paletteGamma;
    merged.paletteInterpolation = editorCandidate.paletteInterpolation;
    merged.equation.stripeAverageEnabled = editorCandidate.equation.stripeAverageEnabled;
    merged.equation.stripeDensity = editorCandidate.equation.stripeDensity;
    merged.equation.stripePhase = editorCandidate.equation.stripePhase;
    merged.equation.stripeStrength = editorCandidate.equation.stripeStrength;
    merged.equation.stripeStartIteration = editorCandidate.equation.stripeStartIteration;
    return merged;
}

Preset MergeEquationEditorCandidate(const Preset& authoritativePreset,
                                    const Preset& editorCandidate) {
    Preset merged = authoritativePreset;
    merged.equation = editorCandidate.equation;
    return merged;
}

CameraParameterSnapshot CaptureCameraParameters(const Preset& preset) noexcept {
    return {
        preset.camera.centreX,
        preset.camera.centreXLow,
        preset.camera.centreY,
        preset.camera.centreYLow,
        preset.camera.scale,
        preset.rotationDegrees,
    };
}

PaletteParameterSnapshot CapturePaletteParameters(const Preset& preset) {
    PaletteParameterSnapshot snapshot;
    snapshot.palette = preset.palette;
    snapshot.customPaletteColours = preset.customPaletteColours;
    snapshot.colourOffset = preset.colourOffset;
    snapshot.paletteFrequency = preset.paletteFrequency;
    snapshot.paletteGamma = preset.paletteGamma;
    snapshot.paletteInterpolation = preset.paletteInterpolation;
    snapshot.brightness = preset.brightness;
    snapshot.contrast = preset.contrast;
    snapshot.saturation = preset.saturation;
    snapshot.interiorColour = preset.interiorColour;
    snapshot.backgroundColour = preset.backgroundColour;
    snapshot.smoothColouring = preset.smoothColouring;
    snapshot.stripeStrength = preset.equation.stripeStrength;
    snapshot.bloomStrength = preset.equation.glowStrength;
    snapshot.bloomRadius = preset.equation.bloomRadius;
    snapshot.edgeLightingStrength = preset.equation.edgeLightingStrength;
    return snapshot;
}

void ApplyCameraParameters(Preset& authoritativePreset,
                           const CameraParameterSnapshot& snapshot) noexcept {
    authoritativePreset.camera.centreX = snapshot.centreX;
    authoritativePreset.camera.centreXLow = snapshot.centreXLow;
    authoritativePreset.camera.centreY = snapshot.centreY;
    authoritativePreset.camera.centreYLow = snapshot.centreYLow;
    authoritativePreset.camera.scale = snapshot.scale;
    authoritativePreset.rotationDegrees = snapshot.rotationDegrees;
    ExactCamera exact;
    std::string ignoredError;
    if (BuildExactCameraFromLegacy(authoritativePreset.camera, exact, ignoredError)) {
        authoritativePreset.exactCamera = std::move(exact);
    }
}

void ApplyPaletteParameters(Preset& authoritativePreset,
                            const PaletteParameterSnapshot& snapshot) {
    authoritativePreset.palette = snapshot.palette;
    authoritativePreset.customPaletteColours = snapshot.customPaletteColours;
    authoritativePreset.colourOffset = snapshot.colourOffset;
    authoritativePreset.paletteFrequency = snapshot.paletteFrequency;
    authoritativePreset.paletteGamma = snapshot.paletteGamma;
    authoritativePreset.paletteInterpolation = snapshot.paletteInterpolation;
    authoritativePreset.brightness = snapshot.brightness;
    authoritativePreset.contrast = snapshot.contrast;
    authoritativePreset.saturation = snapshot.saturation;
    authoritativePreset.interiorColour = snapshot.interiorColour;
    authoritativePreset.backgroundColour = snapshot.backgroundColour;
    authoritativePreset.smoothColouring = snapshot.smoothColouring;
    authoritativePreset.equation.stripeStrength = snapshot.stripeStrength;
    authoritativePreset.equation.glowStrength = snapshot.bloomStrength;
    authoritativePreset.equation.bloomRadius = snapshot.bloomRadius;
    authoritativePreset.equation.edgeLightingStrength = snapshot.edgeLightingStrength;
}

std::string Sha256Hex(std::string_view text) {
    std::vector<std::uint8_t> message;
    message.reserve(text.size() + 72U);
    for (const char character : text) {
        message.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(character)));
    }
    const std::uint64_t bitLength = static_cast<std::uint64_t>(text.size()) * 8ULL;
    message.push_back(0x80U);
    while ((message.size() % 64U) != 56U) message.push_back(0U);
    for (unsigned index = 0U; index < 8U; ++index) {
        const unsigned shift = (7U - index) * 8U;
        message.push_back(static_cast<std::uint8_t>((bitLength >> shift) & 0xFFULL));
    }

    std::array<std::uint32_t, 8> state{{
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    }};
    std::array<std::uint32_t, 64> words{};
    for (std::size_t block = 0U; block < message.size(); block += 64U) {
        for (std::size_t index = 0U; index < 16U; ++index) {
            const std::size_t offset = block + index * 4U;
            words[index] = (static_cast<std::uint32_t>(message[offset]) << 24U) |
                           (static_cast<std::uint32_t>(message[offset + 1U]) << 16U) |
                           (static_cast<std::uint32_t>(message[offset + 2U]) << 8U) |
                           static_cast<std::uint32_t>(message[offset + 3U]);
        }
        for (std::size_t index = 16U; index < words.size(); ++index) {
            const std::uint32_t s0 = RotateRight(words[index - 15U], 7U) ^
                                     RotateRight(words[index - 15U], 18U) ^
                                     (words[index - 15U] >> 3U);
            const std::uint32_t s1 = RotateRight(words[index - 2U], 17U) ^
                                     RotateRight(words[index - 2U], 19U) ^
                                     (words[index - 2U] >> 10U);
            words[index] = words[index - 16U] + s0 + words[index - 7U] + s1;
        }

        std::uint32_t a = state[0];
        std::uint32_t b = state[1];
        std::uint32_t c = state[2];
        std::uint32_t d = state[3];
        std::uint32_t e = state[4];
        std::uint32_t f = state[5];
        std::uint32_t g = state[6];
        std::uint32_t h = state[7];
        for (std::size_t index = 0U; index < words.size(); ++index) {
            const std::uint32_t sigma1 = RotateRight(e, 6U) ^ RotateRight(e, 11U) ^
                                         RotateRight(e, 25U);
            const std::uint32_t choice = (e & f) ^ ((~e) & g);
            const std::uint32_t temporary1 = h + sigma1 + choice +
                                             kSha256RoundConstants[index] + words[index];
            const std::uint32_t sigma0 = RotateRight(a, 2U) ^ RotateRight(a, 13U) ^
                                         RotateRight(a, 22U);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temporary2 = sigma0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temporary1;
            d = c;
            c = b;
            b = a;
            a = temporary1 + temporary2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    constexpr std::string_view hex = "0123456789abcdef";
    std::string digest;
    digest.reserve(64U);
    for (const std::uint32_t word : state) {
        for (unsigned digit = 8U; digit > 0U; --digit) {
            const unsigned shift = (digit - 1U) * 4U;
            digest += hex[static_cast<std::size_t>((word >> shift) & 0x0FU)];
        }
    }
    return digest;
}

bool BuildRenderFingerprintInternal(const Preset& preset,
                                    const RenderFingerprintContext& context,
                                    bool exactIdentity,
                                    RenderFingerprint& result,
                                    std::string& error) {
    result = {};
    error.clear();
    if (context.rendererId.empty() || context.rendererId.size() > 128U) {
        error = "Render fingerprint renderer identity must contain 1 to 128 bytes.";
        return false;
    }
    if (context.width == 0U || context.height == 0U) {
        error = "Render fingerprint dimensions must be positive.";
        return false;
    }
    if (preset.customPaletteColours.size() > 4096U) {
        error = "Render fingerprint custom palette exceeds 4096 colours.";
        return false;
    }

    const bool hasPrecisionPlan = !context.precisionPlanVersion.empty();
    if (hasPrecisionPlan && (context.precisionExecutionBackend.empty() ||
                             context.precisionFormulaCapability.empty() ||
                             context.precisionReason.empty() ||
                             context.precisionPlanVersion.size() > 128U ||
                             context.precisionExecutionBackend.size() > 128U ||
                             context.precisionFormulaCapability.size() > 128U ||
                             context.precisionReason.size() > 512U ||
                             context.precisionRequiredBits < 0 ||
                             context.precisionSelectedBits < 0 ||
                             context.precisionSelectedBits < context.precisionRequiredBits)) {
        error = "Render fingerprint precision-plan trace is invalid.";
        return false;
    }

    const std::string_view canonicalVersion = !exactIdentity
        ? "mw-render-state-v1"
        : (hasPrecisionPlan ? "mw-render-state-v3-exact-precision-plan"
                            : "mw-render-state-v2-exact-camera");
    CanonicalWriter writer(canonicalVersion);
    writer.AddString("context.renderer", context.rendererId);
    writer.AddUInt("context.width", context.width);
    writer.AddUInt("context.height", context.height);
    if (!AddEnum(writer, "context.precision.mode", context.precision.mode,
                 PrecisionModeFromString, error)) {
        return false;
    }
    writer.AddBool("context.precision.allow-float64", context.precision.allowFloat64);
    writer.AddBool("context.precision.allow-split-float", context.precision.allowSplitFloat);
    writer.AddBool("context.precision.allow-perturbation", context.precision.allowPerturbation);
    writer.AddBool("context.precision.allow-arbitrary", context.precision.allowArbitraryPrecision);
    writer.AddBool("context.precision.automatic-fallback", context.precision.automaticFallback);
    writer.AddInt("context.precision.arbitrary-bits", context.precision.arbitraryPrecisionBits);
    if (hasPrecisionPlan) {
        writer.AddString("context.precision.plan-version", context.precisionPlanVersion);
        writer.AddString("context.precision.execution-backend", context.precisionExecutionBackend);
        writer.AddString("context.precision.formula-capability", context.precisionFormulaCapability);
        writer.AddString("context.precision.reason", context.precisionReason);
        writer.AddInt("context.precision.required-bits", context.precisionRequiredBits);
        writer.AddInt("context.precision.selected-bits", context.precisionSelectedBits);
    }
    if (!writer.AddDouble("context.time-seconds", context.timeSeconds, error)) return false;
    writer.AddUInt("context.seed", context.seed);
    writer.AddBool("context.scale-quality-to-resolution", context.scaleQualityToResolution);

    if (exactIdentity) {
        if (!preset.exactCamera.has_value()) {
            error = "Exact render identity requires canonical exact camera state.";
            return false;
        }
        writer.AddString("camera.exact-centre-x", preset.exactCamera->centreX.CanonicalText());
        writer.AddString("camera.exact-centre-y", preset.exactCamera->centreY.CanonicalText());
        writer.AddString("camera.exact-half-height", preset.exactCamera->halfHeight.CanonicalText());
        if (!writer.AddDouble("camera.rotation-degrees", preset.rotationDegrees, error)) return false;
    } else if (!writer.AddDouble("camera.centre-x-high", preset.camera.centreX, error) ||
               !writer.AddDouble("camera.centre-x-low", preset.camera.centreXLow, error) ||
               !writer.AddDouble("camera.centre-y-high", preset.camera.centreY, error) ||
               !writer.AddDouble("camera.centre-y-low", preset.camera.centreYLow, error) ||
               !writer.AddDouble("camera.scale", preset.camera.scale, error) ||
               !writer.AddDouble("camera.rotation-degrees", preset.rotationDegrees, error)) {
        return false;
    }

    writer.AddInt("render.maximum-iterations", preset.maximumIterations);
    writer.AddInt("render.anti-aliasing-level", preset.antiAliasingLevel);
    if (!writer.AddDouble("render.scale", preset.renderScale, error)) return false;

    const EquationSettings& equation = preset.equation;
    if (!AddCoefficient(writer, "equation.quadratic", equation.quadratic, error) ||
        !AddCoefficient(writer, "equation.linear", equation.linear, error) ||
        !AddCoefficient(writer, "equation.parameter", equation.parameter, error) ||
        !AddCoefficient(writer, "equation.constant", equation.constant, error) ||
        !AddCoefficient(writer, "equation.iteration-term", equation.iterationTerm, error) ||
        !AddCoefficient(writer, "equation.reciprocal-coefficient",
                        equation.reciprocalCoefficient, error)) {
        return false;
    }
    writer.AddInt("equation.power", equation.power);
    writer.AddInt("equation.parameter-power", equation.parameterPower);
    writer.AddInt("equation.reciprocal-power", equation.reciprocalPower);
    writer.AddBool("equation.absolute-real", equation.absoluteReal);
    writer.AddBool("equation.absolute-imaginary", equation.absoluteImaginary);
    writer.AddBool("equation.conjugate", equation.conjugate);
    writer.AddBool("equation.swap-real-imaginary", equation.swapRealImaginary);
    if (!AddEnum(writer, "equation.unary-transform", equation.unaryTransform,
                 EquationUnaryTransformFromString, error) ||
        !AddEnum(writer, "equation.initial-z-mode", equation.initialZMode,
                 InitialZModeFromString, error)) {
        return false;
    }
    if (!AddCoefficient(writer, "equation.initial-z", equation.initialZ, error)) return false;
    writer.AddBool("equation.julia-mode", equation.juliaMode);
    if (!AddCoefficient(writer, "equation.julia-parameter", equation.juliaParameter, error) ||
        !writer.AddDouble("equation.bailout-radius", equation.bailoutRadius, error)) {
        return false;
    }
    if (!AddEnum(writer, "equation.render-mode", equation.renderMode,
                 FractalRenderModeFromString, error)) {
        return false;
    }
    writer.AddBool("equation.newton-compatibility-mode", equation.newtonMode);
    writer.AddInt("equation.newton-degree", equation.newtonDegree);
    if (!AddCoefficient(writer, "equation.newton-target", equation.newtonTarget, error) ||
        !AddCoefficient(writer, "equation.newton-relaxation", equation.newtonRelaxation, error) ||
        !writer.AddDouble("equation.convergence-tolerance", equation.convergenceTolerance, error)) {
        return false;
    }
    if (!AddEnum(writer, "equation.colouring-method", equation.colouringMethod,
                 ColouringMethodFromString, error) ||
        !AddEnum(writer, "equation.orbit-trap", equation.orbitTrap,
                 OrbitTrapTypeFromString, error)) {
        return false;
    }
    if (!AddCoefficient(writer, "equation.orbit-trap-point", equation.orbitTrapPoint, error) ||
        !writer.AddDouble("equation.orbit-trap-radius", equation.orbitTrapRadius, error) ||
        !writer.AddDouble("equation.bloom-strength", equation.glowStrength, error) ||
        !writer.AddDouble("equation.bloom-threshold", equation.bloomThreshold, error) ||
        !writer.AddDouble("equation.bloom-soft-knee", equation.bloomSoftKnee, error)) {
        return false;
    }
    writer.AddInt("equation.bloom-radius", equation.bloomRadius);
    if (!writer.AddDouble("equation.edge-lighting-strength", equation.edgeLightingStrength, error) ||
        !writer.AddDouble("equation.depth-strength", equation.depthStrength, error)) {
        return false;
    }
    writer.AddBool("equation.stripe-average-enabled", equation.stripeAverageEnabled);
    if (!writer.AddDouble("equation.stripe-density", equation.stripeDensity, error) ||
        !writer.AddDouble("equation.stripe-phase", equation.stripePhase, error) ||
        !writer.AddDouble("equation.stripe-strength", equation.stripeStrength, error)) {
        return false;
    }
    writer.AddInt("equation.stripe-start-iteration", equation.stripeStartIteration);
    writer.AddBool("equation.animate-coefficients", equation.animateCoefficients);
    if (!writer.AddDouble("equation.coefficient-animation-speed",
                          equation.coefficientAnimationSpeed, error) ||
        !writer.AddDouble("equation.coefficient-animation-amplitude",
                          equation.coefficientAnimationAmplitude, error)) {
        return false;
    }

    if (!AddEnum(writer, "palette.built-in", preset.palette, PaletteFromString, error)) {
        return false;
    }
    writer.AddUInt("palette.custom-count",
                   static_cast<std::uint64_t>(preset.customPaletteColours.size()));
    for (std::size_t index = 0U; index < preset.customPaletteColours.size(); ++index) {
        if (!AddColour(writer, "palette.custom." + std::to_string(index),
                       preset.customPaletteColours[index], error)) {
            return false;
        }
    }
    if (!writer.AddDouble("palette.offset", preset.colourOffset, error) ||
        !writer.AddDouble("palette.frequency", preset.paletteFrequency, error) ||
        !writer.AddDouble("palette.gamma", preset.paletteGamma, error)) {
        return false;
    }
    if (!AddEnum(writer, "palette.interpolation", preset.paletteInterpolation,
                 PaletteInterpolationFromString, error)) {
        return false;
    }
    if (!writer.AddDouble("palette.brightness", preset.brightness, error) ||
        !writer.AddDouble("palette.contrast", preset.contrast, error) ||
        !writer.AddDouble("palette.saturation", preset.saturation, error) ||
        !AddColour(writer, "palette.interior", preset.interiorColour, error) ||
        !AddColour(writer, "palette.background", preset.backgroundColour, error)) {
        return false;
    }
    writer.AddBool("palette.smooth-colouring", preset.smoothColouring);

    result.algorithm = "SHA-256";
    result.canonicalVersion = std::string(canonicalVersion);
    result.canonicalState = writer.Take();
    result.digest = Sha256Hex(result.canonicalState);
    return true;
}

bool BuildRenderFingerprint(const Preset& preset,
                            const RenderFingerprintContext& context,
                            RenderFingerprint& result,
                            std::string& error) {
    return BuildRenderFingerprintInternal(preset, context, false, result, error);
}

bool BuildExactRenderFingerprint(const Preset& preset,
                                 const RenderFingerprintContext& context,
                                 RenderFingerprint& result,
                                 std::string& error) {
    return BuildRenderFingerprintInternal(preset, context, true, result, error);
}

} // namespace mw
