#pragma once
#include <stdint.h>

class TestWindow;

class MultiWS2812
{
	int ledsPerStrip;
	int numStrips;
	uint32_t* buffer = nullptr;
	TestWindow* window = nullptr;
public:
    MultiWS2812(int ledsPerStrip, int numStrips)
    {
		this->ledsPerStrip = ledsPerStrip;
		this->numStrips = numStrips;
    }

	void setWindow(TestWindow* window)
	{
		this->window = window;
	}

    void enableOutputPin(int port, int pin)
    {
    }
	
	void show();

    void setBuffer(uint32_t* buffer)
    {
		this->buffer = buffer;
    }
    void printTimingCycleStats()
    {

    }
};