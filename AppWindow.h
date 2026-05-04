#pragma once

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include <atomic>
#include <string>
#include <vector>

namespace Gdiplus {
class Bitmap;
}

namespace simplejson {
struct Value;
}

class AppWindow {
public:
    bool Create(HINSTANCE instance);
    void Show(int command_show) const;
    bool TranslateAppAccelerator(MSG* message) const;

private:
    friend LRESULT CALLBACK ImportOptionsDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    struct Group {
        std::wstring name;
        bool enabled = true;
    };

    struct Snippet {
        std::wstring trigger;
        std::wstring group;
        std::wstring notes;
        std::wstring content;
        bool enabled = true;
    };

    struct Settings {
        std::wstring theme = L"dark";
        bool always_on_top = false;
        int restore_clipboard_delay_ms = 60;
        bool match_case_sensitive = false;
        std::wstring trigger_mode = L"instant";
        std::vector<std::wstring> word_separators = {L"space", L"enter", L"tab"};
        std::wstring hotkey = L"ctrl+shift+f12";
        bool engine_enabled = true;
        int instant_settle_ms = 10;
        int backspace_delay_ms = 1;
        bool start_with_windows = false;
        bool minimize_to_tray = true;
        bool use_previous_clipboard_trigger = false;
        bool use_previous_clipboard_slash_trigger = false;
        bool window_bounds_valid = false;
        int window_x = 0;
        int window_y = 0;
        int window_width = 1125;
        int window_height = 780;
        int left_panel_width = 210;
        int right_panel_width = 305;
        int groups_panel_height = 248;
        std::wstring last_group_filter;
        int snippet_sort_column = 0;
        bool snippet_sort_ascending = true;
    };

public:
    struct ParsedImportData {
        Settings settings;
        std::vector<Group> groups;
        std::vector<Snippet> snippets;
        bool has_settings = false;
        bool source_groups_available = false;
        bool is_beeftext_format = false;
    };

    struct ImportOptions {
        bool keep_source_groups = false;
        std::wstring target_group;
        bool overwrite_conflicts = false;
    };

