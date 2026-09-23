#pragma once

#include "Core/Models.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace mw {

enum class ParameterDomain {
    Camera,
    Palette,
    PostProcessing,
};

enum class ParameterKey {
    CameraCentreXHigh,
    CameraCentreXLow,
    CameraCentreYHigh,
    CameraCentreYLow,
    CameraScale,
    RotationDegrees,
    PaletteSelection,
    PaletteOffset,
    PaletteFrequency,
    PaletteGamma,
    Brightness,
    Contrast,
    Saturation,
    StripeStrength,
    BloomStrength,
    BloomRadius,
    EdgeLightingStrength,
};

enum class ParameterValueType {
    Real,
    Integer,
};

enum class ParameterInvalidation : std::uint8_t {
    CameraViewport = 1U << 0U,
    PaletteColouring = 1U << 1U,
    PostProcessing = 1U << 2U,
    EquationPrecision = 1U << 3U,
};

using ParameterInvalidationMask = std::uint8_t;

[[nodiscard]] constexpr ParameterInvalidationMask ParameterInvalidationBit(
    ParameterInvalidation invalidation) noexcept {
    return static_cast<ParameterInvalidationMask>(invalidation);
}

[[nodiscard]] constexpr bool HasParameterInvalidation(
    ParameterInvalidationMask mask, ParameterInvalidation invalidation) noexcept {
    return (mask & ParameterInvalidationBit(invalidation)) != 0U;
}

struct ParameterDescriptor {
    ParameterDomain domain;
    ParameterKey key;
    std::string_view stableName;
    ParameterValueType valueType;
    ParameterInvalidation invalidation;
    bool interpolationSupported;
    bool historyEligible;
};

[[nodiscard]] std::span<const ParameterDescriptor> ProjectParameterDescriptors() noexcept;
[[nodiscard]] std::optional<ParameterKey> ParameterKeyFromStableName(
    std::string_view stableName) noexcept;
[[nodiscard]] std::string_view StableParameterName(ParameterKey key) noexcept;

using ParameterValue = std::variant<double, std::int64_t>;

// Reads one registered parameter without exposing a second mutable authority.
bool ReadProjectParameterValue(const Preset& preset, ParameterKey key,
                               ParameterValue& value, std::string& error);

enum class ParameterMutationOrigin {
    UserControl,
    UserGesture,
    PresetLoad,
    Import,
    ScoutApply,
    UndoRedo,
    AnimationEvaluation,
    ExportEvaluation,
    Migration,
    SystemRuntime,
};

enum class ParameterGestureKind {
    None,
    PreviewNavigation,
    PreviewPan,
    PreviewWheelZoom,
    PaletteControl,
};

enum class ProjectPresetReplacementKind {
    PresetLoad,
    ImportedPreset,
    DirectProjectEdit,
    PaletteDialog,
    EquationDialog,
    SettingsDialog,
    JourneyDialog,
};

class ParameterGestureCoalescer {
public:
    [[nodiscard]] std::uint64_t Begin(ParameterGestureKind gesture) noexcept;
    [[nodiscard]] std::uint64_t ContinueOrBegin(ParameterGestureKind gesture,
                                                std::uint64_t eventTimestampMilliseconds,
                                                std::uint64_t maximumGapMilliseconds) noexcept;
    void End(ParameterGestureKind gesture) noexcept;
    void Reset() noexcept;
    [[nodiscard]] std::uint64_t ActiveToken(ParameterGestureKind gesture) const noexcept;

private:
    [[nodiscard]] std::uint64_t NextToken() noexcept;

    ParameterGestureKind activeGesture_{ParameterGestureKind::None};
    std::uint64_t activeToken_{0};
    std::uint64_t lastEventTimestampMilliseconds_{0};
    std::uint64_t nextToken_{1};
};

[[nodiscard]] constexpr bool IsParameterMutationOriginHistoryEligible(
    ParameterMutationOrigin origin) noexcept {
    return origin == ParameterMutationOrigin::UserControl ||
           origin == ParameterMutationOrigin::UserGesture ||
           origin == ParameterMutationOrigin::ScoutApply;
}

struct ParameterMutationContext {
    ParameterMutationOrigin origin{ParameterMutationOrigin::UserControl};
    ParameterGestureKind gestureKind{ParameterGestureKind::None};
    std::uint64_t coalescingToken{0};
};

struct ParameterMutation {
    ParameterKey key;
    ParameterValue value;
};

struct ParameterMutationResult {
    ParameterMutationOrigin origin{ParameterMutationOrigin::UserControl};
    ParameterGestureKind gestureKind{ParameterGestureKind::None};
    std::uint64_t coalescingToken{0};
    bool changed{false};
    bool historyEligible{false};
    bool coalescingEligible{false};
    bool normalised{false};
    ParameterInvalidationMask invalidationMask{0U};
    std::vector<ParameterKey> changedKeys;
};

struct ProjectPresetReplacementContext {
    ParameterMutationOrigin origin{ParameterMutationOrigin::PresetLoad};
    ProjectPresetReplacementKind kind{ProjectPresetReplacementKind::PresetLoad};
    bool historyEligible{true};
};

