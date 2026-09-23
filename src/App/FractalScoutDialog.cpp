#include "App/FractalScoutDialog.h"

#include "App/DialogSupport.h"
#include "Core/FractalScout.h"
#include "Core/DeepZoom.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <iomanip>
#include <initializer_list>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace mw {
#ifdef _WIN32
namespace {

constexpr wchar_t kClassName[] = L"MandelbrotFractalScoutDialog";
constexpr wchar_t kPreviewClassName[] = L"MandelbrotFractalScoutPreview";
constexpr UINT kProgressMessage = WM_APP + 231U;
constexpr UINT kCompleteMessage = WM_APP + 232U;

enum Id : int {
    QualityCombo = 9601,
    GoalCombo,
    SearchButton,
    CancelSearchButton,
    ProgressBar,
    ResultList,
    PreviewWindow,
    DetailsLabel,
    RefineButton,
    UseButton,
    SaveButton,
    CloseButton,
};

struct State {
    HWND owner{};
    HWND window{};
    HWND preview{};
    HINSTANCE instance{};
    Preset sourcePreset;
    PerformanceSettings performance;
    CameraState searchCamera;
    FractalScoutResult result;
    FractalScoutResult workerResult;
    std::string workerError;
    bool workerSucceeded{false};
    std::thread worker;
    std::atomic_bool cancelRequested{false};
    std::mutex workerMutex;
    bool searching{false};
    int selectedIndex{-1};
    Preset selectedCandidate;
    FractalScoutAction action{FractalScoutAction::None};
    HFONT font{};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    bool done{false};
};

HWND Add(State& state, const wchar_t* className, const wchar_t* text,
         DWORD style, int id, int x, int y, int width, int height) {
    HWND control = CreateWindowExW(
        className && std::wcscmp(className, WC_EDITW) == 0 ? WS_EX_CLIENTEDGE : 0,
        className, text,
        WS_CHILD | WS_VISIBLE | AccessibleControlStyle(className, style),
        x, y, width, height, state.window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), state.instance, nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
    return control;
}

std::wstring ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (required <= 1) return {};
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), required);
    result.resize(static_cast<std::size_t>(required - 1));
    return result;
}

void StopWorker(State& state) {
    state.cancelRequested.store(true);
    if (state.worker.joinable()) state.worker.join();
    state.searching = false;
}

FractalScoutRequest RequestForQuality(const State& state) {
    FractalScoutRequest request;
    request.preset = state.sourcePreset;
    request.searchCamera = state.searchCamera;
    const int goal = static_cast<int>(SendMessageW(
        GetDlgItem(state.window, GoalCombo), CB_GETCURSEL, 0, 0));
    switch (goal) {
    case 1: request.goal = FractalScoutGoal::Boundary; break;
    case 2: request.goal = FractalScoutGoal::Filaments; break;
    case 3: request.goal = FractalScoutGoal::Symmetry; break;
    default: request.goal = FractalScoutGoal::Balanced; break;
    }
    request.includeSymmetryScore = request.goal == FractalScoutGoal::Balanced;
    const int selected = static_cast<int>(SendMessageW(
        GetDlgItem(state.window, QualityCombo), CB_GETCURSEL, 0, 0));
    if (selected <= 0) {
        request.candidatePoolSize = 24;
        request.resultCount = 6;
        request.sampleGridWidth = 13;
        request.sampleGridHeight = 9;
        request.thumbnailWidth = 128;
        request.thumbnailHeight = 80;
        request.maximumIterations = std::clamp(state.performance.maximumIterations, 96, 144);
    } else if (selected == 2) {
        request.candidatePoolSize = 54;
        request.resultCount = 12;
        request.sampleGridWidth = 23;
        request.sampleGridHeight = 15;
        request.thumbnailWidth = 176;
        request.thumbnailHeight = 110;
        request.maximumIterations = std::clamp(state.performance.maximumIterations, 192, 384);
    } else {
        request.maximumIterations = std::clamp(state.performance.maximumIterations, 128, 256);
    }
    return request;
}

