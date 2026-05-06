import json
import os
import shutil
import threading
import time
import tempfile
import uuid
import subprocess
from datetime import datetime
import ctypes
from ctypes import wintypes
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import webbrowser
from dataclasses import dataclass, field
from typing import Callable, Optional

try:
    import keyboard
except Exception:
    keyboard = None

try:
    import pyperclip
except Exception:
    pyperclip = None

try:
    import pystray
    from PIL import Image, ImageDraw, ImageTk
except Exception:
    pystray = None
    Image = None
    ImageDraw = None
    ImageTk = None

try:
    import winreg
except Exception:
    winreg = None


APP_TITLE = "BlinkText"
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CPP_DIR = os.path.dirname(BASE_DIR)
CPP_SRC_DIR = os.path.join(CPP_DIR, "src")
CPP_ASSET_DIR = os.path.join(CPP_SRC_DIR, "assets")
CONFIG_FILE = os.path.join(BASE_DIR, "BlinkText_Snippets.json")
ASSET_DIRS = [
    BASE_DIR,
    os.path.join(BASE_DIR, "assets"),
    CPP_ASSET_DIR,
    CPP_SRC_DIR,
    CPP_DIR,
]
WINDOW_ICON_CANDIDATES = ("app.ico", "icon.ico", "app.png", "icon.png")
TRAY_ICON_CANDIDATES = ("app.png", "icon.png", "app.ico", "icon.ico")
PAUSE_TRAY_ICON_CANDIDATES = ("app_pause.ico", "app_pause.png")
GITHUB_IMAGE_CANDIDATES = ("github.png",)
INFO_IMAGE_CANDIDATES = ("info.png",)
GITHUB_URL = "https://github.com/LeaDer-E/BlinkText"
PREVIOUS_CLIPBOARD_TRIGGER = "\\\\"
PREVIOUS_CLIPBOARD_SLASH_TRIGGER = "//"
WIKI_URL = "https://github.com/LeaDer-E/BlinkText/wiki"
VARIABLES_WIKI_URL = "https://github.com/LeaDer-E/BlinkText/wiki/Variables"
ABOUT_TEXT = (
    "BlinkText v1.1.0\n"
    "Fast, local text expansion tool designed for instant, reliable typing with zero delay.\n\n"
    "Key Features\n"
    "- Instant expansion without pastes conflicts\n"
    "- No trigger duplication during rapid input (e.g., Ctrl+V after trigger)\n"
    "- Fully offline - no data collection\n"
    "- Lightweight and optimized for speed\n"
    "- Supports importing exported triggers from compatible tools\n\n"
    "Compatibility\n"
    "- Supports importing exported triggers from Beeftext for seamless migration\n\n"
    "Privacy\n"
    "- No user data is collected, stored, or transmitted\n\n"
    "Developer\n"
    "- Developed by: Eslam Mustafa\n"
    "- Contact: Eslam.Youssef@protonmail.com / Eslam.G.Youssef@gmail.com\n"
    "- GitHub: https://github.com/LeaDer-E"
)

# Fallback map for Arabic keyboard layouts when the low-level hook reports
# the physical Latin key name instead of the real typed Arabic character.
# Based on the common Windows Arabic 101 layout.
ARABIC_101_FALLBACK = {
    "q": "ض", "w": "ص", "e": "ث", "r": "ق", "t": "ف", "y": "غ", "u": "ع", "i": "ه", "o": "خ", "p": "ح",
    "[": "ج", "]": "د", "a": "ش", "s": "س", "d": "ي", "f": "ب", "g": "ل", "h": "ا", "j": "ت",
    "k": "ن", "l": "م", ";": "ك", "'": "ط", "z": "ئ", "x": "ء", "c": "ؤ", "v": "ر", "b": "لا",
    "n": "ى", "m": "ة", ",": "و", ".": "ز", "/": "ظ", "`": "ذ", "-": "-", "=": "=", "\\": "\\",
}


DEFAULT_CONFIG = {
    "settings": {
        "theme": "dark",
        "always_on_top": False,
        "restore_clipboard_delay_ms": 60,
        "match_case_sensitive": False,
        "trigger_mode": "instant",
        "word_separators": ["space", "enter", "tab"],
        "hotkey": "ctrl+shift+f12",
        "engine_enabled": True,
        "instant_settle_ms": 10,
        "backspace_delay_ms": 1,
        "start_with_windows": False,
        "minimize_to_tray": True,
        "use_previous_clipboard_trigger": False,
        "use_previous_clipboard_slash_trigger": False,
        "last_io_directory": "",
    },
    "groups": [
        {"name": "General", "enabled": True},
        {"name": "Work", "enabled": True},
    ],
    "snippets": [
        {
            "trigger": "sig\\",
            "content": "Best regards,\nYour Name\nYour Phone\nYour Email",
            "group": "General",
            "notes": "Signature sample",
            "enabled": True,
        },
        {
            "trigger": "addr\\",
            "content": "Alexandria, Egypt - Full address goes here",
            "group": "Work",
            "notes": "Address sample",
            "enabled": True,
        },
    ],
}


THEMES = {
    "dark": {
        "bg": "#071426",
        "header": "#020817",
        "panel": "#0B1A31",
        "panel_2": "#0F2140",
        "text": "#F8FAFC",
        "muted": "#B4C0D4",
        "entry_bg": "#09111F",
        "entry_fg": "#F8FAFC",
        "border": "#5E7398",
        "border_soft": "#3B4B68",
        "accent": "#22C55E",
        "accent_off": "#991B1B",
        "button": "#1A2B47",
        "button_hover": "#22395E",
        "button_text": "#FFFFFF",
        "select": "#275DF5",
        "list_bg": "#08111F",
        "status": "#07101D",
        "danger": "#8F1D1D",
        "toggle_on": "#22C55E",
        "toggle_off": "#334155",
    },
    "light": {
        "bg": "#EEF3FA",
        "header": "#FFFFFF",
        "panel": "#F8FBFF",
        "panel_2": "#EDF3FB",
        "text": "#0F172A",
        "muted": "#475569",
        "entry_bg": "#FFFFFF",
        "entry_fg": "#0F172A",
        "border": "#7C93B5",
        "border_soft": "#B7C7DE",
        "accent": "#16A34A",
        "accent_off": "#991B1B",
        "button": "#D9E4F3",
        "button_hover": "#C9D8EC",
        "button_text": "#0F172A",
        "select": "#C6D9FF",
        "list_bg": "#FFFFFF",
        "status": "#E7EEF8",
        "danger": "#8F1D1D",
        "toggle_on": "#0F172A",
        "toggle_off": "#D9E4F3",
    },
}


UNICODE_CONTROL_ITEMS = [
    ("LRM", "\u200E", "Left-to-right mark"),
    ("RLM", "\u200F", "Right-to-left mark"),
    ("ZWNJ", "\u200C", "Zero-width non-joiner"),
    ("ZWJ", "\u200D", "Zero-width joiner"),
    ("LRI", "\u2066", "Left-to-right isolate"),
    ("RLI", "\u2067", "Right-to-left isolate"),
    ("FSI", "\u2068", "First strong isolate"),
    ("PDI", "\u2069", "Pop directional isolate"),
]

UNICODE_CONTROL_MAP = {char: (label, description) for label, char, description in UNICODE_CONTROL_ITEMS}


def deep_copy(data):
    return json.loads(json.dumps(data))


def now_iso_ms():
    return datetime.now().isoformat(timespec="milliseconds")


ACTION_TEXT = "text"
ACTION_KEY = "key"
ACTION_SHORTCUT = "shortcut"
ACTION_DELAY = "delay"
ACTION_CURSOR = "cursor"
EXPANSION_COMBO_MAX_DEPTH = 5

TEXT_AFTER_PASTE_DELAY_MS = 60
BEFORE_KEY_DELAY_MS = 25
AFTER_KEY_DELAY_MS = 45
BETWEEN_ACTIONS_DELAY_MS = 35
CURSOR_SETTLE_DELAY_MS = 55
CURSOR_STEP_DELAY_MS = 12

MOD_ALT = 0x0001
MOD_CONTROL = 0x0002
MOD_SHIFT = 0x0004
MOD_WIN = 0x0008

VK_BACK = 0x08
VK_TAB = 0x09
VK_RETURN = 0x0D
VK_SHIFT = 0x10
VK_CONTROL = 0x11
VK_MENU = 0x12
VK_PAUSE = 0x13
VK_ESCAPE = 0x1B
VK_SPACE = 0x20
VK_PRIOR = 0x21
VK_NEXT = 0x22
VK_END = 0x23
VK_HOME = 0x24
VK_LEFT = 0x25
VK_UP = 0x26
VK_RIGHT = 0x27
VK_DOWN = 0x28
VK_INSERT = 0x2D
VK_DELETE = 0x2E
VK_LWIN = 0x5B
VK_RWIN = 0x5C
VK_NUMLOCK = 0x90
VK_SNAPSHOT = 0x2C
VK_VOLUME_MUTE = 0xAD
VK_VOLUME_DOWN = 0xAE
VK_VOLUME_UP = 0xAF
VK_MEDIA_NEXT_TRACK = 0xB0
VK_MEDIA_PREV_TRACK = 0xB1
VK_MEDIA_STOP = 0xB2
VK_MEDIA_PLAY_PAUSE = 0xB3
VK_LAUNCH_MEDIA_SELECT = 0xB5
VK_F1 = 0x70

if os.name == "nt":
    ULONG_PTR = wintypes.WPARAM

    class KEYBDINPUT(ctypes.Structure):
        _fields_ = [
            ("wVk", wintypes.WORD),
            ("wScan", wintypes.WORD),
            ("dwFlags", wintypes.DWORD),
            ("time", wintypes.DWORD),
            ("dwExtraInfo", ULONG_PTR),
        ]

    class _INPUTUNION(ctypes.Union):
        _fields_ = [("ki", KEYBDINPUT)]

    class INPUT(ctypes.Structure):
        _anonymous_ = ("union",)
        _fields_ = [("type", wintypes.DWORD), ("union", _INPUTUNION)]

    INPUT_KEYBOARD = 1
    KEYEVENTF_KEYUP = 0x0002
else:
    INPUT = None
    INPUT_KEYBOARD = 1
    KEYEVENTF_KEYUP = 0x0002


@dataclass
class ExpansionAction:
    type: str
    text: str = ""
    modifiers: int = 0
    vk_code: int = 0
    delay_ms: int = 0
    use_original_clipboard_paste: bool = False


@dataclass
class ExpansionPlan:
    actions: list[ExpansionAction] = field(default_factory=list)
    cursor_relative_move: int = 0
    cursor_relative_move_external: int = 0
    aborted: bool = False


@dataclass
class ExpansionResolveContext:
    clipboard_text: str = ""
    previous_clipboard_text: str = ""
    recursion_depth: int = 0
    aborted: bool = False


def trim_copy(value: str) -> str:
    return str(value or "").strip()


def unescape_variable_text(value: str) -> str:
    text = str(value or "")
    result = []
    index = 0
    while index < len(text):
        ch = text[index]
        if ch == "\\" and index + 1 < len(text):
            nxt = text[index + 1]
            if nxt in {"\\", "}"}:
                result.append(nxt)
                index += 2
                continue
        result.append(ch)
        index += 1
    return "".join(result)


def is_digits_only(value: str) -> bool:
    text = trim_copy(value)
    return bool(text) and text.isdigit()


def try_format_datetime_text(format_text: str) -> str:
    now = datetime.now()
    template = str(format_text or "").strip() or "yyyy-MM-dd HH:mm:ss"
    replacements = [
        ("yyyy", f"{now.year:04d}"),
        ("MM", f"{now.month:02d}"),
        ("dd", f"{now.day:02d}"),
        ("HH", f"{now.hour:02d}"),
        ("mm", f"{now.minute:02d}"),
        ("ss", f"{now.second:02d}"),
    ]
    placeholders = {}
    output = template
    for idx, (token, replacement) in enumerate(replacements):
        placeholder = f"\uFFF0{idx}\uFFF1"
        placeholders[placeholder] = replacement
        output = output.replace(token, placeholder)
    for placeholder, replacement in placeholders.items():
        output = output.replace(placeholder, replacement)
    return output


def map_unicode_case(value: str, mode: str) -> str:
    if mode == "upper":
        return str(value or "").upper()
    if mode == "lower":
        return str(value or "").lower()
    return str(value or "")


def count_visible_units_up_to(value: str, raw_limit: int) -> int:
    raw_limit = max(0, min(raw_limit, len(value)))
    visible_units = 0
    index = 0
    while index < raw_limit:
        if value[index] == "\r" and index + 1 < raw_limit and value[index + 1] == "\n":
            visible_units += 1
            index += 2
            continue
        visible_units += 1
        index += 1
    return visible_units


def vk_to_key_name(vk_code: int) -> str:
    if ord("A") <= vk_code <= ord("Z"):
        return chr(vk_code).lower()
    if ord("0") <= vk_code <= ord("9"):
        return chr(vk_code)
    if VK_F1 <= vk_code <= VK_F1 + 23:
        return f"f{vk_code - VK_F1 + 1}"
    mapping = {
        VK_SPACE: "space",
        VK_TAB: "tab",
        VK_RETURN: "enter",
        VK_INSERT: "insert",
        VK_DELETE: "delete",
        VK_HOME: "home",
        VK_END: "end",
        VK_PRIOR: "pageup",
        VK_NEXT: "pagedown",
        VK_UP: "up",
        VK_DOWN: "down",
        VK_LEFT: "left",
        VK_RIGHT: "right",
        VK_ESCAPE: "escape",
        VK_SNAPSHOT: "printscreen",
        VK_PAUSE: "pause",
        VK_NUMLOCK: "numlock",
        VK_VOLUME_MUTE: "volumemute",
        VK_VOLUME_UP: "volumeup",
        VK_VOLUME_DOWN: "volumedown",
        VK_MEDIA_NEXT_TRACK: "medianexttrack",
        VK_MEDIA_PREV_TRACK: "mediaprevioustrack",
        VK_MEDIA_STOP: "mediastop",
        VK_MEDIA_PLAY_PAUSE: "mediaplaypause",
        VK_LAUNCH_MEDIA_SELECT: "mediaselect",
        VK_LWIN: "windows",
        VK_CONTROL: "control",
        VK_MENU: "alt",
        VK_SHIFT: "shift",
        VK_BACK: "backspace",
    }
    return mapping.get(vk_code, "")


def build_captured_hotkey_string(modifiers: int, vk_code: int) -> str:
    key_name = vk_to_key_name(vk_code)
    if not key_name:
        return ""
    label = key_name[0].upper() + key_name[1:] if key_name else ""
    parts = []
    if modifiers & MOD_CONTROL:
        parts.append("Ctrl")
    if modifiers & MOD_SHIFT:
        parts.append("Shift")
    if modifiers & MOD_ALT:
        parts.append("Alt")
    if modifiers & MOD_WIN:
        parts.append("Win")
    parts.append(label)
    return "+".join(parts)


def parse_hotkey_string(hotkey: str):
    modifiers = 0
    vk_code = 0
    normalized = trim_copy(hotkey).lower()
    if not normalized:
        return 0, 0
    parts = [trim_copy(part) for part in normalized.split("+")]
    if not all(parts):
        return 0, 0
    for token in parts:
        if token in {"ctrl", "control"}:
            modifiers |= MOD_CONTROL
            continue
        if token == "shift":
            modifiers |= MOD_SHIFT
            continue
        if token == "alt":
            modifiers |= MOD_ALT
            continue
        if token in {"win", "windows", "cmd", "meta"}:
            modifiers |= MOD_WIN
            continue
        if vk_code:
            return 0, 0
        parsed = parse_variable_key_string(token)
        if not parsed:
            return 0, 0
        vk_code = parsed
    return modifiers, vk_code if vk_code else 0


def parse_variable_key_string(key_name: str) -> int:
    token = trim_copy(key_name).lower()
    if not token:
        return 0
    if len(token) == 1:
        ch = token[0]
        if "a" <= ch <= "z":
            return ord(ch.upper())
        if "0" <= ch <= "9":
            return ord(ch)
        return 0
    if token.startswith("f") and token[1:].isdigit():
        number = int(token[1:])
        if 1 <= number <= 24:
            return VK_F1 + number - 1
        return 0
    aliases = {
        "space": VK_SPACE,
        "tab": VK_TAB,
        "enter": VK_RETURN,
        "return": VK_RETURN,
        "insert": VK_INSERT,
        "ins": VK_INSERT,
        "delete": VK_DELETE,
        "del": VK_DELETE,
        "home": VK_HOME,
        "end": VK_END,
        "pageup": VK_PRIOR,
        "pgup": VK_PRIOR,
        "pagedown": VK_NEXT,
        "pgdn": VK_NEXT,
        "up": VK_UP,
        "down": VK_DOWN,
        "left": VK_LEFT,
        "right": VK_RIGHT,
        "esc": VK_ESCAPE,
        "escape": VK_ESCAPE,
        "printscreen": VK_SNAPSHOT,
        "prtsc": VK_SNAPSHOT,
        "pause": VK_PAUSE,
        "numlock": VK_NUMLOCK,
        "volumemute": VK_VOLUME_MUTE,
        "volumeup": VK_VOLUME_UP,
        "volumedown": VK_VOLUME_DOWN,
        "medianexttrack": VK_MEDIA_NEXT_TRACK,
        "mediaprevioustrack": VK_MEDIA_PREV_TRACK,
        "mediastop": VK_MEDIA_STOP,
        "mediaplaypause": VK_MEDIA_PLAY_PAUSE,
        "mediaselect": VK_LAUNCH_MEDIA_SELECT,
        "windows": VK_LWIN,
        "win": VK_LWIN,
        "control": VK_CONTROL,
        "ctrl": VK_CONTROL,
        "alt": VK_MENU,
        "shift": VK_SHIFT,
        "backspace": VK_BACK,
        "bksp": VK_BACK,
        "bs": VK_BACK,
    }
    return aliases.get(token, 0)


def _send_input_records(*records) -> bool:
    if os.name != "nt" or INPUT is None or not records:
        return False
    array_type = INPUT * len(records)
    result = ctypes.windll.user32.SendInput(len(records), array_type(*records), ctypes.sizeof(INPUT))
    return result == len(records)


def _keyboard_input(vk_code: int, flags: int = 0) -> INPUT:
    return INPUT(type=INPUT_KEYBOARD, ki=KEYBDINPUT(wVk=vk_code, wScan=0, dwFlags=flags, time=0, dwExtraInfo=0))


def send_virtual_key(vk_code: int):
    if not vk_code or os.name != "nt":
        return
    _send_input_records(
        _keyboard_input(vk_code, 0),
        _keyboard_input(vk_code, KEYEVENTF_KEYUP),
    )


def release_modifier_keys():
    if os.name != "nt":
        return
    for key in (VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN):
        _send_input_records(_keyboard_input(key, KEYEVENTF_KEYUP))


def send_shortcut(modifiers: int, vk_code: int):
    if not vk_code or os.name != "nt":
        return
    down = []
    up = []
    if modifiers & MOD_CONTROL:
        down.append(_keyboard_input(VK_CONTROL, 0))
        up.insert(0, _keyboard_input(VK_CONTROL, KEYEVENTF_KEYUP))
    if modifiers & MOD_SHIFT:
        down.append(_keyboard_input(VK_SHIFT, 0))
        up.insert(0, _keyboard_input(VK_SHIFT, KEYEVENTF_KEYUP))
    if modifiers & MOD_ALT:
        down.append(_keyboard_input(VK_MENU, 0))
        up.insert(0, _keyboard_input(VK_MENU, KEYEVENTF_KEYUP))
    if modifiers & MOD_WIN:
        down.append(_keyboard_input(VK_LWIN, 0))
        up.insert(0, _keyboard_input(VK_LWIN, KEYEVENTF_KEYUP))
    middle = [_keyboard_input(vk_code, 0), _keyboard_input(vk_code, KEYEVENTF_KEYUP)]
    _send_input_records(*(down + middle + up))


def get_foreground_window_handle() -> int:
    if os.name != "nt":
        return 0
    try:
        return int(ctypes.windll.user32.GetForegroundWindow() or 0)
    except Exception:
        return 0


def restore_foreground_window(hwnd: int):
    if os.name != "nt" or not hwnd:
        return
    try:
        SW_RESTORE = 9
        ctypes.windll.user32.ShowWindow(hwnd, SW_RESTORE)
        ctypes.windll.user32.SetForegroundWindow(hwnd)
    except Exception:
        pass


def foreground_matches_window(hwnd: int) -> bool:
    if os.name != "nt" or not hwnd:
        return False
    try:
        current = int(ctypes.windll.user32.GetForegroundWindow() or 0)
        return bool(current and current == int(hwnd))
    except Exception:
        return False


class ConfigManager:
    def __init__(self, path: str):
        self.path = path
        self.last_load_warning = ""
        self.data = self.load()

    def _write_json_atomic(self, path: str, payload: dict, *, indent: int, make_backup: bool):
        folder = os.path.dirname(path) or "."
        os.makedirs(folder, exist_ok=True)
        fd, temp_path = tempfile.mkstemp(prefix="blinktext_", suffix=".tmp", dir=folder)
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as file:
                json.dump(payload, file, ensure_ascii=False, indent=indent)
                file.flush()
                try:
                    if os.name != "nt":
                        os.fsync(file.fileno())
                except Exception:
                    pass
            if make_backup and os.path.exists(path):
                shutil.copy2(path, f"{path}.bak")
            os.replace(temp_path, path)
        except Exception:
            try:
                os.remove(temp_path)
            except Exception:
                pass
            raise

    def _backup_corrupt_config(self):
        if not os.path.exists(self.path):
            return ""
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_path = f"{self.path}.corrupt_{stamp}"
        try:
            shutil.copy2(self.path, backup_path)
            return backup_path
        except Exception:
            return ""

    def _normalize_data(self, data: dict):
        if not isinstance(data, dict):
            data = deep_copy(DEFAULT_CONFIG)

        data.setdefault("settings", {})
        data.setdefault("groups", [])
        data.setdefault("snippets", [])

        settings = data["settings"]
        for key, value in DEFAULT_CONFIG["settings"].items():
            settings.setdefault(key, value)

        groups = []
        seen_groups = set()
        for group in data.get("groups", []):
            name = str(group.get("name", "")).strip()
            if not name or name in seen_groups:
                continue
            groups.append({"name": name, "enabled": bool(group.get("enabled", True))})
            seen_groups.add(name)

        for group in DEFAULT_CONFIG["groups"]:
            if group["name"] not in seen_groups:
                groups.append(deep_copy(group))
                seen_groups.add(group["name"])

        snippets = []
        for item in data.get("snippets", []):
            trigger = str(item.get("trigger") or item.get("shortcut") or item.get("keyword") or "").strip()
            if not trigger:
                continue
            group_name = str(item.get("group", "General") or "General").strip()
            if group_name not in seen_groups:
                groups.append({"name": group_name, "enabled": True})
                seen_groups.add(group_name)
            snippets.append(
                {
                    "trigger": trigger,
                    "content": str(item.get("content") or item.get("snippet") or ""),
                    "group": group_name,
                    "notes": str(item.get("notes") or item.get("description") or item.get("name") or ""),
                    "enabled": bool(item.get("enabled", True)),
                }
            )

        data["groups"] = groups
        data["snippets"] = snippets
        return data

    def load(self):
        self.last_load_warning = ""
        if not os.path.exists(self.path):
            data = deep_copy(DEFAULT_CONFIG)
            self.save(data)
            return data
        try:
            with open(self.path, "r", encoding="utf-8") as file:
                raw = json.load(file)
        except Exception:
            backup_path = self._backup_corrupt_config()
            raw = deep_copy(DEFAULT_CONFIG)
            self.save(raw)
            if backup_path:
                self.last_load_warning = (
                    "Configuration could not be loaded. "
                    f"A backup was saved as {os.path.basename(backup_path)}."
                )
            else:
                self.last_load_warning = "Configuration could not be loaded, so defaults were restored."
        normalized = self._normalize_data(raw)
        return normalized

    def save(self, data=None):
        if data is not None:
            self.data = data
        self.data = self._normalize_data(self.data)
        self._write_json_atomic(self.path, self.data, indent=2, make_backup=True)

    def import_file(self, path: str):
        parsed = self.parse_import_file(path)
        self.data = parsed["data"]
        self.save()

    def parse_import_file(self, path: str):
        with open(path, "r", encoding="utf-8") as file:
            raw = json.load(file)
        if isinstance(raw, dict) and isinstance(raw.get("combos"), list):
            return {"is_beeftext": True, "data": self._from_beeftext(raw)}
        return {"is_beeftext": False, "data": self._normalize_data(raw)}

    def _build_snippet_conflict_key(self, snippet: dict, case_sensitive: bool):
        trigger = str(snippet.get("trigger", "") or "")
        return trigger if case_sensitive else trigger.lower()

    def count_import_conflicts(self, imported_data: dict, keep_source_groups: bool = True, target_group: str = ""):
        settings = self.data.get("settings", {})
        case_sensitive = bool(settings.get("match_case_sensitive", False))
        existing = {}
        for snippet in self.data.get("snippets", []):
            key = self._build_snippet_conflict_key(snippet, case_sensitive)
            existing[key] = snippet
        count = 0
        for snippet in imported_data.get("snippets", []):
            key = self._build_snippet_conflict_key(snippet, case_sensitive)
            if key in existing:
                count += 1
        return count

    def merge_import_data(self, imported_data: dict, keep_source_groups: bool = True, target_group: str = "", overwrite_conflicts: bool = False):
        settings = self.data.get("settings", {})
        case_sensitive = bool(settings.get("match_case_sensitive", False))
        existing_map = {
            self._build_snippet_conflict_key(snippet, case_sensitive): index
            for index, snippet in enumerate(self.data.get("snippets", []))
        }
        group_names = {group["name"] for group in self.data.get("groups", [])}
        added_count = 0
        updated_count = 0
        skipped_count = 0

        for group in imported_data.get("groups", []):
            name = str(group.get("name", "")).strip()
            if keep_source_groups and name and name not in group_names:
                self.data["groups"].append({"name": name, "enabled": bool(group.get("enabled", True))})
                group_names.add(name)

        if not keep_source_groups:
            target_group = str(target_group or "").strip() or "General"
            if target_group not in group_names:
                self.data["groups"].append({"name": target_group, "enabled": True})
                group_names.add(target_group)

        for imported in imported_data.get("snippets", []):
            snippet = deep_copy(imported)
            if not keep_source_groups:
                snippet["group"] = target_group
            else:
                snippet["group"] = str(snippet.get("group", "General") or "General").strip() or "General"
                if snippet["group"] not in group_names:
                    self.data["groups"].append({"name": snippet["group"], "enabled": True})
                    group_names.add(snippet["group"])

            key = self._build_snippet_conflict_key(snippet, case_sensitive)
            if key in existing_map:
                if overwrite_conflicts:
                    self.data["snippets"][existing_map[key]] = snippet
                    updated_count += 1
                else:
                    skipped_count += 1
            else:
                self.data["snippets"].append(snippet)
                existing_map[key] = len(self.data["snippets"]) - 1
                added_count += 1

        self.save()
        return added_count, updated_count, skipped_count

    def export_native(self, path: str, *, current_group_only: bool = False, group_name: str = ""):
        payload = deep_copy(self.data)
        if current_group_only and group_name:
            payload["snippets"] = [deep_copy(item) for item in self.data.get("snippets", []) if item.get("group", "General") == group_name]
            used_groups = {item.get("group", "General") for item in payload["snippets"]}
            payload["groups"] = [deep_copy(group) for group in self.data.get("groups", []) if group.get("name", "") in used_groups]
        self._write_json_atomic(path, payload, indent=2, make_backup=False)

    def export_beeftext(self, path: str, *, current_group_only: bool = False, group_name: str = ""):
        payload = {"combos": []}
        case_value = 1 if self.data["settings"].get("match_case_sensitive", False) else 0
        snippet_items = self.data.get("snippets", [])
        if current_group_only and group_name:
            snippet_items = [item for item in snippet_items if item.get("group", "General") == group_name]
        for item in snippet_items:
            payload["combos"].append(
                {
                    "caseSensitivity": case_value,
                    "creationDateTime": now_iso_ms(),
                    "description": item.get("notes", ""),
                    "enabled": bool(item.get("enabled", True)),
                    "keyword": item.get("trigger", ""),
                    "matchingMode": 0,
                    "modificationDateTime": now_iso_ms(),
                    "name": item.get("notes", "") or item.get("trigger", ""),
                    "snippet": item.get("content", ""),
                    "uuid": "{" + str(uuid.uuid4()) + "}",
                }
            )
        self._write_json_atomic(path, payload, indent=4, make_backup=False)

    def _from_beeftext(self, raw: dict):
        case_sensitive = False
        snippets = []
        groups = [{"name": "General", "enabled": True}]
        for combo in raw.get("combos", []):
            trigger = str(combo.get("keyword", "")).strip()
            if not trigger:
                continue
            case_sensitive = case_sensitive or bool(combo.get("caseSensitivity", 0))
            name = str(combo.get("name", "")).strip()
            description = str(combo.get("description", "")).strip()
            notes = description or name
            snippets.append(
                {
                    "trigger": trigger,
                    "content": str(combo.get("snippet", "")),
                    "group": "General",
                    "notes": notes,
                    "enabled": bool(combo.get("enabled", True)),
                }
            )
        data = deep_copy(DEFAULT_CONFIG)
        data["settings"]["match_case_sensitive"] = case_sensitive
        data["groups"] = groups
        data["snippets"] = snippets
        return self._normalize_data(data)


