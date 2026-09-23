#include "App/FrameSequenceExportDialog.h"

#include "App/DialogSupport.h"
#include "Core/FrameSequenceExport.h"
#include "Core/StillImageRenderer.h"
#include "WindowsIntegration/ImageCodec.h"

#ifdef _WIN32
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <wrl/client.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#ifndef MW_PROJECT_VERSION
#define MW_PROJECT_VERSION "unknown"
#endif

namespace mw {
#ifdef _WIN32
namespace {

using Microsoft::WRL::ComPtr;

constexpr wchar_t kClassName[] = L"MandelbrotFrameSequenceExportDialog";
constexpr UINT kExportProgressMessage = WM_APP + 241U;
constexpr UINT kExportCompleteMessage = WM_APP + 242U;

struct FrameRateChoice {
    const wchar_t* label;
    FrameRate rate;
};

constexpr std::array<FrameRateChoice, 6> kFrameRates{{
    {L"24 fps", {24U, 1U}},
    {L"25 fps", {25U, 1U}},
    {L"29.97 fps (30000/1001)", {30000U, 1001U}},
    {L"30 fps", {30U, 1U}},
    {L"50 fps", {50U, 1U}},
    {L"60 fps", {60U, 1U}},
}};

struct FrameRendererChoice {
    const wchar_t* label;
    const char* rendererId;
};

// The exact path is deliberately a separate user-selected renderer class. It
// must never be substituted for the ordinary CPU renderer during an export.
constexpr std::array<FrameRendererChoice, 5> kFrameRenderers{{
    {L"CPU Float64 compatibility", "cpu-production-still"},
    {L"CPU exact Boost-512 (unrotated, AA 1-4)", "cpu-exact-boost512-v1"},
    {L"CPU exact Boost-2048 (unrotated, AA 1-4)", "cpu-exact-boost2048-v1"},
    {L"CPU exact Boost-8192 (unrotated, AA 1-4)", "cpu-exact-boost8192-v1"},
    {L"CPU exact Boost-16384 (unrotated, AA 1-4)", "cpu-exact-boost16384-v1"},
}};

enum Id : int {
    OutputEdit = 9901,
    BrowseButton,
    WidthEdit,
    HeightEdit,
    DpiEdit,
    FrameRateCombo,
    RendererCombo,
    IncludeEndCheck,
    PrefixEdit,
    ResumeCheck,
    SummaryLabel,
    ProgressBar,
    StatusLabel,
    StartButton,
    CancelButton,
    OpenFolderButton,
    CloseButton,
};

struct State {
    HWND owner{};
    HWND window{};
    HINSTANCE instance{};
    HFONT font{};
    UINT dpi{96U};
    ResponsiveDialogLayout layout;
    Preset baseSnapshot;
    AnimationTimeline timelineSnapshot;
    StaticWallpaperSettings outputSettings;
    std::thread worker;
    std::atomic_bool cancelRequested{false};
    FrameSequenceExportResult workerResult;
    std::string workerError;
    std::filesystem::path completedDirectory;
    bool workerSucceeded{false};
    bool rendering{false};
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

void SetText(HWND control, const std::wstring& value) {
    SetWindowTextW(control, value.c_str());
}

void SetText(HWND control, const std::string& value) {
    SetText(control, Wide(value));
}

bool ParsePositiveUInt32(HWND control, std::uint32_t& value) {
    try {
        const std::wstring text = Read(control);
        std::size_t consumed = 0U;
        const unsigned long parsed = std::stoul(text, &consumed, 10);
        if (consumed != text.size() || parsed == 0UL ||
            parsed > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
            return false;
        }
        value = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

FrameRate SelectedFrameRate(const State& state) {
    int selected = static_cast<int>(SendMessageW(
        GetDlgItem(state.window, FrameRateCombo), CB_GETCURSEL, 0, 0));
    selected = std::clamp(selected, 0, static_cast<int>(kFrameRates.size()) - 1);
    return kFrameRates[static_cast<std::size_t>(selected)].rate;
}

const char* SelectedRendererId(const State& state) {
    int selected = static_cast<int>(SendMessageW(
        GetDlgItem(state.window, RendererCombo), CB_GETCURSEL, 0, 0));
    selected = std::clamp(selected, 0, static_cast<int>(kFrameRenderers.size()) - 1);
    return kFrameRenderers[static_cast<std::size_t>(selected)].rendererId;
}

std::filesystem::path DefaultOutputDirectory() {
    PWSTR pictures = nullptr;
    std::filesystem::path result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, KF_FLAG_DEFAULT,
                                       nullptr, &pictures)) && pictures) {
        result = pictures;
        CoTaskMemFree(pictures);
    } else {
        std::error_code code;
        result = std::filesystem::current_path(code);
    }
    return result / L"Mandelbrot Frame Sequence";
}

std::wstring PickFolder(HWND owner, const std::wstring& initialPath) {
    ComPtr<IFileDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(dialog.GetAddressOf())))) {
        return {};
    }
    DWORD options = 0U;
    if (FAILED(dialog->GetOptions(&options)) ||
        FAILED(dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM |
                                  FOS_PATHMUSTEXIST))) {
        return {};
    }
    dialog->SetTitle(L"Choose Frame Sequence Folder");
    if (!initialPath.empty()) {
        ComPtr<IShellItem> initial;
        if (SUCCEEDED(SHCreateItemFromParsingName(initialPath.c_str(), nullptr,
                                                  IID_PPV_ARGS(initial.GetAddressOf())))) {
            dialog->SetFolder(initial.Get());
        }
    }
    if (FAILED(dialog->Show(owner))) return {};
    ComPtr<IShellItem> selected;
    if (FAILED(dialog->GetResult(selected.GetAddressOf()))) return {};
    PWSTR path = nullptr;
    if (FAILED(selected->GetDisplayName(SIGDN_FILESYSPATH, &path)) || !path) return {};
    std::wstring result(path);
    CoTaskMemFree(path);
    return result;
}