struct ProjectPresetReplacementResult {
    ParameterMutationOrigin origin{ParameterMutationOrigin::PresetLoad};
    ProjectPresetReplacementKind kind{ProjectPresetReplacementKind::PresetLoad};
    bool changed{false};
    bool normalised{false};
    bool historyEligible{false};
    bool requiresFullRender{false};
};

// Applies a bounded batch transactionally to the caller-owned authoritative Preset.
// Values are type-checked, finite real values are required, and the existing model
// normaliser remains the source of range rules. Only requested fields are committed.
bool ApplyProjectParameterMutations(Preset& authoritativePreset,
                                    std::span<const ParameterMutation> mutations,
                                    ParameterMutationResult& result,
                                    std::string& error,
                                    ParameterMutationContext context = {});

// Commits a parsed canonical exact camera as one transaction. The exact value
// is authoritative; CameraState is derived only through the one-way legacy
// adapter for current compatibility consumers. No exact coordinate is first
// reconstructed from a binary floating-point value.
bool ApplyProjectExactCameraMutation(Preset& authoritativePreset,
                                     const ExactCamera& exactCamera,
                                     ParameterMutationResult& result,
                                     std::string& error,
                                     ParameterMutationContext context = {});

// Applies a built-in palette selection as one bounded transaction. When requested,
// changing the selected built-in palette also clears custom stops, matching the
// existing main-window combo behaviour without exposing a second mutation path.
bool ApplyProjectPaletteSelection(Preset& authoritativePreset,
                                  Palette palette,
                                  bool clearCustomPaletteOnChange,
                                  ParameterMutationResult& result,
                                  std::string& error,
                                  ParameterMutationContext context = {});

bool ApplyProjectPresetReplacement(Preset& authoritativePreset,
                                   Preset candidate,
                                   ProjectPresetReplacementResult& result,
                                   std::string& error,
                                   ProjectPresetReplacementContext context = {});

// Modeless editors retain only their owned domain. Merging an editor candidate
// over the latest authoritative preset prevents unrelated changes made while
// the editor is open (especially exact camera navigation) from being reverted.
[[nodiscard]] Preset MergePaletteEditorCandidate(const Preset& authoritativePreset,
                                                  const Preset& editorCandidate);
[[nodiscard]] Preset MergeEquationEditorCandidate(const Preset& authoritativePreset,
                                                   const Preset& editorCandidate);

// Value captures over the current authoritative Preset. They do not own or observe
// live application state and only change a Preset when an explicit Apply call is made.
struct CameraParameterSnapshot {
    double centreX{-0.5};
    double centreXLow{0.0};
    double centreY{0.0};
    double centreYLow{0.0};
    double scale{1.5};
    double rotationDegrees{0.0};
};

struct PaletteParameterSnapshot {
    Palette palette{Palette::ClassicSpectrum};
    std::vector<Colour> customPaletteColours;
    double colourOffset{0.0};
    double paletteFrequency{8.0};
    double paletteGamma{1.0};
    PaletteInterpolation paletteInterpolation{PaletteInterpolation::Linear};
    double brightness{1.0};
    double contrast{1.0};
    double saturation{1.0};
    Colour interiorColour{0.0F, 0.0F, 0.0F, 1.0F};
    Colour backgroundColour{0.0F, 0.0F, 0.0F, 1.0F};
    bool smoothColouring{true};
    double stripeStrength{0.0};
    double bloomStrength{0.0};
    int bloomRadius{1};
    double edgeLightingStrength{0.0};
};

[[nodiscard]] CameraParameterSnapshot CaptureCameraParameters(const Preset& preset) noexcept;
[[nodiscard]] PaletteParameterSnapshot CapturePaletteParameters(const Preset& preset);
void ApplyCameraParameters(Preset& authoritativePreset,
                           const CameraParameterSnapshot& snapshot) noexcept;
void ApplyPaletteParameters(Preset& authoritativePreset,
                            const PaletteParameterSnapshot& snapshot);

struct RenderFingerprintContext {
    std::string rendererId{"cpu-production-still"};
    std::uint32_t width{0};
    std::uint32_t height{0};
    PrecisionSettings precision;
    // Populated for immutable deep jobs.  An empty plan version preserves the
    // legacy v2 exact identity for consumers that have no planner authority.
    std::string precisionPlanVersion;
    std::string precisionExecutionBackend;
    std::string precisionFormulaCapability;
    std::string precisionReason;
    int precisionRequiredBits{0};
    int precisionSelectedBits{0};
    double timeSeconds{0.0};
    std::uint64_t seed{0};
    bool scaleQualityToResolution{false};
};

struct RenderFingerprint {
    std::string algorithm;
    std::string canonicalVersion;
    std::string canonicalState;
    std::string digest;
};

// Serialises only render-affecting state. IDs, names, paths, clocks, progress,
// desktop/runtime resources and mutable application ownership are excluded.
bool BuildRenderFingerprint(const Preset& preset,
                            const RenderFingerprintContext& context,
                            RenderFingerprint& result,
                            std::string& error);

// Exact-camera identity is intentionally a distinct byte contract. It never
// reinterprets `mw-render-state-v1` bytes or permits a missing exact camera.
bool BuildExactRenderFingerprint(const Preset& preset,
                                 const RenderFingerprintContext& context,
                                 RenderFingerprint& result,
                                 std::string& error);

[[nodiscard]] std::string Sha256Hex(std::string_view text);

} // namespace mw
