// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _ANIMATIONS_H
#define _ANIMATIONS_H

#include <math.h>
#include "SimpleString.h"
#include "PixelBuffer.h"
#include "Timer.h"
#include "Vector.h"
#include "HlsColor.h"

enum class AnimationType
{
    Breathe,
    CrossFade,
    Fire,
    Gradient,
    MovingGradient,
    NeuralDrop,
    Rainbow,
    Rain,
    Twinkle,
    WaterDrop,
    CopySource
};

class Animation
{
protected:
    PixelBuffer &buffer;
    Timer timer;
    int frames = 0;
    float fps = 0;
    Animation *overlay = nullptr;
    const double pi = 3.14159265358979323846; // pi
    const int MaxName = 100;
    AnimationType type;

public:
    Animation(AnimationType type, PixelBuffer &buffer) : buffer(buffer), type(type)
    {
    }

    AnimationType GetType()
    {
        return type;
    }

    virtual ~Animation()
    {
        if (this->overlay != nullptr)
        {
            delete this->overlay;
            this->overlay = nullptr;
        }
    }

    // run one timeslice of the animation and return true when the animation has finished.
    virtual bool Run()
    {
        Draw();
        return false; // runs forever
    };

    virtual void Draw()
    {
        RunOverlay();

        if (this->IsOverlay())
        {
            // if this is an overlay it does not write the buffer, the owning Animation needs to do that.
            return;
        }

        buffer.Write();
        frames++;
    }

    virtual void Stop() {}

    virtual void AddOverlay(Animation *new_overlay)
    {
        if (this->overlay != nullptr)
        {
            delete this->overlay;
        }
        this->overlay = new_overlay;
    }

    virtual Animation *RemoveOverlay()
    {
        auto result = this->overlay;
        this->overlay = nullptr;
        return result;
    }

    virtual Animation *GetOverlay()
    {
        return this->overlay;
    }

    virtual bool RunOverlay()
    {
        if (this->overlay != nullptr)
        {
            return this->overlay->Run();
        }
        return true;
    }

    virtual SimpleString GetName()
    {
        return "Animation";
    }

    float GetFps() { return fps; }

    virtual bool IsOverlay() { return false; }
};

class BreatheAnimation : public Animation
{
private:
    uint32_t *snapshot = nullptr;
    float seconds = 0;
    bool forever = false;
    float f1;
    float f2;
    float ramp = 0;
    const float breathing_time = 2e3; // 2 seconds
public:
    BreatheAnimation(PixelBuffer &buffer, float seconds, float f1, float f2) : Animation(AnimationType::Breathe, buffer)
    {
        this->seconds = seconds;
        this->f1 = f1;
        this->f2 = f2;
        ramp = 1 / (float)exp(cos(0)) / f1 + f2;
        forever = (seconds == 0);
        snapshot = buffer.CopyPixels();
        timer.start();
    }

    ~BreatheAnimation()
    {
        delete[] snapshot;
    }

    SimpleString GetName() override
    {
        return "BreatheAnimation";
    }

    // run one timeslice of the animation and return
    bool Run() override
    {
        if (snapshot == nullptr)
        {
            return true;
        }
        int numStrips = buffer.NumStrips();
        int ledsPerStrip = buffer.NumLedsPerStrip();

        uint32_t *original = snapshot;

        if (forever || timer.seconds() < seconds)
        {
            for (int i = 0; i < numStrips; i++)
            {
                for (int j = 0; j < ledsPerStrip; j++)
                {
                    float millis = timer.milliseconds();
                    // this produces a nice sin curve with a bit of an extended quiet time between each peak.
                    // it produces numbers ranging from about 0.89 to 1.47.
                    float amplitude = ramp * (float)exp(cos(millis / breathing_time * 3.1415926)) / f1 + f2;
                    if (amplitude > 1)
                    {
                        amplitude = 1;
                    }

                    uint32_t offset = (j * numStrips) + i;
                    uint32_t *source = (uint32_t *)(original + offset);
                    Color basecolor = Color::from(*source);

                    float r = Clamped((float)basecolor.r, amplitude);
                    float g = Clamped((float)basecolor.g, amplitude);
                    float b = Clamped((float)basecolor.b, amplitude);

                    buffer.SetPixel(Color{(uint8_t)r, (uint8_t)g, (uint8_t)b}, i, j);
                }
            }
            if (ramp > 1)
            {
                ramp -= 0.001f; // about a 5 second ramp down to the color
            }
            else
            {
                ramp = 1;
            }

            Draw();
            return false;
        }
        else
        {
            return true;
        }
    }

