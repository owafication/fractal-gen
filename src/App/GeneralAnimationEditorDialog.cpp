#include "App/GeneralAnimationEditorDialog.h"

#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace mw {
#ifdef _WIN32
namespace {

constexpr wchar_t kAnimationEditorClass[] = L"MandelbrotGeneralAnimationEditorDialog";
constexpr UINT_PTR kPlaybackTimerId = 1U;
constexpr int kScrubberMaximum = 10000;

enum Id : int {
    TrackList = 9801,
    TargetCombo,
    AddTrackButton,
    RemoveTrackButton,
    TrackEnabledCheck,
    KeyframeList,
    KeyframeTimeEdit,
    UpdateKeyframeButton,
    AddCurrentButton,
    RemoveKeyframeButton,
    DurationEdit,
    LoopModeCombo,
    ScrubberTrack,
    TimeLabel,
    InterpolationCombo,
    PlayPauseButton,
    StopButton,
    JourneyToTracksButton,
    TracksToJourneyButton,
    ValidationLabel,
    OkButton,
    CancelButton,
};

struct State {
    HWND owner{};
    HWND window{};
    HINSTANCE instance{};
    HFONT font{};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    Preset basePreset;
    AnimationTimeline originalTimeline;
    AnimationTimeline candidate;
    AnimationTimeline* outputTimeline{};
    AnimationClockBank* clocks{};
    GeneralAnimationEditorDialog::PreviewCallback previewCallback;
    std::optional<std::string> pendingJourneyScript;
    std::chrono::steady_clock::time_point lastPlaybackTick{};
    double originalPreviewTime{0.0};
    std::uint64_t nextId{1U};
    bool playing{false};
    bool accepted{false};
    bool done{false};
};

std::wstring Wide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), result.data(), count);
    return result;
}

std::string Utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0,
                                          nullptr, nullptr);
    if (count <= 0) return {};
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), result.data(), count,
                        nullptr, nullptr);
    return result;
}

std::wstring Read(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) return {};
    std::wstring value(static_cast<std::size_t>(length) + 1U, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

void SetText(HWND control, const std::string& value) {
    SetWindowTextW(control, Wide(value).c_str());
}

std::string FormatDouble(double value, int precision = 10) {
    std::ostringstream stream;
    stream << std::setprecision(precision) << value;
    return stream.str();
}

std::string InterpolationName(AnimationInterpolation interpolation) {
    switch (interpolation) {
    case AnimationInterpolation::Step: return "Step";
    case AnimationInterpolation::Linear: return "Linear";
    case AnimationInterpolation::Smoothstep: return "Smoothstep";
    }
    return "Unknown";
}

std::string LoopModeName(AnimationLoopMode mode) {
    switch (mode) {
    case AnimationLoopMode::Clamp: return "Clamp";
    case AnimationLoopMode::Loop: return "Loop";
    case AnimationLoopMode::PingPong: return "PingPong";
    }
    return "Unknown";
}

std::string ValueText(const AnimationValue& value) {
    if (const auto* real = std::get_if<double>(&value)) return FormatDouble(*real, 12);
    if (const auto* integer = std::get_if<std::int64_t>(&value)) return std::to_string(*integer);
    const auto& compensated = std::get<CompensatedAnimationValue>(value);
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10)
           << compensated.high;
    if (compensated.low != 0.0) stream << " + " << compensated.low;
    return stream.str();
}

int SelectedIndex(HWND control) {
    return static_cast<int>(SendMessageW(control, LB_GETCURSEL, 0, 0));
}

int SelectedComboIndex(HWND control) {
    return static_cast<int>(SendMessageW(control, CB_GETCURSEL, 0, 0));
}

AnimationTrack* SelectedTrack(State& state) {
    const int index = SelectedIndex(GetDlgItem(state.window, TrackList));
    if (index < 0 || index >= static_cast<int>(state.candidate.tracks.size())) return nullptr;
    return &state.candidate.tracks[static_cast<std::size_t>(index)];
}

const AnimationTrack* SelectedTrack(const State& state) {
    const int index = SelectedIndex(GetDlgItem(state.window, TrackList));
    if (index < 0 || index >= static_cast<int>(state.candidate.tracks.size())) return nullptr;
    return &state.candidate.tracks[static_cast<std::size_t>(index)];
}

AnimationInterpolation SelectedInterpolation(const State& state) {
    switch (SelectedComboIndex(GetDlgItem(state.window, InterpolationCombo))) {
    case 0: return AnimationInterpolation::Step;
    case 2: return AnimationInterpolation::Smoothstep;
    default: return AnimationInterpolation::Linear;
    }
}

