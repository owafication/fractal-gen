#include "Core/ProjectHistory.h"

#include <algorithm>
#include <array>
#include <utility>

namespace mw {
namespace {

const ParameterDescriptor* DescriptorFor(ParameterKey key) noexcept {
    for (const auto& descriptor : ProjectParameterDescriptors()) {
        if (descriptor.key == key) return &descriptor;
    }
    return nullptr;
}

constexpr ParameterInvalidationMask FullProjectInvalidationMask() noexcept {
    return static_cast<ParameterInvalidationMask>(
        ParameterInvalidationBit(ParameterInvalidation::CameraViewport) |
        ParameterInvalidationBit(ParameterInvalidation::PaletteColouring) |
        ParameterInvalidationBit(ParameterInvalidation::PostProcessing) |
        ParameterInvalidationBit(ParameterInvalidation::EquationPrecision));
}

std::string DefaultReplacementLabel(ProjectPresetReplacementKind kind) {
    switch (kind) {
    case ProjectPresetReplacementKind::PresetLoad: return "Load Preset";
    case ProjectPresetReplacementKind::ImportedPreset: return "Import Preset";
    case ProjectPresetReplacementKind::DirectProjectEdit: return "Edit Project";
    case ProjectPresetReplacementKind::PaletteDialog: return "Edit Palette";
    case ProjectPresetReplacementKind::EquationDialog: return "Edit Equation";
    case ProjectPresetReplacementKind::SettingsDialog: return "Edit Project Settings";
    case ProjectPresetReplacementKind::JourneyDialog: return "Edit Journey";
    }
    return "Edit Project";
}

bool IsNavigationCameraEntry(const ProjectHistoryEntry& entry) noexcept {
    if (entry.gestureKind != ParameterGestureKind::PreviewNavigation ||
        entry.parameterChanges.empty()) return false;
    return std::all_of(entry.parameterChanges.begin(), entry.parameterChanges.end(),
                       [](const ParameterChangeOperation& change) {
                           const auto* descriptor = DescriptorFor(change.key);
                           return descriptor && descriptor->domain == ParameterDomain::Camera;
                       });
}

} // namespace

ProjectHistory::ProjectHistory(ProjectHistoryLimits limits) : limits_(limits) {
    if (limits_.maximumEntries == 0U) limits_.maximumEntries = 1U;
    if (limits_.maximumEstimatedBytes == 0U) limits_.maximumEstimatedBytes = 1U;
}

bool ProjectHistory::RecordParameterMutation(const Preset& before,
                                             const Preset& after,
                                             const ParameterMutationResult& mutationResult,
                                             std::string label,
                                             std::string& error) {
    error.clear();
    if (!mutationResult.changed) return true;
    if (!mutationResult.historyEligible ||
        !IsParameterMutationOriginHistoryEligible(mutationResult.origin)) {
        return true;
    }

    ProjectHistoryEntry entry;
    entry.kind = ProjectHistoryEntryKind::ParameterMutation;
    entry.label = label.empty() ? "Edit" : std::move(label);
    entry.origin = mutationResult.origin;
    entry.gestureKind = mutationResult.gestureKind;
    entry.coalescingToken = mutationResult.coalescingToken;

    for (const ParameterKey key : mutationResult.changedKeys) {
        const ParameterDescriptor* descriptor = DescriptorFor(key);
        if (!descriptor || !descriptor->historyEligible) continue;
        ParameterValue beforeValue;
        ParameterValue afterValue;
        if (!ReadProjectParameterValue(before, key, beforeValue, error) ||
            !ReadProjectParameterValue(after, key, afterValue, error)) {
            return false;
        }
        if (beforeValue == afterValue) continue;
        entry.parameterChanges.push_back({key, beforeValue, afterValue, descriptor->invalidation});
        entry.invalidationMask = static_cast<ParameterInvalidationMask>(
            entry.invalidationMask | ParameterInvalidationBit(descriptor->invalidation));
    }

    const bool cameraMetadataChanged =
        before.startingScale != after.startingScale || before.animationMode != after.animationMode;
    if (cameraMetadataChanged) {
        const bool hasCameraChange = std::any_of(
            entry.parameterChanges.begin(), entry.parameterChanges.end(),
            [](const ParameterChangeOperation& change) {
                const auto* descriptor = DescriptorFor(change.key);
                return descriptor && descriptor->domain == ParameterDomain::Camera;
            });
        if (hasCameraChange) {
            entry.cameraMetadata = {before.startingScale, after.startingScale,
                                    before.animationMode, after.animationMode, true};
            entry.invalidationMask = static_cast<ParameterInvalidationMask>(
                entry.invalidationMask |
                ParameterInvalidationBit(ParameterInvalidation::CameraViewport));
        }
    }

    if (entry.parameterChanges.empty() && !entry.cameraMetadata.changed) return true;

    // Prove that the scalar representation exactly covers the user action. If
    // it does not (for example selecting a built-in palette removed custom
    // stops), record one atomic structural snapshot instead of partial history.
    Preset represented = before;
    std::vector<ParameterMutation> replayMutations;
    replayMutations.reserve(entry.parameterChanges.size());
    for (const auto& change : entry.parameterChanges) {
        replayMutations.push_back({change.key, change.after});
    }
    if (!replayMutations.empty()) {
        ParameterMutationResult replayResult;
        if (!ApplyProjectParameterMutations(
                represented, replayMutations, replayResult, error,
                {ParameterMutationOrigin::UndoRedo})) {
            return false;
        }
    }
    if (entry.cameraMetadata.changed) {
        represented.startingScale = entry.cameraMetadata.startingScaleAfter;
        represented.animationMode = entry.cameraMetadata.animationModeAfter;
    }
    if (represented != after) {
        ProjectPresetReplacementResult structuralResult;
        structuralResult.origin = mutationResult.origin;
        structuralResult.kind = ProjectPresetReplacementKind::DirectProjectEdit;
        structuralResult.changed = true;
        structuralResult.historyEligible = true;
        structuralResult.requiresFullRender = true;
        return RecordPresetReplacement(before, after, structuralResult,
                                       std::move(entry.label), error);
    }

    entry.estimatedBytes = EstimateEntryBytes(entry);
    if (entry.estimatedBytes > limits_.maximumEstimatedBytes) {
        error = "History entry exceeds the configured memory bound.";
        return false;
    }

    const bool sameNavigationCameraGesture =
        mutationResult.coalescingEligible && cursor_ == entries_.size() &&
        !entries_.empty() &&
        entries_.back().kind == ProjectHistoryEntryKind::ParameterMutation &&
        entries_.back().origin == entry.origin &&
        entries_.back().gestureKind == entry.gestureKind &&
        entries_.back().coalescingToken == entry.coalescingToken &&
        entry.coalescingToken != 0U &&
        IsNavigationCameraEntry(entries_.back()) && IsNavigationCameraEntry(entry);
    const bool canCoalesce = sameNavigationCameraGesture ||
        (mutationResult.coalescingEligible && cursor_ == entries_.size() &&
                             !entries_.empty() &&
                             entries_.back().kind == ProjectHistoryEntryKind::ParameterMutation &&
                             entries_.back().origin == entry.origin &&
                             entries_.back().gestureKind == entry.gestureKind &&
                             entries_.back().coalescingToken == entry.coalescingToken &&
                             entry.coalescingToken != 0U &&
                             SameTargets(entries_.back(), entry));
    if (canCoalesce) {
        ProjectHistoryEntry& previous = entries_.back();
        estimatedBytes_ -= previous.estimatedBytes;
        if (sameNavigationCameraGesture) {
            for (const ParameterChangeOperation& change : entry.parameterChanges) {
                const auto existing = std::find_if(
                    previous.parameterChanges.begin(), previous.parameterChanges.end(),
                    [&](const ParameterChangeOperation& prior) { return prior.key == change.key; });
                if (existing != previous.parameterChanges.end()) existing->after = change.after;
                else previous.parameterChanges.push_back(change);
            }
        } else {
            for (std::size_t index = 0U; index < previous.parameterChanges.size(); ++index) {
                previous.parameterChanges[index].after = entry.parameterChanges[index].after;
            }
        }
        if (entry.cameraMetadata.changed) {
            previous.cameraMetadata.startingScaleAfter = entry.cameraMetadata.startingScaleAfter;
            previous.cameraMetadata.animationModeAfter = entry.cameraMetadata.animationModeAfter;
            previous.cameraMetadata.changed = true;
        }
        previous.invalidationMask = static_cast<ParameterInvalidationMask>(
            previous.invalidationMask | entry.invalidationMask);
        previous.label = entry.label;
        previous.estimatedBytes = EstimateEntryBytes(previous);
        estimatedBytes_ += previous.estimatedBytes;
        EnforceBounds();
        return true;
    }

    return AppendEntry(std::move(entry), error);
}

bool ProjectHistory::RecordPresetReplacement(
    const Preset& before,
    const Preset& after,
    const ProjectPresetReplacementResult& replacementResult,
    std::string label,
    std::string& error) {
    error.clear();
    if (!replacementResult.changed || !replacementResult.historyEligible || before == after) {
        return true;
    }
    if (replacementResult.origin == ParameterMutationOrigin::UndoRedo ||
        replacementResult.origin == ParameterMutationOrigin::AnimationEvaluation ||
        replacementResult.origin == ParameterMutationOrigin::ExportEvaluation ||
        replacementResult.origin == ParameterMutationOrigin::Migration ||
        replacementResult.origin == ParameterMutationOrigin::SystemRuntime) {
        return true;
    }

    ProjectHistoryEntry entry;
    entry.kind = ProjectHistoryEntryKind::PresetReplacement;
    entry.label = label.empty() ? DefaultReplacementLabel(replacementResult.kind)
                                : std::move(label);
    entry.origin = replacementResult.origin;
    entry.invalidationMask = FullProjectInvalidationMask();
    entry.presetReplacement = std::make_unique<PresetReplacementOperation>(
        PresetReplacementOperation{replacementResult.kind, before, after});
    entry.estimatedBytes = EstimateEntryBytes(entry);
    return AppendEntry(std::move(entry), error);
}

bool ProjectHistory::AppendEntry(ProjectHistoryEntry entry, std::string& error) {
    if (entry.estimatedBytes == 0U) entry.estimatedBytes = EstimateEntryBytes(entry);
    if (entry.estimatedBytes > limits_.maximumEstimatedBytes) {
        error = "History entry exceeds the configured memory bound.";
        return false;
    }
    TruncateRedoBranch();
    estimatedBytes_ += entry.estimatedBytes;
    entries_.push_back(std::move(entry));
    cursor_ = entries_.size();
    EnforceBounds();
    return true;
}

bool ProjectHistory::Undo(Preset& authoritativePreset,
                          ProjectHistoryApplyResult& result,
                          std::string& error) {
    result = {};
    error.clear();
    if (!CanUndo()) return true;
    const ProjectHistoryEntry& entry = entries_[cursor_ - 1U];
    if (!ApplyEntry(authoritativePreset, entry, false, result, error)) return false;
    --cursor_;
    return true;
}

bool ProjectHistory::Redo(Preset& authoritativePreset,
                          ProjectHistoryApplyResult& result,
                          std::string& error) {
    result = {};
    error.clear();
    if (!CanRedo()) return true;
    const ProjectHistoryEntry& entry = entries_[cursor_];
    if (!ApplyEntry(authoritativePreset, entry, true, result, error)) return false;
    ++cursor_;
    return true;
}

bool ProjectHistory::ApplyEntry(Preset& authoritativePreset,
                                const ProjectHistoryEntry& entry,
                                bool useAfter,
                                ProjectHistoryApplyResult& result,
                                std::string& error) const {
    if (entry.kind == ProjectHistoryEntryKind::PresetReplacement) {
        if (!entry.presetReplacement) {
            error = "Structural history entry is missing its project snapshots.";
            return false;
        }
        authoritativePreset = useAfter ? entry.presetReplacement->after
                                       : entry.presetReplacement->before;
        result.changed = true;
        result.requiresFullRender = true;
        result.invalidationMask = entry.invalidationMask;
        result.label = entry.label;
        return true;
    }

    Preset candidate = authoritativePreset;
    std::vector<ParameterMutation> mutations;
    mutations.reserve(entry.parameterChanges.size());
    for (const auto& change : entry.parameterChanges) {
        mutations.push_back({change.key, useAfter ? change.after : change.before});
    }
    if (!mutations.empty()) {
        ParameterMutationResult replay;
        if (!ApplyProjectParameterMutations(candidate, mutations, replay, error,
                                            {ParameterMutationOrigin::UndoRedo})) {
            return false;
        }
    }
    if (entry.cameraMetadata.changed) {
        candidate.startingScale = useAfter ? entry.cameraMetadata.startingScaleAfter
                                           : entry.cameraMetadata.startingScaleBefore;
        candidate.animationMode = useAfter ? entry.cameraMetadata.animationModeAfter
                                           : entry.cameraMetadata.animationModeBefore;
    }
    authoritativePreset = std::move(candidate);
    result.changed = true;
    result.requiresFullRender = false;
    result.invalidationMask = entry.invalidationMask;
    result.label = entry.label;
    return true;
}

void ProjectHistory::Clear() noexcept {
    entries_.clear();
    cursor_ = 0U;
    estimatedBytes_ = 0U;
}

bool ProjectHistory::CanUndo() const noexcept { return cursor_ > 0U; }
bool ProjectHistory::CanRedo() const noexcept { return cursor_ < entries_.size(); }

std::string ProjectHistory::UndoLabel() const {
    return CanUndo() ? entries_[cursor_ - 1U].label : std::string{};
}

std::string ProjectHistory::RedoLabel() const {
    return CanRedo() ? entries_[cursor_].label : std::string{};
}

std::size_t ProjectHistory::EntryCount() const noexcept { return entries_.size(); }
std::size_t ProjectHistory::Cursor() const noexcept { return cursor_; }
std::size_t ProjectHistory::EstimatedBytes() const noexcept { return estimatedBytes_; }

void ProjectHistory::TruncateRedoBranch() noexcept {
    while (entries_.size() > cursor_) {
        estimatedBytes_ -= entries_.back().estimatedBytes;
        entries_.pop_back();
    }
}

void ProjectHistory::EnforceBounds() noexcept {
    while (!entries_.empty() &&
           (entries_.size() > limits_.maximumEntries ||
            estimatedBytes_ > limits_.maximumEstimatedBytes)) {
        estimatedBytes_ -= entries_.front().estimatedBytes;
        entries_.erase(entries_.begin());
        if (cursor_ > 0U) --cursor_;
    }
}

std::size_t ProjectHistory::EstimateEntryBytes(const ProjectHistoryEntry& entry) noexcept {
    std::size_t bytes = sizeof(ProjectHistoryEntry) + entry.label.size() +
                        entry.parameterChanges.size() * sizeof(ParameterChangeOperation);
    if (entry.presetReplacement) {
        bytes += sizeof(PresetReplacementOperation);
        bytes += EstimatePresetBytes(entry.presetReplacement->before);
        bytes += EstimatePresetBytes(entry.presetReplacement->after);
    }
    return bytes;
}

std::size_t ProjectHistory::EstimatePresetBytes(const Preset& preset) noexcept {
    return preset.id.capacity() + preset.name.capacity() +
           preset.automaticJourneyWaypoints.capacity() +
           preset.customPaletteColours.capacity() * sizeof(Colour);
}

bool ProjectHistory::SameTargets(const ProjectHistoryEntry& left,
                                 const ProjectHistoryEntry& right) noexcept {
    if (left.kind != ProjectHistoryEntryKind::ParameterMutation ||
        right.kind != ProjectHistoryEntryKind::ParameterMutation ||
        left.parameterChanges.size() != right.parameterChanges.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < left.parameterChanges.size(); ++index) {
        if (left.parameterChanges[index].key != right.parameterChanges[index].key) return false;
    }
    return true;
}

} // namespace mw
