# BlinkText Python Edition

This folder contains the Python edition of BlinkText.

Important status:

- This version is experimental.
- It is not yet as stable or as complete as the main C++ `v1.1.0` edition.
- It is included for reference, development, and early testing.
- For daily use, the C++ version is still the recommended version.

## Current State

The Python version already includes a large part of the BlinkText feature set, including:

- Snippet groups and snippet management
- Dark and light themes
- Tray support
- Import and export
- Variables support
- Clipboard-related variables
- Date/time variables
- Input variables
- Key, shortcut, delay, cursor, combo, environment, and PowerShell variables

However, it is still not fully reliable in all real-world cases.

## Known Limitations

At the moment, the Python version may still have issues such as:

- External text expansion not behaving exactly like the C++ version in all apps
- Inconsistent behavior in programs such as Microsoft Word, browsers, or other rich text fields
- Cursor placement edge cases in some external targets
- Clipboard timing or paste timing differences depending on the target app
- Some UI and behavior differences compared to the C++ edition

Because of that, this version should be treated as:

- a preview build
- a Python port in progress
- not the primary production release

## Recommended Use

Use this Python edition if you want to:

- inspect how BlinkText works in Python
- test features quickly
- extend or prototype ideas
- compare behavior against the main C++ version

If you want the most reliable experience, use the C++ edition instead.

## Requirements

Install the required packages:

```bash
pip install -r requirements.txt
```

The main dependencies are:

- `keyboard`
- `pyperclip`
- `pystray`
- `Pillow`

## Run

From this folder, run:

```bash
python BlinkText.py
```

## Files in This Folder

- `BlinkText.py`  
  Main Python application

- `requirements.txt`  
  Python dependencies

- `snippets_v9.json`  
  Snippet data file used by the Python edition

## Notes About Assets

The Python version is designed to reuse the same BlinkText assets where possible, including icons and toolbar images from the main project.

## Compatibility Note

This Python edition aims to follow the current BlinkText behavior and variable system, but it should not be considered a perfect one-to-one replacement for the C++ release yet.

## Summary

If you are uploading this folder to GitHub, the safest description is:

> The Python edition of BlinkText is an experimental port of the main application. It already supports many core features, but it is not yet fully equivalent to the stable C++ release and may still behave inconsistently in some external applications.