    void Stop() override
    {
        int numStrips = buffer.NumStrips();
        int ledsPerStrip = buffer.NumLedsPerStrip();

        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < ledsPerStrip; j++)
            {
                uint32_t offset = (j * numStrips) + i;
                uint32_t *source = (uint32_t *)(snapshot + offset);
                Color basecolor = Color::from(*source);
                buffer.SetPixel(basecolor, i, j);
            }
        }
    }

    float Clamped(float c, float amplitude)
    {
        c *= amplitude;
        if (c > 255)
            c = 255;
        if (c < 0)
            c = 0;
        return c;
    }
};

// Similar to NeuralDrop but maintains the existing background colors.
class WaterDropAnimation : public Animation
{

private:
    uint32_t *snapshot = nullptr;
    int drops = 0;
    bool forever = false;
    int size = 0;
    int index = 0;
    float amount = 0;
    Vector<float> amplitudes;

public:
    WaterDropAnimation(PixelBuffer &buffer, int drops, int bubble_size, float amount) : Animation(AnimationType::WaterDrop, buffer)
    {
        this->drops = drops;
        this->size = bubble_size;
        this->amount = amount;

        forever = (drops == 0);
        snapshot = buffer.CopyPixels();

        float start = 0;
        for (int i = 0; i < bubble_size; i++)
        {
            float amplitude = pow(sin(start), (float)2.0);
            amplitudes.push_back(amplitude);
            start += (float)(pi / 15);
        }

        index = 0;
        timer.start();
    }

    ~WaterDropAnimation()
    {
        delete[] snapshot;
    }

    SimpleString GetName() override
    {
        return "WaterDropAnimation";
    }

    // run one timeslice of the animation and return
    bool Run() override
    {
        if (forever || drops > 0)
        {
            int numStrips = buffer.NumStrips();
            int ledsPerStrip = buffer.NumLedsPerStrip();
            uint32_t *original = snapshot;

            for (int i = 0; i < numStrips; i++)
            {
                for (int j = 0; j < ledsPerStrip; j++)
                {
                    uint32_t *source = (uint32_t *)(original + (j * numStrips) + i);
                    Color basecolor = Color::from(*source);
                    float r = (float)basecolor.r;
                    float g = (float)basecolor.g;
                    float b = (float)basecolor.b;

                    // Paint the animating water droplet falling down the strip:
                    int offset = j - index;
                    if (offset >= 0 && offset < size)
                    {
                        float percent = amplitudes[offset] * amount;
                        r = Clamp(r + percent);
                        g = Clamp(g + percent);
                        b = Clamp(b + percent);
                    }

                    buffer.SetPixel(Color{(uint8_t)r, (uint8_t)g, (uint8_t)b}, i, j);
                }
            }

            Draw();

            index++;
            if (index == ledsPerStrip)
            {
                index = 0;
                drops--;
            }
            return false;
        }
        return true;
    }

    void Stop() override
    {
        int numStrips = buffer.NumStrips();
        int ledsPerStrip = buffer.NumLedsPerStrip();

        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < ledsPerStrip; j++)
            {
                uint32_t *source = (uint32_t *)(snapshot + (j * numStrips) + i);
                Color basecolor = Color::from(*source);
                buffer.SetPixel(basecolor, i, j);
            }
        }
    }

    float Clamp(float c)
    {
        if (c > 255)
            return 255;
        return c;
    }
};

// This draws a falling rain animation on top of whatever is currently there
// So this is designed to be an overlay on top of something else that constantly
// refreshes the underlying buffer with something else (like a smooth fade).
// In other words this is like WaterDropAnimation without the snapshot.
class RainOverlayAnimation : public Animation
{
private:
    int size = 0;
    int index = 0;
    float amount = 0;
    Vector<float> amplitudes;

public:
    RainOverlayAnimation(PixelBuffer &buffer, int bubble_size, float amount) : Animation(AnimationType::Rain, buffer)
    {
        this->size = bubble_size;
        this->amount = amount;
        float start = 0;
        for (int i = 0; i < bubble_size; i++)
        {
            float amplitude = pow(sin(start), (float)2.0);
            amplitudes.push_back(amplitude);
            start += (float)(pi / bubble_size);
        }
        index = 0;
        timer.start();
    }

