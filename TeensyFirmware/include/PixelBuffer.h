#ifndef _PIXELBUFFER_H
#define _PIXELBUFFER_H

#include <stdint.h>
#include "SimpleString.h"
#include "Timer.h"
#include "Color.h"
#include "Status.h"

#ifndef UNITTEST
#include "MultiWS2812.h"
#endif

// This class abstracts the 16 LED strips as one big pixel buffer that we can setup.
// It provides a "write" method which then sends the buffer to the Teensy.
class PixelBuffer
{
    uint32_t* pixBuffer = nullptr;
    MultiWS2812 strips;
    int numStrips;
    int ledsPerStrip;
    float fps = 0;

public:
    PixelBuffer(int numStrips, int ledsPerStrip) : strips(ledsPerStrip, numStrips)
    {
        this->numStrips = numStrips;
        this->ledsPerStrip = ledsPerStrip;
    }

    ~PixelBuffer()
    {
        if (pixBuffer != nullptr) {
            delete[] pixBuffer;
            pixBuffer = nullptr;
        }
        strips.setBuffer(nullptr);
    }

    void Initialize()
    {
        if (pixBuffer == nullptr)
        {
            pixBuffer = new uint32_t[GetNumberOfPixels()];
        }
        if (pixBuffer == nullptr)
        {
            CrashPrint("### PixelBuffer: out of memory!\r\n");
        }
        else
        {
            ::memset(pixBuffer, 0, GetBufferSize());
            strips.setBuffer(pixBuffer);
        }
    }

    void PrintStatus()
    {
        strips.printTimingCycleStats();
    }

	MultiWS2812& GetDriver() { return strips; }
    int NumStrips() { return numStrips; }
    int NumLedsPerStrip() { return ledsPerStrip; }
    float GetFps() { return fps; }

    uint32_t* GetPixelBuffer() { return pixBuffer; }

    uint32_t* CopyPixels()
    {
        auto result = new uint32_t[GetNumberOfPixels()];
        if (result == nullptr)
        {
            CrashPrint("### CopyPixels: out of memory!\r\n");
        }
        else
        {
            ::memcpy(result, pixBuffer, GetBufferSize());
        }
        return result;
    }

    uint32_t GetNumberOfPixels()
    {
        return (uint32_t)(numStrips * ledsPerStrip);
    }

    uint32_t GetBufferSize()
    {
        return (uint32_t)(GetNumberOfPixels() * sizeof(uint32_t));
    }

    void CopyFrom(uint32_t* source, uint32_t size)
    {
        uint32_t expectedSize = GetBufferSize();
        if (size > expectedSize)
        {
            size = expectedSize;
        }
        ::memcpy(pixBuffer, source, size);
    }

    void CopyTo(uint32_t* source, uint32_t size)
    {
        uint32_t expectedSize = GetBufferSize();
        if (size > expectedSize)
        {
            size = expectedSize;
        }
        ::memcpy(source, pixBuffer, size);
    }

    // Set entire strip to one color.
    void SetColor(Color color)
    {
        // pack colors according to neopixel format.
        uint32_t value = color.pack();
        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < ledsPerStrip; j++)
            {
                uint32_t* pixel = (uint32_t*)(pixBuffer + (j * numStrips) + i);
                *pixel = value;
            }
        }
    }

    // Set a pixel on all strips to this color (so it is a row across the strips).
    void SetRow(Color color, int index)
    {
        uint32_t value = color.pack();
        for (int j = 0; j < numStrips; j++)
        {
            uint32_t* pixel = (uint32_t*)(pixBuffer + (index * numStrips) + j);
            *pixel = value;
        }
    }

    // Set one color for every led in a strip.
    void SetColumn(Color color, int strip)
    {
        uint32_t value = color.pack();
        for (int i = 0; i < ledsPerStrip; i++)
        {
            uint32_t* pixel = (uint32_t*)(pixBuffer + (i * numStrips) + strip);
            *pixel = value;
        }
    }

    // Set individual colors for every led in a strip.
    void SetColumn(int strip, Color* colors, int numColors)
    {
        int i = 0;
        for (; i < ledsPerStrip && i < numColors; i++)
        {
            Color c = colors[i];
            SetPixel(c, strip, i);
        }
        Color black{0,0,0};
        // turn off everything else that was not specified.
        while (i++ < ledsPerStrip)
        {
            SetPixel(black, strip, i);
        }
    }

    inline void SetPixel(const Color& c, int strip, int led)
    {
        uint32_t* pixel = (uint32_t*)(pixBuffer + (led * numStrips) + strip);
        *pixel = c.pack();
    }

    inline Color GetPixel(int strip, int led)
    {
        uint32_t* pixel = (uint32_t*)(pixBuffer + (led * numStrips) + strip);
        return Color::from(*pixel);
    }

    void VerticalGradient(Color start, Color end)
    {
        float dr = (float)end.r - (float)start.r;
        float dg = (float)end.g - (float)start.g;
        float db = (float)end.b - (float)start.b;
        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < ledsPerStrip; j++)
            {
                uint32_t* pixel = (uint32_t*)(pixBuffer + (j * numStrips) + i);
                double percent = (double)j / ledsPerStrip;
                Color ic{ (uint8_t)(start.r + (percent * dr)),
                          (uint8_t)(start.g + (percent * dg)),
                          (uint8_t)(start.b + (percent * db)) };
                *pixel = ic.pack();
            }
        }
    }

    // Setup output pins for Ada:
    void setOutputPins() {

        // Setup output pins and initialize strips array->GPIO mapping
        pinMode(1, OUTPUT);
        pinMode(3, OUTPUT);

        // Level Shifter 1 (U1)
        strips.enableOutputPin(0, 22); // 6
        strips.enableOutputPin(1, 23); // 6
        strips.enableOutputPin(2, 19); // 6
        strips.enableOutputPin(3, 18); // 6
        strips.enableOutputPin(4, 17); // 6
        strips.enableOutputPin(5, 16); // 6
        strips.enableOutputPin(6, 15); // 6
        strips.enableOutputPin(7, 14); // 6

        // Level Shifter 2 (U2)
        strips.enableOutputPin(8, 21); // 6
        strips.enableOutputPin(1, 6); // 7
        strips.enableOutputPin(2, 7); // 7
        strips.enableOutputPin(3, 8); // 7
        strips.enableOutputPin(4, 9); // 7
        strips.enableOutputPin(5, 10); // 7
        strips.enableOutputPin(6, 11); // 7
        strips.enableOutputPin(7, 12); // 7
    }

    int Write()
    {
        strips.show();
        gTeensyStatus.draws++;
        return 0;
    }

};

#endif
