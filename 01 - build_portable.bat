@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "APP_NAME=BlinkText"
set "APP_VERSION=1.1.0"
set "SRC_DIR=src"
set "DIST_DIR=dist"
set "RELEASE_DIR=build_release"
set "OUT_DIR=%DIST_DIR%\BlinkText-Portable"
set "TEMP_BUILD_DIR=%RELEASE_DIR%\_build"
set "RELEASE_EXE=%RELEASE_DIR%\%APP_NAME%.exe"
set "OUT_EXE=%OUT_DIR%\%APP_NAME%.exe"
set "OUT_ZIP=%DIST_DIR%\%APP_NAME%_Portable_v%APP_VERSION%.zip"
set "SNIPPETS_FILE=%APP_NAME%_Snippets.json"

echo =====================================
echo Building BlinkText Portable Package...
echo =====================================

if exist "%OUT_DIR%" (
    echo Cleaning previous portable output...
    rmdir /s /q "%OUT_DIR%"
)
if exist "%RELEASE_DIR%" (
    echo Cleaning previous release staging output...
    rmdir /s /q "%RELEASE_DIR%"
)
mkdir "%OUT_DIR%" || goto :mkdir_failed
mkdir "%RELEASE_DIR%" || goto :mkdir_failed
mkdir "%TEMP_BUILD_DIR%" || goto :mkdir_failed

echo [1/6] Locating compiler tools...
where g++.exe >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++.exe was not found in PATH.
    echo Please open a terminal where MinGW-w64 is available.
    goto :fail
)
where windres.exe >nul 2>nul
if errorlevel 1 (
    echo [ERROR] windres.exe was not found in PATH.
    echo Please open a terminal where MinGW-w64 is available.
    goto :fail
)

set "MINGW_BIN="
for /f "delims=" %%I in ('where g++.exe 2^>nul') do (
    if not defined MINGW_BIN set "MINGW_BIN=%%~dpI"
)
if not defined MINGW_BIN (
    echo [ERROR] Failed to detect the MinGW bin directory.
    goto :fail
)
echo Using MinGW bin: %MINGW_BIN%

echo [2/6] Compiling resources...
windres "%SRC_DIR%\BlinkText.rc" -O coff -o "%TEMP_BUILD_DIR%\resource.o"
if errorlevel 1 (
    echo [ERROR] Resource compilation failed.
    goto :fail
)

echo [3/6] Building executable...
g++ -std=c++17 -O2 -Wall -Wextra ^
 -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX ^
 "%SRC_DIR%\main.cpp" "%SRC_DIR%\AppWindow.cpp" "%SRC_DIR%\SimpleJson.cpp" "%TEMP_BUILD_DIR%\resource.o" ^
 -o "%RELEASE_EXE%" ^
 -mwindows ^
 -luser32 -lgdi32 -lcomdlg32 -lshell32 -lcomctl32 ^
 -luxtheme -ldwmapi -lole32 -luuid -lgdiplus -limm32
if errorlevel 1 (
    echo [ERROR] Executable build failed.
    goto :fail
)

echo [4/6] Copying runtime files if needed...
for %%F in (
    libgcc_s_seh-1.dll
    libgcc_s_dw2-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    libssp-0.dll
) do (
    if exist "%MINGW_BIN%%%F" (
        copy /y "%MINGW_BIN%%%F" "%RELEASE_DIR%\%%F" >nul
        echo   Included %%F
    )
)

echo [5/6] Preparing portable data files...
set "SOURCE_SNIPPETS="
for %%F in (
    "%APP_NAME%_Snippets.json"
    "build\%SNIPPETS_FILE%"
    "build_diagnostics\%SNIPPETS_FILE%"
    "build_keyrollback\%SNIPPETS_FILE%"
    "build_keyspeed\%SNIPPETS_FILE%"
) do (
    if not defined SOURCE_SNIPPETS if exist %%~F set "SOURCE_SNIPPETS=%%~F"
)
if defined SOURCE_SNIPPETS (
    copy /y "%SOURCE_SNIPPETS%" "%RELEASE_DIR%\%SNIPPETS_FILE%" >nul
    echo   Included %SNIPPETS_FILE%
) else (
    echo {^"settings^":{},^"groups^":[],^"snippets^":[]}>"%RELEASE_DIR%\%SNIPPETS_FILE%"
    echo   Created fallback %SNIPPETS_FILE%
)

echo [5.5/6] Preparing portable mirror...
copy /y "%RELEASE_EXE%" "%OUT_EXE%" >nul
for %%F in (
    libgcc_s_seh-1.dll
    libgcc_s_dw2-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    libssp-0.dll
    %SNIPPETS_FILE%
) do (
    if exist "%RELEASE_DIR%\%%F" copy /y "%RELEASE_DIR%\%%F" "%OUT_DIR%\%%F" >nul
)

echo [6/6] Creating ZIP archive...
if exist "%OUT_ZIP%" del /f /q "%OUT_ZIP%" >nul 2>nul
powershell -NoProfile -Command "Compress-Archive -Path '%OUT_DIR%\\*' -DestinationPath '%OUT_ZIP%' -CompressionLevel Optimal"
if errorlevel 1 (
    echo [WARN] ZIP archive creation failed, but the portable folder is ready.
)

if exist "%TEMP_BUILD_DIR%" rmdir /s /q "%TEMP_BUILD_DIR%"

echo.
echo [DONE] Portable package is ready.
echo RELEASE DIR : %RELEASE_DIR%
echo RELEASE EXE : %RELEASE_EXE%
echo PORTABLE DIR: %OUT_DIR%
echo PORTABLE EXE: %OUT_EXE%
if exist "%OUT_ZIP%" echo ZIP : %OUT_ZIP%
if not "%BLINKTEXT_NO_PAUSE%"=="1" pause
popd
exit /b 0

:mkdir_failed
echo [ERROR] Failed to create output folders.
goto :fail

:fail
echo.
echo [FAILED] Portable build did not complete.
if not "%BLINKTEXT_NO_PAUSE%"=="1" pause
if exist "%TEMP_BUILD_DIR%" rmdir /s /q "%TEMP_BUILD_DIR%" >nul 2>nul
popd
exit /b 1