    bool IsOverlay() override { return true; }

    SimpleString GetName() override
    {
        return "RainOverlayAnimation";
    }

    // run one timeslice of the animation and return
    bool Run() override
    {
        int numStrips = buffer.NumStrips();
        int ledsPerStrip = buffer.NumLedsPerStrip();

        for (int i = 0; i < numStrips; i++)
        {
            for (int j = index; j < index + size && j < ledsPerStrip; j++)
            {
                auto basecolor = buffer.GetPixel(i, j);
                float r = (float)basecolor.r;
                float g = (float)basecolor.g;
                float b = (float)basecolor.b;

                // Paint the animating water droplet falling down the strip:
                int offset = j - index;
                if (offset >= 0 && offset < size)
                {
                    float percent = amplitudes[offset] * amount;
                    r = Clamp(r + percent);
                    g = Clamp(g + percent);
                    b = Clamp(b + percent);
                }

                buffer.SetPixel(Color{(uint8_t)r, (uint8_t)g, (uint8_t)b}, i, j);
            }
        }

        Draw();

        index++;
        if (index == ledsPerStrip)
        {
            index = 0;
        }

        // runs forever
        return false;
    }

    float Clamp(float c)
    {
        if (c > 255)
            return 255;
        return c;
    }
};

class CopySourceAnimation : public Animation
{
protected:
    uint32_t *snapshot = nullptr;

public:
    CopySourceAnimation(PixelBuffer &buffer) : Animation(AnimationType::CopySource, buffer)
    {
        snapshot = buffer.CopyPixels();
    }
    ~CopySourceAnimation()
    {
        delete[] snapshot;
        snapshot = nullptr;
    }

    SimpleString GetName() override
    {
        return "CopySourceAnimation";
    }

    bool Run() override
    {
        bool rc = true;
        if (snapshot != nullptr)
        {
            CopySnapshot();
            rc = RunOverlay(); // only run as long as we have an overlay
            buffer.Write();
            frames++;
        }
        return rc;
    }

    void CopySnapshot()
    {
        if (snapshot != nullptr)
        {
            buffer.CopyFrom(snapshot, buffer.GetBufferSize());
        }
    }

    void UpdateSnapshot()
    {
        if (snapshot != nullptr)
        {
            buffer.CopyTo(snapshot, buffer.GetBufferSize());
        }
    }
};

// This can be used as a base class to fade the background under another animation
// from the original target buffer contents to the new buffer contents over a given
// number of seconds.
class BaseCrossFadeAnimation : public Animation
{
protected:
    PixelBuffer &target;
    float seconds = 0;
    uint32_t *original = nullptr;
    bool hold = false; // whether to hold final position and never complete the animation.
public:
    BaseCrossFadeAnimation(PixelBuffer &origin, PixelBuffer &target, float seconds) : Animation(AnimationType::CrossFade, origin), target(target)
    {
        this->seconds = seconds;
    }

    ~BaseCrossFadeAnimation()
    {
        if (original != nullptr)
        {
            delete[] original;
            original = nullptr;
        }
    }

    SimpleString GetName() override
    {
        return "BaseCrossFadeAnimation";
    }

    // Call start to create a snapshot of the origin buffer and start the timer.
    void Start()
    {
        if (original != nullptr)
        {
            delete[] original;
        }
        original = buffer.CopyPixels();
        timer.start();
    }