void JoinWorker(State& state) {
    if (state.worker.joinable()) state.worker.join();
}

void SetInputEnabled(State& state, bool enabled) {
    for (const int id : {OutputEdit, BrowseButton, WidthEdit, HeightEdit, DpiEdit,
                         FrameRateCombo, RendererCombo, IncludeEndCheck, PrefixEdit, ResumeCheck,
                         StartButton}) {
        EnableWindow(GetDlgItem(state.window, id), enabled ? TRUE : FALSE);
    }
    EnableWindow(GetDlgItem(state.window, CancelButton), enabled ? FALSE : TRUE);
    EnableWindow(GetDlgItem(state.window, OpenFolderButton),
                 !state.completedDirectory.empty() && enabled ? TRUE : FALSE);
}

void UpdateSummary(State& state) {
    std::string error;
    const bool includeEnd = SendMessageW(GetDlgItem(state.window, IncludeEndCheck),
                                         BM_GETCHECK, 0, 0) == BST_CHECKED;
    const FrameRate rate = SelectedFrameRate(state);
    const std::uint32_t count = FrameCountForDuration(
        state.timelineSnapshot.durationSeconds, rate, includeEnd, error);
    if (count == 0U) {
        SetText(GetDlgItem(state.window, SummaryLabel), error);
        return;
    }
    FrameSequenceExportSettings settings;
    settings.frameRate = rate;
    const double finalTime = FrameTimeForIndex(settings, count - 1U);
    std::wostringstream summary;
    summary << count << L" PNG frames from t=0 to t=" << std::fixed
            << std::setprecision(6) << finalTime << L" s. "
            << L"Adaptive quality is disabled; preset iterations and AA are pinned.";
    const std::string_view rendererId(SelectedRendererId(state));
    if (rendererId == "cpu-exact-boost512-v1" || rendererId == "cpu-exact-boost2048-v1" ||
        rendererId == "cpu-exact-boost8192-v1" || rendererId == "cpu-exact-boost16384-v1") {
        summary << L" Exact CPU rendering requires an unrotated preset with AA 1 through 4.";
    }
    if (state.timelineSnapshot.tracks.empty()) {
        summary << L" The current timeline has no tracks, so every frame is static.";
    }
    SetText(GetDlgItem(state.window, SummaryLabel), summary.str());
}