std::string NewStableId(State& state, const char* prefix) {
    for (;;) {
        const std::string candidate = std::string(prefix) + std::to_string(state.nextId++);
        bool used = state.candidate.id == candidate;
        for (const auto& track : state.candidate.tracks) {
            used = used || track.id == candidate;
            for (const auto& keyframe : track.keyframes) used = used || keyframe.id == candidate;
        }
        if (!used) return candidate;
    }
}

void SetValidation(State& state, const std::string& text, bool error) {
    HWND label = GetDlgItem(state.window, ValidationLabel);
    SetText(label, text);
    SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
    (void)error;
}

void PopulateSelectedKeyframeControls(State& state) {
    const AnimationTrack* track = SelectedTrack(state);
    const int keyIndex = SelectedIndex(GetDlgItem(state.window, KeyframeList));
    const bool selected = track && keyIndex >= 0 &&
                          keyIndex < static_cast<int>(track->keyframes.size());
    EnableWindow(GetDlgItem(state.window, KeyframeTimeEdit), selected ? TRUE : FALSE);
    EnableWindow(GetDlgItem(state.window, InterpolationCombo), selected ? TRUE : FALSE);
    EnableWindow(GetDlgItem(state.window, UpdateKeyframeButton), selected ? TRUE : FALSE);
    EnableWindow(GetDlgItem(state.window, RemoveKeyframeButton), selected ? TRUE : FALSE);
    if (!selected) {
        SetWindowTextW(GetDlgItem(state.window, KeyframeTimeEdit), L"");
        return;
    }
    const auto& keyframe = track->keyframes[static_cast<std::size_t>(keyIndex)];
    SetText(GetDlgItem(state.window, KeyframeTimeEdit), FormatDouble(keyframe.timeSeconds, 10));
    int interpolation = 1;
    if (keyframe.interpolation == AnimationInterpolation::Step) interpolation = 0;
    else if (keyframe.interpolation == AnimationInterpolation::Smoothstep) interpolation = 2;
    SendMessageW(GetDlgItem(state.window, InterpolationCombo), CB_SETCURSEL,
                 interpolation, 0);
}

void PopulateKeyframes(State& state) {
    HWND list = GetDlgItem(state.window, KeyframeList);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    const AnimationTrack* track = SelectedTrack(state);
    if (!track) {
        PopulateSelectedKeyframeControls(state);
        return;
    }
    for (const auto& keyframe : track->keyframes) {
        std::ostringstream row;
        row << std::fixed << std::setprecision(3) << keyframe.timeSeconds
            << " s  |  " << ValueText(keyframe.value)
            << "  |  " << InterpolationName(keyframe.interpolation);
        const std::wstring wide = Wide(row.str());
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str()));
    }
    if (!track->keyframes.empty()) SendMessageW(list, LB_SETCURSEL, 0, 0);
    PopulateSelectedKeyframeControls(state);
}

void RefreshValidation(State& state) {
    const auto validation = ValidateGeneralAnimationTimeline(state.candidate);
    if (!validation.valid) {
        const auto& issue = validation.issues.front();
        SetValidation(state, "Conflict: " + issue.field + ": " + issue.message, true);
        EnableWindow(GetDlgItem(state.window, OkButton), FALSE);
        return;
    }
    std::ostringstream text;
    text << "Valid runtime-only timeline: " << state.candidate.tracks.size()
         << " track(s), " << state.candidate.durationSeconds << " seconds, "
         << LoopModeName(state.candidate.loopMode) << ".";
    if (!validation.warnings.empty()) text << " Warning: " << validation.warnings.front().message;
    if (state.pendingJourneyScript) text << " Journey update prepared and will apply only with OK.";
    SetValidation(state, text.str(), false);
    EnableWindow(GetDlgItem(state.window, OkButton), TRUE);
}

void PopulateTracks(State& state, int preferredSelection = -1) {
    HWND list = GetDlgItem(state.window, TrackList);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const auto& track : state.candidate.tracks) {
        std::string row = track.enabled ? "[on] " : "[off] ";
        row += track.target + "  (" + std::to_string(track.keyframes.size()) + ")";
        const std::wstring wide = Wide(row);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str()));
    }
    if (!state.candidate.tracks.empty()) {
        const int selection = std::clamp(preferredSelection < 0 ? 0 : preferredSelection,
                                         0, static_cast<int>(state.candidate.tracks.size()) - 1);
        SendMessageW(list, LB_SETCURSEL, selection, 0);
    }
    const AnimationTrack* selected = SelectedTrack(state);
    SendMessageW(GetDlgItem(state.window, TrackEnabledCheck), BM_SETCHECK,
                 selected && selected->enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(state.window, TrackEnabledCheck), selected ? TRUE : FALSE);
    EnableWindow(GetDlgItem(state.window, RemoveTrackButton), selected ? TRUE : FALSE);
    EnableWindow(GetDlgItem(state.window, AddCurrentButton), selected ? TRUE : FALSE);
    PopulateKeyframes(state);
    RefreshValidation(state);
}