    bool Run() override
    {
        size_t ledsPerStrip = buffer.NumLedsPerStrip();
        size_t numStrips = buffer.NumStrips();
        uint32_t *pixelBuffer = buffer.GetPixelBuffer();

        // cross fade to a target buffer.
        if (timer.seconds() < seconds && original != nullptr)
        {
            float percent = 1 - ((float)seconds - (float)timer.seconds()) / (float)seconds;
            uint32_t *goal = target.GetPixelBuffer();
            uint32_t *before = original;
            uint32_t *pixels = pixelBuffer;
            uint32_t position = 0;
            for (size_t i = 0; i < numStrips; i++)
            {
                for (size_t j = 0; j < ledsPerStrip; j++)
                {
                    // extract previous colors, could lift this out of the loop, but it would be a lot of floats!
                    uint32_t value = *before;
                    Color previous = Color::from(value);

                    Color tc = Color::from(*goal); // get target colors

                    float dr = (float)tc.r - (float)previous.r;
                    float dg = (float)tc.g - (float)previous.g;
                    float db = (float)tc.b - (float)previous.b;

                    // interpolate to the new color.
                    Color ic{(uint8_t)(previous.r + (percent * dr)),
                             (uint8_t)(previous.g + (percent * dg)),
                             (uint8_t)(previous.b + (percent * db))};
                    value = ic.pack();
                    *pixels = value;
                    pixels++;
                    before++;
                    goal++;
                    position++;
                }
            }

            // this base class does not call Draw because it lets the subclass take care of that.
            return false;
        }
        else
        {
            Stop();
            return !hold;
        }
    }

    void Stop() override
    {
        size_t ledsPerStrip = buffer.NumLedsPerStrip();
        size_t numStrips = buffer.NumStrips();
        uint32_t *pixelBuffer = buffer.GetPixelBuffer();
        fps = (float)frames / (float)seconds;
        // write the final values.
        uint32_t size = (uint32_t)(ledsPerStrip * numStrips);
        ::memcpy(pixelBuffer, target.GetPixelBuffer(), sizeof(uint32_t) * size);
    }
};

// Loops through a set of colors doing smooth cross-fading to that color where each
// transition takes the given number of seconds.
class CrossFadeToAnimation : public BaseCrossFadeAnimation
{
private:
    PixelBuffer target;
    Vector<Color> colors;
    int currentColor = 0;
    bool hasColors = false;
    bool started = false;

public:
    CrossFadeToAnimation(PixelBuffer &buffer, const Vector<Color> &fade_colors, float seconds)
        : BaseCrossFadeAnimation(buffer, target, seconds), target(buffer.NumStrips(), buffer.NumLedsPerStrip()), colors(fade_colors)
    {
        hasColors = (colors.size() > 0);
        target.Initialize();
    }

    CrossFadeToAnimation(PixelBuffer &buffer, uint32_t *source, uint32_t size, float seconds)
        : BaseCrossFadeAnimation(buffer, target, seconds), target(buffer.NumStrips(), buffer.NumLedsPerStrip())
    {
        target.Initialize();
        this->target.CopyFrom(source, size);
    }

    ~CrossFadeToAnimation()
    {
    }

    SimpleString GetName() override
    {
        return "CrossFadeToAnimation";
    }

    // run one timeslice of the animation and return true when the animation has finished.
    bool Run() override
    {
        if (!started)
        {
            if (hasColors && currentColor < (int)colors.size())
            {
                target.SetColor(colors[currentColor]);
            }
            started = true;
            Start();
        }

        bool rc = BaseCrossFadeAnimation::Run();
        if (rc && hasColors)
        {
            currentColor++;
            if (currentColor < (int)colors.size())
            {
                rc = false;
                started = false;
            }
        }

        Draw();

        return rc;
    }
};

class RainbowAnimation : public Animation
{
private:
    int length = 0;
    uint32_t start_offset = 0;
    float duration = 0;
    bool forever;

public:
    RainbowAnimation(PixelBuffer &buffer, int length, float seconds) : Animation(AnimationType::Rainbow, buffer)
    {
        timer.start();
        this->length = length;
        start_offset = 0;
        duration = seconds;
        forever = (seconds == 0);
    }

    SimpleString GetName() override
    {
        return "RainbowAnimation";
    }