void UpdateDetails(State& state) {
    if (state.selectedIndex < 0 ||
        state.selectedIndex >= static_cast<int>(state.result.candidates.size())) {
        SetWindowTextW(GetDlgItem(state.window, DetailsLabel),
                       L"Select a candidate to inspect its score and coordinates.");
        EnableWindow(GetDlgItem(state.window, RefineButton), FALSE);
        EnableWindow(GetDlgItem(state.window, UseButton), FALSE);
        EnableWindow(GetDlgItem(state.window, SaveButton), FALSE);
        InvalidateRect(state.preview, nullptr, TRUE);
        return;
    }
    const auto& candidate = state.result.candidates[static_cast<std::size_t>(state.selectedIndex)];
    std::wostringstream text;
    text << std::fixed << std::setprecision(3)
         << L"Score " << candidate.score
         << L" | Boundary " << candidate.metrics.boundaryMix
         << L" | Variance " << candidate.metrics.iterationVariance
         << L" | Edges " << candidate.metrics.edgeDensity
         << L" | Symmetry " << candidate.metrics.symmetry
         << L" | Detail " << candidate.metrics.detail
         << L"\r\nDiverse ranking suppressed " << state.result.suppressedNearDuplicates
         << L" near-duplicate candidate(s).\r\n" << std::setprecision(15)
         << CameraCentreX(candidate.preset.camera) << L","
         << CameraCentreY(candidate.preset.camera) << L","
         << candidate.preset.camera.scale;
    SetWindowTextW(GetDlgItem(state.window, DetailsLabel), text.str().c_str());
    EnableWindow(GetDlgItem(state.window, RefineButton), TRUE);
    EnableWindow(GetDlgItem(state.window, UseButton), TRUE);
    EnableWindow(GetDlgItem(state.window, SaveButton), TRUE);
    InvalidateRect(state.preview, nullptr, TRUE);
}

void PopulateResults(State& state) {
    HWND list = GetDlgItem(state.window, ResultList);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (std::size_t index = 0; index < state.result.candidates.size(); ++index) {
        const auto& candidate = state.result.candidates[index];
        std::wostringstream label;
        label << L"Candidate " << (index + 1U) << L"   score "
              << std::fixed << std::setprecision(3) << candidate.score
              << L"   scale " << std::scientific << std::setprecision(1)
              << candidate.preset.camera.scale;
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.str().c_str()));
    }
    state.selectedIndex = state.result.candidates.empty() ? -1 : 0;
    if (state.selectedIndex >= 0) SendMessageW(list, LB_SETCURSEL, 0, 0);
    UpdateDetails(state);
}

void SetSearching(State& state, bool searching) {
    state.searching = searching;
    EnableWindow(GetDlgItem(state.window, SearchButton), !searching);
    EnableWindow(GetDlgItem(state.window, QualityCombo), !searching);
    EnableWindow(GetDlgItem(state.window, GoalCombo), !searching);
    EnableWindow(GetDlgItem(state.window, CancelSearchButton), searching);
    EnableWindow(GetDlgItem(state.window, RefineButton), !searching && state.selectedIndex >= 0);
    EnableWindow(GetDlgItem(state.window, UseButton), !searching && state.selectedIndex >= 0);
    EnableWindow(GetDlgItem(state.window, SaveButton), !searching && state.selectedIndex >= 0);
}

void StartSearch(State& state) {
    StopWorker(state);
    state.cancelRequested.store(false);
    state.workerError.clear();
    state.workerResult = {};
    state.workerSucceeded = false;
    state.result = {};
    state.selectedIndex = -1;
    SendMessageW(GetDlgItem(state.window, ResultList), LB_RESETCONTENT, 0, 0);
    InvalidateRect(state.preview, nullptr, TRUE);
    SetSearching(state, true);
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETRANGE32, 0, 100);
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETPOS, 0, 0);
    SetWindowTextW(GetDlgItem(state.window, DetailsLabel),
                   L"Scoring deterministic candidates around the current search region...");

    const FractalScoutRequest request = RequestForQuality(state);
    state.worker = std::thread([&state, request] {
        FractalScoutResult localResult;
        std::string localError;
        const bool succeeded = RunFractalScout(
            request,
            [&state](const FractalScoutProgress& progress) {
                PostMessageW(state.window, kProgressMessage,
                             static_cast<WPARAM>(progress.completedCandidates),
                             MAKELPARAM(progress.totalCandidates,
                                       progress.renderingThumbnails ? 1 : 0));
            },
            [&state] { return state.cancelRequested.load(); },
            localResult, localError);
        {
            std::lock_guard<std::mutex> lock(state.workerMutex);
            state.workerSucceeded = succeeded;
            state.workerResult = std::move(localResult);
            state.workerError = std::move(localError);
        }
        PostMessageW(state.window, kCompleteMessage, 0, 0);
    });
}

void ChooseCandidate(State& state, FractalScoutAction action) {
    if (state.selectedIndex < 0 ||
        state.selectedIndex >= static_cast<int>(state.result.candidates.size())) return;
    state.selectedCandidate =
        state.result.candidates[static_cast<std::size_t>(state.selectedIndex)].preset;
    state.action = action;
    StopWorker(state);
    DestroyWindow(state.window);
}

