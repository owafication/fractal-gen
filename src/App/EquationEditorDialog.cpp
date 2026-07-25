#include "App/EquationEditorDialog.h"
#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace mw {

#ifdef _WIN32
namespace {

constexpr wchar_t kEquationEditorClass[] = L"MandelbrotAdvancedEquationEditor";

enum Id : int {
    SummaryLabel = 7000,
    ExampleCombo, LoadExampleButton,
    SavedCombo, SavedNameEdit, LoadSavedButton, SaveNewButton, UpdateSavedButton, DeleteSavedButton,
    PowerEdit, ParameterPowerEdit, ReciprocalPowerEdit,
    AReal, AImag, BReal, BImag, CReal, CImag, DReal, DImag,
    IterReal, IterImag, ReciprocalReal, ReciprocalImag,
    AbsRealCheck, AbsImagCheck, ConjugateCheck, SwapCheck, TransformCombo,
    InitialModeCombo, InitialReal, InitialImag, JuliaCheck, JuliaReal, JuliaImag, BailoutEdit,
    RenderModeCombo, NewtonDegreeEdit, NewtonTargetReal, NewtonTargetImag,
    NewtonRelaxReal, NewtonRelaxImag, NewtonToleranceEdit,
    ColouringCombo, TrapCombo, TrapReal, TrapImag, TrapRadiusEdit, GlowEdit, DepthEdit,
    AnimateCheck, AnimationSpeedEdit, AnimationAmplitudeEdit, RandomiseButton,
    ResetButton, OkButton, CancelButton,
};

struct State {
    HWND owner{};
    HWND window{};
    HINSTANCE instance{};
    Preset* preset{};
    std::vector<EquationPreset>* saved{};
    EquationSettings original;
    HFONT font{};
    ResponsiveDialogLayout layout;
    DialogTooltipManager tooltips;
    UINT dpi{96};
    bool accepted{false};
    bool done{false};
};

HWND Add(HWND parent, HINSTANCE instance, const wchar_t* cls, const wchar_t* text,
         DWORD style, int id, int x, int y, int width, int height, HFONT font,
         DWORD exStyle = 0) {
    HWND control = CreateWindowExW(exStyle, cls, text, WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style),
                                   x, y, width, height, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   instance, nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

std::wstring Wide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count);
    return result;
}

std::string Utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count, nullptr, nullptr);
    return result;
}

std::wstring Number(double value) {
    std::wostringstream stream;
    stream << std::setprecision(12) << value;
    return stream.str();
}

void SetText(HWND window, int id, double value) {
    SetWindowTextW(GetDlgItem(window, id), Number(value).c_str());
}

void SetText(HWND window, int id, int value) {
    SetWindowTextW(GetDlgItem(window, id), std::to_wstring(value).c_str());
}

std::wstring ReadText(HWND window, int id) {
    const int length = GetWindowTextLengthW(GetDlgItem(window, id));
    std::wstring result(static_cast<std::size_t>(std::max(0, length)) + 1U, L'\0');
    if (length > 0) GetWindowTextW(GetDlgItem(window, id), result.data(), length + 1);
    result.resize(static_cast<std::size_t>(std::max(0, length)));
    return result;
}

bool ReadDouble(HWND window, int id, double& value, double minimum, double maximum) {
    try {
        const std::wstring input = ReadText(window, id);
        std::size_t consumed = 0;
        const double parsed = std::stod(input, &consumed);
        if (consumed != input.size() || !std::isfinite(parsed) || parsed < minimum || parsed > maximum) return false;
        value = parsed;
        return true;
    } catch (...) { return false; }
}

bool ReadInt(HWND window, int id, int& value, int minimum, int maximum) {
    try {
        const std::wstring input = ReadText(window, id);
        std::size_t consumed = 0;
        const int parsed = std::stoi(input, &consumed);
        if (consumed != input.size() || parsed < minimum || parsed > maximum) return false;
        value = parsed;
        return true;
    } catch (...) { return false; }
}

