# BlinkText

BlinkText is a fast, local text expansion tool for Windows. It is built to expand text instantly, stay responsive during normal typing and paste-heavy workflows, and run fully offline with no cloud dependency.

This repository contains the native Win32 C++ version of BlinkText, including:
- the desktop application source code
- the portable build workflow
- the installer script
- import and export support for compatible snippet tools

## What BlinkText Does

BlinkText lets you type short triggers such as `sig\` and automatically replace them with longer text. It is designed for fast typing, repeatable workflows, prompt templates, signatures, writing blocks, and reusable text snippets.

Examples:
- `sig\` -> your email signature
- `addr\` -> your full address
- `tr1\` -> a translation template
- `sum\` -> a professional summary template

Everything runs locally on your machine:
- no telemetry
- no account
- no sync service
- no online dependency

## Main Features

- Instant text expansion outside the app
- Trigger expansion inside the built-in `Test Area`
- Group-based snippet organization
- Snippet enable / disable support
- Notes and content preview
- Global typing engine
- Light mode and dark mode
- Tray icon support
- Start with Windows
- Always on top
- Minimize to tray
- Recordable global toggle hotkey
- Import from native BlinkText JSON
- Import from Beeftext combos JSON
- Export to native BlinkText JSON
- Export to Beeftext combos JSON
- Safe local saving with backup behavior
- Optional clipboard-history style triggers:
  - `\\`
  - `//`

## Application Layout

BlinkText is organized into five main areas:

### 1. Top Bar

The top bar contains the main app actions:
- `Engine ON/OFF`
- `Theme`
- `Import`
- `Export`
- `GitHub`
- `Info`

### 2. Groups

The left panel is used for organizing snippets into groups.

Available actions:
- `New Group`
- `Rename`
- `Toggle`
- `Delete`

You can also right-click a group for quick actions.

### 3. Test Area

The `Test Area` is a built-in sandbox for testing triggers without leaving the app.

Use it to:
- verify imported snippets
- test trigger behavior
- test separator mode
- test clipboard-based triggers

### 4. Snippets

The middle panel displays the current snippet list with the following columns:
- `Trigger`
- `Group`
- `On`
- `Notes`
- `Preview`

Supported actions:
- select a snippet
- search snippets
- sort by column
- duplicate
- delete
- toggle snippet state
- edit the selected snippet
- create a new snippet

### 5. Snippets Manager

The right panel is split into two tabs:
- `Snippets Manager`
- `Engine Settings`

#### Snippets Manager Tab

Used to create or edit:
- trigger
- group
- enabled state
- notes
- content

Buttons:
- `New`
- `Save`
- `Reset`

#### Engine Settings Tab

Contains:
- `Restore clipboard (ms)`
- current global hotkey
- `Record Hotkey`
- `Always on top`
- `Start with Windows`
- `Minimize to tray`
- `Use \\ for previous clipboard item`
- `Use // for previous clipboard item`
- `Case sensitive matching`
- trigger mode selection
- separator key selection
- `Save Settings`
- `Reset Settings`

## Trigger Modes

BlinkText supports two trigger modes.

### Instant

The snippet expands immediately once the trigger is fully typed.

### Separator

The snippet expands only after a separator key is pressed.

Supported separator keys:
- `Space`
- `Enter`
- `Tab`

## Clipboard-Based Triggers

BlinkText includes two optional built-in triggers:
- `\\`
- `//`

When enabled from `Engine Settings`, these triggers paste a previously copied clipboard item instead of behaving like normal snippets.

Important behavior:
- if enabled, the built-in behavior takes priority over a custom snippet with the same trigger
- clipboard history is based on what BlinkText observes while it is running
- the feature is meant for local recent clipboard reuse, not full Windows clipboard manager replacement

## Search Behavior

The snippet search is focused on snippet content, not group navigation.

Search works against:
- trigger
- notes
- content preview

Search does not depend on group names, because group filtering is already handled by the Groups panel.

## Import

BlinkText supports importing from:
- `Native BlinkText JSON`
- `Beeftext combos JSON`

Import dialog capabilities:
- select a file
- browse for another file
- choose destination behavior
- keep source groups
- import into a selected group
- skip conflicts
- overwrite conflicts

`Import into` is a controlled dropdown list, not a free text field.

## Export

BlinkText supports exporting to:
- `Native BlinkText JSON`
- `Beeftext combos JSON`

Export scope:
- all snippets
- current group only

Default export location:
- Windows `Documents`

Suggested file naming:
- `BlinkText-D-M-Y-H-M.json`
- `BlinkText-BeefText-D-M-Y-H-M.json`

## Tray Behavior

BlinkText keeps a tray icon available during runtime.