void NotifyPreview(State& state, bool active = true) {
    state.previewCallback(state.candidate,
                          state.clocks->Time(AnimationClockDomain::Preview), active);
}

void UpdateScrubberFromClock(State& state) {
    const double duration = std::max(1.0e-12, state.candidate.durationSeconds);
    double time = state.clocks->Time(AnimationClockDomain::Preview);
    if (state.candidate.loopMode == AnimationLoopMode::Clamp) {
        time = std::clamp(time, 0.0, duration);
    } else if (state.candidate.loopMode == AnimationLoopMode::Loop) {
        time = std::fmod(time, duration);
        if (time < 0.0) time += duration;
    } else {
        const double period = duration * 2.0;
        time = std::fmod(time, period);
        if (time < 0.0) time += period;
        if (time > duration) time = period - time;
    }
    const int position = std::clamp(
        static_cast<int>(std::lround((time / duration) * kScrubberMaximum)),
        0, kScrubberMaximum);
    SendMessageW(GetDlgItem(state.window, ScrubberTrack), TBM_SETPOS, TRUE, position);
    SetText(GetDlgItem(state.window, TimeLabel),
            "Time: " + FormatDouble(time, 6) + " / " +
            FormatDouble(state.candidate.durationSeconds, 6) + " s");
}

void SetClockFromScrubber(State& state) {
    const int position = static_cast<int>(SendMessageW(
        GetDlgItem(state.window, ScrubberTrack), TBM_GETPOS, 0, 0));
    const double time = state.candidate.durationSeconds *
                        static_cast<double>(position) /
                        static_cast<double>(kScrubberMaximum);
    std::string error;
    if (!state.clocks->SetTime(AnimationClockDomain::Preview, time, error)) {
        SetValidation(state, error, true);
        return;
    }
    UpdateScrubberFromClock(state);
    NotifyPreview(state);
}

void StopPlayback(State& state) {
    state.playing = false;
    SetWindowTextW(GetDlgItem(state.window, PlayPauseButton), L"Play");
    state.clocks->Reset(AnimationClockDomain::Preview);
    UpdateScrubberFromClock(state);
    NotifyPreview(state);
}

bool ApplyDuration(State& state, bool showError) {
    const std::string text = Utf8(Read(GetDlgItem(state.window, DurationEdit)));
    double duration = 0.0;
    try {
        std::size_t consumed = 0U;
        duration = std::stod(text, &consumed);
        if (consumed != text.size()) throw std::invalid_argument("trailing");
    } catch (...) {
        if (showError) MessageBoxW(state.window, L"Duration must be a finite positive number of seconds.",
                                   L"Animation Timeline", MB_OK | MB_ICONWARNING);
        SetText(GetDlgItem(state.window, DurationEdit), FormatDouble(state.candidate.durationSeconds));
        return false;
    }
    if (!std::isfinite(duration) || duration <= 0.0 || duration > 604800.0) {
        if (showError) MessageBoxW(state.window, L"Duration must be greater than zero and no longer than seven days.",
                                   L"Animation Timeline", MB_OK | MB_ICONWARNING);
        SetText(GetDlgItem(state.window, DurationEdit), FormatDouble(state.candidate.durationSeconds));
        return false;
    }
    for (const auto& track : state.candidate.tracks) {
        if (!track.keyframes.empty() && track.keyframes.back().timeSeconds > duration) {
            if (showError) MessageBoxW(state.window,
                L"Duration cannot be shorter than an existing keyframe.",
                L"Animation Timeline", MB_OK | MB_ICONWARNING);
            SetText(GetDlgItem(state.window, DurationEdit), FormatDouble(state.candidate.durationSeconds));
            return false;
        }
    }
    state.candidate.durationSeconds = duration;
    state.pendingJourneyScript.reset();
    const double current = std::min(state.clocks->Time(AnimationClockDomain::Preview), duration);
    std::string error;
    (void)state.clocks->SetTime(AnimationClockDomain::Preview, current, error);
    UpdateScrubberFromClock(state);
    RefreshValidation(state);
    NotifyPreview(state);
    return true;
}