    // run one timeslice of the animation and return true when the animation has finished.
    bool Run() override
    {
        float thirds = (float)length / 3.0f;
        float factor1, factor2;

        if (forever || timer.seconds() < duration)
        {
            int ledsPerStrip = buffer.NumLedsPerStrip();
            uint32_t offset = start_offset++;
            for (int i = 0; i < ledsPerStrip; i++)
            {
                uint32_t index = offset + i;
                uint32_t position = index % length;
                switch ((uint32_t)(position / thirds))
                {
                case 0:
                    factor1 = 1.0f - (float)position / thirds;
                    factor2 = (float)position / thirds;
                    buffer.SetRow(Color{(uint8_t)(255 * factor1), (uint8_t)(255 * factor2), 0}, i);
                    break;
                case 1:
                    factor1 = 1.0f - ((float)(position - thirds) / thirds);
                    factor2 = (float)((int)(index - thirds) % length) / thirds;
                    buffer.SetRow(Color{0, (uint8_t)(255 * factor1), (uint8_t)(255 * factor2)}, i);
                    break;
                case 2:
                    factor1 = 1.0f - ((float)(position - 2 * thirds) / thirds);
                    factor2 = (float)((int)(index - 2 * thirds) % length) / thirds;
                    buffer.SetRow(Color{(uint8_t)(255 * factor2), 0, (uint8_t)(255 * factor1)}, i);
                    break;
                default:
                    break;
                }
                offset++;
            }

            Draw();
            return false;
        }
        else
        {
            fps = (float)frames / (float)timer.seconds();
            return true;
        }
    }
};

class GradientAnimation : public BaseCrossFadeAnimation
{
private:
    PixelBuffer target;
    Vector<Vector<Color>> strip_colors;
    Vector<Color> colors; // if we are animating all strips.
public:
    GradientAnimation(PixelBuffer &buffer, bool hold = true)
        : BaseCrossFadeAnimation(buffer, target, 0), target(buffer.NumStrips(), buffer.NumLedsPerStrip())
    {
        type = AnimationType::Gradient;
        this->hold = hold;
    }

    void AddStrip(int strip, const Vector<Color> &gradient, float seconds)
    {
        this->seconds = seconds;
        target.Initialize();

        if (gradient.size() == 0)
        {
            return;
        }

        if (strip == -1)
        {
            // animating all strips with the same colors.
            colors = gradient;
        }
        else
        {
            // animating individual strips with the given colors.
            if (strip >= 0 && strip < buffer.NumStrips())
            {
                strip_colors.reserve(buffer.NumStrips());
                strip_colors[strip] = gradient;
            }
        }

        int s = (int)(gradient.size() - 1);

        int ledsPerStrip = buffer.NumLedsPerStrip();
        int numStrips = buffer.NumStrips();
        int num_strip_colors = (int)strip_colors.reserved();

        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < ledsPerStrip; j++)
            {
                int segment = 0;
                float segment_length = (float)ledsPerStrip;
                int num_segments = 1;
                bool has_color = false;
                Color start;
                Color end;
                bool interpolate = false;
                Vector<Color> &colors = this->colors;
                if (i < num_strip_colors && strip_colors[i].size() > 0)
                {
                    // we have a specific color setting for this strip.
                    colors = strip_colors[i];
                }

                // prepare the target buffer with the gradient we want to fade to.
                num_segments = (int)colors.size() - 1;
                if (num_segments > 1)
                {
                    segment_length /= (float)num_segments;
                }

                if (num_segments > 1)
                {
                    segment = (int)(j / segment_length);
                    if (segment > num_segments)
                    {
                        segment = num_segments;
                    }
                }
                int num_colors = (int)colors.size();
                if (num_colors > segment)
                {
                    start = end = colors[segment];
                    if (num_colors > segment + 1)
                    {
                        end = colors[segment + 1];
                        interpolate = true;
                    }
                    has_color = true;
                }

                if (has_color)
                {
                    if (interpolate)
                    {
                        int offset = j - (int)(segment_length * segment);
                        float dr = (float)end.r - (float)start.r;
                        float dg = (float)end.g - (float)start.g;
                        float db = (float)end.b - (float)start.b;
                        float percent = (float)offset / segment_length;
                        if (percent > 1)
                            percent = 1;
                        Color ic{(uint8_t)(start.r + (percent * dr)),
                                 (uint8_t)(start.g + (percent * dg)),
                                 (uint8_t)(start.b + (percent * db))};
                        target.SetPixel(ic, i, j);
                    }
                    else
                    {
                        target.SetPixel(start, i, j);
                    }
                }
            }
        }

        Start();
    }

    SimpleString GetName() override
    {
        return "GradientAnimation";
    }

    // run one timeslice of the animation and return true when the animation has finished.
    bool Run() override
    {
        bool rc = BaseCrossFadeAnimation::Run();
        Draw();
        return rc;
    }
};

