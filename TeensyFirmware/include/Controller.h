// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include "PixelBuffer.h"
#include "Commands.h"
#include "Animations.h"

class Controller;

void StartThread(Controller* ptr);

class Controller
{
    PixelBuffer buffer;
    Command currentCommand;
    Animation* animation = nullptr;

public:
    Controller(int numStrips, int ledsPerStrip)
        : buffer(numStrips, ledsPerStrip)
    {
    }

    void Initialize()
    {
        buffer.Initialize();
    }

    PixelBuffer& GetBuffer() { return buffer; }

    Command& GetCommand()
    {
        return currentCommand;
    }

    bool HasAnimation()
    {
        return animation != nullptr;
    }

    // returns true when command is complete.
    bool RunAnimation()
    {
        if (animation != nullptr)
        {
            if (animation->Run())
            {
				// keep the overlay running.
				Animation* overlay = nullptr;
				if (animation != nullptr)
				{
					overlay = animation->RemoveOverlay();
				}
                // it is complete
				animation->Stop();
                delete animation;
                animation = nullptr;
				if (overlay != nullptr)
				{
					animation = new CopySourceAnimation(buffer);
					if (animation == nullptr)
					{
						CrashPrint("### CopySourceAnimation: out of memory\r\n");
						delete overlay;
					}
					else
					{
						animation->AddOverlay(overlay);
					}
				}
                return true;
            }
        }
        return false;
    }

    void Close()
    {
        StopCommand();
    }

	void SetColor(Color color, int strip = -1, int index = -1)
	{
		StopCommand();
		InternalSetColor(color, strip, index);
	}

    void StartCrossFade(Color color, float seconds)
    {
        currentCommand.type = CommandType::CrossFade;
        currentCommand.command = "CrossFade";
        currentCommand.clearColors();
        currentCommand.addColor(color);
        currentCommand.seconds = seconds;
        StartCommand();
    }

    void StartTwinkle(Color c1, Color c2, float seconds, int density)
    {
        currentCommand.type = CommandType::Twinkle;
        currentCommand.command = "Twinkle";
        currentCommand.clearColors();
        currentCommand.addColor(c1);
        currentCommand.addColor(c2);
        currentCommand.seconds = seconds;
        currentCommand.size = density;
        StartCommand();
    }

    void StartRainbow(int length, float seconds)
    {
        currentCommand.type = CommandType::Rainbow;
        currentCommand.command = "Rainbow";
        currentCommand.size = length;
        currentCommand.seconds = seconds;
        StartCommand();
    }

    void StartFire(int cooling = 55, int sparkle = 120, float seconds = 0)
    {
        currentCommand.type = CommandType::Fire;
        currentCommand.command = "Fire";
        currentCommand.f1 = (float)cooling;
        currentCommand.f2 = (float)sparkle;
        currentCommand.seconds = seconds;
        StartCommand();
    }

    void StartNeuralDrop(int drops)
    {
        currentCommand.type = CommandType::NeuralDrop;
        currentCommand.command = "NeuralDrop";
        currentCommand.iterations = drops;
        StartCommand();
    }

    void StartCommand(Command& cmd)
    {
        this->currentCommand = cmd;
        StartCommand();
    }