void AddTrack(State& state) {
    const int targetIndex = SelectedComboIndex(GetDlgItem(state.window, TargetCombo));
    const auto descriptors = GeneralAnimationTargetDescriptors();
    if (targetIndex < 0 || targetIndex >= static_cast<int>(descriptors.size())) return;
    const auto& descriptor = descriptors[static_cast<std::size_t>(targetIndex)];
    const bool duplicateEnabled = std::any_of(
        state.candidate.tracks.begin(), state.candidate.tracks.end(),
        [&](const AnimationTrack& track) {
            return track.enabled && track.target == descriptor.stableName;
        });
    if (duplicateEnabled) {
        MessageBoxW(state.window, L"An enabled track already owns this Replace target.",
                    L"Animation Timeline", MB_OK | MB_ICONWARNING);
        return;
    }

    AnimationTrack track;
    track.id = NewStableId(state, "track-");
    track.target = std::string(descriptor.stableName);
    track.enabled = true;
    state.candidate.tracks.push_back(std::move(track));
    const int newIndex = static_cast<int>(state.candidate.tracks.size()) - 1;
    PopulateTracks(state, newIndex);

    const AnimationInterpolation interpolation = descriptor.valueType == AnimationValueType::Integer
        ? AnimationInterpolation::Step : AnimationInterpolation::Linear;
    std::string error;
    if (!AddGeneralAnimationKeyframeFromPreset(
            state.candidate,
            state.candidate.tracks[static_cast<std::size_t>(newIndex)].id,
            NewStableId(state, "key-"),
            state.clocks->Time(AnimationClockDomain::Preview),
            state.basePreset, interpolation, error)) {
        state.candidate.tracks.erase(state.candidate.tracks.begin() + newIndex);
        PopulateTracks(state, std::max(0, newIndex - 1));
        MessageBoxW(state.window, Wide(error).c_str(), L"Animation Timeline",
                    MB_OK | MB_ICONWARNING);
        return;
    }
    state.pendingJourneyScript.reset();
    PopulateTracks(state, newIndex);
    NotifyPreview(state);
}

void RemoveTrack(State& state) {
    const int index = SelectedIndex(GetDlgItem(state.window, TrackList));
    if (index < 0 || index >= static_cast<int>(state.candidate.tracks.size())) return;
    state.candidate.tracks.erase(state.candidate.tracks.begin() + index);
    state.pendingJourneyScript.reset();
    PopulateTracks(state, std::max(0, index - 1));
    NotifyPreview(state);
}

void SetTrackEnabled(State& state) {
    AnimationTrack* track = SelectedTrack(state);
    if (!track) return;
    const bool enabled = SendMessageW(GetDlgItem(state.window, TrackEnabledCheck),
                                      BM_GETCHECK, 0, 0) == BST_CHECKED;
    const bool previous = track->enabled;
    track->enabled = enabled;
    const auto validation = ValidateGeneralAnimationTimeline(state.candidate);
    if (!validation.valid) {
        track->enabled = previous;
        SendMessageW(GetDlgItem(state.window, TrackEnabledCheck), BM_SETCHECK,
                     previous ? BST_CHECKED : BST_UNCHECKED, 0);
        MessageBoxW(state.window, Wide(validation.issues.front().message).c_str(),
                    L"Animation Timeline", MB_OK | MB_ICONWARNING);
        return;
    }
    state.pendingJourneyScript.reset();
    PopulateTracks(state, SelectedIndex(GetDlgItem(state.window, TrackList)));
    NotifyPreview(state);
}