    struct ExportOptions {
        bool beeftext_format = false;
        bool current_group_only = false;
    };

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK SnippetsHeaderSubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data);
    static LRESULT CALLBACK ComboBoxSubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data);
    static LRESULT CALLBACK EditContextSubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data);
    static LRESULT CALLBACK InteractiveControlSubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data);
    LRESULT HandleMessage(UINT message, WPARAM wparam, LPARAM lparam);

    void CreateChildren();
    void LayoutChildren(int width, int height);
    void ApplyFonts() const;
    void SetStatusText(const wchar_t* text) const;
    bool IsDarkTheme() const;
    void ApplyTheme();
    void ReleaseThemeBrushes();
    void MarkEditorClean();
    void MarkEditorDirty();
    bool ConfirmDiscardEditorChanges(const wchar_t* action_label);
    void LoadData();
    bool LoadDataFromFile(const std::wstring& path);
    bool ParseImportFile(const std::wstring& path, ParsedImportData& data, std::wstring& error) const;
    bool ParseNativeData(const simplejson::Value& root, ParsedImportData& data) const;
    bool ParseBeeftextData(const simplejson::Value& root, ParsedImportData& data) const;
    bool SaveData();
    bool SaveDataToFile(const std::wstring& path) const;
    bool ExportDataToFile(const std::wstring& path, const ExportOptions& options) const;
    void ImportDataFromDialog();
    void ExportDataToDialog();
    void LoadFallbackData();
    void ApplyLoadedSettings();
    void RefreshSettingsControls() const;
    bool ShowImportOptionsDialog(std::wstring& file_path, ParsedImportData& data, ImportOptions& options) const;
    bool ShowExportOptionsDialog(ExportOptions& options) const;
    int CountImportConflicts(const ParsedImportData& data) const;
    void MergeImportedData(const ParsedImportData& data, const ImportOptions& options, int& added_count, int& updated_count, int& skipped_count);
    void RefreshGroups();
    void RefreshEditorGroupCombo(const std::wstring& preferred_group = L"");
    void UpdateEditorGroupComboDropPanel(bool drop_down_open);
    void RefreshEditorBanner() const;
    void RefreshEditorTabButtons() const;
    void UpdateEditorSectionVisibility() const;
    void SetEditorTab(bool show_engine_settings);
    void RefreshSnippetActionButtons() const;
    void ShowEditContextMenu(HWND control, POINT screen_point);
    void RefreshClipboardHistoryFromSystem();
    bool ExpandPreviousClipboardEntryExternally(int delete_count = -1);
    void RefreshSnippetList(bool keep_selection = true);
    void UpdateSnippetSortIndicators() const;
    void UpdateSnippetListColumns(int total_width) const;
    std::wstring BuildSnippetPreview(const Snippet& snippet) const;
    std::wstring CurrentSearchQuery() const;
    void ShowSearchPlaceholder();
    void HideSearchPlaceholder();
    void UpdateWindowFrameTheme() const;
    void SyncWindowScrollBar(int viewport_height, int content_height);
    void SyncEditorSettingsScrollBar(int viewport_height, int content_height);
    void LayoutEditorSettingsSection();
    std::vector<int> SelectedSnippetIndices() const;
    int PrimarySelectedSnippetIndex() const;
    int SelectedSnippetVisibleIndex() const;
    void SetSnippetListSelection(int visible_index);
    void SelectSnippetByActualIndex(int actual_index);
    void EditSelectedSnippet();
    void PopulateEditorFromSelection();
    void ResetEditor();
    void CreateNewSnippet();
    void StartNewSnippetCommand();
    void CreateGroup();
    void RenameSelectedGroup();
    void ToggleSelectedGroup();
    void DeleteSelectedGroup();
    bool SaveCurrentSnippet();
    void SaveEngineSettings();
    void ResetEngineSettings();
    void DuplicateSelectedSnippet();
    void ToggleSelectedSnippets();
    void DeleteSelectedSnippet();
    void ResetTestArea();
    void ExpandTestAreaIfNeeded();
    void ToggleEngine();
    void RefreshEngineButton() const;
    void UpdateWindowTitle() const;
    bool StartGlobalServices();
    void StopGlobalServices();
    bool RegisterCurrentHotkey();
    void UnregisterCurrentHotkey();
    void ApplyAlwaysOnTopSetting();
    void UpdateHotkeyDisplay();
    void BeginHotkeyCapture();
    void CancelHotkeyCapture(bool restore_registration = true);
    bool FinishHotkeyCapture(UINT vk_code);
    bool ApplyHotkeySetting(const std::wstring& hotkey, bool save_to_json = true);
    bool UpdateStartWithWindows(bool enabled);
    bool ReadStartWithWindows() const;
    bool UsesSurfaceBackground(HWND control) const;
    static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wparam, LPARAM lparam);
    LRESULT HandleLowLevelKeyboard(WPARAM wparam, const KBDLLHOOKSTRUCT& info);
    bool IsAppForeground() const;
    std::wstring TranslateKeyboardEvent(const KBDLLHOOKSTRUCT& info) const;
    const Snippet* FindMatchingSnippet(const std::wstring& text, bool exact_match = false) const;
    void ResetTypedBufferForKey(DWORD vk_code);
    void ExpandSnippetExternally(const Snippet& snippet, int delete_count = -1);
    void PostStatusText(std::wstring text) const;
    bool ReplaceTrailingPreviousClipboardTrigger(std::wstring& text) const;
    std::wstring CurrentGroupFilter() const;
    int CurrentGroupVectorIndex() const;
    std::wstring CurrentEditorGroupSelection() const;
    std::wstring ReadWindowText(HWND control) const;
    void WriteWindowText(HWND control, const std::wstring& value) const;
    static std::wstring ToLowerCopy(std::wstring value);
    bool SnippetMatchesCurrentFilter(const Snippet& snippet) const;
    bool ReplaceTrailingTrigger(std::wstring& text) const;
    bool IsGroupEnabled(const std::wstring& group_name) const;
    bool IsSeparatorEnabled(const std::wstring& separator_name) const;
    void EnsureGroupExists(const std::wstring& group_name);
    bool GroupNameExists(const std::wstring& group_name, int ignore_index = -1) const;
    bool AddTrayIcon();
    void RemoveTrayIcon();
    void MinimizeToTray();
    void RestoreFromTray();
    void ShowTrayMenu();
    void ShowGroupsContextMenu(POINT screen_point, bool clicked_item);
    void ShowSnippetsContextMenu(POINT screen_point, bool clicked_item);
    void UpdateTrayIconTip();
    void CaptureWindowBounds();
    void ApplyWindowBoundsFromSettings();
    void ReleaseLoadedIcons();
    bool LoadAppIcons();
    bool LoadToolbarButtonImages();
    void ReleaseToolbarButtonImages();
    void ShowInfoDialog() const;
    bool HandleSelectAllShortcut();
    void ClampPanelSizes(int client_width, int client_height);

    enum class ActiveSplitter {
        None,
        LeftVertical,
        RightVertical,
        LeftHorizontal,
    };
    ActiveSplitter HitTestSplitter(POINT client_point) const;

    std::wstring DetermineDataFilePath() const;
    static std::wstring JoinPath(const std::wstring& left, const std::wstring& right);
    static bool FileExists(const std::wstring& path);
    static std::wstring GetExecutableDirectory();
    static std::wstring GetExecutablePath();
    static std::wstring Utf8ToWide(const std::string& value);
    static std::string WideToUtf8(const std::wstring& value);
    static std::string ReadFileBytes(const std::wstring& path);
    static bool WriteFileBytesAtomically(const std::wstring& path, const std::string& bytes);
    static bool ParseHotkeyString(const std::wstring& hotkey, UINT& modifiers, UINT& vk_code);
    static std::wstring TrimCopy(const std::wstring& value);
    static bool IsModifierVirtualKey(UINT vk_code);
    static std::wstring HotkeyKeyName(UINT vk_code);
    bool TriggersEqual(const std::wstring& left, const std::wstring& right) const;
    std::wstring NormalizeTriggerForCompare(const std::wstring& trigger) const;
    std::wstring BuildHotkeyString(UINT modifiers, UINT vk_code) const;

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;

    HFONT ui_font_ = nullptr;
    HFONT popup_menu_font_ = nullptr;
    HFONT title_font_ = nullptr;
    HFONT section_font_ = nullptr;
    HBRUSH background_brush_ = nullptr;
    HBRUSH surface_brush_ = nullptr;
    HBRUSH input_brush_ = nullptr;
    COLORREF background_color_ = RGB(245, 247, 251);
    COLORREF surface_color_ = RGB(230, 235, 245);
    COLORREF input_color_ = RGB(255, 255, 255);
    COLORREF text_color_ = RGB(15, 23, 42);
    COLORREF muted_text_color_ = RGB(71, 85, 105);
    COLORREF border_color_ = RGB(203, 213, 225);
    COLORREF header_color_ = RGB(241, 245, 249);
    COLORREF accent_color_ = RGB(37, 99, 235);
    COLORREF selection_color_ = RGB(219, 234, 254);
    COLORREF selection_text_color_ = RGB(15, 23, 42);

    HWND title_label_ = nullptr;
    HWND subtitle_label_ = nullptr;
    HWND engine_button_ = nullptr;
    HWND theme_button_ = nullptr;
    HWND import_button_ = nullptr;
    HWND export_button_ = nullptr;
    HWND github_button_ = nullptr;
    HWND info_button_ = nullptr;
    HWND restore_label_ = nullptr;
    HWND restore_edit_ = nullptr;
    HWND always_on_top_checkbox_ = nullptr;
    HWND start_with_windows_checkbox_ = nullptr;
    HWND minimize_to_tray_checkbox_ = nullptr;
    HWND previous_clipboard_checkbox_ = nullptr;
    HWND previous_clipboard_slash_checkbox_ = nullptr;
    HWND hotkey_label_ = nullptr;
    HWND record_hotkey_button_ = nullptr;

    HWND groups_title_ = nullptr;
    HWND new_group_button_ = nullptr;
    HWND rename_group_button_ = nullptr;
    HWND toggle_group_button_ = nullptr;
    HWND delete_group_button_ = nullptr;
    HWND groups_list_ = nullptr;
    HWND test_title_ = nullptr;
    HWND test_edit_ = nullptr;
    HWND test_reset_button_ = nullptr;

    HWND snippets_title_ = nullptr;
    HWND search_edit_ = nullptr;
    HWND snippets_list_ = nullptr;
    HWND new_snippet_button_ = nullptr;
    HWND duplicate_button_ = nullptr;
    HWND toggle_snippet_button_ = nullptr;
    HWND delete_button_ = nullptr;
    HWND snippets_new_button_ = nullptr;

    HWND editor_title_ = nullptr;
    HWND editor_snippet_tab_button_ = nullptr;
    HWND editor_engine_tab_button_ = nullptr;
    HWND editor_hint_label_ = nullptr;
    HWND trigger_label_ = nullptr;
    HWND trigger_edit_ = nullptr;
    HWND group_label_ = nullptr;
    HWND group_combo_ = nullptr;
    HWND snippet_enabled_checkbox_ = nullptr;
    HWND notes_label_ = nullptr;
    HWND notes_edit_ = nullptr;
    HWND content_label_ = nullptr;
    HWND content_edit_ = nullptr;
    HWND editor_new_button_ = nullptr;
    HWND save_button_ = nullptr;
    HWND reset_button_ = nullptr;
    HWND engine_settings_title_ = nullptr;
    HWND case_sensitive_checkbox_ = nullptr;
    HWND trigger_mode_label_ = nullptr;
    HWND instant_mode_radio_ = nullptr;
    HWND separator_mode_radio_ = nullptr;
    HWND separator_keys_label_ = nullptr;
    HWND separator_space_checkbox_ = nullptr;
    HWND separator_enter_checkbox_ = nullptr;
    HWND separator_tab_checkbox_ = nullptr;
    HWND settings_save_button_ = nullptr;
    HWND settings_reset_button_ = nullptr;
    HWND editor_settings_scrollbar_ = nullptr;

    HWND status_label_ = nullptr;

    std::wstring data_file_path_;
    std::wstring pending_status_text_;
    Settings settings_;
    std::vector<Group> groups_;
    std::vector<Snippet> snippets_;
    std::vector<int> filtered_snippet_indices_;
    int selected_snippet_index_ = -1;
    int editing_snippet_index_ = -1;
    int selected_group_index_ = 0;
    bool loaded_data_from_file_ = false;
    bool engine_enabled_ = true;
    bool updating_test_area_ = false;
    bool editor_new_mode_ = false;
    bool editor_dirty_ = false;
    bool engine_settings_tab_active_ = false;
    bool suppress_editor_change_tracking_ = false;
    bool suppress_snippet_list_notifications_ = false;
    bool search_placeholder_active_ = true;
    bool suppress_search_placeholder_updates_ = false;
    bool is_capturing_hotkey_ = false;
    int snippet_sort_column_ = 0;
    bool snippet_sort_ascending_ = true;
    HHOOK keyboard_hook_ = nullptr;
    UINT registered_hotkey_modifiers_ = MOD_CONTROL | MOD_SHIFT;
    UINT registered_hotkey_vk_ = VK_F12;
    HICON window_icon_big_ = nullptr;
    HICON window_icon_small_ = nullptr;
    HICON tray_icon_handle_ = nullptr;
    HICON tray_icon_paused_handle_ = nullptr;
    Gdiplus::Bitmap* github_button_bitmap_ = nullptr;
    Gdiplus::Bitmap* info_button_bitmap_ = nullptr;
    ULONG_PTR gdiplus_token_ = 0;
    bool owns_window_icons_ = false;
    bool owns_tray_icon_ = false;
    bool owns_tray_pause_icon_ = false;
    NOTIFYICONDATAW tray_icon_data_{};
    bool tray_icon_added_ = false;
    bool is_minimized_to_tray_ = false;
    RECT left_vertical_splitter_rect_{};
    RECT right_vertical_splitter_rect_{};
    RECT left_horizontal_splitter_rect_{};
    RECT groups_card_rect_{};
    RECT test_card_rect_{};
    RECT snippets_card_rect_{};
    RECT editor_card_rect_{};
    RECT engine_settings_card_rect_{};
    int window_scroll_offset_ = 0;
    int window_scroll_max_ = 0;
    int editor_settings_scroll_offset_ = 0;
    int editor_settings_scroll_max_ = 0;
    bool compact_layout_ = false;
    ActiveSplitter active_splitter_ = ActiveSplitter::None;
    std::wstring typed_buffer_;
    std::wstring latest_clipboard_text_;
    std::wstring previous_clipboard_text_;
    HWND active_context_edit_control_ = nullptr;
    HWND hovered_interactive_control_ = nullptr;
    std::atomic<bool> suppress_clipboard_history_updates_{false};
    std::atomic<bool> expansion_in_progress_{false};
    HACCEL accelerators_ = nullptr;
};