// This is an additive animation in that it scrolls a new gradient into the current buffer,
// in a given direction, so there is no need to cross fade into the target buffer.
class MovingGradientAnimation : public CopySourceAnimation
{
private:
    Vector<Color> colors;
    float speed; // number of pixels to move the animation on each frame of the animation, 1 is the smoothest.
    int direction;
    long ticks = 0;
    int size; // size of the gradient that we are scrolling through the buffer.
public:
    MovingGradientAnimation(PixelBuffer &buffer, const Vector<Color> &gradient, float speed, float direction, int size)
        : CopySourceAnimation(buffer), colors(gradient), speed(speed), direction((int)direction), size(size)
    {
        int ledsPerStrip = buffer.NumLedsPerStrip();
        if (size > ledsPerStrip)
        {
            size = ledsPerStrip;
        }
    }

    // run one timeslice of the animation and return true when the animation has finished.
    bool Run() override
    {
        ticks++;
        int offset = (int)(ticks * speed);
        int step = 1;
        bool finished = false;
        int ledsPerStrip = buffer.NumLedsPerStrip();

        // undo any overlay effect.
        this->CopySnapshot();

        if (direction < 0)
        {
            // start at the bottom and scroll in from there.
            offset = ledsPerStrip + size - offset;
            step = -1;
            if (offset < -size)
            {
                finished = true;
            }
        }
        else
        {
            // off the stop and smoothly scroll into view then off the bottom.
            step = 1;
            offset -= size;
            if (offset > ledsPerStrip + size)
            {
                finished = true;
            }
        }

        int numStrips = buffer.NumStrips();

        int num_segments = (int)colors.size() - 1;
        float segment_length = (float)size / (float)num_segments;

        for (int i = 0; i < numStrips; i++)
        {
            int end = offset + (size * step);
            int k = 0;
            for (int j = offset; (direction > 0 && j < end) || (direction < 0 && j > end); j += step)
            {
                if (j >= 0 && j < ledsPerStrip)
                {
                    int segment = (int)(k / segment_length);
                    if (segment > num_segments)
                    {
                        segment = num_segments;
                    }
                    int p = k - (int)(segment_length * segment);
                    Color start = colors[num_segments - segment];
                    Color end = start;
                    int q = num_segments - (segment + 1);
                    if (q >= 0 && q <= num_segments)
                    {
                        end = colors[q];
                    }
                    float dr = (float)end.r - (float)start.r;
                    float dg = (float)end.g - (float)start.g;
                    float db = (float)end.b - (float)start.b;
                    float percent = (float)p / segment_length;
                    if (percent > 1)
                        percent = 1;
                    Color ic{(uint8_t)(start.r + (percent * dr)),
                             (uint8_t)(start.g + (percent * dg)),
                             (uint8_t)(start.b + (percent * db))};
                    buffer.SetPixel(ic, i, j);
                }
                k++;
            }
        }

        // save snapshot before any overlay is applied.
        this->UpdateSnapshot();

        Draw();
        return finished;
    }

    SimpleString GetName() override
    {
        return "MovingGradientAnimation";
    }
};

class NeuralDropAnimation : public Animation
{
private:
    int drops;
    int index;
    bool forever;
    const Color basecolor{0x1b, 0x23, 0x4b};
    const float breathing_time = 3e3; // 3 seconds
    const int bubble_size = 16;
    Vector<Color> bubble_colors;

public:
    NeuralDropAnimation(PixelBuffer &buffer, int drops) : Animation(AnimationType::NeuralDrop, buffer)
    {
        float start = 0;
        for (int i = 0; i < bubble_size; i++)
        {
            int temperature = (int)(128 * pow(sin(start), (float)2.0) + 8);
            uint8_t nc = (uint8_t)(temperature);
            bubble_colors.push_back(Color{nc, 0, nc});
            start += (float)(pi / 15);
        }

        timer.start();
        this->drops = drops;
        forever = (drops == 0);
        index = 0;
    }

    SimpleString GetName() override
    {
        return "NeuralDropAnimation";
    }