    void StartCommand()
    {
        // process overlay commands before stopping anything
        switch (currentCommand.type)
        {
            case CommandType::StartRain:
            {
                if (animation == nullptr) {
                    // then create a copy source animation that runs forever that keeps refreshing the background
                    // so that the animation can run on top of that.
                    animation = new CopySourceAnimation(buffer);
                    if (animation == nullptr)
                    {
                        CrashPrint("### StartRain: out of memory\r\n");
                    }
                }
                if (animation != nullptr)
                {
                    auto overlay = new RainOverlayAnimation(buffer, currentCommand.size, currentCommand.f1);
                    if (overlay == nullptr)
                    {
                        CrashPrint("### StartRain: out of memory2\r\n");
                    }
                    else
                    {
                        animation->AddOverlay(overlay);
                    }
                }
                return;
            }
            case CommandType::StopRain:
            {
                if (animation != nullptr) {
                    Animation* overlay = animation->RemoveOverlay();
                    if (overlay != nullptr)
                    {
                        delete overlay;
                    }
                }
                return;
            }
            case CommandType::Status:
            {
                QueryStatus();
                return;
            }
            default:
                break;
        }

        Animation* overlay = nullptr;
        if (animation != nullptr)
        {
            overlay = animation->RemoveOverlay();
        }

        // gradient command is additive.
        if (currentCommand.type == CommandType::Gradient) {
            GradientAnimation* grad = nullptr;
            if (animation != nullptr && animation->GetType() == AnimationType::Gradient) {
                grad = static_cast<GradientAnimation*>(animation);
            }
            if (grad == nullptr)
            {
                StopCommand();
                grad = new GradientAnimation(buffer);
                if (grad == nullptr)
                {
                    CrashPrint("### Gradient: out of memory\r\n");
                }
            }

            if (grad != nullptr) {
                if (currentCommand.colorsPerStrip > 0) {
                    // Ok, we need to unpack the colors into each separate strip.
                    // Hopefully we have enough colors, otherwise we just abort early.
                    // We are expecting colors.size() == numStrips * colorsPerStrip.
                    int numStrips = buffer.NumStrips();
                    int offset = 0;
                    int num = (int)currentCommand.colors.size();
                    Vector<Color> split;
                    for (int i = 0; i < numStrips && offset < num; i++) {
                        split.clear();
                        for (int j = 0; j < currentCommand.colorsPerStrip && offset < num; j++) {
                            split.push_back(currentCommand.colors[offset]);
                            offset++;
                        }
                        grad->AddStrip(i, split, currentCommand.seconds);
                    }
                }
                else {
                    grad->AddStrip(currentCommand.strip, currentCommand.colors, currentCommand.seconds);
                }
                animation = grad;
            }
        }
        else {
            StopCommand();
        }

        switch(currentCommand.type)
        {
            case CommandType::None:
                break;
            case CommandType::SetColor:
                RunSetColor();
                break;

            case CommandType::FullBuffer:
                if (currentCommand.pixelBuffer != nullptr)
                {
                    uint32_t size = currentCommand.numStrips * currentCommand.ledsPerStrip * sizeof(uint32_t);
                    if (size == 0)
                    {
                        CrashPrint("### FullBuffer is empty?\r\n");
                    }
                    else
                    {
                        animation = new CrossFadeToAnimation(buffer, currentCommand.pixelBuffer, size, currentCommand.seconds);
                        if (animation == nullptr)
                        {
                            CrashPrint("### FullBuffer: out of memory");
                        }
                    }

                    //Color c = buffer.GetPixel(0, 50);
                    //DebugPrint("led 50 is color %d,%d,%d\r\n", (int)c.r, (int)c.g, (int)c.b);
                }
                break;

            case CommandType::Breathe:
                animation = new BreatheAnimation(buffer, currentCommand.seconds, currentCommand.f1, currentCommand.f2);
                if (animation == nullptr)
                {
                    CrashPrint("### Breathe: out of memory\r\n");
                }
                break;
            case CommandType::Gradient:
                // handled above since gradient is additive.
                break;
            case CommandType::MovingGradient:
                animation = new MovingGradientAnimation(buffer, currentCommand.colors, currentCommand.seconds, currentCommand.f1, currentCommand.size);
                if (animation == nullptr)
                {
                    CrashPrint("### Gradient: out of memory\r\n");
                }
                break;

            case CommandType::NeuralDrop:
                animation = new NeuralDropAnimation(buffer, currentCommand.iterations);
                if (animation == nullptr)
                {
                    CrashPrint("### NeuralDrop: out of memory\r\n");
                }
                break;
            case CommandType::CrossFade:
                animation = new CrossFadeToAnimation(buffer, currentCommand.colors, currentCommand.seconds);
                if (animation == nullptr)
                {
                    CrashPrint("### CrossFade: out of memory\r\n");
                }
                break;
            case CommandType::WaterDrop:
                animation = new WaterDropAnimation(buffer, currentCommand.iterations, currentCommand.size, currentCommand.f1);
                if (animation == nullptr)
                {
                    CrashPrint("### WaterDrop: out of memory\r\n");
                }
                break;
            case CommandType::Rainbow:
                animation = new RainbowAnimation(buffer, currentCommand.size, currentCommand.seconds);
                if (animation == nullptr)
                {
                    CrashPrint("### Rainbow: out of memory\r\n");
                }
                break;
            case CommandType::Fire:
                animation = new FireAnimation(buffer, (int)currentCommand.f1, (int)currentCommand.f2, currentCommand.seconds);
                if (animation == nullptr)
                {
                    CrashPrint("Fire: out of memory\r\n");
                }
                break;
            case CommandType::Twinkle:
                if (currentCommand.colors.size() > 1)
                {
                    animation = new TwinkleAnimation(buffer, currentCommand.colors[0], currentCommand.colors[1], currentCommand.seconds, currentCommand.size);
                    if (animation == nullptr)
                    {
                        CrashPrint("### Twinkle: out of memory\r\n");
                    }
                }
                else
                {
                    currentCommand.type = CommandType::None;
                }
                break;
            case CommandType::SpeedTest:
                RunSpeedTest();
                break;
            default:
                break;
        }

        // maintain the existing overlay even when underlying animation switches out.
        if (overlay != nullptr)
        {
            if (animation == nullptr)
            {
				animation = new CopySourceAnimation(buffer);
				if (animation == nullptr)
				{
					CrashPrint("### StartRain: out of memory\r\n");
				}
            }
			if (animation != nullptr)
            {
				animation->AddOverlay(overlay);
            }
			else
			{
				delete overlay;
			}
        }
    }

private:

