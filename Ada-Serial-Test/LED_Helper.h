// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#include <sys/types.h>
#include <stdint.h>
#include "Adafruit_NeoPixel.h"

/**********************************/
/*  pthread Functions             */
/**********************************/

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE thread_handle_t;
#else // assume pthreads
#include <pthread.h>
typedef pthread_t thread_handle_t;
#endif

#ifdef _WIN32
typedef struct pthread_attr_t pthread_attr_t;
struct pthread_attr_t
{
	unsigned p_state;
	void* stack;
	size_t s_size;
};
typedef HANDLE pthread_t;
int pthread_create(pthread_t* thread, pthread_attr_t* attr, LPTHREAD_START_ROUTINE start_routine, void* arg)
{
	if (thread == NULL || start_routine == NULL)
		return 1;

	*thread = CreateThread(NULL, 0, start_routine, arg, 0, NULL);
	if (*thread == NULL)
		return 1;
	return 0;
}
#endif

/**********************************/
/*  Timer Functions               */
/**********************************/

#if defined(WINDOWS)
#include <sys/time.h>
#include <unistd.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}

// https://stackoverflow.com/questions/17250932/how-to-get-the-time-elapsed-in-c-in-milliseconds-windows
long long milliseconds_now() {
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	}
	else {
		return GetTickCount64();
	}
}
static uint64_t millis() { return milliseconds_now(); };
#endif
#if defined(LINUX) || defined(MACOSX)
unsigned long millis(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}
#endif


/**********************************/
/*  RGB LED Functions             */
/**********************************/

class Strip
{
public:
	uint8_t   effect;
	uint8_t   effects;
	uint16_t  effStep;
	uint64_t effStart;
	Adafruit_NeoPixel strip;
	Strip(uint16_t leds, uint8_t pin, uint8_t toteffects, uint16_t striptype) : strip(leds, pin, striptype) {
		effect = -1;
		effects = toteffects;
		Reset();
	}
	void Reset() {
		effStep = 0;
		effect = (effect + 1) % effects;
		effStart = millis();
	}
};

//Adafruit_NeoPixel strip(LED_COUNT, 0, NEO_WRGB + NEO_KHZ800);
const char* TAG_FULLBUFFER_STRING = "##FULLBUFFER##";

const uint32_t LEDSPerStrip = 392;
const uint32_t NUM_LEDStrips = 16;
#define pixBufferAccess( curLED,  curCol)  (*((uint32_t*)(pixBuffer + curLED * NUM_LEDStrips + curCol)))
Strip strip_0(LEDSPerStrip, 0, 1, NEO_WRGB + NEO_KHZ800);

// https://adrianotiger.github.io/Neopixel-Effect-Generator/
uint8_t strip0_loop0_eff0() {
	// Strip ID: 0 - Effect: Rainbow - LEDS: 300
	// Steps: 157 - Delay: 1
	// Colors: 3 (255.0.0, 0.255.0, 0.0.255)
	// Options: rainbowlen=157, toLeft=true,
	if (millis() - strip_0.effStart < 1 * (strip_0.effStep)) return 0x00;
	float factor1, factor2;
	uint16_t ind;
	for (uint16_t j = 0; j < LEDSPerStrip; j++) {
		ind = strip_0.effStep + j * 1;
		switch ((int)((ind % 157) / 52.333333333333336)) {
		case 0: factor1 = 1.0 - ((float)(ind % 157 - 0 * 52.333333333333336) / 52.333333333333336);
			factor2 = (float)((int)(ind - 0) % 157) / 52.333333333333336;
			strip_0.strip.setPixelColor(j, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2);
			break;
		case 1: factor1 = 1.0 - ((float)(ind % 157 - 1 * 52.333333333333336) / 52.333333333333336);
			factor2 = (float)((int)(ind - 52.333333333333336) % 157) / 52.333333333333336;
			strip_0.strip.setPixelColor(j, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2);
			break;
		case 2: factor1 = 1.0 - ((float)(ind % 157 - 2 * 52.333333333333336) / 52.333333333333336);
			factor2 = (float)((int)(ind - 104.66666666666667) % 157) / 52.333333333333336;
			strip_0.strip.setPixelColor(j, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2);
			break;
		}
	}
	if (strip_0.effStep >= 157) { strip_0.Reset(); return 0x03; }
	else strip_0.effStep++;
	return 0x01;
}
