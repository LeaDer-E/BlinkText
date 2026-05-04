@echo off
setlocal

set APP_NAME=BlinkText
set OUT_DIR=dist\BlinkText-Portable

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo [1/3] Compiling resources...
windres BlinkText.rc -O coff -o resource.o
if %errorlevel% neq 0 (
echo [ERROR] Resource compilation failed.
pause
exit /b 1
)

echo [2/3] Building EXE...
g++ -std=c++17 -O2 -Wall -Wextra ^
-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX ^
main.cpp AppWindow.cpp SimpleJson.cpp resource.o ^
-o "%OUT_DIR%\%APP_NAME%_Portable_v1.0.0.exe" ^
-mwindows ^
-static -static-libgcc -static-libstdc++ ^
-luser32 -lgdi32 -lcomdlg32 -lshell32 -lcomctl32 ^
-luxtheme -ldwmapi -lole32 -luuid ^
-lgdiplus -limm32

if %errorlevel% neq 0 (
echo [FAILED] Build did not complete.
pause
exit /b 1
)

echo [3/3] Done!
echo Output: %OUT_DIR%\%APP_NAME%.exe
pause