void UpdateSelectedKeyframe(State& state) {
    AnimationTrack* track = SelectedTrack(state);
    if (!track) return;
    const int keyIndex = SelectedIndex(GetDlgItem(state.window, KeyframeList));
    if (keyIndex < 0 || keyIndex >= static_cast<int>(track->keyframes.size())) return;
    const std::string keyframeId = track->keyframes[static_cast<std::size_t>(keyIndex)].id;
    const std::string timeText = Utf8(Read(GetDlgItem(state.window, KeyframeTimeEdit)));
    double timeSeconds = 0.0;
    try {
        std::size_t consumed = 0U;
        timeSeconds = std::stod(timeText, &consumed);
        if (consumed != timeText.size()) throw std::invalid_argument("trailing");
    } catch (...) {
        MessageBoxW(state.window, L"Keyframe time must be a finite number of seconds.",
                    L"Animation Timeline", MB_OK | MB_ICONWARNING);
        PopulateSelectedKeyframeControls(state);
        return;
    }
    if (!std::isfinite(timeSeconds) || timeSeconds < 0.0 ||
        timeSeconds > state.candidate.durationSeconds) {
        MessageBoxW(state.window, L"Keyframe time must be within the timeline duration.",
                    L"Animation Timeline", MB_OK | MB_ICONWARNING);
        PopulateSelectedKeyframeControls(state);
        return;
    }

    AnimationInterpolation interpolation = SelectedInterpolation(state);
    const auto target = AnimationTargetFromStableName(track->target);
    if (target) {
        const auto descriptors = GeneralAnimationTargetDescriptors();
        const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(),
            [&](const AnimationTargetDescriptor& item) { return item.key == *target; });
        if (descriptor != descriptors.end() && descriptor->valueType == AnimationValueType::Integer) {
            interpolation = AnimationInterpolation::Step;
        }
    }

    AnimationTimeline candidate = state.candidate;
    const int trackIndex = SelectedIndex(GetDlgItem(state.window, TrackList));
    auto& candidateTrack = candidate.tracks[static_cast<std::size_t>(trackIndex)];
    auto& candidateKeyframe = candidateTrack.keyframes[static_cast<std::size_t>(keyIndex)];
    candidateKeyframe.timeSeconds = timeSeconds;
    candidateKeyframe.interpolation = interpolation;
    std::sort(candidateTrack.keyframes.begin(), candidateTrack.keyframes.end(),
              [](const AnimationKeyframe& left, const AnimationKeyframe& right) {
                  return left.timeSeconds < right.timeSeconds;
              });
    const auto validation = ValidateGeneralAnimationTimeline(candidate);
    if (!validation.valid) {
        MessageBoxW(state.window, Wide(validation.issues.front().message).c_str(),
                    L"Animation Timeline", MB_OK | MB_ICONWARNING);
        PopulateSelectedKeyframeControls(state);
        return;
    }
    state.candidate = std::move(candidate);
    state.pendingJourneyScript.reset();
    PopulateTracks(state, trackIndex);
    const AnimationTrack* refreshed = SelectedTrack(state);
    if (refreshed) {
        const auto selectedKeyframe = std::find_if(
            refreshed->keyframes.begin(), refreshed->keyframes.end(),
            [&](const AnimationKeyframe& keyframe) { return keyframe.id == keyframeId; });
        if (selectedKeyframe != refreshed->keyframes.end()) {
            SendMessageW(GetDlgItem(state.window, KeyframeList), LB_SETCURSEL,
                         static_cast<WPARAM>(std::distance(refreshed->keyframes.begin(), selectedKeyframe)), 0);
        }
    }
    PopulateSelectedKeyframeControls(state);
    NotifyPreview(state);
}

void AddCurrentKeyframe(State& state) {
    AnimationTrack* track = SelectedTrack(state);
    if (!track) return;
    const auto target = AnimationTargetFromStableName(track->target);
    AnimationInterpolation interpolation = SelectedInterpolation(state);
    if (target) {
        const auto descriptors = GeneralAnimationTargetDescriptors();
        const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(),
            [&](const AnimationTargetDescriptor& item) { return item.key == *target; });
        if (descriptor != descriptors.end() && descriptor->valueType == AnimationValueType::Integer) {
            interpolation = AnimationInterpolation::Step;
        }
    }
    const int selectedTrackIndex = SelectedIndex(GetDlgItem(state.window, TrackList));
    std::string error;
    if (!AddGeneralAnimationKeyframeFromPreset(
            state.candidate, track->id, NewStableId(state, "key-"),
            state.clocks->Time(AnimationClockDomain::Preview),
            state.basePreset, interpolation, error)) {
        MessageBoxW(state.window, Wide(error).c_str(), L"Animation Timeline",
                    MB_OK | MB_ICONWARNING);
        return;
    }
    state.pendingJourneyScript.reset();
    PopulateTracks(state, selectedTrackIndex);
    const AnimationTrack* refreshed = SelectedTrack(state);
    if (refreshed) {
        const auto found = std::find_if(refreshed->keyframes.begin(), refreshed->keyframes.end(),
            [&](const AnimationKeyframe& keyframe) {
                return keyframe.timeSeconds == state.clocks->Time(AnimationClockDomain::Preview);
            });
        if (found != refreshed->keyframes.end()) {
            SendMessageW(GetDlgItem(state.window, KeyframeList), LB_SETCURSEL,
                         static_cast<WPARAM>(std::distance(refreshed->keyframes.begin(), found)), 0);
        }
    }
    NotifyPreview(state);
}

void RemoveKeyframe(State& state) {
    AnimationTrack* track = SelectedTrack(state);
    if (!track) return;
    const int keyIndex = SelectedIndex(GetDlgItem(state.window, KeyframeList));
    if (keyIndex < 0 || keyIndex >= static_cast<int>(track->keyframes.size())) return;
    track->keyframes.erase(track->keyframes.begin() + keyIndex);
    state.pendingJourneyScript.reset();
    const int selectedTrackIndex = SelectedIndex(GetDlgItem(state.window, TrackList));
    PopulateTracks(state, selectedTrackIndex);
    NotifyPreview(state);
}