class TextExpanderEngine:
    def __init__(self, config_manager: ConfigManager, status_callback=None, state_callback=None):
        self.config_manager = config_manager
        self.status_callback = status_callback or (lambda _msg: None)
        self.state_callback = state_callback or (lambda _value: None)
        self.buffer = ""
        self.max_buffer = 400
        self.running = False
        self.hook_handle = None
        self.hotkey_handle = None
        self.expansion_lock = threading.Lock()
        self.last_expansion_time = 0.0
        self.last_triggered = ""
        self.suppress_until = 0.0
        self.pending_instant_trigger = ""
        self.app_focus_area = "external"  # external | app | test
        self.engine_enabled = bool(self.settings.get("engine_enabled", True))
        self.app_window_handle = 0
        self.app_process_id = os.getpid()
        self.clipboard_history_thread = None
        self.clipboard_history_stop = threading.Event()
        self.current_clipboard_text = ""
        self.previous_clipboard_text = ""
        self.clipboard_history_suspended_until = 0.0
        self.variable_input_callback: Optional[Callable[[str], tuple[bool, str]]] = None

    @property
    def settings(self):
        return self.config_manager.data["settings"]

    def set_status(self, text: str):
        self.status_callback(text)

    def _notify_state_change(self):
        try:
            self.state_callback(bool(self.engine_enabled))
        except Exception:
            pass

    def set_app_focus_area(self, area: str):
        if area not in {"external", "app", "test"}:
            area = "external"
        self.app_focus_area = area

    def set_app_window_handle(self, hwnd: int):
        try:
            resolved = int(hwnd or 0)
            if os.name == "nt" and resolved:
                try:
                    user32 = ctypes.windll.user32
                    # GA_ROOT = 2. Tk may give us a child/client handle; normalize
                    # to the actual top-level BlinkText window.
                    resolved = int(user32.GetAncestor(resolved, 2) or resolved)
                except Exception:
                    pass
            self.app_window_handle = resolved
        except Exception:
            self.app_window_handle = 0

    def set_variable_input_callback(self, callback: Optional[Callable[[str], tuple[bool, str]]]):
        self.variable_input_callback = callback

    def _foreground_belongs_to_app(self) -> bool:
        if os.name != "nt" or not self.app_window_handle:
            return self.app_focus_area in {"app", "test"}
        try:
            user32 = ctypes.windll.user32
            foreground = user32.GetForegroundWindow()
            if not foreground:
                return False
            foreground_pid = wintypes.DWORD()
            app_pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(foreground, ctypes.byref(foreground_pid))
            user32.GetWindowThreadProcessId(self.app_window_handle, ctypes.byref(app_pid))
            if app_pid.value and foreground_pid.value and foreground_pid.value != app_pid.value:
                return False
            if foreground == self.app_window_handle:
                return True
            # GA_ROOT = 2. Compare root windows so focused child controls inside
            # the Tk window still count as the app being foreground.
            foreground_root = user32.GetAncestor(foreground, 2)
            return bool(foreground_root == self.app_window_handle or user32.IsChild(self.app_window_handle, foreground))
        except Exception:
            return self.app_focus_area in {"app", "test"}

    def normalized(self, text: str) -> str:
        return text if self.settings.get("match_case_sensitive", False) else text.lower()

    def get_group_state(self):
        state = {}
        for group in self.config_manager.data.get("groups", []):
            name = group.get("name", "")
            if name:
                state[name] = bool(group.get("enabled", True))
        return state

    def get_enabled_snippets(self):
        group_state = self.get_group_state()
        items = []
        for snippet in self.config_manager.data.get("snippets", []):
            if not snippet.get("enabled", True):
                continue
            trigger = snippet.get("trigger", "")
            if not trigger:
                continue
            group_name = snippet.get("group", "General")
            if group_name in group_state and not group_state[group_name]:
                continue
            items.append(snippet)
        items.sort(key=lambda x: len(x.get("trigger", "")), reverse=True)
        return items

    def start(self):
        if self.running:
            return
        if keyboard is None:
            self.set_status("Python package 'keyboard' is required for the global engine.")
            return
        if pyperclip is None:
            self.set_status("Python package 'pyperclip' is required for clipboard-based expansion.")
            return
        self.running = True
        self.engine_enabled = bool(self.settings.get("engine_enabled", True))
        self._notify_state_change()
        self.hook_handle = keyboard.hook(self._handle_event, suppress=False)
        self._start_clipboard_history()
        hotkey_bound = self._bind_hotkey()
        self.set_status("Engine ready" if hotkey_bound else "Engine ready (hotkey unavailable)")

    def stop(self):
        self.running = False
        if self.hook_handle is not None:
            try:
                keyboard.unhook(self.hook_handle)
            except Exception:
                pass
            self.hook_handle = None
        if self.hotkey_handle is not None:
            try:
                keyboard.remove_hotkey(self.hotkey_handle)
            except Exception:
                pass
            self.hotkey_handle = None
        self._stop_clipboard_history()
        self.set_status("Engine stopped")

    def reload(self):
        self.buffer = ""
        self.pending_instant_trigger = ""
        self.app_focus_area = "external"  # external | app | test
        self.engine_enabled = bool(self.settings.get("engine_enabled", True))
        self._notify_state_change()
        hotkey_bound = self._rebind_hotkey()
        if hotkey_bound:
            self.set_status("Configuration reloaded")
        else:
            self.set_status(f"Configuration reloaded, but hotkey failed: {self.settings.get('hotkey', '')}")
        return hotkey_bound

    def _bind_hotkey(self):
        if keyboard is None:
            self.hotkey_handle = None
            return False
        hotkey = str(self.settings.get("hotkey", DEFAULT_CONFIG["settings"]["hotkey"]) or "").strip().lower()
        if not hotkey:
            hotkey = DEFAULT_CONFIG["settings"]["hotkey"]
        self.settings["hotkey"] = hotkey
        try:
            self.hotkey_handle = keyboard.add_hotkey(hotkey, self.toggle_enabled, suppress=False, trigger_on_release=False)
            return True
        except Exception:
            self.hotkey_handle = None
            self.set_status(f"Could not bind hotkey: {hotkey}")
            return False

    def _rebind_hotkey(self):
        if self.hotkey_handle is not None:
            try:
                keyboard.remove_hotkey(self.hotkey_handle)
            except Exception:
                pass
            self.hotkey_handle = None
        return self._bind_hotkey()

    def set_enabled(self, value: bool):
        self.engine_enabled = bool(value)
        self.settings["engine_enabled"] = self.engine_enabled
        self.config_manager.save()
        self._notify_state_change()
        self.set_status("Engine enabled" if self.engine_enabled else "Engine disabled")

    def toggle_enabled(self):
        self.engine_enabled = not self.engine_enabled
        self.settings["engine_enabled"] = self.engine_enabled
        try:
            self.config_manager.save()
        except Exception:
            pass
        self._notify_state_change()
        self.set_status("Engine enabled" if self.engine_enabled else "Engine disabled")
        return self.engine_enabled

    def _allowed_separator(self, event_name: str) -> bool:
        configured = set(self.settings.get("word_separators", ["space", "enter", "tab"]))
        return event_name in configured

    def _find_exact_match(self, current_word: str):
        normalized_word = self.normalized(current_word)
        for snippet in self.get_enabled_snippets():
            if self.normalized(snippet.get("trigger", "")) == normalized_word:
                return snippet
        return None

    def _find_suffix_match(self, current_buffer: str):
        normalized_buffer = self.normalized(current_buffer)
        for snippet in self.get_enabled_snippets():
            trigger = snippet.get("trigger", "")
            if trigger and normalized_buffer.endswith(self.normalized(trigger)):
                return snippet
        return None

    def _start_clipboard_history(self):
        self.clipboard_history_stop.clear()
        if self.clipboard_history_thread and self.clipboard_history_thread.is_alive():
            return
        self.current_clipboard_text = ""
        self.previous_clipboard_text = ""
        self.clipboard_history_thread = threading.Thread(target=self._clipboard_history_loop, daemon=True)
        self.clipboard_history_thread.start()

    def _stop_clipboard_history(self):
        self.clipboard_history_stop.set()
        self.clipboard_history_thread = None

    def suspend_clipboard_history(self, seconds: float = 0.8):
        self.clipboard_history_suspended_until = max(self.clipboard_history_suspended_until, time.time() + max(0.0, seconds))

    def _clipboard_history_loop(self):
        while not self.clipboard_history_stop.is_set():
            try:
                if time.time() >= self.clipboard_history_suspended_until:
                    value = self._read_clipboard()
                    if isinstance(value, str) and value and value != self.current_clipboard_text:
                        if value != self.previous_clipboard_text:
                            self.previous_clipboard_text = self.current_clipboard_text
                        self.current_clipboard_text = value
            except Exception:
                pass
            self.clipboard_history_stop.wait(0.35)

    def _get_previous_clipboard_snippet(self, trigger_text: str):
        if not self.previous_clipboard_text:
            return None
        return {
            "trigger": trigger_text,
            "content": self.previous_clipboard_text,
            "group": "General",
            "notes": "Previous clipboard item",
            "enabled": True,
        }

    def _reset_buffer_for_navigation(self, name: str):
        if name in {
            "left", "right", "up", "down", "home", "end", "page up", "page down",
            "delete", "insert", "esc", "menu",
        }:
            self.buffer = ""

    def _event_to_char(self, event):
        name = getattr(event, "name", "") or ""
        if os.name != "nt":
            return name if len(name) == 1 else ""

        try:
            scan_code = int(getattr(event, "scan_code", 0) or 0)
            if scan_code <= 0:
                return name if len(name) == 1 else ""

            user32 = ctypes.windll.user32

            # Use the keyboard layout of the foreground window thread, not the
            # Python/Tk thread. This is the main fix for Arabic triggers such as ر\
            # being read as v\ by the low-level keyboard library.
            hwnd = user32.GetForegroundWindow()
            thread_id = user32.GetWindowThreadProcessId(hwnd, None) if hwnd else 0
            layout = user32.GetKeyboardLayout(thread_id) if thread_id else user32.GetKeyboardLayout(0)
            lang_id = layout & 0xFFFF
            primary_lang_id = lang_id & 0x03FF
            is_arabic_layout = primary_lang_id == 0x01

            vk = user32.MapVirtualKeyExW(scan_code, 3, layout)
            if vk == 0:
                vk = user32.MapVirtualKeyExW(scan_code, 1, layout)
            if vk == 0:
                if is_arabic_layout and len(name) == 1:
                    return ARABIC_101_FALLBACK.get(name.lower(), name)
                return name if len(name) == 1 else ""

            keyboard_state = (ctypes.c_ubyte * 256)()
            user32.GetKeyboardState(ctypes.byref(keyboard_state))

            # Clear Ctrl/Alt for character translation. They are already handled
            # as control keys above, and leaving them set can make ToUnicodeEx fail.
            keyboard_state[0x11] = 0  # VK_CONTROL
            keyboard_state[0x12] = 0  # VK_MENU / ALT

            buf = ctypes.create_unicode_buffer(8)
            rc = user32.ToUnicodeEx(vk, scan_code, ctypes.byref(keyboard_state), buf, len(buf), 0, layout)
            if rc > 0:
                value = buf.value[:rc]
                # Defensive fallback: some systems still return the physical Latin
                # key for Arabic layouts when using global hooks.
                if is_arabic_layout and len(name) == 1 and value.lower() == name.lower():
                    return ARABIC_101_FALLBACK.get(name.lower(), value)
                return value

            if is_arabic_layout and len(name) == 1:
                return ARABIC_101_FALLBACK.get(name.lower(), name)
        except Exception:
            pass

        return name if len(name) == 1 else ""

    def _handle_event(self, event):
        if not self.running or not self.engine_enabled:
            return
        if self._foreground_belongs_to_app():
            return
        if event.event_type != "down":
            return
        if time.time() < self.suppress_until:
            return

        name = event.name
        if not name:
            return

        if name in {
            "shift", "left shift", "right shift",
            "ctrl", "left ctrl", "right ctrl",
            "alt", "left alt", "right alt",
            "alt gr", "windows", "left windows", "right windows",
            "caps lock",
        }:
            return

        if name == "backspace":
            self.buffer = self.buffer[:-1]
            return

        trigger_mode = self.settings.get("trigger_mode", "instant")

        # Separator keys must be handled before character translation.
        # Otherwise space/enter/tab can be treated as normal characters,
        # making Separator mode behave like Instant mode or not work at all.
        if name in {"space", "enter", "tab"}:
            if trigger_mode == "separator" and self._allowed_separator(name):
                current_word = self.buffer
                self.buffer = ""
                if self.settings.get("use_previous_clipboard_trigger", False) and current_word == PREVIOUS_CLIPBOARD_TRIGGER:
                    snippet = self._get_previous_clipboard_snippet(PREVIOUS_CLIPBOARD_TRIGGER)
                    if snippet:
                        threading.Thread(
                            target=self._expand_snippet,
                            args=(snippet, len(PREVIOUS_CLIPBOARD_TRIGGER) + 1, False),
                            daemon=True,
                        ).start()
                        return
                if self.settings.get("use_previous_clipboard_slash_trigger", False) and current_word == PREVIOUS_CLIPBOARD_SLASH_TRIGGER:
                    snippet = self._get_previous_clipboard_snippet(PREVIOUS_CLIPBOARD_SLASH_TRIGGER)
                    if snippet:
                        threading.Thread(
                            target=self._expand_snippet,
                            args=(snippet, len(PREVIOUS_CLIPBOARD_SLASH_TRIGGER) + 1, False),
                            daemon=True,
                        ).start()
                        return
                snippet = self._find_exact_match(current_word)
                if snippet:
                    threading.Thread(
                        target=self._expand_snippet,
                        args=(snippet, len(snippet.get("trigger", "")) + 1, False),
                        daemon=True,
                    ).start()
                    return
            else:
                self.buffer = ""
            return

        actual_char = self._event_to_char(event)
        if actual_char:
            self.buffer += actual_char
            self.buffer = self.buffer[-self.max_buffer:]
            if trigger_mode == "instant":
                if self.settings.get("use_previous_clipboard_trigger", False) and self.buffer.endswith(PREVIOUS_CLIPBOARD_TRIGGER):
                    snippet = self._get_previous_clipboard_snippet(PREVIOUS_CLIPBOARD_TRIGGER)
                    if snippet:
                        self.pending_instant_trigger = PREVIOUS_CLIPBOARD_TRIGGER
                        self.suppress_until = time.time() + 0.55
                        threading.Thread(
                            target=self._expand_snippet,
                            args=(snippet, len(PREVIOUS_CLIPBOARD_TRIGGER), True),
                            daemon=True,
                        ).start()
                        return
                if self.settings.get("use_previous_clipboard_slash_trigger", False) and self.buffer.endswith(PREVIOUS_CLIPBOARD_SLASH_TRIGGER):
                    snippet = self._get_previous_clipboard_snippet(PREVIOUS_CLIPBOARD_SLASH_TRIGGER)
                    if snippet:
                        self.pending_instant_trigger = PREVIOUS_CLIPBOARD_SLASH_TRIGGER
                        self.suppress_until = time.time() + 0.55
                        threading.Thread(
                            target=self._expand_snippet,
                            args=(snippet, len(PREVIOUS_CLIPBOARD_SLASH_TRIGGER), True),
                            daemon=True,
                        ).start()
                        return
                snippet = self._find_suffix_match(self.buffer)
                if snippet:
                    trigger = snippet.get("trigger", "")
                    if not self.pending_instant_trigger:
                        self.pending_instant_trigger = trigger
                        self.suppress_until = time.time() + 0.55
                        delete_count = len(trigger)
                        threading.Thread(
                            target=self._expand_snippet,
                            args=(snippet, delete_count, True),
                            daemon=True,
                        ).start()
            return

        self._reset_buffer_for_navigation(name)

    def _read_clipboard(self):
        if pyperclip is None:
            return ""
        try:
            return pyperclip.paste()
        except Exception:
            return ""

    def _write_clipboard(self, value: str):
        if pyperclip is None:
            return
        pyperclip.copy(value)

    def _send_backspaces(self, count: int):
        delay = max(0, int(self.settings.get("backspace_delay_ms", 1))) / 1000.0
        for _ in range(max(0, count)):
            keyboard.send("backspace")
            if delay:
                time.sleep(delay)

    def _request_input_value(self, prompt_text: str) -> tuple[bool, str]:
        if self.variable_input_callback is None:
            return False, ""
        try:
            return self.variable_input_callback(prompt_text)
        except Exception:
            return False, ""

    def _find_exact_snippet(self, trigger_text: str):
        normalized = self.normalized(trigger_text)
        for snippet in self.get_enabled_snippets():
            if self.normalized(snippet.get("trigger", "")) == normalized:
                return snippet
        return None

    def _append_text_action(self, plan: ExpansionPlan, value: str, use_original_clipboard_paste: bool = False):
        if not value:
            return
        if (
            plan.actions
            and plan.actions[-1].type == ACTION_TEXT
            and plan.actions[-1].use_original_clipboard_paste == use_original_clipboard_paste
        ):
            plan.actions[-1].text += value
            return
        plan.actions.append(
            ExpansionAction(
                type=ACTION_TEXT,
                text=value,
                use_original_clipboard_paste=use_original_clipboard_paste,
            )
        )

    def build_expansion_plan(self, snippet: dict, clipboard_text: str, previous_clipboard_text: str) -> ExpansionPlan:
        plan = ExpansionPlan()
        context = ExpansionResolveContext(
            clipboard_text=clipboard_text,
            previous_clipboard_text=previous_clipboard_text,
            recursion_depth=0,
            aborted=False,
        )
        self._append_resolved_content(str(snippet.get("content", "")), plan, context)
        plan.aborted = context.aborted
        self._finalize_expansion_plan(plan)
        return plan

    def _append_resolved_content(self, content: str, plan: ExpansionPlan, context: ExpansionResolveContext) -> bool:
        text_buffer: list[str] = []
        index = 0
        while index < len(content):
            ch = content[index]
            if ch == "\\" and index + 1 < len(content):
                nxt = content[index + 1]
                if nxt in {"\\", "}"}:
                    text_buffer.append(nxt)
                    index += 2
                    continue
            if ch == "#" and index + 1 < len(content) and content[index + 1] == "{":
                token_name_end = index + 2
                while token_name_end < len(content) and content[token_name_end] not in {":", "}"}:
                    token_name_end += 1
                token_name = content[index + 2:token_name_end]
                raw_payload_until_close = token_name in {"combo", "trim", "upper", "lower"}
                close_index = -1
                scan = index + 2
                while scan < len(content):
                    if not raw_payload_until_close and content[scan] == "\\" and scan + 1 < len(content):
                        escaped = content[scan + 1]
                        if escaped in {"\\", "}"}:
                            scan += 2
                            continue
                    if content[scan] == "}":
                        close_index = scan
                        break
                    scan += 1
                if close_index == -1:
                    text_buffer.append(ch)
                    index += 1
                    continue
                self._append_text_action(plan, "".join(text_buffer))
                text_buffer.clear()
                token_text = content[index + 2:close_index]
                self._resolve_variable_token(token_text, plan, context)
                if context.aborted:
                    return True
                index = close_index + 1
                continue
            text_buffer.append(ch)
            index += 1
        self._append_text_action(plan, "".join(text_buffer))
        return True

    def _resolve_variable_token(self, token_text: str, plan: ExpansionPlan, context: ExpansionResolveContext) -> bool:
        raw_token = "#{" + token_text + "}"
        separator = token_text.find(":")
        variable_name = token_text if separator == -1 else token_text[:separator]
        payload = "" if separator == -1 else token_text[separator + 1:]
        if not variable_name:
            self._append_text_action(plan, raw_token)
            return False

        if variable_name == "date":
            self._append_text_action(plan, try_format_datetime_text(unescape_variable_text(payload) or "yyyy-MM-dd"))
            return True
        if variable_name == "time":
            self._append_text_action(plan, try_format_datetime_text(unescape_variable_text(payload) or "HH:mm:ss"))
            return True
        if variable_name == "dateTime":
            self._append_text_action(plan, try_format_datetime_text(unescape_variable_text(payload) or "yyyy-MM-dd HH:mm:ss"))
            return True
        if variable_name == "clipboard" and separator == -1:
            self._append_text_action(plan, context.clipboard_text, use_original_clipboard_paste=True)
            return True
        if variable_name == "previousClipboard" and separator == -1:
            self._append_text_action(plan, context.previous_clipboard_text)
            return True
        if variable_name == "input":
            prompt_text = unescape_variable_text(payload) or "Input"
            accepted, value = self._request_input_value(prompt_text)
            if accepted:
                self._append_text_action(plan, value)
            else:
                context.aborted = True
            return True
        if variable_name == "key" and separator != -1 and payload:
            vk_code = parse_variable_key_string(unescape_variable_text(payload))
            if not vk_code:
                self._append_text_action(plan, raw_token)
                return False
            plan.actions.append(ExpansionAction(type=ACTION_KEY, vk_code=vk_code))
            return True
        if variable_name == "shortcut" and separator != -1 and payload:
            modifiers, vk_code = parse_hotkey_string(unescape_variable_text(payload))
            if not vk_code:
                self._append_text_action(plan, raw_token)
                return False
            plan.actions.append(ExpansionAction(type=ACTION_SHORTCUT, modifiers=modifiers, vk_code=vk_code))
            return True
        if variable_name == "delay" and separator != -1 and payload:
            delay_text = trim_copy(payload)
            if not is_digits_only(delay_text):
                self._append_text_action(plan, raw_token)
                return False
            plan.actions.append(ExpansionAction(type=ACTION_DELAY, delay_ms=int(delay_text)))
            return True
        if variable_name == "cursor" and separator == -1:
            plan.actions.append(ExpansionAction(type=ACTION_CURSOR))
            return True
        if variable_name in {"combo", "trim", "upper", "lower"} and separator != -1 and payload:
            if self._resolve_combo_variable(payload, plan, context, variable_name):
                return True
            self._append_text_action(plan, raw_token)
            return False
        if variable_name == "envVar" and separator != -1 and payload:
            env_name = unescape_variable_text(payload)
            self._append_text_action(plan, os.environ.get(env_name, ""))
            return True
        if variable_name == "powershell" and separator != -1 and payload:
            ok, output = self._resolve_powershell_variable(unescape_variable_text(payload))
            if not ok:
                self._append_text_action(plan, raw_token)
                return False
            self._append_text_action(plan, output)
            return True

        self._append_text_action(plan, raw_token)
        return False

    def _resolve_combo_variable(self, trigger: str, plan: ExpansionPlan, context: ExpansionResolveContext, transform_name: str = "") -> bool:
        if not trigger or context.recursion_depth >= EXPANSION_COMBO_MAX_DEPTH:
            return False
        snippet = self._find_exact_snippet(trigger)
        if not snippet:
            return False
        nested_context = ExpansionResolveContext(
            clipboard_text=context.clipboard_text,
            previous_clipboard_text=context.previous_clipboard_text,
            recursion_depth=context.recursion_depth + 1,
            aborted=False,
        )
        if not transform_name or transform_name == "combo":
            parsed = self._append_resolved_content(str(snippet.get("content", "")), plan, nested_context)
            context.aborted = nested_context.aborted
            return parsed

        nested_plan = ExpansionPlan()
        if not self._append_resolved_content(str(snippet.get("content", "")), nested_plan, nested_context):
            return False
        context.aborted = nested_context.aborted
        nested_plan.aborted = nested_context.aborted
        self._finalize_expansion_plan(nested_plan)
        rendered, _ = self.render_expansion_plan_for_test_area(nested_plan)
        if transform_name == "trim":
            rendered = trim_copy(rendered)
        elif transform_name == "upper":
            rendered = map_unicode_case(rendered, "upper")
        elif transform_name == "lower":
            rendered = map_unicode_case(rendered, "lower")
        self._append_text_action(plan, rendered)
        return True

    def _resolve_powershell_variable(self, payload: str) -> tuple[bool, str]:
        script_path = trim_copy(payload)
        if not script_path:
            return False, ""
        timeout_ms = 10000
        last_colon = script_path.rfind(":")
        if last_colon > 1:
            timeout_text = trim_copy(script_path[last_colon + 1:])
            if is_digits_only(timeout_text):
                timeout_ms = int(timeout_text)
                script_path = trim_copy(script_path[:last_colon])
        if len(script_path) < 4 or not script_path.lower().endswith(".ps1") or not os.path.isfile(script_path):
            return True, ""
        try:
            creation_flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
            completed = subprocess.run(
                [
                    "powershell.exe",
                    "-NoProfile",
                    "-NonInteractive",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    script_path,
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                stdin=subprocess.DEVNULL,
                timeout=None if timeout_ms == 0 else timeout_ms / 1000.0,
                creationflags=creation_flags,
                check=False,
            )
            stdout_bytes = completed.stdout or b""
            try:
                return True, stdout_bytes.decode("utf-8")
            except Exception:
                return True, stdout_bytes.decode(errors="replace")
        except subprocess.TimeoutExpired:
            return True, ""
        except Exception:
            return True, ""

    def _simulate_actions(self, actions: list[ExpansionAction]):
        rendered = ""
        caret = 0
        cursor_marker_seen = False
        cursor_marker_target = 0

        def insert_text_at_caret(value: str):
            nonlocal rendered, caret
            if not value:
                return
            rendered = rendered[:caret] + value + rendered[caret:]
            caret += len(value)

        def line_start_at(position: int) -> int:
            position = max(0, min(position, len(rendered)))
            if position == 0:
                return 0
            newline = rendered.rfind("\n", 0, position)
            return 0 if newline == -1 else newline + 1

        def line_end_at(position: int) -> int:
            position = max(0, min(position, len(rendered)))
            newline = rendered.find("\n", position)
            return len(rendered) if newline == -1 else newline

        def move_caret_vertically(move_down: bool):
            nonlocal caret
            current_line_start = line_start_at(caret)
            current_column = caret - current_line_start
            current_line_end = line_end_at(current_line_start)
            if move_down:
                if current_line_end >= len(rendered):
                    return
                next_line_start = min(len(rendered), current_line_end + 1)
                next_line_end = line_end_at(next_line_start)
                caret = min(next_line_start + current_column, next_line_end)
                return
            if current_line_start == 0:
                return
            previous_line_end = current_line_start - 1
            previous_line_start = line_start_at(previous_line_end)
            previous_line_last = line_end_at(previous_line_start)
            caret = min(previous_line_start + current_column, previous_line_last)

        for action in actions:
            if action.type == ACTION_TEXT:
                insert_text_at_caret(action.text)
            elif action.type == ACTION_KEY:
                if action.vk_code == VK_TAB:
                    insert_text_at_caret("\t")
                elif action.vk_code == VK_RETURN:
                    insert_text_at_caret("\n")
                elif action.vk_code == VK_SPACE:
                    insert_text_at_caret(" ")
                elif action.vk_code == VK_BACK:
                    if caret > 0:
                        rendered = rendered[:caret - 1] + rendered[caret:]
                        caret -= 1
                elif action.vk_code == VK_DELETE:
                    if caret < len(rendered):
                        rendered = rendered[:caret] + rendered[caret + 1:]
                elif action.vk_code == VK_LEFT:
                    caret = max(0, caret - 1)
                elif action.vk_code == VK_RIGHT:
                    caret = min(len(rendered), caret + 1)
                elif action.vk_code == VK_HOME:
                    caret = line_start_at(caret)
                elif action.vk_code == VK_END:
                    caret = line_end_at(caret)
                elif action.vk_code == VK_UP:
                    move_caret_vertically(False)
                elif action.vk_code == VK_DOWN:
                    move_caret_vertically(True)
            elif action.type == ACTION_CURSOR:
                cursor_marker_seen = True
                cursor_marker_target = caret

        return rendered, caret, cursor_marker_seen, cursor_marker_target

    def _finalize_expansion_plan(self, plan: ExpansionPlan):
        rendered, caret, cursor_marker_seen, cursor_marker_target = self._simulate_actions(plan.actions)
        if cursor_marker_seen:
            plan.cursor_relative_move = caret - cursor_marker_target
            cursor_offset = count_visible_units_up_to(rendered, cursor_marker_target)
            final_visible_length = count_visible_units_up_to(rendered, len(rendered))
            final_caret_position = count_visible_units_up_to(rendered, caret)
            plan.cursor_relative_move_external = final_caret_position - cursor_offset
        plan.actions = [action for action in plan.actions if action.type != ACTION_CURSOR]

    def render_expansion_plan_for_test_area(self, plan: ExpansionPlan) -> tuple[str, Optional[int]]:
        rendered, caret, _, _ = self._simulate_actions(plan.actions)
        if plan.cursor_relative_move:
            caret = max(0, min(len(rendered), caret - plan.cursor_relative_move))
        return rendered, caret

    def _paste_current_clipboard(self):
        if keyboard is not None:
            try:
                keyboard.send("ctrl+v")
                return
            except Exception:
                pass
        send_shortcut(MOD_CONTROL, ord("V"))

    def _send_cursor_move_key(self, direction: str):
        direction = trim_copy(direction).lower()
        if direction not in {"left", "right"}:
            return
        if keyboard is not None:
            try:
                keyboard.send(direction)
                return
            except Exception:
                pass
        send_virtual_key(VK_LEFT if direction == "left" else VK_RIGHT)

    def _execute_external_plan(self, plan: ExpansionPlan, original_clipboard: str, target_hwnd: int):
        current_clipboard = self._read_clipboard()
        clipboard_changed = False
        try:
            for action in plan.actions:
                if target_hwnd and not foreground_matches_window(target_hwnd):
                    restore_foreground_window(target_hwnd)
                    time.sleep(0.03)
                if action.type == ACTION_TEXT:
                    segment = original_clipboard if action.use_original_clipboard_paste else action.text
                    if segment:
                        if current_clipboard != segment:
                            self._write_clipboard(segment)
                            current_clipboard = segment
                            clipboard_changed = True
                            time.sleep(0.006)
                        self._paste_current_clipboard()
                        time.sleep(TEXT_AFTER_PASTE_DELAY_MS / 1000.0)
                elif action.type == ACTION_KEY:
                    release_modifier_keys()
                    time.sleep(BEFORE_KEY_DELAY_MS / 1000.0)
                    send_virtual_key(action.vk_code)
                    time.sleep(AFTER_KEY_DELAY_MS / 1000.0)
                elif action.type == ACTION_SHORTCUT:
                    release_modifier_keys()
                    time.sleep(BEFORE_KEY_DELAY_MS / 1000.0)
                    send_shortcut(action.modifiers, action.vk_code)
                    time.sleep(AFTER_KEY_DELAY_MS / 1000.0)
                elif action.type == ACTION_DELAY:
                    if action.delay_ms > 0:
                        time.sleep(action.delay_ms / 1000.0)
                time.sleep(BETWEEN_ACTIONS_DELAY_MS / 1000.0)
            if plan.cursor_relative_move_external:
                if target_hwnd and not foreground_matches_window(target_hwnd):
                    restore_foreground_window(target_hwnd)
                    time.sleep(0.03)
                time.sleep(CURSOR_SETTLE_DELAY_MS / 1000.0)
                if plan.cursor_relative_move_external > 0:
                    for _ in range(plan.cursor_relative_move_external):
                        self._send_cursor_move_key("left")
                        time.sleep(CURSOR_STEP_DELAY_MS / 1000.0)
                else:
                    for _ in range(abs(plan.cursor_relative_move_external)):
                        self._send_cursor_move_key("right")
                        time.sleep(CURSOR_STEP_DELAY_MS / 1000.0)
        finally:
            release_modifier_keys()
            try:
                if clipboard_changed or current_clipboard != original_clipboard:
                    self._write_clipboard(original_clipboard)
            except Exception:
                pass

    def _expand_snippet(self, snippet: dict, delete_count: int, from_instant: bool):
        with self.expansion_lock:
            now = time.time()
            trigger = snippet.get("trigger", "")
            if now - self.last_expansion_time < 0.12 and trigger == self.last_triggered:
                return

            self.last_expansion_time = now
            self.last_triggered = trigger
            self.buffer = ""

            instant_settle = max(1, int(self.settings.get("instant_settle_ms", 14))) / 1000.0
            original_clipboard = self._read_clipboard()
            previous_clipboard_snapshot = original_clipboard
            target_hwnd = get_foreground_window_handle()
            self.suspend_clipboard_history(2.0)
            self.suppress_until = max(self.suppress_until, time.time() + 0.42)

            try:
                if from_instant:
                    time.sleep(instant_settle)
                else:
                    time.sleep(0.012)

                self._send_backspaces(delete_count)
                time.sleep(0.008)

                plan = self.build_expansion_plan(snippet, original_clipboard, previous_clipboard_snapshot)
                if plan.aborted:
                    self.set_status("Expansion cancelled")
                    return

                if not plan.actions:
                    self.set_status(f"Expanded: {trigger}")
                    return

                self._execute_external_plan(plan, original_clipboard, target_hwnd)
                self.set_status(f"Expanded: {trigger}")
            except Exception as exc:
                self.set_status(f"Expansion error: {exc}")
                try:
                    self._write_clipboard(original_clipboard)
                except Exception:
                    pass
            finally:
                time.sleep(0.02)
                self.pending_instant_trigger = ""
                self.suppress_until = 0.0



class ThemedWidgets:
    def __init__(self):
        self.frames = []
        self.labels = []
        self.buttons = []
        self.entries = []
        self.texts = []
        self.listboxes = []
        self.checkbuttons = []
        self.radiobuttons = []
        self.optionmenus = []
        self.comboboxes = []
        self.treeviews = []
        self.canvases = []
        self.scrollbars = []
        self.menus = []


class App:
    UI_FONT = "Segoe UI"
    AR_FONT = "Tahoma"
    MONO_FONT = "Consolas"

    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title(APP_TITLE)
        self.root.geometry("1360x860")
        self.root.minsize(960, 720)

        self.config_manager = ConfigManager(CONFIG_FILE)
        self.engine = TextExpanderEngine(self.config_manager, self.update_status, self._on_engine_state_changed)
        self._local_test_buffer = ""
        self.theme_name = self.config_manager.data["settings"].get("theme", "dark")
        self.theme = THEMES[self.theme_name]
        self.widgets = ThemedWidgets()

        self.selected_group_index = 0
        self.selected_snippet_index = None
        self.editing_snippet_index = None
        self.editor_new_mode = True
        self.filtered_snippet_indices = []
        self.snippet_tree_map = {}
        self.sort_column = "trigger"
        self.sort_reverse = False

        self.search_var = tk.StringVar()
        self.trigger_var = tk.StringVar()
        self.group_var = tk.StringVar(value="General")
        self.enabled_var = tk.BooleanVar(value=True)
        self.notes_var = tk.StringVar()
        self.hotkey_var = tk.StringVar(value=str(self.config_manager.data["settings"].get("hotkey", DEFAULT_CONFIG["settings"]["hotkey"])))
        self.restore_delay_var = tk.StringVar(value=str(self.config_manager.data["settings"].get("restore_clipboard_delay_ms", 60)))
        self.always_on_top_var = tk.BooleanVar(value=bool(self.config_manager.data["settings"].get("always_on_top", False)))
        self.start_with_windows_var = tk.BooleanVar(value=bool(self.config_manager.data["settings"].get("start_with_windows", False)))
        self.minimize_to_tray_var = tk.BooleanVar(value=bool(self.config_manager.data["settings"].get("minimize_to_tray", True)))
        self.previous_clipboard_var = tk.BooleanVar(value=bool(self.config_manager.data["settings"].get("use_previous_clipboard_trigger", False)))
        self.previous_clipboard_slash_var = tk.BooleanVar(value=bool(self.config_manager.data["settings"].get("use_previous_clipboard_slash_trigger", False)))
        self.case_sensitive_var = tk.BooleanVar(value=bool(self.config_manager.data["settings"].get("match_case_sensitive", False)))
        self.mode_var = tk.StringVar(value=self.config_manager.data["settings"].get("trigger_mode", "instant"))
        self.status_var = tk.StringVar(value="Ready")
        self.test_area_hint_var = tk.StringVar(value="Type a trigger here to test it.")
        seps = self.config_manager.data["settings"].get("word_separators", ["space", "enter", "tab"])
        self.sep_space_var = tk.BooleanVar(value="space" in seps)
        self.sep_enter_var = tk.BooleanVar(value="enter" in seps)
        self.sep_tab_var = tk.BooleanVar(value="tab" in seps)
        self.tray_icon = None
        self._tray_active_image = None
        self._tray_paused_image = None
        self._closing = False
        self._minimize_handled = False
        self._app_icon_photo = None
        self._github_button_photo = None
        self._info_button_photo = None
        self._layout_refresh_job = None
        self._restoring_from_tray = False
        self._hotkey_capture_active = False
        self._hotkey_capture_parts = []
        self._hotkey_capture_pressed = set()
        self._hotkey_capture_original = self.hotkey_var.get()
        self._search_placeholder_active = False

        self._build_ui()
        self._refresh_hotkey_label()
        self._setup_window_icon()
        self._setup_toolbar_images()
        self.root.update_idletasks()
        app_hwnd = self.root.winfo_id()
        self.engine.set_app_window_handle(app_hwnd)
        self.engine.set_variable_input_callback(self._show_variable_input_dialog)
        self._bind_app_keyboard_helpers()
        self._bind_edit_context_menus()
        self.root.bind("<Unmap>", self._on_unmap, add="+")
        self.root.bind_all("<Alt-F4>", self._on_alt_f4, add="+")
        self._apply_theme()
        self._stabilize_window_chrome_on_startup()
        self._load_groups()
        self._load_snippets()
        self._schedule_layout_refresh()
        self._set_always_on_top()
        self.engine.start()
        self._show_tray_icon()
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        if self.config_manager.last_load_warning:
            self.root.after(450, lambda: self.update_status(self.config_manager.last_load_warning))

    def _register_frame(self, frame, role="inherit"):
        frame._theme_role = role
        self.widgets.frames.append(frame)
        return frame

    def _styled_frame(self, parent, role="panel", border=True):
        frame = tk.Frame(parent, bd=0, highlightthickness=1 if border else 0)
        self._register_frame(frame, role)
        return frame

    def _styled_label(self, parent, text, font=None, muted=False, role=None, anchor="w"):
        lbl = tk.Label(parent, text=text, anchor=anchor, justify="left", bd=0)
        lbl._theme_muted = muted
        lbl._theme_role = role or getattr(parent, "_theme_role", "inherit")
        lbl._theme_font = font or (self.UI_FONT, 10)
        lbl.configure(font=lbl._theme_font)
        self.widgets.labels.append(lbl)
        return lbl

    def _styled_button(self, parent, text, command, accent=False, danger=False):
        btn = tk.Button(parent, text=text, command=command, relief="flat", bd=0, cursor="hand2", padx=12, pady=8)
        btn._accent = accent
        btn._danger = danger
        btn._theme_role = getattr(parent, "_theme_role", "inherit")
        btn.configure(font=(self.UI_FONT, 10, "bold"))
        self.widgets.buttons.append(btn)
        return btn

    def _styled_entry(self, parent, textvariable=None, width=None, font=None):
        ent = tk.Entry(parent, textvariable=textvariable, relief="flat", bd=0, highlightthickness=1)
        if width:
            ent.configure(width=width)
        ent._theme_role = getattr(parent, "_theme_role", "inherit")
        ent.configure(font=font or (self.UI_FONT, 10))
        self.widgets.entries.append(ent)
        return ent

    def _styled_text(self, parent, height=4, font=None):
        txt = tk.Text(parent, height=height, wrap="word", relief="flat", bd=0, highlightthickness=1, padx=10, pady=8)
        txt._theme_role = getattr(parent, "_theme_role", "inherit")
        txt.configure(font=font or (self.AR_FONT, 10))
        self.widgets.texts.append(txt)
        return txt

    def _styled_listbox(self, parent, height=8, font=None):
        lb = tk.Listbox(parent, height=height, relief="flat", bd=0, highlightthickness=1, activestyle="none")
        lb._theme_role = getattr(parent, "_theme_role", "inherit")
        lb.configure(font=font or (self.UI_FONT, 10))
        self.widgets.listboxes.append(lb)
        return lb

    def _styled_checkbutton(self, parent, text, variable, command=None):
        cb = tk.Checkbutton(parent, text=text, variable=variable, command=command, bd=0, highlightthickness=0)
        cb._theme_role = getattr(parent, "_theme_role", "inherit")
        cb.configure(font=(self.UI_FONT, 10))
        self.widgets.checkbuttons.append(cb)
        return cb

    def _styled_radiobutton(self, parent, text, variable, value):
        rb = tk.Radiobutton(parent, text=text, variable=variable, value=value, bd=0, highlightthickness=0)
        rb._theme_role = getattr(parent, "_theme_role", "inherit")
        rb.configure(font=(self.UI_FONT, 10))
        self.widgets.radiobuttons.append(rb)
        return rb

    def _styled_option_menu(self, parent, variable):
        om = tk.OptionMenu(parent, variable, variable.get() or "General")
        om._theme_role = getattr(parent, "_theme_role", "inherit")
        self.widgets.optionmenus.append(om)
        return om

    def _styled_combobox(self, parent, variable, values=None, width=None):
        combo = ttk.Combobox(parent, textvariable=variable, values=list(values or ()), state="readonly", style="BT.TCombobox")
        if width is not None:
            combo.configure(width=width)
        combo._theme_role = getattr(parent, "_theme_role", "inherit")
        self.widgets.comboboxes.append(combo)
        return combo

    def _styled_icon_button(self, parent, command):
        btn = tk.Button(parent, command=command, relief="flat", bd=0, cursor="hand2", padx=6, pady=6)
        btn._theme_role = getattr(parent, "_theme_role", "inherit")
        btn._icon_only = True
        self.widgets.buttons.append(btn)
        return btn

    def _styled_editor_tab_button(self, parent, text, tab_name):
        btn = tk.Button(parent, text=text, relief="flat", bd=0, cursor="hand2", padx=10, pady=8)
        btn._theme_role = getattr(parent, "_theme_role", "inherit")
        btn._editor_tab_button = True
        btn._editor_tab_name = tab_name
        btn.configure(font=(self.UI_FONT, 10, "bold"), command=lambda name=tab_name: self._select_editor_tab(name))
        self.widgets.buttons.append(btn)
        return btn

    def _normalize_hotkey_text(self, value: str):
        parts = [part.strip().lower() for part in str(value or "").split("+") if part.strip()]
        return "+".join(parts)

    def _refresh_hotkey_label(self):
        hotkey = self._normalize_hotkey_text(self.hotkey_var.get()) or DEFAULT_CONFIG["settings"]["hotkey"]
        if hasattr(self, "hotkey_label") and self.hotkey_label is not None:
            self.hotkey_label.configure(text=f"Global toggle: {hotkey}")

    def _current_search_query(self):
        if getattr(self, "_search_placeholder_active", False):
            return ""
        return self.search_var.get().strip()

    def _update_search_placeholder(self):
        if not hasattr(self, "search_entry"):
            return
        empty = not self.search_var.get().strip()
        try:
            focused = self.root.focus_get() is self.search_entry
        except Exception:
            focused = False
        if empty and not focused:
            self._search_placeholder_active = True
            self.search_var.set("Search")
            self.search_entry.configure(fg=self.theme["muted"])
        elif self._search_placeholder_active and focused:
            self._search_placeholder_active = False
            self.search_var.set("")
            self.search_entry.configure(fg=self.theme["entry_fg"])
        elif not self._search_placeholder_active:
            self.search_entry.configure(fg=self.theme["entry_fg"])

    def _on_search_focus_in(self, _event=None):
        if self._search_placeholder_active:
            self._search_placeholder_active = False
            self.search_var.set("")
        try:
            self.search_entry.configure(fg=self.theme["entry_fg"])
        except Exception:
            pass

    def _on_search_focus_out(self, _event=None):
        self._update_search_placeholder()

    def _on_search_key_release(self, _event=None):
        if self._search_placeholder_active:
            return
        self._load_snippets()

    def _on_engine_state_changed(self, enabled: bool):
        try:
            self.root.after(0, lambda value=bool(enabled): self._apply_engine_state(value))
        except Exception:
            pass

    def _apply_engine_state(self, enabled: bool):
        self.engine.engine_enabled = bool(enabled)
        self._refresh_engine_button()
        self._refresh_tray_icon()

    def _is_hotkey_modifier(self, key_name: str):
        return key_name in {"ctrl", "alt", "alt gr", "shift", "windows"}

    def _normalize_hotkey_event(self, event):
        keysym = str(getattr(event, "keysym", "") or "").strip().lower()
        if not keysym:
            return ""
        mapping = {
            "control_l": "ctrl",
            "control_r": "ctrl",
            "shift_l": "shift",
            "shift_r": "shift",
            "alt_l": "alt",
            "alt_r": "alt gr",
            "option_l": "alt",
            "option_r": "alt gr",
            "super_l": "windows",
            "super_r": "windows",
            "win_l": "windows",
            "win_r": "windows",
            "lwin": "windows",
            "rwin": "windows",
            "return": "enter",
            "escape": "esc",
            "prior": "page up",
            "next": "page down",
            "caps_lock": "caps lock",
            "print": "print screen",
            "snapshot": "print screen",
            "space": "space",
        }
        if keysym in mapping:
            return mapping[keysym]
        if len(keysym) == 1:
            return keysym
        return keysym.replace("_", " ")

    def _ordered_hotkey_parts(self, parts):
        modifier_order = {"ctrl": 0, "alt": 1, "alt gr": 2, "shift": 3, "windows": 4}
        modifiers = sorted(
            [item for item in parts if self._is_hotkey_modifier(item)],
            key=lambda item: modifier_order.get(item, 99),
        )
        others = [item for item in parts if not self._is_hotkey_modifier(item)]
        return modifiers + others

    def _hotkey_capture_display(self, parts):
        ordered = self._ordered_hotkey_parts(parts)
        return "+".join(ordered) if ordered else "Press your shortcut..."

    def _set_hotkey_capture_ui(self):
        if self._hotkey_capture_active:
            self.hotkey_var.set(self._hotkey_capture_display(self._hotkey_capture_parts))
            self.hotkey_capture_button.configure(text="Cancel Capture")
        else:
            current_hotkey = self._normalize_hotkey_text(self.hotkey_var.get()) or DEFAULT_CONFIG["settings"]["hotkey"]
            self.hotkey_var.set(current_hotkey)
            self.hotkey_capture_button.configure(text="Record Hotkey")
        self._refresh_hotkey_label()

    def toggle_hotkey_capture(self):
        if self._hotkey_capture_active:
            self._hotkey_capture_active = False
            self._hotkey_capture_parts = []
            self._hotkey_capture_pressed = set()
            self.hotkey_var.set(self._hotkey_capture_original)
            self._set_hotkey_capture_ui()
            self.update_status("Hotkey capture cancelled")
            return
        self._hotkey_capture_active = True
        self._hotkey_capture_parts = []
        self._hotkey_capture_pressed = set()
        self._hotkey_capture_original = self.hotkey_var.get()
        self._set_hotkey_capture_ui()
        self.update_status("Press the new global hotkey now")
        try:
            self.hotkey_entry.focus_set()
        except Exception:
            pass

    def _commit_hotkey_capture(self):
        ordered = self._ordered_hotkey_parts(self._hotkey_capture_parts)
        hotkey = self._normalize_hotkey_text("+".join(ordered))
        if hotkey:
            self.hotkey_var.set(hotkey)
            self.update_status(f"Captured hotkey: {hotkey}")
        self._hotkey_capture_active = False
        self._hotkey_capture_parts = []
        self._hotkey_capture_pressed = set()
        self._set_hotkey_capture_ui()

    def _on_hotkey_capture_keypress(self, event):
        if not self._hotkey_capture_active:
            return None
        key_name = self._normalize_hotkey_event(event)
        if not key_name:
            return "break"
        self._hotkey_capture_pressed.add(key_name)
        if key_name not in self._hotkey_capture_parts:
            self._hotkey_capture_parts.append(key_name)
        self.hotkey_var.set(self._hotkey_capture_display(self._hotkey_capture_parts))
        self._refresh_hotkey_label()
        if not self._is_hotkey_modifier(key_name):
            self._commit_hotkey_capture()
        return "break"

    def _on_hotkey_capture_keyrelease(self, event):
        if not self._hotkey_capture_active:
            return None
        key_name = self._normalize_hotkey_event(event)
        if key_name:
            self._hotkey_capture_pressed.discard(key_name)
        if self._hotkey_capture_parts and not self._hotkey_capture_pressed:
            self._commit_hotkey_capture()
        return "break"

    def _sync_settings_vars_from_config(self):
        settings = self.config_manager.data["settings"]
        self.theme_name = settings.get("theme", "dark")
        self.restore_delay_var.set(str(settings.get("restore_clipboard_delay_ms", 60)))
        self.always_on_top_var.set(bool(settings.get("always_on_top", False)))
        self.start_with_windows_var.set(bool(settings.get("start_with_windows", False)))
        self.minimize_to_tray_var.set(bool(settings.get("minimize_to_tray", True)))
        self.previous_clipboard_var.set(bool(settings.get("use_previous_clipboard_trigger", False)))
        self.previous_clipboard_slash_var.set(bool(settings.get("use_previous_clipboard_slash_trigger", False)))
        self.case_sensitive_var.set(bool(settings.get("match_case_sensitive", False)))
        self.mode_var.set(settings.get("trigger_mode", "instant"))
        self.hotkey_var.set(str(settings.get("hotkey", DEFAULT_CONFIG["settings"]["hotkey"])))
        separators = settings.get("word_separators", ["space", "enter", "tab"])
        self.sep_space_var.set("space" in separators)
        self.sep_enter_var.set("enter" in separators)
        self.sep_tab_var.set("tab" in separators)
        self._refresh_hotkey_label()

    def _find_existing_icon_path(self, candidates):
        for name in candidates:
            for folder in ASSET_DIRS:
                path = os.path.join(folder, name)
                if os.path.exists(path):
                    return path
        return ""

    def _icon_resample_filter(self):
        if Image is None:
            return None
        try:
            return Image.Resampling.LANCZOS
        except Exception:
            return getattr(Image, "LANCZOS", None)

    def _load_external_icon_image(self, candidates, size=None):
        if Image is None:
            return None
        path = self._find_existing_icon_path(candidates)
        if not path:
            return None
        try:
            image = Image.open(path).convert("RGBA")
            if size:
                resample = self._icon_resample_filter()
                if resample is not None:
                    image.thumbnail(size, resample)
                else:
                    image.thumbnail(size)
                canvas = Image.new("RGBA", size, (0, 0, 0, 0))
                x = max(0, (size[0] - image.width) // 2)
                y = max(0, (size[1] - image.height) // 2)
                canvas.paste(image, (x, y), image)
                return canvas
            return image
        except Exception:
            return None

    def _load_button_photo(self, candidates, size=(16, 16)):
        if Image is not None and ImageTk is not None:
            image = self._load_external_icon_image(candidates, size=size)
            if image is not None:
                try:
                    return ImageTk.PhotoImage(image)
                except Exception:
                    pass
        path = self._find_existing_icon_path(candidates)
        if path and path.lower().endswith(".png"):
            try:
                return tk.PhotoImage(file=path)
            except Exception:
                pass
        return None

    def _setup_toolbar_images(self):
        self._github_button_photo = self._load_button_photo(GITHUB_IMAGE_CANDIDATES)
        self._info_button_photo = self._load_button_photo(INFO_IMAGE_CANDIDATES)
        try:
            if self._github_button_photo is not None:
                self.github_button.configure(text="", image=self._github_button_photo, compound="image")
        except Exception:
            pass
        try:
            if self._info_button_photo is not None:
                self.info_button.configure(text="", image=self._info_button_photo, compound="image")
        except Exception:
            pass

    def open_github(self):
        try:
            webbrowser.open(GITHUB_URL)
            self.update_status("Opened the GitHub page.")
        except Exception as exc:
            self.update_status(f"Could not open GitHub: {exc}")

    def open_wiki(self):
        try:
            webbrowser.open(WIKI_URL)
            self.update_status("Opened the BlinkText wiki.")
        except Exception as exc:
            self.update_status(f"Could not open the wiki: {exc}")

    def _current_editor_tab_name(self) -> str:
        try:
            current = self.editor_notebook.select()
            if current == str(self.snippet_tab):
                return "snippet"
            if current == str(self.engine_tab):
                return "engine"
        except Exception:
            pass
        return "snippet"

    def _refresh_editor_tab_buttons(self):
        active_name = self._current_editor_tab_name()
        buttons = [
            getattr(self, "snippet_manager_tab_button", None),
            getattr(self, "engine_settings_tab_button", None),
        ]
        for btn in buttons:
            if btn is None:
                continue
            btn._editor_tab_active = bool(getattr(btn, "_editor_tab_name", "") == active_name)
        if hasattr(self, "theme"):
            self._apply_theme()

    def _select_editor_tab(self, tab_name: str):
        try:
            if tab_name == "engine":
                self.editor_notebook.select(self.engine_tab)
            else:
                self.editor_notebook.select(self.snippet_tab)
        except Exception:
            pass
        self._refresh_editor_tab_buttons()

    def _on_editor_tab_changed(self, _event=None):
        self._refresh_editor_tab_buttons()

    def open_variables_wiki(self):
        try:
            webbrowser.open(VARIABLES_WIKI_URL)
            self.update_status("Opened the Variables wiki.")
        except Exception as exc:
            self.update_status(f"Could not open the Variables wiki: {exc}")

    def _get_last_io_directory(self):
        candidate = str(self.config_manager.data.get("settings", {}).get("last_io_directory", "") or "").strip()
        if candidate and os.path.isdir(candidate):
            return candidate
        return self._documents_dir()

    def _remember_io_path(self, path: str):
        folder = os.path.dirname(str(path or "").strip())
        if not folder or not os.path.isdir(folder):
            return
        settings = self.config_manager.data.setdefault("settings", {})
        if settings.get("last_io_directory", "") == folder:
            return
        settings["last_io_directory"] = folder
        try:
            self.config_manager.save()
        except Exception:
            pass

    def _insert_into_content_editor(self, text: str):
        try:
            self._select_editor_tab("snippet")
        except Exception:
            pass
        self.content_text.focus_set()
        self.content_text.insert("insert", text)
        self.content_text.see("insert")

    def _insert_content_variable(self, token_text: str):
        self._insert_into_content_editor(token_text)

    def _normalize_recorded_key_name(self, value: str):
        raw = str(value or "").strip()
        if not raw:
            return ""
        lowered = raw.lower()
        aliases = {
            "return": "Enter",
            "enter": "Enter",
            "escape": "Escape",
            "esc": "Escape",
            "backspace": "Backspace",
            "delete": "Delete",
            "del": "Delete",
            "insert": "Insert",
            "ins": "Insert",
            "home": "Home",
            "end": "End",
            "left": "Left",
            "right": "Right",
            "up": "Up",
            "down": "Down",
            "tab": "Tab",
            "space": "Space",
            "prior": "PageUp",
            "pgup": "PageUp",
            "pageup": "PageUp",
            "next": "PageDown",
            "pgdn": "PageDown",
            "pagedown": "PageDown",
            "pause": "Pause",
            "numlock": "NumLock",
            "printscreen": "PrintScreen",
            "prtsc": "PrintScreen",
            "windows": "Windows",
            "win": "Windows",
            "control": "Control",
            "ctrl": "Control",
            "alt": "Alt",
            "shift": "Shift",
            "volumemute": "VolumeMute",
            "volumeup": "VolumeUp",
            "volumedown": "VolumeDown",
            "medianexttrack": "MediaNextTrack",
            "mediaprevioustrack": "MediaPreviousTrack",
            "mediastop": "MediaStop",
            "mediaplaypause": "MediaPlayPause",
            "mediaselect": "MediaSelect",
        }
        if lowered in aliases:
            return aliases[lowered]
        if lowered.startswith("f") and lowered[1:].isdigit():
            number = int(lowered[1:])
            if 1 <= number <= 24:
                return f"F{number}"
        if len(raw) == 1:
            return raw.upper()
        return raw

    def _show_variable_key_insert_dialog(self):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("Insert Key Variable")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        value_var = tk.StringVar()
        hint_var = tk.StringVar(value="Press a key or type its name manually.")
        result = {"value": None}

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        self._styled_label(
            frm,
            "Record a key name for #{key:...}. Examples: Tab, Enter, Escape, Home, Left, F5.",
            role="panel",
        ).pack(anchor="w", pady=(0, 10))

        entry = self._styled_entry(frm, value_var, font=(self.MONO_FONT, 10))
        entry.pack(fill="x", pady=(0, 8), ipady=4)

        self._styled_label(frm, "", muted=True, role="panel").configure(textvariable=hint_var)
        frm.winfo_children()[-1].pack(anchor="w", pady=(0, 14))

        def capture_key(event):
            keysym = getattr(event, "keysym", "") or getattr(event, "keysym_num", "")
            state = int(getattr(event, "state", 0) or 0)
            if not (state & (0x0001 | 0x0004 | 0x0008)):
                if isinstance(keysym, str) and len(keysym) == 1:
                    return None
            normalized = self._normalize_recorded_key_name(keysym)
            if normalized in {"Shift", "Control", "Alt", "Windows"}:
                value_var.set(normalized)
                hint_var.set("Modifier captured. You can keep it or type another key manually.")
                return "break"
            if normalized:
                value_var.set(normalized)
                hint_var.set(f"Captured key: {normalized}")
                return "break"
            return None

        def do_insert():
            key_name = self._normalize_recorded_key_name(value_var.get())
            if key_name:
                result["value"] = f"#{{key:{key_name}}}"
            else:
                result["value"] = "#{key:}"
            dialog.destroy()

        def cancel():
            dialog.destroy()

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")
        self._dialog_button(row, "Insert", do_insert).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Cancel", cancel).pack(side="left", expand=True, fill="x")

        entry.bind("<KeyPress>", capture_key)
        dialog.bind("<Escape>", lambda _e: cancel())
        dialog.protocol("WM_DELETE_WINDOW", cancel)
        self._center_dialog(dialog, 540, 210)
        entry.focus_set()
        dialog.wait_window()
        return result["value"]

    def _format_shortcut_for_variable(self, modifiers: list[str], key_name: str):
        pretty_mods = []
        for item in modifiers:
            lowered = item.lower()
            if lowered == "ctrl":
                pretty_mods.append("Ctrl")
            elif lowered == "shift":
                pretty_mods.append("Shift")
            elif lowered == "alt":
                pretty_mods.append("Alt")
            elif lowered == "win":
                pretty_mods.append("Win")
        if key_name:
            pretty_mods.append(self._normalize_recorded_key_name(key_name))
        return "+".join(pretty_mods)

    def _show_variable_shortcut_insert_dialog(self):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("Insert Shortcut Variable")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        result = {"value": None}
        value_var = tk.StringVar()
        hint_var = tk.StringVar(value="Press the shortcut now, or type it manually.")

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        self._styled_label(
            frm,
            "Record a shortcut for #{shortcut:...}. Examples: Ctrl+S, Ctrl+Shift+S, Alt+F4, Win+R.",
            role="panel",
        ).pack(anchor="w", pady=(0, 10))

        entry = self._styled_entry(frm, value_var, font=(self.MONO_FONT, 10))
        entry.pack(fill="x", pady=(0, 8), ipady=4)

        self._styled_label(frm, "", muted=True, role="panel").configure(textvariable=hint_var)
        frm.winfo_children()[-1].pack(anchor="w", pady=(0, 14))

        def modifiers_from_event(event):
            mods = []
            state = int(getattr(event, "state", 0) or 0)
            if state & 0x0004:
                mods.append("ctrl")
            if state & 0x0001:
                mods.append("shift")
            if state & 0x0008 or state & 0x20000:
                mods.append("alt")
            keysym = (getattr(event, "keysym", "") or "").lower()
            if keysym in {"super_l", "super_r", "win_l", "win_r"} and "win" not in mods:
                mods.append("win")
            return mods

        def capture_shortcut(event):
            keysym = getattr(event, "keysym", "") or ""
            lowered = keysym.lower()
            if lowered in {"shift_l", "shift_r", "control_l", "control_r", "alt_l", "alt_r", "super_l", "super_r", "win_l", "win_r"}:
                return "break"
            key_name = self._normalize_recorded_key_name(keysym)
            if not key_name:
                return "break"
            modifiers = modifiers_from_event(event)
            if not modifiers and isinstance(keysym, str) and len(keysym) == 1:
                return None
            if not modifiers:
                value_var.set(key_name)
                hint_var.set("Add modifiers like Ctrl, Shift, Alt, or Win for shortcut variables.")
                return "break"
            display_value = self._format_shortcut_for_variable(modifiers, key_name)
            value_var.set(display_value)
            hint_var.set(f"Captured shortcut: {display_value}")
            return "break"

        def do_insert():
            typed = value_var.get().strip()
            if not typed:
                result["value"] = "#{shortcut:}"
                dialog.destroy()
                return
            normalized = self._normalize_hotkey_text(typed)
            reserved = self._normalize_hotkey_text(self.hotkey_var.get())
            if normalized and reserved and normalized == reserved:
                hint_var.set("This shortcut is already used by BlinkText. Choose a different shortcut.")
                return
            result["value"] = f"#{{shortcut:{typed}}}"
            dialog.destroy()

        def cancel():
            dialog.destroy()

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")
        self._dialog_button(row, "Insert", do_insert).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Cancel", cancel).pack(side="left", expand=True, fill="x")

        entry.bind("<KeyPress>", capture_shortcut)
        dialog.bind("<Escape>", lambda _e: cancel())
        dialog.protocol("WM_DELETE_WINDOW", cancel)
        self._center_dialog(dialog, 560, 220)
        entry.focus_set()
        dialog.wait_window()
        return result["value"]

    def _show_variables_about_dialog(self):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("Variables About")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        self._styled_label(frm, "Variables About", font=(self.UI_FONT, 14, "bold"), role="panel").pack(anchor="w", pady=(0, 10))

        help_text = (
            "Variables let snippets insert dynamic content or run actions during expansion.\n\n"
            "Examples\n"
            "• #{clipboard} inserts the original clipboard text.\n"
            "• #{previousClipboard} inserts the captured previous clipboard text.\n"
            "• #{dateTime:yyyy-MM-dd} inserts a custom formatted date/time.\n"
            "• #{key:Tab} and #{shortcut:Ctrl+Shift+S} run actions in order.\n"
            "• #{delay:1000} waits 1000 ms.\n"
            "• #{cursor} moves the caret after expansion.\n"
            "• #{combo:trigger} reuses another snippet.\n"
            "• #{input:} asks for multiline input when the snippet expands.\n"
            "• #{envVar:USERNAME} reads a Windows environment variable.\n"
            "• #{powershell:C:\\Temp\\script.ps1} inserts script output."
        )

        help_text = (
            "Variables let snippets insert dynamic text or run actions during expansion.\n\n"
            "Quick examples\n"
            "- #{clipboard} inserts the captured clipboard text.\n"
            "- #{previousClipboard} reuses the clipboard snapshot from the same expansion.\n"
            "- #{dateTime:yyyy-MM-dd} inserts a custom formatted date/time.\n"
            "- #{key:Tab} and #{shortcut:Ctrl+Shift+S} run actions in order.\n"
            "- #{delay:1000} waits 1000 ms.\n"
            "- #{cursor} places the caret after expansion.\n"
            "- #{combo:trigger} reuses another snippet.\n"
            "- #{input:} asks for multiline input when the snippet expands.\n"
            "- #{envVar:USERNAME} reads a Windows environment variable.\n"
            "- #{powershell:C:\\Temp\\script.ps1} inserts PowerShell output.\n\n"
            "Use Open Variables Wiki for the full reference and more examples."
        )

        text = tk.Text(
            frm,
            height=14,
            wrap="word",
            bg=c["entry_bg"],
            fg=c["entry_fg"],
            insertbackground=c["entry_fg"],
            relief="flat",
            bd=0,
            highlightthickness=1,
            highlightbackground=c["border"],
            highlightcolor=c["border"],
            font=(self.UI_FONT, 10),
            padx=10,
            pady=10,
        )
        text.pack(fill="both", expand=True)
        text.insert("1.0", help_text)
        text.configure(state="disabled")

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x", pady=(12, 0))
        self._dialog_button(row, "Open Variables Wiki", self.open_variables_wiki).pack(
            side="left", expand=True, fill="x", padx=(0, 8)
        )
        self._dialog_button(row, "Close", dialog.destroy).pack(side="left", expand=True, fill="x")

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)
        self._center_dialog(dialog, 680, 430)
        dialog.focus_force()
        dialog.wait_window()

    def _get_widget_text_value(self, widget):
        try:
            if isinstance(widget, tk.Text):
                return widget.get("1.0", "end-1c")
            if isinstance(widget, tk.Entry):
                return widget.get()
        except Exception:
            pass
        return ""

    def _insert_text_into_widget(self, widget, value: str):
        if not value:
            return
        try:
            if isinstance(widget, tk.Text):
                widget.insert("insert", value)
                widget.see("insert")
                if widget is self.test_area:
                    self._local_test_buffer = ""
                return
            if isinstance(widget, tk.Entry):
                widget.insert("insert", value)
        except Exception:
            pass

    def _toggle_widget_reading_order(self, widget):
        rtl_enabled = bool(getattr(widget, "_blink_rtl_enabled", False))
        rtl_enabled = not rtl_enabled
        setattr(widget, "_blink_rtl_enabled", rtl_enabled)
        try:
            if isinstance(widget, tk.Entry):
                widget.configure(justify="right" if rtl_enabled else "left")
            elif isinstance(widget, tk.Text):
                widget.tag_configure("blink_rtl", justify="right")
                widget.tag_configure("blink_ltr", justify="left")
                widget.tag_remove("blink_rtl", "1.0", "end")
                widget.tag_remove("blink_ltr", "1.0", "end")
                widget.tag_add("blink_rtl" if rtl_enabled else "blink_ltr", "1.0", "end")
        except Exception:
            pass
        self.update_status("Right-to-left reading order enabled" if rtl_enabled else "Left-to-right reading order restored")

    def _show_text_preview_dialog(self, title: str, heading: str, content: str, *, height: int = 14, width: int = 680):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title(title)
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        self._styled_label(frm, heading, font=(self.UI_FONT, 14, "bold"), role="panel").pack(anchor="w", pady=(0, 10))
        text = tk.Text(
            frm,
            height=height,
            wrap="word",
            bg=c["entry_bg"],
            fg=c["entry_fg"],
            insertbackground=c["entry_fg"],
            relief="flat",
            bd=0,
            highlightthickness=1,
            highlightbackground=c["border"],
            highlightcolor=c["border"],
            font=(self.UI_FONT, 10),
            padx=10,
            pady=10,
        )
        text.pack(fill="both", expand=True)
        text.insert("1.0", content)
        text.configure(state="disabled")

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x", pady=(12, 0))
        self._dialog_button(row, "Close", dialog.destroy).pack(side="left", expand=True, fill="x")

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)
        self._center_dialog(dialog, width, 420)
        dialog.focus_force()
        dialog.wait_window()

    def _show_unicode_control_characters(self, widget):
        text = self._get_widget_text_value(widget)
        if not text:
            self._show_text_preview_dialog(
                "Unicode Control Characters",
                "Unicode Control Characters",
                "This field is empty.\n\nThere are no Unicode control characters to display.",
                height=8,
                width=560,
            )
            return
        found = []
        preview_parts = []
        for index, ch in enumerate(text):
            info = UNICODE_CONTROL_MAP.get(ch)
            if info:
                label, description = info
                found.append(f"{label} at position {index + 1}: {description}")
                preview_parts.append(f"<{label}>")
            else:
                preview_parts.append(ch)
        if not found:
            self._show_text_preview_dialog(
                "Unicode Control Characters",
                "Unicode Control Characters",
                "No Unicode control characters were found in this field.",
                height=8,
                width=560,
            )
            return
        body = "Detected characters:\n" + "\n".join(found) + "\n\nPreview:\n" + "".join(preview_parts)
        self._show_text_preview_dialog(
            "Unicode Control Characters",
            "Unicode Control Characters",
            body,
            height=16,
            width=720,
        )

    def _open_ime_for_widget(self, widget):
        if os.name != "nt":
            self.update_status("IME controls are available on Windows only.")
            return
        try:
            hwnd = int(widget.winfo_id())
            imm32 = ctypes.windll.imm32
            user32 = ctypes.windll.user32
            himc = imm32.ImmGetContext(hwnd)
            if himc:
                imm32.ImmSetOpenStatus(himc, True)
                imm32.ImmReleaseContext(hwnd, himc)
            user32.SetFocus(hwnd)
            self.update_status("IME open request sent.")
        except Exception:
            self.update_status("Could not open IME for this field.")

    def _request_reconversion_for_widget(self, widget):
        if os.name != "nt":
            self.update_status("Reconversion is available on Windows only.")
            return
        try:
            hwnd = int(widget.winfo_id())
            WM_IME_REQUEST = 0x0288
            IMR_RECONVERTSTRING = 0x0004
            ctypes.windll.user32.SendMessageW(hwnd, WM_IME_REQUEST, IMR_RECONVERTSTRING, 0)
            self.update_status("Reconversion request sent.")
        except Exception:
            self.update_status("Could not send reconversion request.")

    def _populate_unicode_insert_menu(self, menu, widget):
        menu.delete(0, "end")
        for label, char, description in UNICODE_CONTROL_ITEMS:
            menu.add_command(
                label=f"{label} - {description}",
                command=lambda value=char, w=widget: self._insert_text_into_widget(w, value),
            )

    def _add_advanced_text_menu_items(self, menu, widget, unicode_menu):
        rtl_label = "Left to right Reading order" if bool(getattr(widget, "_blink_rtl_enabled", False)) else "Right to left Reading order"
        menu.add_separator()
        menu.add_command(label=rtl_label, command=lambda w=widget: self._toggle_widget_reading_order(w))
        menu.add_command(label="Show Unicode control characters", command=lambda w=widget: self._show_unicode_control_characters(w))
        self._populate_unicode_insert_menu(unicode_menu, widget)
        menu.add_cascade(label="Insert Unicode control character", menu=unicode_menu)
        menu.add_separator()
        menu.add_command(label="Open IME", command=lambda w=widget: self._open_ime_for_widget(w))
        menu.add_command(label="Reconversion", command=lambda w=widget: self._request_reconversion_for_widget(w))

    def _refresh_content_editor_menu(self):
        self.content_menu_popup.delete(0, "end")
        self.content_variables_menu.delete(0, "end")
        self.content_variables_datetime_menu.delete(0, "end")
        self.content_variables_combo_menu.delete(0, "end")
        self.content_unicode_menu.delete(0, "end")

        self.content_menu_popup.add_command(label="Undo", command=lambda: self.content_text.event_generate("<<Undo>>"))
        self.content_menu_popup.add_separator()
        self.content_menu_popup.add_command(label="Cut", command=lambda: self.content_text.event_generate("<<Cut>>"))
        self.content_menu_popup.add_command(label="Copy", command=lambda: self.content_text.event_generate("<<Copy>>"))
        self.content_menu_popup.add_command(label="Paste", command=lambda: self.content_text.event_generate("<<Paste>>"))
        self.content_menu_popup.add_command(label="Delete", command=lambda: self.content_text.delete("sel.first", "sel.last") if self.content_text.tag_ranges("sel") else None)
        self.content_menu_popup.add_separator()
        self.content_menu_popup.add_command(label="Select All", command=lambda: self.content_text.tag_add("sel", "1.0", "end-1c"))
        self._add_advanced_text_menu_items(self.content_menu_popup, self.content_text, self.content_unicode_menu)
        self.content_menu_popup.add_separator()

        self.content_variables_menu.add_command(
            label="Content Clipboard",
            command=lambda: self._insert_content_variable("#{clipboard}"),
        )
        self.content_variables_menu.add_command(
            label="Previous Clipboard",
            command=lambda: self._insert_content_variable("#{previousClipboard}"),
        )

        self.content_variables_datetime_menu.add_command(label="Date", command=lambda: self._insert_content_variable("#{date}"))
        self.content_variables_datetime_menu.add_command(label="Time", command=lambda: self._insert_content_variable("#{time}"))
        self.content_variables_datetime_menu.add_command(label="Date & Time", command=lambda: self._insert_content_variable("#{dateTime}"))
        self.content_variables_datetime_menu.add_command(
            label="Custom Date and Time",
            command=lambda: self._insert_content_variable("#{dateTime:}"),
        )
        self.content_variables_menu.add_cascade(label="Date/Time", menu=self.content_variables_datetime_menu)

        self.content_variables_menu.add_command(
            label="Key",
            command=lambda: (lambda value: self._insert_content_variable(value) if value else None)(
                self._show_variable_key_insert_dialog()
            ),
        )
        self.content_variables_menu.add_command(
            label="Shortcut",
            command=lambda: (lambda value: self._insert_content_variable(value) if value else None)(
                self._show_variable_shortcut_insert_dialog()
            ),
        )
        self.content_variables_menu.add_command(label="Delay", command=lambda: self._insert_content_variable("#{delay:}"))
        self.content_variables_menu.add_command(label="Position Cursor", command=lambda: self._insert_content_variable("#{cursor}"))

        self.content_variables_combo_menu.add_command(label="Combo", command=lambda: self._insert_content_variable("#{combo:}"))
        self.content_variables_combo_menu.add_command(label="Trim Combo", command=lambda: self._insert_content_variable("#{trim:}"))
        self.content_variables_combo_menu.add_command(label="Upper Combo", command=lambda: self._insert_content_variable("#{upper:}"))
        self.content_variables_combo_menu.add_command(label="Lower Combo", command=lambda: self._insert_content_variable("#{lower:}"))
        self.content_variables_menu.add_cascade(label="Combo", menu=self.content_variables_combo_menu)

        self.content_variables_menu.add_command(label="Environment Variable", command=lambda: self._insert_content_variable("#{envVar:}"))
        self.content_variables_menu.add_command(
            label="Script PowerShell",
            command=self._insert_selected_powershell_variable,
        )
        self.content_variables_menu.add_command(label="Input User", command=lambda: self._insert_content_variable("#{input:}"))
        self.content_variables_menu.add_separator()
        self.content_variables_menu.add_command(label="Variables About", command=self._show_variables_about_dialog)

        self.content_menu_popup.add_cascade(label="Variables", menu=self.content_variables_menu)

    def _insert_selected_powershell_variable(self):
        path = filedialog.askopenfilename(
            title="Select PowerShell script",
            initialdir=self._get_last_io_directory(),
            filetypes=[("PowerShell scripts", "*.ps1")],
        )
        if path:
            self._remember_io_path(path)
            self._insert_content_variable(f"#{{powershell:{path}}}")

    def on_content_editor_right_click(self, event):
        try:
            self.content_text.focus_set()
            self.content_text.mark_set("insert", f"@{event.x},{event.y}")
        except Exception:
            pass
        self._refresh_content_editor_menu()
        self._popup_menu(self.content_menu_popup, event.x_root, event.y_root)

    def show_info_dialog(self):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("About BlinkText")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        self._styled_label(frm, "About BlinkText", font=(self.UI_FONT, 14, "bold"), role="panel").pack(anchor="w", pady=(0, 10))

        text = tk.Text(
            frm,
            height=17,
            wrap="word",
            bg=c["entry_bg"],
            fg=c["entry_fg"],
            insertbackground=c["entry_fg"],
            relief="flat",
            bd=0,
            highlightthickness=1,
            highlightbackground=c["border"],
            highlightcolor=c["border"],
            font=(self.UI_FONT, 10),
            padx=10,
            pady=10,
        )
        text.pack(fill="both", expand=True)
        text.insert("1.0", ABOUT_TEXT)
        text.configure(state="disabled")

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x", pady=(12, 0))
        self._dialog_button(row, "Open Wiki", self.open_wiki).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Open GitHub", self.open_github).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Close", dialog.destroy).pack(side="left", expand=True, fill="x")

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)
        self._center_dialog(dialog, 620, 480)
        dialog.focus_force()
        dialog.wait_window()

    def _schedule_layout_refresh(self, _event=None):
        if self._layout_refresh_job is not None:
            try:
                self.root.after_cancel(self._layout_refresh_job)
            except Exception:
                pass
        self._layout_refresh_job = self.root.after(18, self._apply_responsive_layout)

    def _apply_responsive_layout(self):
        self._layout_refresh_job = None
        self._resize_snippet_columns()
        self._on_editor_inner_configure()

    def _resize_snippet_columns(self):
        try:
            tree_width = max(0, self.snippet_tree.winfo_width() - 28)
            if tree_width <= 0:
                return
            fixed_width = {"trigger": 110, "group": 120, "enabled": 70}
            flexible = max(320, tree_width - sum(fixed_width.values()))
            notes_width = max(150, min(240, flexible // 3))
            preview_width = max(220, flexible - notes_width)
            self.snippet_tree.column("trigger", width=fixed_width["trigger"], minwidth=90, stretch=False)
            self.snippet_tree.column("group", width=fixed_width["group"], minwidth=100, stretch=False)
            self.snippet_tree.column("enabled", width=fixed_width["enabled"], minwidth=60, stretch=False)
            self.snippet_tree.column("notes", width=notes_width, minwidth=140, stretch=False)
            self.snippet_tree.column("preview", width=preview_width, minwidth=200, stretch=True)
        except Exception:
            pass

    def _pulse_topmost(self):
        try:
            self.root.attributes("-topmost", True)
            self.root.after(180, lambda: self.root.attributes("-topmost", self.always_on_top_var.get()))
        except Exception:
            pass

    def _finish_window_restore(self):
        try:
            self.root.update_idletasks()
            self.root.lift()
            self._pulse_topmost()
            self._stabilize_window_chrome_on_startup()
            self._schedule_layout_refresh()
            self.update_status("Restored from tray")
        finally:
            try:
                self.root.attributes("-alpha", 1.0)
            except Exception:
                pass
            self._restoring_from_tray = False

    def _build_ui(self):
        self.root.configure(bg=self.theme["bg"])

        self.header = self._styled_frame(self.root, role="header", border=False)
        self.header.pack(fill="x")

        self.header_inner = self._styled_frame(self.header, role="header", border=False)
        self.header_inner.pack(fill="x", padx=14, pady=(14, 10))
        self.title_label = self._styled_label(self.header_inner, APP_TITLE, font=(self.UI_FONT, 22, "bold"), role="header")
        self.title_label.pack(anchor="w")
        self.subtitle_label = self._styled_label(
            self.header_inner,
            "Fast, local text expansion with instant triggers, tray support, and compatible import/export.",
            font=(self.UI_FONT, 10),
            muted=True,
            role="header",
        )

        self.toolbar = self._styled_frame(self.root, role="panel")
        self.toolbar.pack(fill="x", padx=12, pady=(0, 10))
        self.toolbar_left = self._styled_frame(self.toolbar, role="panel", border=False)
        self.toolbar_left.pack(side="left", padx=12, pady=12)
        self.toolbar_right = self._styled_frame(self.toolbar, role="panel", border=False)
        self.toolbar_right.pack(side="right", padx=12, pady=12)

        self.engine_button = self._styled_button(self.toolbar_left, "Engine: ON", self.toggle_engine, accent=True)
        self.engine_button.pack(side="left", padx=(0, 8))
        self.theme_button = self._styled_button(self.toolbar_left, "Theme: Dark", self.toggle_theme)
        self.theme_button.pack(side="left", padx=(0, 8))
        self.import_button = self._styled_button(self.toolbar_left, "Import", self.import_data)
        self.import_button.pack(side="left", padx=(8, 8))
        self.export_button = self._styled_button(self.toolbar_left, "Export", self.export_data)
        self.export_button.pack(side="left", padx=(0, 8))
        self.hotkey_label = None
        self.info_button = self._styled_icon_button(self.toolbar_right, self.show_info_dialog)
        self.info_button.configure(text="i")
        self.info_button.pack(side="right", padx=(0, 8))
        self.github_button = self._styled_icon_button(self.toolbar_right, self.open_github)
        self.github_button.configure(text="GH")
        self.github_button.pack(side="right", padx=(0, 8))

        self.content = self._styled_frame(self.root, role="bg", border=False)
        self.content.pack(fill="both", expand=True, padx=12, pady=(0, 12))

        self.content_pane = tk.PanedWindow(self.content, orient="horizontal", sashwidth=10, bd=0, opaqueresize=True)
        self.content_pane.pack(fill="both", expand=True)
        self.widgets.canvases.append(self.content_pane)
        self.root.bind("<Configure>", self._schedule_layout_refresh, add="+")
        self.content_pane.bind("<B1-Motion>", self._schedule_layout_refresh, add="+")
        self.content_pane.bind("<ButtonRelease-1>", self._schedule_layout_refresh, add="+")

        self.groups_panel = self._styled_frame(self.content_pane, role="panel")
        self.snippets_panel = self._styled_frame(self.content_pane, role="panel")
        self.editor_panel = self._styled_frame(self.content_pane, role="panel")

        self.content_pane.add(self.groups_panel, minsize=230, width=250)
        self.content_pane.add(self.snippets_panel, minsize=520, width=760)
        self.content_pane.add(self.editor_panel, minsize=330, width=420)

        self._build_groups_panel()
        self._build_snippets_panel()
        self._build_editor_panel()

    def _build_groups_panel(self):
        panel = self.groups_panel
        self.groups_title_label = self._styled_label(panel, "Groups", font=(self.UI_FONT, 18, "bold"))
        self.groups_title_label.pack(anchor="w", padx=12, pady=(12, 6))

        body = self._styled_frame(panel, role="panel", border=False)
        body.pack(fill="both", expand=True, padx=12, pady=12)

        self.groups_pane = tk.PanedWindow(body, orient="vertical", sashwidth=10, bd=0, opaqueresize=True)
        self.groups_pane.pack(fill="both", expand=True)
        self.widgets.canvases.append(self.groups_pane)
        self.groups_pane.bind("<B1-Motion>", self._schedule_layout_refresh, add="+")
        self.groups_pane.bind("<ButtonRelease-1>", self._schedule_layout_refresh, add="+")

        groups_wrap = self._styled_frame(self.groups_pane, role="panel_2")
        groups_wrap.grid_rowconfigure(1, weight=1)
        groups_wrap.grid_columnconfigure(0, weight=1)

        groups_actions = self._styled_frame(groups_wrap, role="panel_2", border=False)
        groups_actions.grid(row=0, column=0, sticky="ew", padx=10, pady=(10, 8))
        groups_actions.grid_columnconfigure(0, weight=1)
        groups_actions.grid_columnconfigure(1, weight=1)
        self.new_group_button = self._styled_button(groups_actions, "New Group", self.new_group)
        self.new_group_button.grid(row=0, column=0, sticky="ew", padx=(0, 6), pady=(0, 6))
        self.rename_group_button = self._styled_button(groups_actions, "Rename", self.rename_group)
        self.rename_group_button.grid(row=0, column=1, sticky="ew", pady=(0, 6))
        self.toggle_group_button = self._styled_button(groups_actions, "Toggle", self.toggle_group)
        self.toggle_group_button.grid(row=1, column=0, sticky="ew", padx=(0, 6))
        self.delete_group_button = self._styled_button(groups_actions, "Delete", self.delete_group, danger=True)
        self.delete_group_button.grid(row=1, column=1, sticky="ew")

        self.group_listbox = self._styled_listbox(groups_wrap, height=10)
        self.group_listbox.grid(row=1, column=0, sticky="nsew", padx=10, pady=(0, 10))
        self.group_listbox.bind("<<ListboxSelect>>", self.on_group_select)
        self.group_listbox.bind("<Button-3>", self.on_group_right_click)

        test_wrap = self._styled_frame(self.groups_pane, role="panel_2")
        test_wrap.grid_rowconfigure(2, weight=1)
        test_wrap.grid_columnconfigure(0, weight=1)
        self._styled_label(test_wrap, "Test Area", font=(self.UI_FONT, 13, "bold")).grid(row=0, column=0, sticky="w", padx=10, pady=(10, 4))
        self._styled_label(test_wrap, "Type a trigger here and the engine should expand it.", muted=True).grid(row=1, column=0, sticky="w", padx=10)
        self.test_area = self._styled_text(test_wrap, height=10, font=(self.AR_FONT, 10))
        self.test_area.grid(row=2, column=0, sticky="nsew", padx=10, pady=10)
        self.test_area.bind("<KeyRelease>", self._on_test_area_key_release, add="+")
        self.test_area.bind("<KeyRelease>", self._scroll_test_area_to_end, add="+")
        self.test_area.bind("<ButtonRelease-1>", self._scroll_test_area_to_end, add="+")
        test_actions = self._styled_frame(test_wrap, role="panel_2", border=False)
        test_actions.grid(row=3, column=0, sticky="ew", padx=10, pady=(0, 10))
        test_actions.grid_columnconfigure(0, weight=1)
        self.reset_test_area_button = self._styled_button(test_actions, "Reset Test Area", self.reset_test_area)
        self.reset_test_area_button.grid(row=0, column=0, sticky="ew")

        self.groups_pane.add(groups_wrap, minsize=180, height=250)
        self.groups_pane.add(test_wrap, minsize=180, height=420)

        self.group_menu_popup = tk.Menu(self.root, tearoff=0)
        self.widgets.menus.append(self.group_menu_popup)

    def _build_snippets_panel(self):
        panel = self.snippets_panel
        header = self._styled_frame(panel, role="panel", border=False)
        header.pack(fill="x", padx=12, pady=(12, 8))
        self.snippets_title_label = self._styled_label(header, "Snippets", font=(self.UI_FONT, 18, "bold"))
        self.snippets_title_label.pack(side="left")
        self.search_entry = self._styled_entry(header, self.search_var, width=24)
        self.search_entry.pack(side="right")
        self.search_entry.bind("<FocusIn>", self._on_search_focus_in, add="+")
        self.search_entry.bind("<FocusOut>", self._on_search_focus_out, add="+")
        self.search_entry.bind("<KeyRelease>", self._on_search_key_release, add="+")
        self._update_search_placeholder()

        tree_wrap = self._styled_frame(panel, role="panel_2")
        tree_wrap.pack(fill="both", expand=True, padx=12, pady=12)
        tree_wrap.grid_rowconfigure(0, weight=1)
        tree_wrap.grid_columnconfigure(0, weight=1)

        columns = ("trigger", "group", "enabled", "notes", "preview")
        self.snippet_tree = ttk.Treeview(tree_wrap, columns=columns, show="headings", selectmode="browse", style="BT.Treeview")
        self.widgets.treeviews.append(self.snippet_tree)
        self.snippet_tree.grid(row=0, column=0, sticky="nsew")
        tree_scroll = ttk.Scrollbar(tree_wrap, orient="vertical", command=self.snippet_tree.yview, style="BT.Vertical.TScrollbar")
        tree_scroll.grid(row=0, column=1, sticky="ns")
        tree_xscroll = ttk.Scrollbar(tree_wrap, orient="horizontal", command=self.snippet_tree.xview, style="BT.Horizontal.TScrollbar")
        tree_xscroll.grid(row=1, column=0, sticky="ew")
        self.widgets.scrollbars.extend([tree_scroll, tree_xscroll])
        self.snippet_tree.configure(yscrollcommand=tree_scroll.set, xscrollcommand=tree_xscroll.set)
        headings = {
            "trigger": "Trigger",
            "group": "Group",
            "enabled": "On",
            "notes": "Notes",
            "preview": "Preview",
        }
        widths = {"trigger": 110, "group": 120, "enabled": 70, "notes": 180, "preview": 620}
        anchors = {"trigger": "center", "group": "center", "enabled": "center", "notes": "center", "preview": "w"}
        for key in columns:
            self.snippet_tree.heading(key, text=headings[key], anchor="center", command=lambda c=key: self.sort_snippets_by(c))
            self.snippet_tree.column(key, width=widths[key], anchor=anchors[key], stretch=(key == "preview"))
        self.snippet_tree.bind("<<TreeviewSelect>>", self.on_snippet_select)
        self.snippet_tree.bind("<Double-1>", lambda _e: self.edit_selected_snippet())
        self.snippet_tree.bind("<Shift-MouseWheel>", self._on_snippet_tree_shift_mousewheel, add="+")
        self.snippet_tree.bind("<Button-3>", self.on_snippet_right_click)

        self.snippet_menu_popup = tk.Menu(self.root, tearoff=0)
        self.widgets.menus.append(self.snippet_menu_popup)

        bottom = self._styled_frame(panel, role="panel", border=False)
        bottom.pack(fill="x", padx=12, pady=(0, 12))
        for col in range(5):
            bottom.grid_columnconfigure(col, weight=1)
        self.edit_snippet_button = self._styled_button(bottom, "Edit Snippet", self.edit_selected_snippet)
        self.edit_snippet_button.grid(row=0, column=0, sticky="ew", padx=(0, 8))
        self.duplicate_snippet_button = self._styled_button(bottom, "Duplicate", self.duplicate_snippet)
        self.duplicate_snippet_button.grid(row=0, column=1, sticky="ew", padx=(0, 8))
        self.toggle_snippet_button = self._styled_button(bottom, "Toggle Snippet", self._snippet_toggle_selected)
        self.toggle_snippet_button.grid(row=0, column=2, sticky="ew", padx=(0, 8))
        self.delete_snippet_button = self._styled_button(bottom, "Delete Snippet", self.delete_snippet, danger=True)
        self.delete_snippet_button.grid(row=0, column=3, sticky="ew", padx=(0, 8))
        self.new_snippet_button = self._styled_button(bottom, "New", self.new_snippet)
        self.new_snippet_button.grid(row=0, column=4, sticky="ew")

    def _build_editor_panel(self):
        panel = self.editor_panel
        self.editor_inner = self._styled_frame(panel, role="panel", border=False)
        self.editor_inner.pack(fill="both", expand=True)

        inner = self.editor_inner
        self.editor_title_label = self._styled_label(inner, "Snippets Manager", font=(self.UI_FONT, 18, "bold"))
        self.editor_title_label.pack(anchor="w", padx=12, pady=(12, 6))

        self.editor_tab_row = self._styled_frame(inner, role="panel", border=False)
        self.editor_tab_row.pack(fill="x", padx=12, pady=(0, 8))
        self.editor_tab_row.grid_columnconfigure(0, weight=1)
        self.editor_tab_row.grid_columnconfigure(1, weight=1)
        self.snippet_manager_tab_button = self._styled_editor_tab_button(self.editor_tab_row, "Snippets Manager", "snippet")
        self.snippet_manager_tab_button.grid(row=0, column=0, sticky="ew", padx=(0, 6))
        self.engine_settings_tab_button = self._styled_editor_tab_button(self.editor_tab_row, "Engine Settings", "engine")
        self.engine_settings_tab_button.grid(row=0, column=1, sticky="ew", padx=(6, 0))

        self.editor_notebook = ttk.Notebook(inner, style="BT.Tabless.TNotebook")
        self.editor_notebook.pack(fill="both", expand=True, padx=12, pady=(0, 12))
        self.editor_notebook.bind("<<NotebookTabChanged>>", self._on_editor_tab_changed, add="+")

        self.snippet_tab = self._styled_frame(self.editor_notebook, role="panel", border=False)
        self.engine_tab = self._styled_frame(self.editor_notebook, role="panel", border=False)
        self.editor_notebook.add(self.snippet_tab, text="Snippets Manager")
        self.editor_notebook.add(self.engine_tab, text="Engine Settings")
        self._refresh_editor_tab_buttons()

        form = self._styled_frame(self.snippet_tab, role="panel", border=False)
        form.pack(fill="both", expand=True, padx=2, pady=2)

        self._styled_label(form, "Trigger").pack(anchor="w")
        self.trigger_entry = self._styled_entry(form, self.trigger_var)
        self.trigger_entry.pack(fill="x", pady=(4, 10))

        self._styled_label(form, "Group").pack(anchor="w")
        self.group_combo = self._styled_combobox(form, self.group_var)
        self.group_combo.pack(fill="x", pady=(4, 8))

        self.enabled_check = self._styled_checkbutton(form, "Snippet enabled", self.enabled_var)
        self.enabled_check.pack(anchor="w", pady=(0, 12))

        self._styled_label(form, "Notes").pack(anchor="w")
        self.notes_entry = self._styled_entry(form, self.notes_var)
        self.notes_entry.pack(fill="x", pady=(4, 10))

        self._styled_label(form, "Content").pack(anchor="w")
        self.content_text = self._styled_text(form, height=16, font=(self.AR_FONT, 10))
        self.content_text.pack(fill="both", expand=True, pady=(4, 12))
        self.content_text.bind("<Button-3>", self.on_content_editor_right_click)

        self.content_menu_popup = tk.Menu(self.root, tearoff=0)
        self.content_variables_menu = tk.Menu(self.content_menu_popup, tearoff=0)
        self.content_variables_datetime_menu = tk.Menu(self.content_variables_menu, tearoff=0)
        self.content_variables_combo_menu = tk.Menu(self.content_variables_menu, tearoff=0)
        self.content_unicode_menu = tk.Menu(self.content_menu_popup, tearoff=0)
        self.widgets.menus.extend(
            [
                self.content_menu_popup,
                self.content_variables_menu,
                self.content_variables_datetime_menu,
                self.content_variables_combo_menu,
                self.content_unicode_menu,
            ]
        )

        action_row = self._styled_frame(form, role="panel", border=False)
        action_row.pack(fill="x", pady=(0, 4))
        action_row.grid_columnconfigure(0, weight=1)
        action_row.grid_columnconfigure(1, weight=1)
        action_row.grid_columnconfigure(2, weight=1)
        self.editor_new_button = self._styled_button(action_row, "New", self.new_snippet)
        self.editor_new_button.grid(row=0, column=0, sticky="ew", padx=(0, 8))
        self.save_button = self._styled_button(action_row, "Save", self.save_snippet)
        self.save_button.grid(row=0, column=1, sticky="ew", padx=(0, 8))
        self.reset_button = self._styled_button(action_row, "Reset", self.reset_editor)
        self.reset_button.grid(row=0, column=2, sticky="ew")

        settings_box = self._styled_frame(self.engine_tab, role="panel_2")
        settings_box.pack(fill="both", expand=True, padx=2, pady=2)
        self._styled_label(settings_box, "Engine settings", font=(self.UI_FONT, 12, "bold"), role="panel_2").pack(anchor="w", padx=10, pady=(10, 6))
        restore_row = self._styled_frame(settings_box, role="panel_2", border=False)
        restore_row.pack(fill="x", padx=10, pady=(0, 8))
        self._styled_label(restore_row, "Restore clipboard (ms)", role="panel_2").pack(side="left")
        self.restore_entry_settings = self._styled_entry(restore_row, self.restore_delay_var, width=8, font=(self.MONO_FONT, 10))
        self.restore_entry_settings.pack(side="left", padx=(10, 0))
        top_flags_row = self._styled_frame(settings_box, role="panel_2", border=False)
        top_flags_row.pack(fill="x", padx=10, pady=(0, 2))
        self.always_on_top_check_settings = self._styled_checkbutton(top_flags_row, "Always on top", self.always_on_top_var, self._set_always_on_top)
        self.always_on_top_check_settings.pack(anchor="w")
        self.startup_check_settings = self._styled_checkbutton(top_flags_row, "Start with Windows", self.start_with_windows_var, self.toggle_start_with_windows)
        self.startup_check_settings.pack(anchor="w", pady=(4, 0))
        second_flags_row = self._styled_frame(settings_box, role="panel_2", border=False)
        second_flags_row.pack(fill="x", padx=10, pady=(0, 8))
        self.tray_check_settings = self._styled_checkbutton(second_flags_row, "Minimize to tray", self.minimize_to_tray_var, self._save_settings_only_silent)
        self.tray_check_settings.pack(anchor="w")
        self.case_check = self._styled_checkbutton(second_flags_row, "Case sensitive matching", self.case_sensitive_var)
        self.case_check.pack(anchor="w", pady=(4, 0))
        previous_row = self._styled_frame(settings_box, role="panel_2", border=False)
        previous_row.pack(fill="x", padx=10, pady=(0, 8))
        self.previous_clipboard_check = self._styled_checkbutton(previous_row, "Use \\\\ for previous clipboard item", self.previous_clipboard_var)
        self.previous_clipboard_check.pack(anchor="w")
        self.previous_clipboard_slash_check = self._styled_checkbutton(previous_row, "Use // for previous clipboard item", self.previous_clipboard_slash_var)
        self.previous_clipboard_slash_check.pack(anchor="w", pady=(2, 0))
        self._styled_label(settings_box, "Trigger mode", role="panel_2").pack(anchor="w", padx=10, pady=(8, 2))
        rb_row = self._styled_frame(settings_box, role="panel_2", border=False)
        rb_row.pack(fill="x", padx=10)
        self.instant_radio = self._styled_radiobutton(rb_row, "Instant", self.mode_var, "instant")
        self.instant_radio.pack(side="left", padx=(0, 12))
        self.separator_radio = self._styled_radiobutton(rb_row, "Separator", self.mode_var, "separator")
        self.separator_radio.pack(side="left")
        self._styled_label(settings_box, "Separator keys for separator mode", role="panel_2").pack(anchor="w", padx=10, pady=(8, 2))
        sep_row = self._styled_frame(settings_box, role="panel_2", border=False)
        sep_row.pack(fill="x", padx=10, pady=(0, 10))
        self.sep_space_check = self._styled_checkbutton(sep_row, "Space", self.sep_space_var)
        self.sep_space_check.pack(side="left", padx=(0, 10))
        self.sep_enter_check = self._styled_checkbutton(sep_row, "Enter", self.sep_enter_var)
        self.sep_enter_check.pack(side="left", padx=(0, 10))
        self.sep_tab_check = self._styled_checkbutton(sep_row, "Tab", self.sep_tab_var)
        self.sep_tab_check.pack(side="left")
        self._styled_label(settings_box, "Global toggle hotkey", role="panel_2").pack(anchor="w", padx=10, pady=(8, 2))
        hotkey_row = self._styled_frame(settings_box, role="panel_2", border=False)
        hotkey_row.pack(fill="x", padx=10)
        hotkey_row.grid_columnconfigure(0, weight=1)
        self.hotkey_entry = self._styled_entry(hotkey_row, self.hotkey_var, font=(self.MONO_FONT, 10))
        self.hotkey_entry.grid(row=0, column=0, sticky="ew", padx=(0, 8))
        self.hotkey_entry.configure(state="readonly")
        self.hotkey_capture_button = self._styled_button(hotkey_row, "Record Hotkey", self.toggle_hotkey_capture)
        self.hotkey_capture_button.grid(row=0, column=1, sticky="ew")
        self._styled_label(
            settings_box,
            "Click Record Hotkey, then press the shortcut directly.",
            muted=True,
            role="panel_2",
        ).pack(anchor="w", padx=10, pady=(4, 8))
        settings_action_row = self._styled_frame(settings_box, role="panel_2", border=False)
        settings_action_row.pack(side="bottom", fill="x", padx=10, pady=(12, 10))
        settings_action_row.grid_columnconfigure(0, weight=1)
        settings_action_row.grid_columnconfigure(1, weight=1)
        self.settings_save_button = self._styled_button(settings_action_row, "Save Settings", self.save_engine_settings)
        self.settings_save_button.grid(row=0, column=0, sticky="ew", padx=(0, 8))
        self.settings_reset_button = self._styled_button(settings_action_row, "Reset Settings", self.reset_engine_settings)
        self.settings_reset_button.grid(row=0, column=1, sticky="ew")

        self.status_box = tk.Label(panel, textvariable=self.status_var, anchor="w", justify="left", bd=0, padx=10, pady=10)
        self.status_box._theme_role = "status"
        self.status_box.configure(font=(self.MONO_FONT, 10))
        self.widgets.labels.append(self.status_box)
        self.status_box.pack(fill="x", padx=12, pady=(0, 12))

    def _on_editor_inner_configure(self, _event=None):
        return

    def _on_editor_canvas_configure(self, event):
        self._schedule_layout_refresh()

    def _on_mousewheel(self, event):
        return


    def _bind_app_keyboard_helpers(self):
        self.root.bind_all("<FocusIn>", self._on_any_focus_in, add="+")
        self.root.bind("<FocusOut>", self._on_root_focus_out, add="+")
        self.root.bind_all("<Control-KeyPress>", self._on_control_keypress, add="+")
        self.root.bind("<KeyPress>", self._on_hotkey_capture_keypress, add="+")
        self.root.bind("<KeyRelease>", self._on_hotkey_capture_keyrelease, add="+")

    def _bind_edit_context_menus(self):
        self.edit_context_menu_popup = tk.Menu(self.root, tearoff=0)
        self.edit_unicode_menu_popup = tk.Menu(self.edit_context_menu_popup, tearoff=0)
        self.widgets.menus.append(self.edit_context_menu_popup)
        self.widgets.menus.append(self.edit_unicode_menu_popup)
        self._edit_context_target = None
        bindings = [
            self.search_entry,
            self.trigger_entry,
            self.notes_entry,
            self.test_area,
            self.restore_entry_settings,
            self.hotkey_entry,
        ]
        for widget in bindings:
            widget.bind("<Button-3>", self.on_generic_edit_right_click, add="+")

    def _on_any_focus_in(self, event=None):
        widget = getattr(event, "widget", None)
        if widget is self.test_area:
            self.engine.set_app_focus_area("test")
        else:
            self.engine.set_app_focus_area("app")

    def _on_root_focus_out(self, _event=None):
        def check_focus():
            try:
                if self.root.focus_displayof() is None:
                    self.engine.set_app_focus_area("external")
            except Exception:
                self.engine.set_app_focus_area("external")
        self.root.after(80, check_focus)

    def _focused_text_widget(self, widget):
        try:
            if isinstance(widget, (tk.Text, tk.Entry)):
                return widget
        except Exception:
            pass
        return None

    def _delete_selected_text(self, widget):
        try:
            if isinstance(widget, tk.Text):
                if widget.tag_ranges("sel"):
                    widget.delete("sel.first", "sel.last")
                    if widget is self.test_area:
                        self._local_test_buffer = ""
                return
            if isinstance(widget, tk.Entry):
                try:
                    if widget.selection_present():
                        widget.delete("sel.first", "sel.last")
                except Exception:
                    pass
        except Exception:
            pass

    def _refresh_edit_context_menu(self, widget):
        self.edit_context_menu_popup.delete(0, "end")
        self.edit_context_menu_popup.add_command(label="Undo", command=lambda w=widget: self._apply_text_shortcut(w, "undo"))
        self.edit_context_menu_popup.add_separator()
        self.edit_context_menu_popup.add_command(label="Cut", command=lambda w=widget: self._apply_text_shortcut(w, "cut"))
        self.edit_context_menu_popup.add_command(label="Copy", command=lambda w=widget: self._apply_text_shortcut(w, "copy"))
        self.edit_context_menu_popup.add_command(label="Paste", command=lambda w=widget: self._apply_text_shortcut(w, "paste"))
        self.edit_context_menu_popup.add_command(label="Delete", command=lambda w=widget: self._delete_selected_text(w))
        self.edit_context_menu_popup.add_separator()
        self.edit_context_menu_popup.add_command(label="Select All", command=lambda w=widget: self._apply_text_shortcut(w, "select_all"))
        self._add_advanced_text_menu_items(self.edit_context_menu_popup, widget, self.edit_unicode_menu_popup)

    def on_generic_edit_right_click(self, event):
        widget = getattr(event, "widget", None)
        if widget is None:
            return
        try:
            widget.focus_set()
        except Exception:
            pass
        if isinstance(widget, tk.Entry):
            try:
                widget.icursor(f"@{event.x}")
            except Exception:
                pass
        elif isinstance(widget, tk.Text):
            try:
                widget.mark_set("insert", f"@{event.x},{event.y}")
            except Exception:
                pass
        self._refresh_edit_context_menu(widget)
        self._popup_menu(self.edit_context_menu_popup, event.x_root, event.y_root)

    def _on_control_keypress(self, event):
        if self._hotkey_capture_active:
            return "break"
        widget = self._focused_text_widget(getattr(event, "widget", None))
        if widget is None:
            return None
        keycode = int(getattr(event, "keycode", 0) or 0)
        keysym = str(getattr(event, "keysym", "") or "").lower()
        action = None
        if keycode == 65 or keysym == "a":
            action = "select_all"
        elif keycode == 67 or keysym == "c":
            action = "copy"
        elif keycode == 86 or keysym == "v":
            action = "paste"
        elif keycode == 88 or keysym == "x":
            action = "cut"
        elif keycode == 90 or keysym == "z":
            action = "undo"
        if not action:
            return None
        self._apply_text_shortcut(widget, action)
        return "break"

    def _apply_text_shortcut(self, widget, action: str):
        try:
            if isinstance(widget, tk.Text):
                if action == "select_all":
                    widget.tag_add("sel", "1.0", "end-1c")
                    widget.mark_set("insert", "end-1c")
                    widget.see("insert")
                elif action == "copy":
                    if widget.tag_ranges("sel"):
                        self.root.clipboard_clear()
                        self.root.clipboard_append(widget.get("sel.first", "sel.last"))
                elif action == "cut":
                    if widget.tag_ranges("sel"):
                        self.root.clipboard_clear()
                        self.root.clipboard_append(widget.get("sel.first", "sel.last"))
                        widget.delete("sel.first", "sel.last")
                elif action == "paste":
                    text = self.root.clipboard_get()
                    if widget.tag_ranges("sel"):
                        widget.delete("sel.first", "sel.last")
                    widget.insert("insert", text)
                    if widget is self.test_area:
                        self._local_test_buffer = ""
                        self._scroll_test_area_to_end()
                elif action == "undo":
                    try:
                        widget.edit_undo()
                    except Exception:
                        pass
                return
            if isinstance(widget, tk.Entry):
                if action == "select_all":
                    widget.selection_range(0, "end")
                    widget.icursor("end")
                elif action == "copy":
                    try:
                        if widget.selection_present():
                            self.root.clipboard_clear()
                            self.root.clipboard_append(widget.selection_get())
                    except Exception:
                        pass
                elif action == "cut":
                    try:
                        if widget.selection_present():
                            self.root.clipboard_clear()
                            self.root.clipboard_append(widget.selection_get())
                            widget.delete("sel.first", "sel.last")
                    except Exception:
                        pass
                elif action == "paste":
                    text = self.root.clipboard_get()
                    try:
                        if widget.selection_present():
                            widget.delete("sel.first", "sel.last")
                    except Exception:
                        pass
                    widget.insert("insert", text)
                elif action == "undo":
                    try:
                        widget.event_generate("<<Undo>>")
                    except Exception:
                        pass
        except Exception:
            pass

    def _test_area_find_exact_match(self, current_word: str):
        normalized_word = self.engine.normalized(current_word)
        for snippet in self.engine.get_enabled_snippets():
            if self.engine.normalized(snippet.get("trigger", "")) == normalized_word:
                return snippet
        return None

    def _test_area_find_suffix_match(self, current_buffer: str):
        normalized_buffer = self.engine.normalized(current_buffer)
        for snippet in self.engine.get_enabled_snippets():
            trigger = snippet.get("trigger", "")
            if trigger and normalized_buffer.endswith(self.engine.normalized(trigger)):
                return snippet
        return None

    def _replace_test_trigger(self, snippet, delete_count: int):
        trigger = snippet.get("trigger", "")
        try:
            clipboard_snapshot = self.engine._read_clipboard()
            plan = self.engine.build_expansion_plan(snippet, clipboard_snapshot, clipboard_snapshot)
            if plan.aborted:
                self.update_status("Expansion cancelled")
                return
            start_index = self.test_area.index(f"insert-{delete_count}c")
            rendered, caret_position = self.engine.render_expansion_plan_for_test_area(plan)
            self.test_area.delete(start_index, "insert")
            self.test_area.insert(start_index, rendered)
            final_index = f"{start_index}+{(caret_position if caret_position is not None else len(rendered))}c"
            self.test_area.mark_set("insert", final_index)
            self.test_area.see("insert")
            self._local_test_buffer = ""
            self.update_status(f"Expanded in Test Area: {trigger}")
            self._scroll_test_area_to_end()
        except Exception as exc:
            self.update_status(f"Test expansion error: {exc}")

    def _text_before_test_cursor(self) -> str:
        try:
            return self.test_area.get("1.0", "insert")
        except Exception:
            return ""

    def _test_area_find_suffix_in_text(self, text: str):
        if self.previous_clipboard_var.get() and text.endswith(PREVIOUS_CLIPBOARD_TRIGGER) and self.engine.previous_clipboard_text:
            return self.engine._get_previous_clipboard_snippet(PREVIOUS_CLIPBOARD_TRIGGER)
        if self.previous_clipboard_slash_var.get() and text.endswith(PREVIOUS_CLIPBOARD_SLASH_TRIGGER) and self.engine.previous_clipboard_text:
            return self.engine._get_previous_clipboard_snippet(PREVIOUS_CLIPBOARD_SLASH_TRIGGER)
        normalized_text = self.engine.normalized(text)
        for snippet in self.engine.get_enabled_snippets():
            trigger = snippet.get("trigger", "")
            if trigger and normalized_text.endswith(self.engine.normalized(trigger)):
                return snippet
        return None

    def _on_test_area_key_release(self, event):
        if not self.config_manager.data["settings"].get("engine_enabled", True):
            return None
        if getattr(event, "state", 0) & 0x0004:
            return None

        keysym = str(getattr(event, "keysym", "") or "")
        mode = self.config_manager.data["settings"].get("trigger_mode", "instant")

        if keysym in {"Left", "Right", "Up", "Down", "Home", "End", "Prior", "Next", "Delete", "Escape", "BackSpace"}:
            self._local_test_buffer = ""
            return None

        before = self._text_before_test_cursor()

        if keysym in {"space", "Return", "Tab"}:
            sep_name = {"space": "space", "Return": "enter", "Tab": "tab"}[keysym]
            if mode == "separator" and sep_name in self.config_manager.data["settings"].get("word_separators", []):
                # KeyRelease happens after the separator was inserted. Match the
                # trigger immediately before that inserted separator and delete
                # both the trigger and the separator. This works with Arabic too
                # because Tk gives us the real text in the Text widget.
                text_without_sep = before[:-1] if before else ""
                snippet = self._test_area_find_suffix_in_text(text_without_sep)
                if snippet:
                    delete_count = len(snippet.get("trigger", "")) + 1
                    self.root.after_idle(lambda s=snippet, n=delete_count: self._replace_test_trigger(s, n))
            self._local_test_buffer = ""
            return None

        if mode == "instant":
            snippet = self._test_area_find_suffix_in_text(before)
            if snippet:
                delete_count = len(snippet.get("trigger", ""))
                self.root.after_idle(lambda s=snippet, n=delete_count: self._replace_test_trigger(s, n))
        return None

    def _on_snippet_tree_shift_mousewheel(self, event):
        steps = max(1, int(abs(event.delta) / 30))
        direction = -1 if event.delta > 0 else 1
        self.snippet_tree.xview_scroll(direction * steps, "units")
        return "break"

    def _scroll_test_area_to_end(self, _event=None):
        self.root.after_idle(lambda: self.test_area.see("end-1c"))

    def reset_test_area(self):
        self.test_area.delete("1.0", "end")
        self._local_test_buffer = ""
        self.update_status("Test Area cleared")

    def _apply_windows_titlebar_theme(self):
        """
        Keep the native Windows title bar stable.

        Tkinter + DWM dark title-bar styling can make the minimize/maximize
        buttons fail to paint on first launch on some Windows builds. The
        app body still follows Dark/Light themes; the native title bar is
        left to Windows so all three buttons render reliably.
        """
        if os.name != "nt":
            return
        try:
            self.root.update_idletasks()
            hwnd = ctypes.windll.user32.GetParent(self.root.winfo_id()) or self.root.winfo_id()

            # Explicitly disable immersive dark titlebar for stability.
            value = ctypes.c_int(0)
            for attr in (20, 19):
                try:
                    ctypes.windll.dwmapi.DwmSetWindowAttribute(
                        hwnd, attr, ctypes.byref(value), ctypes.sizeof(value)
                    )
                except Exception:
                    pass

            # Force non-client redraw without minimizing/restoring the window.
            try:
                # SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE
                ctypes.windll.user32.SetWindowPos(hwnd, 0, 0, 0, 0, 0, 0x0027)
                # RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME
                ctypes.windll.user32.RedrawWindow(hwnd, None, None, 0x0001 | 0x0100 | 0x0400)
            except Exception:
                pass
        except Exception:
            pass

    def _stabilize_window_chrome_on_startup(self):
        """Run after the window appears to force the native title buttons to repaint."""
        if os.name != "nt":
            return
        def step():
            try:
                self.root.update_idletasks()
                self._apply_windows_titlebar_theme()
            except Exception:
                pass
        for delay in (0, 80, 250, 600):
            self.root.after(delay, step)

    def _is_descendant(self, widget, ancestor):
        current = widget
        while current is not None:
            if current == ancestor:
                return True
            current = current.master
        return False

    def _treeview_style(self):
        style = ttk.Style()
        style.theme_use("default")
        style.configure(
            "BT.Treeview",
            background=self.theme["list_bg"],
            fieldbackground=self.theme["list_bg"],
            foreground=self.theme["text"],
            rowheight=24,
            bordercolor=self.theme["border"],
            lightcolor=self.theme["border"],
            darkcolor=self.theme["border"],
            font=(self.AR_FONT, 10),
        )
        style.configure(
            "BT.Treeview.Heading",
            background=self.theme["button"],
            foreground=self.theme["button_text"],
            relief="flat",
            borderwidth=0,
            font=(self.UI_FONT, 10, "bold"),
            anchor="center",
        )
        style.map("BT.Treeview", background=[("selected", self.theme["select"])], foreground=[("selected", "#FFFFFF")])
        style.configure(
            "BT.Vertical.TScrollbar",
            background=self.theme["button"],
            troughcolor=self.theme["panel_2"],
            bordercolor=self.theme["border"],
            arrowcolor=self.theme["text"],
            darkcolor=self.theme["button"],
            lightcolor=self.theme["button"],
            gripcount=0,
        )
        style.map("BT.Vertical.TScrollbar", background=[("active", self.theme["button_hover"]), ("!active", self.theme["button"])])
        style.configure(
            "BT.Horizontal.TScrollbar",
            background=self.theme["button"],
            troughcolor=self.theme["panel_2"],
            bordercolor=self.theme["border"],
            arrowcolor=self.theme["text"],
            darkcolor=self.theme["button"],
            lightcolor=self.theme["button"],
            gripcount=0,
        )
        style.map("BT.Horizontal.TScrollbar", background=[("active", self.theme["button_hover"]), ("!active", self.theme["button"])])
        style.map(
            "BT.Treeview",
            background=[("selected", self.theme["select"])],
            foreground=[("selected", self.theme["text"])],
        )
        style.map(
            "BT.Treeview.Heading",
            background=[("active", self.theme["button_hover"])],
            foreground=[("active", self.theme["button_text"])],
        )
        style.configure(
            "BT.TNotebook",
            background=self.theme["panel"],
            borderwidth=0,
            tabmargins=(0, 0, 0, 0),
        )
        style.configure(
            "BT.Tabless.TNotebook",
            background=self.theme["panel"],
            borderwidth=0,
            tabmargins=(0, 0, 0, 0),
        )
        try:
            style.layout("BT.Tabless.TNotebook.Tab", [])
        except Exception:
            pass
        style.configure(
            "BT.TNotebook.Tab",
            background=self.theme["button"],
            foreground=self.theme["button_text"],
            padding=(16, 8),
            borderwidth=0,
            font=(self.UI_FONT, 10, "bold"),
        )
        style.map(
            "BT.TNotebook.Tab",
            background=[("selected", self.theme["select"]), ("active", self.theme["button_hover"])],
            foreground=[("selected", "#FFFFFF"), ("active", self.theme["button_text"])],
        )
        style.configure(
            "BT.TCombobox",
            fieldbackground=self.theme["entry_bg"],
            foreground=self.theme["entry_fg"],
            background=self.theme["button"],
            arrowcolor=self.theme["entry_fg"],
            bordercolor=self.theme["border"],
            lightcolor=self.theme["border"],
            darkcolor=self.theme["border"],
            insertcolor=self.theme["entry_fg"],
            padding=(8, 4),
            relief="flat",
        )
        style.map(
            "BT.TCombobox",
            fieldbackground=[("readonly", self.theme["entry_bg"])],
            foreground=[("readonly", self.theme["entry_fg"])],
            background=[("readonly", self.theme["button"]), ("active", self.theme["button_hover"])],
            arrowcolor=[("readonly", self.theme["entry_fg"]), ("active", self.theme["entry_fg"])],
            bordercolor=[("focus", self.theme["select"]), ("!focus", self.theme["border"])],
            selectbackground=[("readonly", self.theme["select"])],
            selectforeground=[("readonly", self.theme["text"])],
        )
        self.snippet_tree.configure(style="BT.Treeview")

    def _apply_theme(self):
        self.theme = THEMES[self.theme_name]
        self.root.configure(bg=self.theme["bg"])
        self._apply_windows_titlebar_theme()
        self.root.after(80, self._apply_windows_titlebar_theme)
        self.root.after(180, self._apply_windows_titlebar_theme)
        for frame in self.widgets.frames:
            role = getattr(frame, "_theme_role", "inherit")
            bg = self.theme["bg"] if role == "bg" else self.theme.get(role, self.theme["panel"])
            border_color = self.theme["border"]
            if role == "header":
                border_color = self.theme["header"]
            try:
                frame.configure(bg=bg, highlightbackground=border_color, highlightcolor=border_color)
            except Exception:
                pass

        for lbl in self.widgets.labels:
            role = getattr(lbl, "_theme_role", getattr(lbl.master, "_theme_role", "panel"))
            bg = self.theme["status"] if role == "status" else self.theme["bg"] if role == "bg" else self.theme.get(role, self.theme["panel"])
            fg = self.theme["muted"] if getattr(lbl, "_theme_muted", False) else self.theme["text"]
            if role == "status":
                fg = self.theme["text"]
            try:
                lbl.configure(bg=bg, fg=fg, highlightbackground=self.theme["border"])
            except Exception:
                pass

        for btn in self.widgets.buttons:
            if getattr(btn, "_editor_tab_button", False):
                if getattr(btn, "_editor_tab_active", False):
                    bg = self.theme["select"]
                    fg = "#FFFFFF"
                    active = self.theme["select"]
                else:
                    bg = self.theme["button"]
                    fg = self.theme["button_text"]
                    active = self.theme["button_hover"]
            elif getattr(btn, "_accent", False):
                bg = self.theme["accent"]
                fg = "#FFFFFF"
                active = self.theme["accent"]
            elif getattr(btn, "_danger", False):
                bg = self.theme["danger"]
                fg = "#FFFFFF"
                active = self.theme["danger"]
            else:
                bg = self.theme["button"]
                fg = self.theme["button_text"]
                active = self.theme["button_hover"]
            try:
                highlight = self.theme["select"] if getattr(btn, "_editor_tab_active", False) else self.theme["border_soft"]
                btn.configure(bg=bg, fg=fg, activebackground=active, activeforeground=fg, highlightbackground=highlight)
                if getattr(btn, "_icon_only", False):
                    btn.configure(padx=6, pady=6)
            except Exception:
                pass

        for ent in self.widgets.entries:
            try:
                ent.configure(
                    bg=self.theme["entry_bg"],
                    fg=self.theme["entry_fg"],
                    insertbackground=self.theme["entry_fg"],
                    highlightbackground=self.theme["border"],
                    highlightcolor=self.theme["select"],
                    readonlybackground=self.theme["entry_bg"],
                    disabledbackground=self.theme["entry_bg"],
                    disabledforeground=self.theme["entry_fg"],
                )
            except Exception:
                pass

        for combo in self.widgets.comboboxes:
            try:
                combo.configure(style="BT.TCombobox")
            except Exception:
                pass

        for txt in self.widgets.texts:
            try:
                txt.configure(
                    bg=self.theme["entry_bg"],
                    fg=self.theme["entry_fg"],
                    insertbackground=self.theme["entry_fg"],
                    highlightbackground=self.theme["border"],
                    highlightcolor=self.theme["select"],
                    selectbackground=self.theme["select"],
                    selectforeground=self.theme["text"],
                )
            except Exception:
                pass

        for lb in self.widgets.listboxes:
            try:
                lb.configure(
                    bg=self.theme["list_bg"],
                    fg=self.theme["text"],
                    selectbackground=self.theme["select"],
                    selectforeground=self.theme["text"],
                    highlightbackground=self.theme["border"],
                    highlightcolor=self.theme["select"],
                )
            except Exception:
                pass

        for cb in self.widgets.checkbuttons:
            role = getattr(cb, "_theme_role", getattr(cb.master, "_theme_role", "panel"))
            bg = self.theme.get(role, self.theme["panel"])
            try:
                cb.configure(bg=bg, fg=self.theme["text"], selectcolor=bg, activebackground=bg, activeforeground=self.theme["text"])
            except Exception:
                pass

        for rb in self.widgets.radiobuttons:
            role = getattr(rb, "_theme_role", getattr(rb.master, "_theme_role", "panel"))
            bg = self.theme.get(role, self.theme["panel"])
            try:
                rb.configure(bg=bg, fg=self.theme["text"], selectcolor=bg, activebackground=bg, activeforeground=self.theme["text"])
            except Exception:
                pass

        for om in self.widgets.optionmenus:
            role = getattr(om, "_theme_role", getattr(om.master, "_theme_role", "panel"))
            bg = self.theme.get(role, self.theme["panel"])
            try:
                om.configure(bg=self.theme["button"], fg=self.theme["button_text"], activebackground=self.theme["button_hover"], activeforeground=self.theme["button_text"], highlightthickness=1, bd=0)
                menu = om["menu"]
                menu.configure(bg=self.theme["entry_bg"], fg=self.theme["entry_fg"], activebackground=self.theme["select"], activeforeground=self.theme["text"], bd=0)
            except Exception:
                pass

        for canvas in self.widgets.canvases:
            try:
                canvas.configure(bg=self.theme["panel"], sashrelief="flat")
            except Exception:
                pass
            try:
                canvas.configure(highlightbackground=self.theme["border"])
            except Exception:
                pass
        for sb in self.widgets.scrollbars:
            try:
                sb.configure(style="BT.Vertical.TScrollbar" if str(sb.cget("orient")) == "vertical" else "BT.Horizontal.TScrollbar")
            except Exception:
                try:
                    sb.configure(bg=self.theme["button"], troughcolor=self.theme["panel_2"], activebackground=self.theme["button_hover"], highlightbackground=self.theme["border"])
                except Exception:
                    pass
        for menu in self.widgets.menus:
            menu.configure(bg=self.theme["entry_bg"], fg=self.theme["entry_fg"], activebackground=self.theme["select"], activeforeground=self.theme["text"], bd=0)

        self._treeview_style()
        self.theme_button.configure(text=f"Theme: {'Dark' if self.theme_name == 'dark' else 'Light'}")
        self._refresh_engine_button()
        self._update_search_placeholder()
        self.root.update_idletasks()
        self._schedule_layout_refresh()

    def update_status(self, text: str):
        self.root.after(0, lambda: self.status_var.set(text))

    def _get_groups(self):
        return self.config_manager.data.get("groups", [])

    def _get_snippets(self):
        return self.config_manager.data.get("snippets", [])

    def _refresh_group_menu(self):
        groups = [g["name"] for g in self._get_groups()]
        if not groups:
            groups = ["General"]
        if hasattr(self, "group_combo"):
            self.group_combo.configure(values=groups)
        if self.group_var.get() not in groups:
            self.group_var.set(groups[0])

    def _load_groups(self):
        current = self.selected_group_index
        self.group_listbox.delete(0, "end")
        groups = self._get_groups()
        total = len(self._get_snippets())
        try:
            self.groups_title_label.configure(text=f"Groups ({len(groups)})")
        except Exception:
            pass
        self.group_listbox.insert("end", f"All Snippets ({total})")
        for group in groups:
            count = sum(1 for s in self._get_snippets() if s.get("group", "General") == group["name"])
            prefix = "" if group.get("enabled", True) else "[Off] "
            self.group_listbox.insert("end", f"{prefix}{group['name']} ({count})")
        if current >= self.group_listbox.size():
            current = 0
        self.group_listbox.selection_clear(0, "end")
        self.group_listbox.selection_set(current)
        self.selected_group_index = current
        self._refresh_group_menu()
        self._schedule_layout_refresh()

    def _selected_group_name(self):
        if self.selected_group_index <= 0:
            return None
        groups = self._get_groups()
        idx = self.selected_group_index - 1
        if 0 <= idx < len(groups):
            return groups[idx]["name"]
        return None

    def _matches_search(self, snippet: dict, query: str):
        if not query:
            return True
        q = query.lower()
        fields = [
            snippet.get("trigger", ""),
            snippet.get("notes", ""),
            snippet.get("content", "")[:1000],
        ]
        return any(q in str(value).lower() for value in fields)

    def _snippet_sort_value(self, snippet: dict, column: str):
        if column == "enabled":
            return 1 if snippet.get("enabled", True) else 0
        return str(snippet.get(column, "")).lower()

    def sort_snippets_by(self, column: str):
        if self.sort_column == column:
            self.sort_reverse = not self.sort_reverse
        else:
            self.sort_column = column
            self.sort_reverse = False
        self._load_snippets(keep_selection=True)

    def _load_snippets(self, keep_selection=False):
        selected_actual = self.selected_snippet_index if keep_selection else None
        self.snippet_tree_map = {}
        for iid in self.snippet_tree.get_children():
            self.snippet_tree.delete(iid)

        group_filter = self._selected_group_name()
        query = self._current_search_query()
        rows = []
        for index, snippet in enumerate(self._get_snippets()):
            if group_filter and snippet.get("group", "General") != group_filter:
                continue
            if not self._matches_search(snippet, query):
                continue
            rows.append((index, snippet))

        rows.sort(key=lambda item: self._snippet_sort_value(item[1], self.sort_column), reverse=self.sort_reverse)
        for index, snippet in rows:
            preview = snippet.get("content", "").replace("\n", " ⏎ ")[:180]
            iid = str(index)
            self.snippet_tree.insert("", "end", iid=iid, values=(
                snippet.get("trigger", ""),
                snippet.get("group", "General"),
                "Yes" if snippet.get("enabled", True) else "No",
                snippet.get("notes", ""),
                preview,
            ))
            self.snippet_tree_map[iid] = index

        if selected_actual is not None and str(selected_actual) in self.snippet_tree_map:
            self.snippet_tree.selection_set(str(selected_actual))
            self.snippet_tree.focus(str(selected_actual))
        elif self.selected_snippet_index is not None and str(self.selected_snippet_index) in self.snippet_tree_map:
            self.snippet_tree.selection_set(str(self.selected_snippet_index))
            self.snippet_tree.focus(str(self.selected_snippet_index))
        else:
            self.selected_snippet_index = None
        title = "Snippets"
        if group_filter:
            visible = len(rows)
            total_group = sum(1 for item in self._get_snippets() if item.get("group", "General") == group_filter)
            title = f"Snippets - {group_filter} ({visible})" if visible == total_group else f"Snippets - {group_filter} ({visible}/{total_group})"
        else:
            title = f"Snippets ({len(rows)})"
        try:
            self.snippets_title_label.configure(text=title)
        except Exception:
            pass
        self._schedule_layout_refresh()

    def on_group_select(self, _event=None):
        sel = self.group_listbox.curselection()
        if not sel:
            return
        self.selected_group_index = sel[0]
        self._load_snippets()

    def _popup_menu(self, menu, x_root, y_root):
        try:
            menu.tk_popup(x_root, y_root)
        finally:
            try:
                menu.grab_release()
            except Exception:
                pass

    def _show_group_menu_for_empty_area(self, event):
        self.group_menu_popup.delete(0, "end")
        self.group_menu_popup.add_command(label="New Group", command=self.new_group)
        self._popup_menu(self.group_menu_popup, event.x_root, event.y_root)

    def _show_group_menu_for_group(self, event, idx):
        self.group_listbox.selection_clear(0, "end")
        self.group_listbox.selection_set(idx)
        self.selected_group_index = idx
        group_name = self._selected_group_name()

        self.group_menu_popup.delete(0, "end")
        self.group_menu_popup.add_command(label="New Group", command=self.new_group)
        self.group_menu_popup.add_command(label="Rename Group", command=self.rename_group)
        self.group_menu_popup.add_command(label="Toggle Group", command=self.toggle_group)
        if group_name != "General":
            self.group_menu_popup.add_separator()
            self.group_menu_popup.add_command(label="Delete Group", command=self.delete_group)
        self._popup_menu(self.group_menu_popup, event.x_root, event.y_root)

    def _select_snippet_iid(self, iid):
        if not iid:
            return False
        actual_index = self.snippet_tree_map.get(iid)
        if actual_index is None:
            return False
        self.snippet_tree.selection_set(iid)
        self.snippet_tree.focus(iid)
        self.selected_snippet_index = actual_index
        self.on_snippet_select()
        return True

    def _snippet_toggle_selected(self):
        if self.selected_snippet_index is None:
            messagebox.showinfo(APP_TITLE, "Select a snippet first.")
            return
        snippets = self._get_snippets()
        if not (0 <= self.selected_snippet_index < len(snippets)):
            return
        snippet = snippets[self.selected_snippet_index]
        snippet["enabled"] = not bool(snippet.get("enabled", True))
        self.config_manager.save()
        self.engine.reload()
        self._load_snippets(keep_selection=True)
        state = "enabled" if snippet.get("enabled", True) else "disabled"
        self.update_status(f"Snippet {state}: {snippet.get('trigger', '')}")

    def _show_snippet_menu_for_empty_area(self, event):
        self.snippet_menu_popup.delete(0, "end")
        self.snippet_menu_popup.add_command(label="New", command=self.new_snippet)
        self._popup_menu(self.snippet_menu_popup, event.x_root, event.y_root)

    def _show_snippet_menu_for_item(self, event, iid):
        self._select_snippet_iid(iid)
        self.snippet_menu_popup.delete(0, "end")
        self.snippet_menu_popup.add_command(label="Edit Snippet", command=self.edit_selected_snippet)
        self.snippet_menu_popup.add_command(label="Duplicate", command=self.duplicate_snippet)
        self.snippet_menu_popup.add_command(label="Toggle Snippet", command=self._snippet_toggle_selected)
        self.snippet_menu_popup.add_command(label="Delete", command=self.delete_snippet)
        self.snippet_menu_popup.add_separator()
        self.snippet_menu_popup.add_command(label="New", command=self.new_snippet)
        self._popup_menu(self.snippet_menu_popup, event.x_root, event.y_root)

    def on_group_right_click(self, event):
        idx = self.group_listbox.nearest(event.y)

        # Listbox.nearest() returns the last item even when clicking below all items.
        # Compare the click position with the item bbox to know if the user clicked
        # an actual group row or empty space.
        if idx < 0 or idx >= self.group_listbox.size():
            self._show_group_menu_for_empty_area(event)
            return "break"

        bbox = self.group_listbox.bbox(idx)
        if not bbox:
            self._show_group_menu_for_empty_area(event)
            return "break"

        x, y, w, h = bbox
        if event.y < y or event.y > y + h:
            self._show_group_menu_for_empty_area(event)
            return "break"

        self._show_group_menu_for_group(event, idx)
        return "break"

    def on_snippet_right_click(self, event):
        region = self.snippet_tree.identify_region(event.x, event.y)
        iid = self.snippet_tree.identify_row(event.y)

        if region in {"heading", "separator"}:
            return None

        if iid:
            self._show_snippet_menu_for_item(event, iid)
        else:
            self._show_snippet_menu_for_empty_area(event)
        return "break"

    def on_snippet_select(self, _event=None):
        selection = self.snippet_tree.selection()
        if not selection:
            return
        iid = selection[0]
        actual_index = self.snippet_tree_map.get(iid)
        if actual_index is None:
            return
        self.selected_snippet_index = actual_index
        snippet = self._get_snippets()[actual_index]
        self.update_status(f"Selected snippet: {snippet.get('trigger', '')}")

    def edit_selected_snippet(self):
        if self.selected_snippet_index is None:
            messagebox.showinfo(APP_TITLE, "Select a snippet first.")
            return
        self.editing_snippet_index = self.selected_snippet_index
        self.editor_new_mode = False
        snippet = self._get_snippets()[self.editing_snippet_index]
        self.trigger_var.set(snippet.get("trigger", ""))
        self.group_var.set(snippet.get("group", "General"))
        self.enabled_var.set(snippet.get("enabled", True))
        self.notes_var.set(snippet.get("notes", ""))
        self.content_text.delete("1.0", "end")
        self.content_text.insert("1.0", snippet.get("content", ""))
        self.content_text.see("1.0")
        try:
            self._select_editor_tab("snippet")
        except Exception:
            pass
        self.trigger_entry.focus_set()
        self.update_status(f"Loaded '{snippet.get('trigger', '')}' into Snippets Manager.")

    def reset_editor(self):
        if self.editing_snippet_index is not None:
            snippet = self._get_snippets()[self.editing_snippet_index]
            self.trigger_var.set(snippet.get("trigger", ""))
            self.group_var.set(snippet.get("group", "General"))
            self.enabled_var.set(snippet.get("enabled", True))
            self.notes_var.set(snippet.get("notes", ""))
            self.content_text.delete("1.0", "end")
            self.content_text.insert("1.0", snippet.get("content", ""))
            self.content_text.see("1.0")
            self.update_status("Snippets Manager reset to the loaded snippet.")
        else:
            self.new_snippet()

    def new_snippet(self):
        self.editing_snippet_index = None
        self.editor_new_mode = True
        self.trigger_var.set("")
        selected_group = self._selected_group_name() or self._get_groups()[0]["name"]
        self.group_var.set(selected_group)
        self.enabled_var.set(True)
        self.notes_var.set("")
        self.content_text.delete("1.0", "end")
        try:
            self._select_editor_tab("snippet")
        except Exception:
            pass
        self.trigger_entry.focus_set()
        self.update_status(f"Creating a new snippet in {selected_group}")

    def _save_settings_only(self):
        try:
            restore_ms = int(self.restore_delay_var.get().strip())
        except Exception:
            messagebox.showerror(APP_TITLE, "Restore ms must be a valid number.")
            return False
        hotkey = self._normalize_hotkey_text(self.hotkey_var.get())
        if not hotkey:
            messagebox.showerror(APP_TITLE, "Global hotkey is required.")
            return False
        if not (self.sep_space_var.get() or self.sep_enter_var.get() or self.sep_tab_var.get()):
            messagebox.showerror(APP_TITLE, "Enable at least one separator key.")
            return False
        settings = self.config_manager.data["settings"]
        settings["theme"] = self.theme_name
        settings["always_on_top"] = self.always_on_top_var.get()
        settings["start_with_windows"] = self.start_with_windows_var.get()
        settings["minimize_to_tray"] = self.minimize_to_tray_var.get()
        settings["use_previous_clipboard_trigger"] = self.previous_clipboard_var.get()
        settings["use_previous_clipboard_slash_trigger"] = self.previous_clipboard_slash_var.get()
        settings["restore_clipboard_delay_ms"] = max(0, restore_ms)
        settings["match_case_sensitive"] = self.case_sensitive_var.get()
        settings["trigger_mode"] = self.mode_var.get()
        settings["hotkey"] = hotkey
        settings["word_separators"] = [name for name, var in (("space", self.sep_space_var), ("enter", self.sep_enter_var), ("tab", self.sep_tab_var)) if var.get()]
        settings["engine_enabled"] = self.engine.engine_enabled
        self.hotkey_var.set(hotkey)
        self._refresh_hotkey_label()
        self.config_manager.save()
        hotkey_bound = self.engine.reload()
        self._set_always_on_top()
        if not hotkey_bound:
            messagebox.showwarning(
                APP_TITLE,
                f"The settings were saved, but the hotkey '{hotkey}' could not be bound.\n\n"
                "Use a shortcut like ctrl+shift+f12.",
            )
        return True

    def save_engine_settings(self):
        if self._save_settings_only():
            self.update_status("Engine settings saved")

    def reset_engine_settings(self):
        self.restore_delay_var.set(str(DEFAULT_CONFIG["settings"]["restore_clipboard_delay_ms"]))
        self.case_sensitive_var.set(DEFAULT_CONFIG["settings"]["match_case_sensitive"])
        self.mode_var.set("instant")
        self.hotkey_var.set(DEFAULT_CONFIG["settings"]["hotkey"])
        self.sep_space_var.set(True)
        self.sep_enter_var.set(True)
        self.sep_tab_var.set(True)
        self.previous_clipboard_var.set(bool(DEFAULT_CONFIG["settings"]["use_previous_clipboard_trigger"]))
        self.previous_clipboard_slash_var.set(bool(DEFAULT_CONFIG["settings"]["use_previous_clipboard_slash_trigger"]))
        self._refresh_hotkey_label()
        self.update_status("Engine settings reset to defaults")

    def save_snippet(self):
        trigger = self.trigger_var.get().strip()
        group = self.group_var.get().strip()
        notes = self.notes_var.get().strip()
        content = self.content_text.get("1.0", "end-1c")
        enabled = self.enabled_var.get()

        if not trigger:
            messagebox.showerror(APP_TITLE, "Trigger is required.")
            return
        if not content:
            messagebox.showerror(APP_TITLE, "Content is required.")
            return
        if not group:
            messagebox.showerror(APP_TITLE, "Group is required.")
            return

        for idx, item in enumerate(self._get_snippets()):
            if idx == self.editing_snippet_index:
                continue
            same = item.get("trigger", "") == trigger if self.case_sensitive_var.get() else item.get("trigger", "").lower() == trigger.lower()
            if same:
                messagebox.showerror(APP_TITLE, "This trigger already exists.")
                return

        payload = {"trigger": trigger, "group": group, "notes": notes, "content": content, "enabled": enabled}
        if self.editing_snippet_index is None:
            self._get_snippets().append(payload)
            self.editing_snippet_index = len(self._get_snippets()) - 1
            self.selected_snippet_index = self.editing_snippet_index
        else:
            self._get_snippets()[self.editing_snippet_index] = payload
            self.selected_snippet_index = self.editing_snippet_index

        if self._save_settings_only():
            self._load_groups()
            self._load_snippets(keep_selection=True)
            self.update_status(f"Saved: {trigger}")

    def duplicate_snippet(self):
        if self.selected_snippet_index is None:
            messagebox.showinfo(APP_TITLE, "Select a snippet first.")
            return
        source = deep_copy(self._get_snippets()[self.selected_snippet_index])
        base = source.get("trigger", "copy")
        new_trigger = f"{base}_copy"
        existing = {s.get("trigger", "") for s in self._get_snippets()}
        counter = 2
        while new_trigger in existing:
            new_trigger = f"{base}_copy{counter}"
            counter += 1
        source["trigger"] = new_trigger
        self._get_snippets().append(source)
        self.selected_snippet_index = len(self._get_snippets()) - 1
        self.editing_snippet_index = self.selected_snippet_index
        self.editor_new_mode = False
        self.config_manager.save()
        self._load_groups()
        self._load_snippets(keep_selection=True)
        self.edit_selected_snippet()
        self.engine.reload()
        self.update_status(f"Duplicated: {new_trigger}")

    def delete_snippet(self):
        if self.selected_snippet_index is None:
            messagebox.showinfo(APP_TITLE, "Select a snippet first.")
            return
        snippet = self._get_snippets()[self.selected_snippet_index]
        if not messagebox.askyesno(APP_TITLE, f"Delete snippet '{snippet.get('trigger', '')}'?"):
            return
        del self._get_snippets()[self.selected_snippet_index]
        if self.editing_snippet_index == self.selected_snippet_index:
            self.editing_snippet_index = None
        self.selected_snippet_index = None
        self.config_manager.save()
        self._load_groups()
        self._load_snippets()
        self.new_snippet()
        self.engine.reload()
        self.update_status("Snippet deleted")

    def new_group(self):
        name = self._ask_string("New Group", "Group name:")
        if not name:
            return
        if any(g["name"].lower() == name.lower() for g in self._get_groups()):
            messagebox.showerror(APP_TITLE, "A group with this name already exists.")
            return
        self._get_groups().append({"name": name, "enabled": True})
        self.config_manager.save()
        self._load_groups()
        for idx, group in enumerate(self._get_groups(), start=1):
            if group["name"] == name:
                self.group_listbox.selection_clear(0, "end")
                self.group_listbox.selection_set(idx)
                self.selected_group_index = idx
                break
        self._refresh_group_menu()
        self.update_status(f"Group created: {name}")

    def rename_group(self):
        current = self._selected_group_name()
        if not current:
            messagebox.showinfo(APP_TITLE, "Select a group first.")
            return
        new_name = self._ask_string("Rename Group", "New group name:", current)
        if not new_name or new_name == current:
            return
        if any(g["name"].lower() == new_name.lower() for g in self._get_groups() if g["name"] != current):
            messagebox.showerror(APP_TITLE, "A group with this name already exists.")
            return
        for group in self._get_groups():
            if group["name"] == current:
                group["name"] = new_name
        for snippet in self._get_snippets():
            if snippet.get("group", "General") == current:
                snippet["group"] = new_name
        if self.group_var.get() == current:
            self.group_var.set(new_name)
        self.config_manager.save()
        self._load_groups()
        self._load_snippets(keep_selection=True)
        self.update_status(f"Group renamed to {new_name}")

    def delete_group(self):
        current = self._selected_group_name()
        if not current:
            messagebox.showinfo(APP_TITLE, "Select a group first.")
            return
        if current == "General":
            messagebox.showinfo(APP_TITLE, "The 'General' group cannot be deleted.")
            return

        used = [s for s in self._get_snippets() if s.get("group", "General") == current]

        choice = self._ask_delete_group_choice(current, len(used))
        if not choice or choice.get("value") == "cancel":
            return

        if used:
            if choice.get("value") == "move":
                target_group = choice.get("target_group") or "General"
                for snippet in used:
                    snippet["group"] = target_group
            elif choice.get("value") == "delete_all":
                self.config_manager.data["snippets"] = [
                    item for item in self._get_snippets() if item.get("group", "General") != current
                ]
            else:
                return
        else:
            if choice.get("value") not in {"move", "delete_all"}:
                return

        self.config_manager.data["groups"] = [g for g in self._get_groups() if g["name"] != current]
        self.selected_group_index = 0
        self.config_manager.save()
        self._load_groups()
        self._load_snippets()
        self._refresh_group_menu()
        self.engine.reload()
        if used and choice.get("value") == "delete_all":
            self.update_status(f"Group and snippets deleted: {current}")
        elif used and choice.get("value") == "move":
            self.update_status(f"Group deleted; snippets moved to {choice.get('target_group') or 'General'}: {current}")
        else:
            self.update_status(f"Empty group deleted: {current}")

    def toggle_group(self):
        current = self._selected_group_name()
        if not current:
            messagebox.showinfo(APP_TITLE, "Select a group first.")
            return
        for group in self._get_groups():
            if group["name"] == current:
                group["enabled"] = not group.get("enabled", True)
                state = "enabled" if group["enabled"] else "disabled"
                self.update_status(f"Group {state}: {current}")
                break
        self.config_manager.save()
        self._load_groups()
        self._load_snippets(keep_selection=True)
        self.engine.reload()

    def import_data(self):
        path = filedialog.askopenfilename(
            title="Import snippets",
            initialdir=self._get_last_io_directory(),
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return
        try:
            options = self._show_import_options_dialog(path)
            if not options:
                return
            added_count, updated_count, skipped_count = self.config_manager.merge_import_data(
                options["parsed"]["data"],
                keep_source_groups=options["keep_source_groups"],
                target_group=options["target_group"],
                overwrite_conflicts=options["overwrite_conflicts"],
            )
            self._remember_io_path(options["path"])
            self._sync_settings_vars_from_config()
            self._apply_theme()
            self._set_always_on_top()
            self.engine.reload()
            self._load_groups()
            self._load_snippets()
            self._schedule_layout_refresh()
            self.update_status(
                f"Imported {added_count} added / {updated_count} updated / {skipped_count} skipped from {os.path.basename(options['path'])}"
            )
        except Exception as exc:
            messagebox.showerror(APP_TITLE, f"Import failed:\n{exc}")

    def export_data(self):
        options = self._show_export_options_dialog()
        if not options:
            return
        path = filedialog.asksaveasfilename(
            title="Export snippets",
            initialdir=self._get_last_io_directory(),
            initialfile=self._suggest_export_filename(options["beeftext_format"]),
            defaultextension=".json",
            filetypes=[("JSON files", "*.json")],
        )
        if not path:
            return
        try:
            group_name = self._selected_group_name() if options["current_group_only"] else ""
            if options["beeftext_format"]:
                self.config_manager.export_beeftext(path, current_group_only=options["current_group_only"], group_name=group_name)
            else:
                self.config_manager.export_native(path, current_group_only=options["current_group_only"], group_name=group_name)
            self._remember_io_path(path)
            self.update_status(f"Exported to {os.path.basename(path)}")
        except Exception as exc:
            messagebox.showerror(APP_TITLE, f"Export failed:\n{exc}")


    def _setup_window_icon(self):
        ico_path = self._find_existing_icon_path(("app.ico", "icon.ico"))
        png_path = self._find_existing_icon_path(("app.png", "icon.png"))
        try:
            if ico_path and os.name == "nt":
                self.root.iconbitmap(default=ico_path)
        except Exception:
            pass
        try:
            if png_path:
                self._app_icon_photo = tk.PhotoImage(file=png_path)
                self.root.iconphoto(True, self._app_icon_photo)
                return
            if ico_path and Image is not None and ImageTk is not None:
                image = Image.open(ico_path).convert("RGBA")
                self._app_icon_photo = ImageTk.PhotoImage(image)
                self.root.iconphoto(True, self._app_icon_photo)
                return
            icon = tk.PhotoImage(width=32, height=32)
            bg = "#0B1220"
            accent = "#22C55E"
            light = "#F8FAFC"
            icon.put(bg, to=(0, 0, 32, 32))
            icon.put(accent, to=(5, 5, 27, 27))
            icon.put(bg, to=(8, 8, 24, 24))
            icon.put(light, to=(11, 9, 15, 23))
            icon.put(light, to=(17, 9, 21, 23))
            icon.put(accent, to=(11, 24, 21, 27))
            self._app_icon_photo = icon
            self.root.iconphoto(True, icon)
        except Exception:
            pass

    def _create_tray_image(self):
        if Image is None or ImageDraw is None:
            return None
        external = self._load_external_icon_image(TRAY_ICON_CANDIDATES, size=(64, 64))
        if external is not None:
            return external
        img = Image.new("RGBA", (64, 64), (11, 18, 32, 255))
        d = ImageDraw.Draw(img)
        d.rounded_rectangle((8, 8, 56, 56), radius=12, fill=(34, 197, 94, 255))
        d.rounded_rectangle((17, 16, 47, 48), radius=7, fill=(11, 18, 32, 255))
        d.rectangle((24, 20, 30, 43), fill=(248, 250, 252, 255))
        d.rectangle((34, 20, 40, 43), fill=(248, 250, 252, 255))
        d.rectangle((24, 47, 40, 52), fill=(34, 197, 94, 255))
        return img

    def _create_pause_tray_image(self):
        if Image is None or ImageDraw is None:
            return None
        external = self._load_external_icon_image(PAUSE_TRAY_ICON_CANDIDATES, size=(64, 64))
        if external is not None:
            return external
        img = Image.new("RGBA", (64, 64), (11, 18, 32, 255))
        d = ImageDraw.Draw(img)
        d.rounded_rectangle((8, 8, 56, 56), radius=12, fill=(153, 27, 27, 255))
        d.rounded_rectangle((17, 16, 47, 48), radius=7, fill=(11, 18, 32, 255))
        d.rectangle((24, 20, 30, 43), fill=(248, 250, 252, 255))
        d.rectangle((34, 20, 40, 43), fill=(248, 250, 252, 255))
        return img

    def _refresh_tray_icon(self):
        if self.tray_icon is None:
            return
        try:
            self.tray_icon.title = f"{APP_TITLE} - {'Running' if self.engine.engine_enabled else 'Paused'}"
            if self.engine.engine_enabled:
                self.tray_icon.icon = self._tray_active_image or self._create_tray_image()
            else:
                self.tray_icon.icon = self._tray_paused_image or self._create_pause_tray_image()
            try:
                self.tray_icon.update_menu()
            except Exception:
                pass
            try:
                self.tray_icon.update_icon()
            except Exception:
                pass
        except Exception:
            pass

    def _run_on_ui(self, func):
        try:
            self.root.after(0, func)
        except Exception:
            pass

    def _on_unmap(self, event=None):
        if event is not None and event.widget is not self.root:
            return
        if self._closing or self._minimize_handled:
            return
        try:
            if self.root.state() == "iconic" and self.minimize_to_tray_var.get():
                self._minimize_to_tray()
        except Exception:
            pass

    def _minimize_to_tray(self):
        if pystray is None or Image is None:
            self.update_status("Minimize to tray requires pystray and pillow.")
            return
        self._minimize_handled = True
        self.root.after(150, lambda: setattr(self, "_minimize_handled", False))
        self.root.withdraw()
        self._show_tray_icon()
        self.update_status("Minimized to tray")

    def _show_window_from_tray(self, icon=None, item=None):
        def show():
            if self._restoring_from_tray:
                return
            self._restoring_from_tray = True
            try:
                try:
                    self.root.attributes("-alpha", 0.0)
                except Exception:
                    pass
                self.root.deiconify()
                self.root.state("normal")
                self.root.update_idletasks()
                self.root.after(24, self._finish_window_restore)
            except Exception:
                self._finish_window_restore()
        self._run_on_ui(show)

    def _hide_to_tray(self, status_text="Hidden to tray"):
        if self._closing:
            return
        if pystray is None or Image is None:
            self.exit_app()
            return
        try:
            self.root.withdraw()
        except Exception:
            pass
        self._show_tray_icon()
        self.update_status(status_text)

    def _tray_toggle_engine(self, icon=None, item=None):
        self._run_on_ui(self.toggle_engine)

    def _tray_quit(self, icon=None, item=None):
        self._run_on_ui(self.exit_app)

    def _show_tray_icon(self):
        if pystray is None or Image is None:
            self.update_status("Tray icon unavailable. Install pystray and pillow to enable it.")
            return
        if self.tray_icon is not None:
            self._refresh_tray_icon()
            return
        self._tray_active_image = self._create_tray_image()
        self._tray_paused_image = self._create_pause_tray_image()
        image = self._tray_active_image if self.engine.engine_enabled else self._tray_paused_image
        menu = pystray.Menu(
            pystray.MenuItem("Open", self._show_window_from_tray, default=True),
            pystray.MenuItem("Pause / Resume", self._tray_toggle_engine),
            pystray.MenuItem("Exit", self._tray_quit),
        )
        self.tray_icon = pystray.Icon(APP_TITLE, image, APP_TITLE, menu)

        def run_icon():
            try:
                self.tray_icon.run()
            except Exception:
                self.tray_icon = None

        threading.Thread(target=run_icon, daemon=True).start()
        self.root.after(120, self._refresh_tray_icon)

    def _stop_tray_icon(self):
        if self.tray_icon is not None:
            try:
                self.tray_icon.stop()
            except Exception:
                pass
            self.tray_icon = None

    def _startup_command(self):
        if getattr(sys, "frozen", False):
            return f'"{os.path.abspath(sys.executable)}"'
        py = sys.executable
        script = os.path.abspath(__file__)
        if os.name == "nt":
            candidate = os.path.join(os.path.dirname(py), "pythonw.exe")
            if os.path.exists(candidate):
                py = candidate
        return f'"{py}" "{script}"'

    def toggle_start_with_windows(self):
        ok = self._set_startup_registry(self.start_with_windows_var.get())
        if ok:
            self._save_settings_only_silent()
            self.update_status("Start with Windows enabled" if self.start_with_windows_var.get() else "Start with Windows disabled")
        else:
            self.start_with_windows_var.set(not self.start_with_windows_var.get())

    def _set_startup_registry(self, enabled: bool) -> bool:
        if os.name != "nt" or winreg is None:
            messagebox.showerror(APP_TITLE, "Start with Windows is supported on Windows only.")
            return False
        try:
            key_path = r"Software\Microsoft\Windows\CurrentVersion\Run"
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path, 0, winreg.KEY_SET_VALUE) as key:
                if enabled:
                    winreg.SetValueEx(key, APP_TITLE, 0, winreg.REG_SZ, self._startup_command())
                else:
                    try:
                        winreg.DeleteValue(key, APP_TITLE)
                    except FileNotFoundError:
                        pass
            return True
        except Exception as exc:
            messagebox.showerror(APP_TITLE, f"Could not update Windows startup setting:\n{exc}")
            return False

    def _save_settings_only_silent(self):
        try:
            settings = self.config_manager.data["settings"]
            settings["theme"] = self.theme_name
            settings["always_on_top"] = self.always_on_top_var.get()
            settings["start_with_windows"] = self.start_with_windows_var.get()
            settings["minimize_to_tray"] = self.minimize_to_tray_var.get()
            settings["use_previous_clipboard_trigger"] = self.previous_clipboard_var.get()
            settings["use_previous_clipboard_slash_trigger"] = self.previous_clipboard_slash_var.get()
            self.config_manager.save()
        except Exception:
            pass

    def _set_always_on_top(self):
        self.root.attributes("-topmost", self.always_on_top_var.get())
        self._save_settings_only_silent()

    def toggle_theme(self):
        self.theme_name = "light" if self.theme_name == "dark" else "dark"
        self.config_manager.data["settings"]["theme"] = self.theme_name
        self.config_manager.save()
        self._apply_theme()
        self.update_status(f"Theme changed to {self.theme_name}")

    def toggle_engine(self):
        self.engine.toggle_enabled()
        self._refresh_engine_button()
        self._refresh_tray_icon()

    def _refresh_engine_button(self):
        enabled = self.engine.engine_enabled
        self.engine_button.configure(text="Engine: ON" if enabled else "Engine: OFF")
        self.engine_button._accent = enabled
        self.engine_button._danger = not enabled
        self._apply_theme_button_only(self.engine_button)

    def _apply_theme_button_only(self, btn):
        if getattr(btn, "_accent", False):
            btn.configure(bg=self.theme["accent"], fg="#FFFFFF", activebackground=self.theme["accent"], activeforeground="#FFFFFF")
        elif getattr(btn, "_danger", False):
            btn.configure(bg=self.theme["danger"], fg="#FFFFFF", activebackground=self.theme["danger"], activeforeground="#FFFFFF")
        else:
            btn.configure(bg=self.theme["button"], fg=self.theme["button_text"], activebackground=self.theme["button_hover"], activeforeground=self.theme["button_text"])


    def _dialog_theme_values(self):
        t = self.theme
        return {
            "bg": t.get("panel", "#0f172a"),
            "bg2": t.get("panel_2", t.get("panel", "#0f172a")),
            "fg": t.get("text", "#ffffff"),
            "muted": t.get("muted", t.get("text", "#ffffff")),
            "border": t.get("border", "#334155"),
            "button": t.get("button", t.get("panel_2", "#1e293b")),
            "button_fg": t.get("button_text", t.get("text", "#ffffff")),
            "danger": t.get("danger", "#991b1b"),
            "danger_fg": "#ffffff",
            "entry_bg": t.get("entry_bg", "#020617"),
            "entry_fg": t.get("entry_fg", t.get("text", "#ffffff")),
        }

    def _center_dialog(self, dialog, width=None, height=None):
        try:
            dialog.update_idletasks()
            if width is None:
                width = max(dialog.winfo_reqwidth(), 360)
            if height is None:
                height = max(dialog.winfo_reqheight(), 180)
            rx = self.root.winfo_rootx()
            ry = self.root.winfo_rooty()
            rw = self.root.winfo_width()
            rh = self.root.winfo_height()
            x = rx + max(0, (rw - width) // 2)
            y = ry + max(0, (rh - height) // 2)
            dialog.geometry(f"{width}x{height}+{x}+{y}")
        except Exception:
            pass

    def _dialog_button(self, master, text, command, danger=False):
        c = self._dialog_theme_values()
        btn = tk.Button(
            master,
            text=text,
            command=command,
            bg=c["danger"] if danger else c["button"],
            fg=c["danger_fg"] if danger else c["button_fg"],
            activebackground=c["danger"] if danger else c["button"],
            activeforeground=c["danger_fg"] if danger else c["button_fg"],
            relief="flat",
            bd=0,
            padx=14,
            pady=10,
            font=(self.UI_FONT, 10, "bold"),
            cursor="hand2",
            takefocus=True,
        )
        return btn

    def _ask_confirm(self, title, message, ok_text="OK", cancel_text="Cancel"):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title(title)
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])
        result = {"value": False}

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        tk.Label(
            frm,
            text=message,
            bg=c["bg"],
            fg=c["fg"],
            justify="left",
            wraplength=420,
            font=(self.UI_FONT, 11, "bold"),
        ).pack(anchor="w", pady=(0, 18))

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")

        def ok():
            result["value"] = True
            dialog.destroy()

        def cancel():
            result["value"] = False
            dialog.destroy()

        self._dialog_button(row, ok_text, ok).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, cancel_text, cancel).pack(side="left", expand=True, fill="x")

        dialog.protocol("WM_DELETE_WINDOW", cancel)
        dialog.bind("<Return>", lambda _e: ok())
        dialog.bind("<Escape>", lambda _e: cancel())
        self._center_dialog(dialog, 460, 180)
        dialog.focus_force()
        dialog.wait_window()
        return result["value"]

    def _ask_delete_group_choice(self, group_name: str, snippet_count: int):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("Delete Group")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        if snippet_count:
            msg = (
                f"Delete group '{group_name}'?\n\n"
                f"{snippet_count} snippet(s) need a destination group before delete."
            )
        else:
            msg = f"Delete empty group '{group_name}'?"

        tk.Label(
            frm,
            text=msg,
            bg=c["bg"],
            fg=c["fg"],
            justify="left",
            wraplength=520,
            font=(self.UI_FONT, 11, "bold"),
        ).pack(anchor="w", pady=(0, 18))

        result = {"value": None, "target_group": ""}
        target_var = tk.StringVar(value="General")

        def choose(value):
            result["value"] = value
            result["target_group"] = target_var.get().strip()
            dialog.destroy()

        if snippet_count:
            tk.Label(
                frm,
                text="Move snippets to",
                bg=c["bg"],
                fg=c["fg"],
                justify="left",
                font=(self.UI_FONT, 10),
            ).pack(anchor="w", pady=(0, 8))
            options = [group["name"] for group in self._get_groups() if group["name"] != group_name]
            if not options:
                options = ["General"]
            if target_var.get() not in options:
                target_var.set(options[0])
            target_combo = self._styled_combobox(frm, target_var, values=options)
            target_combo.pack(fill="x", pady=(0, 16))

        tk.Label(
            frm,
            text="This cannot be undone from the UI.",
            bg=c["bg"],
            fg=c["fg"],
            justify="left",
            font=(self.UI_FONT, 10),
        ).pack(anchor="w", pady=(0, 16))

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")

        self._dialog_button(row, "Delete", lambda: choose("move"), danger=True).pack(
            side="left", expand=True, fill="x", padx=(0, 8)
        )
        if snippet_count:
            self._dialog_button(row, "Delete all snippets", lambda: choose("delete_all"), danger=True).pack(
                side="left", expand=True, fill="x", padx=(0, 8)
            )
        self._dialog_button(row, "Cancel", lambda: choose("cancel")).pack(
            side="left", expand=True, fill="x"
        )

        dialog.protocol("WM_DELETE_WINDOW", lambda: choose("cancel"))
        dialog.bind("<Escape>", lambda _e: choose("cancel"))
        self._center_dialog(dialog, 620 if snippet_count else 460, 250 if snippet_count else 180)
        dialog.focus_force()
        dialog.wait_window()
        return result

    def _ask_string(self, title, prompt, initial=""):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title(title)
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        result = {"value": None}
        value_var = tk.StringVar(value=initial)

        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        tk.Label(
            frm,
            text=prompt,
            bg=c["bg"],
            fg=c["fg"],
            font=(self.UI_FONT, 10, "bold"),
        ).pack(anchor="w", pady=(0, 8))

        entry = tk.Entry(
            frm,
            textvariable=value_var,
            bg=c["entry_bg"],
            fg=c["entry_fg"],
            insertbackground=c["entry_fg"],
            relief="solid",
            bd=1,
            highlightthickness=1,
            highlightbackground=c["border"],
            highlightcolor=c["border"],
            font=(self.UI_FONT, 10),
        )
        entry.pack(fill="x", pady=(0, 14), ipady=4)
        entry.focus_set()
        entry.icursor("end")

        def ok():
            val = value_var.get().strip()
            if val:
                result["value"] = val
            dialog.destroy()

        def cancel():
            dialog.destroy()

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")
        self._dialog_button(row, "OK", ok).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Cancel", cancel).pack(side="left", expand=True, fill="x")

        dialog.protocol("WM_DELETE_WINDOW", cancel)
        dialog.bind("<Return>", lambda _e: ok())
        dialog.bind("<Escape>", lambda _e: cancel())
        self._center_dialog(dialog, 420, 180)
        dialog.focus_force()
        dialog.wait_window()
        return result["value"]

    def _show_variable_input_dialog(self, prompt_text: str) -> tuple[bool, str]:
        if threading.current_thread() is threading.main_thread():
            return self._show_variable_input_dialog_sync(prompt_text)
        result = {"accepted": False, "text": ""}
        finished = threading.Event()

        def run_dialog():
            try:
                accepted, text = self._show_variable_input_dialog_sync(prompt_text)
                result["accepted"] = accepted
                result["text"] = text
            finally:
                finished.set()

        try:
            self.root.after(0, run_dialog)
            finished.wait()
        except Exception:
            return False, ""
        return bool(result["accepted"]), str(result["text"])

    def _show_variable_input_dialog_sync(self, prompt_text: str) -> tuple[bool, str]:
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("BlinkText Input")
        dialog.configure(bg=c["bg"])
        dialog.resizable(False, False)
        dialog.grab_set()
        try:
            dialog.attributes("-topmost", True)
        except Exception:
            pass

        result = {"accepted": False, "text": ""}
        frm = tk.Frame(dialog, bg=c["bg"], padx=20, pady=18)
        frm.pack(fill="both", expand=True)

        tk.Label(
            frm,
            text=prompt_text or "Input",
            bg=c["bg"],
            fg=c["fg"],
            justify="left",
            wraplength=520,
            font=(self.UI_FONT, 11, "bold"),
        ).pack(anchor="w", pady=(0, 10))

        text = tk.Text(
            frm,
            height=8,
            wrap="word",
            bg=c["entry_bg"],
            fg=c["entry_fg"],
            insertbackground=c["entry_fg"],
            relief="flat",
            bd=0,
            highlightthickness=1,
            highlightbackground=c["border"],
            highlightcolor=c["border"],
            font=(self.AR_FONT, 10),
            padx=10,
            pady=8,
        )
        text.pack(fill="both", expand=True, pady=(0, 12))
        text.focus_set()

        def ok():
            result["accepted"] = True
            result["text"] = text.get("1.0", "end-1c")
            dialog.destroy()

        def cancel():
            result["accepted"] = False
            result["text"] = ""
            dialog.destroy()

        def on_text_return(event):
            if event.state & 0x0001:
                return None
            ok()
            return "break"

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")
        self._dialog_button(row, "OK", ok).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Cancel", cancel).pack(side="left", expand=True, fill="x")

        text.bind("<Return>", on_text_return)
        dialog.bind("<Escape>", lambda _e: cancel())
        dialog.protocol("WM_DELETE_WINDOW", cancel)
        self._center_dialog(dialog, 580, 320)
        dialog.wait_window()
        return bool(result["accepted"]), str(result["text"])

    def _documents_dir(self):
        candidate = os.path.join(os.path.expanduser("~"), "Documents")
        return candidate if os.path.isdir(candidate) else BASE_DIR

    def _export_timestamp(self):
        return datetime.now().strftime("%d-%m-%Y-%H-%M")

    def _suggest_export_filename(self, beeftext_format: bool):
        stamp = self._export_timestamp()
        if beeftext_format:
            return f"BlinkText-BeefText-{stamp}.json"
        return f"BlinkText-{stamp}.json"

    def _show_export_options_dialog(self):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("Export options")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        format_var = tk.StringVar(value="native")
        current_group = self._selected_group_name()
        scope_var = tk.StringVar(value="group" if current_group else "all")
        result = {"value": None}

        frm = tk.Frame(dialog, bg=c["bg"], padx=18, pady=16)
        frm.pack(fill="both", expand=True)

        format_box = tk.LabelFrame(frm, text="Format", bg=c["bg"], fg=c["fg"], bd=1, relief="solid")
        format_box.pack(fill="x", pady=(0, 14))
        self._styled_radiobutton(format_box, "Native BlinkText JSON", format_var, "native").pack(anchor="w", padx=12, pady=(10, 4))
        self._styled_radiobutton(format_box, "Beeftext combos JSON", format_var, "beeftext").pack(anchor="w", padx=12, pady=(0, 12))

        scope_box = tk.LabelFrame(frm, text="Scope", bg=c["bg"], fg=c["fg"], bd=1, relief="solid")
        scope_box.pack(fill="x", pady=(0, 16))
        self._styled_radiobutton(scope_box, f"All snippets ({len(self._get_snippets())})", scope_var, "all").pack(anchor="w", padx=12, pady=(10, 4))
        group_label = f"Current group '{current_group}'" if current_group else "Current group only"
        group_count = 0
        if current_group:
            group_count = sum(1 for item in self._get_snippets() if item.get("group", "General") == current_group)
            group_label = f"Current group '{current_group}' ({group_count})"
        group_radio = self._styled_radiobutton(scope_box, group_label, scope_var, "group")
        group_radio.pack(anchor="w", padx=12, pady=(0, 12))
        if not current_group:
            group_radio.configure(state="disabled")

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")

        def do_export():
            result["value"] = {
                "beeftext_format": format_var.get() == "beeftext",
                "current_group_only": scope_var.get() == "group" and bool(current_group),
            }
            dialog.destroy()

        self._dialog_button(row, "Export", do_export).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Cancel", dialog.destroy).pack(side="left", expand=True, fill="x")

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)
        self._center_dialog(dialog, 620, 280)
        dialog.focus_force()
        dialog.wait_window()
        return result["value"]

    def _show_import_options_dialog(self, initial_path: str):
        c = self._dialog_theme_values()
        dialog = tk.Toplevel(self.root)
        dialog.title("Import snippets")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(False, False)
        dialog.configure(bg=c["bg"])

        file_var = tk.StringVar(value=initial_path)
        import_target_var = tk.StringVar(value="<Keep source groups>")
        conflict_var = tk.StringVar(value="skip")
        count_var = tk.StringVar(value="Imported snippets: 0")
        current_parsed = {"data": None}
        result = {"value": None}

        frm = tk.Frame(dialog, bg=c["bg"], padx=18, pady=16)
        frm.pack(fill="both", expand=True)

        file_box = tk.LabelFrame(frm, text="File", bg=c["bg"], fg=c["fg"], bd=1, relief="solid")
        file_box.pack(fill="x", pady=(0, 14))
        file_row = tk.Frame(file_box, bg=c["bg"])
        file_row.pack(fill="x", padx=12, pady=12)
        file_entry = tk.Entry(
            file_row,
            textvariable=file_var,
            bg=c["entry_bg"],
            fg=c["entry_fg"],
            insertbackground=c["entry_fg"],
            relief="solid",
            bd=1,
            highlightthickness=1,
            highlightbackground=c["border"],
            highlightcolor=c["border"],
            font=(self.UI_FONT, 10),
        )
        file_entry.pack(side="left", fill="x", expand=True, padx=(0, 8), ipady=4)

        import_box = tk.LabelFrame(frm, text="Import options", bg=c["bg"], fg=c["fg"], bd=1, relief="solid")
        import_box.pack(fill="x", pady=(0, 14))
        import_row = tk.Frame(import_box, bg=c["bg"])
        import_row.pack(fill="x", padx=12, pady=12)
        self._styled_label(import_row, "Import into", role="panel").pack(side="left", padx=(0, 8))
        group_option = self._styled_combobox(import_row, import_target_var, values=("<Keep source groups>",))
        group_option.pack(side="left", fill="x", expand=True)
        imported_label = self._styled_label(import_row, "", role="panel")
        imported_label.pack(side="left", padx=(12, 0))

        conflict_box = tk.LabelFrame(frm, text="Conflict resolution", bg=c["bg"], fg=c["fg"], bd=1, relief="solid")
        conflict_box.pack(fill="x", pady=(0, 16))
        skip_radio = self._styled_radiobutton(conflict_box, "Skip 0 conflicting snippets.", conflict_var, "skip")
        skip_radio.pack(anchor="w", padx=12, pady=(10, 4))
        overwrite_radio = self._styled_radiobutton(conflict_box, "Overwrite 0 conflicting snippets.", conflict_var, "overwrite")
        overwrite_radio.pack(anchor="w", padx=12, pady=(0, 12))

        row = tk.Frame(frm, bg=c["bg"])
        row.pack(fill="x")

        def refresh_group_menu():
            options = ["<Keep source groups>"] + [group["name"] for group in self._get_groups()]
            group_option.configure(values=options)
            if import_target_var.get() not in options:
                import_target_var.set(options[0])

        def refresh_preview():
            parsed = current_parsed["data"]
            if not parsed:
                count_var.set("Imported snippets: 0")
                imported_label.configure(text=count_var.get())
                skip_radio.configure(text="Skip 0 conflicting snippets.")
                overwrite_radio.configure(text="Overwrite 0 conflicting snippets.")
                return
            count = len(parsed["data"].get("snippets", []))
            count_var.set(f"Imported snippets: {count}")
            imported_label.configure(text=count_var.get())
            keep_source_groups = import_target_var.get() == "<Keep source groups>"
            target_group = "" if keep_source_groups else import_target_var.get()
            conflicts = self.config_manager.count_import_conflicts(parsed["data"], keep_source_groups=keep_source_groups, target_group=target_group)
            skip_radio.configure(text=f"Skip {conflicts} conflicting snippets.")
            overwrite_radio.configure(text=f"Overwrite {conflicts} conflicting snippets.")

        def load_path(path):
            try:
                parsed = self.config_manager.parse_import_file(path)
            except Exception as exc:
                messagebox.showerror(APP_TITLE, f"Import failed:\n{exc}")
                return False
            current_parsed["data"] = parsed
            file_var.set(path)
            refresh_preview()
            return True

        def browse():
            selected = filedialog.askopenfilename(
                title="Import snippets",
                initialdir=os.path.dirname(file_var.get()) if file_var.get() else self._get_last_io_directory(),
                filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
            )
            if selected:
                load_path(selected)

        browse_button = self._dialog_button(file_row, "Browse", browse)
        browse_button.pack(side="left")

        def do_import():
            parsed = current_parsed["data"]
            if not parsed:
                return
            result["value"] = {
                "path": file_var.get(),
                "parsed": parsed,
                "keep_source_groups": import_target_var.get() == "<Keep source groups>",
                "target_group": "" if import_target_var.get() == "<Keep source groups>" else import_target_var.get(),
                "overwrite_conflicts": conflict_var.get() == "overwrite",
            }
            dialog.destroy()

        self._dialog_button(row, "Import", do_import).pack(side="left", expand=True, fill="x", padx=(0, 8))
        self._dialog_button(row, "Cancel", dialog.destroy).pack(side="left", expand=True, fill="x")

        refresh_group_menu()
        import_target_var.trace_add("write", lambda *_: refresh_preview())
        load_path(initial_path)

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)
        self._center_dialog(dialog, 740, 380)
        dialog.focus_force()
        dialog.wait_window()
        return result["value"]

    def on_close(self):
        self._hide_to_tray("BlinkText is still running in the tray.")

    def _on_alt_f4(self, event=None):
        self.on_close()
        return "break"

    def exit_app(self):
        self._closing = True
        try:
            self._save_settings_only()
        except Exception:
            pass
        try:
            self.engine.stop()
        except Exception:
            pass
        self._stop_tray_icon()
        try:
            self.root.destroy()
        except Exception:
            pass


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
