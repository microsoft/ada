#pragma once
#include <math.h>
#include <stdint.h>

#include "Color.h"

class HlsColor
{
public:
    float hue = 0;
    float luminance = 0;
    float saturation = 0;

    /// <summary>
    /// Constructs an instance of the class with the specified rgb color.
    /// </summary>
    HlsColor(Color c)
    {
        ToHLS(c.r, c.b, c.g);
    }

    /// <summary>
    /// Constructs an instance of the class with the specified hue, luminance
    /// and saturation values.
    /// </summary>
    /// <param name="hue">The Hue (between 0.0 and 360.0)</param>
    /// <param name="luminance">The Luminance (between 0.0 and 1.0)</param>
    /// <param name="saturation">The Saturation (between 0.0 and 1.0)</param>
    HlsColor(float hue, float luminance, float saturation)
    {
        SetHue(hue);
        SetLuminance(luminance);
        SetSaturation(saturation);
    }

    /// <summary>
    /// Constructs an instance of the class with the specified red, green and
    /// blue values.
    /// </summary>
    /// <param name="red">The red component.</param>
    /// <param name="green">The green component.</param>
    /// <param name="blue">The blue component.</param>
    HlsColor(uint8_t red, uint8_t green, uint8_t blue)
    {
        ToHLS(red, green, blue);
    }

    /// <summary>
    /// Constructs an instance of the class using the settings of another
    /// instance.
    /// </summary>
    /// <param name="HlsColor">The instance to clone.</param>
    HlsColor(const HlsColor& hls)
    {
        this->luminance = hls.luminance;
        this->hue = hls.hue;
        this->saturation = hls.saturation;
    }

    /// <summary>
    /// Sets the Luminance (0.0 to 1.0) of the color.
    /// </summary>
    float SetLuminance(float value)
    {
        luminance = maximum(0, minimum(1, value));
        return luminance;
    }

    /// <summary>
    /// Gets or sets the Hue (0.0 to 360.0) of the color
    /// </summary>
    float SetHue(float value)
    {
        hue = maximum(0, minimum(360.0f, value)); 
        return hue;
    }

    /// <summary>
    /// Gets or sets the Saturation (0.0 to 1.0) of the color.
    /// </summary>
    /// <exception cref="ArgumentOutOfRangeException">If the value is out of
    /// the acceptable range for saturation (0.0 to 1.0)</exception>
    float SetSaturation(float value)
    {
        saturation = maximum(0, minimum(1, value));
        return saturation;
    }
    
    /// <summary>
    /// Set an RGB color
    /// </summary>
    void SetRGB(const Color& c)
    {
        ToHLS(c.r, c.g, c.b);
    }

    /// <summary>
    /// Lightens the color by the specified amount by modifying
    /// the luminance (for example, 0.2 would lighten the color by 20%)
    /// </summary>
    void Lighten(float percent)
    {
        luminance *= (1.0f + percent);
        if (luminance > 1.0f)
        {
            luminance = 1.0f;
        }
    }

    /// <summary>
    /// Darkens the color by the specified amount by modifying
    /// the luminance (for example, 0.2 would darken the color by 20%)
    /// </summary>
    void Darken(float percent)
    {
        luminance *= (1 - percent);
    }

    void ToHLS(uint8_t red, uint8_t green, uint8_t blue)
    {
        uint8_t minval = (uint8_t)minimum(red, minimum(green, blue));
        uint8_t maxval = (uint8_t)maximum(red, maximum(green, blue));

        float mdiff = (float)(maxval - minval);
        float msum = (float)(maxval + minval);

        luminance = msum / 510.0f;

        if (maxval == minval)
        {
            saturation = 0.0f;
            hue = 0.0f;
        }
        else
        {
            float rnorm = (maxval - red) / mdiff;
            float gnorm = (maxval - green) / mdiff;
            float bnorm = (maxval - blue) / mdiff;

            saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff /
                (510.0f - msum));

            if (red == maxval)
            {
                hue = 60.0f * (6.0f + bnorm - gnorm);
            }
            if (green == maxval)
            {
                hue = 60.0f * (2.0f + rnorm - bnorm);
            }
            if (blue == maxval)
            {
                hue = 60.0f * (4.0f + gnorm - rnorm);
            }
            if (hue > 360.0f)
            {
                hue = hue - 360.0f;
            }
        }
    }

    /// <summary>
    /// Convert back to RGB color
    /// </summary>
    Color GetRGB()
    {
        Color c;

        if (saturation == 0.0)
        {
            c.r = c.g = c.b = (uint8_t)(luminance * 255.0F);
        }
        else
        {
            float rm1;
            float rm2;

            if (luminance <= 0.5f)
            {
                rm2 = luminance + luminance * saturation;
            }
            else
            {
                rm2 = luminance + saturation - luminance * saturation;
            }
            rm1 = 2.0f * luminance - rm2;
            c.r = ToRGB1(rm1, rm2, hue + 120.0f);
            c.g = ToRGB1(rm1, rm2, hue);
            c.b = ToRGB1(rm1, rm2, hue - 120.0f);
        }
        return c;
    }

private:
    uint8_t ToRGB1(float rm1, float rm2, float rh)
    {
        if (rh > 360.0f)
        {
            rh -= 360.0f;
        }
        else if (rh < 0.0f)
        {
            rh += 360.0f;
        }

        if (rh < 60.0f)
        {
            rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;
        }
        else if (rh < 180.0f)
        {
            rm1 = rm2;
        }
        else if (rh < 240.0f)
        {
            rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;
        }

        return (uint8_t)(rm1 * 255);
    }

    float minimum(float a, float b)
    {
        if (a < b) return a;
        return b;
    }

    float maximum(float a, float b)
    {
        if (a > b) return a;
        return b;
    }
};