bool BuildJobFromControls(State& state, FrameSequenceExportJob& job) {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint32_t dpi = 0U;
    if (!ParsePositiveUInt32(GetDlgItem(state.window, WidthEdit), width) ||
        !ParsePositiveUInt32(GetDlgItem(state.window, HeightEdit), height) ||
        !ParsePositiveUInt32(GetDlgItem(state.window, DpiEdit), dpi)) {
        MessageBoxW(state.window,
                    L"Width, height, and DPI must be positive whole numbers.",
                    L"Frame Sequence Export", MB_OK | MB_ICONERROR);
        return false;
    }
    const std::wstring outputText = Read(GetDlgItem(state.window, OutputEdit));
    if (outputText.empty()) {
        MessageBoxW(state.window, L"Choose an output folder.",
                    L"Frame Sequence Export", MB_OK | MB_ICONERROR);
        return false;
    }
    const std::string prefix = Utf8(Read(GetDlgItem(state.window, PrefixEdit)));
    const bool includeEnd = SendMessageW(GetDlgItem(state.window, IncludeEndCheck),
                                         BM_GETCHECK, 0, 0) == BST_CHECKED;
    std::string error;
    const FrameRate rate = SelectedFrameRate(state);
    const std::uint32_t frameCount = FrameCountForDuration(
        state.timelineSnapshot.durationSeconds, rate, includeEnd, error);
    if (frameCount == 0U) {
        MessageBoxW(state.window, Wide(error).c_str(), L"Frame Sequence Export",
                    MB_OK | MB_ICONERROR);
        return false;
    }

    FrameSequenceExportSettings settings;
    settings.width = width;
    settings.height = height;
    settings.dpi = dpi;
    settings.tileWidth = 256U;
    settings.frameRate = rate;
    settings.startTimeSeconds = 0.0;
    settings.frameCount = frameCount;
    settings.seed = 0U;
    settings.rendererId = SelectedRendererId(state);
    settings.applicationVersion = MW_PROJECT_VERSION;
    settings.filePrefix = prefix;
    settings.firstFrameNumber = 0U;
    settings.frameNumberDigits = std::max<std::uint32_t>(
        6U, static_cast<std::uint32_t>(std::to_string(frameCount - 1U).size()));
    settings.scaleQualityToResolution = false;
    if (!BuildFrameSequenceExportJob(state.baseSnapshot, state.timelineSnapshot, settings,
                                     std::filesystem::path(outputText), job, error)) {
        MessageBoxW(state.window, Wide(error).c_str(), L"Frame Sequence Export",
                    MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

void StartExport(State& state) {
    if (state.rendering) return;
    FrameSequenceExportJob job;
    if (!BuildJobFromControls(state, job)) return;
    JoinWorker(state);
    state.cancelRequested.store(false);
    state.workerResult = {};
    state.workerError.clear();
    state.workerSucceeded = false;
    state.completedDirectory.clear();
    state.rendering = true;
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETPOS, 0, 0);
    SetText(GetDlgItem(state.window, StatusLabel),
            L"Preparing immutable export snapshot and manifest...");
    SetInputEnabled(state, false);
    const bool allowResume = SendMessageW(GetDlgItem(state.window, ResumeCheck),
                                          BM_GETCHECK, 0, 0) == BST_CHECKED;
    State* statePointer = &state;
    try {
        state.worker = std::thread([statePointer, job = std::move(job), allowResume]() {
            const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool comInitialised = SUCCEEDED(comResult);
            bool succeeded = false;
            std::string error;
            FrameSequenceExportResult result;
            try {
                if (!comInitialised && comResult != RPC_E_CHANGED_MODE) {
                    error = "Windows Imaging Component could not initialise on the export thread.";
                } else {
                    succeeded = RunFrameSequenceExport(
                    job, allowResume,
                    [statePointer, &job](const FrameSequenceFrameRequest& frame,
                                         const std::filesystem::path& temporaryPath,
                                         std::string& renderError) {
                        WicRowEncoder encoder;
                        if (!encoder.Initialise(temporaryPath, SavedImageFormat::Png,
                                                statePointer->outputSettings.compressionQuality,
                                                job.settings.width, job.settings.height,
                                                job.settings.dpi, renderError)) {
                            return false;
                        }
                        unsigned lastPermille = 0U;
                        const auto writeRow = [&encoder](std::uint32_t,
                                                         std::span<const std::uint32_t> row,
                                                         std::string& writerError) {
                            return encoder.WriteRow(row, writerError);
                        };
                        const auto reportProgress = [statePointer, &job, &frame, &lastPermille](
                                                        const StillRenderProgress& progress) {
                            const std::uint64_t withinFrame = progress.totalRows == 0U
                                ? 0U
                                : static_cast<std::uint64_t>(progress.completedRows) * 1000ULL /
                                      progress.totalRows;
                            const std::uint64_t global =
                                (static_cast<std::uint64_t>(frame.frameIndex) * 1000ULL + withinFrame) /
                                job.settings.frameCount;
                            const unsigned permille = static_cast<unsigned>(
                                std::min<std::uint64_t>(1000ULL, global));
                            if (permille != lastPermille) {
                                lastPermille = permille;
                                PostMessageW(statePointer->window, kExportProgressMessage,
                                             static_cast<WPARAM>(permille),
                                             static_cast<LPARAM>(frame.frameIndex));
                            }
                        };
                        const auto cancelled = [statePointer]() {
                            return statePointer->cancelRequested.load();
                        };
                        bool rendered = false;
                        if (job.settings.rendererId == "cpu-exact-boost512-v1" ||
                            job.settings.rendererId == "cpu-exact-boost2048-v1" ||
                            job.settings.rendererId == "cpu-exact-boost8192-v1" ||
                            job.settings.rendererId == "cpu-exact-boost16384-v1") {
                            if (!frame.framePreset.exactCamera) {
                                renderError = "Exact CPU export requires exact camera state.";
                                return false;
                            }
                            ExactDirectStillRenderRequest request;
                            request.preset = frame.framePreset;
                            request.camera = *frame.framePreset.exactCamera;
                            request.width = job.settings.width;
                            request.height = job.settings.height;
                            request.maximumIterations = std::clamp(
                                frame.framePreset.maximumIterations, 32, 4096);
                            request.precisionBits = frame.precisionPlan.selectedBits;
                            StillRenderResult renderResult;
                            rendered = RenderExactDirectStillImage(
                                request, writeRow, reportProgress, cancelled,
                                renderResult, renderError);
                        } else if (job.settings.rendererId == "cpu-production-still") {
                            StillRenderRequest request;
                            request.preset = frame.framePreset;
                            request.width = job.settings.width;
                            request.height = job.settings.height;
                            request.tileWidth = job.settings.tileWidth;
                            request.previewMaximumWidth = 0U;
                            request.previewMaximumHeight = 0U;
                            request.timeSeconds = frame.timeSeconds;
                            request.scaleQualityToResolution = false;
                            StillRenderResult renderResult;
                            rendered = RenderStillImageTiled(
                                request, writeRow, reportProgress, cancelled,
                                renderResult, renderError);
                        } else {
                            renderError = "The validated frame-export job selected an unimplemented renderer.";
                            return false;
                        }
                        return rendered && !statePointer->cancelRequested.load() &&
                               encoder.Commit(renderError);
                    },
                    [](const std::filesystem::path& path,
                       std::uint32_t expectedWidth,
                       std::uint32_t expectedHeight,
                       std::string& validationError) {
                        return ValidateImageDimensionsWithWic(path, expectedWidth,
                                                              expectedHeight,
                                                              validationError);
                    },
                    [statePointer](const FrameSequenceExportProgress& progress) {
                        const unsigned permille = progress.totalFrames == 0U
                            ? 0U
                            : static_cast<unsigned>(
                                  static_cast<std::uint64_t>(progress.completedFrames) * 1000ULL /
                                  progress.totalFrames);
                        PostMessageW(statePointer->window, kExportProgressMessage,
                                     static_cast<WPARAM>(permille),
                                     static_cast<LPARAM>(progress.activeFrameIndex));
                    },
                        [statePointer]() { return statePointer->cancelRequested.load(); },
                        result, error);
                }
            } catch (const std::bad_alloc&) {
                error = "There is not enough memory to continue the frame-sequence export.";
            } catch (const std::exception& exception) {
                error = std::string("The frame-sequence worker failed: ") + exception.what();
            } catch (...) {
                error = "The frame-sequence worker failed with an unknown error.";
            }
            statePointer->workerSucceeded = succeeded;
            statePointer->workerResult = result;
            statePointer->workerError = error;
            if (succeeded) statePointer->completedDirectory = job.outputDirectory;
            if (comInitialised) CoUninitialize();
            PostMessageW(statePointer->window, kExportCompleteMessage, 0, 0);
        });
    } catch (const std::exception& exception) {
        state.rendering = false;
        state.workerError = std::string("The export thread could not start: ") + exception.what();
        SetInputEnabled(state, true);
        MessageBoxW(state.window, Wide(state.workerError).c_str(),
                    L"Frame Sequence Export", MB_OK | MB_ICONERROR);
    }
}

void CancelExport(State& state) {
    if (!state.rendering) return;
    state.cancelRequested.store(true);
    EnableWindow(GetDlgItem(state.window, CancelButton), FALSE);
    SetText(GetDlgItem(state.window, StatusLabel),
            L"Cancelling after the current bounded render operation...");
}

void CompleteExport(State& state) {
    JoinWorker(state);
    state.rendering = false;
    SetInputEnabled(state, true);
    if (!state.workerSucceeded) {
        SetText(GetDlgItem(state.window, StatusLabel), L"Export failed.");
        MessageBoxW(state.window,
                    Wide(state.workerError.empty() ? "Frame sequence export failed."
                                                   : state.workerError).c_str(),
                    L"Frame Sequence Export", MB_OK | MB_ICONERROR);
        return;
    }
    if (state.workerResult.cancelled) {
        std::wostringstream status;
        status << L"Cancelled. Preserved "
               << state.workerResult.renderedFrames + state.workerResult.resumedFrames
               << L" verified frame(s) and the resumable manifest.";
        SetText(GetDlgItem(state.window, StatusLabel), status.str());
        return;
    }
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETPOS, 1000, 0);
    std::wostringstream status;
    status << L"Complete: " << state.workerResult.renderedFrames << L" rendered, "
           << state.workerResult.resumedFrames << L" resumed. Manifest: "
           << state.workerResult.manifestPath.wstring();
    SetText(GetDlgItem(state.window, StatusLabel), status.str());
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

    add(WC_STATICW, L"Output folder", SS_LEFT, 0, 14, 14, 120, 22);
    add(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, OutputEdit,
        136, 10, 470, 28, WS_EX_CLIENTEDGE);
    add(WC_BUTTONW, L"Browse...", BS_PUSHBUTTON | WS_TABSTOP,
        BrowseButton, 616, 10, 92, 30);

    add(WC_STATICW, L"Width", SS_LEFT, 0, 14, 58, 72, 22);
    add(WC_EDITW, L"1920", ES_AUTOHSCROLL | WS_TABSTOP, WidthEdit,
        86, 54, 92, 28, WS_EX_CLIENTEDGE);
    add(WC_STATICW, L"Height", SS_LEFT, 0, 194, 58, 72, 22);
    add(WC_EDITW, L"1080", ES_AUTOHSCROLL | WS_TABSTOP, HeightEdit,
        266, 54, 92, 28, WS_EX_CLIENTEDGE);
    add(WC_STATICW, L"DPI", SS_LEFT, 0, 374, 58, 50, 22);
    add(WC_EDITW, L"96", ES_AUTOHSCROLL | WS_TABSTOP, DpiEdit,
        424, 54, 82, 28, WS_EX_CLIENTEDGE);
    add(WC_STATICW, L"Frame rate", SS_LEFT, 0, 520, 58, 82, 22);
    HWND rate = add(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                    FrameRateCombo, 602, 54, 106, 180);
    for (const auto& choice : kFrameRates) {
        SendMessageW(rate, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(choice.label));
    }
    SendMessageW(rate, CB_SETCURSEL, 3, 0);

    add(WC_STATICW, L"Renderer", SS_LEFT, 0, 14, 100, 100, 22);
    HWND renderer = add(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                        RendererCombo, 120, 96, 300, 180);
    for (const auto& choice : kFrameRenderers) {
        SendMessageW(renderer, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(choice.label));
    }
    SendMessageW(renderer, CB_SETCURSEL, 0, 0);

    add(WC_STATICW, L"Filename prefix", SS_LEFT, 0, 14, 142, 120, 22);
    add(WC_EDITW, L"frame", ES_AUTOHSCROLL | WS_TABSTOP, PrefixEdit,
        136, 138, 190, 28, WS_EX_CLIENTEDGE);
    add(WC_BUTTONW, L"Include aligned end frame", BS_AUTOCHECKBOX | WS_TABSTOP,
        IncludeEndCheck, 346, 136, 184, 30);
    SendMessageW(GetDlgItem(state.window, IncludeEndCheck), BM_SETCHECK,
                 BST_CHECKED, 0);
    add(WC_BUTTONW, L"Resume matching manifest", BS_AUTOCHECKBOX | WS_TABSTOP,
        ResumeCheck, 536, 136, 172, 30);
    SendMessageW(GetDlgItem(state.window, ResumeCheck), BM_SETCHECK,
                 BST_CHECKED, 0);

    add(WC_STATICW, L"", SS_LEFT, SummaryLabel, 14, 180, 694, 48);
    add(PROGRESS_CLASSW, L"", PBS_SMOOTH, ProgressBar, 14, 236, 694, 24);
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETRANGE32, 0, 1000);
    add(WC_STATICW,
        L"PNG sequence export uses the selected CPU renderer and a bounded scanline encoder.",
        SS_LEFT, StatusLabel, 14, 272, 694, 74);

    add(WC_BUTTONW, L"Start Export", BS_DEFPUSHBUTTON | WS_TABSTOP,
        StartButton, 250, 366, 110, 32);
    add(WC_BUTTONW, L"Cancel", BS_PUSHBUTTON | WS_TABSTOP,
        CancelButton, 370, 366, 92, 32);
    add(WC_BUTTONW, L"Open Folder", BS_PUSHBUTTON | WS_TABSTOP,
        OpenFolderButton, 472, 366, 106, 32);
    add(WC_BUTTONW, L"Close", BS_PUSHBUTTON | WS_TABSTOP,
        CloseButton, 588, 366, 120, 32);
    SendMessageW(state.window, DM_SETDEFID, StartButton, 0);
    EnableWindow(GetDlgItem(state.window, CancelButton), FALSE);
    EnableWindow(GetDlgItem(state.window, OpenFolderButton), FALSE);
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
        SetText(GetDlgItem(window, OutputEdit), DefaultOutputDirectory().wstring());
        UpdateSummary(*state);
        state->layout.Initialise(window, state->dpi, state->font, 650, 392);
        state->layout.Focus(GetDlgItem(window, OutputEdit));
        return 0;
    case WM_GETMINMAXINFO:
        state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;
    case WM_SIZE:
        state->layout.OnSize();
        return 0;
    case WM_VSCROLL:
    case WM_HSCROLL:
        if (lParam == 0 && state->layout.OnScroll(message, wParam)) return 0;
        break;
    case WM_MOUSEWHEEL:
        if (state->layout.OnMouseWheel(wParam)) return 0;
        break;
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (id == BrowseButton) {
            const std::wstring selected = PickFolder(window, Read(GetDlgItem(window, OutputEdit)));
            if (!selected.empty()) SetText(GetDlgItem(window, OutputEdit), selected);
        } else if ((id == FrameRateCombo || id == RendererCombo) && notification == CBN_SELCHANGE) {
            UpdateSummary(*state);
        } else if (id == IncludeEndCheck) {
            UpdateSummary(*state);
        } else if (id == StartButton) {
            StartExport(*state);
        } else if (id == CancelButton) {
            CancelExport(*state);
        } else if (id == OpenFolderButton && !state->completedDirectory.empty()) {
            ShellExecuteW(window, L"open", state->completedDirectory.c_str(),
                          nullptr, nullptr, SW_SHOWNORMAL);
        } else if (id == CloseButton) {
            if (state->rendering) CancelExport(*state);
            else DestroyWindow(window);
        }
        return 0;
    }
    case kExportProgressMessage: {
        const unsigned permille = static_cast<unsigned>(
            std::min<WPARAM>(wParam, static_cast<WPARAM>(1000U)));
        SendMessageW(GetDlgItem(window, ProgressBar), PBM_SETPOS,
                     static_cast<WPARAM>(permille), 0);
        std::wostringstream status;
        status << L"Rendering frame " << static_cast<std::uint32_t>(lParam) + 1U
               << L" of the immutable sequence...";
        SetText(GetDlgItem(window, StatusLabel), status.str());
        return 0;
    }
    case kExportCompleteMessage:
        CompleteExport(*state);
        return 0;
    case WM_CLOSE:
        if (state->rendering) CancelExport(*state);
        else DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        state->cancelRequested.store(true);
        JoinWorker(*state);
        RememberDialogPlacement(window, kClassName, state->dpi);
        state->layout.Shutdown();
        if (state->font) DeleteObject(state->font);
        state->font = nullptr;
        state->done = true;
        if (IsWindow(state->owner)) {
            EnableWindow(state->owner, TRUE);
            SetForegroundWindow(state->owner);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

void FrameSequenceExportDialog::Show(HWND owner,
                                     HINSTANCE instance,
                                     const Preset& baseSnapshot,
                                     const AnimationTimeline& timelineSnapshot,
                                     const StaticWallpaperSettings& outputSettings) {
    WNDCLASSEXW cls{};
    cls.cbSize = sizeof(cls);
    cls.lpfnWndProc = Procedure;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    cls.lpszClassName = kClassName;
    RegisterClassExW(&cls);

    State state;
    state.owner = owner;
    state.instance = instance;
    state.baseSnapshot = baseSnapshot;
    state.timelineSnapshot = timelineSnapshot;
    state.outputSettings = outputSettings;
    state.dpi = DialogDpi(owner);
    if (state.timelineSnapshot.id.empty()) {
        state.timelineSnapshot.id = "timeline-export-static";
        state.timelineSnapshot.durationSeconds = 10.0;
        state.timelineSnapshot.loopMode = AnimationLoopMode::Clamp;
    }

    const RECT rect = ResponsiveDialogRect(owner, 730, 452, state.dpi, kClassName);
    HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        kClassName, L"Deterministic PNG Frame Sequence",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
            WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        owner, nullptr, instance, &state);
    if (!window) return;
    EnableWindow(owner, FALSE);

    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, CloseButton, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}
#endif
} // namespace mw