void ConvertJourney(State& state) {
    AnimationTimeline converted;
    std::string error;
    if (!ConvertJourneyToGeneralAnimation(state.basePreset, converted, error)) {
        MessageBoxW(state.window, Wide(error).c_str(), L"Journey to Tracks",
                    MB_OK | MB_ICONWARNING);
        return;
    }
    state.candidate = std::move(converted);
    state.pendingJourneyScript.reset();
    state.clocks->Reset(AnimationClockDomain::Preview);
    SetText(GetDlgItem(state.window, DurationEdit), FormatDouble(state.candidate.durationSeconds));
    SendMessageW(GetDlgItem(state.window, LoopModeCombo), CB_SETCURSEL,
                 static_cast<WPARAM>(state.candidate.loopMode), 0);
    PopulateTracks(state);
    UpdateScrubberFromClock(state);
    NotifyPreview(state);
}

void PrepareJourney(State& state) {
    std::string script;
    std::string error;
    if (!ConvertGeneralAnimationToJourney(state.basePreset, state.candidate, script, error)) {
        state.pendingJourneyScript.reset();
        MessageBoxW(state.window, Wide(error).c_str(), L"Tracks to Journey",
                    MB_OK | MB_ICONWARNING);
        RefreshValidation(state);
        return;
    }
    state.pendingJourneyScript = std::move(script);
    RefreshValidation(state);
    MessageBoxW(state.window,
                L"A lossless camera-only Journey update is prepared. It will be applied only when you choose OK.",
                L"Tracks to Journey", MB_OK | MB_ICONINFORMATION);
}

void TogglePlayback(State& state) {
    state.playing = !state.playing;
    state.lastPlaybackTick = std::chrono::steady_clock::now();
    SetWindowTextW(GetDlgItem(state.window, PlayPauseButton),
                   state.playing ? L"Pause" : L"Play");
}

void PlaybackTick(State& state) {
    if (!state.playing) return;
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - state.lastPlaybackTick).count();
    state.lastPlaybackTick = now;
    std::string error;
    if (!state.clocks->Advance(AnimationClockDomain::Preview,
                               std::clamp(elapsed, 0.0, 0.25), error)) {
        state.playing = false;
        SetWindowTextW(GetDlgItem(state.window, PlayPauseButton), L"Play");
        SetValidation(state, error, true);
        return;
    }
    if (state.candidate.loopMode == AnimationLoopMode::Clamp &&
        state.clocks->Time(AnimationClockDomain::Preview) >= state.candidate.durationSeconds) {
        (void)state.clocks->SetTime(AnimationClockDomain::Preview,
                                    state.candidate.durationSeconds, error);
        state.playing = false;
        SetWindowTextW(GetDlgItem(state.window, PlayPauseButton), L"Play");
    }
    UpdateScrubberFromClock(state);
    NotifyPreview(state);
}