    // run one timeslice of the animation and return
    bool Run() override
    {
        int ledsPerStrip = buffer.NumLedsPerStrip();
        if (forever || drops > 0)
        {
            // Generate a 'breathing' animation with a pseudo triangular wave with different decay:
            float millis = timer.milliseconds();
            // this produces a nice sin curve with a bit of an extended quiet time between each peak.
            float amplitude = (float)exp(sin(millis / breathing_time * 3.1415926)) / 6.0f + 0.25f;
            uint8_t r = (uint8_t)((float)basecolor.r * amplitude);
            uint8_t g = (uint8_t)((float)basecolor.g * amplitude);
            uint8_t b = (uint8_t)((float)basecolor.b * amplitude);

            // Paint the entire canvas the 'background' breathing color:
            buffer.SetColor(Color{r, g, b});

            // Paint the animating section falling down the strip:
            for (int j = index; j < (index + bubble_size); j++)
            {
                if (j < ledsPerStrip)
                {
                    Color &c = bubble_colors[j - index];
                    buffer.SetRow(c, j);
                }
            }

            Draw();

            index++;
            if (index == ledsPerStrip)
            {
                index = 0;
                drops--;
            }
            return false;
        }
        return true;
    }

    void Stop() override
    {
        buffer.SetColor(basecolor);
    }
};

class TwinkleAnimation : public Animation
{
private:
    Timer timer;
    Color baseColor;
    Color twinkle;
    float speed;
    int density;

    class Star
    {
    public:
        int index;
        int start;
        int count;
    };

    Star *stars = nullptr;

public:
    TwinkleAnimation(PixelBuffer &buffer, Color baseColor, Color twinkle, float speed, int density) : Animation(AnimationType::Twinkle, buffer)
    {
        timer.start();
        this->twinkle = twinkle;
        this->baseColor = baseColor;
        this->speed = speed;
        this->density = density;

        int numStrips = buffer.NumStrips();
        this->stars = new Star[numStrips * density];
        if (stars == nullptr)
        {
            CrashPrint("out of memory\r\n");
        }
        GetStars(speed, density, stars);
    }

    ~TwinkleAnimation()
    {
        if (stars != nullptr)
        {
            delete[] stars;
        }
    }

    SimpleString GetName() override
    {
        return "TwinkleAnimation";
    }

    bool Run() override
    {
        // twinkle animates random pixels between baseColor and twinkle color
        float dr = (float)twinkle.r - (float)baseColor.r;
        float dg = (float)twinkle.g - (float)baseColor.g;
        float db = (float)twinkle.b - (float)baseColor.b;

        if (density < 1)
        {
            density = 1;
        }

        if (stars == nullptr)
        {
            return false;
        }

        int numStrips = buffer.NumStrips();

        buffer.SetColor(baseColor);

        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < density; j++)
            {
                int index = (i * density) + j;
                Star &star = stars[index];
                float brightness = 0;
                if (speed == 0 || star.count < star.start)
                {
                    buffer.SetPixel(baseColor, i, star.index);
                }
                else
                {
                    int position = star.count - star.start;
                    brightness = (float)sin((float)position * pi / (float)speed);
                    // interpolate the twinkle effect
                    Color ic{(uint8_t)(baseColor.r + (brightness * dr)),
                             (uint8_t)(baseColor.g + (brightness * dg)),
                             (uint8_t)(baseColor.b + (brightness * db))};

                    buffer.SetPixel(ic, i, star.index);
                }

                star.count++;
                if (star.count - star.start >= speed)
                {
                    // this star is done, replace it with a new one.
                    star = GetRandomStar(speed);
                }
            }
        }

        Draw();
        return false; // runs forever
    }
    void Stop() override
    {
        buffer.SetColor(baseColor);
    }

private:
    // get a set of random leds for each strip
    void GetStars(float speed, int density, Star *array)
    {
        int numStrips = buffer.NumStrips();
        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < density; j++)
            {
                int index = (i * density) + j;
                array[index] = GetRandomStar(speed);
            }
        }
    }

    Star GetRandomStar(float speed)
    {
        int ledsPerStrip = buffer.NumLedsPerStrip();
        int index = (int)(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * ledsPerStrip);
        int start = 0;
        if (speed > 0)
        {
            start = (int)(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * speed);
        }
        return Star{index, start, 0};
    }
};

