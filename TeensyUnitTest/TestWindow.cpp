// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include <iostream>
#include <sstream>

#include "TestWindow.h"
#include "Bitmap.h"
#include "HlsColor.h"


#define WM_UPDATEBITMAP WM_USER
const int GWL_USERDATA = -21;

extern "C"
{

    LRESULT TestWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_CREATE:
            {
                TestWindow* window = (TestWindow*)((LPCREATESTRUCT)lParam)->lpCreateParams;
                SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)window);
            }
            break;
        case WM_SIZE:
            {
                int width = LOWORD(lParam);  // Macro to get the low-order word.
                int height = HIWORD(lParam); // Macro to get the high-order word.
                TestWindow* window = (TestWindow*)GetWindowLongPtr(hWnd, GWL_USERDATA);
                window->OnResize(width, height);
            }
            break;
        case WM_PAINT:
            {
                TestWindow* window = (TestWindow*)GetWindowLongPtr(hWnd, GWL_USERDATA);
                window->OnPaint();
            }
            break;
        case WM_CLOSE:
            {
                TestWindow* window = (TestWindow*)GetWindowLongPtr(hWnd, GWL_USERDATA);
                window->OnClose();
                return 0;
            }
            break;
        case WM_UPDATEBITMAP:
            {
                TestWindow * window = (TestWindow*)GetWindowLongPtr(hWnd, GWL_USERDATA);
                window->OnUpdateBitmap();
                return 0;
            }
            break;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

struct TestWindow::TestWindowImpl
{
public:
    HWND hwnd;
    HINSTANCE hinstance;
    int width = 0;
    int height = 0;
    Bitmap* bitmap = NULL;
    HBITMAP hbmp = NULL;
    bool closed = false;
};

TestWindow::TestWindow()
{
    _impl = new TestWindow::TestWindowImpl();

    _impl->hinstance = GetModuleHandle(NULL);

    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Test Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc = (WNDPROC)&TestWindowProc;
    wc.hInstance = _impl->hinstance;
    wc.lpszClassName = CLASS_NAME;
    ATOM atom = RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Test Window",                 // Window text
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,       // Parent window
        NULL,       // Menu
        _impl->hinstance,  // Instance handle
        (LPVOID)this    // Additional application data
    );

    if (hwnd == NULL)
    {
        auto hr = GetLastError();
        std::cout << "Error creating window " << hr << "\n";
    }
    _impl->hwnd = hwnd;
}

TestWindow::~TestWindow()
{
    OnClose();

    if (_impl->hbmp != NULL)
    {
        ::DeleteObject(_impl->hbmp);
        delete _impl->bitmap;
    }

    if (_impl->hwnd != NULL)
    {
        DestroyWindow(_impl->hwnd);
        _impl->hwnd = NULL;
    }
    delete _impl;
    _impl = NULL;
}

void TestWindow::DrawBitmap(const Bitmap& bitmap)
{
    if (_impl->hbmp != NULL)
    {
        ::DeleteObject(_impl->hbmp);
        _impl->hbmp = NULL;
        delete _impl->bitmap;
    }
    _impl->bitmap = new Bitmap(bitmap);
    ::PostMessage(_impl->hwnd, WM_UPDATEBITMAP, 0, 0);
}

void TestWindow::OnUpdateBitmap()
{
    int w = _impl->bitmap->GetWidth();
    int h = _impl->bitmap->GetHeight();

    // create a windows bitmap bitmap
    std::stringstream mem(std::ios::binary | std::ios::out | std::ios::in);
    _impl->bitmap->SaveBitmap(mem);
    int pos = (int)mem.tellp();
    char* buffer = new char[pos];
    mem.seekg(0);
    mem.read(buffer, pos);

    BITMAPFILEHEADER* bmfh = (BITMAPFILEHEADER*)buffer;
    BITMAPINFOHEADER* bmih = (BITMAPINFOHEADER*)(buffer + sizeof(BITMAPFILEHEADER));
    BITMAPINFO* bmi;
    bmi = (BITMAPINFO*)bmih;

    void* bits;
    bits = (void*)(buffer + bmfh->bfOffBits);

    if (_impl->hbmp != NULL)
    {
        ::DeleteObject(_impl->hbmp);
        _impl->hbmp = NULL;
    }

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(_impl->hwnd, &ps);
    _impl->hbmp = CreateDIBitmap(hdc, bmih, CBM_INIT, bits, bmi, DIB_RGB_COLORS);

    delete[] buffer;
    ::ReleaseDC(NULL, hdc);
    Invalidate();
}

void TestWindow::SetSize(int width, int height)
{
    ::SetWindowPos(_impl->hwnd, NULL, 0, 0, width, height, SWP_NOREPOSITION);
}

void TestWindow::Run()
{
    ShowWindow(_impl->hwnd, SW_NORMAL);
    MSG msg = { };
    while (_impl != NULL && !_impl->closed && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void TestWindow::Invalidate() {

    RECT bounds{ 0, 0, _impl->width, _impl->height };
    ::InvalidateRect(_impl->hwnd, &bounds, FALSE);
}

void TestWindow::OnResize(int width, int height)
{
    _impl->width = width;
    _impl->height = height;
    Invalidate();
}

void TestWindow::OnPaint()
{
    PAINTSTRUCT ps;
    auto hwnd = _impl->hwnd;
    HDC hdc = BeginPaint(hwnd, &ps);

    if (_impl->hbmp != NULL)
    {
        int w = _impl->width;
        int h = _impl->height;
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP saved = (HBITMAP)SelectObject(memDC, _impl->hbmp);
        StretchBlt(hdc, 0, 0, w, h, memDC, 0, 0, _impl->bitmap->GetWidth(), _impl->bitmap->GetHeight(), SRCCOPY);
        SelectObject(memDC, saved);
        DeleteDC(memDC);
    }
    else
    {
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
    }
    EndPaint(hwnd, &ps);

    ::ReleaseDC(NULL, hdc);
}

void TestWindow::Close()
{
    ::PostMessage(_impl->hwnd, WM_CLOSE, 0, 0);
}

void TestWindow::OnClose()
{
    _impl->closed = true;
}