void Check(HWND window, int id, bool value) {
    SendMessageW(GetDlgItem(window, id), BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool Checked(HWND window, int id) {
    return SendMessageW(GetDlgItem(window, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

int ComboIndex(HWND window, int id) {
    return static_cast<int>(SendMessageW(GetDlgItem(window, id), CB_GETCURSEL, 0, 0));
}

void SetCombo(HWND window, int id, int value) {
    SendMessageW(GetDlgItem(window, id), CB_SETCURSEL, static_cast<WPARAM>(value), 0);
}

void Populate(HWND window, const EquationSettings& equation) {
    SetText(window, PowerEdit, equation.power);
    SetText(window, ParameterPowerEdit, equation.parameterPower);
    SetText(window, ReciprocalPowerEdit, equation.reciprocalPower);
    SetText(window, AReal, equation.quadratic.real); SetText(window, AImag, equation.quadratic.imaginary);
    SetText(window, BReal, equation.linear.real); SetText(window, BImag, equation.linear.imaginary);
    SetText(window, CReal, equation.parameter.real); SetText(window, CImag, equation.parameter.imaginary);
    SetText(window, DReal, equation.constant.real); SetText(window, DImag, equation.constant.imaginary);
    SetText(window, IterReal, equation.iterationTerm.real); SetText(window, IterImag, equation.iterationTerm.imaginary);
    SetText(window, ReciprocalReal, equation.reciprocalCoefficient.real);
    SetText(window, ReciprocalImag, equation.reciprocalCoefficient.imaginary);
    Check(window, AbsRealCheck, equation.absoluteReal); Check(window, AbsImagCheck, equation.absoluteImaginary);
    Check(window, ConjugateCheck, equation.conjugate); Check(window, SwapCheck, equation.swapRealImaginary);
    SetCombo(window, TransformCombo, static_cast<int>(equation.unaryTransform));
    SetCombo(window, InitialModeCombo, static_cast<int>(equation.initialZMode));
    SetText(window, InitialReal, equation.initialZ.real); SetText(window, InitialImag, equation.initialZ.imaginary);
    Check(window, JuliaCheck, equation.juliaMode);
    SetText(window, JuliaReal, equation.juliaParameter.real); SetText(window, JuliaImag, equation.juliaParameter.imaginary);
    SetText(window, BailoutEdit, equation.bailoutRadius);
    SetCombo(window, RenderModeCombo, equation.newtonMode || equation.renderMode == FractalRenderMode::Newton ? 1 : 0);
    SetText(window, NewtonDegreeEdit, equation.newtonDegree);
    SetText(window, NewtonTargetReal, equation.newtonTarget.real); SetText(window, NewtonTargetImag, equation.newtonTarget.imaginary);
    SetText(window, NewtonRelaxReal, equation.newtonRelaxation.real); SetText(window, NewtonRelaxImag, equation.newtonRelaxation.imaginary);
    SetText(window, NewtonToleranceEdit, equation.convergenceTolerance);
    SetCombo(window, ColouringCombo, static_cast<int>(equation.colouringMethod));
    SetCombo(window, TrapCombo, static_cast<int>(equation.orbitTrap));
    SetText(window, TrapReal, equation.orbitTrapPoint.real); SetText(window, TrapImag, equation.orbitTrapPoint.imaginary);
    SetText(window, TrapRadiusEdit, equation.orbitTrapRadius);
    SetText(window, GlowEdit, equation.glowStrength); SetText(window, DepthEdit, equation.depthStrength);
    Check(window, AnimateCheck, equation.animateCoefficients);
    SetText(window, AnimationSpeedEdit, equation.coefficientAnimationSpeed);
    SetText(window, AnimationAmplitudeEdit, equation.coefficientAnimationAmplitude);
    SetWindowTextW(GetDlgItem(window, SummaryLabel), Wide(EquationSummary(equation)).c_str());
}

bool ReadEquation(HWND window, EquationSettings& equation) {
    if (!ReadInt(window, PowerEdit, equation.power, 1, 12) ||
        !ReadInt(window, ParameterPowerEdit, equation.parameterPower, 1, 12) ||
        !ReadInt(window, ReciprocalPowerEdit, equation.reciprocalPower, 0, 12) ||
        !ReadDouble(window, AReal, equation.quadratic.real, -8, 8) || !ReadDouble(window, AImag, equation.quadratic.imaginary, -8, 8) ||
        !ReadDouble(window, BReal, equation.linear.real, -8, 8) || !ReadDouble(window, BImag, equation.linear.imaginary, -8, 8) ||
        !ReadDouble(window, CReal, equation.parameter.real, -8, 8) || !ReadDouble(window, CImag, equation.parameter.imaginary, -8, 8) ||
        !ReadDouble(window, DReal, equation.constant.real, -8, 8) || !ReadDouble(window, DImag, equation.constant.imaginary, -8, 8) ||
        !ReadDouble(window, IterReal, equation.iterationTerm.real, -8, 8) || !ReadDouble(window, IterImag, equation.iterationTerm.imaginary, -8, 8) ||
        !ReadDouble(window, ReciprocalReal, equation.reciprocalCoefficient.real, -8, 8) || !ReadDouble(window, ReciprocalImag, equation.reciprocalCoefficient.imaginary, -8, 8) ||
        !ReadDouble(window, InitialReal, equation.initialZ.real, -8, 8) || !ReadDouble(window, InitialImag, equation.initialZ.imaginary, -8, 8) ||
        !ReadDouble(window, JuliaReal, equation.juliaParameter.real, -8, 8) || !ReadDouble(window, JuliaImag, equation.juliaParameter.imaginary, -8, 8) ||
        !ReadDouble(window, BailoutEdit, equation.bailoutRadius, 1.01, 1.0e6) ||
        !ReadInt(window, NewtonDegreeEdit, equation.newtonDegree, 2, 12) ||
        !ReadDouble(window, NewtonTargetReal, equation.newtonTarget.real, -8, 8) || !ReadDouble(window, NewtonTargetImag, equation.newtonTarget.imaginary, -8, 8) ||
        !ReadDouble(window, NewtonRelaxReal, equation.newtonRelaxation.real, -8, 8) || !ReadDouble(window, NewtonRelaxImag, equation.newtonRelaxation.imaginary, -8, 8) ||
        !ReadDouble(window, NewtonToleranceEdit, equation.convergenceTolerance, 1.0e-12, 0.1) ||
        !ReadDouble(window, TrapReal, equation.orbitTrapPoint.real, -8, 8) || !ReadDouble(window, TrapImag, equation.orbitTrapPoint.imaginary, -8, 8) ||
        !ReadDouble(window, TrapRadiusEdit, equation.orbitTrapRadius, 1.0e-6, 8) ||
        !ReadDouble(window, GlowEdit, equation.glowStrength, 0, 4) || !ReadDouble(window, DepthEdit, equation.depthStrength, 0, 4) ||
        !ReadDouble(window, AnimationSpeedEdit, equation.coefficientAnimationSpeed, 0, 8) ||
        !ReadDouble(window, AnimationAmplitudeEdit, equation.coefficientAnimationAmplitude, 0, 2)) return false;
    equation.absoluteReal = Checked(window, AbsRealCheck); equation.absoluteImaginary = Checked(window, AbsImagCheck);
    equation.conjugate = Checked(window, ConjugateCheck); equation.swapRealImaginary = Checked(window, SwapCheck);
    equation.unaryTransform = static_cast<EquationUnaryTransform>(std::clamp(ComboIndex(window, TransformCombo), 0, 4));
    equation.initialZMode = static_cast<InitialZMode>(std::clamp(ComboIndex(window, InitialModeCombo), 0, 3));
    equation.juliaMode = Checked(window, JuliaCheck);
    equation.newtonMode = ComboIndex(window, RenderModeCombo) == 1;
    equation.renderMode = equation.newtonMode ? FractalRenderMode::Newton : FractalRenderMode::EscapeTime;
    equation.colouringMethod = static_cast<ColouringMethod>(std::clamp(ComboIndex(window, ColouringCombo), 0, 3));
    equation.orbitTrap = static_cast<OrbitTrapType>(std::clamp(ComboIndex(window, TrapCombo), 0, 2));
    equation.animateCoefficients = Checked(window, AnimateCheck);
    return true;
}

void Refresh(State& state) {
    EquationSettings equation = state.preset->equation;
    if (!ReadEquation(state.window, equation)) {
        SetWindowTextW(GetDlgItem(state.window, SummaryLabel), L"One or more values are outside their supported range.");
        return;
    }
    state.preset->equation = equation;
    SetWindowTextW(GetDlgItem(state.window, SummaryLabel), Wide(EquationSummary(equation)).c_str());
}

void PopulateSaved(State& state) {
    HWND combo = GetDlgItem(state.window, SavedCombo);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    for (const auto& saved : *state.saved) {
        const auto name = Wide(saved.name);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }
    if (!state.saved->empty()) SendMessageW(combo, CB_SETCURSEL, 0, 0);
}

std::string Slug(const std::string& value) {
    std::string result;
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) result.push_back(static_cast<char>(std::tolower(ch)));
        else if (!result.empty() && result.back() != '-') result.push_back('-');
    }
    if (result.empty()) result = "equation";
    return result;
}

void SaveEquationPreset(State& state, bool update) {
    Refresh(state);
    const std::string name = Utf8(ReadText(state.window, SavedNameEdit));
    if (name.empty() || name.size() > 120U) {
        MessageBoxW(state.window, L"Enter an equation preset name containing 1 to 120 characters.",
                    L"Equation Preset", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (!update && state.saved->size() >= 256U) {
        MessageBoxW(state.window, L"The equation preset library already contains the 256-entry safety maximum.",
                    L"Equation Preset", MB_OK | MB_ICONINFORMATION);
        return;
    }
    const int selected = ComboIndex(state.window, SavedCombo);
    if (update && selected >= 0 && selected < static_cast<int>(state.saved->size())) {
        auto& item = (*state.saved)[static_cast<std::size_t>(selected)];
        item.name = name;
        item.equation = state.preset->equation;
    } else {
        const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        state.saved->push_back({Slug(name) + "-" + std::to_string(stamp), name, state.preset->equation});
    }
    PopulateSaved(state);
    SendMessageW(GetDlgItem(state.window, SavedCombo), CB_SETCURSEL,
                 static_cast<WPARAM>(state.saved->size() - 1U), 0);
}

void Randomise(State& state) {
    static std::mt19937 generator{std::random_device{}()};
    std::uniform_real_distribution<double> coefficient(-1.4, 1.4);
    std::uniform_int_distribution<int> power(2, 8);
    EquationSettings equation;
    equation.power = power(generator);
    equation.quadratic = {coefficient(generator), coefficient(generator) * 0.25};
    equation.parameter = {coefficient(generator), coefficient(generator) * 0.25};
    equation.constant = {coefficient(generator) * 0.3, coefficient(generator) * 0.3};
    equation.bailoutRadius = 4.0;
    Populate(state.window, equation);
    Refresh(state);
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
        Add(window, state->instance, WC_STATICW, L"Equation preview", SS_LEFT, 0, 14, 12, 110, 22, state->font);
        Add(window, state->instance, WC_STATICW, L"", SS_LEFT | SS_SUNKEN, SummaryLabel, 126, 8, 842, 34, state->font);
        Add(window, state->instance, WC_STATICW, L"Built-in equation", SS_LEFT, 0, 14, 52, 122, 22, state->font);
        HWND examples = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                            ExampleCombo, 138, 48, 260, 360, state->font);
        for (const auto& name : EquationExampleNames()) {
            const auto wide = Wide(name); SendMessageW(examples, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str()));
        }
        SendMessageW(examples, CB_SETCURSEL, 0, 0);
        Add(window, state->instance, WC_BUTTONW, L"Load", BS_PUSHBUTTON, LoadExampleButton, 404, 48, 64, 27, state->font);
        Add(window, state->instance, WC_STATICW, L"Saved", SS_LEFT, 0, 480, 52, 52, 22, state->font);
        Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP, SavedCombo, 532, 48, 138, 180, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, SavedNameEdit, 676, 48, 126, 27, state->font, WS_EX_CLIENTEDGE);
        Add(window, state->instance, WC_BUTTONW, L"Load", BS_PUSHBUTTON, LoadSavedButton, 808, 48, 54, 27, state->font);
        Add(window, state->instance, WC_BUTTONW, L"New", BS_PUSHBUTTON, SaveNewButton, 866, 48, 48, 27, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Update", BS_PUSHBUTTON, UpdateSavedButton, 918, 48, 62, 27, state->font);

        Add(window, state->instance, WC_BUTTONW, L"Recurrence", BS_GROUPBOX, 0, 14, 86, 316, 530, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Initial value, Julia and Newton", BS_GROUPBOX, 0, 338, 86, 316, 530, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Colouring and animation", BS_GROUPBOX, 0, 662, 86, 318, 530, state->font);

        auto label = [&](int x, int y, int w, const wchar_t* value) { Add(window, state->instance, WC_STATICW, value, SS_LEFT, 0, x, y, w, 21, state->font); };
        auto edit = [&](int x, int y, int w, int id) { Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, id, x, y, w, 25, state->font, WS_EX_CLIENTEDGE); };
        int y = 112;
        label(28,y,62,L"z power"); edit(94,y-3,46,PowerEdit); label(148,y,62,L"c power"); edit(214,y-3,46,ParameterPowerEdit); label(266,y,42,L"1/z"); edit(292,y-3,28,ReciprocalPowerEdit); y+=34;
        label(28,y,112,L"Term"); label(152,y,72,L"Real"); label(236,y,72,L"Imaginary"); y+=26;
        struct Row {const wchar_t* name;int r;int i;};
        const std::array<Row,6> rows{{{L"A · T(z)^p",AReal,AImag},{L"B · T(z)",BReal,BImag},{L"C · c^r",CReal,CImag},{L"D constant",DReal,DImag},{L"E · iteration",IterReal,IterImag},{L"λ / T(z)^q",ReciprocalReal,ReciprocalImag}}};
        for (const auto& row:rows){label(28,y+3,116,row.name);edit(150,y,76,row.r);edit(234,y,76,row.i);y+=34;}
        label(28,y+4,112,L"Function"); HWND transform=Add(window,state->instance,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,TransformCombo,150,y,160,150,state->font);for(const wchar_t* value:{L"None",L"sin(z)",L"cos(z)",L"exp(z)",L"log(z)"})SendMessageW(transform,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value));y+=36;
        Add(window,state->instance,WC_BUTTONW,L"Absolute real",BS_AUTOCHECKBOX,AbsRealCheck,28,y,128,24,state->font);Add(window,state->instance,WC_BUTTONW,L"Absolute imaginary",BS_AUTOCHECKBOX,AbsImagCheck,164,y,146,24,state->font);y+=28;
        Add(window,state->instance,WC_BUTTONW,L"Conjugate z",BS_AUTOCHECKBOX,ConjugateCheck,28,y,128,24,state->font);Add(window,state->instance,WC_BUTTONW,L"Swap real / imaginary",BS_AUTOCHECKBOX,SwapCheck,164,y,146,24,state->font);

        y=112; label(352,y+4,110,L"Render mode");HWND render=Add(window,state->instance,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,RenderModeCombo,468,y,172,120,state->font);for(const wchar_t* value:{L"Escape-time / Julia",L"Newton convergence"})SendMessageW(render,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value));y+=38;
        label(352,y+4,110,L"Initial z");HWND initial=Add(window,state->instance,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,InitialModeCombo,468,y,172,150,state->font);for(const wchar_t* value:{L"Zero",L"Fixed value",L"Use c",L"Critical point"})SendMessageW(initial,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value));y+=38;
        label(352,y+3,108,L"Fixed z real");edit(468,y,76,InitialReal);label(550,y+3,36,L"imag");edit(588,y,52,InitialImag);y+=36;
        Add(window,state->instance,WC_BUTTONW,L"Julia mode — pixel is z₀ and c is fixed",BS_AUTOCHECKBOX,JuliaCheck,352,y,288,24,state->font);y+=31;
        label(352,y+3,108,L"Fixed c real");edit(468,y,76,JuliaReal);label(550,y+3,36,L"imag");edit(588,y,52,JuliaImag);y+=36;
        label(352,y+3,108,L"Bailout radius");edit(468,y,172,BailoutEdit);y+=44;
        label(352,y,270,L"Newton f(z)=z^degree−target");y+=25;
        label(352,y+3,108,L"Degree 2–12");edit(468,y,72,NewtonDegreeEdit);y+=34;
        label(352,y+3,108,L"Target real");edit(468,y,76,NewtonTargetReal);label(550,y+3,36,L"imag");edit(588,y,52,NewtonTargetImag);y+=34;
        label(352,y+3,108,L"Relaxation real");edit(468,y,76,NewtonRelaxReal);label(550,y+3,36,L"imag");edit(588,y,52,NewtonRelaxImag);y+=34;
        label(352,y+3,108,L"Tolerance");edit(468,y,172,NewtonToleranceEdit);

        y=112;label(676,y+4,112,L"Colour method");HWND colouring=Add(window,state->instance,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,ColouringCombo,790,y,176,150,state->font);for(const wchar_t* value:{L"Smooth escape",L"Orbit trap",L"Distance estimate",L"Newton basins"})SendMessageW(colouring,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value));y+=38;
        label(676,y+4,112,L"Orbit trap");HWND trap=Add(window,state->instance,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,TrapCombo,790,y,176,120,state->font);for(const wchar_t* value:{L"Point",L"Cross",L"Circle"})SendMessageW(trap,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value));y+=38;
        label(676,y+3,108,L"Trap point real");edit(790,y,76,TrapReal);label(872,y+3,36,L"imag");edit(910,y,56,TrapImag);y+=34;
        label(676,y+3,108,L"Circle radius");edit(790,y,176,TrapRadiusEdit);y+=34;
        label(676,y+3,108,L"Glow 0–4");edit(790,y,176,GlowEdit);y+=34;
        label(676,y+3,108,L"Depth 0–4");edit(790,y,176,DepthEdit);y+=46;
        Add(window,state->instance,WC_BUTTONW,L"Animate coefficients",BS_AUTOCHECKBOX,AnimateCheck,676,y,200,24,state->font);y+=31;
        label(676,y+3,108,L"Speed 0–8");edit(790,y,176,AnimationSpeedEdit);y+=34;
        label(676,y+3,108,L"Amplitude 0–2");edit(790,y,176,AnimationAmplitudeEdit);y+=42;
        Add(window,state->instance,WC_BUTTONW,L"Randomise bounded equation",BS_PUSHBUTTON,RandomiseButton,676,y,290,30,state->font);y+=42;
        Add(window,state->instance,WC_STATICW,L"Distance estimation is used for analytic polynomial maps. Unsupported transforms fall back to smooth colouring. Perturbation deep zoom remains limited to compatible quadratic maps.",SS_LEFT,0,676,y,290,76,state->font);

        Add(window,state->instance,WC_BUTTONW,L"&Reset Mandelbrot",BS_PUSHBUTTON,ResetButton,14,630,150,32,state->font);
        Add(window,state->instance,WC_BUTTONW,L"Delete saved",BS_PUSHBUTTON,DeleteSavedButton,174,630,120,32,state->font);
        Add(window,state->instance,WC_BUTTONW,L"&OK",BS_DEFPUSHBUTTON,OkButton,746,630,106,32,state->font);
        Add(window,state->instance,WC_BUTTONW,L"&Cancel",BS_PUSHBUTTON,CancelButton,864,630,106,32,state->font);
        PopulateSaved(*state); Populate(window,state->preset->equation);
        state->tooltips.Initialise(window, state->dpi, state->font);
        const auto tip = [&](int id, const wchar_t* text) { state->tooltips.Add(GetDlgItem(window, id), text); };
        tip(PowerEdit, L"Integer exponent applied to the transformed z value. Supported range: 1 to 12.");
        tip(ParameterPowerEdit, L"Integer exponent applied to c. Use 2 for c squared, including the reference z + c² equation.");
        tip(ReciprocalPowerEdit, L"Exponent q in the rational-map term lambda / T(z)^q. Set the lambda coefficient to zero to disable it.");
        tip(TransformCombo, L"Optional complex transform applied before the recurrence terms. Transcendental transforms may restrict deep-zoom precision modes.");
        tip(InitialModeCombo, L"Chooses z0 for parameter-set rendering: zero, a fixed value, the pixel coordinate c, or a supported critical point.");
        tip(JuliaCheck, L"Julia mode treats each pixel as z0 and uses the fixed complex c value below.");
        tip(BailoutEdit, L"Escape radius. Larger values can improve smooth colouring for higher powers but may require more iterations.");
        tip(RenderModeCombo, L"Escape-time renders Mandelbrot, Julia and rational maps. Newton mode colours convergence to roots.");
        tip(NewtonDegreeEdit, L"Degree of f(z)=z^degree-target for Newton convergence rendering. Supported range: 2 to 12.");
        tip(NewtonRelaxReal, L"Real part of the Newton relaxation multiplier. The usual value is 1.");
        tip(NewtonRelaxImag, L"Imaginary part of the Newton relaxation multiplier. Leave at 0 for standard Newton iteration.");
        tip(NewtonToleranceEdit, L"Convergence threshold. Smaller values sharpen basin boundaries but can require more iterations.");
        tip(ColouringCombo, L"Selects smooth escape, orbit-trap, distance-estimate or Newton-basin colouring.");
        tip(TrapCombo, L"Shape used to measure the nearest orbit approach when orbit-trap colouring is selected.");
        tip(GlowEdit, L"Post-process glow intensity from 0 to 4. Higher values cost additional GPU time.");
        tip(DepthEdit, L"Depth-shading strength from 0 to 4. This affects appearance, not the fractal equation.");
        tip(AnimateCheck, L"Animates bounded equation coefficients using the speed and amplitude settings below.");
        SendMessageW(window, DM_SETDEFID, OkButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 720, 520);
        state->layout.Focus(GetDlgItem(window, ExampleCombo));
        return 0;
    }

    if(message==WM_GETMINMAXINFO){state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));return 0;}
    if(message==WM_SIZE){state->layout.OnSize();return 0;}
    if((message==WM_VSCROLL||message==WM_HSCROLL)&&lParam==0){if(state->layout.OnScroll(message,wParam))return 0;}
    if(message==WM_MOUSEWHEEL&&state->layout.OnMouseWheel(wParam))return 0;
    if(message==WM_DPICHANGED){const UINT newDpi=HIWORD(wParam);HFONT newFont=CreateResponsiveDialogFont(newDpi);if(!newFont)newFont=state->font;const RECT suggested=*reinterpret_cast<const RECT*>(lParam);state->layout.OnDpiChanged(newDpi,suggested,newFont);if(newFont!=state->font&&state->font)DeleteObject(state->font);state->font=newFont;state->dpi=newDpi;state->tooltips.SetFont(newDpi,newFont);return 0;}

    if (message == WM_COMMAND) {
        const int id=LOWORD(wParam), notification=HIWORD(wParam);
        if(id==LoadExampleButton){const int selected=ComboIndex(window,ExampleCombo);if(selected>=0){Populate(window,EquationExample(static_cast<std::size_t>(selected)));Refresh(*state);}}
        else if(id==LoadSavedButton){const int selected=ComboIndex(window,SavedCombo);if(selected>=0&&selected<static_cast<int>(state->saved->size())){Populate(window,(*state->saved)[static_cast<std::size_t>(selected)].equation);SetWindowTextW(GetDlgItem(window,SavedNameEdit),Wide((*state->saved)[static_cast<std::size_t>(selected)].name).c_str());Refresh(*state);}}
        else if(id==SaveNewButton)SaveEquationPreset(*state,false);
        else if(id==UpdateSavedButton)SaveEquationPreset(*state,true);
        else if(id==DeleteSavedButton){const int selected=ComboIndex(window,SavedCombo);if(selected>=0&&selected<static_cast<int>(state->saved->size())){state->saved->erase(state->saved->begin()+selected);PopulateSaved(*state);}}
        else if(id==RandomiseButton)Randomise(*state);
        else if(id==ResetButton){Populate(window,EquationSettings{});Refresh(*state);}
        else if(id==OkButton){EquationSettings equation;if(!ReadEquation(window,equation)){MessageBoxW(window,L"Check the highlighted equation values and supported ranges.",L"Invalid Equation",MB_OK|MB_ICONERROR);return 0;}state->preset->equation=equation;state->accepted=true;DestroyWindow(window);}
        else if(id==CancelButton)DestroyWindow(window);
        else if(notification==EN_CHANGE||notification==CBN_SELCHANGE||notification==BN_CLICKED)Refresh(*state);
        return 0;
    }
    if(message==WM_CLOSE){DestroyWindow(window);return 0;}
    if(message==WM_DESTROY){if(!state->accepted)state->preset->equation=state->original;RememberDialogPlacement(window,kEquationEditorClass,state->dpi);state->tooltips.Shutdown();state->layout.Shutdown();if(state->font)DeleteObject(state->font);state->font=nullptr;state->done=true;EnableWindow(state->owner,TRUE);SetForegroundWindow(state->owner);return 0;}
    return DefWindowProcW(window,message,wParam,lParam);
}

} // namespace