Tray behavior includes:
- always-visible tray icon while the app is running
- minimize to tray
- restore from tray
- paused tray icon support
- tray menu actions

If the engine is paused, BlinkText can use `app_pause.ico` for the tray icon.

## Global Hotkey

Default global toggle hotkey:
- `Ctrl + Shift + F12`

You can change it directly from the UI using `Record Hotkey`.

## Context Menus

BlinkText uses themed context menus that follow the current app theme.

Standard editing actions include:
- Undo
- Cut
- Copy
- Paste
- Delete
- Select All

Additional advanced text options:
- Right to left Reading order
- Show Unicode control characters
- Insert Unicode control character
- Open IME
- Reconversion

## Appearance and Themes

BlinkText supports:
- `Light mode`
- `Dark mode`

The UI is designed to keep a consistent style across:
- buttons
- snippet list
- group list
- editor fields
- dialogs
- pop-up menus
- engine settings controls

## Data Storage and Safety

BlinkText stores its data locally in JSON files.

Safety behavior includes:
- temporary file write before replacement
- backup behavior
- local-only storage
- no online sync

This helps protect snippet data during saves and updates.

## Privacy

BlinkText is fully offline by design.

Privacy guarantees:
- no user data is collected
- no user data is stored remotely
- no user data is transmitted
- no account or login is required

## Compatibility

BlinkText is intended for Windows desktop use.

It supports importing exported triggers from Beeftext for easier migration.

## About Window Content

The current `Info` / `About` section uses the following app information:

```text
BlinkText v1.0
Fast, local text expansion tool designed for instant, reliable typing with zero delay.

Key Features
- Instant expansion without paste conflicts
- No trigger duplication during rapid input (e.g. Ctrl+V after trigger)
- Fully offline - no data collection
- Lightweight and optimized for speed
- Supports importing exported triggers from compatible tools

Compatibility
- Supports importing exported triggers from Beeftext for seamless migration

Privacy
- No user data is collected, stored, or transmitted

Developer
- Developed by: Eslam Mustafa
- Contact: Eslam.Youssef@protonmail.com / Eslam.G.Youssef@gmail.com
- GitHub: https://github.com/LeaDer-E
```

## Build

### Recommended Portable Build

The easiest way to build the portable version is to use:

```bat
src\01 - build_portable.bat
```

This script:
- compiles resources
- builds the executable
- outputs a portable EXE under:

```text
src\dist\BlinkText-Portable\BlinkText.exe
```

### Portable Build Details

The batch script builds:
- `main.cpp`
- `AppWindow.cpp`
- `SimpleJson.cpp`
- `BlinkText.rc`

And links against the required Windows libraries, including:
- `user32`
- `gdi32`
- `comdlg32`
- `shell32`
- `comctl32`
- `uxtheme`
- `dwmapi`
- `ole32`
- `uuid`
- `gdiplus`
- `imm32`

### Manual Build Example

If you want to build manually with MinGW:

```powershell
windres src\BlinkText.rc -O coff -o build\resource.o
g++ -std=c++17 -O2 -Wall -Wextra `
  -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX `
  src\main.cpp src\AppWindow.cpp src\SimpleJson.cpp build\resource.o `
  -o build\BlinkText.exe `
  -mwindows `
  -luser32 -lgdi32 -lcomdlg32 -lshell32 -lcomctl32 `
  -luxtheme -ldwmapi -lole32 -luuid -lgdiplus -limm32
```

## Installer

The Inno Setup installer script is included here:

```text
src\02 - BlinkText_Installer.iss
```

Compile it with Inno Setup Compiler to generate the installer.

Important current setup details:
- app name: `BlinkText`
- app version: `1.0.0`
- output filename: `BlinkText_Setup_v1.0.0.exe`

## Assets

Application assets are stored in:

```text
src\assets
```

Important files:
- `app.ico`
- `app_pause.ico`
- `github.png`
- `info.png`
- `icon.png`

## Project Structure

```text
BlinkText_cpp/
  src/
    assets/
      app.ico
      app_pause.ico
      github.png
      info.png
      icon.png
    01 - build_portable.bat
    02 - BlinkText_Installer.iss
    AppWindow.cpp
    AppWindow.h
    BlinkText.rc
    main.cpp
    resource.h
    SimpleJson.cpp
    SimpleJson.h
  README.md
```

## Developer

- Developed by: Eslam Mustafa
- Contact: Eslam.Youssef@protonmail.com
- Contact: Eslam.G.Youssef@gmail.com
- GitHub: https://github.com/LeaDer-E

## Notes

- BlinkText is built to stay lightweight, local-first, and responsive.
- The portable build workflow and installer workflow are both included in this repository.
- A Python-based reference version can be maintained separately if needed for legacy comparison or experimentation.