LRESULT CALLBACK PreviewProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<State*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        FillRect(dc, &client, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        if (state && state->selectedIndex >= 0 &&
            state->selectedIndex < static_cast<int>(state->result.candidates.size())) {
            const auto& preview = state->result.candidates[
                static_cast<std::size_t>(state->selectedIndex)].thumbnail;
            if (!preview.pixels.empty() && preview.width > 0U && preview.height > 0U) {
                BITMAPINFO info{};
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biWidth = static_cast<LONG>(preview.width);
                info.bmiHeader.biHeight = -static_cast<LONG>(preview.height);
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biCompression = BI_RGB;
                const int targetWidth = client.right - client.left;
                const int targetHeight = client.bottom - client.top;
                StretchDIBits(dc, 0, 0, targetWidth, targetHeight,
                              0, 0, static_cast<int>(preview.width),
                              static_cast<int>(preview.height), preview.pixels.data(),
                              &info, DIB_RGB_COLORS, SRCCOPY);
            }
        }
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
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

    if (message == WM_CREATE) {
        state->font = CreateResponsiveDialogFont(state->dpi);
        Add(*state, WC_STATICW,
            L"Searches a bounded deterministic set around the current view. Results stay temporary until you choose Use & Save as New.",
            SS_LEFT, 0, 16, 14, 746, 40);
        Add(*state, WC_STATICW, L"Search quality", SS_LEFT, 0, 16, 64, 110, 22);
        HWND quality = Add(*state, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                           QualityCombo, 130, 60, 180, 130);
        for (const wchar_t* item : {L"Fast — 6 results", L"Balanced — 9 results", L"Detailed — 12 results"}) {
            SendMessageW(quality, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        }
        SendMessageW(quality, CB_SETCURSEL, 1, 0);
        Add(*state, WC_STATICW, L"Search target", SS_LEFT, 0, 326, 64, 100, 22);
        HWND goal = Add(*state, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                        GoalCombo, 430, 60, 172, 150);
        for (const wchar_t* item : {L"Balanced detail", L"Boundary structures",
                                    L"Fine filaments", L"Symmetry and rings"}) {
            SendMessageW(goal, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        }
        SendMessageW(goal, CB_SETCURSEL, 0, 0);
        Add(*state, WC_BUTTONW, L"Search Current Region", BS_DEFPUSHBUTTON | WS_TABSTOP,
            SearchButton, 612, 59, 150, 32);
        Add(*state, WC_BUTTONW, L"Cancel Search", BS_PUSHBUTTON | WS_TABSTOP,
            CancelSearchButton, 16, 99, 120, 32);
        Add(*state, PROGRESS_CLASSW, L"", PBS_SMOOTH, ProgressBar, 146, 102, 616, 25);

        Add(*state, WC_LISTBOXW, L"", LBS_NOTIFY | WS_TABSTOP | WS_VSCROLL | WS_BORDER,
            ResultList, 16, 142, 248, 380);
        state->preview = Add(*state, kPreviewClassName, L"", SS_BLACKRECT,
                             PreviewWindow, 278, 142, 484, 322);
        Add(*state, WC_STATICW, L"Select a candidate after searching.", SS_LEFT,
            DetailsLabel, 278, 474, 484, 58);

        Add(*state, WC_BUTTONW, L"Refine Around Selected", BS_PUSHBUTTON | WS_TABSTOP,
            RefineButton, 16, 548, 176, 34);
        Add(*state, WC_BUTTONW, L"Use in Preview", BS_PUSHBUTTON | WS_TABSTOP,
            UseButton, 202, 548, 130, 34);
        Add(*state, WC_BUTTONW, L"Use && Save as New...", BS_PUSHBUTTON | WS_TABSTOP,
            SaveButton, 342, 548, 168, 34);
        Add(*state, WC_BUTTONW, L"Close", BS_PUSHBUTTON | WS_TABSTOP,
            CloseButton, 642, 548, 120, 34);
        EnableWindow(GetDlgItem(window, CancelSearchButton), FALSE);
        EnableWindow(GetDlgItem(window, RefineButton), FALSE);
        EnableWindow(GetDlgItem(window, UseButton), FALSE);
        EnableWindow(GetDlgItem(window, SaveButton), FALSE);
        SendMessageW(window, DM_SETDEFID, SearchButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 720, 560);
        state->layout.Focus(GetDlgItem(window, SearchButton));
        StartSearch(*state);
        return 0;
    }
    if (message == kProgressMessage) {
        const std::uint32_t completed = static_cast<std::uint32_t>(wParam);
        const std::uint32_t total = LOWORD(lParam);
        const bool thumbnails = HIWORD(lParam) != 0;
        const int percent = total > 0U
            ? static_cast<int>((100ULL * completed) / total) : 0;
        SendMessageW(GetDlgItem(window, ProgressBar), PBM_SETPOS, percent, 0);
        SetWindowTextW(GetDlgItem(window, DetailsLabel), thumbnails
            ? L"Rendering bounded candidate thumbnails..."
            : L"Scoring boundary mix, iteration variance, edge density and symmetry...");
        return 0;
    }
    if (message == kCompleteMessage) {
        if (state->worker.joinable()) state->worker.join();
        {
            std::lock_guard<std::mutex> lock(state->workerMutex);
            state->result = std::move(state->workerResult);
        }
        SetSearching(*state, false);
        SendMessageW(GetDlgItem(window, ProgressBar), PBM_SETPOS, 100, 0);
        if (!state->workerSucceeded) {
            const std::wstring text = L"Fractal Scout could not complete. " + ToWide(state->workerError);
            MessageBoxW(window, text.c_str(), L"Fractal Scout", MB_OK | MB_ICONERROR);
            SetWindowTextW(GetDlgItem(window, DetailsLabel), text.c_str());
        } else if (state->result.cancelled) {
            SetWindowTextW(GetDlgItem(window, DetailsLabel), L"Search cancelled. No partial candidates were applied.");
        } else {
            PopulateResults(*state);
        }
        return 0;
    }
    if (message == WM_GETMINMAXINFO) {
        state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;
    }
    if (message == WM_SIZE) { state->layout.OnSize(); return 0; }
    if ((message == WM_VSCROLL || message == WM_HSCROLL) && lParam == 0 &&
        state->layout.OnScroll(message, wParam)) return 0;
    if (message == WM_MOUSEWHEEL && state->layout.OnMouseWheel(wParam)) return 0;
    if (message == WM_DPICHANGED) {
        const UINT newDpi = HIWORD(wParam);
        HFONT newFont = CreateResponsiveDialogFont(newDpi);
        if (!newFont) newFont = state->font;
        const RECT suggested = *reinterpret_cast<const RECT*>(lParam);
        state->layout.OnDpiChanged(newDpi, suggested, newFont);
        if (newFont != state->font && state->font) DeleteObject(state->font);
        state->font = newFont;
        state->dpi = newDpi;
        return 0;
    }
    if (message == WM_COMMAND) {
        const int id = LOWORD(wParam);
        if (id == SearchButton) {
            StartSearch(*state);
        } else if (id == CancelSearchButton) {
            state->cancelRequested.store(true);
        } else if (id == ResultList && HIWORD(wParam) == LBN_SELCHANGE) {
            state->selectedIndex = static_cast<int>(SendMessageW(
                GetDlgItem(window, ResultList), LB_GETCURSEL, 0, 0));
            UpdateDetails(*state);
        } else if (id == RefineButton) {
            if (state->selectedIndex >= 0 &&
                state->selectedIndex < static_cast<int>(state->result.candidates.size())) {
                state->searchCamera = state->result.candidates[
                    static_cast<std::size_t>(state->selectedIndex)].preset.camera;
                StartSearch(*state);
            }
        } else if (id == UseButton) {
            ChooseCandidate(*state, FractalScoutAction::UseInPreview);
        } else if (id == SaveButton) {
            ChooseCandidate(*state, FractalScoutAction::UseAndSaveAsNew);
        } else if (id == CloseButton) {
            StopWorker(*state);
            DestroyWindow(window);
        }
        return 0;
    }
    if (message == WM_CLOSE) {
        StopWorker(*state);
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_DESTROY) {
        StopWorker(*state);
        RememberDialogPlacement(window, kClassName, state->dpi);
        state->layout.Shutdown();
        if (state->font) DeleteObject(state->font);
        state->font = nullptr;
        state->done = true;
        EnableWindow(state->owner, TRUE);
        SetForegroundWindow(state->owner);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

FractalScoutAction FractalScoutDialog::Show(HWND owner, HINSTANCE instance,
                                             const Preset& sourcePreset,
                                             const PerformanceSettings& performance,
                                             Preset& selectedCandidate) {
    WNDCLASSEXW previewClass{};
    previewClass.cbSize = sizeof(previewClass);
    previewClass.lpfnWndProc = PreviewProcedure;
    previewClass.hInstance = instance;
    previewClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    previewClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    previewClass.lpszClassName = kPreviewClassName;
    RegisterClassExW(&previewClass);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kClassName;
    RegisterClassExW(&windowClass);

    State state;
    state.owner = owner;
    state.instance = instance;
    state.sourcePreset = sourcePreset;
    state.performance = performance;
    state.searchCamera = sourcePreset.camera;
    state.dpi = DialogDpi(owner);
    const RECT rect = ResponsiveDialogRect(owner, 800, 640, state.dpi, kClassName);
    HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kClassName, L"Fractal Scout",
        WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX |
        WS_POPUP | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        owner, nullptr, instance, &state);
    if (!window) return FractalScoutAction::None;

    EnableWindow(owner, FALSE);
    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, CloseButton, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (state.action != FractalScoutAction::None) {
        selectedCandidate = state.selectedCandidate;
    }
    return state.action;
}
#endif
} // namespace mw