    void QueryStatus()
    {
        buffer.PrintStatus();
        if (currentCommand.error.size() > 0)
        {
            DebugPrint("%s\r\n", currentCommand.error.c_str());
        }
        else
        {
            DebugPrint("no current command\r\n");
        }

        if (animation != nullptr)
        {
            DebugPrint("Animation: %s\r\n", animation->GetName().c_str());
            Animation* overlay = animation->GetOverlay();
            if (overlay != nullptr)
            {
                DebugPrint("Overlay animation: %s\r\n", overlay->GetName().c_str());
            }
        }
    }

    void StopCommand()
    {
        if (animation != nullptr)
        {
			animation->Stop();
            delete animation;
            animation = nullptr;
        }
    }

    void InternalSetColor(Color color, int strip, int index)
    {
        if (strip >= 0)
        {
            if (index >= 0)
            {
                buffer.SetPixel(color, strip, index);
            }
            else
            {
                buffer.SetColumn(color, strip);
            }
        }
        else {
            buffer.SetColor(color);
        }
        buffer.Write();
    }

    bool RunSetColor()
    {
        if (currentCommand.colors.size() > 0)
        {
            auto c = currentCommand.colors[0];
            InternalSetColor(c, currentCommand.strip, currentCommand.index);
        }
        return true;
    }

    const int testbuffersize = 1000;
    char testbuffer[1000];

    void RunSpeedTest()
    {
        int pos = 0;
        uint8_t x = 0;
        Timer timer;
        timer.start();
        int errors = 0;
        int total = 0;
        bool done = false;

        while (timer.seconds() < 10 && !done) {
            if (Serial.available())
            {
                uint32_t bytesRead = Serial.readBytes((char*)testbuffer, testbuffersize);
                for (size_t i = 0; i < bytesRead; i++)
                {
                    uint8_t y = testbuffer[i];
                    if (y == 255)
                    {
                        done = true;
                        break;
                    }
                    if (y != x)
                    {
                        errors++;
                        x = y;
                        pos = 0; // try and recover
                    }
                    pos++;
                    if (pos % 1000 == 0)
                    {
                        x++;
                        if (x == 255){
                            x = 0;
                        }
                    }
                }
                total += bytesRead;
            }
        }

        DebugPrint("received %d bytes, and found %d errors\r\n", total, errors);
    }
};

#endif
