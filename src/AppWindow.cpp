#include "AppWindow.h"

#include "SimpleJson.h"

#include <algorithm>
#include <cmath>
#include <commdlg.h>
#include <commctrl.h>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <dwmapi.h>
#include <gdiplus.h>
#include <imm.h>
#include <objidl.h>
#include <memory>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <thread>
#include <uxtheme.h>
#include <windowsx.h>
#include <vector>
#include <winver.h>
#include "AppWindow.h"
#include "SimpleJson.h"
#include "resource.h"

AppWindow* g_app_window = nullptr;
constexpr wchar_t kAppDisplayName[] = L"BlinkText";
constexpr int kGlobalToggleHotkeyId = 1;
constexpr UINT kStatusMessage = WM_APP + 1;
constexpr UINT kTrayIconMessage = WM_APP + 2;
constexpr wchar_t kDefaultDataFileName[] = L"BlinkText_Snippets.json";
constexpr wchar_t kLegacyProjectDataPath[] = L"BlinkText_Snippets.json";
constexpr UINT kTrayMenuOpen = 6101;
constexpr UINT kTrayMenuToggleEngine = 6102;
constexpr UINT kTrayMenuExit = 6103;
constexpr UINT kSnippetMenuEdit = 6201;
constexpr UINT kSnippetMenuNew = 6202;
constexpr UINT kSnippetMenuDuplicate = 6203;
constexpr UINT kSnippetMenuDelete = 6204;
constexpr UINT kSnippetMenuFocusSearch = 6205;
constexpr UINT kAppCommandSelectAll = 6206;
constexpr UINT kEditMenuUndo = 6210;
constexpr UINT kEditMenuCut = 6211;
constexpr UINT kEditMenuCopy = 6212;
constexpr UINT kEditMenuPaste = 6213;
constexpr UINT kEditMenuDelete = 6214;
constexpr UINT kEditMenuSelectAll = 6215;
constexpr UINT kEditMenuRtlReadingOrder = 6216;
constexpr UINT kEditMenuShowUnicodeControls = 6217;
constexpr UINT kEditMenuInsertUnicodeControl = 6218;
constexpr UINT kEditMenuOpenIme = 6219;
constexpr UINT kEditMenuReconversion = 6220;
constexpr UINT kEditMenuInsertLrm = 6221;
constexpr UINT kEditMenuInsertRlm = 6222;
constexpr UINT kEditMenuInsertZwnj = 6223;
constexpr UINT kEditMenuInsertZwj = 6224;
constexpr UINT kEditMenuInsertLri = 6225;
constexpr UINT kEditMenuInsertRli = 6226;
constexpr UINT kEditMenuInsertFsi = 6227;
constexpr UINT kEditMenuInsertPdi = 6228;
constexpr const wchar_t* kWindowIconCandidates[] = {L"app.ico", L"icon.ico"};
constexpr const wchar_t* kTrayPauseIconCandidates[] = {L"app_pause.ico", L"App_Pause.ico"};
constexpr wchar_t kGithubButtonImageName[] = L"github.png";
constexpr wchar_t kInfoButtonImageName[] = L"info.png";
constexpr wchar_t kSearchPlaceholderText[] = L"Search";
constexpr wchar_t kPreviousClipboardTriggerText[] = L"\\\\";
constexpr wchar_t kPreviousClipboardSlashTriggerText[] = L"//";
constexpr int kSplitterThickness = 8;
constexpr int kMinWindowWidth = 960;
constexpr int kMinWindowHeight = 720;
constexpr int kMinLeftPanelWidth = 128;
constexpr int kMinCenterPanelWidth = 188;
constexpr int kMinRightPanelWidth = 165;
constexpr int kFixedLeftPanelWidth = 219;
constexpr int kFixedRightPanelWidth = 305;
constexpr int kPanelGap = 11;
constexpr int kCardRadius = 11;
constexpr double kMainUiScale = 0.75;

int ScaleMainUi(int value) {
    return std::max(1, static_cast<int>(std::lround(static_cast<double>(value) * kMainUiScale)));
}

std::wstring GetDocumentsDirectory() {
    wchar_t documents_path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documents_path)) &&
        documents_path[0] != L'\0') {
        return std::wstring(documents_path);
    }
    return std::wstring();
}

POINT GetEffectiveMinimumWindowSizeForMonitor(const MONITORINFO& monitor_info) {
    const int monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
    const int monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
    const int work_width = monitor_info.rcWork.right - monitor_info.rcWork.left;
    const int work_height = monitor_info.rcWork.bottom - monitor_info.rcWork.top;
    const bool tiny_screen = monitor_width <= 800 || monitor_height <= 600;

    POINT minimum{
        tiny_screen ? std::min(800, monitor_width) : kMinWindowWidth,
        tiny_screen ? std::min(600, monitor_height) : kMinWindowHeight
    };

    minimum.x = std::min(static_cast<int>(minimum.x), std::max(640, work_width));
    minimum.y = std::min(static_cast<int>(minimum.y), std::max(480, work_height));
    return minimum;
}

POINT GetEffectiveMinimumWindowSize(HWND hwnd) {
    HMONITOR monitor = hwnd != nullptr
        ? MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)
        : MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (monitor != nullptr && GetMonitorInfoW(monitor, &monitor_info)) {
        return GetEffectiveMinimumWindowSizeForMonitor(monitor_info);
    }

    return POINT{kMinWindowWidth, kMinWindowHeight};
}

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef HDM_SETBKCOLOR
#define HDM_SETBKCOLOR (HDM_FIRST + 19)
#endif
#ifndef HDM_SETTEXTCOLOR
#define HDM_SETTEXTCOLOR (HDM_FIRST + 20)
#endif

enum ControlId : int {
    kEngineButton = 1001,
    kThemeButton,
    kImportButton,
    kExportButton,
    kGithubButton,
    kInfoButton,
    kRecordHotkeyButton,
    kNewGroupButton,
    kRenameGroupButton,
    kToggleGroupButton,
    kDeleteGroupButton,
    kTestResetButton,
    kNewSnippetButton,
    kDuplicateButton,
    kToggleSnippetButton,
    kDeleteButton,
    kSnippetPanelNewButton,
    kEditorNewButton,
    kEditorSnippetTabButton,
    kEditorEngineTabButton,
    kSaveButton,
    kResetButton,
    kSettingsSaveButton,
    kSettingsResetButton,
    kAlwaysOnTopCheckbox,
    kStartWithWindowsCheckbox,
    kMinimizeToTrayCheckbox,
    kPreviousClipboardCheckbox,
    kPreviousClipboardSlashCheckbox,
    kSnippetEnabledCheckbox,
    kCaseSensitiveCheckbox,
    kInstantModeRadio,
    kSeparatorModeRadio,
    kSeparatorSpaceCheckbox,
    kSeparatorEnterCheckbox,
    kSeparatorTabCheckbox,
    kGroupsList,
    kSnippetsList,
    kSearchEdit,
    kTestEdit,
    kEditorGroupCombo,
    kEditorSettingsScrollbar,
};

constexpr wchar_t kTemporaryGithubUrl[] = L"https://github.com/LeaDer-E/BlinkText";

HWND CreateControl(
    DWORD ex_style,
    const wchar_t* class_name,
    const wchar_t* text,
    DWORD style,
    int id,
    HWND parent,
    HINSTANCE instance
) {
    return CreateWindowExW(
        ex_style,
        class_name,
        text,
        style,
        0,
        0,
        0,
        0,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        instance,
        nullptr
    );
}

constexpr wchar_t kOwnerDrawCheckedProp[] = L"BlinkTexts.OwnerDrawChecked";

bool IsOwnerDrawChecked(HWND hwnd) {
    return hwnd != nullptr && GetPropW(hwnd, kOwnerDrawCheckedProp) != nullptr;
}

void SetOwnerDrawChecked(HWND hwnd, bool checked) {
    if (hwnd == nullptr) {
        return;
    }
    if (checked) {
        SetPropW(hwnd, kOwnerDrawCheckedProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(1)));
    } else {
        RemovePropW(hwnd, kOwnerDrawCheckedProp);
    }
    InvalidateRect(hwnd, nullptr, TRUE);
}

std::wstring TitleCaseCopy(std::wstring value) {
    if (value.empty()) {
        return value;
    }

    bool make_upper = true;
    for (wchar_t& ch : value) {
        if (iswspace(ch) || ch == L'-' || ch == L'_') {
            make_upper = true;
            if (ch == L'_') {
                ch = L' ';
            }
            continue;
        }

        ch = static_cast<wchar_t>(make_upper ? towupper(ch) : towlower(ch));
        make_upper = false;
    }

    return value;
}

std::wstring FileNameFromPath(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

const simplejson::Value* FindField(const simplejson::Value* object, const std::wstring& key) {
    if (object == nullptr || object->type != simplejson::Value::Type::Object) {
        return nullptr;
    }
    return object->Find(key);
}

std::wstring JsonStringValue(const simplejson::Value* value, const std::wstring& fallback = L"") {
    if (value == nullptr || value->type != simplejson::Value::Type::String) {
        return fallback;
    }
    return value->string_value;
}

bool JsonBoolValue(const simplejson::Value* value, bool fallback) {
    if (value == nullptr || value->type != simplejson::Value::Type::Bool) {
        return fallback;
    }
    return value->bool_value;
}

int JsonIntValue(const simplejson::Value* value, int fallback) {
    if (value == nullptr || value->type != simplejson::Value::Type::Number) {
        return fallback;
    }
    return static_cast<int>(value->number_value);
}

simplejson::Value JsonStringArray(const std::vector<std::wstring>& values) {
    simplejson::Value array = simplejson::Value::Array();
    for (const auto& value : values) {
        array.array_value.push_back(simplejson::Value::String(value));
    }
    return array;
}

constexpr int kImportOptionsDialogWidth = 740;
constexpr int kImportOptionsDialogHeight = 428;
constexpr int kImportDialogGroupComboId = 7001;
constexpr int kImportDialogSkipRadioId = 7002;
constexpr int kImportDialogOverwriteRadioId = 7003;
constexpr int kImportDialogImportButtonId = 7004;
constexpr int kImportDialogCancelButtonId = 7005;
constexpr int kImportDialogFileGroupId = 7006;
constexpr int kImportDialogOptionsGroupId = 7007;
constexpr int kImportDialogConflictGroupId = 7008;
constexpr int kImportDialogBrowseButtonId = 7009;
constexpr wchar_t kKeepSourceGroupsOption[] = L"<Keep source groups>";

struct ImportDialogConfig {
    const AppWindow* owner = nullptr;
    std::wstring file_path;
    std::wstring format_label;
    std::wstring default_group;
    std::vector<std::wstring> existing_groups;
    bool dark_theme = false;
    bool allow_keep_source_groups = false;
    int imported_count = 0;
    int conflict_count = 0;
};

struct ImportDialogResult {
    bool accepted = false;
    bool keep_source_groups = false;
    std::wstring target_group;
    bool overwrite_conflicts = false;
    std::wstring file_path;
    AppWindow::ParsedImportData parsed_data;
};

struct ImportDialogState {
    HINSTANCE instance = nullptr;
    const ImportDialogConfig* config = nullptr;
    ImportDialogResult* result = nullptr;
    HWND hwnd = nullptr;
    HWND file_edit = nullptr;
    HWND format_label = nullptr;
    HWND group_combo = nullptr;
    HWND imported_label = nullptr;
    HWND skip_radio = nullptr;
    HWND overwrite_radio = nullptr;
    HFONT font = nullptr;
    HFONT title_font = nullptr;
    HBRUSH background_brush = nullptr;
    HBRUSH input_brush = nullptr;
    HBRUSH surface_brush = nullptr;
    std::wstring current_file_path;
    AppWindow::ParsedImportData current_data;
    int current_conflict_count = 0;
    bool finished = false;
};

constexpr int kTextEntryDialogWidth = 560;
constexpr int kTextEntryDialogHeight = 224;
constexpr int kTextEntryEditId = 7201;
constexpr int kTextEntryOkId = 7202;
constexpr int kTextEntryCancelId = 7203;
constexpr int kDeleteGroupDialogWidth = 690;
constexpr int kDeleteGroupDialogHeight = 326;
constexpr int kDeleteGroupTargetComboId = 7301;
constexpr int kDeleteGroupOkId = 7302;
constexpr int kDeleteGroupCancelId = 7303;
constexpr int kDeleteGroupDeleteAllId = 7304;
constexpr int kExportOptionsDialogWidth = 680;
constexpr int kExportOptionsDialogHeight = 370;
constexpr int kExportDialogNativeRadioId = 7401;
constexpr int kExportDialogBeeftextRadioId = 7402;
constexpr int kExportDialogAllSnippetsRadioId = 7403;
constexpr int kExportDialogCurrentGroupRadioId = 7404;
constexpr int kExportDialogExportButtonId = 7405;
constexpr int kExportDialogCancelButtonId = 7406;
constexpr int kExportDialogFormatGroupId = 7407;
constexpr int kExportDialogScopeGroupId = 7408;

struct TextEntryDialogConfig {
    std::wstring title;
    std::wstring prompt;
    std::wstring initial_value;
    std::wstring ok_button_label = L"OK";
    bool dark_theme = false;
};

struct TextEntryDialogResult {
    bool accepted = false;
    std::wstring text;
};

struct TextEntryDialogState {
    HINSTANCE instance = nullptr;
    const TextEntryDialogConfig* config = nullptr;
    TextEntryDialogResult* result = nullptr;
    HWND hwnd = nullptr;
    HWND edit = nullptr;
    HFONT font = nullptr;
    HFONT title_font = nullptr;
    HBRUSH background_brush = nullptr;
    HBRUSH input_brush = nullptr;
    HBRUSH surface_brush = nullptr;
    bool finished = false;
};

struct DeleteGroupDialogConfig {
    std::wstring group_name;
    int moved_snippet_count = 0;
    std::vector<std::wstring> target_groups;
    std::wstring default_target_group;
    bool dark_theme = false;
};

struct DeleteGroupDialogResult {
    bool accepted = false;
    std::wstring target_group;
    bool delete_all_snippets = false;
};

struct DeleteGroupDialogState {
    HINSTANCE instance = nullptr;
    const DeleteGroupDialogConfig* config = nullptr;
    DeleteGroupDialogResult* result = nullptr;
    HWND hwnd = nullptr;
    HWND target_combo = nullptr;
    HFONT font = nullptr;
    HFONT title_font = nullptr;
    HBRUSH background_brush = nullptr;
    HBRUSH input_brush = nullptr;
    HBRUSH surface_brush = nullptr;
    bool finished = false;
};

struct ExportOptionsDialogConfig {
    std::wstring current_group_name;
    int all_snippet_count = 0;
    int current_group_snippet_count = 0;
    bool can_export_current_group = false;
    bool dark_theme = false;
};

struct ExportOptionsDialogResult {
    bool accepted = false;
    bool beeftext_format = false;
    bool current_group_only = false;
};

struct ExportOptionsDialogState {
    HINSTANCE instance = nullptr;
    const ExportOptionsDialogConfig* config = nullptr;
    ExportOptionsDialogResult* result = nullptr;
    HWND hwnd = nullptr;
    HFONT font = nullptr;
    HFONT title_font = nullptr;
    HBRUSH background_brush = nullptr;
    HBRUSH input_brush = nullptr;
    HBRUSH surface_brush = nullptr;
    bool finished = false;
};

constexpr int kPromptDialogPrimaryId = 7501;
constexpr int kPromptDialogSecondaryId = 7502;
constexpr int kPromptDialogCancelId = 7503;
constexpr int kPromptDialogWidth = 630;
constexpr int kPromptDialogHeight = 260;

enum class PromptDialogChoice {
    None,
    Primary,
    Secondary,
    Cancel,
};

struct PromptDialogConfig {
    std::wstring title;
    std::wstring message;
    std::wstring primary_label;
    std::wstring secondary_label;
    std::wstring cancel_label = L"Cancel";
    bool show_secondary = false;
    bool primary_destructive = false;
    bool dark_theme = false;
    int width = 0;
    int height = 0;
};

struct PromptDialogResult {
    PromptDialogChoice choice = PromptDialogChoice::None;
};

struct PromptDialogState {
    HINSTANCE instance = nullptr;
    const PromptDialogConfig* config = nullptr;
    PromptDialogResult* result = nullptr;
    HWND hwnd = nullptr;
    HFONT font = nullptr;
    HFONT title_font = nullptr;
    HBRUSH background_brush = nullptr;
    HBRUSH input_brush = nullptr;
    HBRUSH surface_brush = nullptr;
    bool finished = false;
};

struct PopupMenuItemData {
    std::wstring text;
    bool destructive = false;
    bool enabled = true;
    bool separator = false;
    bool has_submenu = false;
};

struct DialogTheme {
    bool dark = false;
    COLORREF background = RGB(244, 246, 250);
    COLORREF surface = RGB(252, 253, 255);
    COLORREF input = RGB(255, 255, 255);
    COLORREF text = RGB(17, 24, 39);
    COLORREF muted = RGB(100, 116, 139);
    COLORREF border = RGB(210, 218, 230);
    COLORREF accent = RGB(70, 102, 174);
};

DialogTheme BuildDialogTheme(bool dark) {
    DialogTheme theme;
    theme.dark = dark;
    if (dark) {
        theme.background = RGB(11, 15, 23);
        theme.surface = RGB(20, 27, 40);
        theme.input = RGB(9, 13, 20);
        theme.text = RGB(241, 245, 249);
        theme.muted = RGB(148, 163, 184);
        theme.border = RGB(56, 69, 92);
        theme.accent = RGB(82, 118, 196);
    }
    return theme;
}

void InitDialogVisuals(HFONT& font, HFONT& title_font, HBRUSH& background_brush, HBRUSH& input_brush, HBRUSH& surface_brush, bool dark) {
    const DialogTheme theme = BuildDialogTheme(dark);
    font = CreateFontW(-ScaleMainUi(18), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    title_font = CreateFontW(-ScaleMainUi(20), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    background_brush = CreateSolidBrush(theme.background);
    input_brush = CreateSolidBrush(theme.input);
    surface_brush = CreateSolidBrush(theme.surface);
}

void ReleaseDialogVisuals(HFONT& font, HFONT& title_font, HBRUSH& background_brush, HBRUSH& input_brush, HBRUSH& surface_brush) {
    if (font != nullptr) {
        DeleteObject(font);
        font = nullptr;
    }
    if (title_font != nullptr) {
        DeleteObject(title_font);
        title_font = nullptr;
    }
    if (background_brush != nullptr) {
        DeleteObject(background_brush);
        background_brush = nullptr;
    }
    if (input_brush != nullptr) {
        DeleteObject(input_brush);
        input_brush = nullptr;
    }
    if (surface_brush != nullptr) {
        DeleteObject(surface_brush);
        surface_brush = nullptr;
    }
}

void ApplyDialogControlTheme(HWND control, bool dark, bool combo = false) {
    if (control == nullptr) {
        return;
    }
    if (combo) {
        SetWindowTheme(control, dark ? L"DarkMode_CFD" : L"", nullptr);
        COMBOBOXINFO info{};
        info.cbSize = sizeof(info);
        if (GetComboBoxInfo(control, &info)) {
            if (info.hwndItem != nullptr) {
                SetWindowTheme(info.hwndItem, dark ? L"DarkMode_Explorer" : L"", nullptr);
            }
            if (info.hwndList != nullptr) {
                SetWindowTheme(info.hwndList, dark ? L"DarkMode_Explorer" : L"", nullptr);
            }
        }
        return;
    }
    SetWindowTheme(control, dark ? L"DarkMode_Explorer" : L"", nullptr);
}

void ApplyPopupMenuTheme(HMENU menu, HBRUSH background_brush) {
    if (menu == nullptr) {
        return;
    }

    MENUINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND | MIM_STYLE;
    info.hbrBack = background_brush != nullptr ? background_brush : reinterpret_cast<HBRUSH>(COLOR_MENU + 1);
    info.dwStyle = MNS_NOCHECK;
    SetMenuInfo(menu, &info);
}

std::wstring MakeUnicodeControlCharactersVisible(const std::wstring& input) {
    std::wstring output;
    output.reserve(input.size() + 32);
    auto append_marker = [&output](const wchar_t* marker) {
        output += marker;
    };
    for (wchar_t ch : input) {
        switch (ch) {
        case 0x200E: append_marker(L"[LRM]"); break;
        case 0x200F: append_marker(L"[RLM]"); break;
        case 0x200C: append_marker(L"[ZWNJ]"); break;
        case 0x200D: append_marker(L"[ZWJ]"); break;
        case 0x2066: append_marker(L"[LRI]"); break;
        case 0x2067: append_marker(L"[RLI]"); break;
        case 0x2068: append_marker(L"[FSI]"); break;
        case 0x2069: append_marker(L"[PDI]"); break;
        case 0x202A: append_marker(L"[LRE]"); break;
        case 0x202B: append_marker(L"[RLE]"); break;
        case 0x202D: append_marker(L"[LRO]"); break;
        case 0x202E: append_marker(L"[RLO]"); break;
        case 0x202C: append_marker(L"[PDF]"); break;
        case 0x2060: append_marker(L"[WJ]"); break;
        case 0xFEFF: append_marker(L"[BOM]"); break;
        default:
            output.push_back(ch);
            break;
        }
    }
    return output;
}

void InsertTextIntoEditControl(HWND control, const std::wstring& text) {
    if (control == nullptr || text.empty()) {
        return;
    }
    SetFocus(control);
    SendMessageW(control, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text.c_str()));
}

bool ToggleEditControlRtlReading(HWND control) {
    if (control == nullptr) {
        return false;
    }
    LONG_PTR ex_style = GetWindowLongPtrW(control, GWL_EXSTYLE);
    LONG_PTR style = GetWindowLongPtrW(control, GWL_STYLE);
    const bool enable_rtl = (ex_style & WS_EX_RTLREADING) == 0;
    if (enable_rtl) {
        ex_style |= WS_EX_RTLREADING | WS_EX_RIGHT;
        style |= ES_RIGHT;
    } else {
        ex_style &= ~static_cast<LONG_PTR>(WS_EX_RTLREADING | WS_EX_RIGHT);
        style &= ~static_cast<LONG_PTR>(ES_RIGHT);
    }
    SetWindowLongPtrW(control, GWL_EXSTYLE, ex_style);
    SetWindowLongPtrW(control, GWL_STYLE, style);
    SetWindowPos(control, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    InvalidateRect(control, nullptr, TRUE);
    UpdateWindow(control);
    return enable_rtl;
}

bool OpenImeForControl(HWND control) {
    if (control == nullptr) {
        return false;
    }
    SetFocus(control);
    HIMC himc = ImmGetContext(control);
    if (himc == nullptr) {
        return false;
    }
    const BOOL result = ImmSetOpenStatus(himc, TRUE);
    ImmReleaseContext(control, himc);
    return result != FALSE;
}

bool RequestImeReconversion(HWND control) {
    if (control == nullptr) {
        return false;
    }
    SetFocus(control);
    const LRESULT result = SendMessageW(control, WM_IME_REQUEST, IMR_RECONVERTSTRING, 0);
    return result != 0;
}

std::wstring ReadUnicodeClipboardText() {
    std::wstring text;
    if (!OpenClipboard(nullptr)) {
        return text;
    }

    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if (handle != nullptr) {
        const auto* data = static_cast<const wchar_t*>(GlobalLock(handle));
        if (data != nullptr) {
            text = data;
            GlobalUnlock(handle);
        }
    }

    CloseClipboard();
    return text;
}

bool WriteUnicodeClipboardText(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    EmptyClipboard();
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (handle == nullptr) {
        CloseClipboard();
        return false;
    }

    void* memory = GlobalLock(handle);
    memcpy(memory, text.c_str(), bytes);
    GlobalUnlock(handle);
    SetClipboardData(CF_UNICODETEXT, handle);
    CloseClipboard();
    return true;
}

RECT BuildDialogWindowRectForClient(int client_width, int client_height, DWORD style, DWORD ex_style) {
    RECT rect{0, 0, client_width, client_height};
    AdjustWindowRectEx(&rect, style, FALSE, ex_style);
    return rect;
}

void DrawThemedDialogButton(
    const DialogTheme& theme,
    HFONT font,
    DRAWITEMSTRUCT* draw,
    bool primary,
    bool destructive
) {
    const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
    const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    const bool focused = (draw->itemState & ODS_FOCUS) != 0;
    bool hovered = false;
    POINT cursor{};
    RECT window_rect{};
    if (GetCursorPos(&cursor) && GetWindowRect(draw->hwndItem, &window_rect)) {
        hovered = PtInRect(&window_rect, cursor) != FALSE;
    }

    auto mix = [](COLORREF a, COLORREF b, int weight_b) {
        const int weight_a = 100 - weight_b;
        return RGB(
            (GetRValue(a) * weight_a + GetRValue(b) * weight_b) / 100,
            (GetGValue(a) * weight_a + GetGValue(b) * weight_b) / 100,
            (GetBValue(a) * weight_a + GetBValue(b) * weight_b) / 100
        );
    };

    COLORREF fill = theme.surface;
    COLORREF border = theme.border;
    COLORREF text = theme.text;
    if (destructive) {
        fill = theme.dark ? RGB(127, 29, 29) : RGB(254, 226, 226);
        border = theme.dark ? RGB(239, 68, 68) : RGB(248, 113, 113);
        text = theme.dark ? RGB(254, 242, 242) : RGB(127, 29, 29);
    } else if (primary) {
        fill = theme.accent;
        border = theme.accent;
        text = RGB(255, 255, 255);
    } else {
        fill = theme.dark ? RGB(39, 51, 72) : RGB(248, 250, 252);
    }

    COLORREF hover_tint = theme.accent;
    if (destructive) {
        hover_tint = border;
    } else if (primary) {
        hover_tint = fill;
    }
    if (hovered && !disabled && !pressed) {
        fill = mix(fill, hover_tint, theme.dark ? 14 : 10);
        border = mix(border, hover_tint, theme.dark ? 34 : 26);
    }
    if (pressed) {
        fill = mix(fill, RGB(0, 0, 0), theme.dark ? 18 : 8);
    }
    if (disabled) {
        fill = mix(fill, theme.background, 45);
        border = mix(border, theme.background, 45);
        text = theme.muted;
    }

    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;

    HBRUSH bg_brush = CreateSolidBrush(theme.background);
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    RECT button_rect = rect;
    InflateRect(&button_rect, -1, -1);

    HBRUSH fill_brush = CreateSolidBrush(fill);
    HPEN border_pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ old_brush = SelectObject(dc, fill_brush);
    HGDIOBJ old_pen = SelectObject(dc, border_pen);

    RoundRect(dc, button_rect.left, button_rect.top, button_rect.right, button_rect.bottom, 10, 10);

    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(fill_brush);
    DeleteObject(border_pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, text);
    if (font != nullptr) {
        SelectObject(dc, font);
    }

    RECT text_rect = button_rect;
    text_rect.left += 8;
    text_rect.right -= 8;

    const int length = GetWindowTextLengthW(draw->hwndItem);
    std::wstring button_text(static_cast<size_t>(std::max(0, length)) + 1, L'\0');

    if (length > 0) {
        GetWindowTextW(draw->hwndItem, button_text.data(), length + 1);
        button_text.resize(static_cast<size_t>(length));
    } else {
        button_text.clear();
    }

    DrawTextW(dc, button_text.c_str(), -1, &text_rect,
            DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
}

void DrawThemedDialogChoiceControl(
    const DialogTheme& theme,
    HFONT font,
    DRAWITEMSTRUCT* draw,
    bool is_radio
) {
    const bool checked = GetPropW(draw->hwndItem, L"BTChecked") != nullptr;
    const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
    const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    const bool focused = (draw->itemState & ODS_FOCUS) != 0;

    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    HBRUSH background_brush = CreateSolidBrush(theme.background);
    FillRect(dc, &rect, background_brush != nullptr ? background_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
    if (background_brush != nullptr) {
        DeleteObject(background_brush);
    }

    RECT indicator_rect{
        rect.left + 2,
        rect.top + (rect.bottom - rect.top - 18) / 2,
        rect.left + 20,
        rect.top + (rect.bottom - rect.top - 18) / 2 + 18
    };

    COLORREF indicator_fill = checked ? theme.accent : theme.input;
    COLORREF indicator_border = checked ? theme.accent : theme.border;
    COLORREF text = disabled ? theme.muted : theme.text;
    if (pressed) {
        indicator_fill = checked
            ? RGB(std::max(0, GetRValue(indicator_fill) - 12), std::max(0, GetGValue(indicator_fill) - 12), std::max(0, GetBValue(indicator_fill) - 12))
            : theme.surface;
    }

    HBRUSH indicator_brush = CreateSolidBrush(indicator_fill);
    HPEN indicator_pen = CreatePen(PS_SOLID, 1, indicator_border);
    HGDIOBJ old_brush = SelectObject(dc, indicator_brush);
    HGDIOBJ old_pen = SelectObject(dc, indicator_pen);
    if (is_radio) {
        Ellipse(dc, indicator_rect.left, indicator_rect.top, indicator_rect.right, indicator_rect.bottom);
    } else {
        RoundRect(dc, indicator_rect.left, indicator_rect.top, indicator_rect.right, indicator_rect.bottom, 4, 4);
    }
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(indicator_brush);
    DeleteObject(indicator_pen);

    if (checked) {
        HBRUSH mark_brush = CreateSolidBrush(RGB(255, 255, 255));
        HPEN mark_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        old_brush = SelectObject(dc, mark_brush);
        old_pen = SelectObject(dc, mark_pen);
        if (is_radio) {
            Ellipse(dc, indicator_rect.left + 4, indicator_rect.top + 4, indicator_rect.right - 4, indicator_rect.bottom - 4);
        } else {
            POINT check_points[] = {
                {indicator_rect.left + 4, indicator_rect.top + 10},
                {indicator_rect.left + 8, indicator_rect.top + 14},
                {indicator_rect.left + 14, indicator_rect.top + 6}
            };
            Polyline(dc, check_points, 3);
        }
        SelectObject(dc, old_brush);
        SelectObject(dc, old_pen);
        DeleteObject(mark_brush);
        DeleteObject(mark_pen);
    }

    RECT text_rect = rect;
    text_rect.left = indicator_rect.right + 8;
    text_rect.right -= 4;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, text);
    if (font != nullptr) {
        SelectObject(dc, font);
    }
    DrawTextW(dc, L"", 0, &text_rect, DT_SINGLELINE); // establish metrics on selected font
    const int length = GetWindowTextLengthW(draw->hwndItem);
    std::wstring label(static_cast<size_t>(std::max(0, length)) + 1, L'\0');
    if (length > 0) {
        GetWindowTextW(draw->hwndItem, label.data(), length + 1);
        label.resize(static_cast<size_t>(length));
    } else {
        label.clear();
    }
    DrawTextW(dc, label.c_str(), -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    if (focused) {
        RECT focus_rect = rect;
        focus_rect.left = indicator_rect.right + 4;
        InflateRect(&focus_rect, -2, -3);
        DrawFocusRect(dc, &focus_rect);
    }
}

void DrawThemedDialogSection(
    const DialogTheme& theme,
    HFONT font,
    DRAWITEMSTRUCT* draw
) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    HBRUSH background_brush = CreateSolidBrush(theme.background);
    FillRect(dc, &rect, background_brush != nullptr ? background_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
    if (background_brush != nullptr) {
        DeleteObject(background_brush);
    }

    const int length = GetWindowTextLengthW(draw->hwndItem);
    std::wstring title(static_cast<size_t>(std::max(0, length)) + 1, L'\0');
    if (length > 0) {
        GetWindowTextW(draw->hwndItem, title.data(), length + 1);
        title.resize(static_cast<size_t>(length));
    } else {
        title.clear();
    }

    SIZE title_size{};
    if (font != nullptr) {
        SelectObject(dc, font);
    }
    GetTextExtentPoint32W(dc, title.c_str(), static_cast<int>(title.size()), &title_size);

    RECT title_rect{rect.left + 14, rect.top, rect.left + 24 + title_size.cx, rect.top + 20};
    HBRUSH title_brush = CreateSolidBrush(theme.background);
    FillRect(dc, &title_rect, title_brush);
    DeleteObject(title_brush);

    HPEN border_pen = CreatePen(PS_SOLID, 1, theme.border);
    HGDIOBJ old_pen = SelectObject(dc, border_pen);
    MoveToEx(dc, rect.left + 8, rect.top + 10, nullptr);
    LineTo(dc, title_rect.left - 6, rect.top + 10);
    MoveToEx(dc, title_rect.right + 6, rect.top + 10, nullptr);
    LineTo(dc, rect.right - 8, rect.top + 10);
    MoveToEx(dc, rect.left + 8, rect.top + 10, nullptr);
    LineTo(dc, rect.left + 8, rect.bottom - 8);
    LineTo(dc, rect.right - 8, rect.bottom - 8);
    LineTo(dc, rect.right - 8, rect.top + 10);
    SelectObject(dc, old_pen);
    DeleteObject(border_pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, theme.text);
    DrawTextW(dc, title.c_str(), -1, &title_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
}

void PaintDialogSectionFrame(
    HDC dc,
    const RECT& rect,
    const wchar_t* title,
    const DialogTheme& theme,
    HFONT font
) {
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return;
    }

    std::wstring title_text = title != nullptr ? title : L"";
    SIZE title_size{};
    HGDIOBJ old_font = nullptr;
    if (font != nullptr) {
        old_font = SelectObject(dc, font);
    }
    GetTextExtentPoint32W(dc, title_text.c_str(), static_cast<int>(title_text.size()), &title_size);

    RECT title_rect{rect.left + 14, rect.top, rect.left + 24 + title_size.cx, rect.top + 20};
    HBRUSH title_brush = CreateSolidBrush(theme.background);
    FillRect(dc, &title_rect, title_brush);
    DeleteObject(title_brush);

    HPEN border_pen = CreatePen(PS_SOLID, 1, theme.border);
    HGDIOBJ old_pen = SelectObject(dc, border_pen);
    MoveToEx(dc, rect.left + 8, rect.top + 10, nullptr);
    LineTo(dc, title_rect.left - 6, rect.top + 10);
    MoveToEx(dc, title_rect.right + 6, rect.top + 10, nullptr);
    LineTo(dc, rect.right - 8, rect.top + 10);
    MoveToEx(dc, rect.left + 8, rect.top + 10, nullptr);
    LineTo(dc, rect.left + 8, rect.bottom - 8);
    LineTo(dc, rect.right - 8, rect.bottom - 8);
    LineTo(dc, rect.right - 8, rect.top + 10);
    SelectObject(dc, old_pen);
    DeleteObject(border_pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, theme.text);
    DrawTextW(dc, title_text.c_str(), -1, &title_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    if (old_font != nullptr) {
        SelectObject(dc, old_font);
    }
}

bool ShowPromptDialogWindow(HWND parent, HINSTANCE instance, const PromptDialogConfig& config, PromptDialogResult& result);

std::wstring TrimWhitespaceCopy(std::wstring value) {
    size_t start = 0;
    while (start < value.size() && iswspace(value[start])) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && iswspace(value[end - 1])) {
        --end;
    }

    return value.substr(start, end - start);
}

LRESULT CALLBACK TextEntryDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<TextEntryDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        state = static_cast<TextEntryDialogState*>(create_struct->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    if (state == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_CREATE: {
        InitDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush, state->config->dark_theme);
        const int margin = 18;
        const int width = kTextEntryDialogWidth;
        const int button_width = 112;
        const int button_height = 34;
        const int button_gap = 12;
        const int buttons_y = kTextEntryDialogHeight - margin - button_height;
        const int edit_width = width - margin * 2;

        HWND prompt_label = CreateWindowExW(0, L"STATIC", state->config->prompt.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, 20, edit_width, 30, hwnd, nullptr, state->instance, nullptr);
        state->edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state->config->initial_value.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            margin, 68, edit_width, 36, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTextEntryEditId)), state->instance, nullptr);
        HWND ok_button = CreateWindowExW(0, L"BUTTON", state->config->ok_button_label.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - (button_width * 2) - button_gap, buttons_y, button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTextEntryOkId)), state->instance, nullptr);
        HWND cancel_button = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - button_width, buttons_y, button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTextEntryCancelId)), state->instance, nullptr);

        SendMessageW(prompt_label, WM_SETFONT, reinterpret_cast<WPARAM>(state->title_font), TRUE);
        SendMessageW(state->edit, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(ok_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(cancel_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SetWindowTheme(hwnd, state->config->dark_theme ? L"DarkMode_Explorer" : L"", nullptr);
        ApplyDialogControlTheme(state->edit, state->config->dark_theme);
        SendMessageW(state->edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));
        if (state->config->dark_theme) {
            const BOOL use_dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));
        }
        InvalidateRect(hwnd, nullptr, TRUE);

        SetFocus(state->edit);
        SendMessageW(state->edit, EM_SETSEL, 0, -1);
        return 0;
    }

    case WM_COMMAND: {
        const int control_id = LOWORD(wparam);
        const int notify = HIWORD(wparam);
        if (control_id == kTextEntryCancelId && notify == BN_CLICKED) {
            DestroyWindow(hwnd);
            return 0;
        }
        if ((control_id == kTextEntryOkId && notify == BN_CLICKED) ||
            (control_id == kTextEntryEditId && notify == EN_MAXTEXT)) {
            wchar_t buffer[512] = {};
            GetWindowTextW(state->edit, buffer, static_cast<int>(std::size(buffer)));
            state->result->text = TrimWhitespaceCopy(buffer);
            state->result->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_KEYDOWN:
        if (wparam == VK_RETURN) {
            PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(kTextEntryOkId, BN_CLICKED), 0);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        FillRect(dc, &rect, state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        if (message == WM_CTLCOLOREDIT) {
            SetTextColor(dc, theme.text);
            SetBkColor(dc, theme.input);
            return reinterpret_cast<INT_PTR>(state->input_brush != nullptr ? state->input_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        }
        SetTextColor(dc, theme.text);
        SetBkColor(dc, theme.background);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    }

    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw == nullptr || draw->CtlType != ODT_BUTTON) {
            break;
        }
        const bool primary = draw->CtlID == kTextEntryOkId;
        DrawThemedDialogButton(BuildDialogTheme(state->config->dark_theme), state->font, draw, primary, false);
        return TRUE;
    }

    case WM_DESTROY:
        ReleaseDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush);
        state->finished = true;
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

void RefreshImportDialogContent(ImportDialogState* state) {
    if (state == nullptr) {
        return;
    }

    if (state->file_edit != nullptr) {
        SetWindowTextW(state->file_edit, state->current_file_path.c_str());
    }

    if (state->group_combo != nullptr) {
        SendMessageW(state->group_combo, CB_RESETCONTENT, 0, 0);
        if (state->current_data.source_groups_available) {
            SendMessageW(state->group_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kKeepSourceGroupsOption));
        }
        for (const auto& group_name : state->config->existing_groups) {
            SendMessageW(state->group_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(group_name.c_str()));
        }

        const std::wstring default_group = state->config->default_group.empty() ? L"General" : state->config->default_group;
        int selected_index = CB_ERR;
        if (state->current_data.source_groups_available) {
            selected_index = 0;
        } else {
            const int item_count = static_cast<int>(SendMessageW(state->group_combo, CB_GETCOUNT, 0, 0));
            for (int index = 0; index < item_count; ++index) {
                wchar_t item_text[512] = {};
                SendMessageW(state->group_combo, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(item_text));
                if (default_group == item_text) {
                    selected_index = index;
                    break;
                }
            }
            if (selected_index == CB_ERR && item_count > 0) {
                selected_index = 0;
            }
        }
        if (selected_index != CB_ERR) {
            SendMessageW(state->group_combo, CB_SETCURSEL, selected_index, 0);
        }
    }

    if (state->imported_label != nullptr) {
        const std::wstring imported_text = L"Imported snippets: " + std::to_wstring(static_cast<int>(state->current_data.snippets.size()));
        SetWindowTextW(state->imported_label, imported_text.c_str());
    }

    if (state->skip_radio != nullptr) {
        const std::wstring skip_label = L"Skip " + std::to_wstring(state->current_conflict_count) + L" conflicting snippets.";
        SetWindowTextW(state->skip_radio, skip_label.c_str());
    }
    if (state->overwrite_radio != nullptr) {
        const std::wstring overwrite_label = L"Overwrite " + std::to_wstring(state->current_conflict_count) + L" conflicting snippets.";
        SetWindowTextW(state->overwrite_radio, overwrite_label.c_str());
    }
}

LRESULT CALLBACK ImportOptionsDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<ImportDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        state = static_cast<ImportDialogState*>(create_struct->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    if (state == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_CREATE: {
        InitDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush, state->config->dark_theme);
        const int margin = 18;
        const int width = kImportOptionsDialogWidth;
        const int button_width = 116;
        const int button_height = 34;
        const int button_gap = 12;
        const int button_y = kImportOptionsDialogHeight - margin - button_height;
        const int field_height = 32;
        const int choice_height = 32;
        const int browse_width = 104;

        state->current_file_path = state->config->file_path;
        state->current_data = state->result->parsed_data;
        state->current_conflict_count = state->config->owner != nullptr
            ? state->config->owner->CountImportConflicts(state->current_data)
            : state->config->conflict_count;

        state->file_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state->config->file_path.c_str(),
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
            margin + 16, 36, width - 2 * margin - 32 - browse_width - 10, field_height, hwnd, nullptr, state->instance, nullptr);
        HWND browse_button = CreateWindowExW(0, L"BUTTON", L"Browse",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - 16 - browse_width, 36, browse_width, field_height, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportDialogBrowseButtonId)), state->instance, nullptr);
        HWND group_label = CreateWindowExW(0, L"STATIC", L"Import into: ", WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin + 16, 118, 128, 22, hwnd, nullptr, state->instance, nullptr);
        state->group_combo = CreateWindowExW(0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            margin + 126, 116, 332, 240, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportDialogGroupComboId)), state->instance, nullptr);

        state->imported_label = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin + 476, 118, 210, 22, hwnd, nullptr, state->instance, nullptr);

        state->skip_radio = CreateWindowExW(0, L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | WS_GROUP,
            margin + 18, 216, width - 2 * margin - 36, choice_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportDialogSkipRadioId)), state->instance, nullptr);
        state->overwrite_radio = CreateWindowExW(0, L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            margin + 18, 254, width - 2 * margin - 36, choice_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportDialogOverwriteRadioId)), state->instance, nullptr);
        SendMessageW(state->skip_radio, BM_SETCHECK, BST_CHECKED, 0);
        SetPropW(state->skip_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
        RemovePropW(state->overwrite_radio, L"BTChecked");

        HWND import_button = CreateWindowExW(0, L"BUTTON", L"Import",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - (button_width * 2) - button_gap, button_y, button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportDialogImportButtonId)), state->instance, nullptr);
        HWND cancel_button = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - button_width, button_y, button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportDialogCancelButtonId)), state->instance, nullptr);

        const HWND controls[] = {
            state->file_edit, browse_button,
            group_label, state->group_combo, state->imported_label,
            state->skip_radio, state->overwrite_radio, import_button, cancel_button
        };
        for (HWND control : controls) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        }
        SetWindowTheme(hwnd, state->config->dark_theme ? L"DarkMode_Explorer" : L"", nullptr);
        ApplyDialogControlTheme(state->file_edit, state->config->dark_theme);
        ApplyDialogControlTheme(state->group_combo, state->config->dark_theme, true);
        SetWindowSubclass(state->file_edit, AppWindow::EditContextSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state->config->owner));
        SetWindowSubclass(state->group_combo, AppWindow::ComboBoxSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state->config->owner));
        SendMessageW(state->file_edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));
        RefreshImportDialogContent(state);
        if (state->config->dark_theme) {
            const BOOL use_dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));
        }
        SetFocus(state->group_combo);
        return 0;
    }

    case WM_COMMAND: {
        const int control_id = LOWORD(wparam);
        const int notify = HIWORD(wparam);
        if (notify == BN_CLICKED) {
            if (control_id == kImportDialogSkipRadioId || control_id == kImportDialogOverwriteRadioId) {
                const HWND skip_radio = GetDlgItem(hwnd, kImportDialogSkipRadioId);
                const HWND overwrite_radio = GetDlgItem(hwnd, kImportDialogOverwriteRadioId);
                if (control_id == kImportDialogSkipRadioId) {
                    SetPropW(skip_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
                    RemovePropW(overwrite_radio, L"BTChecked");
                } else {
                    RemovePropW(skip_radio, L"BTChecked");
                    SetPropW(overwrite_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
                }

                InvalidateRect(skip_radio, nullptr, TRUE);
                InvalidateRect(overwrite_radio, nullptr, TRUE);
                UpdateWindow(skip_radio);
                UpdateWindow(overwrite_radio);
                return 0;
            }
            if (control_id == kImportDialogBrowseButtonId) {
                wchar_t file_path[32768] = {};
                wcsncpy_s(file_path, state->current_file_path.c_str(), _TRUNCATE);
                constexpr wchar_t filter[] = L"JSON files (*.json)\0*.json\0All files (*.*)\0*.*\0";
                OPENFILENAMEW dialog{};
                dialog.lStructSize = sizeof(dialog);
                dialog.hwndOwner = hwnd;
                dialog.lpstrFilter = filter;
                dialog.lpstrFile = file_path;
                dialog.nMaxFile = static_cast<DWORD>(std::size(file_path));
                dialog.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                dialog.lpstrDefExt = L"json";
                if (!GetOpenFileNameW(&dialog)) {
                    return 0;
                }

                AppWindow::ParsedImportData parsed;
                std::wstring error;
                if (state->config->owner == nullptr || !state->config->owner->ParseImportFile(file_path, parsed, error)) {
                    PromptDialogConfig dialog_config;
                    dialog_config.title = L"Import snippets";
                    dialog_config.message = error.empty() ? L"Could not parse the selected file." : error;
                    dialog_config.primary_label = L"OK";
                    dialog_config.dark_theme = state->config->dark_theme;
                    PromptDialogResult dialog_result;
                    ShowPromptDialogWindow(hwnd, state->instance, dialog_config, dialog_result);
                    return 0;
                }

                state->current_file_path = file_path;
                state->current_data = parsed;
                state->current_conflict_count = state->config->owner->CountImportConflicts(state->current_data);
                RefreshImportDialogContent(state);
                InvalidateRect(hwnd, nullptr, TRUE);
                UpdateWindow(hwnd);
                return 0;
            }
        }
        if (control_id == kImportDialogCancelButtonId && notify == BN_CLICKED) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (control_id == kImportDialogImportButtonId && notify == BN_CLICKED) {
            std::wstring group_value;
            const LRESULT selected_index = SendMessageW(state->group_combo, CB_GETCURSEL, 0, 0);
            if (selected_index != CB_ERR) {
                wchar_t buffer[512] = {};
                SendMessageW(state->group_combo, CB_GETLBTEXT, static_cast<WPARAM>(selected_index), reinterpret_cast<LPARAM>(buffer));
                group_value = TrimWhitespaceCopy(buffer);
            }

            state->result->keep_source_groups = state->current_data.source_groups_available && group_value == kKeepSourceGroupsOption;
            state->result->overwrite_conflicts = GetPropW(state->overwrite_radio, L"BTChecked") != nullptr;
            state->result->file_path = state->current_file_path;
            state->result->parsed_data = state->current_data;

            if (!state->result->keep_source_groups) {
                if (group_value.empty()) {
                    group_value = state->config->default_group.empty() ? L"General" : state->config->default_group;
                }
                state->result->target_group = group_value;
            } else {
                state->result->target_group.clear();
            }

            state->result->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        RECT client{};
        GetClientRect(hwnd, &client);
        FillRect(dc, &client, state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        const int margin = 18;
        PaintDialogSectionFrame(dc, RECT{margin, 10, kImportOptionsDialogWidth - margin, 88}, L"File", theme, state->font);
        PaintDialogSectionFrame(dc, RECT{margin, 96, kImportOptionsDialogWidth - margin, 174}, L"Import options", theme, state->font);
        PaintDialogSectionFrame(dc, RECT{margin, 186, kImportOptionsDialogWidth - margin, 304}, L"Conflict resolution", theme, state->font);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        FillRect(dc, &rect, state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX) {
            SetTextColor(dc, theme.text);
            SetBkColor(dc, theme.input);
            return reinterpret_cast<INT_PTR>(state->input_brush != nullptr ? state->input_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        }
        SetTextColor(dc, theme.text);
        SetBkColor(dc, theme.background);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    }

    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw == nullptr || draw->CtlType != ODT_BUTTON) {
            break;
        }
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        switch (draw->CtlID) {
        case kImportDialogSkipRadioId:
        case kImportDialogOverwriteRadioId:
            DrawThemedDialogChoiceControl(theme, state->font, draw, true);
            return TRUE;
        default: {
            const bool primary = draw->CtlID == kImportDialogImportButtonId;
            DrawThemedDialogButton(theme, state->font, draw, primary, false);
            return TRUE;
        }
        }
    }

    case WM_DESTROY:
        ReleaseDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush);
        state->finished = true;
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK DeleteGroupDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<DeleteGroupDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        state = static_cast<DeleteGroupDialogState*>(create_struct->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    if (state == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_CREATE: {
        InitDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush, state->config->dark_theme);
        const int margin = 18;
        const int width = kDeleteGroupDialogWidth;
        const int delete_button_width = 112;
        const int delete_all_button_width = 188;
        const int cancel_button_width = 112;
        const int button_height = 34;
        const int button_gap = 12;
        const int buttons_y = kDeleteGroupDialogHeight - margin - button_height;

        const std::wstring header = L"Delete group '" + state->config->group_name + L"'?";
        HWND header_label = CreateWindowExW(0, L"STATIC", header.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, 18, width - margin * 2, 28, hwnd, nullptr, state->instance, nullptr);

        const std::wstring body = state->config->moved_snippet_count > 0
            ? std::to_wstring(state->config->moved_snippet_count) + L" snippets need a destination group before delete."
            : L"This group has no snippets. You can delete it now.";
        HWND body_label = CreateWindowExW(0, L"STATIC", body.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, 56, width - margin * 2, 40, hwnd, nullptr, state->instance, nullptr);

        HWND target_label = CreateWindowExW(0, L"STATIC", L"Move snippets to", WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, 112, 180, 18, hwnd, nullptr, state->instance, nullptr);

        state->target_combo = CreateWindowExW(0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            margin, 136, width - margin * 2, 260, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDeleteGroupTargetComboId)), state->instance, nullptr);

        for (const auto& group_name : state->config->target_groups) {
            SendMessageW(state->target_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(group_name.c_str()));
        }

        int default_index = CB_ERR;
        for (int index = 0; index < static_cast<int>(state->config->target_groups.size()); ++index) {
            if (state->config->target_groups[index] == state->config->default_target_group) {
                default_index = index;
                break;
            }
        }
        if (default_index == CB_ERR && !state->config->target_groups.empty()) {
            default_index = 0;
        }
        if (default_index != CB_ERR) {
            SendMessageW(state->target_combo, CB_SETCURSEL, default_index, 0);
        }

        if (state->config->moved_snippet_count == 0) {
            EnableWindow(target_label, FALSE);
            EnableWindow(state->target_combo, FALSE);
        }

        HWND warning_label = CreateWindowExW(0, L"STATIC", L"This cannot be undone from the UI.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, 182, width - margin * 2, 22, hwnd, nullptr, state->instance, nullptr);

        const int action_row_width = state->config->moved_snippet_count > 0
            ? (delete_button_width + delete_all_button_width + cancel_button_width + button_gap * 2)
            : (delete_button_width + cancel_button_width + button_gap);
        const int action_row_x = width - margin - action_row_width;

        HWND ok_button = CreateWindowExW(0, L"BUTTON", L"Delete",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            action_row_x, buttons_y, delete_button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDeleteGroupOkId)), state->instance, nullptr);
        HWND delete_all_button = nullptr;
        if (state->config->moved_snippet_count > 0) {
            delete_all_button = CreateWindowExW(0, L"BUTTON", L"Delete all snippets",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                action_row_x + delete_button_width + button_gap, buttons_y, delete_all_button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDeleteGroupDeleteAllId)), state->instance, nullptr);
        }
        HWND cancel_button = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - cancel_button_width, buttons_y, cancel_button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDeleteGroupCancelId)), state->instance, nullptr);

        SendMessageW(header_label, WM_SETFONT, reinterpret_cast<WPARAM>(state->title_font), TRUE);
        SendMessageW(body_label, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(target_label, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(state->target_combo, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(warning_label, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(ok_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        if (delete_all_button != nullptr) {
            SendMessageW(delete_all_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        }
        SendMessageW(cancel_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SetWindowTheme(hwnd, state->config->dark_theme ? L"DarkMode_Explorer" : L"", nullptr);
        ApplyDialogControlTheme(state->target_combo, state->config->dark_theme, true);
        if (state->config->dark_theme) {
            const BOOL use_dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));
        }

        SetFocus(state->config->moved_snippet_count > 0 ? state->target_combo : ok_button);
        return 0;
    }

    case WM_COMMAND: {
        const int control_id = LOWORD(wparam);
        const int notify = HIWORD(wparam);
        if (control_id == kDeleteGroupCancelId && notify == BN_CLICKED) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (control_id == kDeleteGroupDeleteAllId && notify == BN_CLICKED) {
            state->result->delete_all_snippets = true;
            state->result->target_group.clear();
            state->result->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (control_id == kDeleteGroupOkId && notify == BN_CLICKED) {
            if (state->config->moved_snippet_count > 0) {
                const int selected_index = static_cast<int>(SendMessageW(state->target_combo, CB_GETCURSEL, 0, 0));
                if (selected_index == CB_ERR || selected_index < 0 || selected_index >= static_cast<int>(state->config->target_groups.size())) {
                    PromptDialogConfig dialog_config;
                    dialog_config.title = L"Delete Group";
                    dialog_config.message = L"Choose a target group first.";
                    dialog_config.primary_label = L"OK";
                    dialog_config.dark_theme = state->config->dark_theme;
                    PromptDialogResult dialog_result;
                    ShowPromptDialogWindow(hwnd, state->instance, dialog_config, dialog_result);
                    SetFocus(state->target_combo);
                    return 0;
                }
                state->result->target_group = state->config->target_groups[selected_index];
            } else {
                state->result->target_group.clear();
            }
            state->result->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        FillRect(dc, &rect, state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX) {
            SetTextColor(dc, theme.text);
            SetBkColor(dc, theme.input);
            return reinterpret_cast<INT_PTR>(state->input_brush != nullptr ? state->input_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        }
        SetTextColor(dc, theme.text);
        SetBkColor(dc, theme.background);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    }

    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw == nullptr || draw->CtlType != ODT_BUTTON) {
            break;
        }
        const bool destructive = draw->CtlID == kDeleteGroupOkId || draw->CtlID == kDeleteGroupDeleteAllId;
        DrawThemedDialogButton(BuildDialogTheme(state->config->dark_theme), state->font, draw, false, destructive);
        return TRUE;
    }

    case WM_DESTROY:
        ReleaseDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush);
        state->finished = true;
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK ExportOptionsDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<ExportOptionsDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        state = static_cast<ExportOptionsDialogState*>(create_struct->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    if (state == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_CREATE: {
        InitDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush, state->config->dark_theme);
        const int margin = 18;
        const int width = kExportOptionsDialogWidth;
        const int button_width = 118;
        const int button_height = 34;
        const int button_gap = 12;
        const int button_y = kExportOptionsDialogHeight - margin - button_height;
        const int choice_height = 32;

        HWND native_radio = CreateWindowExW(0, L"BUTTON", L"Native BlinkText JSON",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | WS_GROUP,
            margin + 18, 42, width - margin * 2 - 36, choice_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kExportDialogNativeRadioId)), state->instance, nullptr);
        HWND beeftext_radio = CreateWindowExW(0, L"BUTTON", L"Beeftext combos JSON",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            margin + 18, 90, width - margin * 2 - 36, choice_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kExportDialogBeeftextRadioId)), state->instance, nullptr);
        SendMessageW(native_radio, BM_SETCHECK, BST_CHECKED, 0);
        SetPropW(native_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
        RemovePropW(beeftext_radio, L"BTChecked");

        const std::wstring all_label = L"All snippets (" + std::to_wstring(state->config->all_snippet_count) + L")";
        std::wstring current_group_label = L"Current group";
        if (!state->config->current_group_name.empty()) {
            current_group_label += L" '" + state->config->current_group_name + L"'";
        }
        current_group_label += L" (" + std::to_wstring(state->config->current_group_snippet_count) + L")";
        HWND all_radio = CreateWindowExW(0, L"BUTTON", all_label.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | WS_GROUP,
            margin + 18, 156, width - margin * 2 - 36, choice_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kExportDialogAllSnippetsRadioId)), state->instance, nullptr);
        HWND current_group_radio = CreateWindowExW(0, L"BUTTON", current_group_label.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            margin + 18, 194, width - margin * 2 - 36, choice_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kExportDialogCurrentGroupRadioId)), state->instance, nullptr);
        SendMessageW(all_radio, BM_SETCHECK, BST_CHECKED, 0);
        SetPropW(all_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
        RemovePropW(current_group_radio, L"BTChecked");

        if (!state->config->can_export_current_group) {
            EnableWindow(current_group_radio, FALSE);
        }

        HWND export_button = CreateWindowExW(0, L"BUTTON", L"Export",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - (button_width * 2) - button_gap, button_y, button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kExportDialogExportButtonId)), state->instance, nullptr);
        HWND cancel_button = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            width - margin - button_width, button_y, button_width, button_height, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kExportDialogCancelButtonId)), state->instance, nullptr);

        const HWND controls[] = {
            native_radio, beeftext_radio,
            all_radio, current_group_radio,
            export_button, cancel_button
        };
        for (HWND control : controls) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        }
        SetWindowTheme(hwnd, state->config->dark_theme ? L"DarkMode_Explorer" : L"", nullptr);
        if (state->config->dark_theme) {
            const BOOL use_dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));
        }
        SetFocus(native_radio);
        return 0;
    }

    case WM_COMMAND: {
        const int control_id = LOWORD(wparam);
        const int notify = HIWORD(wparam);
        if (notify == BN_CLICKED) {
            if (control_id == kExportDialogNativeRadioId || control_id == kExportDialogBeeftextRadioId) {
                const HWND native_radio = GetDlgItem(hwnd, kExportDialogNativeRadioId);
                const HWND beeftext_radio = GetDlgItem(hwnd, kExportDialogBeeftextRadioId);
                if (control_id == kExportDialogNativeRadioId) {
                    SetPropW(native_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
                    RemovePropW(beeftext_radio, L"BTChecked");
                } else {
                    RemovePropW(native_radio, L"BTChecked");
                    SetPropW(beeftext_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
                }
                InvalidateRect(native_radio, nullptr, TRUE);
                InvalidateRect(beeftext_radio, nullptr, TRUE);
                UpdateWindow(native_radio);
                UpdateWindow(beeftext_radio);
                return 0;
            }
            if (control_id == kExportDialogAllSnippetsRadioId || control_id == kExportDialogCurrentGroupRadioId) {
                const HWND all_radio = GetDlgItem(hwnd, kExportDialogAllSnippetsRadioId);
                const HWND current_group_radio = GetDlgItem(hwnd, kExportDialogCurrentGroupRadioId);
                if (control_id == kExportDialogCurrentGroupRadioId && !IsWindowEnabled(current_group_radio)) {
                    return 0;
                }
                if (control_id == kExportDialogAllSnippetsRadioId) {
                    SetPropW(all_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
                    RemovePropW(current_group_radio, L"BTChecked");
                } else {
                    RemovePropW(all_radio, L"BTChecked");
                    SetPropW(current_group_radio, L"BTChecked", reinterpret_cast<HANDLE>(1));
                }
                InvalidateRect(all_radio, nullptr, TRUE);
                InvalidateRect(current_group_radio, nullptr, TRUE);
                UpdateWindow(all_radio);
                UpdateWindow(current_group_radio);
                return 0;
            }
        }
        if (control_id == kExportDialogCancelButtonId && notify == BN_CLICKED) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (control_id == kExportDialogExportButtonId && notify == BN_CLICKED) {
            state->result->beeftext_format =
                GetPropW(GetDlgItem(hwnd, kExportDialogBeeftextRadioId), L"BTChecked") != nullptr;

            state->result->current_group_only =
                state->config->can_export_current_group &&
                GetPropW(GetDlgItem(hwnd, kExportDialogCurrentGroupRadioId), L"BTChecked") != nullptr;
            state->result->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        RECT client{};
        GetClientRect(hwnd, &client);
        FillRect(dc, &client, state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        const int margin = 18;
        PaintDialogSectionFrame(dc, RECT{margin, 12, kExportOptionsDialogWidth - margin, 130}, L"Format", theme, state->font);
        PaintDialogSectionFrame(dc, RECT{margin, 138, kExportOptionsDialogWidth - margin, 244}, L"Scope", theme, state->font);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        FillRect(dc, &rect, state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        SetTextColor(dc, theme.text);
        SetBkColor(dc, theme.background);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    }

    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw == nullptr || draw->CtlType != ODT_BUTTON) {
            break;
        }
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        switch (draw->CtlID) {
        case kExportDialogNativeRadioId:
        case kExportDialogBeeftextRadioId:
        case kExportDialogAllSnippetsRadioId:
        case kExportDialogCurrentGroupRadioId:
            DrawThemedDialogChoiceControl(theme, state->font, draw, true);
            return TRUE;
        default: {
            const bool primary = draw->CtlID == kExportDialogExportButtonId;
            DrawThemedDialogButton(theme, state->font, draw, primary, false);
            return TRUE;
        }
        }
    }

    case WM_DESTROY:
        ReleaseDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush);
        state->finished = true;
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK PromptDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<PromptDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        state = static_cast<PromptDialogState*>(create_struct->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    if (state == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_CREATE: {
        InitDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush, state->config->dark_theme);
        const int margin = 18;
        const int width = state->config->width > 0 ? state->config->width : kPromptDialogWidth;
        const int height = state->config->height > 0 ? state->config->height : kPromptDialogHeight;
        const int button_height = 36;
        const int button_gap = 12;
        const int button_count = state->config->show_secondary ? 3 : 2;
        const int button_width = state->config->show_secondary ? 138 : 148;
        const int button_row_width = button_count * button_width + (button_count - 1) * button_gap;
        const int button_x = width - margin - button_row_width;
        const int button_y = height - margin - button_height;
        const int message_height = button_y - 36;

        HWND message_label = CreateWindowExW(
            0, L"STATIC", state->config->message.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, 20, width - margin * 2, message_height,
            hwnd, nullptr, state->instance, nullptr
        );
        HWND primary_button = CreateWindowExW(
            0, L"BUTTON", state->config->primary_label.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            button_x, button_y, button_width, button_height,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPromptDialogPrimaryId)), state->instance, nullptr
        );
        HWND secondary_button = nullptr;
        if (state->config->show_secondary) {
            secondary_button = CreateWindowExW(
                0, L"BUTTON", state->config->secondary_label.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                button_x + button_width + button_gap, button_y, button_width, button_height,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPromptDialogSecondaryId)), state->instance, nullptr
            );
        }
        HWND cancel_button = CreateWindowExW(
            0, L"BUTTON", state->config->cancel_label.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            button_x + (state->config->show_secondary ? (button_width + button_gap) * 2 : button_width + button_gap),
            button_y,
            button_width,
            button_height,
            hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPromptDialogCancelId)),
            state->instance,
            nullptr
        );

        SendMessageW(message_label, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(primary_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        if (secondary_button != nullptr) {
            SendMessageW(secondary_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        }
        SendMessageW(cancel_button, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);

        SetWindowTheme(hwnd, state->config->dark_theme ? L"DarkMode_Explorer" : L"", nullptr);
        if (state->config->dark_theme) {
            const BOOL use_dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));
        }
        SetFocus(primary_button);
        return 0;
    }

    case WM_COMMAND: {
        const int control_id = LOWORD(wparam);
        const int notify = HIWORD(wparam);
        if (notify == BN_CLICKED) {
            if (control_id == kPromptDialogPrimaryId) {
                state->result->choice = PromptDialogChoice::Primary;
                DestroyWindow(hwnd);
                return 0;
            }
            if (control_id == kPromptDialogSecondaryId) {
                state->result->choice = PromptDialogChoice::Secondary;
                DestroyWindow(hwnd);
                return 0;
            }
            if (control_id == kPromptDialogCancelId) {
                state->result->choice = PromptDialogChoice::Cancel;
                DestroyWindow(hwnd);
                return 0;
            }
        }
        break;
    }

    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) {
            state->result->choice = PromptDialogChoice::Cancel;
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        state->result->choice = PromptDialogChoice::Cancel;
        DestroyWindow(hwnd);
        return 0;

    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        FillRect(dc, &rect, state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        const DialogTheme theme = BuildDialogTheme(state->config->dark_theme);
        SetTextColor(dc, theme.text);
        SetBkColor(dc, theme.background);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(state->background_brush != nullptr ? state->background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    }

    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw == nullptr || draw->CtlType != ODT_BUTTON) {
            break;
        }
        const bool primary = draw->CtlID == kPromptDialogPrimaryId;
        const bool destructive = primary && state->config->primary_destructive;
        DrawThemedDialogButton(BuildDialogTheme(state->config->dark_theme), state->font, draw, primary && !destructive, destructive);
        return TRUE;
    }

    case WM_DESTROY:
        ReleaseDialogVisuals(state->font, state->title_font, state->background_brush, state->input_brush, state->surface_brush);
        state->finished = true;
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool ShowPromptDialogWindow(HWND parent, HINSTANCE instance, const PromptDialogConfig& config, PromptDialogResult& result) {
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = PromptDialogProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"BlinkTextPromptDialog";
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&window_class);

    RECT parent_rect{};
    GetWindowRect(parent, &parent_rect);
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD ex_style = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    const int client_width = config.width > 0 ? config.width : kPromptDialogWidth;
    const int client_height = config.height > 0 ? config.height : kPromptDialogHeight;
    const RECT window_rect = BuildDialogWindowRectForClient(client_width, client_height, style, ex_style);
    const int width = window_rect.right - window_rect.left;
    const int height = window_rect.bottom - window_rect.top;
    const int x = parent_rect.left + std::max(0L, ((parent_rect.right - parent_rect.left) - width) / 2);
    const int y = parent_rect.top + std::max(0L, ((parent_rect.bottom - parent_rect.top) - height) / 2);

    PromptDialogState state{};
    state.instance = instance;
    state.config = &config;
    state.result = &result;

    HWND dialog = CreateWindowExW(
        ex_style,
        window_class.lpszClassName,
        config.title.c_str(),
        style,
        x, y, width, height,
        parent, nullptr, instance, &state
    );
    if (dialog == nullptr) {
        return false;
    }

    EnableWindow(parent, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG message{};
    while (!state.finished && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    SetForegroundWindow(parent);
    return result.choice != PromptDialogChoice::None;
}

bool ShowImportOptionsDialogWindow(HWND parent, HINSTANCE instance, const ImportDialogConfig& config, ImportDialogResult& result) {
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = ImportOptionsDialogProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"BlinkTextImportOptionsDialog";
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassW(&window_class);

    RECT parent_rect{};
    GetWindowRect(parent, &parent_rect);
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD ex_style = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    const RECT window_rect = BuildDialogWindowRectForClient(kImportOptionsDialogWidth, kImportOptionsDialogHeight, style, ex_style);
    const int width = window_rect.right - window_rect.left;
    const int height = window_rect.bottom - window_rect.top;
    const int x = parent_rect.left + std::max(0L, ((parent_rect.right - parent_rect.left) - width) / 2);
    const int y = parent_rect.top + std::max(0L, ((parent_rect.bottom - parent_rect.top) - height) / 2);

    ImportDialogState state{};
    state.instance = instance;
    state.config = &config;
    state.result = &result;

    HWND dialog = CreateWindowExW(
        ex_style,
        window_class.lpszClassName,
        L"Import snippets",
        style,
        x,
        y,
        width,
        height,
        parent,
        nullptr,
        instance,
        &state
    );
    if (dialog == nullptr) {
        return false;
    }

    EnableWindow(parent, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG message{};
    while (!state.finished && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    SetForegroundWindow(parent);
    return result.accepted;
}

bool ShowTextEntryDialogWindow(HWND parent, HINSTANCE instance, const TextEntryDialogConfig& config, TextEntryDialogResult& result) {
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = TextEntryDialogProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"BlinkTextTextEntryDialog";
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassW(&window_class);

    RECT parent_rect{};
    GetWindowRect(parent, &parent_rect);
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD ex_style = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    const RECT window_rect = BuildDialogWindowRectForClient(kTextEntryDialogWidth, kTextEntryDialogHeight, style, ex_style);
    const int width = window_rect.right - window_rect.left;
    const int height = window_rect.bottom - window_rect.top;
    const int x = parent_rect.left + std::max(0L, ((parent_rect.right - parent_rect.left) - width) / 2);
    const int y = parent_rect.top + std::max(0L, ((parent_rect.bottom - parent_rect.top) - height) / 2);

    TextEntryDialogState state{};
    state.instance = instance;
    state.config = &config;
    state.result = &result;

    HWND dialog = CreateWindowExW(
        ex_style,
        window_class.lpszClassName,
        config.title.c_str(),
        style,
        x,
        y,
        width,
        height,
        parent,
        nullptr,
        instance,
        &state
    );
    if (dialog == nullptr) {
        return false;
    }

    EnableWindow(parent, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG message{};
    while (!state.finished && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    SetForegroundWindow(parent);
    return result.accepted;
}

bool ShowDeleteGroupDialogWindow(HWND parent, HINSTANCE instance, const DeleteGroupDialogConfig& config, DeleteGroupDialogResult& result) {
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = DeleteGroupDialogProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"BlinkTextDeleteGroupDialog";
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassW(&window_class);

    RECT parent_rect{};
    GetWindowRect(parent, &parent_rect);
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD ex_style = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    const RECT window_rect = BuildDialogWindowRectForClient(kDeleteGroupDialogWidth, kDeleteGroupDialogHeight, style, ex_style);
    const int width = window_rect.right - window_rect.left;
    const int height = window_rect.bottom - window_rect.top;
    const int x = parent_rect.left + std::max(0L, ((parent_rect.right - parent_rect.left) - width) / 2);
    const int y = parent_rect.top + std::max(0L, ((parent_rect.bottom - parent_rect.top) - height) / 2);

    DeleteGroupDialogState state{};
    state.instance = instance;
    state.config = &config;
    state.result = &result;

    HWND dialog = CreateWindowExW(
        ex_style,
        window_class.lpszClassName,
        L"Delete Group",
        style,
        x,
        y,
        width,
        height,
        parent,
        nullptr,
        instance,
        &state
    );
    if (dialog == nullptr) {
        return false;
    }

    EnableWindow(parent, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG message{};
    while (!state.finished && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    SetForegroundWindow(parent);
    return result.accepted;
}

bool ShowExportOptionsDialogWindow(HWND parent, HINSTANCE instance, const ExportOptionsDialogConfig& config, ExportOptionsDialogResult& result) {
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = ExportOptionsDialogProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"BlinkTextExportOptionsDialog";
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassW(&window_class);

    RECT parent_rect{};
    GetWindowRect(parent, &parent_rect);
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD ex_style = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    const RECT window_rect = BuildDialogWindowRectForClient(kExportOptionsDialogWidth, kExportOptionsDialogHeight, style, ex_style);
    const int width = window_rect.right - window_rect.left;
    const int height = window_rect.bottom - window_rect.top;
    const int x = parent_rect.left + std::max(0L, ((parent_rect.right - parent_rect.left) - width) / 2);
    const int y = parent_rect.top + std::max(0L, ((parent_rect.bottom - parent_rect.top) - height) / 2);

    ExportOptionsDialogState state{};
    state.instance = instance;
    state.config = &config;
    state.result = &result;

    HWND dialog = CreateWindowExW(
        ex_style,
        window_class.lpszClassName,
        L"Export options",
        style,
        x,
        y,
        width,
        height,
        parent,
        nullptr,
        instance,
        &state
    );
    if (dialog == nullptr) {
        return false;
    }

    EnableWindow(parent, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG message{};
    while (!state.finished && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    SetForegroundWindow(parent);
    return result.accepted;
}


bool AppWindow::Create(HINSTANCE instance) {
    instance_ = instance;
    g_app_window = this;
    LoadAppIcons();
    LoadToolbarButtonImages();
    const POINT effective_minimum = GetEffectiveMinimumWindowSize(nullptr);

    WNDCLASSW window_class{};
    window_class.lpfnWndProc = AppWindow::WindowProc;
    window_class.hInstance = instance_;
    window_class.lpszClassName = L"BlinkTextWindow";
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hIcon = window_icon_big_ != nullptr ? window_icon_big_ : LoadIconW(nullptr, IDI_APPLICATION);
    window_class.hbrBackground = nullptr;

    RegisterClassW(&window_class);

    hwnd_ = CreateWindowExW(
        0,
        window_class.lpszClassName,
        kAppDisplayName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        std::max(static_cast<int>(effective_minimum.x), settings_.window_width),
        std::max(static_cast<int>(effective_minimum.y), settings_.window_height),
        nullptr,
        nullptr,
        instance_,
        this
    );

    if (hwnd_ != nullptr) {
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(window_icon_big_));
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(window_icon_small_));
    }

    if (accelerators_ == nullptr) {
        ACCEL entries[] = {
            {FVIRTKEY | FCONTROL, 'S', kSaveButton},
            {FVIRTKEY | FCONTROL, 'N', kSnippetMenuNew},
            {FVIRTKEY | FCONTROL, 'E', kSnippetMenuEdit},
            {FVIRTKEY | FCONTROL, 'F', kSnippetMenuFocusSearch},
            {FVIRTKEY | FCONTROL, 'A', kAppCommandSelectAll},
            {FVIRTKEY | FCONTROL, 'R', kTestResetButton},
            {FVIRTKEY, VK_F2, kSnippetMenuEdit},
        };
        accelerators_ = CreateAcceleratorTableW(entries, static_cast<int>(std::size(entries)));
    }

    return hwnd_ != nullptr;
}

void AppWindow::Show(int command_show) const {
    ShowWindow(hwnd_, command_show);
    UpdateWindow(hwnd_);
}

bool AppWindow::TranslateAppAccelerator(MSG* message) const {
    if (message == nullptr || accelerators_ == nullptr || hwnd_ == nullptr || is_capturing_hotkey_) {
        return false;
    }
    return TranslateAcceleratorW(hwnd_, accelerators_, message) != 0;
}

void AppWindow::ShowInfoDialog() const {
    PromptDialogConfig dialog_config;
    dialog_config.title = L"About BlinkText";
    dialog_config.message =
        L"BlinkText v1.0\r\n"
        L"Fast, local text expansion tool designed for instant, reliable typing with zero delay.\r\n\r\n"
        L"Key Features\r\n"
        L"\u2022 Instant expansion without pastes conflicts\r\n"
        L"\u2022 No trigger duplication during rapid input (e.g., Ctrl+V after trigger)\r\n"
        L"\u2022 Fully offline - no data collection\r\n"
        L"\u2022 Lightweight and optimized for speed\r\n"
        L"\u2022 Supports importing exported triggers from compatible tools\r\n\r\n"
        L"Compatibility\r\n"
        L"\u2022 Supports importing exported triggers from Beeftext for seamless migration\r\n\r\n"
        L"Privacy\r\n"
        L"\u2022 No user data is collected, stored, or transmitted\r\n\r\n"
        L"Developer\r\n"
        L"\u2022 Developed by: Eslam Mustafa\r\n"
        L"\u2022 Contact: Eslam.Youssef@protonmail.com / Eslam.G.Youssef@gmail.com\r\n"
        L"\u2022 GitHub: https://github.com/LeaDer-E";
    dialog_config.primary_label = L"OK";
    dialog_config.dark_theme = IsDarkTheme();
    dialog_config.width = 760;
    dialog_config.height = 520;

    PromptDialogResult result;
    ShowPromptDialogWindow(hwnd_, instance_, dialog_config, result);
}

bool AppWindow::IsDarkTheme() const {
    const std::wstring normalized = ToLowerCopy(settings_.theme);
    return normalized.empty() || normalized == L"dark";
}

void AppWindow::ReleaseThemeBrushes() {
    if (background_brush_ != nullptr) {
        DeleteObject(background_brush_);
        background_brush_ = nullptr;
    }
    if (surface_brush_ != nullptr) {
        DeleteObject(surface_brush_);
        surface_brush_ = nullptr;
    }
    if (input_brush_ != nullptr) {
        DeleteObject(input_brush_);
        input_brush_ = nullptr;
    }
}

void AppWindow::ApplyTheme() {
    if (IsDarkTheme()) {
        settings_.theme = L"dark";
        background_color_ = RGB(10, 14, 22);
        surface_color_ = RGB(18, 24, 35);
        input_color_ = RGB(11, 16, 24);
        text_color_ = RGB(244, 247, 251);
        muted_text_color_ = RGB(146, 160, 182);
        border_color_ = RGB(61, 75, 98);
        header_color_ = RGB(15, 21, 32);
        accent_color_ = RGB(76, 116, 214);
        selection_color_ = RGB(71, 89, 119);
        selection_text_color_ = RGB(255, 255, 255);
    } else {
        settings_.theme = L"light";
        background_color_ = RGB(246, 248, 252);
        surface_color_ = RGB(255, 255, 255);
        input_color_ = RGB(255, 255, 255);
        text_color_ = RGB(20, 28, 45);
        muted_text_color_ = RGB(98, 112, 133);
        border_color_ = RGB(209, 217, 228);
        header_color_ = RGB(239, 243, 248);
        accent_color_ = RGB(56, 102, 220);
        selection_color_ = RGB(217, 228, 244);
        selection_text_color_ = RGB(20, 28, 45);
    }

    ReleaseThemeBrushes();
    background_brush_ = CreateSolidBrush(background_color_);
    surface_brush_ = CreateSolidBrush(surface_color_);
    input_brush_ = CreateSolidBrush(input_color_);

    if (theme_button_ != nullptr) {
        SetWindowTextW(theme_button_, (L"Theme: " + TitleCaseCopy(settings_.theme)).c_str());
    }
    if (snippets_list_ != nullptr) {
        ListView_SetTextColor(snippets_list_, text_color_);
        ListView_SetTextBkColor(snippets_list_, CLR_NONE);
        ListView_SetBkColor(snippets_list_, input_color_);
        if (const HWND header = ListView_GetHeader(snippets_list_); header != nullptr) {
            SendMessageW(header, HDM_SETBKCOLOR, 0, static_cast<LPARAM>(header_color_));
            SendMessageW(header, HDM_SETTEXTCOLOR, 0, static_cast<LPARAM>(text_color_));
            InvalidateRect(header, nullptr, TRUE);
            RedrawWindow(header, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
    }
    if (hwnd_ != nullptr) {
        if (IsDarkTheme()) {
            SetWindowTheme(hwnd_, L"DarkMode_Explorer", nullptr);
        } else {
            SetWindowTheme(hwnd_, L"", nullptr);
        }
    }

    auto apply_native_theme = [this](HWND control) {
        if (control == nullptr) {
            return;
        }
        if (control == group_combo_) {
            if (IsDarkTheme()) {
                SetWindowTheme(control, L"DarkMode_CFD", nullptr);
            } else {
                SetWindowTheme(control, L"", nullptr);
            }
            return;
        }
        if (control == snippets_list_) {
            if (IsDarkTheme()) {
                SetWindowTheme(control, L"DarkMode_Explorer", nullptr);
            } else {
                SetWindowTheme(control, L"", nullptr);
            }
            return;
        }
        if (control == ListView_GetHeader(snippets_list_)) {
            if (IsDarkTheme()) {
                SetWindowTheme(control, L"DarkMode_Explorer", nullptr);
            } else {
                SetWindowTheme(control, L"", nullptr);
            }
            return;
        }
        if (IsDarkTheme()) {
            SetWindowTheme(control, L"DarkMode_Explorer", nullptr);
        } else {
            SetWindowTheme(control, L"", nullptr);
        }
    };
    if (snippets_list_ != nullptr) {
        apply_native_theme(ListView_GetHeader(snippets_list_));
    }
    apply_native_theme(groups_list_);
    apply_native_theme(search_edit_);
    apply_native_theme(restore_edit_);
    apply_native_theme(trigger_edit_);
    apply_native_theme(notes_edit_);
    apply_native_theme(content_edit_);
    apply_native_theme(test_edit_);
    apply_native_theme(group_combo_);
    if (group_combo_ != nullptr) {
        COMBOBOXINFO combo_info{};
        combo_info.cbSize = sizeof(combo_info);
        if (GetComboBoxInfo(group_combo_, &combo_info)) {
            apply_native_theme(combo_info.hwndItem);
            apply_native_theme(combo_info.hwndList);
        }
        InvalidateRect(group_combo_, nullptr, TRUE);
        RedrawWindow(group_combo_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
    apply_native_theme(snippets_list_);
    apply_native_theme(editor_settings_scrollbar_);
    if (search_edit_ != nullptr) {
        InvalidateRect(search_edit_, nullptr, TRUE);
    }

    if (hwnd_ != nullptr) {
        UpdateWindowFrameTheme();
        UpdateWindowTitle();
        InvalidateRect(hwnd_, nullptr, FALSE);
        RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}

std::wstring AppWindow::CurrentSearchQuery() const {
    if (search_placeholder_active_) {
        return L"";
    }
    return TrimCopy(ReadWindowText(search_edit_));
}

void AppWindow::ShowSearchPlaceholder() {
    if (search_edit_ == nullptr) {
        return;
    }

    suppress_search_placeholder_updates_ = true;
    SetWindowTextW(search_edit_, kSearchPlaceholderText);
    SendMessageW(search_edit_, EM_SETSEL, 0, 0);
    suppress_search_placeholder_updates_ = false;
    search_placeholder_active_ = true;
    InvalidateRect(search_edit_, nullptr, TRUE);
}

void AppWindow::HideSearchPlaceholder() {
    if (search_edit_ == nullptr || !search_placeholder_active_) {
        return;
    }

    suppress_search_placeholder_updates_ = true;
    SetWindowTextW(search_edit_, L"");
    suppress_search_placeholder_updates_ = false;
    search_placeholder_active_ = false;
    InvalidateRect(search_edit_, nullptr, TRUE);
}

void AppWindow::UpdateWindowFrameTheme() const {
    if (hwnd_ == nullptr) {
        return;
    }

    const BOOL use_dark = FALSE;
    DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));

    const COLORREF caption_color = RGB(255, 255, 255);
    const COLORREF caption_text_color = RGB(15, 23, 42);
    const COLORREF border = RGB(218, 223, 232);
    DwmSetWindowAttribute(hwnd_, DWMWA_CAPTION_COLOR, &caption_color, sizeof(caption_color));
    DwmSetWindowAttribute(hwnd_, DWMWA_TEXT_COLOR, &caption_text_color, sizeof(caption_text_color));
    DwmSetWindowAttribute(hwnd_, DWMWA_BORDER_COLOR, &border, sizeof(border));
    SetWindowPos(
        hwnd_,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
    );
    DwmFlush();
}

LRESULT CALLBACK AppWindow::SnippetsHeaderSubclassProc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR subclass_id,
    DWORD_PTR ref_data
) {
    auto* self = reinterpret_cast<AppWindow*>(ref_data);
    if (self == nullptr) {
        return DefSubclassProc(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        HBRUSH fill_brush = CreateSolidBrush(self->header_color_);
        FillRect(dc, &rect, fill_brush != nullptr ? fill_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
        if (fill_brush != nullptr) {
            DeleteObject(fill_brush);
        }
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client{};
        GetClientRect(hwnd, &client);

        HBRUSH fill_brush = CreateSolidBrush(self->header_color_);
        FillRect(dc, &client, fill_brush != nullptr ? fill_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
        if (fill_brush != nullptr) {
            DeleteObject(fill_brush);
        }

        const int column_count = Header_GetItemCount(hwnd);
        for (int index = 0; index < column_count; ++index) {
            RECT rect{};
            if (!Header_GetItemRect(hwnd, index, &rect)) {
                continue;
            }

            HPEN border_pen = CreatePen(PS_SOLID, 1, self->border_color_);
            HGDIOBJ old_pen = SelectObject(dc, border_pen);
            MoveToEx(dc, rect.left, rect.bottom - 1, nullptr);
            LineTo(dc, rect.right, rect.bottom - 1);
            MoveToEx(dc, rect.right - 1, rect.top, nullptr);
            LineTo(dc, rect.right - 1, rect.bottom);
            SelectObject(dc, old_pen);
            DeleteObject(border_pen);

            wchar_t text_buffer[128] = {};
            HDITEMW item{};
            item.mask = HDI_TEXT | HDI_FORMAT;
            item.pszText = text_buffer;
            item.cchTextMax = static_cast<int>(std::size(text_buffer));
            Header_GetItem(hwnd, index, &item);

            RECT text_rect = rect;
            text_rect.left += 10;
            text_rect.right -= 10;

            const bool sort_up = (item.fmt & HDF_SORTUP) != 0;
            const bool sort_down = (item.fmt & HDF_SORTDOWN) != 0;
            if (sort_up || sort_down) {
                POINT arrow[3]{};
                const int arrow_mid_x = rect.right - 12;
                const int arrow_mid_y = rect.top + ((rect.bottom - rect.top) / 2);
                if (sort_up) {
                    arrow[0] = {arrow_mid_x - 4, arrow_mid_y + 2};
                    arrow[1] = {arrow_mid_x + 4, arrow_mid_y + 2};
                    arrow[2] = {arrow_mid_x, arrow_mid_y - 3};
                } else {
                    arrow[0] = {arrow_mid_x - 4, arrow_mid_y - 2};
                    arrow[1] = {arrow_mid_x + 4, arrow_mid_y - 2};
                    arrow[2] = {arrow_mid_x, arrow_mid_y + 3};
                }
                HBRUSH arrow_brush = CreateSolidBrush(self->accent_color_);
                HGDIOBJ old_brush = SelectObject(dc, arrow_brush);
                Polygon(dc, arrow, 3);
                SelectObject(dc, old_brush);
                DeleteObject(arrow_brush);
                text_rect.right -= 16;
            }

            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, self->text_color_);
            if (self->ui_font_ != nullptr) {
                SelectObject(dc, self->ui_font_);
            }
            DrawTextW(dc, text_buffer, -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, SnippetsHeaderSubclassProc, subclass_id);
        break;

    default:
        break;
    }

    return DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK AppWindow::ComboBoxSubclassProc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR subclass_id,
    DWORD_PTR ref_data
) {
    auto* self = reinterpret_cast<AppWindow*>(ref_data);
    switch (message) {
    case WM_PAINT:
    case WM_PRINTCLIENT: {
        const LRESULT result = DefSubclassProc(hwnd, message, wparam, lparam);
        if (self == nullptr) {
            return result;
        }

        HDC dc = message == WM_PRINTCLIENT ? reinterpret_cast<HDC>(wparam) : GetDC(hwnd);
        if (dc == nullptr) {
            return result;
        }

        COMBOBOXINFO info{};
        info.cbSize = sizeof(info);
        if (GetComboBoxInfo(hwnd, &info)) {
            RECT client_rect{};
            GetClientRect(hwnd, &client_rect);

            RECT button_rect = info.rcButton;
            MapWindowPoints(HWND_DESKTOP, hwnd, reinterpret_cast<POINT*>(&button_rect), 2);

            const bool focused = GetFocus() == hwnd || GetFocus() == info.hwndItem;
            const COLORREF border = focused ? self->accent_color_ : self->border_color_;
            const COLORREF button_fill = self->surface_color_;

            HBRUSH button_brush = CreateSolidBrush(button_fill);
            FillRect(dc, &button_rect, button_brush != nullptr ? button_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
            if (button_brush != nullptr) {
                DeleteObject(button_brush);
            }

            HPEN border_pen = CreatePen(PS_SOLID, 1, border);
            HGDIOBJ old_pen = SelectObject(dc, border_pen);
            HGDIOBJ old_brush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
            RoundRect(dc, client_rect.left, client_rect.top, client_rect.right, client_rect.bottom, 8, 8);
            MoveToEx(dc, button_rect.left, button_rect.top + 4, nullptr);
            LineTo(dc, button_rect.left, button_rect.bottom - 4);
            SelectObject(dc, old_brush);
            SelectObject(dc, old_pen);
            DeleteObject(border_pen);

            const int arrow_mid_x = (button_rect.left + button_rect.right) / 2;
            const int arrow_mid_y = (button_rect.top + button_rect.bottom) / 2;
            HPEN arrow_pen = CreatePen(PS_SOLID, 2, self->text_color_);
            HGDIOBJ old_arrow_pen = SelectObject(dc, arrow_pen);
            MoveToEx(dc, arrow_mid_x - 5, arrow_mid_y - 1, nullptr);
            LineTo(dc, arrow_mid_x, arrow_mid_y + 4);
            LineTo(dc, arrow_mid_x + 5, arrow_mid_y - 1);
            SelectObject(dc, old_arrow_pen);
            DeleteObject(arrow_pen);
        }

        if (message == WM_PAINT) {
            ReleaseDC(hwnd, dc);
        }
        return result;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, ComboBoxSubclassProc, subclass_id);
        break;

    default:
        break;
    }

    return DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK AppWindow::EditContextSubclassProc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR subclass_id,
    DWORD_PTR ref_data
) {
    auto* self = reinterpret_cast<AppWindow*>(ref_data);
    switch (message) {
    case WM_CONTEXTMENU:
        if (self != nullptr) {
            POINT screen_point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            if (screen_point.x == -1 && screen_point.y == -1) {
                RECT edit_rect{};
                GetWindowRect(hwnd, &edit_rect);
                screen_point.x = edit_rect.left + 18;
                screen_point.y = edit_rect.top + 18;
            }
            self->ShowEditContextMenu(hwnd, screen_point);
            return 0;
        }
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, EditContextSubclassProc, subclass_id);
        break;

    default:
        break;
    }

    return DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK AppWindow::InteractiveControlSubclassProc(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR subclass_id,
    DWORD_PTR ref_data
) {
    auto* self = reinterpret_cast<AppWindow*>(ref_data);
    switch (message) {
    case WM_MOUSEMOVE:
        if (self != nullptr) {
            if (self->hovered_interactive_control_ != hwnd) {
                HWND previous = self->hovered_interactive_control_;
                self->hovered_interactive_control_ = hwnd;
                if (previous != nullptr && IsWindow(previous)) {
                    InvalidateRect(previous, nullptr, TRUE);
                }
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            TRACKMOUSEEVENT tracking{};
            tracking.cbSize = sizeof(tracking);
            tracking.dwFlags = TME_LEAVE;
            tracking.hwndTrack = hwnd;
            TrackMouseEvent(&tracking);
        }
        break;

    case WM_MOUSELEAVE:
        if (self != nullptr && self->hovered_interactive_control_ == hwnd) {
            self->hovered_interactive_control_ = nullptr;
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;

    case WM_NCDESTROY:
        if (self != nullptr && self->hovered_interactive_control_ == hwnd) {
            self->hovered_interactive_control_ = nullptr;
        }
        RemoveWindowSubclass(hwnd, InteractiveControlSubclassProc, subclass_id);
        break;

    default:
        break;
    }

    return DefSubclassProc(hwnd, message, wparam, lparam);
}

void AppWindow::SyncWindowScrollBar(int viewport_height, int content_height) {
    if (hwnd_ == nullptr) {
        return;
    }

    window_scroll_max_ = std::max(0, content_height - viewport_height);
    window_scroll_offset_ = std::clamp(window_scroll_offset_, 0, window_scroll_max_);

    if (viewport_height <= 0 || window_scroll_max_ <= 0) {
        ShowScrollBar(hwnd_, SB_VERT, FALSE);
        SetScrollPos(hwnd_, SB_VERT, 0, FALSE);
        return;
    }

    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMin = 0;
    info.nMax = std::max(0, content_height - 1);
    info.nPage = static_cast<UINT>(viewport_height);
    info.nPos = window_scroll_offset_;
    SetScrollInfo(hwnd_, SB_VERT, &info, TRUE);
    ShowScrollBar(hwnd_, SB_VERT, TRUE);
}

void AppWindow::SyncEditorSettingsScrollBar(int viewport_height, int content_height) {
    if (editor_settings_scrollbar_ == nullptr) {
        return;
    }

    editor_settings_scroll_max_ = std::max(0, content_height - viewport_height);
    editor_settings_scroll_offset_ = std::clamp(editor_settings_scroll_offset_, 0, editor_settings_scroll_max_);

    if (viewport_height <= 0 || editor_settings_scroll_max_ <= 0) {
        ShowWindow(editor_settings_scrollbar_, SW_HIDE);
        SetScrollPos(editor_settings_scrollbar_, SB_CTL, 0, FALSE);
        return;
    }

    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMin = 0;
    info.nMax = std::max(0, content_height - 1);
    info.nPage = static_cast<UINT>(viewport_height);
    info.nPos = editor_settings_scroll_offset_;
    SetScrollInfo(editor_settings_scrollbar_, SB_CTL, &info, TRUE);
    ShowWindow(editor_settings_scrollbar_, SW_SHOWNA);
}

void AppWindow::LayoutEditorSettingsSection() {
    const int button_height = 36;
    const int input_height = 32;
    const int label_height = 22;
    const int checkbox_height = 24;
    const int row_gap = 8;
    const int settings_gap = 12;
    const int separator_gap = 10;

    if (engine_settings_card_rect_.right <= engine_settings_card_rect_.left ||
        engine_settings_card_rect_.bottom <= engine_settings_card_rect_.top) {
        return;
    }

    const int settings_x = engine_settings_card_rect_.left + 12;
    const int settings_inner_right = engine_settings_card_rect_.right - 12;
    const int settings_width = std::max(0, settings_inner_right - settings_x);
    const int settings_title_y = engine_settings_card_rect_.top + 12;
    const int footer_y = engine_settings_card_rect_.bottom - 14 - button_height;
    const int content_top = settings_title_y + 38;
    const int settings_column_width = std::max(138, (settings_width - settings_gap) / 2);
    const int separator_width = std::max(100, (settings_width - separator_gap * 2) / 3);

    if (engine_settings_title_ != nullptr) {
        SetWindowPos(
            engine_settings_title_,
            nullptr,
            settings_x,
            settings_title_y,
            settings_width,
            24,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW
        );
    }

    if (settings_save_button_ != nullptr) {
        SetWindowPos(
            settings_save_button_,
            nullptr,
            settings_x,
            footer_y,
            settings_column_width,
            button_height,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW
        );
    }
    if (settings_reset_button_ != nullptr) {
        SetWindowPos(
            settings_reset_button_,
            nullptr,
            settings_x + settings_column_width + settings_gap,
            footer_y,
            settings_column_width,
            button_height,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW
        );
    }

    if (editor_settings_scrollbar_ != nullptr) {
        editor_settings_scroll_offset_ = 0;
        editor_settings_scroll_max_ = 0;
        ShowWindow(editor_settings_scrollbar_, SW_HIDE);
        SetScrollPos(editor_settings_scrollbar_, SB_CTL, 0, FALSE);
    }

    int logical_y = 0;
    const int restore_row_y = logical_y;
    logical_y += input_height + 8;
    const int hotkey_row_y = logical_y;
    logical_y += 22 + 10;
    const int record_button_y = logical_y;
    logical_y += button_height + 10;
    const int always_on_top_y = logical_y;
    logical_y += checkbox_height + 6;
    const int start_with_windows_y = logical_y;
    logical_y += checkbox_height + 6;
    const int minimize_to_tray_y = logical_y;
    logical_y += checkbox_height + 6;
    const int previous_clipboard_y = logical_y;
    logical_y += checkbox_height + 6;
    const int previous_clipboard_slash_y = logical_y;
    logical_y += checkbox_height + 6;
    const int case_sensitive_y = logical_y;
    logical_y += checkbox_height + row_gap;
    const int trigger_mode_label_y = logical_y;
    logical_y += label_height + 4;
    const int trigger_mode_options_y = logical_y;
    logical_y += checkbox_height + row_gap;
    const int separator_label_y = logical_y;
    logical_y += label_height + 4;
    const int separator_options_y = logical_y;
    logical_y += checkbox_height + 4;

    auto place_control = [&](HWND control, int x_pos, int logical_top, int control_width, int control_height) {
        if (control == nullptr) {
            return;
        }

        SetWindowPos(
            control,
            nullptr,
            x_pos,
            content_top + logical_top,
            control_width,
            control_height,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW
        );
    };

    place_control(restore_label_, settings_x, restore_row_y + 5, 64, label_height);
    place_control(restore_edit_, settings_x + 70, restore_row_y, 82, input_height);
    place_control(hotkey_label_, settings_x, hotkey_row_y, settings_width, 22);
    place_control(record_hotkey_button_, settings_x, record_button_y, settings_width, button_height);
    place_control(always_on_top_checkbox_, settings_x, always_on_top_y, settings_width, checkbox_height);
    place_control(start_with_windows_checkbox_, settings_x, start_with_windows_y, settings_width, checkbox_height);
    place_control(minimize_to_tray_checkbox_, settings_x, minimize_to_tray_y, settings_width, checkbox_height);
    place_control(previous_clipboard_checkbox_, settings_x, previous_clipboard_y, settings_width, checkbox_height);
    place_control(previous_clipboard_slash_checkbox_, settings_x, previous_clipboard_slash_y, settings_width, checkbox_height);
    place_control(case_sensitive_checkbox_, settings_x, case_sensitive_y, settings_width, checkbox_height);
    place_control(trigger_mode_label_, settings_x, trigger_mode_label_y, settings_width, label_height);
    place_control(instant_mode_radio_, settings_x, trigger_mode_options_y, settings_column_width, checkbox_height);
    place_control(separator_mode_radio_, settings_x + settings_column_width + settings_gap, trigger_mode_options_y, settings_column_width, checkbox_height);
    place_control(separator_keys_label_, settings_x, separator_label_y, settings_width, label_height);
    place_control(separator_space_checkbox_, settings_x, separator_options_y, separator_width, checkbox_height);
    place_control(separator_enter_checkbox_, settings_x + separator_width + separator_gap, separator_options_y, separator_width, checkbox_height);
    place_control(separator_tab_checkbox_, settings_x + (separator_width + separator_gap) * 2, separator_options_y, separator_width, checkbox_height);
}

bool AppWindow::UsesSurfaceBackground(HWND control) const {
    return control != nullptr &&
        control != title_label_ &&
        control != subtitle_label_ &&
        control != status_label_;
}

void AppWindow::MarkEditorClean() {
    editor_dirty_ = false;
    RefreshEditorBanner();
    RefreshSnippetActionButtons();
}

void AppWindow::MarkEditorDirty() {
    if (!suppress_editor_change_tracking_) {
        editor_dirty_ = true;
        RefreshEditorBanner();
        RefreshSnippetActionButtons();
    }
}

bool AppWindow::ConfirmDiscardEditorChanges(const wchar_t* action_label) {
    if (!editor_dirty_) {
        return true;
    }

    std::wstring message = L"You have unsaved changes in Snippet Manager.";
    if (action_label != nullptr && *action_label != L'\0') {
        message += L"\n\nYes: save and continue ";
        message += std::wstring(action_label);
        message += L"\nNo: discard changes and continue";
        message += L"\nCancel: stay here";
    } else {
        message += L"\n\nYes: save and continue";
        message += L"\nNo: discard changes and continue";
        message += L"\nCancel: stay here";
    }

    PromptDialogConfig dialog_config;
    dialog_config.title = kAppDisplayName;
    dialog_config.message = message;
    dialog_config.primary_label = L"Save";
    dialog_config.secondary_label = L"Discard";
    dialog_config.cancel_label = L"Cancel";
    dialog_config.show_secondary = true;
    dialog_config.dark_theme = IsDarkTheme();

    PromptDialogResult result;
    if (!ShowPromptDialogWindow(hwnd_, instance_, dialog_config, result)) {
        SetStatusText(L"Action cancelled.");
        return false;
    }

    if (result.choice == PromptDialogChoice::Primary) {
        return SaveCurrentSnippet();
    }
    if (result.choice == PromptDialogChoice::Secondary) {
        MarkEditorClean();
        return true;
    }
    SetStatusText(L"Action cancelled.");
    return false;
}

LRESULT CALLBACK AppWindow::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    AppWindow* self = nullptr;

    if (message == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<AppWindow*>(create_struct->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->HandleMessage(message, wparam, lparam);
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT AppWindow::HandleMessage(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        ui_font_ = CreateFontW(-ScaleMainUi(18), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        popup_menu_font_ = CreateFontW(-ScaleMainUi(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        title_font_ = CreateFontW(-ScaleMainUi(34), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        section_font_ = CreateFontW(-ScaleMainUi(26), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        CreateChildren();
        LoadData();
        ApplyLoadedSettings();
        ApplyFonts();
        RefreshEngineButton();
        RefreshGroups();
        RefreshSnippetList(false);
        CreateNewSnippet();
        AddClipboardFormatListener(hwnd_);
        latest_clipboard_text_ = ReadUnicodeClipboardText();
        previous_clipboard_text_.clear();

        const bool services_started = StartGlobalServices();
        const bool tray_ready = AddTrayIcon();
        std::wstring status = pending_status_text_.empty()
            ? std::wstring(kAppDisplayName) + L" is ready."
            : pending_status_text_;
        if (!services_started) {
            status += L" Global hotkey or keyboard hook could not start.";
        }
        if (!tray_ready) {
            status += L" Tray icon could not start.";
        }
        SetStatusText(status.c_str());
        return 0;
    }

    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
        const POINT effective_minimum = GetEffectiveMinimumWindowSize(hwnd_);
        info->ptMinTrackSize.x = effective_minimum.x;
        info->ptMinTrackSize.y = effective_minimum.y;
        return 0;
    }

    case WM_SIZE:
        if (wparam == SIZE_MINIMIZED && settings_.minimize_to_tray) {
            MinimizeToTray();
            return 0;
        }
        window_scroll_offset_ = 0;
        ClampPanelSizes(LOWORD(lparam), HIWORD(lparam));
        LayoutChildren(LOWORD(lparam), HIWORD(lparam));
        RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd_, &ps);
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        FillRect(dc, &rect, background_brush_ != nullptr ? background_brush_ : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

        auto draw_card = [&](const RECT& card_rect, bool emphasize = false) {
            if (card_rect.right <= card_rect.left || card_rect.bottom <= card_rect.top) {
                return;
            }

            const COLORREF fill = surface_color_;
            const COLORREF border = emphasize
                ? (IsDarkTheme() ? RGB(79, 101, 136) : RGB(160, 173, 191))
                : border_color_;
            if (!IsDarkTheme()) {
                const COLORREF shadow = RGB(229, 234, 241);
                RECT shadow_rect = card_rect;
                OffsetRect(&shadow_rect, 0, 2);
                HBRUSH shadow_brush = CreateSolidBrush(shadow);
                HPEN shadow_pen = CreatePen(PS_SOLID, 1, shadow);
                HGDIOBJ old_shadow_brush = SelectObject(dc, shadow_brush);
                HGDIOBJ old_shadow_pen = SelectObject(dc, shadow_pen);
                RoundRect(dc, shadow_rect.left, shadow_rect.top, shadow_rect.right, shadow_rect.bottom, kCardRadius, kCardRadius);
                SelectObject(dc, old_shadow_brush);
                SelectObject(dc, old_shadow_pen);
                DeleteObject(shadow_brush);
                DeleteObject(shadow_pen);
            }

            HBRUSH fill_brush = CreateSolidBrush(fill);
            HPEN border_pen = CreatePen(PS_SOLID, 1, border);
            HGDIOBJ old_brush = SelectObject(dc, fill_brush);
            HGDIOBJ old_pen = SelectObject(dc, border_pen);
            RoundRect(dc, card_rect.left, card_rect.top, card_rect.right, card_rect.bottom, kCardRadius, kCardRadius);
            SelectObject(dc, old_brush);
            SelectObject(dc, old_pen);
            DeleteObject(fill_brush);
            DeleteObject(border_pen);
        };

        draw_card(groups_card_rect_);
        draw_card(test_card_rect_);
        draw_card(snippets_card_rect_);
        draw_card(editor_card_rect_, true);
        draw_card(engine_settings_card_rect_);

        auto draw_control_frame = [&](HWND control, int radius = 6) {
            if (control == nullptr || !IsWindowVisible(control)) {
                return;
            }

            RECT control_rect{};
            GetWindowRect(control, &control_rect);
            MapWindowPoints(nullptr, hwnd_, reinterpret_cast<POINT*>(&control_rect), 2);

            HPEN border_pen = CreatePen(PS_SOLID, 1, border_color_);
            HGDIOBJ old_pen = SelectObject(dc, border_pen);
            HGDIOBJ old_brush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
            if (radius > 0) {
                RoundRect(dc, control_rect.left - 1, control_rect.top - 1, control_rect.right + 1, control_rect.bottom + 1, radius, radius);
            } else {
                Rectangle(dc, control_rect.left - 1, control_rect.top - 1, control_rect.right + 1, control_rect.bottom + 1);
            }
            SelectObject(dc, old_brush);
            SelectObject(dc, old_pen);
            DeleteObject(border_pen);
        };

        draw_control_frame(search_edit_, 4);
        draw_control_frame(groups_list_, 4);
        draw_control_frame(test_edit_, 4);
        draw_control_frame(snippets_list_, 4);
        draw_control_frame(trigger_edit_, 4);
        draw_control_frame(notes_edit_, 4);
        draw_control_frame(content_edit_, 4);
        draw_control_frame(restore_edit_, 4);

        EndPaint(hwnd_, &ps);
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        FillRect(dc, &rect, background_brush_ != nullptr ? background_brush_ : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        return 1;
    }

    case WM_SETCURSOR:
        if (LOWORD(lparam) == HTCLIENT) {
            POINT cursor{};
            GetCursorPos(&cursor);
            ScreenToClient(hwnd_, &cursor);
            switch (HitTestSplitter(cursor)) {
            case ActiveSplitter::LeftVertical:
            case ActiveSplitter::RightVertical:
                SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
                return TRUE;
            case ActiveSplitter::LeftHorizontal:
                SetCursor(LoadCursorW(nullptr, IDC_SIZENS));
                return TRUE;
            case ActiveSplitter::None:
                break;
            }
        }
        break;

    case WM_LBUTTONDOWN: {
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        active_splitter_ = HitTestSplitter(point);
        if (active_splitter_ != ActiveSplitter::None) {
            SetCapture(hwnd_);
            return 0;
        }
        break;
    }

    case WM_MOUSEMOVE:
        if (active_splitter_ != ActiveSplitter::None) {
            RECT rect{};
            GetClientRect(hwnd_, &rect);
            const int width = rect.right - rect.left;
            const int height = rect.bottom - rect.top;
            const int margin = 16;
            const int title_height = 48;
            const int button_height = 36;
            const int toolbar_y = margin + title_height;
            const int content_top = toolbar_y + button_height + 18;

            const POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            switch (active_splitter_) {
            case ActiveSplitter::LeftVertical:
                settings_.left_panel_width = point.x - margin;
                break;
            case ActiveSplitter::RightVertical:
                settings_.right_panel_width = width - margin - point.x - kSplitterThickness;
                break;
            case ActiveSplitter::LeftHorizontal:
                settings_.groups_panel_height = point.y - content_top;
                break;
            case ActiveSplitter::None:
                break;
            }
            ClampPanelSizes(width, height);
            LayoutChildren(width, height);
            RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (active_splitter_ != ActiveSplitter::None) {
            ReleaseCapture();
            active_splitter_ = ActiveSplitter::None;
            SaveData();
            RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            return 0;
        }
        break;

    case WM_CAPTURECHANGED:
        active_splitter_ = ActiveSplitter::None;
        break;

    case WM_VSCROLL: {
        HWND scroll_hwnd = reinterpret_cast<HWND>(lparam);
        if (scroll_hwnd == nullptr) {
            if (!compact_layout_ || hwnd_ == nullptr) {
                return 0;
            }

            SCROLLINFO info{};
            info.cbSize = sizeof(info);
            info.fMask = SIF_ALL;
            GetScrollInfo(hwnd_, SB_VERT, &info);

            int next_offset = window_scroll_offset_;
            switch (LOWORD(wparam)) {
            case SB_TOP:
                next_offset = info.nMin;
                break;
            case SB_BOTTOM:
                next_offset = info.nMax;
                break;
            case SB_LINEUP:
                next_offset -= 36;
                break;
            case SB_LINEDOWN:
                next_offset += 36;
                break;
            case SB_PAGEUP:
                next_offset -= static_cast<int>(info.nPage);
                break;
            case SB_PAGEDOWN:
                next_offset += static_cast<int>(info.nPage);
                break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                next_offset = info.nTrackPos;
                break;
            default:
                break;
            }

            next_offset = std::clamp(next_offset, 0, window_scroll_max_);
            if (next_offset != window_scroll_offset_) {
                window_scroll_offset_ = next_offset;
                RECT rect{};
                GetClientRect(hwnd_, &rect);
                LayoutChildren(rect.right - rect.left, rect.bottom - rect.top);
                RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            }
            return 0;
        }
        if (scroll_hwnd == editor_settings_scrollbar_) {
            SCROLLINFO info{};
            info.cbSize = sizeof(info);
            info.fMask = SIF_ALL;
            GetScrollInfo(editor_settings_scrollbar_, SB_CTL, &info);

            int next_offset = editor_settings_scroll_offset_;
            switch (LOWORD(wparam)) {
            case SB_TOP:
                next_offset = info.nMin;
                break;
            case SB_BOTTOM:
                next_offset = info.nMax;
                break;
            case SB_LINEUP:
                next_offset -= 28;
                break;
            case SB_LINEDOWN:
                next_offset += 28;
                break;
            case SB_PAGEUP:
                next_offset -= static_cast<int>(info.nPage);
                break;
            case SB_PAGEDOWN:
                next_offset += static_cast<int>(info.nPage);
                break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                next_offset = info.nTrackPos;
                break;
            default:
                break;
            }

            next_offset = std::clamp(next_offset, 0, editor_settings_scroll_max_);
            if (next_offset != editor_settings_scroll_offset_) {
                editor_settings_scroll_offset_ = next_offset;
                LayoutEditorSettingsSection();
                RedrawWindow(
                    hwnd_,
                    &engine_settings_card_rect_,
                    nullptr,
                    RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN
                );
            }
            return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        POINT cursor{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        const POINT screen_cursor = cursor;
        ScreenToClient(hwnd_, &cursor);
        if (compact_layout_ && window_scroll_max_ > 0) {
            HWND hovered = WindowFromPoint(screen_cursor);
            auto inside_control = [hovered](HWND control) {
                return control != nullptr && hovered != nullptr && (hovered == control || IsChild(control, hovered));
            };

            if (!inside_control(snippets_list_) &&
                !inside_control(groups_list_) &&
                !inside_control(test_edit_) &&
                !inside_control(content_edit_)) {
                const int step = (GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA) * 48;
                const int next_offset = std::clamp(window_scroll_offset_ - step, 0, window_scroll_max_);
                if (next_offset != window_scroll_offset_) {
                    window_scroll_offset_ = next_offset;
                    RECT rect{};
                    GetClientRect(hwnd_, &rect);
                    LayoutChildren(rect.right - rect.left, rect.bottom - rect.top);
                    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                    return 0;
                }
            }
        }
        if (editor_settings_scrollbar_ != nullptr && IsWindowVisible(editor_settings_scrollbar_) && PtInRect(&engine_settings_card_rect_, cursor)) {
            const int step = (GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA) * 36;
            const int next_offset = std::clamp(editor_settings_scroll_offset_ - step, 0, editor_settings_scroll_max_);
            if (next_offset != editor_settings_scroll_offset_) {
                editor_settings_scroll_offset_ = next_offset;
                LayoutEditorSettingsSection();
                RedrawWindow(
                    hwnd_,
                    &engine_settings_card_rect_,
                    nullptr,
                    RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN
                );
                return 0;
            }
        }
        break;
    }

    case WM_CONTEXTMENU: {
        HWND context_hwnd = reinterpret_cast<HWND>(wparam);
        wchar_t class_name[64] = {};
        if (context_hwnd != nullptr) {
            GetClassNameW(context_hwnd, class_name, static_cast<int>(std::size(class_name)));
        }
        const bool is_main_edit_control =
            context_hwnd == search_edit_ ||
            context_hwnd == trigger_edit_ ||
            context_hwnd == notes_edit_ ||
            context_hwnd == content_edit_ ||
            context_hwnd == test_edit_ ||
            context_hwnd == restore_edit_;
        const bool is_any_app_edit =
            context_hwnd != nullptr &&
            _wcsicmp(class_name, L"Edit") == 0 &&
            (IsChild(hwnd_, context_hwnd) != FALSE || GetParent(context_hwnd) == hwnd_);
        if (is_main_edit_control || is_any_app_edit) {
            POINT screen_point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            if (screen_point.x == -1 && screen_point.y == -1) {
                RECT edit_rect{};
                GetWindowRect(context_hwnd, &edit_rect);
                screen_point.x = edit_rect.left + 18;
                screen_point.y = edit_rect.top + 18;
            }
            ShowEditContextMenu(context_hwnd, screen_point);
            return 0;
        }
        if (context_hwnd == groups_list_) {
            POINT screen_point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            bool clicked_item = false;
            if (screen_point.x == -1 && screen_point.y == -1) {
                RECT list_rect{};
                GetWindowRect(groups_list_, &list_rect);
                screen_point.x = list_rect.left + 24;
                screen_point.y = list_rect.top + 24;
            } else {
                POINT client_point = screen_point;
                ScreenToClient(groups_list_, &client_point);
                const LRESULT hit = SendMessageW(groups_list_, LB_ITEMFROMPOINT, 0, MAKELPARAM(client_point.x, client_point.y));
                if (HIWORD(hit) == 0) {
                    clicked_item = true;
                    selected_group_index_ = static_cast<int>(LOWORD(hit));
                    SendMessageW(groups_list_, LB_SETCURSEL, selected_group_index_, 0);
                    RefreshSnippetList(false);
                }
            }
            ShowGroupsContextMenu(screen_point, clicked_item);
            return 0;
        }
        break;
    }

    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lparam);
        if (measure != nullptr && measure->CtlType == ODT_MENU) {
            auto* item = reinterpret_cast<PopupMenuItemData*>(measure->itemData);
            if (item != nullptr && item->separator) {
                measure->itemHeight = 10;
                measure->itemWidth = 118;
                return TRUE;
            }

            measure->itemHeight = 32;
            int width = 168;
            if (item != nullptr && !item->text.empty() && hwnd_ != nullptr) {
                HDC dc = GetDC(hwnd_);
                if (dc != nullptr) {
                    HGDIOBJ old_font = nullptr;
                    if (popup_menu_font_ != nullptr) {
                        old_font = SelectObject(dc, popup_menu_font_);
                    } else if (ui_font_ != nullptr) {
                        old_font = SelectObject(dc, ui_font_);
                    }
                    SIZE size{};
                    GetTextExtentPoint32W(dc, item->text.c_str(), static_cast<int>(item->text.size()), &size);
                    width = size.cx + (item != nullptr && item->has_submenu ? 38 : 32);
                    if (old_font != nullptr) {
                        SelectObject(dc, old_font);
                    }
                    ReleaseDC(hwnd_, dc);
                }
            }
            measure->itemWidth = width;
            return TRUE;
        }
        if (measure != nullptr && measure->CtlType == ODT_HEADER) {
            measure->itemHeight = 36;
            return TRUE;
        }
        if (measure != nullptr && measure->CtlType == ODT_LISTBOX && measure->CtlID == kGroupsList) {
            measure->itemHeight = 38;
            return TRUE;
        }
        if (measure != nullptr && measure->CtlType == ODT_COMBOBOX && measure->CtlID == kEditorGroupCombo) {
            measure->itemHeight = 34;
            measure->itemWidth = 220;
            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (draw == nullptr) {
            break;
        }

        if (draw->CtlType == ODT_MENU) {
            auto* item = reinterpret_cast<PopupMenuItemData*>(draw->itemData);
            if (item == nullptr) {
                return TRUE;
            }

            HDC dc = draw->hDC;
            RECT rect = draw->rcItem;
            const bool selected = (draw->itemState & ODS_SELECTED) != 0;
            const bool disabled = !item->enabled || (draw->itemState & ODS_DISABLED) != 0;

            HBRUSH bg_brush = CreateSolidBrush(surface_color_);
            FillRect(dc, &rect, bg_brush != nullptr ? bg_brush : reinterpret_cast<HBRUSH>(COLOR_MENU + 1));
            if (bg_brush != nullptr) {
                DeleteObject(bg_brush);
            }

            if (item->separator) {
                HPEN pen = CreatePen(PS_SOLID, 1, border_color_);
                HGDIOBJ old_pen = SelectObject(dc, pen);
                const int mid_y = (rect.top + rect.bottom) / 2;
                MoveToEx(dc, rect.left + 10, mid_y, nullptr);
                LineTo(dc, rect.right - 10, mid_y);
                SelectObject(dc, old_pen);
                DeleteObject(pen);
                return TRUE;
            }

            if (selected) {
                RECT selection_rect = rect;
                InflateRect(&selection_rect, -4, -2);
                COLORREF selection_fill = selection_color_;
                COLORREF selection_border = accent_color_;
                if (item->destructive) {
                    selection_fill = IsDarkTheme() ? RGB(88, 30, 36) : RGB(254, 226, 226);
                    selection_border = IsDarkTheme() ? RGB(220, 38, 38) : RGB(248, 113, 113);
                }
                HBRUSH selection_brush = CreateSolidBrush(selection_fill);
                HPEN selection_pen = CreatePen(PS_SOLID, 1, selection_border);
                HGDIOBJ old_brush = SelectObject(dc, selection_brush);
                HGDIOBJ old_pen = SelectObject(dc, selection_pen);
                RoundRect(dc, selection_rect.left, selection_rect.top, selection_rect.right, selection_rect.bottom, 8, 8);
                SelectObject(dc, old_brush);
                SelectObject(dc, old_pen);
                DeleteObject(selection_brush);
                DeleteObject(selection_pen);
            }

            COLORREF text = disabled ? muted_text_color_ : text_color_;
            if (item->destructive && !disabled) {
                text = IsDarkTheme() ? RGB(254, 242, 242) : RGB(127, 29, 29);
            }
            if (selected) {
                text = selection_text_color_;
            }

            RECT text_rect = rect;
            text_rect.left += 14;
            text_rect.right -= 14;
            if (item->has_submenu) {
                text_rect.right -= 14;
            }
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, text);
            if (popup_menu_font_ != nullptr) {
                SelectObject(dc, popup_menu_font_);
            } else if (ui_font_ != nullptr) {
                SelectObject(dc, ui_font_);
            }
            DrawTextW(dc, item->text.c_str(), -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
            return TRUE;
        }

        if (draw->CtlType == ODT_HEADER && snippets_list_ != nullptr && draw->hwndItem == ListView_GetHeader(snippets_list_)) {
            const HWND header_hwnd = draw->hwndItem;
            RECT rect = draw->rcItem;
            const bool selected = (draw->itemState & ODS_SELECTED) != 0;
            const COLORREF fill = selected
                ? (IsDarkTheme() ? RGB(18, 26, 38) : RGB(230, 235, 242))
                : header_color_;

            HBRUSH fill_brush = CreateSolidBrush(fill);
            FillRect(draw->hDC, &rect, fill_brush != nullptr ? fill_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
            if (fill_brush != nullptr) {
                DeleteObject(fill_brush);
            }

            HPEN border_pen = CreatePen(PS_SOLID, 1, border_color_);
            HGDIOBJ old_pen = SelectObject(draw->hDC, border_pen);
            MoveToEx(draw->hDC, rect.left, rect.bottom - 1, nullptr);
            LineTo(draw->hDC, rect.right, rect.bottom - 1);
            MoveToEx(draw->hDC, rect.right - 1, rect.top + 6, nullptr);
            LineTo(draw->hDC, rect.right - 1, rect.bottom - 6);
            SelectObject(draw->hDC, old_pen);
            DeleteObject(border_pen);

            wchar_t text_buffer[128] = {};
            HDITEMW item{};
            item.mask = HDI_TEXT | HDI_FORMAT;
            item.pszText = text_buffer;
            item.cchTextMax = static_cast<int>(std::size(text_buffer));
            Header_GetItem(header_hwnd, static_cast<int>(draw->itemID), &item);

            RECT text_rect = rect;
            text_rect.left += 12;
            text_rect.right -= 12;
            const bool sort_up = (item.fmt & HDF_SORTUP) != 0;
            const bool sort_down = (item.fmt & HDF_SORTDOWN) != 0;
            if (sort_up || sort_down) {
                POINT arrow[3]{};
                const int arrow_mid_x = rect.right - 14;
                const int arrow_mid_y = rect.top + ((rect.bottom - rect.top) / 2);
                if (sort_up) {
                    arrow[0] = {arrow_mid_x - 4, arrow_mid_y + 3};
                    arrow[1] = {arrow_mid_x + 4, arrow_mid_y + 3};
                    arrow[2] = {arrow_mid_x, arrow_mid_y - 2};
                } else {
                    arrow[0] = {arrow_mid_x - 4, arrow_mid_y - 3};
                    arrow[1] = {arrow_mid_x + 4, arrow_mid_y - 3};
                    arrow[2] = {arrow_mid_x, arrow_mid_y + 2};
                }
                HBRUSH arrow_brush = CreateSolidBrush(accent_color_);
                HGDIOBJ old_brush = SelectObject(draw->hDC, arrow_brush);
                Polygon(draw->hDC, arrow, 3);
                SelectObject(draw->hDC, old_brush);
                DeleteObject(arrow_brush);
                text_rect.right -= 18;
            }

            SetBkMode(draw->hDC, TRANSPARENT);
            SetTextColor(draw->hDC, text_color_);
            if (ui_font_ != nullptr) {
                SelectObject(draw->hDC, ui_font_);
            }
            DrawTextW(draw->hDC, text_buffer, -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
            return TRUE;
        }

        if (draw->CtlType == ODT_COMBOBOX && draw->CtlID == kEditorGroupCombo) {
            HDC dc = draw->hDC;
            RECT rect = draw->rcItem;
            HBRUSH background_brush = CreateSolidBrush(input_color_);
            FillRect(dc, &rect, background_brush != nullptr ? background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            if (background_brush != nullptr) {
                DeleteObject(background_brush);
            }

            std::wstring item_text;
            if (draw->itemID != static_cast<UINT>(-1)) {
                const LRESULT text_length = SendMessageW(draw->hwndItem, CB_GETLBTEXTLEN, draw->itemID, 0);
                if (text_length > 0) {
                    item_text.resize(static_cast<size_t>(text_length));
                    SendMessageW(draw->hwndItem, CB_GETLBTEXT, draw->itemID, reinterpret_cast<LPARAM>(item_text.data()));
                }
            } else {
                item_text = TrimCopy(ReadWindowText(draw->hwndItem));
            }

            const bool selected = (draw->itemState & ODS_SELECTED) != 0;
            const bool focused = (draw->itemState & ODS_FOCUS) != 0;
            if (selected) {
                RECT selection_rect = rect;
                InflateRect(&selection_rect, -2, -2);
                HBRUSH selection_brush = CreateSolidBrush(selection_color_);
                HPEN selection_pen = CreatePen(PS_SOLID, 1, accent_color_);
                HGDIOBJ old_brush = SelectObject(dc, selection_brush);
                HGDIOBJ old_pen = SelectObject(dc, selection_pen);
                RoundRect(dc, selection_rect.left, selection_rect.top, selection_rect.right, selection_rect.bottom, 8, 8);
                SelectObject(dc, old_brush);
                SelectObject(dc, old_pen);
                DeleteObject(selection_brush);
                DeleteObject(selection_pen);
            }

            RECT text_rect = rect;
            text_rect.left += 12;
            text_rect.right -= 10;
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, selected ? selection_text_color_ : text_color_);
            if (ui_font_ != nullptr) {
                SelectObject(dc, ui_font_);
            }
            DrawTextW(dc, item_text.c_str(), -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            if (focused && draw->itemID != static_cast<UINT>(-1)) {
                RECT focus_rect = rect;
                InflateRect(&focus_rect, -4, -3);
                DrawFocusRect(dc, &focus_rect);
            }
            return TRUE;
        }

        if (draw->CtlType == ODT_LISTBOX && draw->CtlID == kGroupsList) {
            HDC dc = draw->hDC;
            RECT rect = draw->rcItem;
            HBRUSH background_brush = CreateSolidBrush(input_color_);
            FillRect(dc, &rect, background_brush != nullptr ? background_brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            if (background_brush != nullptr) {
                DeleteObject(background_brush);
            }

            if (draw->itemID == static_cast<UINT>(-1)) {
                return TRUE;
            }

            wchar_t buffer[512] = {};
            SendMessageW(groups_list_, LB_GETTEXT, draw->itemID, reinterpret_cast<LPARAM>(buffer));
            const bool selected = (draw->itemState & ODS_SELECTED) != 0;
            const bool focused = (draw->itemState & ODS_FOCUS) != 0;
            const bool group_off = wcsstr(buffer, L"[off]") != nullptr;

            const COLORREF fill = selected
                ? selection_color_
                : input_color_;
            const COLORREF border = selected
                ? (IsDarkTheme() ? RGB(102, 117, 141) : RGB(177, 187, 201))
                : border_color_;
            const COLORREF text = selected ? selection_text_color_ : (group_off ? muted_text_color_ : text_color_);

            HBRUSH fill_brush = CreateSolidBrush(fill);
            HPEN border_pen = CreatePen(PS_SOLID, 1, border);
            HGDIOBJ old_brush = SelectObject(dc, fill_brush);
            HGDIOBJ old_pen = SelectObject(dc, border_pen);
            RoundRect(dc, rect.left, rect.top + 1, rect.right, rect.bottom - 1, 10, 10);
            SelectObject(dc, old_brush);
            SelectObject(dc, old_pen);
            DeleteObject(fill_brush);
            DeleteObject(border_pen);

            RECT text_rect = rect;
            text_rect.left += 10;
            text_rect.right -= 10;
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, text);
            if (ui_font_ != nullptr) {
                SelectObject(dc, ui_font_);
            }
            DrawTextW(dc, buffer, -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            if (focused) {
                RECT focus_rect = rect;
                InflateRect(&focus_rect, -3, -3);
                DrawFocusRect(dc, &focus_rect);
            }
            return TRUE;
        }

        if (draw->CtlType != ODT_BUTTON) {
            break;
        }

        const int control_id = static_cast<int>(draw->CtlID);
        const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
        const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
        const bool focused = (draw->itemState & ODS_FOCUS) != 0;
        const bool hovered = hovered_interactive_control_ == draw->hwndItem;

        auto mix = [](COLORREF a, COLORREF b, int weight_b) {
            const int weight_a = 100 - weight_b;
            return RGB(
                (GetRValue(a) * weight_a + GetRValue(b) * weight_b) / 100,
                (GetGValue(a) * weight_a + GetGValue(b) * weight_b) / 100,
                (GetBValue(a) * weight_a + GetBValue(b) * weight_b) / 100
            );
        };

        auto is_checkbox_control = [](int id) {
            switch (id) {
            case kAlwaysOnTopCheckbox:
            case kStartWithWindowsCheckbox:
            case kMinimizeToTrayCheckbox:
            case kPreviousClipboardCheckbox:
            case kPreviousClipboardSlashCheckbox:
            case kSnippetEnabledCheckbox:
            case kCaseSensitiveCheckbox:
            case kSeparatorSpaceCheckbox:
            case kSeparatorEnterCheckbox:
            case kSeparatorTabCheckbox:
                return true;
            default:
                return false;
            }
        };

        auto is_radio_control = [](int id) {
            return id == kInstantModeRadio || id == kSeparatorModeRadio;
        };

        if (is_checkbox_control(control_id) || is_radio_control(control_id)) {
            const bool is_radio = is_radio_control(control_id);
            const bool checked = IsOwnerDrawChecked(draw->hwndItem);
            HDC dc = draw->hDC;
            RECT rect = draw->rcItem;

            HBRUSH background_brush = CreateSolidBrush(surface_color_);
            FillRect(dc, &rect, background_brush != nullptr ? background_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
            if (background_brush != nullptr) {
                DeleteObject(background_brush);
            }

            const int indicator_size = is_radio ? 16 : 18;
            RECT indicator_rect{
                rect.left + 2,
                rect.top + std::max(0L, ((rect.bottom - rect.top) - indicator_size) / 2),
                rect.left + 2 + indicator_size,
                rect.top + std::max(0L, ((rect.bottom - rect.top) - indicator_size) / 2) + indicator_size
            };

            COLORREF indicator_fill = checked ? accent_color_ : input_color_;
            COLORREF indicator_border = checked ? accent_color_ : border_color_;
            COLORREF text = disabled ? muted_text_color_ : text_color_;
            if (hovered && !disabled && !pressed) {
                indicator_fill = mix(indicator_fill, accent_color_, IsDarkTheme() ? 14 : 9);
                indicator_border = mix(indicator_border, accent_color_, IsDarkTheme() ? 40 : 28);
            }
            if (pressed && !disabled) {
                indicator_fill = mix(indicator_fill, RGB(0, 0, 0), IsDarkTheme() ? 14 : 6);
            }
            if (focused && !checked) {
                indicator_border = accent_color_;
            }

            HBRUSH indicator_brush = CreateSolidBrush(indicator_fill);
            HPEN indicator_pen = CreatePen(PS_SOLID, 1, indicator_border);
            HGDIOBJ old_brush = SelectObject(dc, indicator_brush);
            HGDIOBJ old_pen = SelectObject(dc, indicator_pen);
            if (is_radio) {
                Ellipse(dc, indicator_rect.left, indicator_rect.top, indicator_rect.right, indicator_rect.bottom);
            } else {
                RoundRect(dc, indicator_rect.left, indicator_rect.top, indicator_rect.right, indicator_rect.bottom, 5, 5);
            }
            SelectObject(dc, old_brush);
            SelectObject(dc, old_pen);
            DeleteObject(indicator_brush);
            DeleteObject(indicator_pen);

            if (checked) {
                if (is_radio) {
                    const int inset = 4;
                    HBRUSH dot_brush = CreateSolidBrush(RGB(255, 255, 255));
                    HGDIOBJ old_dot_brush = SelectObject(dc, dot_brush);
                    HGDIOBJ old_dot_pen = SelectObject(dc, GetStockObject(NULL_PEN));
                    Ellipse(
                        dc,
                        indicator_rect.left + inset,
                        indicator_rect.top + inset,
                        indicator_rect.right - inset,
                        indicator_rect.bottom - inset
                    );
                    SelectObject(dc, old_dot_pen);
                    SelectObject(dc, old_dot_brush);
                    DeleteObject(dot_brush);
                } else {
                    HPEN check_pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
                    HGDIOBJ old_check_pen = SelectObject(dc, check_pen);
                    MoveToEx(dc, indicator_rect.left + 4, indicator_rect.top + 9, nullptr);
                    LineTo(dc, indicator_rect.left + 8, indicator_rect.bottom - 4);
                    LineTo(dc, indicator_rect.right - 4, indicator_rect.top + 4);
                    SelectObject(dc, old_check_pen);
                    DeleteObject(check_pen);
                }
            }

            RECT text_rect = rect;
            text_rect.left = indicator_rect.right + 8;
            text_rect.right -= 2;
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, text);
            if (ui_font_ != nullptr) {
                SelectObject(dc, ui_font_);
            }
            std::wstring toggle_text = ReadWindowText(draw->hwndItem);
            DrawTextW(dc, toggle_text.c_str(), -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            if (focused) {
                RECT focus_rect = text_rect;
                focus_rect.left = std::max(rect.left, focus_rect.left - 2);
                DrawFocusRect(dc, &focus_rect);
            }
            return TRUE;
        }

        if (control_id == kGithubButton || control_id == kInfoButton) {
            COLORREF fill = IsDarkTheme() ? RGB(34, 45, 63) : RGB(248, 250, 252);
            COLORREF border = IsDarkTheme() ? RGB(86, 99, 124) : RGB(203, 213, 225);
            COLORREF text = disabled ? muted_text_color_ : text_color_;
            if (hovered && !disabled && !pressed) {
                fill = mix(fill, accent_color_, IsDarkTheme() ? 18 : 12);
                border = mix(border, accent_color_, IsDarkTheme() ? 42 : 30);
            }
            if (pressed) {
                fill = mix(fill, RGB(0, 0, 0), IsDarkTheme() ? 20 : 8);
            }
            if (disabled) {
                fill = mix(fill, background_color_, 45);
                border = mix(border, background_color_, 45);
                text = muted_text_color_;
            }

            HDC dc = draw->hDC;
            RECT rect = draw->rcItem;

            HBRUSH fill_brush = CreateSolidBrush(fill);
            HPEN border_pen = CreatePen(PS_SOLID, 1, border);
            HGDIOBJ old_brush = SelectObject(dc, fill_brush);
            HGDIOBJ old_pen = SelectObject(dc, border_pen);
            Ellipse(dc, rect.left, rect.top, rect.right, rect.bottom);
            SelectObject(dc, old_brush);
            SelectObject(dc, old_pen);
            DeleteObject(fill_brush);
            DeleteObject(border_pen);

            const int image_size = std::max(ScaleMainUi(18), static_cast<int>(rect.bottom - rect.top - ScaleMainUi(12)));
            const int image_x = rect.left + std::max(0L, ((rect.right - rect.left) - image_size) / 2);
            const int image_y = rect.top + std::max(0L, ((rect.bottom - rect.top) - image_size) / 2);

            Gdiplus::Bitmap* bitmap = control_id == kGithubButton ? github_button_bitmap_ : info_button_bitmap_;
            if (bitmap != nullptr) {
                Gdiplus::Graphics graphics(dc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
                graphics.DrawImage(bitmap, image_x, image_y, image_size, image_size);
            } else {
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, text);
                if (ui_font_ != nullptr) {
                    SelectObject(dc, ui_font_);
                }
                RECT text_rect = rect;
                const wchar_t* fallback_text = control_id == kGithubButton ? L"G" : L"i";
                DrawTextW(dc, fallback_text, -1, &text_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            }

            if (focused) {
                RECT focus_rect = rect;
                InflateRect(&focus_rect, -4, -4);
                DrawFocusRect(dc, &focus_rect);
            }
            return TRUE;
        }

        COLORREF fill = surface_color_;
        COLORREF border = border_color_;
        COLORREF text = disabled ? muted_text_color_ : text_color_;

        if (control_id == kEngineButton) {
            fill = engine_enabled_
                ? (IsDarkTheme() ? RGB(21, 128, 61) : RGB(34, 197, 94))
                : (IsDarkTheme() ? RGB(127, 29, 29) : RGB(239, 68, 68));
            border = fill;
            text = RGB(255, 255, 255);
        } else if (control_id == kEditorSnippetTabButton || control_id == kEditorEngineTabButton) {
            const bool active_tab =
                (control_id == kEditorSnippetTabButton && !engine_settings_tab_active_) ||
                (control_id == kEditorEngineTabButton && engine_settings_tab_active_);
            if (active_tab) {
                fill = selection_color_;
                border = accent_color_;
                text = text_color_;
            } else {
                fill = IsDarkTheme() ? RGB(34, 45, 63) : RGB(248, 250, 252);
                border = border_color_;
                text = disabled ? muted_text_color_ : text_color_;
            }
        } else if (control_id == kDeleteButton || control_id == kDeleteGroupButton) {
            fill = IsDarkTheme() ? RGB(127, 29, 29) : RGB(254, 226, 226);
            border = IsDarkTheme() ? RGB(239, 68, 68) : RGB(248, 113, 113);
            text = IsDarkTheme() ? RGB(254, 242, 242) : RGB(127, 29, 29);
        } else if (control_id == kSaveButton || control_id == kSettingsSaveButton) {
            fill = accent_color_;
            border = accent_color_;
            text = RGB(255, 255, 255);
        } else if (control_id == kResetButton || control_id == kSettingsResetButton || control_id == kTestResetButton) {
            fill = IsDarkTheme() ? RGB(44, 56, 76) : RGB(244, 247, 251);
        } else if (control_id == kEditorNewButton || control_id == kSnippetPanelNewButton || control_id == kNewGroupButton || control_id == kRenameGroupButton ||
                   control_id == kToggleGroupButton || control_id == kNewSnippetButton || control_id == kDuplicateButton || control_id == kToggleSnippetButton ||
                   control_id == kThemeButton || control_id == kImportButton || control_id == kExportButton ||
                   control_id == kRecordHotkeyButton) {
            fill = IsDarkTheme() ? RGB(39, 51, 72) : RGB(248, 250, 252);
        }

        COLORREF hover_tint = accent_color_;
        if (control_id == kEngineButton || control_id == kDeleteButton || control_id == kDeleteGroupButton) {
            hover_tint = border;
        } else if (control_id == kSaveButton || control_id == kSettingsSaveButton) {
            hover_tint = fill;
        }
        if (hovered && !disabled && !pressed) {
            fill = mix(fill, hover_tint, IsDarkTheme() ? 16 : 10);
            border = mix(border, hover_tint, IsDarkTheme() ? 46 : 32);
        }
        if (pressed) {
            fill = mix(fill, RGB(0, 0, 0), IsDarkTheme() ? 18 : 8);
        }
        if (disabled) {
            fill = mix(fill, background_color_, 45);
            border = mix(border, background_color_, 45);
            text = muted_text_color_;
        }

        HDC dc = draw->hDC;
        RECT rect = draw->rcItem;
        HBRUSH fill_brush = CreateSolidBrush(fill);
        HPEN border_pen = CreatePen(PS_SOLID, 1, border);
        HGDIOBJ old_brush = SelectObject(dc, fill_brush);
        HGDIOBJ old_pen = SelectObject(dc, border_pen);
        RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
        SelectObject(dc, old_brush);
        SelectObject(dc, old_pen);
        DeleteObject(fill_brush);
        DeleteObject(border_pen);

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, text);
        if (ui_font_ != nullptr) {
            SelectObject(dc, ui_font_);
        }

        RECT text_rect = rect;
        text_rect.left += 8;
        text_rect.right -= 8;
        std::wstring button_text = ReadWindowText(draw->hwndItem);
        DrawTextW(dc, button_text.c_str(), -1, &text_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);

        if (focused) {
            RECT focus_rect = rect;
            InflateRect(&focus_rect, -4, -4);
            DrawFocusRect(dc, &focus_rect);
        }
        return TRUE;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        HWND control = reinterpret_cast<HWND>(lparam);
        if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX) {
            const bool is_search_placeholder = control == search_edit_ && search_placeholder_active_;
            SetTextColor(dc, is_search_placeholder ? muted_text_color_ : text_color_);
            SetBkColor(dc, input_color_);
            return reinterpret_cast<LRESULT>(input_brush_ != nullptr ? input_brush_ : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        }

        const bool is_subtle_label = control == subtitle_label_ || control == status_label_ || control == editor_hint_label_;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, is_subtle_label ? muted_text_color_ : text_color_);
        const bool use_surface = UsesSurfaceBackground(control);
        const COLORREF background = use_surface ? surface_color_ : background_color_;
        const HBRUSH brush = use_surface ? surface_brush_ : background_brush_;

        if (message == WM_CTLCOLORBTN) {
            SetBkColor(dc, background);
            return reinterpret_cast<LRESULT>(brush != nullptr ? brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
        }

        SetBkColor(dc, background);
        return reinterpret_cast<LRESULT>(brush != nullptr ? brush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    }

    case WM_NOTIFY: {
        auto* header = reinterpret_cast<NMHDR*>(lparam);
        const HWND snippets_header = snippets_list_ != nullptr ? ListView_GetHeader(snippets_list_) : nullptr;
        if (header != nullptr && snippets_header != nullptr && header->hwndFrom == snippets_header && header->code == NM_CUSTOMDRAW) {
            auto* draw = reinterpret_cast<NMCUSTOMDRAW*>(lparam);
            if (draw->dwDrawStage == CDDS_PREPAINT) {
                return CDRF_NOTIFYITEMDRAW;
            }
            if (draw->dwDrawStage == CDDS_ITEMPREPAINT) {
                RECT rect = draw->rc;
                const COLORREF header_fill = header_color_;
                const COLORREF header_border = border_color_;
                HBRUSH fill_brush = CreateSolidBrush(header_fill);
                FillRect(draw->hdc, &rect, fill_brush != nullptr ? fill_brush : reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
                if (fill_brush != nullptr) {
                    DeleteObject(fill_brush);
                }

                HPEN border_pen = CreatePen(PS_SOLID, 1, header_border);
                HGDIOBJ old_pen = SelectObject(draw->hdc, border_pen);
                MoveToEx(draw->hdc, rect.left, rect.bottom - 1, nullptr);
                LineTo(draw->hdc, rect.right, rect.bottom - 1);
                MoveToEx(draw->hdc, rect.right - 1, rect.top, nullptr);
                LineTo(draw->hdc, rect.right - 1, rect.bottom);
                SelectObject(draw->hdc, old_pen);
                DeleteObject(border_pen);

                wchar_t text_buffer[128] = {};
                HDITEMW item{};
                item.mask = HDI_TEXT | HDI_FORMAT;
                item.pszText = text_buffer;
                item.cchTextMax = static_cast<int>(std::size(text_buffer));
                Header_GetItem(snippets_header, static_cast<int>(draw->dwItemSpec), &item);

                RECT text_rect = rect;
                text_rect.left += 8;
                text_rect.right -= 8;
                const bool sort_up = (item.fmt & HDF_SORTUP) != 0;
                const bool sort_down = (item.fmt & HDF_SORTDOWN) != 0;
                if (sort_up || sort_down) {
                    POINT arrow[3]{};
                    const int arrow_mid_x = rect.right - 12;
                    const int arrow_mid_y = rect.top + ((rect.bottom - rect.top) / 2);
                    if (sort_up) {
                        arrow[0] = {arrow_mid_x - 4, arrow_mid_y + 2};
                        arrow[1] = {arrow_mid_x + 4, arrow_mid_y + 2};
                        arrow[2] = {arrow_mid_x, arrow_mid_y - 3};
                    } else {
                        arrow[0] = {arrow_mid_x - 4, arrow_mid_y - 2};
                        arrow[1] = {arrow_mid_x + 4, arrow_mid_y - 2};
                        arrow[2] = {arrow_mid_x, arrow_mid_y + 3};
                    }
                    HBRUSH arrow_brush = CreateSolidBrush(text_color_);
                    HGDIOBJ old_brush = SelectObject(draw->hdc, arrow_brush);
                    Polygon(draw->hdc, arrow, 3);
                    SelectObject(draw->hdc, old_brush);
                    DeleteObject(arrow_brush);
                    text_rect.right -= 16;
                }

                SetBkMode(draw->hdc, TRANSPARENT);
                SetTextColor(draw->hdc, text_color_);
                if (ui_font_ != nullptr) {
                    SelectObject(draw->hdc, ui_font_);
                }
                DrawTextW(draw->hdc, text_buffer, -1, &text_rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
                return CDRF_SKIPDEFAULT;
            }
        }
        if (header != nullptr && header->hwndFrom == snippets_list_ && header->idFrom == kSnippetsList) {
            if (header->code == LVN_COLUMNCLICK) {
                auto* click = reinterpret_cast<NMLISTVIEW*>(lparam);
                if (click->iSubItem >= 0 && click->iSubItem <= 4) {
                    if (snippet_sort_column_ == click->iSubItem) {
                        snippet_sort_ascending_ = !snippet_sort_ascending_;
                    } else {
                        snippet_sort_column_ = click->iSubItem;
                        snippet_sort_ascending_ = true;
                    }
                    RefreshSnippetList(true);
                    static const wchar_t* kColumnLabels[] = {L"Trigger", L"Group", L"On", L"Notes", L"Preview"};
                    std::wstring status = L"Sorted snippets by ";
                    status += kColumnLabels[snippet_sort_column_];
                    status += snippet_sort_ascending_ ? L" (ascending)." : L" (descending).";
                    SetStatusText(status.c_str());
                }
                return 0;
            }

            if (header->code == LVN_KEYDOWN) {
                auto* key = reinterpret_cast<NMLVKEYDOWN*>(lparam);
                if (key->wVKey == VK_RETURN) {
                    EditSelectedSnippet();
                    return 0;
                }
                if (key->wVKey == VK_F2) {
                    EditSelectedSnippet();
                    return 0;
                }
                if (key->wVKey == VK_DELETE) {
                    DeleteSelectedSnippet();
                    return 0;
                }
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && key->wVKey == VK_SPACE) {
                    ToggleSelectedSnippets();
                    return 0;
                }
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && key->wVKey == 'D') {
                    DuplicateSelectedSnippet();
                    return 0;
                }
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && key->wVKey == 'N') {
                    StartNewSnippetCommand();
                    return 0;
                }
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && key->wVKey == 'F') {
                    SetFocus(search_edit_);
                    SendMessageW(search_edit_, EM_SETSEL, 0, -1);
                    SetStatusText(L"Search box focused.");
                    return 0;
                }
                if (key->wVKey == VK_NEXT || key->wVKey == VK_PRIOR) {
                    const int item_count = static_cast<int>(filtered_snippet_indices_.size());
                    if (item_count <= 0) {
                        return 0;
                    }

                    int focused = ListView_GetNextItem(snippets_list_, -1, LVNI_FOCUSED);
                    if (focused < 0) {
                        focused = SelectedSnippetVisibleIndex();
                    }
                    if (focused < 0) {
                        focused = 0;
                    }

                    const int page_size = std::max(1, ListView_GetCountPerPage(snippets_list_) - 1);
                    const int target = std::clamp(
                        focused + (key->wVKey == VK_NEXT ? page_size : -page_size),
                        0,
                        item_count - 1
                    );
                    const bool shift_down = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    const bool ctrl_down = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

                    suppress_snippet_list_notifications_ = true;
                    if (!shift_down && !ctrl_down) {
                        ListView_SetItemState(snippets_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
                    }
                    if (shift_down) {
                        if (!ctrl_down) {
                            ListView_SetItemState(snippets_list_, -1, 0, LVIS_SELECTED);
                        }
                        const int start = std::min(focused, target);
                        const int end = std::max(focused, target);
                        for (int index = start; index <= end; ++index) {
                            ListView_SetItemState(snippets_list_, index, LVIS_SELECTED, LVIS_SELECTED);
                        }
                    } else {
                        ListView_SetItemState(snippets_list_, target, LVIS_SELECTED, LVIS_SELECTED);
                    }
                    ListView_SetItemState(snippets_list_, target, LVIS_FOCUSED, LVIS_FOCUSED);
                    ListView_EnsureVisible(snippets_list_, target, FALSE);
                    suppress_snippet_list_notifications_ = false;

                    selected_snippet_index_ = filtered_snippet_indices_[target];
                    RefreshEditorBanner();
                    RefreshSnippetActionButtons();
                    return 0;
                }
            }

            if (header->code == NM_RCLICK) {
                POINT cursor{};
                GetCursorPos(&cursor);
                POINT client_point = cursor;
                ScreenToClient(snippets_list_, &client_point);

                LVHITTESTINFO hit_test{};
                hit_test.pt = client_point;
                const int hit_index = ListView_SubItemHitTest(snippets_list_, &hit_test);
                const bool clicked_item = hit_index >= 0 && hit_index < static_cast<int>(filtered_snippet_indices_.size());
                if (hit_index >= 0 && hit_index < static_cast<int>(filtered_snippet_indices_.size())) {
                    SelectSnippetByActualIndex(filtered_snippet_indices_[hit_index]);
                }
                ShowSnippetsContextMenu(cursor, clicked_item);
                return 0;
            }

            if (header->code == NM_CUSTOMDRAW) {
                auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(lparam);
                auto apply_row_colors = [this](const Snippet& snippet, bool is_selected, COLORREF& row_text, COLORREF& row_background) {
                    (void)snippet;
                    row_text = text_color_;
                    row_background = input_color_;

                    if (is_selected) {
                        row_background = selection_color_;
                        row_text = selection_text_color_;
                    }
                };

                if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
                    return CDRF_NOTIFYITEMDRAW;
                }
                if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    const int actual_index = static_cast<int>(draw->nmcd.lItemlParam);
                    if (actual_index >= 0 && actual_index < static_cast<int>(snippets_.size())) {
                        const int visible_index = static_cast<int>(draw->nmcd.dwItemSpec);
                        const bool is_selected =
                            (ListView_GetItemState(snippets_list_, visible_index, LVIS_SELECTED) & LVIS_SELECTED) != 0;
                        COLORREF row_text = text_color_;
                        COLORREF row_background = input_color_;
                        apply_row_colors(snippets_[actual_index], is_selected, row_text, row_background);
                        if (is_selected) {
                            RECT row_rect{};
                            ListView_GetItemRect(snippets_list_, visible_index, &row_rect, LVIR_BOUNDS);
                            InflateRect(&row_rect, -2, -1);

                            const bool is_focused =
                                (ListView_GetItemState(snippets_list_, visible_index, LVIS_FOCUSED) & LVIS_FOCUSED) != 0;
                            const COLORREF border = is_focused
                                ? accent_color_
                                : (IsDarkTheme() ? RGB(150, 169, 199) : RGB(120, 140, 172));
                            HBRUSH fill_brush = CreateSolidBrush(row_background);
                            HPEN border_pen = CreatePen(PS_SOLID, 2, border);
                            HGDIOBJ old_brush = SelectObject(draw->nmcd.hdc, fill_brush);
                            HGDIOBJ old_pen = SelectObject(draw->nmcd.hdc, border_pen);
                            RoundRect(draw->nmcd.hdc, row_rect.left, row_rect.top, row_rect.right, row_rect.bottom, 10, 10);
                            SelectObject(draw->nmcd.hdc, old_brush);
                            SelectObject(draw->nmcd.hdc, old_pen);
                            DeleteObject(fill_brush);
                            DeleteObject(border_pen);

                        }
                    }
                    return CDRF_NOTIFYSUBITEMDRAW;
                }
                if (draw->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
                    const int actual_index = static_cast<int>(draw->nmcd.lItemlParam);
                    if (actual_index < 0 || actual_index >= static_cast<int>(snippets_.size())) {
                        return CDRF_DODEFAULT;
                    }

                    const int visible_index = static_cast<int>(draw->nmcd.dwItemSpec);
                    const bool is_selected =
                        (ListView_GetItemState(snippets_list_, visible_index, LVIS_SELECTED) & LVIS_SELECTED) != 0;
                    COLORREF row_text = text_color_;
                    COLORREF row_background = input_color_;
                    apply_row_colors(snippets_[actual_index], is_selected, row_text, row_background);
                    if (!is_selected) {
                        draw->clrText = row_text;
                        draw->clrTextBk = row_background;
                        return CDRF_DODEFAULT;
                    }

                    RECT subitem_rect{};
                    ListView_GetSubItemRect(
                        snippets_list_,
                        static_cast<int>(draw->nmcd.dwItemSpec),
                        draw->iSubItem,
                        LVIR_BOUNDS,
                        &subitem_rect
                    );
                    subitem_rect.left += 10;
                    subitem_rect.right -= 10;

                    wchar_t text_buffer[2048] = {};
                    ListView_GetItemText(
                        snippets_list_,
                        static_cast<int>(draw->nmcd.dwItemSpec),
                        draw->iSubItem,
                        text_buffer,
                        static_cast<int>(std::size(text_buffer))
                    );

                    SetBkMode(draw->nmcd.hdc, TRANSPARENT);
                    SetTextColor(draw->nmcd.hdc, row_text);
                    if (ui_font_ != nullptr) {
                        SelectObject(draw->nmcd.hdc, ui_font_);
                    }
                    DrawTextW(
                        draw->nmcd.hdc,
                        text_buffer,
                        -1,
                        &subitem_rect,
                        DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS
                    );
                    return CDRF_SKIPDEFAULT;
                }
            }

            if (header->code == LVN_ITEMACTIVATE) {
                EditSelectedSnippet();
                return 0;
            }

            if (header->code == LVN_ITEMCHANGED && !suppress_snippet_list_notifications_) {
                auto* info = reinterpret_cast<NMLISTVIEW*>(lparam);
                if ((info->uChanged & LVIF_STATE) == 0) {
                    return 0;
                }
                const bool was_selected = (info->uOldState & LVIS_SELECTED) != 0;
                const bool is_selected = (info->uNewState & LVIS_SELECTED) != 0;
                if (!is_selected && was_selected) {
                    if (ListView_GetNextItem(snippets_list_, -1, LVNI_SELECTED) == -1) {
                        selected_snippet_index_ = -1;
                        RefreshEditorBanner();
                        RefreshSnippetActionButtons();
                    }
                    return 0;
                }
                if (!is_selected || was_selected) {
                    return 0;
                }
                if (info->iItem < 0 || info->iItem >= static_cast<int>(filtered_snippet_indices_.size())) {
                    return 0;
                }

                const int actual_index = filtered_snippet_indices_[info->iItem];
                if (actual_index == selected_snippet_index_) {
                    return 0;
                }

                selected_snippet_index_ = actual_index;
                RefreshEditorBanner();
                RefreshSnippetActionButtons();
                return 0;
            }
        }
        break;
    }

    case WM_HOTKEY:
        if (wparam == kGlobalToggleHotkeyId) {
            ToggleEngine();
            return 0;
        }
        break;

    case WM_CLIPBOARDUPDATE:
        RefreshClipboardHistoryFromSystem();
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (is_capturing_hotkey_) {
            const UINT vk_code = static_cast<UINT>(wparam);
            if (vk_code == VK_ESCAPE) {
                CancelHotkeyCapture();
                SetStatusText(L"Hotkey capture cancelled.");
                return 0;
            }
            if (IsModifierVirtualKey(vk_code)) {
                return 0;
            }
            if (FinishHotkeyCapture(vk_code)) {
                return 0;
            }
        }
        break;

    case kTrayIconMessage:
        switch (LOWORD(lparam)) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            RestoreFromTray();
            return 0;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu();
            return 0;
        default:
            break;
        }
        break;

    case kStatusMessage: {
        std::unique_ptr<std::wstring> text(reinterpret_cast<std::wstring*>(lparam));
        if (text) {
            SetStatusText(text->c_str());
        }
        return 0;
    }

    case WM_CLOSE:
        if (!ConfirmDiscardEditorChanges(L"closing the app")) {
            return 0;
        }
        SaveData();
        DestroyWindow(hwnd_);
        return 0;

    case WM_COMMAND: {
        const int control_id = LOWORD(wparam);
        const int notify = HIWORD(wparam);
        HWND control_hwnd = reinterpret_cast<HWND>(lparam);

        if (notify == BN_CLICKED) {
            auto toggle_checkbox = [](HWND control) {
                if (control == nullptr) {
                    return;
                }
                SetOwnerDrawChecked(control, !IsOwnerDrawChecked(control));
            };
            auto set_trigger_mode_selection = [this](bool separator_mode) {
                SetOwnerDrawChecked(instant_mode_radio_, !separator_mode);
                SetOwnerDrawChecked(separator_mode_radio_, separator_mode);
            };

            switch (control_id) {
            case kEngineButton:
                ToggleEngine();
                return 0;
            case kThemeButton:
                settings_.theme = IsDarkTheme() ? L"light" : L"dark";
                ApplyTheme();
                if (SaveData()) {
                    SetStatusText(IsDarkTheme() ? L"Theme switched to dark." : L"Theme switched to light.");
                } else {
                    SetStatusText(L"Theme changed in memory, but JSON save failed.");
                }
                return 0;
            case kImportButton:
                if (!ConfirmDiscardEditorChanges(L"with import")) {
                    return 0;
                }
                ImportDataFromDialog();
                return 0;
            case kExportButton:
                ExportDataToDialog();
                return 0;
            case kGithubButton: {
                const HINSTANCE open_result = ShellExecuteW(hwnd_, L"open", kTemporaryGithubUrl, nullptr, nullptr, SW_SHOWNORMAL);
                if (reinterpret_cast<INT_PTR>(open_result) <= 32) {
                    SetStatusText(L"Could not open the GitHub link.");
                } else {
                    SetStatusText(L"Opened the GitHub link.");
                }
                return 0;
            }
            case kInfoButton:
                ShowInfoDialog();
                return 0;
            case kRecordHotkeyButton:
                if (is_capturing_hotkey_) {
                    CancelHotkeyCapture();
                    SetStatusText(L"Hotkey capture cancelled.");
                } else {
                    BeginHotkeyCapture();
                }
                return 0;
            case kNewGroupButton:
                CreateGroup();
                return 0;
            case kRenameGroupButton:
                RenameSelectedGroup();
                return 0;
            case kToggleGroupButton:
                ToggleSelectedGroup();
                return 0;
            case kDeleteGroupButton:
                DeleteSelectedGroup();
                return 0;
            case kTestResetButton:
                ResetTestArea();
                return 0;
            case kNewSnippetButton:
                EditSelectedSnippet();
                return 0;
            case kEditorNewButton:
                StartNewSnippetCommand();
                return 0;
            case kDuplicateButton:
                DuplicateSelectedSnippet();
                return 0;
            case kToggleSnippetButton:
                ToggleSelectedSnippets();
                return 0;
            case kDeleteButton:
                DeleteSelectedSnippet();
                return 0;
            case kSnippetPanelNewButton:
                StartNewSnippetCommand();
                return 0;
            case kEditorSnippetTabButton:
                SetEditorTab(false);
                return 0;
            case kEditorEngineTabButton:
                SetEditorTab(true);
                return 0;
            case kSaveButton:
                SaveCurrentSnippet();
                return 0;
            case kResetButton:
                ResetEditor();
                return 0;
            case kSnippetEnabledCheckbox:
                toggle_checkbox(snippet_enabled_checkbox_);
                MarkEditorDirty();
                return 0;
            case kSettingsSaveButton:
                SaveEngineSettings();
                return 0;
            case kSettingsResetButton:
                ResetEngineSettings();
                return 0;
            case kAlwaysOnTopCheckbox:
                toggle_checkbox(always_on_top_checkbox_);
                settings_.always_on_top = IsOwnerDrawChecked(always_on_top_checkbox_);
                ApplyAlwaysOnTopSetting();
                if (SaveData()) {
                    SetStatusText(settings_.always_on_top ? L"Always on top enabled." : L"Always on top disabled.");
                } else {
                    SetStatusText(L"Always on top changed, but JSON save failed.");
                }
                return 0;
            case kStartWithWindowsCheckbox: {
                toggle_checkbox(start_with_windows_checkbox_);
                const bool enabled = IsOwnerDrawChecked(start_with_windows_checkbox_);
                if (!UpdateStartWithWindows(enabled)) {
                    SetOwnerDrawChecked(start_with_windows_checkbox_, !enabled);
                    SetStatusText(L"Could not update Start with Windows.");
                    return 0;
                }
                settings_.start_with_windows = enabled;
                if (SaveData()) {
                    SetStatusText(enabled ? L"Start with Windows enabled." : L"Start with Windows disabled.");
                } else {
                    SetStatusText(enabled ? L"Registry updated, but JSON save failed." : L"Registry updated, but JSON save failed.");
                }
                return 0;
            }
            case kMinimizeToTrayCheckbox:
                toggle_checkbox(minimize_to_tray_checkbox_);
                settings_.minimize_to_tray = IsOwnerDrawChecked(minimize_to_tray_checkbox_);
                if (SaveData()) {
                    SetStatusText(settings_.minimize_to_tray ? L"Minimize to tray enabled." : L"Minimize to tray disabled.");
                } else {
                    SetStatusText(L"Minimize to tray changed, but JSON save failed.");
                }
                return 0;
            case kPreviousClipboardCheckbox:
                toggle_checkbox(previous_clipboard_checkbox_);
                settings_.use_previous_clipboard_trigger = IsOwnerDrawChecked(previous_clipboard_checkbox_);
                typed_buffer_.clear();
                SetStatusText(settings_.use_previous_clipboard_trigger
                    ? L"Previous clipboard trigger enabled. Click Save Settings to persist."
                    : L"Previous clipboard trigger disabled. Click Save Settings to persist.");
                return 0;
            case kPreviousClipboardSlashCheckbox:
                toggle_checkbox(previous_clipboard_slash_checkbox_);
                settings_.use_previous_clipboard_slash_trigger = IsOwnerDrawChecked(previous_clipboard_slash_checkbox_);
                typed_buffer_.clear();
                SetStatusText(settings_.use_previous_clipboard_slash_trigger
                    ? L"// previous clipboard trigger enabled. Click Save Settings to persist."
                    : L"// previous clipboard trigger disabled. Click Save Settings to persist.");
                return 0;
            case kCaseSensitiveCheckbox:
                toggle_checkbox(case_sensitive_checkbox_);
                return 0;
            case kInstantModeRadio:
                set_trigger_mode_selection(false);
                return 0;
            case kSeparatorModeRadio:
                set_trigger_mode_selection(true);
                return 0;
            case kSeparatorSpaceCheckbox:
                toggle_checkbox(separator_space_checkbox_);
                return 0;
            case kSeparatorEnterCheckbox:
                toggle_checkbox(separator_enter_checkbox_);
                return 0;
            case kSeparatorTabCheckbox:
                toggle_checkbox(separator_tab_checkbox_);
                return 0;
            default:
                break;
            }
        }

        if (notify == 0 || notify == 1) {
            switch (control_id) {
            case kTrayMenuOpen:
                RestoreFromTray();
                return 0;
            case kTrayMenuToggleEngine:
                ToggleEngine();
                return 0;
            case kTrayMenuExit:
                PostMessageW(hwnd_, WM_CLOSE, 0, 0);
                return 0;
            case kSnippetMenuEdit:
                EditSelectedSnippet();
                return 0;
            case kSnippetMenuNew:
                StartNewSnippetCommand();
                return 0;
            case kSnippetMenuDuplicate:
                DuplicateSelectedSnippet();
                return 0;
            case kToggleSnippetButton:
                ToggleSelectedSnippets();
                return 0;
            case kSnippetMenuDelete:
                DeleteSelectedSnippet();
                return 0;
            case kSnippetPanelNewButton:
                StartNewSnippetCommand();
                return 0;
            case kNewGroupButton:
                CreateGroup();
                return 0;
            case kRenameGroupButton:
                RenameSelectedGroup();
                return 0;
            case kToggleGroupButton:
                ToggleSelectedGroup();
                return 0;
            case kDeleteGroupButton:
                DeleteSelectedGroup();
                return 0;
            case kTestResetButton:
                ResetTestArea();
                return 0;
            case kSaveButton:
                SaveCurrentSnippet();
                return 0;
            case kAppCommandSelectAll:
                if (HandleSelectAllShortcut()) {
                    return 0;
                }
                break;
            case kEditMenuUndo:
                if (active_context_edit_control_ != nullptr) {
                    SendMessageW(active_context_edit_control_, WM_UNDO, 0, 0);
                    return 0;
                }
                break;
            case kEditMenuCut:
                if (active_context_edit_control_ != nullptr) {
                    SendMessageW(active_context_edit_control_, WM_CUT, 0, 0);
                    return 0;
                }
                break;
            case kEditMenuCopy:
                if (active_context_edit_control_ != nullptr) {
                    SendMessageW(active_context_edit_control_, WM_COPY, 0, 0);
                    return 0;
                }
                break;
            case kEditMenuPaste:
                if (active_context_edit_control_ != nullptr) {
                    SendMessageW(active_context_edit_control_, WM_PASTE, 0, 0);
                    return 0;
                }
                break;
            case kEditMenuDelete:
                if (active_context_edit_control_ != nullptr) {
                    SendMessageW(active_context_edit_control_, WM_CLEAR, 0, 0);
                    return 0;
                }
                break;
            case kEditMenuSelectAll:
                if (active_context_edit_control_ != nullptr) {
                    SendMessageW(active_context_edit_control_, EM_SETSEL, 0, -1);
                    return 0;
                }
                break;
            case kEditMenuRtlReadingOrder:
                if (active_context_edit_control_ != nullptr) {
                    const bool enabled = ToggleEditControlRtlReading(active_context_edit_control_);
                    SetStatusText(enabled ? L"Right-to-left reading enabled." : L"Right-to-left reading disabled.");
                    return 0;
                }
                break;
            case kEditMenuShowUnicodeControls:
                if (active_context_edit_control_ != nullptr) {
                    std::wstring visible_text = MakeUnicodeControlCharactersVisible(ReadWindowText(active_context_edit_control_));
                    if (visible_text.empty()) {
                        visible_text = L"(Empty text)";
                    }
                    if (visible_text.size() > 1800) {
                        visible_text.resize(1800);
                        visible_text += L"\r\n\r\n[Preview truncated]";
                    }
                    PromptDialogConfig dialog_config;
                    dialog_config.title = L"Unicode control characters";
                    dialog_config.message = visible_text;
                    dialog_config.primary_label = L"OK";
                    dialog_config.dark_theme = IsDarkTheme();
                    PromptDialogResult dialog_result;
                    ShowPromptDialogWindow(hwnd_, instance_, dialog_config, dialog_result);
                    return 0;
                }
                break;
            case kEditMenuInsertUnicodeControl:
                if (active_context_edit_control_ != nullptr) {
                    return 0;
                }
                break;
            case kEditMenuOpenIme:
                if (active_context_edit_control_ != nullptr) {
                    if (OpenImeForControl(active_context_edit_control_)) {
                        SetStatusText(L"IME opened.");
                    } else {
                        SetStatusText(L"Could not open IME on this control.");
                    }
                    return 0;
                }
                break;
            case kEditMenuReconversion:
                if (active_context_edit_control_ != nullptr) {
                    if (RequestImeReconversion(active_context_edit_control_)) {
                        SetStatusText(L"IME reconversion requested.");
                    } else {
                        SetStatusText(L"Reconversion is not available here.");
                    }
                    return 0;
                }
                break;
            case kEditMenuInsertLrm:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u200E");
                    return 0;
                }
                break;
            case kEditMenuInsertRlm:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u200F");
                    return 0;
                }
                break;
            case kEditMenuInsertZwnj:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u200C");
                    return 0;
                }
                break;
            case kEditMenuInsertZwj:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u200D");
                    return 0;
                }
                break;
            case kEditMenuInsertLri:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u2066");
                    return 0;
                }
                break;
            case kEditMenuInsertRli:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u2067");
                    return 0;
                }
                break;
            case kEditMenuInsertFsi:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u2068");
                    return 0;
                }
                break;
            case kEditMenuInsertPdi:
                if (active_context_edit_control_ != nullptr) {
                    InsertTextIntoEditControl(active_context_edit_control_, L"\u2069");
                    return 0;
                }
                break;
            case kSnippetMenuFocusSearch:
                SetFocus(search_edit_);
                SendMessageW(search_edit_, EM_SETSEL, 0, -1);
                SetStatusText(L"Search box focused.");
                return 0;
            default:
                break;
            }
        }

        if (control_id == kGroupsList && notify == LBN_SELCHANGE) {
            const int next_group_index = static_cast<int>(SendMessageW(groups_list_, LB_GETCURSEL, 0, 0));
            selected_group_index_ = next_group_index == LB_ERR ? 0 : next_group_index;
            RefreshSnippetList(false);
            return 0;
        }

        if (control_id == kSearchEdit) {
            if (notify == EN_SETFOCUS) {
                HideSearchPlaceholder();
                return 0;
            }
            if (notify == EN_KILLFOCUS) {
                if (TrimCopy(ReadWindowText(search_edit_)).empty()) {
                    ShowSearchPlaceholder();
                    RefreshSnippetList(true);
                }
                return 0;
            }
            if (notify == EN_CHANGE) {
                if (!suppress_search_placeholder_updates_) {
                    search_placeholder_active_ = false;
                    RefreshSnippetList(true);
                }
                return 0;
            }
        }

        if (control_id == kTestEdit && notify == EN_CHANGE && !updating_test_area_) {
            ExpandTestAreaIfNeeded();
            return 0;
        }

        if ((control_hwnd == trigger_edit_ || control_hwnd == notes_edit_ || control_hwnd == content_edit_) && notify == EN_CHANGE) {
            MarkEditorDirty();
            return 0;
        }

        if (control_id == kEditorGroupCombo) {
            if (notify == CBN_DROPDOWN) {
                UpdateEditorGroupComboDropPanel(true);
                return 0;
            }
            if (notify == CBN_CLOSEUP) {
                UpdateEditorGroupComboDropPanel(false);
                return 0;
            }
            if (notify == CBN_SELCHANGE) {
                MarkEditorDirty();
                return 0;
            }
        }
        break;
    }

    case WM_DESTROY:
        RemoveClipboardFormatListener(hwnd_);
        if (is_capturing_hotkey_) {
            CancelHotkeyCapture(false);
        }
        StopGlobalServices();
        RemoveTrayIcon();
        ReleaseThemeBrushes();
        ReleaseToolbarButtonImages();
        ReleaseLoadedIcons();
        g_app_window = nullptr;
        if (ui_font_ != nullptr) {
            DeleteObject(ui_font_);
            ui_font_ = nullptr;
        }
        if (popup_menu_font_ != nullptr) {
            DeleteObject(popup_menu_font_);
            popup_menu_font_ = nullptr;
        }
        if (title_font_ != nullptr) {
            DeleteObject(title_font_);
            title_font_ = nullptr;
        }
        if (section_font_ != nullptr) {
            DeleteObject(section_font_);
            section_font_ = nullptr;
        }
        if (accelerators_ != nullptr) {
            DestroyAcceleratorTable(accelerators_);
            accelerators_ = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd_, message, wparam, lparam);
}

void AppWindow::CreateChildren() {
    INITCOMMONCONTROLSEX common_controls{};
    common_controls.dwSize = sizeof(common_controls);
    common_controls.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&common_controls);

    const DWORD label_style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    const DWORD button_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW;
    const DWORD checkbox_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW;
    const DWORD radio_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW;
    const DWORD input_style = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
    const DWORD multi_line_style = WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
    const DWORD list_style = WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT;
    const DWORD snippet_list_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS;
    const DWORD combo_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST;
    const DWORD editor_combo_style = combo_style | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS;

    title_label_ = CreateControl(0, L"STATIC", kAppDisplayName, label_style, 0, hwnd_, instance_);
    subtitle_label_ = CreateControl(0, L"STATIC", L"", label_style, 0, hwnd_, instance_);

    engine_button_ = CreateControl(0, L"BUTTON", L"Engine: ON", button_style, kEngineButton, hwnd_, instance_);
    theme_button_ = CreateControl(0, L"BUTTON", L"Theme: Dark", button_style, kThemeButton, hwnd_, instance_);
    import_button_ = CreateControl(0, L"BUTTON", L"Import", button_style, kImportButton, hwnd_, instance_);
    export_button_ = CreateControl(0, L"BUTTON", L"Export", button_style, kExportButton, hwnd_, instance_);
    github_button_ = CreateControl(0, L"BUTTON", L"GitHub", button_style, kGithubButton, hwnd_, instance_);
    info_button_ = CreateControl(0, L"BUTTON", L"Info", button_style, kInfoButton, hwnd_, instance_);
    restore_label_ = CreateControl(0, L"STATIC", L"Restore clipboard (ms)", label_style, 0, hwnd_, instance_);
    restore_edit_ = CreateControl(0, L"EDIT", L"60", input_style, 0, hwnd_, instance_);
    SetWindowSubclass(restore_edit_, EditContextSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    always_on_top_checkbox_ = CreateControl(0, L"BUTTON", L"Always on top", checkbox_style, kAlwaysOnTopCheckbox, hwnd_, instance_);
    start_with_windows_checkbox_ = CreateControl(0, L"BUTTON", L"Start with Windows", checkbox_style, kStartWithWindowsCheckbox, hwnd_, instance_);
    minimize_to_tray_checkbox_ = CreateControl(0, L"BUTTON", L"Minimize to tray", checkbox_style, kMinimizeToTrayCheckbox, hwnd_, instance_);
    previous_clipboard_checkbox_ = CreateControl(0, L"BUTTON", L"Use \\\\ for previous clipboard item", checkbox_style, kPreviousClipboardCheckbox, hwnd_, instance_);
    previous_clipboard_slash_checkbox_ = CreateControl(0, L"BUTTON", L"Use // for previous clipboard item", checkbox_style, kPreviousClipboardSlashCheckbox, hwnd_, instance_);
    hotkey_label_ = CreateControl(0, L"STATIC", L"Hotkey: ctrl+shift+f12", label_style, 0, hwnd_, instance_);
    record_hotkey_button_ = CreateControl(0, L"BUTTON", L"Record Hotkey", button_style, kRecordHotkeyButton, hwnd_, instance_);

    groups_title_ = CreateControl(0, L"STATIC", L"Groups", label_style, 0, hwnd_, instance_);
    new_group_button_ = CreateControl(0, L"BUTTON", L"New Group", button_style, kNewGroupButton, hwnd_, instance_);
    rename_group_button_ = CreateControl(0, L"BUTTON", L"Rename", button_style, kRenameGroupButton, hwnd_, instance_);
    toggle_group_button_ = CreateControl(0, L"BUTTON", L"Toggle", button_style, kToggleGroupButton, hwnd_, instance_);
    delete_group_button_ = CreateControl(0, L"BUTTON", L"Delete", button_style, kDeleteGroupButton, hwnd_, instance_);
    groups_list_ = CreateControl(0, L"LISTBOX", L"", list_style, kGroupsList, hwnd_, instance_);
    test_title_ = CreateControl(0, L"STATIC", L"Test Area", label_style, 0, hwnd_, instance_);
    test_edit_ = CreateControl(0, L"EDIT", L"", multi_line_style, kTestEdit, hwnd_, instance_);
    SetWindowSubclass(test_edit_, EditContextSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    test_reset_button_ = CreateControl(0, L"BUTTON", L"Reset Test Area", button_style, kTestResetButton, hwnd_, instance_);

    snippets_title_ = CreateControl(0, L"STATIC", L"Snippets", label_style, 0, hwnd_, instance_);
    search_edit_ = CreateControl(0, L"EDIT", L"", input_style, kSearchEdit, hwnd_, instance_);
    SetWindowSubclass(search_edit_, EditContextSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    snippets_list_ = CreateControl(0, WC_LISTVIEWW, L"", snippet_list_style, kSnippetsList, hwnd_, instance_);
    ListView_SetExtendedListViewStyle(
        snippets_list_,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP
    );
    SetWindowTheme(snippets_list_, L"", L"");
    if (const HWND header = ListView_GetHeader(snippets_list_); header != nullptr) {
        SetWindowTheme(header, L"", L"");
        SetWindowSubclass(header, SnippetsHeaderSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    }
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.cx = 140;
    column.iSubItem = 0;
    column.pszText = const_cast<LPWSTR>(L"Trigger");
    ListView_InsertColumn(snippets_list_, 0, &column);
    column.cx = 120;
    column.iSubItem = 1;
    column.pszText = const_cast<LPWSTR>(L"Group");
    ListView_InsertColumn(snippets_list_, 1, &column);
    column.cx = 64;
    column.iSubItem = 2;
    column.pszText = const_cast<LPWSTR>(L"On");
    ListView_InsertColumn(snippets_list_, 2, &column);
    column.cx = 260;
    column.iSubItem = 3;
    column.pszText = const_cast<LPWSTR>(L"Notes");
    ListView_InsertColumn(snippets_list_, 3, &column);
    column.cx = 280;
    column.iSubItem = 4;
    column.pszText = const_cast<LPWSTR>(L"Preview");
    ListView_InsertColumn(snippets_list_, 4, &column);
    new_snippet_button_ = CreateControl(0, L"BUTTON", L"Edit Snippet", button_style, kNewSnippetButton, hwnd_, instance_);
    duplicate_button_ = CreateControl(0, L"BUTTON", L"Duplicate", button_style, kDuplicateButton, hwnd_, instance_);
    toggle_snippet_button_ = CreateControl(0, L"BUTTON", L"Toggle Snippet", button_style, kToggleSnippetButton, hwnd_, instance_);
    delete_button_ = CreateControl(0, L"BUTTON", L"Delete Snippet", button_style, kDeleteButton, hwnd_, instance_);
    snippets_new_button_ = CreateControl(0, L"BUTTON", L"New", button_style, kSnippetPanelNewButton, hwnd_, instance_);

    editor_title_ = CreateControl(0, L"STATIC", L"Snippet Manager", label_style, 0, hwnd_, instance_);
    editor_snippet_tab_button_ = CreateControl(0, L"BUTTON", L"Snippet", button_style, kEditorSnippetTabButton, hwnd_, instance_);
    editor_engine_tab_button_ = CreateControl(0, L"BUTTON", L"Engine Settings", button_style, kEditorEngineTabButton, hwnd_, instance_);
    editor_hint_label_ = CreateControl(0, L"STATIC", L"", label_style, 0, hwnd_, instance_);
    trigger_label_ = CreateControl(0, L"STATIC", L"Trigger", label_style, 0, hwnd_, instance_);
    trigger_edit_ = CreateControl(0, L"EDIT", L"", input_style, 0, hwnd_, instance_);
    SetWindowSubclass(trigger_edit_, EditContextSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    group_label_ = CreateControl(0, L"STATIC", L"Group", label_style, 0, hwnd_, instance_);
    group_combo_ = CreateControl(0, L"COMBOBOX", L"", editor_combo_style, kEditorGroupCombo, hwnd_, instance_);
    SetWindowSubclass(group_combo_, ComboBoxSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    SendMessageW(group_combo_, CB_SETMINVISIBLE, 8, 0);
    SendMessageW(group_combo_, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), ScaleMainUi(28));
    SendMessageW(group_combo_, CB_SETITEMHEIGHT, 0, ScaleMainUi(32));
    snippet_enabled_checkbox_ = CreateControl(0, L"BUTTON", L"Snippet enabled", checkbox_style, kSnippetEnabledCheckbox, hwnd_, instance_);
    notes_label_ = CreateControl(0, L"STATIC", L"Notes", label_style, 0, hwnd_, instance_);
    notes_edit_ = CreateControl(0, L"EDIT", L"", input_style, 0, hwnd_, instance_);
    SetWindowSubclass(notes_edit_, EditContextSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    content_label_ = CreateControl(0, L"STATIC", L"Content", label_style, 0, hwnd_, instance_);
    content_edit_ = CreateControl(0, L"EDIT", L"", multi_line_style, 0, hwnd_, instance_);
    SetWindowSubclass(content_edit_, EditContextSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    editor_new_button_ = CreateControl(0, L"BUTTON", L"New", button_style, kEditorNewButton, hwnd_, instance_);
    save_button_ = CreateControl(0, L"BUTTON", L"Save", button_style, kSaveButton, hwnd_, instance_);
    reset_button_ = CreateControl(0, L"BUTTON", L"Reset", button_style, kResetButton, hwnd_, instance_);
    engine_settings_title_ = CreateControl(0, L"STATIC", L"Engine settings", label_style, 0, hwnd_, instance_);
    case_sensitive_checkbox_ = CreateControl(0, L"BUTTON", L"Case sensitive matching", checkbox_style, kCaseSensitiveCheckbox, hwnd_, instance_);
    trigger_mode_label_ = CreateControl(0, L"STATIC", L"Trigger mode", label_style, 0, hwnd_, instance_);
    instant_mode_radio_ = CreateControl(0, L"BUTTON", L"Instant", radio_style | WS_GROUP, kInstantModeRadio, hwnd_, instance_);
    separator_mode_radio_ = CreateControl(0, L"BUTTON", L"Separator", radio_style, kSeparatorModeRadio, hwnd_, instance_);
    separator_keys_label_ = CreateControl(0, L"STATIC", L"Separator keys for separator mode", label_style, 0, hwnd_, instance_);
    separator_space_checkbox_ = CreateControl(0, L"BUTTON", L"Space", checkbox_style, kSeparatorSpaceCheckbox, hwnd_, instance_);
    separator_enter_checkbox_ = CreateControl(0, L"BUTTON", L"Enter", checkbox_style, kSeparatorEnterCheckbox, hwnd_, instance_);
    separator_tab_checkbox_ = CreateControl(0, L"BUTTON", L"Tab", checkbox_style, kSeparatorTabCheckbox, hwnd_, instance_);
    settings_save_button_ = CreateControl(0, L"BUTTON", L"Save Settings", button_style, kSettingsSaveButton, hwnd_, instance_);
    settings_reset_button_ = CreateControl(0, L"BUTTON", L"Reset Settings", button_style, kSettingsResetButton, hwnd_, instance_);
    editor_settings_scrollbar_ = nullptr;

    status_label_ = CreateControl(0, L"STATIC", L"", label_style, 0, hwnd_, instance_);

    const HWND interactive_controls[] = {
        engine_button_, theme_button_, import_button_, export_button_, github_button_, info_button_,
        always_on_top_checkbox_, start_with_windows_checkbox_, minimize_to_tray_checkbox_,
        previous_clipboard_checkbox_, previous_clipboard_slash_checkbox_, record_hotkey_button_,
        new_group_button_, rename_group_button_, toggle_group_button_, delete_group_button_,
        test_reset_button_, new_snippet_button_, duplicate_button_, toggle_snippet_button_,
        delete_button_, snippets_new_button_, editor_snippet_tab_button_, editor_engine_tab_button_,
        snippet_enabled_checkbox_, editor_new_button_, save_button_, reset_button_,
        case_sensitive_checkbox_, instant_mode_radio_, separator_mode_radio_,
        separator_space_checkbox_, separator_enter_checkbox_, separator_tab_checkbox_,
        settings_save_button_, settings_reset_button_
    };
    for (HWND control : interactive_controls) {
        if (control != nullptr) {
            SetWindowSubclass(control, InteractiveControlSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
        }
    }

    const LPARAM edit_margins = MAKELPARAM(10, 10);
    const LPARAM content_margins = MAKELPARAM(8, 8);
    SendMessageW(search_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, edit_margins);
    SendMessageW(restore_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, edit_margins);
    SendMessageW(trigger_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, edit_margins);
    SendMessageW(notes_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, edit_margins);
    SendMessageW(test_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, content_margins);
    SendMessageW(content_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, content_margins);

    SendMessageW(search_edit_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(kSearchPlaceholderText));
    SendMessageW(restore_edit_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"ms"));
    ShowSearchPlaceholder();
    RefreshEditorTabButtons();
    UpdateEditorSectionVisibility();
}

void AppWindow::LoadData() {
    settings_ = Settings{};
    snippets_.clear();
    groups_.clear();
    loaded_data_from_file_ = false;

    data_file_path_ = DetermineDataFilePath();
    if (!data_file_path_.empty() && FileExists(data_file_path_) && LoadDataFromFile(data_file_path_)) {
        loaded_data_from_file_ = true;
        pending_status_text_ = L"Loaded " + std::to_wstring(snippets_.size()) + L" snippets from " + FileNameFromPath(data_file_path_) + L".";
        return;
    }

    if (data_file_path_.empty()) {
        data_file_path_ = JoinPath(GetExecutableDirectory(), kDefaultDataFileName);
    }

    LoadFallbackData();
    pending_status_text_ = L"Using fallback sample data until " + FileNameFromPath(data_file_path_) + L" is available.";
}

bool AppWindow::LoadDataFromFile(const std::wstring& path) {
    ParsedImportData parsed;
    std::wstring error;
    if (!ParseImportFile(path, parsed, error)) {
        pending_status_text_ = L"Could not parse " + FileNameFromPath(path) + L". " + error;
        return false;
    }

    settings_ = parsed.has_settings ? parsed.settings : Settings{};
    groups_ = std::move(parsed.groups);
    snippets_ = std::move(parsed.snippets);

    if (groups_.empty()) {
        groups_.push_back({L"General", true});
    }
    for (auto& snippet : snippets_) {
        if (TrimWhitespaceCopy(snippet.group).empty()) {
            snippet.group = groups_.front().name;
        }
        EnsureGroupExists(snippet.group);
    }

    if (snippets_.empty()) {
        pending_status_text_ = L"Loaded " + FileNameFromPath(path) + L", but it does not contain snippets.";
    } else if (parsed.is_beeftext_format) {
        pending_status_text_ = L"Loaded Beeftext combos from " + FileNameFromPath(path) + L".";
    }
    engine_enabled_ = settings_.engine_enabled;
    return true;
}

bool AppWindow::ParseImportFile(const std::wstring& path, ParsedImportData& data, std::wstring& error) const {
    const std::string bytes = ReadFileBytes(path);
    const std::wstring source = Utf8ToWide(bytes);
    if (source.empty()) {
        error = L"File is empty or could not be read.";
        return false;
    }

    simplejson::Value root;
    if (!simplejson::Parse(source, root, &error) || root.type != simplejson::Value::Type::Object) {
        if (error.empty()) {
            error = L"JSON parsing failed.";
        }
        return false;
    }

    if (const simplejson::Value* combos = FindField(&root, L"combos");
        combos != nullptr && combos->type == simplejson::Value::Type::Array) {
        if (!ParseBeeftextData(root, data)) {
            error = L"Unsupported Beeftext combo structure.";
            return false;
        }
        return true;
    }

    if (ParseNativeData(root, data)) {
        return true;
    }

    error = L"Unsupported JSON format. Expected BlinkText snippets or Beeftext combos.";
    return false;
}

bool AppWindow::ParseNativeData(const simplejson::Value& root, ParsedImportData& data) const {
    data = ParsedImportData{};
    bool saw_supported_shape = false;
    bool explicit_group_info = false;

    if (const simplejson::Value* settings = FindField(&root, L"settings")) {
        saw_supported_shape = true;
        data.has_settings = true;
        data.settings.theme = JsonStringValue(FindField(settings, L"theme"), data.settings.theme);
        data.settings.always_on_top = JsonBoolValue(FindField(settings, L"always_on_top"), data.settings.always_on_top);
        data.settings.restore_clipboard_delay_ms = JsonIntValue(FindField(settings, L"restore_clipboard_delay_ms"), data.settings.restore_clipboard_delay_ms);
        data.settings.match_case_sensitive = JsonBoolValue(FindField(settings, L"match_case_sensitive"), data.settings.match_case_sensitive);
        data.settings.trigger_mode = JsonStringValue(FindField(settings, L"trigger_mode"), data.settings.trigger_mode);
        data.settings.hotkey = JsonStringValue(FindField(settings, L"hotkey"), data.settings.hotkey);
        data.settings.engine_enabled = JsonBoolValue(FindField(settings, L"engine_enabled"), data.settings.engine_enabled);
        data.settings.instant_settle_ms = JsonIntValue(FindField(settings, L"instant_settle_ms"), data.settings.instant_settle_ms);
        data.settings.backspace_delay_ms = JsonIntValue(FindField(settings, L"backspace_delay_ms"), data.settings.backspace_delay_ms);
        data.settings.start_with_windows = JsonBoolValue(FindField(settings, L"start_with_windows"), data.settings.start_with_windows);
        data.settings.minimize_to_tray = JsonBoolValue(FindField(settings, L"minimize_to_tray"), data.settings.minimize_to_tray);
        data.settings.use_previous_clipboard_trigger = JsonBoolValue(FindField(settings, L"use_previous_clipboard_trigger"), data.settings.use_previous_clipboard_trigger);
        data.settings.use_previous_clipboard_slash_trigger = JsonBoolValue(FindField(settings, L"use_previous_clipboard_slash_trigger"), data.settings.use_previous_clipboard_slash_trigger);
        data.settings.window_bounds_valid = JsonBoolValue(FindField(settings, L"window_bounds_valid"), data.settings.window_bounds_valid);
        data.settings.window_x = JsonIntValue(FindField(settings, L"window_x"), data.settings.window_x);
        data.settings.window_y = JsonIntValue(FindField(settings, L"window_y"), data.settings.window_y);
        data.settings.window_width = JsonIntValue(FindField(settings, L"window_width"), data.settings.window_width);
        data.settings.window_height = JsonIntValue(FindField(settings, L"window_height"), data.settings.window_height);
        data.settings.left_panel_width = JsonIntValue(FindField(settings, L"left_panel_width"), data.settings.left_panel_width);
        data.settings.right_panel_width = JsonIntValue(FindField(settings, L"right_panel_width"), data.settings.right_panel_width);
        data.settings.groups_panel_height = JsonIntValue(FindField(settings, L"groups_panel_height"), data.settings.groups_panel_height);
        data.settings.last_group_filter = TrimWhitespaceCopy(JsonStringValue(FindField(settings, L"last_group_filter")));
        data.settings.snippet_sort_column = JsonIntValue(FindField(settings, L"snippet_sort_column"), data.settings.snippet_sort_column);
        data.settings.snippet_sort_ascending = JsonBoolValue(FindField(settings, L"snippet_sort_ascending"), data.settings.snippet_sort_ascending);

        if (const simplejson::Value* separators = FindField(settings, L"word_separators");
            separators != nullptr && separators->type == simplejson::Value::Type::Array) {
            data.settings.word_separators.clear();
            for (const auto& item : separators->array_value) {
                if (item.type == simplejson::Value::Type::String) {
                    data.settings.word_separators.push_back(item.string_value);
                }
            }
            if (data.settings.word_separators.empty()) {
                data.settings.word_separators = Settings{}.word_separators;
            }
        }
    }

    if (const simplejson::Value* groups = FindField(&root, L"groups");
        groups != nullptr && groups->type == simplejson::Value::Type::Array) {
        saw_supported_shape = true;
        for (const auto& item : groups->array_value) {
            std::wstring name;
            bool enabled = true;

            if (item.type == simplejson::Value::Type::Object) {
                name = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"name")));
                enabled = JsonBoolValue(FindField(&item, L"enabled"), true);
            } else if (item.type == simplejson::Value::Type::String) {
                name = TrimWhitespaceCopy(item.string_value);
            }

            if (name.empty()) {
                continue;
            }

            explicit_group_info = true;
            if (std::none_of(data.groups.begin(), data.groups.end(), [&name](const Group& group) { return group.name == name; })) {
                data.groups.push_back({name, enabled});
            }
        }
    }

    if (const simplejson::Value* snippets = FindField(&root, L"snippets");
        snippets != nullptr && snippets->type == simplejson::Value::Type::Array) {
        saw_supported_shape = true;
        for (const auto& item : snippets->array_value) {
            if (item.type != simplejson::Value::Type::Object) {
                continue;
            }

            Snippet snippet;
            snippet.trigger = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"trigger")));
            snippet.content = JsonStringValue(FindField(&item, L"content"));
            snippet.notes = JsonStringValue(FindField(&item, L"notes"));
            snippet.enabled = JsonBoolValue(FindField(&item, L"enabled"), true);

            const simplejson::Value* group_value = FindField(&item, L"group");
            const std::wstring explicit_group = TrimWhitespaceCopy(JsonStringValue(group_value));
            if (!explicit_group.empty()) {
                snippet.group = explicit_group;
                explicit_group_info = true;
            }

            if (snippet.trigger.empty()) {
                continue;
            }
            data.snippets.push_back(std::move(snippet));
        }
    }

    data.source_groups_available = explicit_group_info;
    data.is_beeftext_format = false;
    return saw_supported_shape;
}

bool AppWindow::ParseBeeftextData(const simplejson::Value& root, ParsedImportData& data) const {
    data = ParsedImportData{};
    data.is_beeftext_format = true;
    data.has_settings = true;

    std::vector<std::wstring> detected_group_names;
    if (const simplejson::Value* groups = FindField(&root, L"groups");
        groups != nullptr && groups->type == simplejson::Value::Type::Array) {
        for (const auto& item : groups->array_value) {
            std::wstring group_name;
            if (item.type == simplejson::Value::Type::Object) {
                group_name = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"name")));
            } else if (item.type == simplejson::Value::Type::String) {
                group_name = TrimWhitespaceCopy(item.string_value);
            }
            if (!group_name.empty() &&
                std::none_of(detected_group_names.begin(), detected_group_names.end(), [&group_name](const std::wstring& existing) { return existing == group_name; })) {
                detected_group_names.push_back(group_name);
                data.groups.push_back({group_name, true});
            }
        }
    }

    const std::wstring single_detected_group = detected_group_names.size() == 1 ? detected_group_names.front() : L"";
    bool saw_combo_group = false;

    const simplejson::Value* combos = FindField(&root, L"combos");
    if (combos == nullptr || combos->type != simplejson::Value::Type::Array) {
        return false;
    }

    for (const auto& item : combos->array_value) {
        if (item.type != simplejson::Value::Type::Object) {
            continue;
        }

        Snippet snippet;
        snippet.trigger = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"keyword")));
        snippet.content = JsonStringValue(FindField(&item, L"snippet"));
        const std::wstring description = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"description")));
        const std::wstring name = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"name")));
        snippet.notes = !description.empty() ? description : name;
        snippet.enabled = JsonBoolValue(FindField(&item, L"enabled"), true);

        std::wstring group_name = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"group")));
        if (group_name.empty()) {
            group_name = TrimWhitespaceCopy(JsonStringValue(FindField(&item, L"groupName")));
        }
        if (group_name.empty()) {
            group_name = single_detected_group;
        }
        if (!group_name.empty()) {
            saw_combo_group = true;
            snippet.group = group_name;
            if (std::none_of(data.groups.begin(), data.groups.end(), [&group_name](const Group& group) { return group.name == group_name; })) {
                data.groups.push_back({group_name, true});
            }
        }

        data.settings.match_case_sensitive = data.settings.match_case_sensitive || JsonIntValue(FindField(&item, L"caseSensitivity"), 0) != 0;

        if (snippet.trigger.empty()) {
            continue;
        }
        data.snippets.push_back(std::move(snippet));
    }

    data.source_groups_available = saw_combo_group;
    return true;
}

bool AppWindow::SaveData() {
    settings_.engine_enabled = engine_enabled_;
    CaptureWindowBounds();
    settings_.last_group_filter = CurrentGroupFilter();
    settings_.snippet_sort_column = std::clamp(snippet_sort_column_, 0, 4);
    settings_.snippet_sort_ascending = snippet_sort_ascending_;
    const std::wstring restore_text = TrimCopy(ReadWindowText(restore_edit_));
    if (!restore_text.empty()) {
        try {
            settings_.restore_clipboard_delay_ms = std::max(0, std::stoi(restore_text));
        } catch (...) {
            settings_.restore_clipboard_delay_ms = std::max(0, settings_.restore_clipboard_delay_ms);
        }
    }

    if (data_file_path_.empty()) {
        data_file_path_ = DetermineDataFilePath();
    }
    if (data_file_path_.empty()) {
        return false;
    }
    return SaveDataToFile(data_file_path_);
}

bool AppWindow::SaveDataToFile(const std::wstring& path) const {
    simplejson::Value root = simplejson::Value::Object();

    simplejson::Value settings = simplejson::Value::Object();
    settings.object_value.emplace_back(L"theme", simplejson::Value::String(settings_.theme));
    settings.object_value.emplace_back(L"always_on_top", simplejson::Value::Bool(settings_.always_on_top));
    settings.object_value.emplace_back(L"restore_clipboard_delay_ms", simplejson::Value::Number(settings_.restore_clipboard_delay_ms));
    settings.object_value.emplace_back(L"match_case_sensitive", simplejson::Value::Bool(settings_.match_case_sensitive));
    settings.object_value.emplace_back(L"trigger_mode", simplejson::Value::String(settings_.trigger_mode));
    settings.object_value.emplace_back(L"word_separators", JsonStringArray(settings_.word_separators));
    settings.object_value.emplace_back(L"hotkey", simplejson::Value::String(settings_.hotkey));
    settings.object_value.emplace_back(L"engine_enabled", simplejson::Value::Bool(engine_enabled_));
    settings.object_value.emplace_back(L"instant_settle_ms", simplejson::Value::Number(settings_.instant_settle_ms));
    settings.object_value.emplace_back(L"backspace_delay_ms", simplejson::Value::Number(settings_.backspace_delay_ms));
    settings.object_value.emplace_back(L"start_with_windows", simplejson::Value::Bool(settings_.start_with_windows));
    settings.object_value.emplace_back(L"minimize_to_tray", simplejson::Value::Bool(settings_.minimize_to_tray));
    settings.object_value.emplace_back(L"use_previous_clipboard_trigger", simplejson::Value::Bool(settings_.use_previous_clipboard_trigger));
    settings.object_value.emplace_back(L"use_previous_clipboard_slash_trigger", simplejson::Value::Bool(settings_.use_previous_clipboard_slash_trigger));
    settings.object_value.emplace_back(L"window_bounds_valid", simplejson::Value::Bool(settings_.window_bounds_valid));
    settings.object_value.emplace_back(L"window_x", simplejson::Value::Number(settings_.window_x));
    settings.object_value.emplace_back(L"window_y", simplejson::Value::Number(settings_.window_y));
    settings.object_value.emplace_back(L"window_width", simplejson::Value::Number(settings_.window_width));
    settings.object_value.emplace_back(L"window_height", simplejson::Value::Number(settings_.window_height));
    settings.object_value.emplace_back(L"left_panel_width", simplejson::Value::Number(settings_.left_panel_width));
    settings.object_value.emplace_back(L"right_panel_width", simplejson::Value::Number(settings_.right_panel_width));
    settings.object_value.emplace_back(L"groups_panel_height", simplejson::Value::Number(settings_.groups_panel_height));
    settings.object_value.emplace_back(L"last_group_filter", simplejson::Value::String(settings_.last_group_filter));
    settings.object_value.emplace_back(L"snippet_sort_column", simplejson::Value::Number(settings_.snippet_sort_column));
    settings.object_value.emplace_back(L"snippet_sort_ascending", simplejson::Value::Bool(settings_.snippet_sort_ascending));
    root.object_value.emplace_back(L"settings", std::move(settings));

    simplejson::Value groups = simplejson::Value::Array();
    for (const auto& group : groups_) {
        simplejson::Value item = simplejson::Value::Object();
        item.object_value.emplace_back(L"name", simplejson::Value::String(group.name));
        item.object_value.emplace_back(L"enabled", simplejson::Value::Bool(group.enabled));
        groups.array_value.push_back(std::move(item));
    }
    root.object_value.emplace_back(L"groups", std::move(groups));

    simplejson::Value snippets = simplejson::Value::Array();
    for (const auto& snippet : snippets_) {
        simplejson::Value item = simplejson::Value::Object();
        item.object_value.emplace_back(L"trigger", simplejson::Value::String(snippet.trigger));
        item.object_value.emplace_back(L"content", simplejson::Value::String(snippet.content));
        item.object_value.emplace_back(L"group", simplejson::Value::String(snippet.group));
        item.object_value.emplace_back(L"notes", simplejson::Value::String(snippet.notes));
        item.object_value.emplace_back(L"enabled", simplejson::Value::Bool(snippet.enabled));
        snippets.array_value.push_back(std::move(item));
    }
    root.object_value.emplace_back(L"snippets", std::move(snippets));

    std::wstring text = simplejson::Stringify(root, 4);
    text += L"\n";
    return WriteFileBytesAtomically(path, WideToUtf8(text));
}

bool AppWindow::ExportDataToFile(const std::wstring& path, const ExportOptions& options) const {
    const std::wstring group_filter = options.current_group_only ? CurrentGroupFilter() : L"";

    std::vector<const Snippet*> export_snippets;
    export_snippets.reserve(snippets_.size());
    for (const auto& snippet : snippets_) {
        if (!group_filter.empty() && snippet.group != group_filter) {
            continue;
        }
        export_snippets.push_back(&snippet);
    }

    std::vector<const Group*> export_groups;
    if (options.current_group_only && !group_filter.empty()) {
        for (const auto& group : groups_) {
            if (group.name == group_filter) {
                export_groups.push_back(&group);
                break;
            }
        }
    } else {
        for (const auto& group : groups_) {
            export_groups.push_back(&group);
        }
    }

    simplejson::Value root = simplejson::Value::Object();
    if (options.beeftext_format) {
        simplejson::Value groups = simplejson::Value::Array();
        for (const Group* group : export_groups) {
            simplejson::Value item = simplejson::Value::Object();
            item.object_value.emplace_back(L"name", simplejson::Value::String(group->name));
            groups.array_value.push_back(std::move(item));
        }
        root.object_value.emplace_back(L"groups", std::move(groups));

        simplejson::Value combos = simplejson::Value::Array();
        for (const Snippet* snippet : export_snippets) {
            simplejson::Value item = simplejson::Value::Object();
            item.object_value.emplace_back(L"keyword", simplejson::Value::String(snippet->trigger));
            item.object_value.emplace_back(L"snippet", simplejson::Value::String(snippet->content));
            item.object_value.emplace_back(L"description", simplejson::Value::String(snippet->notes));
            item.object_value.emplace_back(L"name", simplejson::Value::String(snippet->notes.empty() ? snippet->trigger : snippet->notes));
            item.object_value.emplace_back(L"enabled", simplejson::Value::Bool(snippet->enabled));
            item.object_value.emplace_back(L"groupName", simplejson::Value::String(snippet->group));
            item.object_value.emplace_back(L"caseSensitivity", simplejson::Value::Number(settings_.match_case_sensitive ? 1 : 0));
            combos.array_value.push_back(std::move(item));
        }
        root.object_value.emplace_back(L"combos", std::move(combos));
    } else {
        simplejson::Value settings = simplejson::Value::Object();
        settings.object_value.emplace_back(L"theme", simplejson::Value::String(settings_.theme));
        settings.object_value.emplace_back(L"always_on_top", simplejson::Value::Bool(settings_.always_on_top));
        settings.object_value.emplace_back(L"restore_clipboard_delay_ms", simplejson::Value::Number(settings_.restore_clipboard_delay_ms));
        settings.object_value.emplace_back(L"match_case_sensitive", simplejson::Value::Bool(settings_.match_case_sensitive));
        settings.object_value.emplace_back(L"trigger_mode", simplejson::Value::String(settings_.trigger_mode));
        settings.object_value.emplace_back(L"word_separators", JsonStringArray(settings_.word_separators));
        settings.object_value.emplace_back(L"hotkey", simplejson::Value::String(settings_.hotkey));
        settings.object_value.emplace_back(L"engine_enabled", simplejson::Value::Bool(engine_enabled_));
        settings.object_value.emplace_back(L"instant_settle_ms", simplejson::Value::Number(settings_.instant_settle_ms));
        settings.object_value.emplace_back(L"backspace_delay_ms", simplejson::Value::Number(settings_.backspace_delay_ms));
        settings.object_value.emplace_back(L"start_with_windows", simplejson::Value::Bool(settings_.start_with_windows));
        settings.object_value.emplace_back(L"minimize_to_tray", simplejson::Value::Bool(settings_.minimize_to_tray));
        settings.object_value.emplace_back(L"use_previous_clipboard_trigger", simplejson::Value::Bool(settings_.use_previous_clipboard_trigger));
        settings.object_value.emplace_back(L"use_previous_clipboard_slash_trigger", simplejson::Value::Bool(settings_.use_previous_clipboard_slash_trigger));
        root.object_value.emplace_back(L"settings", std::move(settings));

        simplejson::Value groups = simplejson::Value::Array();
        for (const Group* group : export_groups) {
            simplejson::Value item = simplejson::Value::Object();
            item.object_value.emplace_back(L"name", simplejson::Value::String(group->name));
            item.object_value.emplace_back(L"enabled", simplejson::Value::Bool(group->enabled));
            groups.array_value.push_back(std::move(item));
        }
        root.object_value.emplace_back(L"groups", std::move(groups));

        simplejson::Value snippets = simplejson::Value::Array();
        for (const Snippet* snippet : export_snippets) {
            simplejson::Value item = simplejson::Value::Object();
            item.object_value.emplace_back(L"trigger", simplejson::Value::String(snippet->trigger));
            item.object_value.emplace_back(L"content", simplejson::Value::String(snippet->content));
            item.object_value.emplace_back(L"group", simplejson::Value::String(snippet->group));
            item.object_value.emplace_back(L"notes", simplejson::Value::String(snippet->notes));
            item.object_value.emplace_back(L"enabled", simplejson::Value::Bool(snippet->enabled));
            snippets.array_value.push_back(std::move(item));
        }
        root.object_value.emplace_back(L"snippets", std::move(snippets));
    }

    std::wstring text = simplejson::Stringify(root, 4);
    text += L"\n";
    return WriteFileBytesAtomically(path, WideToUtf8(text));
}

bool AppWindow::ShowImportOptionsDialog(std::wstring& file_path, ParsedImportData& data, ImportOptions& options) const {
    ImportDialogConfig config;
    config.owner = this;
    config.file_path = file_path;
    config.imported_count = static_cast<int>(data.snippets.size());
    config.conflict_count = CountImportConflicts(data);
    config.allow_keep_source_groups = data.source_groups_available;
    config.dark_theme = IsDarkTheme();
    config.default_group = CurrentGroupFilter();
    if (config.default_group.empty()) {
        config.default_group = groups_.empty() ? L"General" : groups_.front().name;
    }

    if (data.is_beeftext_format) {
        config.format_label = L"Detected format: Beeftext combos JSON";
    } else {
        config.format_label = L"Detected format: Native BlinkText JSON";
    }

    for (const auto& group : groups_) {
        if (std::none_of(config.existing_groups.begin(), config.existing_groups.end(), [&group](const std::wstring& name) { return name == group.name; })) {
            config.existing_groups.push_back(group.name);
        }
    }
    if (config.existing_groups.empty()) {
        config.existing_groups.push_back(L"General");
    }

    ImportDialogResult result;
    result.file_path = file_path;
    result.parsed_data = data;
    if (!ShowImportOptionsDialogWindow(hwnd_, instance_, config, result)) {
        return false;
    }

    file_path = result.file_path;
    data = result.parsed_data;
    options.keep_source_groups = result.keep_source_groups && data.source_groups_available;
    options.target_group = result.target_group;
    options.overwrite_conflicts = result.overwrite_conflicts;
    return true;
}

bool AppWindow::ShowExportOptionsDialog(ExportOptions& options) const {
    ExportOptionsDialogConfig config;
    config.current_group_name = CurrentGroupFilter();
    config.all_snippet_count = static_cast<int>(snippets_.size());
    config.dark_theme = IsDarkTheme();
    if (!config.current_group_name.empty()) {
        for (const auto& snippet : snippets_) {
            if (snippet.group == config.current_group_name) {
                ++config.current_group_snippet_count;
            }
        }
    }
    config.can_export_current_group = !config.current_group_name.empty();

    ExportOptionsDialogResult result;
    if (!ShowExportOptionsDialogWindow(hwnd_, instance_, config, result)) {
        return false;
    }

    options.beeftext_format = result.beeftext_format;
    options.current_group_only = result.current_group_only;
    return true;
}

int AppWindow::CountImportConflicts(const ParsedImportData& data) const {
    int conflict_count = 0;
    for (const auto& imported : data.snippets) {
        if (imported.trigger.empty()) {
            continue;
        }
        if (std::any_of(snippets_.begin(), snippets_.end(), [this, &imported](const Snippet& existing) {
                return TriggersEqual(existing.trigger, imported.trigger);
            })) {
            ++conflict_count;
        }
    }
    return conflict_count;
}

void AppWindow::MergeImportedData(const ParsedImportData& data, const ImportOptions& options, int& added_count, int& updated_count, int& skipped_count) {
    added_count = 0;
    updated_count = 0;
    skipped_count = 0;

    if (options.keep_source_groups && data.source_groups_available) {
        for (const auto& group : data.groups) {
            auto it = std::find_if(groups_.begin(), groups_.end(), [&group](const Group& existing) {
                return existing.name == group.name;
            });
            if (it == groups_.end()) {
                groups_.push_back(group);
            }
        }
    } else {
        const std::wstring fallback_group = options.target_group.empty() ? L"General" : options.target_group;
        EnsureGroupExists(fallback_group);
    }

    for (const auto& imported : data.snippets) {
        if (imported.trigger.empty()) {
            continue;
        }

        Snippet candidate = imported;
        if (options.keep_source_groups && data.source_groups_available) {
            if (TrimWhitespaceCopy(candidate.group).empty()) {
                candidate.group = groups_.empty() ? L"General" : groups_.front().name;
            }
        } else {
            candidate.group = options.target_group.empty() ? L"General" : options.target_group;
        }
        EnsureGroupExists(candidate.group);

        auto existing = std::find_if(snippets_.begin(), snippets_.end(), [this, &candidate](const Snippet& item) {
            return TriggersEqual(item.trigger, candidate.trigger);
        });

        if (existing == snippets_.end()) {
            snippets_.push_back(std::move(candidate));
            ++added_count;
            continue;
        }

        if (options.overwrite_conflicts) {
            *existing = std::move(candidate);
            ++updated_count;
        } else {
            ++skipped_count;
        }
    }
}

void AppWindow::ImportDataFromDialog() {
    wchar_t file_path[32768] = {};
    if (!data_file_path_.empty()) {
        wcsncpy_s(file_path, data_file_path_.c_str(), _TRUNCATE);
    }

    wchar_t filter[] = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = file_path;
    dialog.nMaxFile = static_cast<DWORD>(std::size(file_path));
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = L"json";

    if (!GetOpenFileNameW(&dialog)) {
        return;
    }

    std::wstring selected_file_path = file_path;
    ParsedImportData imported_data;
    std::wstring error;
    if (!ParseImportFile(selected_file_path, imported_data, error)) {
        const std::wstring message = L"Import failed: " + (error.empty() ? std::wstring(L"unsupported JSON format.") : error);
        SetStatusText(message.c_str());
        return;
    }
    if (imported_data.snippets.empty()) {
        SetStatusText(L"Import cancelled: the selected file does not contain snippets.");
        return;
    }

    ImportOptions options;
    if (!ShowImportOptionsDialog(selected_file_path, imported_data, options)) {
        SetStatusText(L"Import cancelled.");
        return;
    }

    int added_count = 0;
    int updated_count = 0;
    int skipped_count = 0;
    MergeImportedData(imported_data, options, added_count, updated_count, skipped_count);

    ShowSearchPlaceholder();
    if (!options.keep_source_groups && !TrimWhitespaceCopy(options.target_group).empty()) {
        selected_group_index_ = 0;
        for (int index = 0; index < static_cast<int>(groups_.size()); ++index) {
            if (groups_[index].name == options.target_group) {
                selected_group_index_ = index + 1;
                break;
            }
        }
    } else {
        selected_group_index_ = 0;
    }

    RefreshGroups();
    RefreshSnippetList(false);
    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        PopulateEditorFromSelection();
    } else {
        CreateNewSnippet();
    }

    const bool saved = SaveData();
    std::wstring status = L"Imported from " + FileNameFromPath(file_path) +
        L" | added: " + std::to_wstring(added_count) +
        L", updated: " + std::to_wstring(updated_count) +
        L", skipped: " + std::to_wstring(skipped_count);
    if (!saved) {
        status += L" | save failed";
    }
    SetStatusText(status.c_str());
}

void AppWindow::ExportDataToDialog() {
    if (!ConfirmDiscardEditorChanges(L"with export")) {
        return;
    }

    ExportOptions options;
    if (!ShowExportOptionsDialog(options)) {
        SetStatusText(L"Export cancelled.");
        return;
    }

    const std::wstring current_group = options.current_group_only ? CurrentGroupFilter() : L"";
    auto directory_from_path = [](const std::wstring& path) {
        const size_t slash = path.find_last_of(L"\\/");
        if (slash == std::wstring::npos) {
            return std::wstring();
        }
        return path.substr(0, slash);
    };
    auto make_export_timestamp = []() {
        SYSTEMTIME local_time{};
        GetLocalTime(&local_time);
        wchar_t buffer[64] = {};
        swprintf_s(
            buffer,
            L"%02u-%02u-%04u-%02u-%02u",
            static_cast<unsigned>(local_time.wDay),
            static_cast<unsigned>(local_time.wMonth),
            static_cast<unsigned>(local_time.wYear),
            static_cast<unsigned>(local_time.wHour),
            static_cast<unsigned>(local_time.wMinute)
        );
        return std::wstring(buffer);
    };

    const std::wstring timestamp = make_export_timestamp();
    std::wstring suggested_name = options.beeftext_format
        ? (L"BlinkText-BeefText-" + timestamp + L".json")
        : (L"BlinkText-" + timestamp + L".json");

    std::wstring suggested_path;
    const std::wstring documents_directory = GetDocumentsDirectory();
    if (!documents_directory.empty()) {
        suggested_path = JoinPath(documents_directory, suggested_name);
    } else if (!data_file_path_.empty()) {
        const std::wstring directory = directory_from_path(data_file_path_);
        suggested_path = directory.empty() ? suggested_name : JoinPath(directory, suggested_name);
    } else {
        suggested_path = suggested_name;
    }

    wchar_t file_path[32768] = {};
    wcsncpy_s(file_path, suggested_path.c_str(), _TRUNCATE);

    const wchar_t native_filter[] = L"Native BlinkText JSON (*.json)\0*.json\0All Files (*.*)\0*.*\0\0";
    const wchar_t beeftext_filter[] = L"Beeftext Combos JSON (*.json)\0*.json\0All Files (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter = options.beeftext_format ? beeftext_filter : native_filter;
    dialog.lpstrFile = file_path;
    dialog.lpstrInitialDir = documents_directory.empty() ? nullptr : documents_directory.c_str();
    dialog.nMaxFile = static_cast<DWORD>(std::size(file_path));
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = L"json";

    if (!GetSaveFileNameW(&dialog)) {
        SetStatusText(L"Export cancelled.");
        return;
    }

    if (ExportDataToFile(file_path, options)) {
        std::wstring status = L"Exported ";
        status += options.beeftext_format ? L"Beeftext combos" : L"native BlinkText JSON";
        if (options.current_group_only && !current_group.empty()) {
            status += L" for group '" + current_group + L"'";
        } else {
            status += L" for all snippets";
        }
        status += L" to " + FileNameFromPath(file_path);
        SetStatusText(status.c_str());
    } else {
        SetStatusText(L"Export failed.");
    }
}

void AppWindow::LoadFallbackData() {
    settings_ = Settings{};
    settings_.engine_enabled = true;
    groups_ = {
        {L"General", true},
        {L"Work", true},
    };
    snippets_ = {
        {L"sig\\", L"General", L"Signature sample", L"Best regards,\r\nEslam Mustafa\r\n+201283447065\r\nEslam.G.Youssef@gmail.com\r\nhttps://github.com/LeaDer-E", true},
        {L"addr\\", L"Work", L"Address sample", L"BlinkText v1.0\r\n\r\nFast, local text expansion tool designed for instant, reliable typing with zero delay.\r\n\r\nKey Features\r\n\r\nInstant expansion without paste conflicts\r\nNo trigger duplication during rapid input (e.g., Ctrl+V after trigger)\r\nFully offline – no data collection\r\nLightweight and optimized for speed\r\nSupports importing exported triggers from compatible tools\r\n\r\nCompatibility\r\n\r\nSupports importing exported triggers from Beeftext for seamless migration\r\n\r\nPrivacy\r\n\r\nNo user data is collected, stored, or transmitted\r\n\r\nDeveloper\r\n\r\nDeveloped by: https://github.com/LeaDer-E\r\nContact: Eslam.Youssef@protonmail.com", true},
    };
    engine_enabled_ = settings_.engine_enabled;
}

void AppWindow::ApplyLoadedSettings() {
    const auto scale_legacy_metric = [](int value, int legacy_threshold, int minimum_value) {
        if (value >= legacy_threshold) {
            return std::max(minimum_value, ScaleMainUi(value));
        }
        return std::max(minimum_value, value);
    };

    settings_.left_panel_width = scale_legacy_metric(settings_.left_panel_width, 250, kFixedLeftPanelWidth);
    settings_.right_panel_width = scale_legacy_metric(settings_.right_panel_width, 360, kFixedRightPanelWidth);
    settings_.groups_panel_height = scale_legacy_metric(settings_.groups_panel_height, 300, ScaleMainUi(230));
    if (settings_.window_bounds_valid &&
        (settings_.window_width >= 1350 || settings_.window_height >= 950)) {
        settings_.window_width = std::max(kMinWindowWidth, ScaleMainUi(settings_.window_width));
        settings_.window_height = std::max(kMinWindowHeight, ScaleMainUi(settings_.window_height));
    }

    if (settings_.start_with_windows) {
        settings_.start_with_windows = UpdateStartWithWindows(true) || ReadStartWithWindows();
    } else {
        settings_.start_with_windows = ReadStartWithWindows();
    }
    engine_enabled_ = settings_.engine_enabled;
    WriteWindowText(restore_edit_, std::to_wstring(std::max(0, settings_.restore_clipboard_delay_ms)));
    SetWindowTextW(subtitle_label_, L"");

    UINT modifiers = 0;
    UINT vk_code = 0;
    if (!ParseHotkeyString(settings_.hotkey, modifiers, vk_code)) {
        settings_.hotkey = L"ctrl+shift+f12";
        modifiers = MOD_CONTROL | MOD_SHIFT;
        vk_code = VK_F12;
    }
    registered_hotkey_modifiers_ = modifiers;
    registered_hotkey_vk_ = vk_code;
    snippet_sort_column_ = std::clamp(settings_.snippet_sort_column, 0, 4);
    snippet_sort_ascending_ = settings_.snippet_sort_ascending;
    selected_group_index_ = 0;
    if (!settings_.last_group_filter.empty()) {
        for (int index = 0; index < static_cast<int>(groups_.size()); ++index) {
            if (groups_[index].name == settings_.last_group_filter) {
                selected_group_index_ = index + 1;
                break;
            }
        }
    }
    UpdateHotkeyDisplay();
    SetOwnerDrawChecked(always_on_top_checkbox_, settings_.always_on_top);
    SetOwnerDrawChecked(start_with_windows_checkbox_, settings_.start_with_windows);
    SetOwnerDrawChecked(minimize_to_tray_checkbox_, settings_.minimize_to_tray);
    RefreshSettingsControls();
    ApplyTheme();
    ApplyAlwaysOnTopSetting();
    ApplyWindowBoundsFromSettings();
    UpdateTrayIconTip();
}

void AppWindow::RefreshSettingsControls() const {
    SetOwnerDrawChecked(previous_clipboard_checkbox_, settings_.use_previous_clipboard_trigger);
    SetOwnerDrawChecked(previous_clipboard_slash_checkbox_, settings_.use_previous_clipboard_slash_trigger);
    SetOwnerDrawChecked(case_sensitive_checkbox_, settings_.match_case_sensitive);
    SetOwnerDrawChecked(instant_mode_radio_, settings_.trigger_mode != L"separator");
    SetOwnerDrawChecked(separator_mode_radio_, settings_.trigger_mode == L"separator");
    SetOwnerDrawChecked(separator_space_checkbox_, IsSeparatorEnabled(L"space"));
    SetOwnerDrawChecked(separator_enter_checkbox_, IsSeparatorEnabled(L"enter"));
    SetOwnerDrawChecked(separator_tab_checkbox_, IsSeparatorEnabled(L"tab"));
}

void AppWindow::RefreshEditorBanner() const {
    if (editor_title_ != nullptr) {
        SetWindowTextW(editor_title_, L"");
    }
    if (editor_hint_label_ != nullptr) {
        SetWindowTextW(editor_hint_label_, L"");
    }
}

void AppWindow::RefreshEditorTabButtons() const {
    if (editor_snippet_tab_button_ != nullptr) {
        SetWindowTextW(editor_snippet_tab_button_, L"Snippets Manager");
        InvalidateRect(editor_snippet_tab_button_, nullptr, TRUE);
    }
    if (editor_engine_tab_button_ != nullptr) {
        SetWindowTextW(editor_engine_tab_button_, L"Engine Settings");
        InvalidateRect(editor_engine_tab_button_, nullptr, TRUE);
    }
}

void AppWindow::UpdateEditorSectionVisibility() const {
    const bool show_engine = engine_settings_tab_active_;
    const int snippet_visibility = show_engine ? SW_HIDE : SW_SHOWNA;
    const int engine_visibility = show_engine ? SW_SHOWNA : SW_HIDE;

    const HWND snippet_controls[] = {
        trigger_label_,
        trigger_edit_,
        group_label_,
        group_combo_,
        snippet_enabled_checkbox_,
        notes_label_,
        notes_edit_,
        content_label_,
        content_edit_,
        editor_new_button_,
        save_button_,
        reset_button_
    };
    const HWND engine_controls[] = {
        engine_settings_title_,
        restore_label_,
        restore_edit_,
        hotkey_label_,
        record_hotkey_button_,
        always_on_top_checkbox_,
        start_with_windows_checkbox_,
        minimize_to_tray_checkbox_,
        previous_clipboard_checkbox_,
        previous_clipboard_slash_checkbox_,
        case_sensitive_checkbox_,
        trigger_mode_label_,
        instant_mode_radio_,
        separator_mode_radio_,
        separator_keys_label_,
        separator_space_checkbox_,
        separator_enter_checkbox_,
        separator_tab_checkbox_,
        settings_save_button_,
        settings_reset_button_
    };

    for (HWND control : snippet_controls) {
        if (control != nullptr) {
            ShowWindow(control, snippet_visibility);
        }
    }
    for (HWND control : engine_controls) {
        if (control != nullptr) {
            ShowWindow(control, engine_visibility);
        }
    }
    if (editor_settings_scrollbar_ != nullptr) {
        ShowWindow(editor_settings_scrollbar_, SW_HIDE);
    }
}

void AppWindow::SetEditorTab(bool show_engine_settings) {
    if (engine_settings_tab_active_ == show_engine_settings) {
        return;
    }

    engine_settings_tab_active_ = show_engine_settings;
    RefreshEditorTabButtons();
    UpdateEditorSectionVisibility();

    if (hwnd_ != nullptr) {
        if (compact_layout_) {
            window_scroll_offset_ = 0;
        }
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        LayoutChildren(rect.right - rect.left, rect.bottom - rect.top);
        InvalidateRect(hwnd_, &editor_card_rect_, TRUE);
        RedrawWindow(hwnd_, &editor_card_rect_, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
}

void AppWindow::RefreshSnippetActionButtons() const {
    const bool has_selection = PrimarySelectedSnippetIndex() >= 0;
    const bool has_loaded_editor = editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size());
    const bool can_save_editor = has_loaded_editor || editor_dirty_;
    const bool can_reset_editor = has_loaded_editor || editor_dirty_ || editor_new_mode_;

    if (new_snippet_button_ != nullptr) {
        EnableWindow(new_snippet_button_, has_selection ? TRUE : FALSE);
    }
    if (duplicate_button_ != nullptr) {
        EnableWindow(duplicate_button_, has_selection ? TRUE : FALSE);
    }
    if (toggle_snippet_button_ != nullptr) {
        EnableWindow(toggle_snippet_button_, has_selection ? TRUE : FALSE);
    }
    if (delete_button_ != nullptr) {
        EnableWindow(delete_button_, has_selection ? TRUE : FALSE);
    }
    if (snippets_new_button_ != nullptr) {
        EnableWindow(snippets_new_button_, TRUE);
    }
    if (save_button_ != nullptr) {
        EnableWindow(save_button_, can_save_editor ? TRUE : FALSE);
    }
    if (reset_button_ != nullptr) {
        EnableWindow(reset_button_, can_reset_editor ? TRUE : FALSE);
    }
    if (editor_new_button_ != nullptr) {
        EnableWindow(editor_new_button_, TRUE);
    }
}

std::wstring AppWindow::BuildSnippetPreview(const Snippet& snippet) const {
    std::wstring preview = snippet.content;
    for (wchar_t& ch : preview) {
        if (ch == L'\r' || ch == L'\n' || ch == L'\t') {
            ch = L' ';
        }
    }
    preview = TrimCopy(preview);
    if (preview.size() > 90) {
        preview.resize(89);
        preview += L"...";
    }
    return preview;
}

void AppWindow::StartNewSnippetCommand() {
    if (editor_new_mode_ && editing_snippet_index_ < 0) {
        CreateNewSnippet();
        SetStatusText(L"Snippet Manager cleared for a fresh new snippet.");
        return;
    }
    if (!ConfirmDiscardEditorChanges(L"to a new snippet")) {
        return;
    }
    CreateNewSnippet();
    SetStatusText(L"Snippet Manager is ready for a new snippet.");
}

void AppWindow::ResetTestArea() {
    updating_test_area_ = true;
    WriteWindowText(test_edit_, L"");
    updating_test_area_ = false;
    SetFocus(test_edit_);
    SetStatusText(L"Test Area cleared.");
}

bool AppWindow::HandleSelectAllShortcut() {
    HWND focus = GetFocus();
    if (focus == nullptr) {
        return false;
    }

    if (focus == trigger_edit_ || focus == notes_edit_ || focus == content_edit_ ||
        focus == test_edit_ || focus == search_edit_ || focus == restore_edit_) {
        SendMessageW(focus, EM_SETSEL, 0, -1);
        return true;
    }

    if (focus == snippets_list_) {
        suppress_snippet_list_notifications_ = true;
        for (int index = 0; index < static_cast<int>(filtered_snippet_indices_.size()); ++index) {
            ListView_SetItemState(snippets_list_, index, LVIS_SELECTED, LVIS_SELECTED);
        }
        const int current_visible = SelectedSnippetVisibleIndex();
        if (current_visible >= 0) {
            ListView_SetItemState(snippets_list_, current_visible, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(snippets_list_, current_visible, FALSE);
        }
        suppress_snippet_list_notifications_ = false;
        SetStatusText(L"Selected all visible snippets.");
        return true;
    }
    if (focus == groups_list_) {
        SetStatusText(L"The groups list keeps a single active selection.");
        return true;
    }

    return false;
}

void AppWindow::ClampPanelSizes(int client_width, int client_height) {
    const int margin = ScaleMainUi(16);
    const int title_height = ScaleMainUi(48);
    const int button_height = ScaleMainUi(36);
    const int status_height = ScaleMainUi(34);
    const int toolbar_y = margin + title_height;
    const int content_top = toolbar_y + button_height + ScaleMainUi(18);
    const int content_height = std::max(0, client_height - content_top - status_height - margin);
    settings_.left_panel_width = kFixedLeftPanelWidth;
    settings_.right_panel_width = kFixedRightPanelWidth;
    settings_.groups_panel_height = std::max(ScaleMainUi(230), (content_height * 42) / 100);
}

AppWindow::ActiveSplitter AppWindow::HitTestSplitter(POINT client_point) const {
    (void)client_point;
    return ActiveSplitter::None;
}

void AppWindow::ApplyFonts() const {
    const HWND title_controls[] = {title_label_};
    const HWND section_controls[] = {groups_title_, snippets_title_, editor_title_, test_title_};
    const HWND regular_controls[] = {
        subtitle_label_, engine_button_, theme_button_, import_button_, export_button_, github_button_, info_button_, restore_label_, restore_edit_,
        always_on_top_checkbox_, start_with_windows_checkbox_, minimize_to_tray_checkbox_, hotkey_label_, record_hotkey_button_,
        new_group_button_, rename_group_button_, toggle_group_button_, delete_group_button_,
        groups_list_, test_edit_, test_reset_button_, search_edit_, snippets_list_, new_snippet_button_, duplicate_button_, toggle_snippet_button_, delete_button_, snippets_new_button_,
        editor_snippet_tab_button_, editor_engine_tab_button_,
        editor_hint_label_, trigger_label_, trigger_edit_, group_label_, group_combo_, snippet_enabled_checkbox_, notes_label_, notes_edit_,
        content_label_, content_edit_, editor_new_button_, save_button_, reset_button_, engine_settings_title_, case_sensitive_checkbox_,
        trigger_mode_label_, instant_mode_radio_, separator_mode_radio_, separator_keys_label_, separator_space_checkbox_,
        separator_enter_checkbox_, separator_tab_checkbox_, settings_save_button_, settings_reset_button_, status_label_
    };

    for (HWND control : title_controls) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(title_font_), TRUE);
    }
    for (HWND control : section_controls) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(section_font_), TRUE);
    }
    for (HWND control : regular_controls) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(ui_font_), TRUE);
    }
}

void AppWindow::LayoutChildren(int width, int height) {
    const bool was_compact_layout = compact_layout_;
    const int margin = ScaleMainUi(16);
    const int button_height = ScaleMainUi(36);
    const int input_height = ScaleMainUi(32);
    const int label_height = ScaleMainUi(22);
    const int checkbox_height = ScaleMainUi(24);
    const int row_gap = ScaleMainUi(8);
    const int section_gap = ScaleMainUi(14);
    const int status_height = ScaleMainUi(34);
    const int title_height = ScaleMainUi(48);
    const int card_padding = ScaleMainUi(14);
    const int card_title_height = ScaleMainUi(32);
    const int vertical_gap = ScaleMainUi(12);

    if (width <= 0 || height <= 0) {
        return;
    }

    groups_card_rect_ = {};
    test_card_rect_ = {};
    snippets_card_rect_ = {};
    editor_card_rect_ = {};
    engine_settings_card_rect_ = {};

    ClampPanelSizes(width, height);

    const int top_y = margin;
    MoveWindow(title_label_, margin, top_y, std::max(0, width - margin * 2), ScaleMainUi(44), TRUE);
    MoveWindow(subtitle_label_, 0, 0, 0, 0, FALSE);

    const int toolbar_y = top_y + title_height;
    int x = margin;
    MoveWindow(engine_button_, x, toolbar_y, ScaleMainUi(128), button_height, TRUE);
    x += ScaleMainUi(136);
    MoveWindow(theme_button_, x, toolbar_y, ScaleMainUi(128), button_height, TRUE);
    x += ScaleMainUi(136);
    MoveWindow(import_button_, x, toolbar_y, ScaleMainUi(96), button_height, TRUE);
    x += ScaleMainUi(104);
    MoveWindow(export_button_, x, toolbar_y, ScaleMainUi(96), button_height, TRUE);
    const int icon_button_size = button_height;
    const int icon_button_gap = ScaleMainUi(10);
    const int info_button_x = std::max(margin, width - margin - icon_button_size);
    const int github_button_x = std::max(margin, info_button_x - icon_button_gap - icon_button_size);
    MoveWindow(github_button_, github_button_x, toolbar_y, icon_button_size, button_height, TRUE);
    MoveWindow(info_button_, info_button_x, toolbar_y, icon_button_size, button_height, TRUE);

    const int content_top = toolbar_y + button_height + ScaleMainUi(18);
    const int status_y = std::max(content_top + ScaleMainUi(80), height - status_height - ScaleMainUi(8));
    const int viewport_height = std::max(0, status_y - content_top - ScaleMainUi(10));
    const bool compact_layout = false;
    const int available_width = std::max(0, width - margin * 2 - kPanelGap * 2);
    const int fallback_min_left = kMinLeftPanelWidth;
    const int fallback_min_right = kMinRightPanelWidth;
    const int fallback_min_center = kMinCenterPanelWidth;
    int left_width = settings_.left_panel_width;
    int right_width = settings_.right_panel_width;
    int center_width = std::max(0, available_width - left_width - right_width);

    if (center_width < kMinCenterPanelWidth) {
        int deficit = kMinCenterPanelWidth - center_width;
        const int right_shrink = std::min(deficit, std::max(0, right_width - kMinRightPanelWidth));
        right_width -= right_shrink;
        deficit -= right_shrink;

        const int left_shrink = std::min(deficit, std::max(0, left_width - kMinLeftPanelWidth));
        left_width -= left_shrink;
        deficit -= left_shrink;
        center_width = std::max(0, available_width - left_width - right_width);
    }

    if (center_width < fallback_min_center) {
        right_width = std::max(fallback_min_right, std::min(right_width, available_width - fallback_min_left - fallback_min_center));
        left_width = std::max(fallback_min_left, std::min(left_width, available_width - right_width - fallback_min_center));
        center_width = std::max(fallback_min_center, available_width - left_width - right_width);
        if (left_width + right_width + center_width > available_width) {
            center_width = std::max(0, available_width - left_width - right_width);
        }
    }

    if (compact_layout != was_compact_layout) {
        window_scroll_offset_ = 0;
    }
    compact_layout_ = false;
    const int left_x = margin;
    const int center_x = left_x + left_width + kPanelGap;
    const int right_x = center_x + center_width + kPanelGap;
    const int groups_panel_height = std::clamp(settings_.groups_panel_height, ScaleMainUi(254), std::max(ScaleMainUi(254), viewport_height / 2));
    const int test_panel_height = std::max(ScaleMainUi(140), viewport_height - groups_panel_height - vertical_gap);

    left_vertical_splitter_rect_ = {0, 0, 0, 0};
    right_vertical_splitter_rect_ = {0, 0, 0, 0};
    left_horizontal_splitter_rect_ = {0, 0, 0, 0};

    auto layout_left_column = [&](int x_pos, int y_pos, int panel_width, int panel_height) {
        groups_card_rect_ = {x_pos, y_pos, x_pos + panel_width, y_pos + groups_panel_height};
        const int test_y = y_pos + groups_panel_height + vertical_gap;
        const int test_height = panel_height - groups_panel_height - vertical_gap;
        test_card_rect_ = {x_pos, test_y, x_pos + panel_width, test_y + test_height};

        const int groups_inner_x = groups_card_rect_.left + card_padding;
        const int groups_inner_y = groups_card_rect_.top + ScaleMainUi(12);
        const int groups_inner_width = std::max(0, panel_width - card_padding * 2);
        const int group_button_gap = groups_inner_width < ScaleMainUi(180) ? ScaleMainUi(6) : ScaleMainUi(10);
        const int group_button_width = std::max(ScaleMainUi(64), (groups_inner_width - group_button_gap) / 2);
        const int group_buttons_y = groups_inner_y + card_title_height + ScaleMainUi(10);
        const int group_row2_y = group_buttons_y + button_height + ScaleMainUi(10);
        const int groups_list_y = group_row2_y + button_height + ScaleMainUi(14);
        const int groups_list_height = std::max(ScaleMainUi(120), static_cast<int>(groups_card_rect_.bottom - card_padding - groups_list_y));

        MoveWindow(groups_title_, groups_inner_x, groups_inner_y, groups_inner_width, card_title_height, TRUE);
        MoveWindow(new_group_button_, groups_inner_x, group_buttons_y, group_button_width, button_height, TRUE);
        MoveWindow(rename_group_button_, groups_inner_x + group_button_width + group_button_gap, group_buttons_y, group_button_width, button_height, TRUE);
        MoveWindow(toggle_group_button_, groups_inner_x, group_row2_y, group_button_width, button_height, TRUE);
        MoveWindow(delete_group_button_, groups_inner_x + group_button_width + group_button_gap, group_row2_y, group_button_width, button_height, TRUE);
        MoveWindow(groups_list_, groups_inner_x, groups_list_y, groups_inner_width, groups_list_height, TRUE);

        const int test_inner_x = test_card_rect_.left + card_padding;
        const int test_inner_y = test_card_rect_.top + ScaleMainUi(12);
        const int test_inner_width = std::max(0, panel_width - card_padding * 2);
        const int test_button_y = test_card_rect_.bottom - card_padding - button_height;
        const int test_edit_y = test_inner_y + card_title_height + ScaleMainUi(8);
        const int test_edit_height = std::max(ScaleMainUi(140), test_button_y - test_edit_y - ScaleMainUi(14));

        MoveWindow(test_title_, test_inner_x, test_inner_y, test_inner_width, card_title_height, TRUE);
        MoveWindow(test_edit_, test_inner_x, test_edit_y, test_inner_width, test_edit_height, TRUE);
        MoveWindow(test_reset_button_, test_inner_x, test_button_y, test_inner_width, button_height, TRUE);
    };

    auto layout_snippets_panel = [&](int x_pos, int y_pos, int panel_width, int panel_height) {
        snippets_card_rect_ = {x_pos, y_pos, x_pos + panel_width, y_pos + panel_height};

        const int inner_x = snippets_card_rect_.left + card_padding;
        const int inner_y = snippets_card_rect_.top + ScaleMainUi(12);
        const int inner_width = std::max(0, panel_width - card_padding * 2);
        const int search_width = std::clamp(inner_width / 3, ScaleMainUi(120), ScaleMainUi(280));
        const int title_width = std::max(ScaleMainUi(100), inner_width - search_width - ScaleMainUi(16));
        const int list_y = inner_y + card_title_height + ScaleMainUi(8);
        const int actions_y = snippets_card_rect_.bottom - card_padding - button_height;
        const int list_height = std::max(ScaleMainUi(220), actions_y - list_y - ScaleMainUi(12));
        const int action_gap = ScaleMainUi(8);
        const int action_width = std::max(ScaleMainUi(52), (inner_width - action_gap * 4) / 5);

        MoveWindow(snippets_title_, inner_x, inner_y, title_width, card_title_height, TRUE);
        MoveWindow(search_edit_, inner_x + std::max(0, inner_width - search_width), inner_y, search_width, input_height, TRUE);
        MoveWindow(snippets_list_, inner_x, list_y, inner_width, list_height, TRUE);
        UpdateSnippetListColumns(inner_width);
        MoveWindow(new_snippet_button_, inner_x, actions_y, action_width, button_height, TRUE);
        MoveWindow(duplicate_button_, inner_x + action_width + action_gap, actions_y, action_width, button_height, TRUE);
        MoveWindow(toggle_snippet_button_, inner_x + action_width * 2 + action_gap * 2, actions_y, action_width, button_height, TRUE);
        MoveWindow(delete_button_, inner_x + action_width * 3 + action_gap * 3, actions_y, action_width, button_height, TRUE);
        MoveWindow(snippets_new_button_, inner_x + action_width * 4 + action_gap * 4, actions_y, action_width, button_height, TRUE);
    };

    auto layout_editor_panel = [&](int x_pos, int y_pos, int panel_width, int panel_height) {
        editor_card_rect_ = {x_pos, y_pos, x_pos + panel_width, y_pos + panel_height};
        engine_settings_card_rect_ = {};

        const int inner_x = editor_card_rect_.left + card_padding;
        const int inner_y = editor_card_rect_.top + ScaleMainUi(12);
        const int inner_width = std::max(0, panel_width - card_padding * 2);
        const int trio_gap = ScaleMainUi(10);
        const int trio_button_width = std::max(ScaleMainUi(64), (inner_width - trio_gap * 2) / 3);
        const int tab_gap = ScaleMainUi(8);
        const int tab_height = ScaleMainUi(34);
        const int tab_width = std::max(ScaleMainUi(96), (inner_width - tab_gap) / 2);
        const int content_bottom = editor_card_rect_.bottom - card_padding;

        int cursor_y = inner_y;
        MoveWindow(editor_title_, 0, 0, 0, 0, FALSE);
        ShowWindow(editor_title_, SW_HIDE);
        MoveWindow(editor_hint_label_, 0, 0, 0, 0, FALSE);

        MoveWindow(editor_snippet_tab_button_, inner_x, cursor_y, tab_width, tab_height, TRUE);
        MoveWindow(
            editor_engine_tab_button_,
            inner_x + tab_width + tab_gap,
            cursor_y,
            tab_width,
            tab_height,
            TRUE
        );
        cursor_y += tab_height + ScaleMainUi(14);

        if (!engine_settings_tab_active_) {
            const int snippet_action_y = content_bottom - button_height;
            const int combo_closed_height = std::max(ScaleMainUi(36), input_height + ScaleMainUi(4));
            const int combo_drop_panel_height = std::max(ScaleMainUi(220), combo_closed_height + ScaleMainUi(180));

            MoveWindow(trigger_label_, inner_x, cursor_y, inner_width, label_height, TRUE);
            cursor_y += label_height + ScaleMainUi(4);
            MoveWindow(trigger_edit_, inner_x, cursor_y, inner_width, input_height, TRUE);
            cursor_y += input_height + ScaleMainUi(10);

            MoveWindow(group_label_, inner_x, cursor_y, inner_width, label_height, TRUE);
            cursor_y += label_height + ScaleMainUi(4);
            MoveWindow(group_combo_, inner_x, cursor_y, inner_width, combo_drop_panel_height, TRUE);
            UpdateEditorGroupComboDropPanel(false);
            cursor_y += combo_closed_height + ScaleMainUi(8);
            MoveWindow(
                snippet_enabled_checkbox_,
                inner_x,
                cursor_y,
                inner_width,
                checkbox_height,
                TRUE
            );
            cursor_y += checkbox_height + ScaleMainUi(10);

            MoveWindow(notes_label_, inner_x, cursor_y, inner_width, label_height, TRUE);
            cursor_y += label_height + ScaleMainUi(4);
            MoveWindow(notes_edit_, inner_x, cursor_y, inner_width, input_height, TRUE);
            cursor_y += input_height + ScaleMainUi(10);

            MoveWindow(content_label_, inner_x, cursor_y, inner_width, label_height, TRUE);
            const int content_edit_y = cursor_y + label_height + ScaleMainUi(6);
            const int content_edit_height = std::max(ScaleMainUi(140), snippet_action_y - content_edit_y - ScaleMainUi(12));
            MoveWindow(content_edit_, inner_x, content_edit_y, inner_width, content_edit_height, TRUE);

            MoveWindow(editor_new_button_, inner_x, snippet_action_y, trio_button_width, button_height, TRUE);
            MoveWindow(save_button_, inner_x + trio_button_width + trio_gap, snippet_action_y, trio_button_width, button_height, TRUE);
            MoveWindow(reset_button_, inner_x + (trio_button_width + trio_gap) * 2, snippet_action_y, trio_button_width, button_height, TRUE);
        } else {
            const int column_gap = ScaleMainUi(12);
            const int settings_button_gap = ScaleMainUi(10);
            const int settings_button_width = std::max(ScaleMainUi(110), (inner_width - settings_button_gap) / 2);
            const int radio_width = std::max(ScaleMainUi(100), (inner_width - column_gap) / 2);
            const int separator_gap = ScaleMainUi(10);
            const int separator_width = std::max(ScaleMainUi(72), (inner_width - separator_gap * 2) / 3);
            const int footer_y = content_bottom - button_height;

            MoveWindow(engine_settings_title_, inner_x, cursor_y, inner_width, label_height + 2, TRUE);
            cursor_y += label_height + ScaleMainUi(10);

            MoveWindow(restore_label_, inner_x, cursor_y + ScaleMainUi(5), ScaleMainUi(152), label_height, TRUE);
            MoveWindow(restore_edit_, inner_x + ScaleMainUi(160), cursor_y, ScaleMainUi(88), input_height, TRUE);
            cursor_y += input_height + ScaleMainUi(10);

            MoveWindow(hotkey_label_, inner_x, cursor_y, inner_width, ScaleMainUi(22), TRUE);
            cursor_y += ScaleMainUi(28);

            MoveWindow(record_hotkey_button_, inner_x, cursor_y, inner_width, button_height, TRUE);
            cursor_y += button_height + ScaleMainUi(12);

            MoveWindow(always_on_top_checkbox_, inner_x, cursor_y, inner_width, checkbox_height, TRUE);
            cursor_y += checkbox_height + ScaleMainUi(6);
            MoveWindow(start_with_windows_checkbox_, inner_x, cursor_y, inner_width, checkbox_height, TRUE);
            cursor_y += checkbox_height + ScaleMainUi(6);
            MoveWindow(minimize_to_tray_checkbox_, inner_x, cursor_y, inner_width, checkbox_height, TRUE);
            cursor_y += checkbox_height + ScaleMainUi(6);
            MoveWindow(previous_clipboard_checkbox_, inner_x, cursor_y, inner_width, checkbox_height, TRUE);
            cursor_y += checkbox_height + ScaleMainUi(6);
            MoveWindow(previous_clipboard_slash_checkbox_, inner_x, cursor_y, inner_width, checkbox_height, TRUE);
            cursor_y += checkbox_height + ScaleMainUi(6);
            MoveWindow(case_sensitive_checkbox_, inner_x, cursor_y, inner_width, checkbox_height, TRUE);
            cursor_y += checkbox_height + ScaleMainUi(10);

            MoveWindow(trigger_mode_label_, inner_x, cursor_y, inner_width, label_height, TRUE);
            cursor_y += label_height + ScaleMainUi(4);
            MoveWindow(instant_mode_radio_, inner_x, cursor_y, radio_width, checkbox_height, TRUE);
            MoveWindow(
                separator_mode_radio_,
                inner_x + radio_width + column_gap,
                cursor_y,
                std::max(120, inner_width - radio_width - column_gap),
                checkbox_height,
                TRUE
            );
            cursor_y += checkbox_height + ScaleMainUi(10);

            MoveWindow(separator_keys_label_, inner_x, cursor_y, inner_width, label_height, TRUE);
            cursor_y += label_height + ScaleMainUi(4);
            MoveWindow(separator_space_checkbox_, inner_x, cursor_y, separator_width, checkbox_height, TRUE);
            MoveWindow(separator_enter_checkbox_, inner_x + separator_width + separator_gap, cursor_y, separator_width, checkbox_height, TRUE);
            MoveWindow(separator_tab_checkbox_, inner_x + (separator_width + separator_gap) * 2, cursor_y, separator_width, checkbox_height, TRUE);

            MoveWindow(settings_save_button_, inner_x, footer_y, settings_button_width, button_height, TRUE);
            MoveWindow(
                settings_reset_button_,
                inner_x + settings_button_width + settings_button_gap,
                footer_y,
                std::max(140, inner_width - settings_button_width - settings_button_gap),
                button_height,
                TRUE
            );
        }

        UpdateEditorSectionVisibility();
        RefreshEditorTabButtons();
    };

    window_scroll_offset_ = 0;
    SyncWindowScrollBar(viewport_height, viewport_height);
    layout_left_column(left_x, content_top, left_width, viewport_height);
    layout_snippets_panel(center_x, content_top, center_width, viewport_height);
    layout_editor_panel(right_x, content_top, right_width, viewport_height);

    MoveWindow(status_label_, margin, status_y, std::max(0, width - margin * 2), ScaleMainUi(24), TRUE);

    const HWND fixed_top_controls[] = {
        title_label_,
        engine_button_,
        theme_button_,
        import_button_,
        export_button_,
        github_button_,
        info_button_
    };
    for (HWND control : fixed_top_controls) {
        if (control != nullptr) {
            SetWindowPos(
                control,
                HWND_TOP,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
            );
        }
    }
}

void AppWindow::SetStatusText(const wchar_t* text) const {
    SetWindowTextW(status_label_, text);
}

void AppWindow::PostStatusText(std::wstring text) const {
    auto* payload = new std::wstring(std::move(text));
    PostMessageW(hwnd_, kStatusMessage, 0, reinterpret_cast<LPARAM>(payload));
}

bool AppWindow::StartGlobalServices() {
    const bool hotkey_registered = RegisterCurrentHotkey();
    keyboard_hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
    return hotkey_registered && keyboard_hook_ != nullptr;
}

void AppWindow::StopGlobalServices() {
    UnregisterCurrentHotkey();
    if (keyboard_hook_ != nullptr) {
        UnhookWindowsHookEx(keyboard_hook_);
        keyboard_hook_ = nullptr;
    }
}

bool AppWindow::RegisterCurrentHotkey() {
    if (hwnd_ == nullptr || is_capturing_hotkey_) {
        return true;
    }
    return RegisterHotKey(
        hwnd_,
        kGlobalToggleHotkeyId,
        registered_hotkey_modifiers_ | MOD_NOREPEAT,
        registered_hotkey_vk_
    ) != 0;
}

void AppWindow::UnregisterCurrentHotkey() {
    if (hwnd_ != nullptr) {
        UnregisterHotKey(hwnd_, kGlobalToggleHotkeyId);
    }
}

void AppWindow::ApplyAlwaysOnTopSetting() {
    SetWindowPos(
        hwnd_,
        settings_.always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );
}

void AppWindow::UpdateHotkeyDisplay() {
    if (hotkey_label_ != nullptr) {
        SetWindowTextW(hotkey_label_, (L"Hotkey: " + settings_.hotkey).c_str());
    }
    if (record_hotkey_button_ != nullptr) {
        SetWindowTextW(record_hotkey_button_, is_capturing_hotkey_ ? L"Press Keys..." : L"Record Hotkey");
    }
}

void AppWindow::BeginHotkeyCapture() {
    if (is_capturing_hotkey_) {
        return;
    }
    is_capturing_hotkey_ = true;
    UnregisterCurrentHotkey();
    UpdateHotkeyDisplay();
    SetForegroundWindow(hwnd_);
    SetActiveWindow(hwnd_);
    SetFocus(hwnd_);
    SetStatusText(L"Press the new global toggle hotkey. Press Esc to cancel.");
}

void AppWindow::CancelHotkeyCapture(bool restore_registration) {
    if (!is_capturing_hotkey_) {
        return;
    }
    is_capturing_hotkey_ = false;
    UpdateHotkeyDisplay();
    if (restore_registration) {
        RegisterCurrentHotkey();
    }
}

bool AppWindow::FinishHotkeyCapture(UINT vk_code) {
    UINT modifiers = 0;
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) {
        modifiers |= MOD_CONTROL;
    }
    if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
        modifiers |= MOD_SHIFT;
    }
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) {
        modifiers |= MOD_ALT;
    }
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0) {
        modifiers |= MOD_WIN;
    }

    const std::wstring hotkey = BuildHotkeyString(modifiers, vk_code);
    if (hotkey.empty()) {
        CancelHotkeyCapture();
        SetStatusText(L"That key is not supported as a global hotkey.");
        return false;
    }

    is_capturing_hotkey_ = false;
    UpdateHotkeyDisplay();

    if (!ApplyHotkeySetting(hotkey, true)) {
        SetStatusText(L"Could not register the selected hotkey. Try a different one.");
        return false;
    }

    SetStatusText((L"Global toggle hotkey updated to " + hotkey).c_str());
    return true;
}

bool AppWindow::ApplyHotkeySetting(const std::wstring& hotkey, bool save_to_json) {
    UINT new_modifiers = 0;
    UINT new_vk_code = 0;
    if (!ParseHotkeyString(hotkey, new_modifiers, new_vk_code)) {
        return false;
    }

    const std::wstring previous_hotkey = settings_.hotkey;
    const UINT previous_modifiers = registered_hotkey_modifiers_;
    const UINT previous_vk_code = registered_hotkey_vk_;

    settings_.hotkey = hotkey;
    registered_hotkey_modifiers_ = new_modifiers;
    registered_hotkey_vk_ = new_vk_code;
    UpdateHotkeyDisplay();

    if (hwnd_ != nullptr) {
        UnregisterCurrentHotkey();
        if (!RegisterCurrentHotkey()) {
            settings_.hotkey = previous_hotkey;
            registered_hotkey_modifiers_ = previous_modifiers;
            registered_hotkey_vk_ = previous_vk_code;
            UpdateHotkeyDisplay();
            RegisterCurrentHotkey();
            return false;
        }
    }

    if (save_to_json && !SaveData()) {
        SetStatusText(L"Hotkey changed, but JSON save failed.");
    }
    return true;
}

bool AppWindow::UpdateStartWithWindows(bool enabled) {
    HKEY key = nullptr;
    const LONG open_result = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        nullptr,
        0,
        KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr
    );
    if (open_result != ERROR_SUCCESS) {
        return false;
    }

    const std::wstring executable_path = GetExecutablePath();
    const std::wstring command = L"\"" + executable_path + L"\"";
    LONG result = ERROR_SUCCESS;

    if (enabled) {
        result = RegSetValueExW(
            key,
            L"BlinkText",
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t))
        );
    } else {
        result = RegDeleteValueW(key, L"BlinkText");
        if (result == ERROR_FILE_NOT_FOUND) {
            RegDeleteValueW(key, L"BlinkTexts");
            result = ERROR_SUCCESS;
        }
    }

    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool AppWindow::ReadStartWithWindows() const {
    HKEY key = nullptr;
    const LONG open_result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_QUERY_VALUE,
        &key
    );
    if (open_result != ERROR_SUCCESS) {
        return false;
    }

    DWORD type = 0;
    DWORD bytes = 0;
    LONG query_result = RegQueryValueExW(key, L"BlinkText", nullptr, &type, nullptr, &bytes);
    const wchar_t* value_name = L"BlinkText";
    if (query_result == ERROR_FILE_NOT_FOUND) {
        query_result = RegQueryValueExW(key, L"BlinkTexts", nullptr, &type, nullptr, &bytes);
        value_name = L"BlinkTexts";
    }
    if (query_result != ERROR_SUCCESS || type != REG_SZ || bytes == 0) {
        RegCloseKey(key);
        return false;
    }

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    query_result = RegQueryValueExW(
        key,
        value_name,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(value.data()),
        &bytes
    );
    RegCloseKey(key);
    if (query_result != ERROR_SUCCESS) {
        return false;
    }

    if (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }

    return !value.empty();
}

LRESULT CALLBACK AppWindow::LowLevelKeyboardProc(int code, WPARAM wparam, LPARAM lparam) {
    if (code >= 0 && g_app_window != nullptr && lparam != 0) {
        g_app_window->HandleLowLevelKeyboard(wparam, *reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam));
    }
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

bool AppWindow::IsAppForeground() const {
    HWND foreground = GetForegroundWindow();
    if (foreground == nullptr || hwnd_ == nullptr) {
        return false;
    }
    if (foreground == hwnd_) {
        return true;
    }
    const HWND root = GetAncestor(foreground, GA_ROOT);
    return root == hwnd_ || IsChild(hwnd_, foreground);
}

std::wstring AppWindow::TranslateKeyboardEvent(const KBDLLHOOKSTRUCT& info) const {
    BYTE keyboard_state[256] = {};
    if (!GetKeyboardState(keyboard_state)) {
        return L"";
    }

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        keyboard_state[VK_SHIFT] |= 0x80;
    }
    if (GetAsyncKeyState(VK_CAPITAL) & 0x0001) {
        keyboard_state[VK_CAPITAL] |= 0x01;
    }

    keyboard_state[VK_CONTROL] = 0;
    keyboard_state[VK_MENU] = 0;

    HWND foreground = GetForegroundWindow();
    DWORD thread_id = foreground != nullptr ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    HKL layout = GetKeyboardLayout(thread_id);
    wchar_t buffer[8] = {};
    const int rc = ToUnicodeEx(
        info.vkCode,
        info.scanCode,
        keyboard_state,
        buffer,
        static_cast<int>(std::size(buffer)),
        0,
        layout
    );
    if (rc <= 0) {
        return L"";
    }
    return std::wstring(buffer, buffer + rc);
}

void AppWindow::ResetTypedBufferForKey(DWORD vk_code) {
    switch (vk_code) {
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_DELETE:
    case VK_INSERT:
    case VK_ESCAPE:
        typed_buffer_.clear();
        break;
    default:
        break;
    }
}

const AppWindow::Snippet* AppWindow::FindMatchingSnippet(const std::wstring& text, bool exact_match) const {
    if (text.empty()) {
        return nullptr;
    }

    std::vector<const Snippet*> enabled_snippets;
    enabled_snippets.reserve(snippets_.size());
    for (const auto& snippet : snippets_) {
        if (snippet.enabled && !snippet.trigger.empty() && IsGroupEnabled(snippet.group)) {
            enabled_snippets.push_back(&snippet);
        }
    }

    std::sort(enabled_snippets.begin(), enabled_snippets.end(), [](const Snippet* a, const Snippet* b) {
        return a->trigger.size() > b->trigger.size();
    });

    const std::wstring buffer = settings_.match_case_sensitive ? text : ToLowerCopy(text);
    for (const Snippet* snippet : enabled_snippets) {
        const std::wstring trigger = settings_.match_case_sensitive ? snippet->trigger : ToLowerCopy(snippet->trigger);
        if (trigger.empty()) {
            continue;
        }
        if (settings_.use_previous_clipboard_trigger && trigger == kPreviousClipboardTriggerText) {
            continue;
        }
        if (settings_.use_previous_clipboard_slash_trigger && trigger == kPreviousClipboardSlashTriggerText) {
            continue;
        }
        if (exact_match) {
            if (buffer == trigger) {
                return snippet;
            }
            continue;
        }
        if (buffer.size() >= trigger.size() &&
            buffer.compare(buffer.size() - trigger.size(), trigger.size(), trigger) == 0) {
            return snippet;
        }
    }
    return nullptr;
}

LRESULT AppWindow::HandleLowLevelKeyboard(WPARAM wparam, const KBDLLHOOKSTRUCT& info) {
    if (!engine_enabled_ || expansion_in_progress_.load() || IsAppForeground()) {
        return 0;
    }

    if ((info.flags & LLKHF_INJECTED) != 0) {
        return 0;
    }

    if (wparam != WM_KEYDOWN && wparam != WM_SYSKEYDOWN) {
        return 0;
    }

    if (settings_.use_previous_clipboard_trigger || settings_.use_previous_clipboard_slash_trigger) {
        RefreshClipboardHistoryFromSystem();
    }

    switch (info.vkCode) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
    case VK_CAPITAL:
        return 0;
    case VK_BACK:
        if (!typed_buffer_.empty()) {
            typed_buffer_.pop_back();
        }
        return 0;
    default:
        break;
    }

    std::wstring separator_name;
    if (info.vkCode == VK_SPACE) {
        separator_name = L"space";
    } else if (info.vkCode == VK_TAB) {
        separator_name = L"tab";
    } else if (info.vkCode == VK_RETURN) {
        separator_name = L"enter";
    }
    if (!separator_name.empty()) {
        const std::wstring current_word = typed_buffer_;
        typed_buffer_.clear();
        if (settings_.trigger_mode == L"separator" && IsSeparatorEnabled(separator_name)) {
            if ((settings_.use_previous_clipboard_trigger && current_word == kPreviousClipboardTriggerText) ||
                (settings_.use_previous_clipboard_slash_trigger && current_word == kPreviousClipboardSlashTriggerText)) {
                ExpandPreviousClipboardEntryExternally(static_cast<int>(current_word.size()) + 1);
                return 0;
            }
            if (const Snippet* snippet = FindMatchingSnippet(current_word, true)) {
                ExpandSnippetExternally(*snippet, static_cast<int>(snippet->trigger.size()) + 1);
            }
        }
        return 0;
    }

    std::wstring translated = TranslateKeyboardEvent(info);
    if (!translated.empty()) {
        typed_buffer_ += translated;
        if (typed_buffer_.size() > 400) {
            typed_buffer_.erase(0, typed_buffer_.size() - 400);
        }
        if ((settings_.use_previous_clipboard_trigger &&
             typed_buffer_.size() >= std::wcslen(kPreviousClipboardTriggerText) &&
             typed_buffer_.compare(
                 typed_buffer_.size() - std::wcslen(kPreviousClipboardTriggerText),
                 std::wcslen(kPreviousClipboardTriggerText),
                 kPreviousClipboardTriggerText
             ) == 0) ||
            (settings_.use_previous_clipboard_slash_trigger &&
             typed_buffer_.size() >= std::wcslen(kPreviousClipboardSlashTriggerText) &&
             typed_buffer_.compare(
                 typed_buffer_.size() - std::wcslen(kPreviousClipboardSlashTriggerText),
                 std::wcslen(kPreviousClipboardSlashTriggerText),
                 kPreviousClipboardSlashTriggerText
             ) == 0)) {
            typed_buffer_.clear();
            ExpandPreviousClipboardEntryExternally(2);
            return 0;
        }
        if (settings_.trigger_mode == L"instant") {
            if (const Snippet* snippet = FindMatchingSnippet(typed_buffer_)) {
                typed_buffer_.clear();
                ExpandSnippetExternally(*snippet);
            }
        }
        return 0;
    }

    ResetTypedBufferForKey(info.vkCode);
    return 0;
}

void AppWindow::RefreshGroups() {
    const std::wstring editor_group = group_combo_ != nullptr ? CurrentEditorGroupSelection() : L"";
    SendMessageW(groups_list_, LB_RESETCONTENT, 0, 0);
    const std::wstring all_label = L"All Snippets (" + std::to_wstring(snippets_.size()) + L")";
    SendMessageW(groups_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(all_label.c_str()));
    if (groups_title_ != nullptr) {
        SetWindowTextW(groups_title_, (L"Groups (" + std::to_wstring(groups_.size()) + L")").c_str());
    }

    for (const auto& group : groups_) {
        int count = 0;
        for (const auto& snippet : snippets_) {
            if (snippet.group == group.name) {
                ++count;
            }
        }
        std::wstring label = group.name + L" (" + std::to_wstring(count) + L")";
        if (!group.enabled) {
            label += L" [off]";
        }
        SendMessageW(groups_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    const int max_index = static_cast<int>(groups_.size());
    if (selected_group_index_ < 0 || selected_group_index_ > max_index) {
        selected_group_index_ = 0;
    }
    SendMessageW(groups_list_, LB_SETCURSEL, selected_group_index_, 0);
    RefreshEditorGroupCombo(editor_group);
}

void AppWindow::RefreshEditorGroupCombo(const std::wstring& preferred_group) {
    if (group_combo_ == nullptr) {
        return;
    }

    const bool previous_suppression = suppress_editor_change_tracking_;
    suppress_editor_change_tracking_ = true;

    std::wstring group_to_select = TrimCopy(preferred_group);
    if (group_to_select.empty()) {
        group_to_select = TrimCopy(ReadWindowText(group_combo_));
    }
    if (group_to_select.empty()) {
        group_to_select = CurrentGroupFilter();
    }
    if (group_to_select.empty()) {
        group_to_select = groups_.empty() ? L"General" : groups_.front().name;
    }

    SendMessageW(group_combo_, CB_RESETCONTENT, 0, 0);
    for (const auto& group : groups_) {
        SendMessageW(group_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(group.name.c_str()));
    }

    int selected_index = CB_ERR;
    for (int index = 0; index < static_cast<int>(groups_.size()); ++index) {
        if (groups_[index].name == group_to_select) {
            selected_index = index;
            break;
        }
    }

    if (selected_index == CB_ERR && !groups_.empty()) {
        selected_index = 0;
        group_to_select = groups_.front().name;
    }

    if (selected_index != CB_ERR) {
        SendMessageW(group_combo_, CB_SETCURSEL, selected_index, 0);
    }
    SetWindowTextW(group_combo_, group_to_select.c_str());
    suppress_editor_change_tracking_ = previous_suppression;
}

void AppWindow::UpdateEditorGroupComboDropPanel(bool drop_down_open) {
    if (group_combo_ == nullptr) {
        return;
    }

    if (drop_down_open) {
        SetWindowRgn(group_combo_, nullptr, TRUE);
        SetWindowPos(group_combo_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        return;
    }

    RECT combo_rect{};
    GetClientRect(group_combo_, &combo_rect);
    const int closed_height = std::max(ScaleMainUi(36), ScaleMainUi(34));
    const int clipped_height = std::max(1, std::min(static_cast<int>(combo_rect.bottom), closed_height));
    const int clipped_width = std::max(1, static_cast<int>(combo_rect.right));
    HRGN visible_region = CreateRectRgn(0, 0, clipped_width, clipped_height);
    SetWindowRgn(group_combo_, visible_region, TRUE);

    if (snippet_enabled_checkbox_ != nullptr) {
        SetWindowPos(
            group_combo_,
            snippet_enabled_checkbox_,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
        );
    }
}

void AppWindow::RefreshClipboardHistoryFromSystem() {
    if (suppress_clipboard_history_updates_.load()) {
        return;
    }

    const std::wstring clipboard_text = ReadUnicodeClipboardText();
    if (clipboard_text.empty() || clipboard_text == latest_clipboard_text_) {
        return;
    }

    if (!latest_clipboard_text_.empty()) {
        previous_clipboard_text_ = latest_clipboard_text_;
    }
    latest_clipboard_text_ = clipboard_text;
}

std::wstring AppWindow::CurrentGroupFilter() const {
    if (selected_group_index_ <= 0 || selected_group_index_ > static_cast<int>(groups_.size())) {
        return L"";
    }
    return groups_[selected_group_index_ - 1].name;
}

int AppWindow::CurrentGroupVectorIndex() const {
    if (selected_group_index_ <= 0 || selected_group_index_ > static_cast<int>(groups_.size())) {
        return -1;
    }
    return selected_group_index_ - 1;
}

std::wstring AppWindow::CurrentEditorGroupSelection() const {
    if (group_combo_ == nullptr) {
        return CurrentGroupFilter();
    }

    std::wstring group_name = TrimCopy(ReadWindowText(group_combo_));
    if (group_name.empty()) {
        group_name = CurrentGroupFilter();
    }
    if (group_name.empty()) {
        group_name = groups_.empty() ? L"General" : groups_.front().name;
    }
    return group_name;
}

std::wstring AppWindow::ToLowerCopy(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return value;
}

bool AppWindow::SnippetMatchesCurrentFilter(const Snippet& snippet) const {
    const std::wstring group_filter = CurrentGroupFilter();
    if (!group_filter.empty() && snippet.group != group_filter) {
        return false;
    }

    const std::wstring query = ToLowerCopy(CurrentSearchQuery());
    if (query.empty()) {
        return true;
    }

    const std::wstring fields = ToLowerCopy(
        snippet.trigger + L"\n" + snippet.notes + L"\n" + snippet.content
    );
    return fields.find(query) != std::wstring::npos;
}

void AppWindow::RefreshSnippetList(bool keep_selection) {
    const int previous_selection = selected_snippet_index_;
    const std::wstring current_group = CurrentGroupFilter();
    int scoped_total_count = 0;
    for (const auto& snippet : snippets_) {
        if (current_group.empty() || snippet.group == current_group) {
            ++scoped_total_count;
        }
    }
    filtered_snippet_indices_.clear();
    ListView_DeleteAllItems(snippets_list_);

    for (int index = 0; index < static_cast<int>(snippets_.size()); ++index) {
        const auto& snippet = snippets_[index];
        if (!SnippetMatchesCurrentFilter(snippet)) {
            continue;
        }
        filtered_snippet_indices_.push_back(index);
    }

    auto compare_text = [this](const std::wstring& left, const std::wstring& right) {
        const std::wstring normalized_left = ToLowerCopy(left);
        const std::wstring normalized_right = ToLowerCopy(right);
        if (normalized_left < normalized_right) {
            return -1;
        }
        if (normalized_left > normalized_right) {
            return 1;
        }
        return 0;
    };
    std::stable_sort(filtered_snippet_indices_.begin(), filtered_snippet_indices_.end(), [this, &compare_text](int left_index, int right_index) {
        const Snippet& left = snippets_[left_index];
        const Snippet& right = snippets_[right_index];

        int result = 0;
        switch (snippet_sort_column_) {
        case 1:
            result = compare_text(left.group, right.group);
            break;
        case 2: {
            const std::wstring left_on = left.enabled && IsGroupEnabled(left.group) ? L"Yes" : L"No";
            const std::wstring right_on = right.enabled && IsGroupEnabled(right.group) ? L"Yes" : L"No";
            result = compare_text(left_on, right_on);
            break;
        }
        case 3:
            result = compare_text(left.notes, right.notes);
            break;
        case 4:
            result = compare_text(BuildSnippetPreview(left), BuildSnippetPreview(right));
            break;
        case 0:
        default:
            result = compare_text(left.trigger, right.trigger);
            break;
        }

        if (result == 0) {
            result = compare_text(left.trigger, right.trigger);
        }
        if (result == 0) {
            result = left_index < right_index ? -1 : (left_index > right_index ? 1 : 0);
        }
        return snippet_sort_ascending_ ? (result < 0) : (result > 0);
    });

    for (int visible_index = 0; visible_index < static_cast<int>(filtered_snippet_indices_.size()); ++visible_index) {
        const Snippet& snippet = snippets_[filtered_snippet_indices_[visible_index]];
        const std::wstring on_text = snippet.enabled && IsGroupEnabled(snippet.group) ? L"Yes" : L"No";

        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = visible_index;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(snippet.trigger.c_str());
        item.lParam = filtered_snippet_indices_[visible_index];
        const int row_index = ListView_InsertItem(snippets_list_, &item);

        ListView_SetItemText(snippets_list_, row_index, 1, const_cast<LPWSTR>(snippet.group.c_str()));
        ListView_SetItemText(snippets_list_, row_index, 2, const_cast<LPWSTR>(on_text.c_str()));
        ListView_SetItemText(snippets_list_, row_index, 3, const_cast<LPWSTR>(snippet.notes.c_str()));
        std::wstring preview_text = BuildSnippetPreview(snippet);
        ListView_SetItemText(snippets_list_, row_index, 4, const_cast<LPWSTR>(preview_text.c_str()));
    }
    UpdateSnippetSortIndicators();
    UpdateSnippetListColumns(-1);
    if (snippets_title_ != nullptr) {
        std::wstring title = L"Snippets";
        if (!current_group.empty()) {
            title += L" - " + current_group;
        }
        title += L" (";
        title += std::to_wstring(filtered_snippet_indices_.size());
        if (filtered_snippet_indices_.size() != static_cast<size_t>(scoped_total_count)) {
            title += L" shown / " + std::to_wstring(scoped_total_count);
        }
        title += L")";
        SetWindowTextW(snippets_title_, title.c_str());
    }

    if (filtered_snippet_indices_.empty()) {
        SetSnippetListSelection(-1);
        if (!keep_selection) {
            selected_snippet_index_ = -1;
        }
        RefreshEditorBanner();
        RefreshSnippetActionButtons();
        return;
    }

    if (keep_selection) {
        for (int visible_index = 0; visible_index < static_cast<int>(filtered_snippet_indices_.size()); ++visible_index) {
            if (filtered_snippet_indices_[visible_index] == previous_selection) {
                SetSnippetListSelection(visible_index);
                if (!editor_dirty_) {
                    SelectSnippetByActualIndex(previous_selection);
                }
                return;
            }
        }
        if (editor_dirty_) {
            SetSnippetListSelection(-1);
            RefreshEditorBanner();
            RefreshSnippetActionButtons();
            return;
        }
    }

    SelectSnippetByActualIndex(filtered_snippet_indices_.front());
}

void AppWindow::UpdateSnippetListColumns(int total_width) const {
    if (snippets_list_ == nullptr) {
        return;
    }

    RECT rect{};
    GetClientRect(snippets_list_, &rect);
    int available_width = total_width > 0 ? total_width : (rect.right - rect.left);
    const int scrollbar_width = GetSystemMetrics(SM_CXVSCROLL);
    available_width = std::max(420, available_width - scrollbar_width - 4);

    const int on_width = 62;
    const int trigger_width = std::max(120, available_width / 8);
    const int group_width = std::max(120, available_width / 8);
    const int notes_width = std::max(150, available_width / 6);
    const int preview_width = std::max(220, available_width - trigger_width - group_width - on_width - notes_width);

    ListView_SetColumnWidth(snippets_list_, 0, trigger_width);
    ListView_SetColumnWidth(snippets_list_, 1, group_width);
    ListView_SetColumnWidth(snippets_list_, 2, on_width);
    ListView_SetColumnWidth(snippets_list_, 3, notes_width);
    ListView_SetColumnWidth(snippets_list_, 4, preview_width);
}

void AppWindow::UpdateSnippetSortIndicators() const {
    if (snippets_list_ == nullptr) {
        return;
    }

    const HWND header = ListView_GetHeader(snippets_list_);
    if (header == nullptr) {
        return;
    }

    const int column_count = Header_GetItemCount(header);
    for (int index = 0; index < column_count; ++index) {
        HDITEMW item{};
        item.mask = HDI_FORMAT;
        if (!Header_GetItem(header, index, &item)) {
            continue;
        }
        item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (index == snippet_sort_column_) {
            item.fmt |= snippet_sort_ascending_ ? HDF_SORTUP : HDF_SORTDOWN;
        }
        Header_SetItem(header, index, &item);
    }
    InvalidateRect(header, nullptr, TRUE);
    RedrawWindow(header, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

int AppWindow::SelectedSnippetVisibleIndex() const {
    for (int index = 0; index < static_cast<int>(filtered_snippet_indices_.size()); ++index) {
        if (filtered_snippet_indices_[index] == selected_snippet_index_) {
            return index;
        }
    }
    return -1;
}

std::vector<int> AppWindow::SelectedSnippetIndices() const {
    std::vector<int> selected_indices;
    if (snippets_list_ == nullptr) {
        if (selected_snippet_index_ >= 0 && selected_snippet_index_ < static_cast<int>(snippets_.size())) {
            selected_indices.push_back(selected_snippet_index_);
        }
        return selected_indices;
    }

    int visible_index = -1;
    while ((visible_index = ListView_GetNextItem(snippets_list_, visible_index, LVNI_SELECTED)) != -1) {
        if (visible_index >= 0 && visible_index < static_cast<int>(filtered_snippet_indices_.size())) {
            selected_indices.push_back(filtered_snippet_indices_[visible_index]);
        }
    }

    if (selected_indices.empty() && selected_snippet_index_ >= 0 && selected_snippet_index_ < static_cast<int>(snippets_.size())) {
        selected_indices.push_back(selected_snippet_index_);
    }
    return selected_indices;
}

int AppWindow::PrimarySelectedSnippetIndex() const {
    if (snippets_list_ != nullptr) {
        const int focused_visible = ListView_GetNextItem(snippets_list_, -1, LVNI_FOCUSED);
        if (focused_visible >= 0 && focused_visible < static_cast<int>(filtered_snippet_indices_.size())) {
            if ((ListView_GetItemState(snippets_list_, focused_visible, LVIS_SELECTED) & LVIS_SELECTED) != 0) {
                return filtered_snippet_indices_[focused_visible];
            }
        }
    }

    const std::vector<int> selected_indices = SelectedSnippetIndices();
    if (!selected_indices.empty()) {
        return selected_indices.front();
    }
    return -1;
}

void AppWindow::SetSnippetListSelection(int visible_index) {
    if (snippets_list_ == nullptr) {
        return;
    }

    suppress_snippet_list_notifications_ = true;
    ListView_SetItemState(snippets_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (visible_index >= 0 && visible_index < static_cast<int>(filtered_snippet_indices_.size())) {
        ListView_SetItemState(snippets_list_, visible_index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(snippets_list_, visible_index, FALSE);
    }
    suppress_snippet_list_notifications_ = false;
}

void AppWindow::SelectSnippetByActualIndex(int actual_index) {
    if (actual_index < 0 || actual_index >= static_cast<int>(snippets_.size())) {
        selected_snippet_index_ = -1;
        SetSnippetListSelection(-1);
        RefreshEditorBanner();
        RefreshSnippetActionButtons();
        return;
    }
    selected_snippet_index_ = actual_index;
    SetSnippetListSelection(SelectedSnippetVisibleIndex());
    RefreshEditorBanner();
    RefreshSnippetActionButtons();
}

void AppWindow::EditSelectedSnippet() {
    const int active_index = PrimarySelectedSnippetIndex();
    if (active_index < 0 || active_index >= static_cast<int>(snippets_.size())) {
        SetStatusText(L"Select a snippet first, then press Edit.");
        return;
    }
    if (editing_snippet_index_ == active_index) {
        SetEditorTab(false);
        SetFocus(trigger_edit_);
        SetStatusText(L"That snippet is already loaded in Snippet Manager.");
        return;
    }
    if (!ConfirmDiscardEditorChanges(L"to edit a different snippet")) {
        return;
    }

    selected_snippet_index_ = active_index;
    editor_new_mode_ = false;
    editing_snippet_index_ = selected_snippet_index_;
    SetEditorTab(false);
    PopulateEditorFromSelection();
    SetFocus(trigger_edit_);
    SetStatusText((L"Loaded '" + snippets_[editing_snippet_index_].trigger + L"' into Snippet Manager.").c_str());
}

void AppWindow::PopulateEditorFromSelection() {
    if (editing_snippet_index_ < 0 || editing_snippet_index_ >= static_cast<int>(snippets_.size())) {
        CreateNewSnippet();
        return;
    }
    suppress_editor_change_tracking_ = true;
    const auto& snippet = snippets_[editing_snippet_index_];
    WriteWindowText(trigger_edit_, snippet.trigger);
    RefreshEditorGroupCombo(snippet.group);
    SetOwnerDrawChecked(snippet_enabled_checkbox_, snippet.enabled);
    WriteWindowText(notes_edit_, snippet.notes);
    WriteWindowText(content_edit_, snippet.content);
    suppress_editor_change_tracking_ = false;
    MarkEditorClean();
}

void AppWindow::ResetEditor() {
    if (!ConfirmDiscardEditorChanges(L"with reset")) {
        return;
    }
    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        PopulateEditorFromSelection();
        SetStatusText(L"Snippet Manager reset to the loaded snippet.");
        return;
    }
    CreateNewSnippet();
    SetStatusText(L"Snippet Manager cleared for a new snippet.");
}

void AppWindow::CreateNewSnippet() {
    SetEditorTab(false);
    editor_new_mode_ = true;
    editing_snippet_index_ = -1;
    suppress_editor_change_tracking_ = true;
    WriteWindowText(trigger_edit_, L"");
    std::wstring preferred_group = CurrentGroupFilter();
    if (preferred_group.empty() && selected_snippet_index_ >= 0 && selected_snippet_index_ < static_cast<int>(snippets_.size())) {
        preferred_group = snippets_[selected_snippet_index_].group;
    }
    RefreshEditorGroupCombo(preferred_group);
    SetOwnerDrawChecked(snippet_enabled_checkbox_, true);
    WriteWindowText(notes_edit_, L"");
    WriteWindowText(content_edit_, L"");
    suppress_editor_change_tracking_ = false;
    MarkEditorClean();
    SetFocus(trigger_edit_);
}

void AppWindow::CreateGroup() {
    TextEntryDialogConfig dialog_config;
    dialog_config.title = L"New Group";
    dialog_config.prompt = L"Enter the new group name";
    dialog_config.ok_button_label = L"Create";
    dialog_config.dark_theme = IsDarkTheme();

    TextEntryDialogResult result;
    if (!ShowTextEntryDialogWindow(hwnd_, instance_, dialog_config, result)) {
        return;
    }

    const std::wstring group_name = TrimCopy(result.text);
    if (group_name.empty()) {
        SetStatusText(L"Group name is required.");
        return;
    }
    if (GroupNameExists(group_name)) {
        SetStatusText(L"A group with this name already exists.");
        return;
    }
    if (!ConfirmDiscardEditorChanges(L"while creating a group")) {
        return;
    }

    groups_.push_back({group_name, true});
    selected_group_index_ = static_cast<int>(groups_.size());
    RefreshGroups();
    RefreshSnippetList(false);
    CreateNewSnippet();

    if (SaveData()) {
        SetStatusText((L"Created group: " + group_name).c_str());
    } else {
        SetStatusText(L"Group created in memory, but JSON save failed.");
    }
}

void AppWindow::RenameSelectedGroup() {
    const int group_index = CurrentGroupVectorIndex();
    if (group_index < 0) {
        SetStatusText(L"Select a group first.");
        return;
    }

    const std::wstring old_name = groups_[group_index].name;
    TextEntryDialogConfig dialog_config;
    dialog_config.title = L"Rename Group";
    dialog_config.prompt = L"Enter the new group name";
    dialog_config.initial_value = old_name;
    dialog_config.ok_button_label = L"Rename";
    dialog_config.dark_theme = IsDarkTheme();

    TextEntryDialogResult result;
    if (!ShowTextEntryDialogWindow(hwnd_, instance_, dialog_config, result)) {
        return;
    }

    const std::wstring new_name = TrimCopy(result.text);
    if (new_name.empty()) {
        SetStatusText(L"Group name is required.");
        return;
    }
    if (new_name == old_name) {
        SetStatusText(L"Group name is unchanged.");
        return;
    }
    if (GroupNameExists(new_name, group_index)) {
        SetStatusText(L"A group with this name already exists.");
        return;
    }
    if (!ConfirmDiscardEditorChanges(L"while renaming a group")) {
        return;
    }

    groups_[group_index].name = new_name;
    for (auto& snippet : snippets_) {
        if (snippet.group == old_name) {
            snippet.group = new_name;
        }
    }

    RefreshGroups();
    RefreshSnippetList(false);
    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        PopulateEditorFromSelection();
    } else {
        RefreshEditorGroupCombo(new_name);
        RefreshEditorBanner();
    }

    if (SaveData()) {
        SetStatusText((L"Renamed group to: " + new_name).c_str());
    } else {
        SetStatusText(L"Group renamed in memory, but JSON save failed.");
    }
}

void AppWindow::ToggleSelectedGroup() {
    const int group_index = CurrentGroupVectorIndex();
    if (group_index < 0) {
        SetStatusText(L"Select a group first.");
        return;
    }
    if (!ConfirmDiscardEditorChanges(L"while toggling a group")) {
        return;
    }

    groups_[group_index].enabled = !groups_[group_index].enabled;
    RefreshGroups();
    RefreshSnippetList(true);
    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        PopulateEditorFromSelection();
    } else {
        RefreshEditorBanner();
    }

    const bool enabled = groups_[group_index].enabled;
    const std::wstring status = (enabled ? L"Enabled group: " : L"Disabled group: ") + groups_[group_index].name;
    if (SaveData()) {
        SetStatusText(status.c_str());
    } else {
        SetStatusText(L"Group toggle applied in memory, but JSON save failed.");
    }
}

void AppWindow::DeleteSelectedGroup() {
    const int group_index = CurrentGroupVectorIndex();
    if (group_index < 0) {
        SetStatusText(L"Select a group first.");
        return;
    }
    if (groups_.size() <= 1) {
        SetStatusText(L"You need at least one group.");
        return;
    }
    if (!ConfirmDiscardEditorChanges(L"while deleting a group")) {
        return;
    }

    const std::wstring group_name = groups_[group_index].name;
    const std::wstring previous_filter = CurrentGroupFilter();
    int moved_count = 0;
    for (const auto& snippet : snippets_) {
        if (snippet.group == group_name) {
            ++moved_count;
        }
    }

    std::vector<std::wstring> available_target_groups;
    std::wstring fallback_group;
    for (int index = 0; index < static_cast<int>(groups_.size()); ++index) {
        if (index != group_index) {
            available_target_groups.push_back(groups_[index].name);
            if (fallback_group.empty()) {
                fallback_group = groups_[index].name;
            }
        }
    }

    DeleteGroupDialogConfig dialog_config;
    dialog_config.group_name = group_name;
    dialog_config.moved_snippet_count = moved_count;
    dialog_config.target_groups = available_target_groups;
    dialog_config.default_target_group = fallback_group;
    dialog_config.dark_theme = IsDarkTheme();

    DeleteGroupDialogResult dialog_result;
    if (!ShowDeleteGroupDialogWindow(hwnd_, instance_, dialog_config, dialog_result)) {
        SetStatusText(L"Group delete cancelled.");
        return;
    }

    const bool delete_all_snippets = dialog_result.delete_all_snippets;
    const std::wstring chosen_target_group = delete_all_snippets
        ? L""
        : (dialog_result.target_group.empty() ? fallback_group : dialog_result.target_group);

    if (!delete_all_snippets && !chosen_target_group.empty()) {
        for (auto& snippet : snippets_) {
            if (snippet.group == group_name) {
                snippet.group = chosen_target_group;
            }
        }
    } else if (delete_all_snippets) {
        int next_editing_index = editing_snippet_index_;
        int next_selected_index = selected_snippet_index_;
        bool removed_editing_snippet = false;
        bool removed_selected_snippet = false;

        for (int index = static_cast<int>(snippets_.size()) - 1; index >= 0; --index) {
            if (snippets_[index].group != group_name) {
                continue;
            }
            if (editing_snippet_index_ == index) {
                removed_editing_snippet = true;
            } else if (editing_snippet_index_ > index) {
                --next_editing_index;
            }
            if (selected_snippet_index_ == index) {
                removed_selected_snippet = true;
            } else if (selected_snippet_index_ > index) {
                --next_selected_index;
            }
            snippets_.erase(snippets_.begin() + index);
        }

        editing_snippet_index_ = removed_editing_snippet ? -1 : next_editing_index;
        selected_snippet_index_ = removed_selected_snippet ? -1 : next_selected_index;
    }

    groups_.erase(groups_.begin() + group_index);
    selected_group_index_ = 0;
    if (!previous_filter.empty() && previous_filter != group_name) {
        for (int index = 0; index < static_cast<int>(groups_.size()); ++index) {
            if (groups_[index].name == previous_filter) {
                selected_group_index_ = index + 1;
                break;
            }
        }
    } else if (!delete_all_snippets && !chosen_target_group.empty()) {
        for (int index = 0; index < static_cast<int>(groups_.size()); ++index) {
            if (groups_[index].name == chosen_target_group) {
                selected_group_index_ = index + 1;
                break;
            }
        }
    }

    RefreshGroups();
    RefreshSnippetList(true);
    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        PopulateEditorFromSelection();
    } else {
        CreateNewSnippet();
    }

    const std::wstring status = moved_count > 0
        ? (delete_all_snippets
            ? L"Deleted group: " + group_name + L" | deleted snippets: " + std::to_wstring(moved_count)
            : L"Deleted group: " + group_name + L" | moved to: " + chosen_target_group + L" | snippets: " + std::to_wstring(moved_count))
        : L"Deleted group: " + group_name;
    if (SaveData()) {
        SetStatusText(status.c_str());
    } else {
        SetStatusText(L"Group deleted in memory, but JSON save failed.");
    }
}

bool AppWindow::SaveCurrentSnippet() {
    const std::wstring trigger = ReadWindowText(trigger_edit_);
    const std::wstring notes = ReadWindowText(notes_edit_);
    const std::wstring content = ReadWindowText(content_edit_);
    const bool snippet_enabled = IsOwnerDrawChecked(snippet_enabled_checkbox_);
    if (trigger.empty()) {
        SetStatusText(L"Trigger is required.");
        return false;
    }
    if (content.empty()) {
        SetStatusText(L"Content is required.");
        return false;
    }

    std::wstring group = CurrentEditorGroupSelection();
    if (group.empty()) {
        group = groups_.empty() ? L"General" : groups_.front().name;
    }
    EnsureGroupExists(group);

    for (int index = 0; index < static_cast<int>(snippets_.size()); ++index) {
        if (index == editing_snippet_index_) {
            continue;
        }
        if (TriggersEqual(snippets_[index].trigger, trigger)) {
            SetStatusText(L"This trigger already exists.");
            return false;
        }
    }

    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        auto& snippet = snippets_[editing_snippet_index_];
        snippet.trigger = trigger;
        snippet.notes = notes;
        snippet.content = content;
        snippet.group = group;
        snippet.enabled = snippet_enabled;
    } else {
        snippets_.push_back({trigger, group, notes, content, snippet_enabled});
        editing_snippet_index_ = static_cast<int>(snippets_.size()) - 1;
    }
    editor_new_mode_ = false;
    selected_snippet_index_ = editing_snippet_index_;

    RefreshGroups();
    RefreshSnippetList(true);
    const bool saved = SaveData();
    if (saved) {
        MarkEditorClean();
        SetStatusText(L"Snippet saved to JSON.");
    } else {
        MarkEditorDirty();
        SetStatusText(L"Snippet updated in memory, but JSON save failed.");
    }
    return saved;
}

void AppWindow::SaveEngineSettings() {
    const std::wstring restore_text = TrimCopy(ReadWindowText(restore_edit_));
    int restore_ms = settings_.restore_clipboard_delay_ms;
    if (!restore_text.empty()) {
        try {
            restore_ms = std::max(0, std::stoi(restore_text));
        } catch (...) {
            SetStatusText(L"Restore must be a valid number.");
            return;
        }
    }

    std::vector<std::wstring> separators;
    if (IsOwnerDrawChecked(separator_space_checkbox_)) {
        separators.push_back(L"space");
    }
    if (IsOwnerDrawChecked(separator_enter_checkbox_)) {
        separators.push_back(L"enter");
    }
    if (IsOwnerDrawChecked(separator_tab_checkbox_)) {
        separators.push_back(L"tab");
    }
    if (separators.empty()) {
        SetStatusText(L"Enable at least one separator key.");
        return;
    }

    settings_.restore_clipboard_delay_ms = restore_ms;
    settings_.match_case_sensitive = IsOwnerDrawChecked(case_sensitive_checkbox_);
    settings_.trigger_mode = IsOwnerDrawChecked(separator_mode_radio_) ? L"separator" : L"instant";
    settings_.use_previous_clipboard_trigger = IsOwnerDrawChecked(previous_clipboard_checkbox_);
    settings_.use_previous_clipboard_slash_trigger = IsOwnerDrawChecked(previous_clipboard_slash_checkbox_);
    settings_.word_separators = std::move(separators);
    typed_buffer_.clear();
    RefreshSettingsControls();
    WriteWindowText(restore_edit_, std::to_wstring(settings_.restore_clipboard_delay_ms));

    if (SaveData()) {
        SetStatusText(L"Engine settings saved.");
    } else {
        SetStatusText(L"Engine settings updated in memory, but JSON save failed.");
    }
}

void AppWindow::ResetEngineSettings() {
    WriteWindowText(restore_edit_, std::to_wstring(std::max(0, settings_.restore_clipboard_delay_ms)));
    RefreshSettingsControls();
    typed_buffer_.clear();
    SetStatusText(L"Engine settings reset to the saved values.");
}

void AppWindow::DuplicateSelectedSnippet() {
    if (!ConfirmDiscardEditorChanges(L"while duplicating a snippet")) {
        return;
    }
    const int active_index = PrimarySelectedSnippetIndex();
    if (active_index < 0 || active_index >= static_cast<int>(snippets_.size())) {
        SetStatusText(L"Select a snippet first.");
        return;
    }

    selected_snippet_index_ = active_index;
    Snippet copy = snippets_[selected_snippet_index_];
    std::wstring base = copy.trigger.empty() ? L"copy" : copy.trigger;
    std::wstring candidate = base + L"_copy";
    int counter = 2;

    auto exists = [this](const std::wstring& trigger) {
        return std::any_of(snippets_.begin(), snippets_.end(), [this, &trigger](const Snippet& snippet) {
            return TriggersEqual(snippet.trigger, trigger);
        });
    };

    while (exists(candidate)) {
        candidate = base + L"_copy" + std::to_wstring(counter++);
    }

    copy.trigger = candidate;
    snippets_.push_back(copy);
    selected_snippet_index_ = static_cast<int>(snippets_.size()) - 1;
    editing_snippet_index_ = selected_snippet_index_;
    editor_new_mode_ = false;
    RefreshGroups();
    RefreshSnippetList(true);
    PopulateEditorFromSelection();
    if (SaveData()) {
        SetStatusText(L"Snippet duplicated and loaded into Snippet Manager.");
    } else {
        SetStatusText(L"Snippet duplicated in memory, but JSON save failed.");
    }
}

void AppWindow::ToggleSelectedSnippets() {
    const std::vector<int> selected_indices = SelectedSnippetIndices();
    if (selected_indices.empty()) {
        SetStatusText(L"Select one or more snippets first.");
        return;
    }

    for (int index : selected_indices) {
        if (index >= 0 && index < static_cast<int>(snippets_.size())) {
            snippets_[index].enabled = !snippets_[index].enabled;
        }
    }

    const int active_index = PrimarySelectedSnippetIndex();
    if (active_index >= 0) {
        selected_snippet_index_ = active_index;
    }

    RefreshSnippetList(true);
    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        PopulateEditorFromSelection();
    } else {
        RefreshEditorBanner();
    }

    if (SaveData()) {
        if (selected_indices.size() == 1) {
            const Snippet& snippet = snippets_[selected_indices.front()];
            const std::wstring status = (snippet.enabled ? std::wstring(L"Enabled snippet: ") : std::wstring(L"Disabled snippet: ")) + snippet.trigger;
            SetStatusText(status.c_str());
        } else {
            SetStatusText((L"Toggled " + std::to_wstring(selected_indices.size()) + L" snippets.").c_str());
        }
    } else {
        SetStatusText(L"Snippet toggle applied in memory, but JSON save failed.");
    }
}

void AppWindow::DeleteSelectedSnippet() {
    if (!ConfirmDiscardEditorChanges(L"while deleting a snippet")) {
        return;
    }
    std::vector<int> selected_indices = SelectedSnippetIndices();
    if (selected_indices.empty()) {
        SetStatusText(L"Select a snippet first.");
        return;
    }
    std::sort(selected_indices.begin(), selected_indices.end());
    selected_indices.erase(std::unique(selected_indices.begin(), selected_indices.end()), selected_indices.end());

    std::wstring message;
    if (selected_indices.size() == 1) {
        const Snippet snippet = snippets_[selected_indices.front()];
        message = L"Delete snippet '" + snippet.trigger + L"'?";
        if (!TrimCopy(snippet.notes).empty()) {
            message += L"\n\nNotes: " + snippet.notes;
        }
    } else {
        message = L"Delete " + std::to_wstring(selected_indices.size()) + L" selected snippets?";
        message += L"\n\nExamples:";
        const size_t preview_count = std::min<size_t>(5, selected_indices.size());
        for (size_t idx = 0; idx < preview_count; ++idx) {
            message += L"\n- " + snippets_[selected_indices[idx]].trigger;
        }
        if (selected_indices.size() > preview_count) {
            message += L"\n- ...";
        }
    }
    message += L"\n\nThis cannot be undone from the UI.";
    PromptDialogConfig dialog_config;
    dialog_config.title = L"Delete Snippet";
    dialog_config.message = message;
    dialog_config.primary_label = L"Delete";
    dialog_config.cancel_label = L"Cancel";
    dialog_config.primary_destructive = true;
    dialog_config.dark_theme = IsDarkTheme();
    PromptDialogResult dialog_result;
    if (!ShowPromptDialogWindow(hwnd_, instance_, dialog_config, dialog_result) ||
        dialog_result.choice != PromptDialogChoice::Primary) {
        SetStatusText(L"Snippet delete cancelled.");
        return;
    }

    const int active_index = PrimarySelectedSnippetIndex();
    selected_snippet_index_ = -1;
    int new_editing_index = editing_snippet_index_;
    bool deleted_editing_snippet = false;
    for (auto it = selected_indices.rbegin(); it != selected_indices.rend(); ++it) {
        const int deleted_index = *it;
        if (editing_snippet_index_ == deleted_index) {
            deleted_editing_snippet = true;
        } else if (editing_snippet_index_ > deleted_index) {
            --new_editing_index;
        }
        snippets_.erase(snippets_.begin() + deleted_index);
    }

    selected_snippet_index_ = -1;
    if (deleted_editing_snippet) {
        editing_snippet_index_ = -1;
        editor_new_mode_ = true;
    } else {
        editing_snippet_index_ = new_editing_index;
    }
    RefreshGroups();
    RefreshSnippetList(false);
    if (active_index >= 0 && active_index < static_cast<int>(snippets_.size())) {
        selected_snippet_index_ = std::min(active_index, static_cast<int>(snippets_.size()) - 1);
        SelectSnippetByActualIndex(selected_snippet_index_);
    }
    if (editing_snippet_index_ >= 0 && editing_snippet_index_ < static_cast<int>(snippets_.size())) {
        PopulateEditorFromSelection();
    } else {
        CreateNewSnippet();
    }

    if (SaveData()) {
        if (selected_indices.size() == 1) {
            SetStatusText(L"Snippet deleted from JSON.");
        } else {
            SetStatusText((L"Deleted " + std::to_wstring(selected_indices.size()) + L" snippets from JSON.").c_str());
        }
    } else {
        SetStatusText(L"Snippet deleted in memory, but JSON save failed.");
    }
}

void AppWindow::ToggleEngine() {
    engine_enabled_ = !engine_enabled_;
    typed_buffer_.clear();
    RefreshEngineButton();
    UpdateTrayIconTip();
    if (SaveData()) {
        SetStatusText(engine_enabled_ ? L"Engine enabled." : L"Engine disabled.");
    } else {
        SetStatusText(engine_enabled_ ? L"Engine enabled, but JSON save failed." : L"Engine disabled, but JSON save failed.");
    }
}

void AppWindow::RefreshEngineButton() const {
    SetWindowTextW(engine_button_, engine_enabled_ ? L"Engine: ON" : L"Engine: OFF");
    UpdateWindowTitle();
}

void AppWindow::UpdateWindowTitle() const {
    if (hwnd_ == nullptr) {
        return;
    }
    std::wstring title = kAppDisplayName;
    if (!engine_enabled_) {
        title += L" - PAUSED";
    }
    SetWindowTextW(hwnd_, title.c_str());
}

std::wstring AppWindow::ReadWindowText(HWND control) const {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) {
        return L"";
    }
    std::wstring value(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(control, &value[0], length + 1);
    value.resize(static_cast<size_t>(length));
    return value;
}

void AppWindow::WriteWindowText(HWND control, const std::wstring& value) const {
    SetWindowTextW(control, value.c_str());
}

void AppWindow::ExpandSnippetExternally(const Snippet& snippet, int delete_count) {
    bool expected = false;
    if (!expansion_in_progress_.compare_exchange_strong(expected, true)) {
        return;
    }

    const std::wstring content = snippet.content;
    const std::wstring trigger = snippet.trigger;
    const int actual_delete_count = delete_count >= 0 ? delete_count : static_cast<int>(snippet.trigger.size());
    const DWORD settle_delay = static_cast<DWORD>(std::max(0, settings_.instant_settle_ms));
    const DWORD backspace_delay = static_cast<DWORD>(std::max(0, settings_.backspace_delay_ms));
    const DWORD restore_delay = static_cast<DWORD>(std::max(0, settings_.restore_clipboard_delay_ms));

    std::thread([this, content, trigger, actual_delete_count, settle_delay, backspace_delay, restore_delay]() {
        auto send_key = [](WORD vk) {
            INPUT inputs[2] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = vk;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = vk;
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        };

        auto send_ctrl_v = []() {
            INPUT inputs[4] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_CONTROL;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = 'V';
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = 'V';
            inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = VK_CONTROL;
            inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(4, inputs, sizeof(INPUT));
        };

        auto read_clipboard_text = []() -> std::wstring {
            std::wstring text;
            if (!OpenClipboard(nullptr)) {
                return text;
            }
            HANDLE handle = GetClipboardData(CF_UNICODETEXT);
            if (handle != nullptr) {
                const auto* data = static_cast<const wchar_t*>(GlobalLock(handle));
                if (data != nullptr) {
                    text = data;
                    GlobalUnlock(handle);
                }
            }
            CloseClipboard();
            return text;
        };

        auto write_clipboard_text = [](const std::wstring& text) -> bool {
            if (!OpenClipboard(nullptr)) {
                return false;
            }
            EmptyClipboard();
            const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
            HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
            if (handle == nullptr) {
                CloseClipboard();
                return false;
            }
            void* memory = GlobalLock(handle);
            memcpy(memory, text.c_str(), bytes);
            GlobalUnlock(handle);
            SetClipboardData(CF_UNICODETEXT, handle);
            CloseClipboard();
            return true;
        };

        const std::wstring original_clipboard = read_clipboard_text();

        if (settle_delay > 0) {
            Sleep(settle_delay);
        }
        for (int index = 0; index < actual_delete_count; ++index) {
            send_key(VK_BACK);
            if (backspace_delay > 0) {
                Sleep(backspace_delay);
            }
        }

        suppress_clipboard_history_updates_.store(true);

        if (!write_clipboard_text(content)) {
            suppress_clipboard_history_updates_.store(false);
            expansion_in_progress_.store(false);
            PostStatusText(L"Could not access the clipboard for expansion.");
            return;
        }

        Sleep(6);
        send_ctrl_v();
        if (restore_delay > 0) {
            Sleep(restore_delay);
        }
        write_clipboard_text(original_clipboard);
        suppress_clipboard_history_updates_.store(false);
        expansion_in_progress_.store(false);
        PostStatusText(L"Expanded externally: " + trigger);
    }).detach();
}

bool AppWindow::ExpandPreviousClipboardEntryExternally(int delete_count) {
    const std::wstring content = previous_clipboard_text_;
    if (content.empty()) {
        SetStatusText(L"Copy at least two text items first.");
        return false;
    }
    if (expansion_in_progress_.exchange(true)) {
        return false;
    }

    const int actual_delete_count = delete_count >= 0 ? delete_count : static_cast<int>(std::wcslen(kPreviousClipboardTriggerText));
    const DWORD settle_delay = static_cast<DWORD>(std::max(0, settings_.instant_settle_ms));
    const DWORD backspace_delay = static_cast<DWORD>(std::max(0, settings_.backspace_delay_ms));
    const DWORD restore_delay = static_cast<DWORD>(std::max(0, settings_.restore_clipboard_delay_ms));

    std::thread([this, content, actual_delete_count, settle_delay, backspace_delay, restore_delay]() {
        auto send_key = [](WORD vk) {
            INPUT inputs[2] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = vk;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = vk;
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        };

        auto send_ctrl_v = []() {
            INPUT inputs[4] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_CONTROL;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = 'V';
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = 'V';
            inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = VK_CONTROL;
            inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(4, inputs, sizeof(INPUT));
        };

        const std::wstring original_clipboard = ReadUnicodeClipboardText();

        if (settle_delay > 0) {
            Sleep(settle_delay);
        }
        for (int index = 0; index < actual_delete_count; ++index) {
            send_key(VK_BACK);
            if (backspace_delay > 0) {
                Sleep(backspace_delay);
            }
        }

        suppress_clipboard_history_updates_.store(true);
        if (!WriteUnicodeClipboardText(content)) {
            suppress_clipboard_history_updates_.store(false);
            expansion_in_progress_.store(false);
            PostStatusText(L"Could not access the clipboard history item.");
            return;
        }

        Sleep(6);
        send_ctrl_v();
        if (restore_delay > 0) {
            Sleep(restore_delay);
        }
        WriteUnicodeClipboardText(original_clipboard);
        suppress_clipboard_history_updates_.store(false);
        expansion_in_progress_.store(false);
        PostStatusText(L"Pasted the previous clipboard item.");
    }).detach();
    return true;
}

bool AppWindow::ReplaceTrailingPreviousClipboardTrigger(std::wstring& text) const {
    if ((!settings_.use_previous_clipboard_trigger && !settings_.use_previous_clipboard_slash_trigger) || previous_clipboard_text_.empty()) {
        return false;
    }

    if (settings_.use_previous_clipboard_trigger) {
        const size_t trigger_length = std::wcslen(kPreviousClipboardTriggerText);
        if (text.size() >= trigger_length &&
            text.compare(text.size() - trigger_length, trigger_length, kPreviousClipboardTriggerText) == 0) {
            text.replace(text.size() - trigger_length, trigger_length, previous_clipboard_text_);
            return true;
        }
    }

    if (settings_.use_previous_clipboard_slash_trigger) {
        const size_t trigger_length = std::wcslen(kPreviousClipboardSlashTriggerText);
        if (text.size() >= trigger_length &&
            text.compare(text.size() - trigger_length, trigger_length, kPreviousClipboardSlashTriggerText) == 0) {
            text.replace(text.size() - trigger_length, trigger_length, previous_clipboard_text_);
            return true;
        }
    }

    return false;
}

bool AppWindow::ReplaceTrailingTrigger(std::wstring& text) const {
    if (const Snippet* snippet = FindMatchingSnippet(text)) {
        text.replace(text.size() - snippet->trigger.size(), snippet->trigger.size(), snippet->content);
        return true;
    }
    return false;
}

void AppWindow::ExpandTestAreaIfNeeded() {
    if (!engine_enabled_) {
        return;
    }

    std::wstring text = ReadWindowText(test_edit_);
    bool replaced = false;

    if (ReplaceTrailingPreviousClipboardTrigger(text)) {
        replaced = true;
    } else if (settings_.trigger_mode == L"separator") {
        struct SeparatorCandidate {
            std::wstring name;
            std::wstring text;
        };
        const SeparatorCandidate candidates[] = {
            {L"space", L" "},
            {L"tab", L"\t"},
            {L"enter", L"\r\n"},
            {L"enter", L"\n"},
            {L"enter", L"\r"},
        };

        for (const auto& candidate : candidates) {
            if (!IsSeparatorEnabled(candidate.name) || text.size() < candidate.text.size()) {
                continue;
            }
            if (text.compare(text.size() - candidate.text.size(), candidate.text.size(), candidate.text) != 0) {
                continue;
            }

            const std::wstring without_separator = text.substr(0, text.size() - candidate.text.size());
            if (previous_clipboard_text_.size() > 0) {
                if (settings_.use_previous_clipboard_trigger &&
                    without_separator.size() >= std::wcslen(kPreviousClipboardTriggerText) &&
                    without_separator.compare(
                        without_separator.size() - std::wcslen(kPreviousClipboardTriggerText),
                        std::wcslen(kPreviousClipboardTriggerText),
                        kPreviousClipboardTriggerText
                    ) == 0) {
                    text.replace(
                        text.size() - std::wcslen(kPreviousClipboardTriggerText) - candidate.text.size(),
                        std::wcslen(kPreviousClipboardTriggerText) + candidate.text.size(),
                        previous_clipboard_text_
                    );
                    replaced = true;
                    break;
                }
                if (settings_.use_previous_clipboard_slash_trigger &&
                    without_separator.size() >= std::wcslen(kPreviousClipboardSlashTriggerText) &&
                    without_separator.compare(
                        without_separator.size() - std::wcslen(kPreviousClipboardSlashTriggerText),
                        std::wcslen(kPreviousClipboardSlashTriggerText),
                        kPreviousClipboardSlashTriggerText
                    ) == 0) {
                    text.replace(
                        text.size() - std::wcslen(kPreviousClipboardSlashTriggerText) - candidate.text.size(),
                        std::wcslen(kPreviousClipboardSlashTriggerText) + candidate.text.size(),
                        previous_clipboard_text_
                    );
                    replaced = true;
                    break;
                }
            }
            if (const Snippet* snippet = FindMatchingSnippet(without_separator)) {
                text.replace(
                    text.size() - snippet->trigger.size() - candidate.text.size(),
                    snippet->trigger.size() + candidate.text.size(),
                    snippet->content
                );
                replaced = true;
                break;
            }
        }
    } else {
        replaced = ReplaceTrailingTrigger(text);
    }

    if (!replaced) {
        return;
    }

    updating_test_area_ = true;
    WriteWindowText(test_edit_, text);
    updating_test_area_ = false;
    SendMessageW(test_edit_, EM_SETSEL, static_cast<WPARAM>(text.size()), static_cast<LPARAM>(text.size()));
    SetStatusText(L"Expanded in C++ Test Area.");
}

bool AppWindow::IsGroupEnabled(const std::wstring& group_name) const {
    for (const auto& group : groups_) {
        if (group.name == group_name) {
            return group.enabled;
        }
    }
    return true;
}

bool AppWindow::IsSeparatorEnabled(const std::wstring& separator_name) const {
    const std::wstring normalized_name = ToLowerCopy(TrimCopy(separator_name));
    return std::any_of(settings_.word_separators.begin(), settings_.word_separators.end(), [&normalized_name, this](const std::wstring& value) {
        return ToLowerCopy(value) == normalized_name;
    });
}

void AppWindow::EnsureGroupExists(const std::wstring& group_name) {
    if (group_name.empty()) {
        return;
    }

    const auto it = std::find_if(groups_.begin(), groups_.end(), [&group_name](const Group& group) {
        return group.name == group_name;
    });
    if (it == groups_.end()) {
        groups_.push_back({group_name, true});
    }
}

bool AppWindow::GroupNameExists(const std::wstring& group_name, int ignore_index) const {
    const std::wstring normalized = TrimCopy(group_name);
    for (int index = 0; index < static_cast<int>(groups_.size()); ++index) {
        if (index == ignore_index) {
            continue;
        }
        if (groups_[index].name == normalized) {
            return true;
        }
    }
    return false;
}

bool AppWindow::AddTrayIcon() {
    if (tray_icon_added_) {
        return true;
    }

    tray_icon_data_ = {};
    tray_icon_data_.cbSize = sizeof(tray_icon_data_);
    tray_icon_data_.hWnd = hwnd_;
    tray_icon_data_.uID = 1;
    tray_icon_data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tray_icon_data_.uCallbackMessage = kTrayIconMessage;
    tray_icon_data_.hIcon = tray_icon_handle_ != nullptr ? tray_icon_handle_ : LoadIconW(nullptr, IDI_APPLICATION);
    UpdateTrayIconTip();

    if (!Shell_NotifyIconW(NIM_ADD, &tray_icon_data_)) {
        tray_icon_data_ = {};
        return false;
    }

    tray_icon_added_ = true;
    tray_icon_data_.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &tray_icon_data_);
    Shell_NotifyIconW(NIM_MODIFY, &tray_icon_data_);
    return true;
}

void AppWindow::UpdateTrayIconTip() {
    if (tray_icon_handle_ != nullptr) {
        tray_icon_data_.hIcon = engine_enabled_
            ? tray_icon_handle_
            : (tray_icon_paused_handle_ != nullptr ? tray_icon_paused_handle_ : tray_icon_handle_);
    }
    std::wstring tip = kAppDisplayName;
    tip += engine_enabled_ ? L" - Running" : L" - Paused";
    wcsncpy_s(tray_icon_data_.szTip, tip.c_str(), _TRUNCATE);
    if (tray_icon_added_) {
        tray_icon_data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        Shell_NotifyIconW(NIM_MODIFY, &tray_icon_data_);
    }
}

void AppWindow::CaptureWindowBounds() {
    if (hwnd_ == nullptr) {
        return;
    }

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    if (!GetWindowPlacement(hwnd_, &placement)) {
        return;
    }

    const RECT rect = placement.rcNormalPosition;
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        return;
    }

    settings_.window_bounds_valid = true;
    settings_.window_x = rect.left;
    settings_.window_y = rect.top;
    settings_.window_width = width;
    settings_.window_height = height;
}

void AppWindow::ApplyWindowBoundsFromSettings() {
    if (hwnd_ == nullptr || !settings_.window_bounds_valid) {
        return;
    }

    const POINT fallback_minimum = GetEffectiveMinimumWindowSize(hwnd_);
    RECT desired{
        settings_.window_x,
        settings_.window_y,
        settings_.window_x + std::max(static_cast<int>(fallback_minimum.x), settings_.window_width),
        settings_.window_y + std::max(static_cast<int>(fallback_minimum.y), settings_.window_height)
    };

    HMONITOR monitor = MonitorFromRect(&desired, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (monitor != nullptr && GetMonitorInfoW(monitor, &monitor_info)) {
        const POINT effective_minimum = GetEffectiveMinimumWindowSizeForMonitor(monitor_info);
        desired.right = desired.left + std::max(static_cast<int>(effective_minimum.x), settings_.window_width);
        desired.bottom = desired.top + std::max(static_cast<int>(effective_minimum.y), settings_.window_height);
        const RECT work = monitor_info.rcWork;
        const int width = desired.right - desired.left;
        const int height = desired.bottom - desired.top;
        if (desired.left < work.left) {
            desired.left = work.left;
            desired.right = desired.left + width;
        }
        if (desired.top < work.top) {
            desired.top = work.top;
            desired.bottom = desired.top + height;
        }
        if (desired.right > work.right) {
            desired.right = work.right;
            desired.left = desired.right - width;
        }
        if (desired.bottom > work.bottom) {
            desired.bottom = work.bottom;
            desired.top = desired.bottom - height;
        }
    }

    SetWindowPos(
        hwnd_,
        nullptr,
        desired.left,
        desired.top,
        desired.right - desired.left,
        desired.bottom - desired.top,
        SWP_NOZORDER | SWP_NOACTIVATE
    );
}

void AppWindow::RemoveTrayIcon() {
    if (!tray_icon_added_) {
        return;
    }
    Shell_NotifyIconW(NIM_DELETE, &tray_icon_data_);
    tray_icon_data_ = {};
    tray_icon_added_ = false;
}

void AppWindow::MinimizeToTray() {
    if (is_minimized_to_tray_) {
        return;
    }
    if (!AddTrayIcon()) {
        return;
    }

    is_minimized_to_tray_ = true;
    ShowWindow(hwnd_, SW_HIDE);
    SetStatusText(L"Running in tray.");
}

void AppWindow::RestoreFromTray() {
    if (!is_minimized_to_tray_ && !tray_icon_added_) {
        return;
    }

    is_minimized_to_tray_ = false;
    ShowWindow(hwnd_, SW_RESTORE);
    ShowWindow(hwnd_, SW_SHOW);
    RECT rect{};
    GetClientRect(hwnd_, &rect);
    LayoutChildren(rect.right - rect.left, rect.bottom - rect.top);
    InvalidateRect(hwnd_, nullptr, TRUE);
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    SetForegroundWindow(hwnd_);
    SetActiveWindow(hwnd_);
    SetStatusText(L"Restored from tray.");
}

void AppWindow::ShowGroupsContextMenu(POINT screen_point, bool clicked_item) {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    ApplyPopupMenuTheme(menu, surface_brush_);

    std::vector<std::unique_ptr<PopupMenuItemData>> menu_items;
    auto append_menu_item = [&](UINT id, const std::wstring& text, bool enabled, bool destructive = false) {
        auto item = std::make_unique<PopupMenuItemData>();
        item->text = text;
        item->enabled = enabled;
        item->destructive = destructive;
        PopupMenuItemData* item_ptr = item.get();
        menu_items.push_back(std::move(item));

        MENUITEMINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_DATA | MIIM_STATE;
        info.wID = id;
        info.fType = MFT_OWNERDRAW;
        info.dwItemData = reinterpret_cast<ULONG_PTR>(item_ptr);
        info.fState = enabled ? MFS_ENABLED : MFS_DISABLED;
        InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &info);
    };

    const UINT can_edit_group = clicked_item && selected_group_index_ > 0 ? MF_ENABLED : MF_GRAYED;
    append_menu_item(kNewGroupButton, L"New Group", true);
    if (clicked_item) {
        const bool can_edit = can_edit_group == MF_ENABLED;
        append_menu_item(kRenameGroupButton, L"Rename Group", can_edit);
        append_menu_item(kToggleGroupButton, L"Toggle Group", can_edit);
        append_menu_item(kDeleteGroupButton, L"Delete Group", can_edit, true);
    }

    SetForegroundWindow(hwnd_);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN, screen_point.x, screen_point.y, 0, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (command != 0) {
        PostMessageW(hwnd_, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
}

void AppWindow::ShowSnippetsContextMenu(POINT screen_point, bool clicked_item) {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    ApplyPopupMenuTheme(menu, surface_brush_);

    std::vector<std::unique_ptr<PopupMenuItemData>> menu_items;
    auto append_menu_item = [&](UINT id, const std::wstring& text, bool enabled, bool destructive = false, bool separator = false) {
        auto item = std::make_unique<PopupMenuItemData>();
        item->text = text;
        item->enabled = enabled;
        item->destructive = destructive;
        item->separator = separator;
        PopupMenuItemData* item_ptr = item.get();
        menu_items.push_back(std::move(item));

        MENUITEMINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_DATA | MIIM_STATE;
        info.wID = separator ? 0 : id;
        info.fType = MFT_OWNERDRAW;
        info.dwItemData = reinterpret_cast<ULONG_PTR>(item_ptr);
        info.fState = enabled ? MFS_ENABLED : MFS_DISABLED;
        InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &info);
    };

    append_menu_item(kSnippetMenuNew, L"New", true);
    if (clicked_item) {
        append_menu_item(0, L"", false, false, true);
        const bool has_selection = selected_snippet_index_ >= 0;
        append_menu_item(kSnippetMenuEdit, L"Edit Selected\tEnter / F2", has_selection);
        append_menu_item(kSnippetMenuDuplicate, L"Duplicate\tCtrl+D", has_selection);
        append_menu_item(kSnippetMenuDelete, L"Delete\tDel", has_selection, true);
    }

    SetForegroundWindow(hwnd_);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN, screen_point.x, screen_point.y, 0, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (command != 0) {
        PostMessageW(hwnd_, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
}

void AppWindow::ShowEditContextMenu(HWND control, POINT screen_point) {
    if (control == nullptr) {
        return;
    }

    active_context_edit_control_ = control;

    DWORD selection_range = static_cast<DWORD>(SendMessageW(control, EM_GETSEL, 0, 0));
    const int selection_start = LOWORD(selection_range);
    const int selection_end = HIWORD(selection_range);
    const bool has_selection = selection_end > selection_start;
    const bool can_undo = SendMessageW(control, EM_CANUNDO, 0, 0) != 0;
    const bool can_paste = IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE;
    const bool has_text = !ReadWindowText(control).empty();
    const bool rtl_enabled = (GetWindowLongPtrW(control, GWL_EXSTYLE) & WS_EX_RTLREADING) != 0;

    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    ApplyPopupMenuTheme(menu, surface_brush_);
    HMENU insert_unicode_submenu = CreatePopupMenu();
    if (insert_unicode_submenu != nullptr) {
        ApplyPopupMenuTheme(insert_unicode_submenu, surface_brush_);
    }

    std::vector<std::unique_ptr<PopupMenuItemData>> menu_items;
    auto append_menu_item = [&](HMENU target_menu, UINT id, const std::wstring& text, bool enabled, bool destructive = false, bool separator = false, HMENU submenu = nullptr) {
        auto item = std::make_unique<PopupMenuItemData>();
        item->text = text;
        item->enabled = enabled;
        item->destructive = destructive;
        item->separator = separator;
        item->has_submenu = submenu != nullptr;
        PopupMenuItemData* item_ptr = item.get();
        menu_items.push_back(std::move(item));

        MENUITEMINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_DATA | MIIM_STATE;
        if (submenu != nullptr) {
            info.fMask |= MIIM_SUBMENU;
        }
        info.wID = separator ? 0 : id;
        info.fType = MFT_OWNERDRAW;
        info.dwItemData = reinterpret_cast<ULONG_PTR>(item_ptr);
        info.fState = enabled ? MFS_ENABLED : MFS_DISABLED;
        info.hSubMenu = submenu;
        InsertMenuItemW(target_menu, GetMenuItemCount(target_menu), TRUE, &info);
    };

    append_menu_item(menu, kEditMenuUndo, L"Undo", can_undo);
    append_menu_item(menu, 0, L"", false, false, true);
    append_menu_item(menu, kEditMenuCut, L"Cut", has_selection);
    append_menu_item(menu, kEditMenuCopy, L"Copy", has_selection);
    append_menu_item(menu, kEditMenuPaste, L"Paste", can_paste);
    append_menu_item(menu, kEditMenuDelete, L"Delete", has_selection);
    append_menu_item(menu, 0, L"", false, false, true);
    append_menu_item(menu, kEditMenuSelectAll, L"Select All", has_text);
    append_menu_item(menu, 0, L"", false, false, true);
    append_menu_item(
        menu,
        kEditMenuRtlReadingOrder,
        rtl_enabled ? L"Right to left Reading order [On]" : L"Right to left Reading order",
        true
    );
    append_menu_item(menu, kEditMenuShowUnicodeControls, L"Show Unicode control characters", has_text);
    if (insert_unicode_submenu != nullptr) {
        append_menu_item(insert_unicode_submenu, kEditMenuInsertLrm, L"LRM - Left-to-right mark", true);
        append_menu_item(insert_unicode_submenu, kEditMenuInsertRlm, L"RLM - Right-to-left mark", true);
        append_menu_item(insert_unicode_submenu, kEditMenuInsertZwnj, L"ZWNJ - Zero-width non-joiner", true);
        append_menu_item(insert_unicode_submenu, kEditMenuInsertZwj, L"ZWJ - Zero-width joiner", true);
        append_menu_item(insert_unicode_submenu, kEditMenuInsertLri, L"LRI - Left-to-right isolate", true);
        append_menu_item(insert_unicode_submenu, kEditMenuInsertRli, L"RLI - Right-to-left isolate", true);
        append_menu_item(insert_unicode_submenu, kEditMenuInsertFsi, L"FSI - First strong isolate", true);
        append_menu_item(insert_unicode_submenu, kEditMenuInsertPdi, L"PDI - Pop directional isolate", true);
        append_menu_item(menu, kEditMenuInsertUnicodeControl, L"Insert Unicode control character", true, false, false, insert_unicode_submenu);
    } else {
        append_menu_item(menu, kEditMenuInsertUnicodeControl, L"Insert Unicode control character", false);
    }
    append_menu_item(menu, kEditMenuOpenIme, L"Open IME", true);
    append_menu_item(menu, kEditMenuReconversion, L"Reconversion", true);

    SetForegroundWindow(hwnd_);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN, screen_point.x, screen_point.y, 0, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (command != 0) {
        PostMessageW(hwnd_, WM_COMMAND, MAKEWPARAM(command, 0), reinterpret_cast<LPARAM>(control));
    }
}

void AppWindow::ShowTrayMenu() {
    if (!tray_icon_added_) {
        return;
    }

    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    ApplyPopupMenuTheme(menu, surface_brush_);

    std::vector<std::unique_ptr<PopupMenuItemData>> menu_items;
    auto append_menu_item = [&](UINT id, const std::wstring& text, bool enabled, bool destructive = false, bool separator = false) {
        auto item = std::make_unique<PopupMenuItemData>();
        item->text = text;
        item->enabled = enabled;
        item->destructive = destructive;
        item->separator = separator;
        PopupMenuItemData* item_ptr = item.get();
        menu_items.push_back(std::move(item));

        MENUITEMINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_DATA | MIIM_STATE;
        info.wID = separator ? 0 : id;
        info.fType = MFT_OWNERDRAW;
        info.dwItemData = reinterpret_cast<ULONG_PTR>(item_ptr);
        info.fState = enabled ? MFS_ENABLED : MFS_DISABLED;
        InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &info);
    };

    append_menu_item(kTrayMenuOpen, L"Open BlinkText", true);
    append_menu_item(kTrayMenuToggleEngine, engine_enabled_ ? L"Pause Engine" : L"Resume Engine", true);
    append_menu_item(0, L"", false, false, true);
    append_menu_item(kTrayMenuExit, L"Exit", true, true);

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, cursor.x, cursor.y, 0, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

void AppWindow::ReleaseToolbarButtonImages() {
    if (github_button_bitmap_ != nullptr) {
        delete github_button_bitmap_;
        github_button_bitmap_ = nullptr;
    }
    if (info_button_bitmap_ != nullptr) {
        delete info_button_bitmap_;
        info_button_bitmap_ = nullptr;
    }
    if (gdiplus_token_ != 0) {
        Gdiplus::GdiplusShutdown(gdiplus_token_);
        gdiplus_token_ = 0;
    }
}

bool AppWindow::LoadToolbarButtonImages() {
    ReleaseToolbarButtonImages();

    Gdiplus::GdiplusStartupInput startup_input;
    if (Gdiplus::GdiplusStartup(&gdiplus_token_, &startup_input, nullptr) != Gdiplus::Ok) {
        gdiplus_token_ = 0;
        return false;
    }

    auto load_bitmap_from_resource = [](WORD resource_id) -> Gdiplus::Bitmap* {
        HMODULE module = GetModuleHandleW(nullptr);
        HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resource_id), RT_RCDATA);
        if (resource == nullptr) {
            return nullptr;
        }

        HGLOBAL loaded_resource = LoadResource(module, resource);
        if (loaded_resource == nullptr) {
            return nullptr;
        }

        const DWORD resource_size = SizeofResource(module, resource);
        void* resource_data = LockResource(loaded_resource);
        if (resource_data == nullptr || resource_size == 0) {
            return nullptr;
        }

        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, resource_size);
        if (memory == nullptr) {
            return nullptr;
        }

        void* memory_data = GlobalLock(memory);
        if (memory_data == nullptr) {
            GlobalFree(memory);
            return nullptr;
        }

        std::memcpy(memory_data, resource_data, resource_size);
        GlobalUnlock(memory);

        IStream* stream = nullptr;
        if (CreateStreamOnHGlobal(memory, TRUE, &stream) != S_OK || stream == nullptr) {
            GlobalFree(memory);
            return nullptr;
        }

        auto* bitmap = Gdiplus::Bitmap::FromStream(stream, FALSE);
        stream->Release();

        if (bitmap != nullptr && bitmap->GetLastStatus() == Gdiplus::Ok) {
            return bitmap;
        }

        delete bitmap;
        return nullptr;
    };

    github_button_bitmap_ = load_bitmap_from_resource(IDB_GITHUB_PNG);
    info_button_bitmap_ = load_bitmap_from_resource(IDB_INFO_PNG);

    if (github_button_bitmap_ != nullptr || info_button_bitmap_ != nullptr) {
        return true;
    }

    std::vector<std::wstring> search_roots;
    const std::wstring exe_dir = GetExecutableDirectory();
    if (!exe_dir.empty()) {
        search_roots.push_back(exe_dir);
        search_roots.push_back(JoinPath(exe_dir, L"..\\src"));
    }

    wchar_t current_dir_buffer[32768] = {};
    const DWORD current_dir_length = GetCurrentDirectoryW(static_cast<DWORD>(std::size(current_dir_buffer)), current_dir_buffer);
    if (current_dir_length > 0 && current_dir_length < std::size(current_dir_buffer)) {
        const std::wstring current_dir(current_dir_buffer, current_dir_length);
        if (std::none_of(search_roots.begin(), search_roots.end(), [&current_dir](const std::wstring& item) { return item == current_dir; })) {
            search_roots.push_back(current_dir);
        }
        const std::wstring current_src_dir = JoinPath(current_dir, L"src");
        if (std::none_of(search_roots.begin(), search_roots.end(), [&current_src_dir](const std::wstring& item) { return item == current_src_dir; })) {
            search_roots.push_back(current_src_dir);
        }
    }

    auto load_bitmap = [&](const wchar_t* file_name) -> Gdiplus::Bitmap* {
        for (const auto& root : search_roots) {
            const std::wstring image_path = JoinPath(root, file_name);
            if (!FileExists(image_path)) {
                continue;
            }

            auto* bitmap = Gdiplus::Bitmap::FromFile(image_path.c_str(), FALSE);
            if (bitmap != nullptr && bitmap->GetLastStatus() == Gdiplus::Ok) {
                return bitmap;
            }
            delete bitmap;
        }
        return nullptr;
    };

    github_button_bitmap_ = load_bitmap(kGithubButtonImageName);
    info_button_bitmap_ = load_bitmap(kInfoButtonImageName);
    if (github_button_bitmap_ == nullptr && info_button_bitmap_ == nullptr) {
        ReleaseToolbarButtonImages();
        return false;
    }
    return true;
}

void AppWindow::ReleaseLoadedIcons() {
    if (owns_window_icons_) {
        if (window_icon_big_ != nullptr) {
            DestroyIcon(window_icon_big_);
        }
        if (window_icon_small_ != nullptr && window_icon_small_ != window_icon_big_) {
            DestroyIcon(window_icon_small_);
        }
    }
    if (owns_tray_icon_ && tray_icon_handle_ != nullptr && tray_icon_handle_ != window_icon_small_ && tray_icon_handle_ != window_icon_big_) {
        DestroyIcon(tray_icon_handle_);
    }
    if (owns_tray_pause_icon_ &&
        tray_icon_paused_handle_ != nullptr &&
        tray_icon_paused_handle_ != tray_icon_handle_ &&
        tray_icon_paused_handle_ != window_icon_small_ &&
        tray_icon_paused_handle_ != window_icon_big_) {
        DestroyIcon(tray_icon_paused_handle_);
    }

    window_icon_big_ = nullptr;
    window_icon_small_ = nullptr;
    tray_icon_handle_ = nullptr;
    tray_icon_paused_handle_ = nullptr;
    owns_window_icons_ = false;
    owns_tray_icon_ = false;
    owns_tray_pause_icon_ = false;
}

bool AppWindow::LoadAppIcons() {
    ReleaseLoadedIcons();

    std::vector<std::wstring> search_roots;
    const std::wstring exe_dir = GetExecutableDirectory();
    if (!exe_dir.empty()) {
        search_roots.push_back(exe_dir);
        search_roots.push_back(JoinPath(exe_dir, L"..\\src"));
    }

    wchar_t current_dir_buffer[32768] = {};
    const DWORD current_dir_length = GetCurrentDirectoryW(static_cast<DWORD>(std::size(current_dir_buffer)), current_dir_buffer);
    if (current_dir_length > 0 && current_dir_length < std::size(current_dir_buffer)) {
        const std::wstring current_dir(current_dir_buffer, current_dir_length);
        if (std::none_of(search_roots.begin(), search_roots.end(), [&current_dir](const std::wstring& item) { return item == current_dir; })) {
            search_roots.push_back(current_dir);
        }
        const std::wstring current_src_dir = JoinPath(current_dir, L"src");
        if (std::none_of(search_roots.begin(), search_roots.end(), [&current_src_dir](const std::wstring& item) { return item == current_src_dir; })) {
            search_roots.push_back(current_src_dir);
        }
    }

    auto load_icon_from_candidates = [&](const wchar_t* const* candidates, std::size_t candidate_count, int icon_size) -> HICON {
        for (const auto& root : search_roots) {
            for (std::size_t index = 0; index < candidate_count; ++index) {
                const wchar_t* icon_name = candidates[index];
                const std::wstring icon_path = JoinPath(root, icon_name);
                if (!FileExists(icon_path)) {
                    continue;
                }

                HICON icon = static_cast<HICON>(LoadImageW(nullptr, icon_path.c_str(), IMAGE_ICON, icon_size, icon_size, LR_LOADFROMFILE));
                if (icon != nullptr) {
                    return icon;
                }
            }
        }
        return nullptr;
    };

    for (const auto& root : search_roots) {
        for (const wchar_t* icon_name : kWindowIconCandidates) {
            const std::wstring icon_path = JoinPath(root, icon_name);
            if (!FileExists(icon_path)) {
                continue;
            }

            HICON big = static_cast<HICON>(LoadImageW(nullptr, icon_path.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE));
            HICON small = static_cast<HICON>(LoadImageW(nullptr, icon_path.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE));
            HICON tray = static_cast<HICON>(LoadImageW(nullptr, icon_path.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE));

            if (big == nullptr && small == nullptr && tray == nullptr) {
                continue;
            }

            if (big == nullptr && small != nullptr) {
                big = CopyIcon(small);
            }
            if (small == nullptr && big != nullptr) {
                small = CopyIcon(big);
            }
            if (tray == nullptr && small != nullptr) {
                tray = CopyIcon(small);
            }
            if (tray == nullptr && big != nullptr) {
                tray = CopyIcon(big);
            }

            window_icon_big_ = big;
            window_icon_small_ = small != nullptr ? small : big;
            tray_icon_handle_ = tray != nullptr ? tray : window_icon_small_;
            tray_icon_paused_handle_ = load_icon_from_candidates(kTrayPauseIconCandidates, std::size(kTrayPauseIconCandidates), 16);
            if (tray_icon_paused_handle_ == nullptr) {
                tray_icon_paused_handle_ = tray_icon_handle_;
            }
            owns_window_icons_ = true;
            owns_tray_icon_ = tray_icon_handle_ != nullptr;
            owns_tray_pause_icon_ = tray_icon_paused_handle_ != nullptr && tray_icon_paused_handle_ != tray_icon_handle_;
            return true;
        }
    }

    HINSTANCE module = GetModuleHandleW(nullptr);

	window_icon_big_ = static_cast<HICON>(LoadImageW(
		module,
		MAKEINTRESOURCEW(IDI_APP_ICON),
		IMAGE_ICON,
		32,
		32,
		LR_DEFAULTCOLOR
	));

	window_icon_small_ = static_cast<HICON>(LoadImageW(
		module,
		MAKEINTRESOURCEW(IDI_APP_ICON),
		IMAGE_ICON,
		16,
		16,
		LR_DEFAULTCOLOR
	));

	tray_icon_handle_ = static_cast<HICON>(LoadImageW(
		module,
		MAKEINTRESOURCEW(IDI_APP_ICON),
		IMAGE_ICON,
		16,
		16,
		LR_DEFAULTCOLOR
	));

	if (window_icon_big_ == nullptr) {
		window_icon_big_ = LoadIconW(nullptr, IDI_APPLICATION);
	}
	if (window_icon_small_ == nullptr) {
		window_icon_small_ = window_icon_big_;
	}
	if (tray_icon_handle_ == nullptr) {
		tray_icon_handle_ = window_icon_small_;
	}
    tray_icon_paused_handle_ = load_icon_from_candidates(kTrayPauseIconCandidates, std::size(kTrayPauseIconCandidates), 16);
    if (tray_icon_paused_handle_ == nullptr) {
        tray_icon_paused_handle_ = static_cast<HICON>(LoadImageW(
            GetModuleHandleW(nullptr),
            MAKEINTRESOURCEW(IDI_APP_PAUSE_ICON),
            IMAGE_ICON,
            16,
            16,
            LR_DEFAULTCOLOR
        ));
    }
    if (tray_icon_paused_handle_ == nullptr) {
        tray_icon_paused_handle_ = tray_icon_handle_;
    }
	
	owns_window_icons_ = true;
	owns_tray_icon_ = true;
    owns_tray_pause_icon_ = tray_icon_paused_handle_ != nullptr && tray_icon_paused_handle_ != tray_icon_handle_;
	
    
    return window_icon_big_ != nullptr;
}


std::wstring AppWindow::DetermineDataFilePath() const {
    std::wstring preferred_path;

    wchar_t env_buffer[32768] = {};
    const DWORD env_length = GetEnvironmentVariableW(L"BlinkTexts_JSON_PATH", env_buffer, static_cast<DWORD>(std::size(env_buffer)));
    if (env_length > 0 && env_length < std::size(env_buffer)) {
        preferred_path.assign(env_buffer, env_length);
    }

    std::vector<std::wstring> candidates;
    if (!preferred_path.empty()) {
        candidates.push_back(preferred_path);
    }

    const std::wstring exe_dir = GetExecutableDirectory();
    if (!exe_dir.empty()) {
        candidates.push_back(JoinPath(exe_dir, kDefaultDataFileName));
    }

    wchar_t current_dir_buffer[32768] = {};
    const DWORD current_dir_length = GetCurrentDirectoryW(static_cast<DWORD>(std::size(current_dir_buffer)), current_dir_buffer);
    if (current_dir_length > 0 && current_dir_length < std::size(current_dir_buffer)) {
        candidates.push_back(JoinPath(std::wstring(current_dir_buffer, current_dir_length), kDefaultDataFileName));
    }

    candidates.push_back(kLegacyProjectDataPath);

    for (const auto& candidate : candidates) {
        if (FileExists(candidate)) {
            return candidate;
        }
    }

    if (!preferred_path.empty()) {
        return preferred_path;
    }
    if (!exe_dir.empty()) {
        return JoinPath(exe_dir, kDefaultDataFileName);
    }
    if (current_dir_length > 0 && current_dir_length < std::size(current_dir_buffer)) {
        return JoinPath(std::wstring(current_dir_buffer, current_dir_length), kDefaultDataFileName);
    }
    return kDefaultDataFileName;
}

std::wstring AppWindow::JoinPath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    if (left.back() == L'\\' || left.back() == L'/') {
        return left + right;
    }
    return left + L"\\" + right;
}

bool AppWindow::FileExists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring AppWindow::GetExecutableDirectory() {
    wchar_t buffer[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    if (length == 0 || length >= std::size(buffer)) {
        return L"";
    }

    std::wstring path(buffer, length);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, slash);
}

std::wstring AppWindow::GetExecutablePath() {
    wchar_t buffer[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    if (length == 0 || length >= std::size(buffer)) {
        return L"";
    }
    return std::wstring(buffer, length);
}

std::wstring AppWindow::Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return L"";
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0) {
        return L"";
    }
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length);
    return result;
}

std::string AppWindow::WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return "";
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return "";
    }
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr);
    return result;
}

std::string AppWindow::ReadFileBytes(const std::wstring& path) {
    HANDLE handle = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (handle == INVALID_HANDLE_VALUE) {
        return "";
    }

    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(handle, &size) || size.QuadPart < 0) {
        CloseHandle(handle);
        return "";
    }

    std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
    DWORD total_read = 0;
    while (total_read < bytes.size()) {
        DWORD chunk_read = 0;
        const DWORD to_read = static_cast<DWORD>(std::min<size_t>(1 << 20, bytes.size() - total_read));
        if (!ReadFile(handle, bytes.data() + total_read, to_read, &chunk_read, nullptr)) {
            CloseHandle(handle);
            return "";
        }
        if (chunk_read == 0) {
            break;
        }
        total_read += chunk_read;
    }
    bytes.resize(total_read);
    CloseHandle(handle);
    return bytes;
}

bool AppWindow::WriteFileBytesAtomically(const std::wstring& path, const std::string& bytes) {
    const std::wstring temp_path = path + L".tmp";
    const std::wstring backup_path = path + L".bak";

    if (FileExists(path)) {
        CopyFileW(path.c_str(), backup_path.c_str(), FALSE);
    }

    HANDLE handle = CreateFileW(
        temp_path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD total_written = 0;
    while (total_written < bytes.size()) {
        DWORD chunk_written = 0;
        const DWORD to_write = static_cast<DWORD>(std::min<size_t>(1 << 20, bytes.size() - total_written));
        if (!WriteFile(handle, bytes.data() + total_written, to_write, &chunk_written, nullptr)) {
            CloseHandle(handle);
            DeleteFileW(temp_path.c_str());
            return false;
        }
        total_written += chunk_written;
    }

    CloseHandle(handle);

    if (!MoveFileExW(temp_path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp_path.c_str());
        return false;
    }

    return true;
}

bool AppWindow::ParseHotkeyString(const std::wstring& hotkey, UINT& modifiers, UINT& vk_code) {
    modifiers = 0;
    vk_code = 0;

    const std::wstring normalized = ToLowerCopy(TrimCopy(hotkey));
    if (normalized.empty()) {
        return false;
    }

    size_t start = 0;
    while (start <= normalized.size()) {
        const size_t separator = normalized.find(L'+', start);
        std::wstring token = TrimCopy(normalized.substr(start, separator == std::wstring::npos ? std::wstring::npos : separator - start));
        if (token.empty()) {
            return false;
        }

        if (token == L"ctrl" || token == L"control") {
            modifiers |= MOD_CONTROL;
        } else if (token == L"shift") {
            modifiers |= MOD_SHIFT;
        } else if (token == L"alt") {
            modifiers |= MOD_ALT;
        } else if (token == L"win" || token == L"windows" || token == L"cmd" || token == L"meta") {
            modifiers |= MOD_WIN;
        } else {
            if (vk_code != 0) {
                return false;
            }

            if (token.size() == 1) {
                const wchar_t ch = token.front();
                if (ch >= L'a' && ch <= L'z') {
                    vk_code = static_cast<UINT>(towupper(ch));
                } else if (ch >= L'0' && ch <= L'9') {
                    vk_code = static_cast<UINT>(ch);
                } else {
                    return false;
                }
            } else if (token.size() >= 2 && token.front() == L'f') {
                try {
                    const int number = std::stoi(token.substr(1));
                    if (number < 1 || number > 24) {
                        return false;
                    }
                    vk_code = static_cast<UINT>(VK_F1 + number - 1);
                } catch (...) {
                    return false;
                }
            } else if (token == L"tab") {
                vk_code = VK_TAB;
            } else if (token == L"space") {
                vk_code = VK_SPACE;
            } else if (token == L"enter" || token == L"return") {
                vk_code = VK_RETURN;
            } else if (token == L"esc" || token == L"escape") {
                vk_code = VK_ESCAPE;
            } else if (token == L"insert" || token == L"ins") {
                vk_code = VK_INSERT;
            } else if (token == L"delete" || token == L"del") {
                vk_code = VK_DELETE;
            } else if (token == L"home") {
                vk_code = VK_HOME;
            } else if (token == L"end") {
                vk_code = VK_END;
            } else if (token == L"pageup" || token == L"pgup") {
                vk_code = VK_PRIOR;
            } else if (token == L"pagedown" || token == L"pgdn") {
                vk_code = VK_NEXT;
            } else if (token == L"up") {
                vk_code = VK_UP;
            } else if (token == L"down") {
                vk_code = VK_DOWN;
            } else if (token == L"left") {
                vk_code = VK_LEFT;
            } else if (token == L"right") {
                vk_code = VK_RIGHT;
            } else {
                return false;
            }
        }

        if (separator == std::wstring::npos) {
            break;
        }
        start = separator + 1;
    }

    return vk_code != 0;
}

std::wstring AppWindow::TrimCopy(const std::wstring& value) {
    size_t start = 0;
    while (start < value.size() && iswspace(value[start])) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && iswspace(value[end - 1])) {
        --end;
    }

    return value.substr(start, end - start);
}

bool AppWindow::IsModifierVirtualKey(UINT vk_code) {
    switch (vk_code) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
        return true;
    default:
        return false;
    }
}

std::wstring AppWindow::HotkeyKeyName(UINT vk_code) {
    if (vk_code >= 'A' && vk_code <= 'Z') {
        return std::wstring(1, static_cast<wchar_t>(towlower(static_cast<wchar_t>(vk_code))));
    }
    if (vk_code >= '0' && vk_code <= '9') {
        return std::wstring(1, static_cast<wchar_t>(vk_code));
    }
    if (vk_code >= VK_F1 && vk_code <= VK_F24) {
        return L"f" + std::to_wstring(vk_code - VK_F1 + 1);
    }

    switch (vk_code) {
    case VK_TAB:
        return L"tab";
    case VK_SPACE:
        return L"space";
    case VK_RETURN:
        return L"enter";
    case VK_ESCAPE:
        return L"escape";
    case VK_INSERT:
        return L"insert";
    case VK_DELETE:
        return L"delete";
    case VK_HOME:
        return L"home";
    case VK_END:
        return L"end";
    case VK_PRIOR:
        return L"pageup";
    case VK_NEXT:
        return L"pagedown";
    case VK_UP:
        return L"up";
    case VK_DOWN:
        return L"down";
    case VK_LEFT:
        return L"left";
    case VK_RIGHT:
        return L"right";
    default:
        return L"";
    }
}

std::wstring AppWindow::BuildHotkeyString(UINT modifiers, UINT vk_code) const {
    const std::wstring key_name = HotkeyKeyName(vk_code);
    if (key_name.empty()) {
        return L"";
    }

    std::wstring hotkey;
    if ((modifiers & MOD_CONTROL) != 0) {
        hotkey += L"ctrl+";
    }
    if ((modifiers & MOD_SHIFT) != 0) {
        hotkey += L"shift+";
    }
    if ((modifiers & MOD_ALT) != 0) {
        hotkey += L"alt+";
    }
    if ((modifiers & MOD_WIN) != 0) {
        hotkey += L"win+";
    }
    hotkey += key_name;
    return hotkey;
}

std::wstring AppWindow::NormalizeTriggerForCompare(const std::wstring& trigger) const {
    const std::wstring trimmed = TrimCopy(trigger);
    return settings_.match_case_sensitive ? trimmed : ToLowerCopy(trimmed);
}

bool AppWindow::TriggersEqual(const std::wstring& left, const std::wstring& right) const {
    return NormalizeTriggerForCompare(left) == NormalizeTriggerForCompare(right);
}
