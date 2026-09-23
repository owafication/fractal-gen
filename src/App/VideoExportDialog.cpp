#include "App/VideoExportDialog.h"

#include "App/DialogSupport.h"
#include "Core/ExternalVideoExport.h"
#include "WindowsIntegration/ExternalProcess.h"

#ifdef _WIN32
#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <wrl/client.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mw {
#ifdef _WIN32
namespace {

using Microsoft::WRL::ComPtr;

constexpr wchar_t kClassName[] = L"MandelbrotExternalVideoExportDialog";
constexpr UINT kVideoProgressMessage = WM_APP + 251U;
constexpr UINT kVideoStatusMessage = WM_APP + 252U;
constexpr UINT kVideoCompleteMessage = WM_APP + 253U;
constexpr std::uint32_t kProbeTimeoutMilliseconds = 10000U;

constexpr std::array<const wchar_t*, 9U> kEncoderPresets{
    L"ultrafast", L"superfast", L"veryfast", L"faster", L"fast",
    L"medium", L"slow", L"slower", L"veryslow"};

enum Id : int {
    SequenceEdit = 10101,
    SequenceBrowseButton,
    FfmpegEdit,
    FfmpegBrowseButton,
    DetectButton,
    OutputEdit,
    OutputBrowseButton,
    PresetCombo,
    CrfEdit,
    CleanupCheck,
    SummaryLabel,
    ProgressBar,
    StatusLabel,
    StartButton,
    CancelButton,
    OpenOutputButton,
    CloseButton,
};

enum class WorkerStatus : WPARAM {
    CheckingFfmpeg = 1U,
    VerifyingFrames = 2U,
    Encoding = 3U,
    VerifyingVideo = 4U,
};

struct State {
    HWND owner{};
    HWND window{};
    HINSTANCE instance{};
    HFONT font{};
    UINT dpi{96U};
    ResponsiveDialogLayout layout;
    std::thread worker;
    std::atomic_bool cancelRequested{false};
    ExternalVideoExportResult workerResult;
    std::string workerError;
    std::string encoderVersion;
    bool workerSucceeded{false};
    bool running{false};
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

class ComApartment {
public:
    ComApartment() noexcept {
        result_ = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        owns_ = result_ == S_OK || result_ == S_FALSE;
    }
    ~ComApartment() {
        if (owns_) CoUninitialize();
    }
    [[nodiscard]] bool Available() const noexcept {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }

private:
    HRESULT result_{E_FAIL};
    bool owns_{false};
};

std::wstring PickFolder(HWND owner, const std::wstring& initialPath) {
    ComApartment apartment;
    if (!apartment.Available()) return {};
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
    dialog->SetTitle(L"Choose Verified PNG Frame Sequence");
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

std::wstring PickExecutable(HWND owner, const std::wstring& initialPath) {
    std::array<wchar_t, 32768U> buffer{};
    wcsncpy_s(buffer.data(), buffer.size(), initialPath.c_str(), _TRUNCATE);
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"FFmpeg executable (ffmpeg.exe)\0ffmpeg.exe\0Executables (*.exe)\0*.exe\0All files\0*.*\0\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrTitle = L"Choose External FFmpeg Executable";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) return {};
    return buffer.data();
}

std::wstring PickOutput(HWND owner, const std::wstring& initialPath) {
    std::array<wchar_t, 32768U> buffer{};
    wcsncpy_s(buffer.data(), buffer.size(), initialPath.c_str(), _TRUNCATE);
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"MP4 video (*.mp4)\0*.mp4\0\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrDefExt = L"mp4";
    dialog.lpstrTitle = L"Choose New MP4 Output";
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) return {};
    return buffer.data();
}

std::wstring DiscoverFfmpegOnPath() {
    const DWORD required = SearchPathW(nullptr, L"ffmpeg.exe", nullptr, 0U, nullptr, nullptr);
    if (required == 0U || required > 32767U) return {};
    std::vector<wchar_t> buffer(static_cast<std::size_t>(required) + 1U, L'\0');
    const DWORD written = SearchPathW(nullptr, L"ffmpeg.exe", nullptr,
                                      static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
    if (written == 0U || written >= static_cast<DWORD>(buffer.size())) return {};
    return buffer.data();
}

void JoinWorker(State& state) {
    if (state.worker.joinable()) state.worker.join();
}

void SetInputEnabled(State& state, bool enabled) {
    for (const int id : {SequenceEdit, SequenceBrowseButton, FfmpegEdit,
                         FfmpegBrowseButton, DetectButton, OutputEdit,
                         OutputBrowseButton, PresetCombo, CrfEdit, CleanupCheck,
                         StartButton}) {
        EnableWindow(GetDlgItem(state.window, id), enabled ? TRUE : FALSE);
    }
    EnableWindow(GetDlgItem(state.window, CancelButton), enabled ? FALSE : TRUE);
    EnableWindow(GetDlgItem(state.window, OpenOutputButton),
                 enabled && !state.workerResult.outputPath.empty() ? TRUE : FALSE);
}

void UpdateSummary(State& state) {
    const std::wstring sequence = Read(GetDlgItem(state.window, SequenceEdit));
    std::wostringstream summary;
    summary << L"Uses a complete PH08 manifest, libx264 H.264, yuv420p, and MP4. "
            << L"FFmpeg is external and is never invoked through a command shell.";
    if (!sequence.empty()) {
        FrameSequenceManifest manifest;
        std::string error;
        if (LoadFrameSequenceManifest(
                std::filesystem::path(sequence) / L".mw-frame-sequence" / L"manifest.json",
                manifest, error)) {
            summary << L" Sequence: " << manifest.frameCount << L" frames at "
                    << manifest.frameRate.numerator << L"/"
                    << manifest.frameRate.denominator << L" fps, "
                    << manifest.width << L"x" << manifest.height << L".";
        }
    }
    SetText(GetDlgItem(state.window, SummaryLabel), summary.str());
}

bool ParseCrf(HWND control, std::uint32_t& value) {
    try {
        const std::wstring text = Read(control);
        std::size_t consumed = 0U;
        const unsigned long parsed = std::stoul(text, &consumed, 10);
        if (consumed != text.size() || parsed > 51UL) return false;
        value = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool ReadSettings(State& state, ExternalVideoExportSettings& settings) {
    settings = {};
    settings.frameSequenceDirectory = Read(GetDlgItem(state.window, SequenceEdit));
    settings.ffmpegExecutable = Read(GetDlgItem(state.window, FfmpegEdit));
    settings.outputPath = Read(GetDlgItem(state.window, OutputEdit));
    if (settings.frameSequenceDirectory.empty() || settings.ffmpegExecutable.empty() ||
        settings.outputPath.empty()) {
        MessageBoxW(state.window,
                    L"Choose a verified frame-sequence folder, FFmpeg executable, and MP4 output.",
                    L"External Video Export", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!ParseCrf(GetDlgItem(state.window, CrfEdit), settings.crf)) {
        MessageBoxW(state.window, L"CRF must be a whole number from 0 to 51.",
                    L"External Video Export", MB_OK | MB_ICONERROR);
        return false;
    }
    int selected = static_cast<int>(SendMessageW(GetDlgItem(state.window, PresetCombo),
                                                  CB_GETCURSEL, 0, 0));
    selected = std::clamp(selected, 0, static_cast<int>(kEncoderPresets.size()) - 1);
    settings.encoderPreset = Utf8(kEncoderPresets[static_cast<std::size_t>(selected)]);
    settings.cleanupFramesAfterSuccess =
        SendMessageW(GetDlgItem(state.window, CleanupCheck), BM_GETCHECK, 0, 0) == BST_CHECKED;
    return true;
}

bool ProbeCommand(const std::filesystem::path& executable,
                  const std::vector<std::wstring>& arguments,
                  const std::function<bool()>& cancellation,
                  std::string& output,
                  std::string& error) {
    ExternalProcessResult result;
    if (!RunOwnedProcess(executable, arguments, cancellation, {}, result, error,
                         kProbeTimeoutMilliseconds)) {
        return false;
    }
    if (result.cancelled) {
        error = "FFmpeg capability checking was cancelled.";
        return false;
    }
    if (result.exitCode != 0) {
        error = "FFmpeg capability checking failed.";
        return false;
    }
    output = result.standardOutput;
    output.append("\n");
    output.append(result.standardError);
    return true;
}

bool ProbeFfmpeg(const std::filesystem::path& executable,
                 const std::function<bool()>& cancellation,
                 ExternalEncoderCapabilities& capabilities,
                 std::string& error) {
    std::string version;
    std::string encoder;
    std::string muxer;
    if (!ProbeCommand(executable, {L"-version"}, cancellation, version, error) ||
        !ProbeCommand(executable, {L"-hide_banner", L"-h", L"encoder=libx264"},
                      cancellation, encoder, error) ||
        !ProbeCommand(executable, {L"-hide_banner", L"-h", L"muxer=mp4"},
                      cancellation, muxer, error)) {
        return false;
    }
    return ParseFfmpegCapabilities(version, encoder, muxer, capabilities, error);
}

void StartExport(State& state) {
    if (state.running) return;
    ExternalVideoExportSettings settings;
    if (!ReadSettings(state, settings)) return;
    JoinWorker(state);
    state.cancelRequested.store(false);
    state.workerResult = {};
    state.workerError.clear();
    state.encoderVersion.clear();
    state.workerSucceeded = false;
    state.running = true;
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETPOS, 0, 0);
    SetText(GetDlgItem(state.window, StatusLabel),
            L"Checking the external FFmpeg executable...");
    SetInputEnabled(state, false);
    State* statePointer = &state;
    try {
        state.worker = std::thread([statePointer, settings = std::move(settings)]() mutable {
            bool succeeded = false;
            std::string error;
            ExternalVideoExportResult result;
            try {
                const auto cancellation = [statePointer]() {
                    return statePointer->cancelRequested.load();
                };
                PostMessageW(statePointer->window, kVideoStatusMessage,
                             static_cast<WPARAM>(WorkerStatus::CheckingFfmpeg), 0);
                ExternalEncoderCapabilities capabilities;
                if (!ProbeFfmpeg(settings.ffmpegExecutable, cancellation,
                                 capabilities, error)) {
                    // Error already describes the failed capability boundary.
                } else {
                    statePointer->encoderVersion = capabilities.versionLine;
                    settings.encoderVersionLine = capabilities.versionLine;
                    PostMessageW(statePointer->window, kVideoStatusMessage,
                                 static_cast<WPARAM>(WorkerStatus::VerifyingFrames), 0);
                    FrameSequenceManifest manifest;
                    if (LoadVerifiedFrameSequence(settings.frameSequenceDirectory,
                                                  manifest, error, cancellation)) {
                        ExternalVideoExportJob job;
                        if (BuildExternalVideoExportJob(manifest, settings, job, error)) {
                            PostMessageW(statePointer->window, kVideoStatusMessage,
                                         static_cast<WPARAM>(WorkerStatus::Encoding), 0);
                            succeeded = RunExternalVideoExport(
                                job,
                                [statePointer, &job](const std::filesystem::path& executable,
                                   const std::vector<std::wstring>& arguments,
                                   const std::function<bool()>& cancel,
                                   const std::function<void(std::string_view)>& stdoutChunk,
                                   ExternalProcessResult& processResult,
                                   std::string& processError) {
                                    if (arguments == job.verifyArguments) {
                                        PostMessageW(statePointer->window, kVideoStatusMessage,
                                                     static_cast<WPARAM>(WorkerStatus::VerifyingVideo), 0);
                                    }
                                    return RunOwnedProcess(executable, arguments, cancel,
                                                           stdoutChunk, processResult,
                                                           processError, 0U);
                                },
                                cancellation,
                                [statePointer](const ExternalVideoExportProgress& progress) {
                                    const unsigned permille = progress.totalFrames == 0U
                                        ? 0U
                                        : static_cast<unsigned>(
                                              static_cast<std::uint64_t>(progress.encodedFrames) *
                                              1000ULL / progress.totalFrames);
                                    PostMessageW(statePointer->window, kVideoProgressMessage,
                                                 static_cast<WPARAM>(permille),
                                                 static_cast<LPARAM>(progress.encodedFrames));
                                }, result, error);
                        }
                    }
                }
            } catch (const std::bad_alloc&) {
                error = "There is not enough memory to continue external video encoding.";
            } catch (const std::exception& exception) {
                error = std::string("The external video worker failed: ") + exception.what();
            } catch (...) {
                error = "The external video worker failed with an unknown error.";
            }
            statePointer->workerSucceeded = succeeded;
            statePointer->workerResult = result;
            statePointer->workerError = error;
            PostMessageW(statePointer->window, kVideoCompleteMessage, 0, 0);
        });
    } catch (const std::exception& exception) {
        state.running = false;
        state.workerError = std::string("The video export thread could not start: ") + exception.what();
        SetInputEnabled(state, true);
        MessageBoxW(state.window, Wide(state.workerError).c_str(),
                    L"External Video Export", MB_OK | MB_ICONERROR);
    }
}

void CancelExport(State& state) {
    if (!state.running) return;
    state.cancelRequested.store(true);
    EnableWindow(GetDlgItem(state.window, CancelButton), FALSE);
    SetText(GetDlgItem(state.window, StatusLabel),
            L"Cancelling the owned FFmpeg process; verified PNG frames will be preserved...");
}

void CompleteExport(State& state) {
    JoinWorker(state);
    state.running = false;
    SetInputEnabled(state, true);
    if (!state.workerSucceeded) {
        if (state.workerResult.cancelled || state.cancelRequested.load()) {
            SetText(GetDlgItem(state.window, StatusLabel),
                    L"Cancelled. Source PNG frames and encoder logs were preserved.");
            return;
        }
        SetText(GetDlgItem(state.window, StatusLabel), L"Video encoding failed.");
        MessageBoxW(state.window,
                    Wide(state.workerError.empty() ? "External video export failed."
                                                   : state.workerError).c_str(),
                    L"External Video Export", MB_OK | MB_ICONERROR);
        return;
    }
    if (state.workerResult.cancelled) {
        SetText(GetDlgItem(state.window, StatusLabel),
                L"Cancelled. Source PNG frames and encoder logs were preserved.");
        return;
    }
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETPOS, 1000, 0);
    std::wostringstream status;
    status << L"Verified MP4 complete: " << state.workerResult.outputPath.wstring();
    if (!state.encoderVersion.empty()) status << L"\r\n" << Wide(state.encoderVersion);
    if (state.workerResult.sourceFramesCleaned) {
        status << L"\r\nTracked source frames were removed after successful verification.";
    } else if (!state.workerResult.cleanupWarning.empty()) {
        status << L"\r\nCleanup warning: " << Wide(state.workerResult.cleanupWarning);
    } else {
        status << L"\r\nSource PNG frames were preserved.";
    }
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

    add(WC_STATICW, L"PNG sequence folder", SS_LEFT, 0, 14, 14, 128, 22);
    add(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, SequenceEdit,
        146, 10, 470, 28, WS_EX_CLIENTEDGE);
    add(WC_BUTTONW, L"Browse...", BS_PUSHBUTTON | WS_TABSTOP,
        SequenceBrowseButton, 626, 10, 92, 30);

    add(WC_STATICW, L"FFmpeg executable", SS_LEFT, 0, 14, 56, 128, 22);
    add(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, FfmpegEdit,
        146, 52, 368, 28, WS_EX_CLIENTEDGE);
    add(WC_BUTTONW, L"Browse...", BS_PUSHBUTTON | WS_TABSTOP,
        FfmpegBrowseButton, 524, 52, 92, 30);
    add(WC_BUTTONW, L"Detect PATH", BS_PUSHBUTTON | WS_TABSTOP,
        DetectButton, 626, 52, 92, 30);

    add(WC_STATICW, L"MP4 output", SS_LEFT, 0, 14, 98, 128, 22);
    add(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, OutputEdit,
        146, 94, 470, 28, WS_EX_CLIENTEDGE);
    add(WC_BUTTONW, L"Browse...", BS_PUSHBUTTON | WS_TABSTOP,
        OutputBrowseButton, 626, 94, 92, 30);

    add(WC_STATICW, L"Encoder preset", SS_LEFT, 0, 14, 140, 100, 22);
    HWND preset = add(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                      PresetCombo, 116, 136, 128, 180);
    for (const wchar_t* value : kEncoderPresets) {
        SendMessageW(preset, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(value));
    }
    SendMessageW(preset, CB_SETCURSEL, 5, 0);
    add(WC_STATICW, L"CRF (0–51)", SS_LEFT, 0, 264, 140, 82, 22);
    add(WC_EDITW, L"18", ES_AUTOHSCROLL | WS_TABSTOP, CrfEdit,
        348, 136, 68, 28, WS_EX_CLIENTEDGE);
    add(WC_BUTTONW, L"Delete verified PNGs after success",
        BS_AUTOCHECKBOX | WS_TABSTOP, CleanupCheck, 438, 134, 280, 32);

    add(WC_STATICW, L"", SS_LEFT, SummaryLabel, 14, 180, 704, 66);
    add(PROGRESS_CLASSW, L"", PBS_SMOOTH, ProgressBar, 14, 254, 704, 24);
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETRANGE32, 0, 1000);
    add(WC_STATICW,
        L"Select an external FFmpeg executable. The app does not download, bundle, or remember it.",
        SS_LEFT, StatusLabel, 14, 288, 704, 86);

    add(WC_BUTTONW, L"Encode MP4", BS_DEFPUSHBUTTON | WS_TABSTOP,
        StartButton, 246, 390, 112, 32);
    add(WC_BUTTONW, L"Cancel", BS_PUSHBUTTON | WS_TABSTOP,
        CancelButton, 368, 390, 92, 32);
    add(WC_BUTTONW, L"Open Output", BS_PUSHBUTTON | WS_TABSTOP,
        OpenOutputButton, 470, 390, 110, 32);
    add(WC_BUTTONW, L"Close", BS_PUSHBUTTON | WS_TABSTOP,
        CloseButton, 590, 390, 128, 32);
    SendMessageW(state.window, DM_SETDEFID, StartButton, 0);
    EnableWindow(GetDlgItem(state.window, CancelButton), FALSE);
    EnableWindow(GetDlgItem(state.window, OpenOutputButton), FALSE);
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
    case WM_CREATE: {
        state->font = CreateResponsiveDialogFont(state->dpi);
        BuildControls(*state);
        const std::wstring detected = DiscoverFfmpegOnPath();
        if (!detected.empty()) SetText(GetDlgItem(window, FfmpegEdit), detected);
        UpdateSummary(*state);
        state->layout.Initialise(window, state->dpi, state->font, 690, 420);
        state->layout.Focus(GetDlgItem(window, SequenceEdit));
        return 0;
    }
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
        if (id == SequenceBrowseButton) {
            const std::wstring selected = PickFolder(window, Read(GetDlgItem(window, SequenceEdit)));
            if (!selected.empty()) {
                SetText(GetDlgItem(window, SequenceEdit), selected);
                if (Read(GetDlgItem(window, OutputEdit)).empty()) {
                    SetText(GetDlgItem(window, OutputEdit),
                            (std::filesystem::path(selected) / L"mandelbrot-animation.mp4").wstring());
                }
                UpdateSummary(*state);
            }
        } else if (id == SequenceEdit && notification == EN_CHANGE) {
            UpdateSummary(*state);
        } else if (id == FfmpegBrowseButton) {
            const std::wstring selected = PickExecutable(window, Read(GetDlgItem(window, FfmpegEdit)));
            if (!selected.empty()) SetText(GetDlgItem(window, FfmpegEdit), selected);
        } else if (id == DetectButton) {
            const std::wstring detected = DiscoverFfmpegOnPath();
            if (detected.empty()) {
                MessageBoxW(window, L"ffmpeg.exe was not found on the current process PATH.",
                            L"External Video Export", MB_OK | MB_ICONINFORMATION);
            } else {
                SetText(GetDlgItem(window, FfmpegEdit), detected);
            }
        } else if (id == OutputBrowseButton) {
            const std::wstring selected = PickOutput(window, Read(GetDlgItem(window, OutputEdit)));
            if (!selected.empty()) SetText(GetDlgItem(window, OutputEdit), selected);
        } else if (id == StartButton) {
            StartExport(*state);
        } else if (id == CancelButton) {
            CancelExport(*state);
        } else if (id == OpenOutputButton && !state->workerResult.outputPath.empty()) {
            ShellExecuteW(window, L"open", state->workerResult.outputPath.c_str(),
                          nullptr, nullptr, SW_SHOWNORMAL);
        } else if (id == CloseButton) {
            if (state->running) CancelExport(*state);
            else DestroyWindow(window);
        }
        return 0;
    }
    case kVideoStatusMessage:
        switch (static_cast<WorkerStatus>(wParam)) {
        case WorkerStatus::CheckingFfmpeg:
            SetText(GetDlgItem(window, StatusLabel),
                    L"Checking FFmpeg version, libx264 encoder, and MP4 muxer capabilities...");
            break;
        case WorkerStatus::VerifyingFrames:
            SetText(GetDlgItem(window, StatusLabel),
                    L"Digest-verifying every manifest-tracked PNG source frame...");
            break;
        case WorkerStatus::Encoding:
            SetText(GetDlgItem(window, StatusLabel),
                    L"Encoding to an owned temporary MP4 with a fixed argument vector...");
            break;
        case WorkerStatus::VerifyingVideo:
            SetText(GetDlgItem(window, StatusLabel),
                    L"Decoding the temporary MP4 for verification before promotion...");
            break;
        }
        return 0;
    case kVideoProgressMessage: {
        const unsigned permille = static_cast<unsigned>(
            std::min<WPARAM>(wParam, static_cast<WPARAM>(1000U)));
        SendMessageW(GetDlgItem(window, ProgressBar), PBM_SETPOS,
                     static_cast<WPARAM>(permille), 0);
        std::wostringstream status;
        status << L"FFmpeg encoded approximately " << static_cast<std::uint32_t>(lParam)
               << L" source frame(s). Source PNGs remain untouched.";
        SetText(GetDlgItem(window, StatusLabel), status.str());
        return 0;
    }
    case kVideoCompleteMessage:
        CompleteExport(*state);
        return 0;
    case WM_CLOSE:
        if (state->running) CancelExport(*state);
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

void VideoExportDialog::Show(HWND owner, HINSTANCE instance) {
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
    state.dpi = DialogDpi(owner);
    const RECT rect = ResponsiveDialogRect(owner, 740, 480, state.dpi, kClassName);
    HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        kClassName, L"External FFmpeg H.264 / MP4 Export",
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
