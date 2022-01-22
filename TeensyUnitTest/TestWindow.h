#pragma once

#include <stdint.h>
#include "Bitmap.h"

class TestWindow
{
public:
    TestWindow();
    ~TestWindow();
    void SetSize(int width, int height);
    void DrawBitmap(const Bitmap& bitmap);
    void Run(); // run the message loop
    void OnResize(int width, int height);
    void OnPaint();
    void OnClose();
    void Invalidate();
    void Close();
    void OnUpdateBitmap();
private:
    struct TestWindowImpl;
    TestWindowImpl* _impl;
};