void BuildControls(State& state) {
    auto add = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int id,
                   int x, int y, int width, int height, DWORD exStyle = 0) {
        HWND control = CreateWindowExW(exStyle, cls, text,
            WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style),
            x, y, width, height, state.window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), state.instance, nullptr);
        if (control) SendMessageW(control, WM_SETFONT,
                                  reinterpret_cast<WPARAM>(state.font), TRUE);
        return control;
    };

    add(WC_STATICW, L"Tracks", SS_LEFT, 0, 14, 12, 340, 22);
    add(WC_LISTBOXW, L"", LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP,
        TrackList, 14, 36, 340, 220, WS_EX_CLIENTEDGE);
    add(WC_STATICW, L"Target", SS_LEFT, 0, 14, 264, 100, 22);
    HWND target = add(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                      TargetCombo, 14, 286, 340, 260);
    for (const auto& descriptor : GeneralAnimationTargetDescriptors()) {
        const std::wstring name = Wide(std::string(descriptor.stableName));
        SendMessageW(target, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }
    SendMessageW(target, CB_SETCURSEL, 0, 0);
    add(WC_BUTTONW, L"Add Track", BS_PUSHBUTTON | WS_TABSTOP,
        AddTrackButton, 14, 320, 104, 30);
    add(WC_BUTTONW, L"Remove Track", BS_PUSHBUTTON | WS_TABSTOP,
        RemoveTrackButton, 126, 320, 112, 30);
    add(WC_BUTTONW, L"Enabled", BS_AUTOCHECKBOX | WS_TABSTOP,
        TrackEnabledCheck, 246, 320, 108, 30);

    add(WC_STATICW, L"Keyframes: time | value | interpolation", SS_LEFT, 0,
        370, 12, 500, 22);
    add(WC_LISTBOXW, L"", LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
        KeyframeList, 370, 36, 500, 220, WS_EX_CLIENTEDGE);
    add(WC_STATICW, L"Selected time", SS_LEFT, 0, 370, 264, 104, 22);
    add(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP,
        KeyframeTimeEdit, 370, 286, 104, 28, WS_EX_CLIENTEDGE);
    add(WC_STATICW, L"Interpolation", SS_LEFT, 0, 484, 264, 166, 22);
    HWND interpolation = add(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
        InterpolationCombo, 484, 286, 166, 160);
    for (const wchar_t* item : {L"Step", L"Linear", L"Smoothstep"}) {
        SendMessageW(interpolation, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    }
    SendMessageW(interpolation, CB_SETCURSEL, 1, 0);
    add(WC_BUTTONW, L"Update Keyframe", BS_PUSHBUTTON | WS_TABSTOP,
        UpdateKeyframeButton, 660, 286, 126, 30);
    add(WC_BUTTONW, L"Add Current Value", BS_PUSHBUTTON | WS_TABSTOP,
        AddCurrentButton, 370, 320, 156, 30);
    add(WC_BUTTONW, L"Remove Keyframe", BS_PUSHBUTTON | WS_TABSTOP,
        RemoveKeyframeButton, 536, 320, 142, 30);

    add(WC_STATICW, L"Duration (seconds)", SS_LEFT, 0, 14, 368, 150, 22);
    add(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP,
        DurationEdit, 164, 364, 118, 28, WS_EX_CLIENTEDGE);
    add(WC_STATICW, L"Loop mode", SS_LEFT, 0, 296, 368, 90, 22);
    HWND loop = add(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                    LoopModeCombo, 386, 364, 134, 140);
    for (const wchar_t* item : {L"Clamp", L"Loop", L"PingPong"}) {
        SendMessageW(loop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    }
    add(TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP,
        ScrubberTrack, 14, 406, 642, 34);
    SendMessageW(GetDlgItem(state.window, ScrubberTrack), TBM_SETRANGE,
                 TRUE, MAKELPARAM(0, kScrubberMaximum));
    add(WC_STATICW, L"Time: 0 / 1 s", SS_LEFT, TimeLabel,
        666, 408, 204, 28);
    add(WC_BUTTONW, L"Play", BS_PUSHBUTTON | WS_TABSTOP,
        PlayPauseButton, 666, 442, 96, 30);
    add(WC_BUTTONW, L"Stop", BS_PUSHBUTTON | WS_TABSTOP,
        StopButton, 774, 442, 96, 30);

    add(WC_BUTTONW, L"Journey → Tracks", BS_PUSHBUTTON | WS_TABSTOP,
        JourneyToTracksButton, 14, 452, 160, 32);
    add(WC_BUTTONW, L"Tracks → Journey", BS_PUSHBUTTON | WS_TABSTOP,
        TracksToJourneyButton, 184, 452, 160, 32);
    add(WC_STATICW,
        L"Timeline data is runtime-only in PH07. Cancel restores the prior preview and does not change the project.",
        SS_LEFT, ValidationLabel, 14, 496, 856, 48);
    add(WC_BUTTONW, L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP,
        OkButton, 646, 556, 104, 32);
    add(WC_BUTTONW, L"Cancel", BS_PUSHBUTTON | WS_TABSTOP,
        CancelButton, 766, 556, 104, 32);
    SendMessageW(state.window, DM_SETDEFID, OkButton, 0);
}

LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<State*>(create->lpCreateParams);
        state->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(window, message, wParam, lParam);

    switch (message) {
    case WM_CREATE:
        state->font = CreateResponsiveDialogFont(state->dpi);
        BuildControls(*state);
        SetText(GetDlgItem(window, DurationEdit), FormatDouble(state->candidate.durationSeconds));
        SendMessageW(GetDlgItem(window, LoopModeCombo), CB_SETCURSEL,
                     static_cast<WPARAM>(state->candidate.loopMode), 0);
        PopulateTracks(*state);
        UpdateScrubberFromClock(*state);
        state->layout.Initialise(window, state->dpi, state->font, 760, 520);
        state->layout.Focus(GetDlgItem(window, TrackList));
        SetTimer(window, kPlaybackTimerId, 30, nullptr);
        NotifyPreview(*state);
        return 0;
    case WM_TIMER:
        if (wParam == kPlaybackTimerId) PlaybackTick(*state);
        return 0;
    case WM_GETMINMAXINFO:
        state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;
    case WM_SIZE:
        state->layout.OnSize();
        return 0;
    case WM_VSCROLL:
        if (lParam == reinterpret_cast<LPARAM>(GetDlgItem(window, ScrubberTrack))) {
            SetClockFromScrubber(*state);
            return 0;
        }
        if (lParam == 0 && state->layout.OnScroll(message, wParam)) return 0;
        break;
    case WM_HSCROLL:
        if (lParam == reinterpret_cast<LPARAM>(GetDlgItem(window, ScrubberTrack))) {
            SetClockFromScrubber(*state);
            return 0;
        }
        if (lParam == 0 && state->layout.OnScroll(message, wParam)) return 0;
        break;
    case WM_MOUSEWHEEL:
        if (state->layout.OnMouseWheel(wParam)) return 0;
        break;
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (id == TrackList && notification == LBN_SELCHANGE) {
            const AnimationTrack* track = SelectedTrack(*state);
            SendMessageW(GetDlgItem(window, TrackEnabledCheck), BM_SETCHECK,
                         track && track->enabled ? BST_CHECKED : BST_UNCHECKED, 0);
            PopulateKeyframes(*state);
        } else if (id == KeyframeList && notification == LBN_SELCHANGE) {
            PopulateSelectedKeyframeControls(*state);
        } else if (id == AddTrackButton) AddTrack(*state);
        else if (id == RemoveTrackButton) RemoveTrack(*state);
        else if (id == TrackEnabledCheck) SetTrackEnabled(*state);
        else if (id == UpdateKeyframeButton) UpdateSelectedKeyframe(*state);
        else if (id == AddCurrentButton) AddCurrentKeyframe(*state);
        else if (id == RemoveKeyframeButton) RemoveKeyframe(*state);
        else if (id == DurationEdit && notification == EN_KILLFOCUS) (void)ApplyDuration(*state, false);
        else if (id == LoopModeCombo && notification == CBN_SELCHANGE) {
            state->candidate.loopMode = static_cast<AnimationLoopMode>(
                std::clamp(SelectedComboIndex(GetDlgItem(window, LoopModeCombo)), 0, 2));
            state->pendingJourneyScript.reset();
            RefreshValidation(*state);
            UpdateScrubberFromClock(*state);
            NotifyPreview(*state);
        } else if (id == PlayPauseButton) TogglePlayback(*state);
        else if (id == StopButton) StopPlayback(*state);
        else if (id == JourneyToTracksButton) ConvertJourney(*state);
        else if (id == TracksToJourneyButton) PrepareJourney(*state);
        else if (id == OkButton) {
            if (!ApplyDuration(*state, true)) return 0;
            const auto validation = ValidateGeneralAnimationTimeline(state->candidate);
            if (!validation.valid) {
                MessageBoxW(window, Wide(validation.issues.front().message).c_str(),
                            L"Animation Timeline", MB_OK | MB_ICONWARNING);
                return 0;
            }
            *state->outputTimeline = state->candidate;
            state->accepted = true;
            DestroyWindow(window);
        } else if (id == CancelButton) {
            DestroyWindow(window);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY: {
        KillTimer(window, kPlaybackTimerId);
        state->playing = false;
        std::string clockError;
        (void)state->clocks->SetTime(AnimationClockDomain::Preview,
                                     state->originalPreviewTime, clockError);
        state->previewCallback(state->originalTimeline,
                               state->originalPreviewTime, false);
        RememberDialogPlacement(window, kAnimationEditorClass, state->dpi);
        state->layout.Shutdown();
        if (state->font) DeleteObject(state->font);
        state->font = nullptr;
        state->done = true;
        if (IsWindow(state->owner)) {
            EnableWindow(state->owner, TRUE);
            SetForegroundWindow(state->owner);
        }
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

GeneralAnimationEditorResult GeneralAnimationEditorDialog::Show(
    HWND owner,
    HINSTANCE instance,
    const Preset& basePreset,
    AnimationTimeline& timeline,
    AnimationClockBank& clocks,
    PreviewCallback previewCallback) {
    WNDCLASSEXW cls{};
    cls.cbSize = sizeof(cls);
    cls.lpfnWndProc = Procedure;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    cls.lpszClassName = kAnimationEditorClass;
    RegisterClassExW(&cls);

    State state;
    state.owner = owner;
    state.instance = instance;
    state.basePreset = basePreset;
    state.originalTimeline = timeline;
    state.candidate = timeline;
    state.outputTimeline = &timeline;
    state.clocks = &clocks;
    state.previewCallback = std::move(previewCallback);
    state.dpi = DialogDpi(owner);
    state.originalPreviewTime = clocks.Time(AnimationClockDomain::Preview);
    if (state.candidate.id.empty()) {
        state.candidate.id = "timeline-main";
        state.candidate.durationSeconds = 10.0;
        state.candidate.loopMode = AnimationLoopMode::Clamp;
    }

    const RECT rect = ResponsiveDialogRect(owner, 900, 640, state.dpi,
                                            kAnimationEditorClass);
    HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        kAnimationEditorClass, L"General Animation Timeline",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        owner, nullptr, instance, &state);
    if (!window) return {};
    EnableWindow(owner, FALSE);

    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, CancelButton, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    GeneralAnimationEditorResult result;
    result.accepted = state.accepted;
    if (state.accepted) result.journeyScript = std::move(state.pendingJourneyScript);
    return result;
}
#endif
} // namespace mw
