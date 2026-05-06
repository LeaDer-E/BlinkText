# BlinkText Packaging README

This short guide explains how to build the Portable and Installer versions of BlinkText.

## Files

- `00 - BlinkText_Version.issinc`
- `01 - build_portable.bat`
- `02 - BlinkText_Installer.iss`

## Version Source

BlinkText version metadata is stored in one place:

```text
00 - BlinkText_Version.issinc
```

Update that file first when preparing a new release.

It currently defines:

- app name
- app version
- publisher
- exe name

## Step 1: Build the Portable Files

Run:

```bat
01 - build_portable.bat
```

This script will:

- build `BlinkText.exe`
- create a clean release folder
- copy required runtime DLL files
- copy `BlinkText_Snippets.json`
- create a portable folder
- create a ZIP archive

It also reads the current version automatically from:

```text
00 - BlinkText_Version.issinc
```

## Output Folders

### Main release source

```text
build_release
```

This folder contains the final files used by the installer.

### Portable folder

```text
dist\BlinkText-Portable
```

This folder contains the ready-to-run portable version.

### Portable ZIP

```text
dist\BlinkText_Portable_v1.1.0.zip
```

This file is ready to share directly.

## Step 2: Build the Installer

Open:

```text
02 - BlinkText_Installer.iss
```

Then compile it using Inno Setup.

Important:

- the installer does **not** depend on the portable folder
- it reads files directly from:

```text
build_release
```
- and it reads version/app metadata from:

```text
00 - BlinkText_Version.issinc
```

## What the Installer Includes

The installer package includes:

- `BlinkText.exe`
- required DLL files
- `BlinkText_Snippets.json`

## Requirements

To build successfully, you typically need:

- MinGW-w64 with `g++` and `windres` available in `PATH`
- Inno Setup for the installer build

## Notes

- If you only want a portable version, running the batch file is enough.
- If you want the installer, run the batch file first, then compile the `.iss` file.
- The batch file also creates a ZIP archive automatically.
