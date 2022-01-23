// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _COLOR_H
#define _COLOR_H
#include <stdint.h>

// Simple rgb color class.
class Color
{
public:
    uint8_t r;
    uint8_t g;
    uint8_t b;

    uint32_t pack() const {
        // the Teensy colors are packed in GRB order.
        return (g << 16) + (r << 8) + b;
    }

    static Color from(uint32_t value)
    {
        uint8_t r = (uint8_t)((value >> 8) & 0xff);
        uint8_t g = (uint8_t)((value >> 16) & 0xff);
        uint8_t b = (uint8_t)(value & 0xff);
        return Color{ r, g, b };
    }

    static Color fromrgb(uint32_t value)
    {
        uint8_t r = (uint8_t)((value >> 16) & 0xff);
        uint8_t g = (uint8_t)((value >> 8) & 0xff);
        uint8_t b = (uint8_t)(value & 0xff);
        return Color{ r, g, b };
    }

    bool operator==(const Color& other) const
    {
        return r == other.r && g == other.g && b == other.b;
    }

    bool operator!=(const Color& other) const
    {
        return r != other.r || g != other.g || b != other.b;
    }
};

#endif