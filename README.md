# BlinkText – Fast Local Text Expansion Tool (Beeftext Alternative)

Fast, local text expansion tool designed for instant, reliable typing with zero delay.

---

## 🚀 Download

👉 **Get the latest version:**
https://github.com/LeaDer-E/BlinkText/releases/latest

### Available Versions

* **Portable** → Run directly, no installation required
* **Installer** → Standard Windows setup

---

## ✨ Overview

BlinkText is a fast, local text expansion tool for Windows. It expands text instantly, remains responsive during normal typing and paste-heavy workflows, and runs fully offline with no cloud dependency.

This repository contains:

* Native Win32 C++ application
* Portable build workflow
* Installer script
* Import/export support for compatible tools

---

## ⚡ What BlinkText Does

BlinkText lets you type short triggers such as:

```text
sig\  -> email signature  
addr\ -> full address  
tr1\  -> translation template  
sum\  -> professional summary  
```

Everything runs locally:

* No telemetry
* No account
* No sync
* No internet required

---

## 🔥 Key Features

* Instant text expansion outside the app
* No trigger duplication during rapid input
* Built-in Test Area
* Group-based snippet organization
* Enable / disable snippets
* Global typing engine
* Light / Dark mode
* Tray support
* Start with Windows
* Global hotkey support
* Import / Export (BlinkText & Beeftext)
* Clipboard-based triggers (`\\` and `//`)

---

## 🧠 AI Contribution

This project was developed with the assistance of AI tools for:

* architecture design
* debugging
* performance improvements
* UI and system behavior refinement

The final implementation, structure, and integration were directed and validated manually.

---

## 🖥️ How to Use

### Portable

1. Download the portable version
2. Extract the ZIP
3. Run `BlinkText.exe`

### Installer

1. Download the setup
2. Run installer
3. Follow instructions

---

## 📦 Import / Export

Supported formats:

* BlinkText JSON
* Beeftext combos JSON

Options:

* Import into group
* Overwrite / skip conflicts
* Export all or by group

---

## 🔒 Privacy

BlinkText is fully offline.

* No data collection
* No remote storage
* No tracking
* No login

---

## 🛠️ Built With

* C++
* Win32 API
* GDI+

---

## ⚙️ Build

### Portable Build

```bat
src\build_portable.bat
```

Output:

```text
src\dist\BlinkText-Portable\BlinkText.exe
```

---

### Manual Build

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

---

## 📦 Installer

```text
src\BlinkText_Installer.iss
```

Compile using **Inno Setup**.

---

## 📁 Project Structure

```text
BlinkText/
  src/
    assets/
    build_portable.bat
    BlinkText_Installer.iss
    main.cpp
    AppWindow.cpp
    AppWindow.h
    SimpleJson.cpp
    SimpleJson.h
    resource.h
    BlinkText.rc
  README.md
```

---

## 👨‍💻 Developer

Eslam Mustafa
https://github.com/LeaDer-E

---

## 📄 License

MIT License
