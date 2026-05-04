#include <windows.h>
#include <olectl.h>
#include <gdiplus.h>

#include "resource.h"
#include "AppWindow.h"

using namespace Gdiplus;

Image* LoadPngFromResource(HINSTANCE hInstance, int resourceId) {
    HRSRC hResource = FindResourceW(hInstance, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!hResource) return nullptr;

    DWORD imageSize = SizeofResource(hInstance, hResource);
    if (imageSize == 0) return nullptr;

    HGLOBAL hLoadedResource = LoadResource(hInstance, hResource);
    if (!hLoadedResource) return nullptr;

    void* imageData = LockResource(hLoadedResource);
    if (!imageData) return nullptr;

    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (!hBuffer) return nullptr;

    void* buffer = GlobalLock(hBuffer);
    if (!buffer) {
        GlobalFree(hBuffer);
        return nullptr;
    }

    CopyMemory(buffer, imageData, imageSize);
    GlobalUnlock(hBuffer);

    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(hBuffer, TRUE, &stream) != S_OK) {
        GlobalFree(hBuffer);
        return nullptr;
    }

    Image* image = Image::FromStream(stream);
    stream->Release();

    if (!image || image->GetLastStatus() != Ok) {
        delete image;
        return nullptr;
    }

    return image;
}

void ShowSplash(HINSTANCE hInstance) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR token = 0;

    if (GdiplusStartup(&token, &gdiplusStartupInput, nullptr) != Ok) {
        Sleep(3000);
        return;
    }

    Image* img = LoadPngFromResource(hInstance, IDR_SPLASH_PNG);
    if (!img) {
        GdiplusShutdown(token);
        Sleep(3000);
        return;
    }

    const int width = 320;
    const int height = 320;

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);

    int x = (screen_w - width) / 2;
    int y = (screen_h - height) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"STATIC",
        nullptr,
        WS_POPUP,
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        delete img;
        GdiplusShutdown(token);
        Sleep(3000);
        return;
    }

    HDC screen_dc = GetDC(nullptr);
    HDC mem_dc = CreateCompatibleDC(screen_dc);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(screen_dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(mem_dc, hBitmap));

    {
        Graphics graphics(mem_dc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        graphics.Clear(Color(0, 0, 0, 0));
        graphics.DrawImage(img, 0, 0, width, height);
    }

    POINT ptSrc{0, 0};
    POINT ptDst{x, y};
    SIZE sizeWnd{width, height};

    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(
        hwnd,
        screen_dc,
        &ptDst,
        &sizeWnd,
        mem_dc,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    Sleep(2000);

    SelectObject(mem_dc, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(mem_dc);
    ReleaseDC(nullptr, screen_dc);

    DestroyWindow(hwnd);
    delete img;
    GdiplusShutdown(token);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int command_show) {
    ShowSplash(instance);

    AppWindow window;
    if (!window.Create(instance)) {
        return -1;
    }

    window.Show(command_show);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0)) {
        if (window.TranslateAppAccelerator(&message)) {
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}