class FireAnimation : public Animation
{
private:
    // Array of temperature readings at each simulation cell
    float *heat = nullptr;

    // There are two main parameters you can play with to control the look and
    // feel of your fire: COOLING (used in step 1 above), and SPARKING (used
    // in step 3 above).
    //
    // How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 50, suggested range 20-100
    int cooling = 55;

    // What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    int sparkling = 120;

    float duration = 0;
    bool forever;
    bool reverse;

public:
    FireAnimation(PixelBuffer &buffer, int cooling, int sparkling, float seconds, bool reverse = true) : Animation(AnimationType::Fire, buffer)
    {
        timer.start();
        this->reverse = reverse;
        this->cooling = (int)(fmin(cooling, 100));
        this->sparkling = (int)(fmin(sparkling, 255));
        heat = new float[buffer.GetNumberOfPixels()];
        duration = seconds;
        forever = (seconds == 0);
    }

    ~FireAnimation()
    {
        if (heat != nullptr)
        {
            delete[] heat;
        }
    }

    SimpleString GetName() override
    {
        return "FireAnimation";
    }

    float random(int min, int max)
    {
        int range = max - min;
        float r = static_cast<float>(rand());
        r = min + ((r / static_cast<float>(RAND_MAX)) * range);
        return r;
    }

    bool Run() override
    {
        if (!forever && timer.seconds() > duration)
        {
            fps = (float)frames / (float)timer.seconds();
            return true;
        }

        int num_leds = buffer.NumLedsPerStrip();
        int numStrips = buffer.NumStrips();
        for (int x = 0; x < numStrips; x++)
        {
            int base = (x * num_leds);
            // Step 1.  Cool down every cell a little
            for (int i = 0; i < num_leds; i++)
            {
                int index = base + i;
                auto cool = random(0, ((cooling * 10) / num_leds) + 2);
                auto v = heat[index] - cool;
                if (v < 0)
                    v = 0;
                heat[index] = v;
            }

            // Step 2.  Heat from each cell drifts 'up' and diffuses a little
            for (int i = num_leds - 1; i >= 2; i--)
            {
                int index = base + i;
                auto convection = (heat[index - 1] + heat[index - 2] + heat[index - 2]) / 3;
                if (convection > 255)
                    convection = 255;
                heat[index] = convection;
            }

            // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
            if (random(0, 255) < sparkling)
            {
                int i = (int)random(0, 7);
                int index = base + i;
                auto v = heat[index] + random(160, 255);
                if (v > 255)
                    v = 255;
                heat[index] = v;
            }

            // Step 4.  Map from heat cells to LED colors
            for (int i = 0; i < num_leds; i++)
            {
                int index = base + i;
                Color color = HeatColor(heat[index]);
                buffer.SetPixel(color, x, reverse ? num_leds - i - 1 : i);
            }
        }
        Draw();
        return false; // runs forever
    }

    Color HeatColor(float temperature)
    {
        Color heatcolor;

        // Scale 'heat' down from 0-255 to 0-191,
        // which can then be easily divided into three
        // equal 'thirds' of 64 units each.
        uint8_t t192 = (uint8_t)(temperature * 191 / 255);

        // calculate a value that ramps up from
        // zero to 255 in each 'third' of the scale.
        uint8_t heatramp = t192 & 0x3F; // 0..63
        heatramp <<= 2;                 // scale up to 0..252

        // now figure out which third of the spectrum we're in:
        if (t192 & 0x80)
        {
            // we're in the hottest third
            heatcolor.r = 255;      // full red
            heatcolor.g = 255;      // full green
            heatcolor.b = heatramp; // ramp up blue
        }
        else if (t192 & 0x40)
        {
            // we're in the middle third
            heatcolor.r = 255;      // full red
            heatcolor.g = heatramp; // ramp up green
            heatcolor.b = 0;        // no blue
        }
        else
        {
            // we're in the coolest third
            heatcolor.r = heatramp; // ramp up red
            heatcolor.g = 0;        // no green
            heatcolor.b = 0;        // no blue
        }

        return heatcolor;
    }
};

#endif