bool EquationEditorDialog::Show(HWND owner, HINSTANCE instance, Preset& preset,
                                std::vector<EquationPreset>& savedPresets) {
    WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);cls.lpfnWndProc=Procedure;cls.hInstance=instance;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);cls.lpszClassName=kEquationEditorClass;RegisterClassExW(&cls);
    State state;state.owner=owner;state.instance=instance;state.preset=&preset;state.saved=&savedPresets;state.original=preset.equation;state.dpi=DialogDpi(owner);
    const RECT dialogRect=ResponsiveDialogRect(owner,1000,720,state.dpi,kEquationEditorClass);
    HWND window=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_CONTROLPARENT,kEquationEditorClass,L"Advanced Fractal Equation Editor",WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MAXIMIZEBOX|WS_POPUP|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL,dialogRect.left,dialogRect.top,dialogRect.right-dialogRect.left,dialogRect.bottom-dialogRect.top,owner,nullptr,instance,&state);if(!window)return false;EnableWindow(owner,FALSE);
    MSG message{};while(!state.done&&GetMessageW(&message,nullptr,0,0)>0){if(!ProcessModalDialogMessage(window,CancelButton,message,&state.layout)){TranslateMessage(&message);DispatchMessageW(&message);}}
    return state.accepted;
}
#endif

} // namespace mw
