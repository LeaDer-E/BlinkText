@echo off
setlocal EnableExtensions
pushd "%~dp0"

set "PORTABLE_SCRIPT=01 - build_portable.bat"
set "INSTALLER_SCRIPT=02 - BlinkText_Installer.iss"
set "ISCC_PATH="

echo =====================================
echo Building BlinkText Installer...
echo =====================================

if not exist "%PORTABLE_SCRIPT%" (
    echo [ERROR] Missing file: %PORTABLE_SCRIPT%
    goto :fail
)

if not exist "%INSTALLER_SCRIPT%" (
    echo [ERROR] Missing file: %INSTALLER_SCRIPT%
    goto :fail
)

echo [1/3] Building release files from source...
set "BLINKTEXT_NO_PAUSE=1"
call "%PORTABLE_SCRIPT%"
if errorlevel 1 (
    echo [ERROR] Portable/release build failed.
    goto :fail
)

echo [2/3] Locating Inno Setup Compiler...
for /f "delims=" %%I in ('where ISCC.exe 2^>nul') do (
    if not defined ISCC_PATH set "ISCC_PATH=%%I"
)

if not defined ISCC_PATH if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC_PATH=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not defined ISCC_PATH if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC_PATH=%ProgramFiles%\Inno Setup 6\ISCC.exe"

if not defined ISCC_PATH (
    echo [ERROR] ISCC.exe was not found.
    echo Please install Inno Setup 6, or add ISCC.exe to PATH.
    goto :fail
)

echo Using ISCC: %ISCC_PATH%

echo [3/3] Compiling installer...
"%ISCC_PATH%" "%INSTALLER_SCRIPT%"
if errorlevel 1 (
    echo [ERROR] Installer build failed.
    goto :fail
)

echo.
echo [DONE] Installer build completed successfully.
echo Output folder: installer_output
if not "%BLINKTEXT_NO_PAUSE%"=="1" pause
popd
exit /b 0

:fail
echo.
echo [FAILED] Installer build did not complete.
if not "%BLINKTEXT_NO_PAUSE%"=="1" pause
popd
exit /b 1
