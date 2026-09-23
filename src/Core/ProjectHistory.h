#pragma once

#include "Core/ProjectState.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mw {

struct ParameterChangeOperation {
    ParameterKey key{};
    ParameterValue before{};
    ParameterValue after{};
    ParameterInvalidation invalidation{ParameterInvalidation::CameraViewport};
};

struct CameraHistoryMetadata {
    double startingScaleBefore{1.5};
    double startingScaleAfter{1.5};
    AnimationMode animationModeBefore{AnimationMode::ManualView};
    AnimationMode animationModeAfter{AnimationMode::ManualView};
    bool changed{false};
};

enum class ProjectHistoryEntryKind {
    ParameterMutation,
    PresetReplacement,
};

struct PresetReplacementOperation {
    ProjectPresetReplacementKind kind{ProjectPresetReplacementKind::PresetLoad};
    Preset before;
    Preset after;
};

struct ProjectHistoryEntry {
    ProjectHistoryEntryKind kind{ProjectHistoryEntryKind::ParameterMutation};
    std::string label;
    ParameterMutationOrigin origin{ParameterMutationOrigin::UserControl};
    ParameterGestureKind gestureKind{ParameterGestureKind::None};
    std::uint64_t coalescingToken{0};
    ParameterInvalidationMask invalidationMask{0U};
    std::vector<ParameterChangeOperation> parameterChanges;
    CameraHistoryMetadata cameraMetadata;
    std::unique_ptr<PresetReplacementOperation> presetReplacement;
    std::size_t estimatedBytes{0U};
};

struct ProjectHistoryLimits {
    // Hard containment bounds, configurable by the caller. These are not persisted.
    std::size_t maximumEntries{256U};
    std::size_t maximumEstimatedBytes{4U * 1024U * 1024U};
};

struct ProjectHistoryApplyResult {
    bool changed{false};
    bool requiresFullRender{false};
    ParameterInvalidationMask invalidationMask{0U};
    std::string label;
};

class ProjectHistory {
public:
    explicit ProjectHistory(ProjectHistoryLimits limits = {});

    bool RecordParameterMutation(const Preset& before,
                                 const Preset& after,
                                 const ParameterMutationResult& mutationResult,
                                 std::string label,
                                 std::string& error);

    bool RecordPresetReplacement(const Preset& before,
                                 const Preset& after,
                                 const ProjectPresetReplacementResult& replacementResult,
                                 std::string label,
                                 std::string& error);

    bool Undo(Preset& authoritativePreset,
              ProjectHistoryApplyResult& result,
              std::string& error);
    bool Redo(Preset& authoritativePreset,
              ProjectHistoryApplyResult& result,
              std::string& error);

    void Clear() noexcept;

    [[nodiscard]] bool CanUndo() const noexcept;
    [[nodiscard]] bool CanRedo() const noexcept;
    [[nodiscard]] std::string UndoLabel() const;
    [[nodiscard]] std::string RedoLabel() const;
    [[nodiscard]] std::size_t EntryCount() const noexcept;
    [[nodiscard]] std::size_t Cursor() const noexcept;
    [[nodiscard]] std::size_t EstimatedBytes() const noexcept;

private:
    bool AppendEntry(ProjectHistoryEntry entry, std::string& error);
    bool ApplyEntry(Preset& authoritativePreset,
                    const ProjectHistoryEntry& entry,
                    bool useAfter,
                    ProjectHistoryApplyResult& result,
                    std::string& error) const;
    void TruncateRedoBranch() noexcept;
    void EnforceBounds() noexcept;
    [[nodiscard]] static std::size_t EstimateEntryBytes(const ProjectHistoryEntry& entry) noexcept;
    [[nodiscard]] static std::size_t EstimatePresetBytes(const Preset& preset) noexcept;
    [[nodiscard]] static bool SameTargets(const ProjectHistoryEntry& left,
                                          const ProjectHistoryEntry& right) noexcept;

    ProjectHistoryLimits limits_;
    std::vector<ProjectHistoryEntry> entries_;
    std::size_t cursor_{0U};
    std::size_t estimatedBytes_{0U};
};

} // namespace mw
