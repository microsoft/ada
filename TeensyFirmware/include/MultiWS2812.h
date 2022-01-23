/*--------------------------------------------------------------------
  This file is part of the Adafruit NeoPixel library.

  NeoPixel is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of
  the License, or (at your option) any later version.

  NeoPixel is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with NeoPixel.  If not, see
  <http://www.gnu.org/licenses/>.
  --------------------------------------------------------------------*/
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef _MultiWS2812_H
#define _MultiWS2812_H

#define MULTIWS2812_MAX_GPIOS (16)
#define MULTIWS2812_FAST_DBG_GPIO (1)

#ifndef UNITTEST
#if (ARDUINO_TEENSY40)
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif
#if defined(__AVR__) || defined(KINETISK) || defined(__MKL26Z64__)
#error "Sorry, MultiWS2812 only works on 32 bit Teensy boards, Teensy 4.0 specifically.  AVR isn't supported."
#endif
#endif

#include "Timer.h"

#ifdef MULTIWS2812_FAST_DBG_GPIO
static uint32_t* dbg_pin1_set, * dbg_pin1_clr, dbg_pin1_msk;
// 4x faster than digitalWrite(1,HIGH) ~5cycles
static inline void dbg_write(uint8_t val) {
    if (val == HIGH)
        * dbg_pin1_set = dbg_pin1_msk;
    else if (val == LOW)
        * dbg_pin1_clr = dbg_pin1_msk;
}
#endif


// Define the limits and defaults for WS2812 signal generation
const struct {
    // Nominal values (F_CPU_ACTUAL typically is 600,000,000, 600MHz, a cycle is 1.666667ns)
    // Convert cycles to ns: cycles * 1/600e6 = ns (use 1/0.6, to go directly to ns)
    // Convert cycles to Hz: 1/cycles * 600e6 = Hz (600e3, for kHz)
    const uint32_t DEFAULT_CYCLES_800 = 600e6 / 800000; // 750 cycles [1.25us, 800kHz]
    //const uint32_t DEFAULT_CYCLES_800 = 726; // 750 cycles [1.25us, 800kHz]
    const uint32_t DEFAULT_CYCLES_T0H = 600e6 / 4000000; // 150 cycles [250ns]
    //const uint32_t DEFAULT_CYCLES_T1H = 600e6 / 1250000; // 480 cycles [800ns]
    //const uint32_t DEFAULT_CYCLES_T1H = 390; // 390 cycles [650ns]
    const uint32_t DEFAULT_CYCLES_T1H = 522; // 522 cycles [870ns]
    const uint32_t CYCLES_MIN = 480; //  800ns [1250kHz] [ 800ns = 220ns +  580ns]
    const uint32_t CYCLES_MAX = 1212; // 2020ns [ 495kHz] [2020ns = 1600ns + 420ns]
    const uint32_t CYCLES_T0H_MIN = 132; //  220ns
    const uint32_t CYCLES_T0H_MAX = 228; //  380ns
    const uint32_t CYCLES_T1H_MIN = 348; //  480ns
    const uint32_t CYCLES_T1H_MAX = 960; // 1600ns

    // Wait a bit longer than 300us to avoid any timing issues
    const uint32_t DEFAULT_FRAME_RESET = 3310; // us of delay
    const uint32_t FRAME_RESET_MIN = 280; // us of delay
    const uint32_t FRAME_RESET_MAX = 500; // us of delay
} WS2812Timing;

extern void DebugPrint(const char* format, ...);

class MultiWS2812 {
public:
    MultiWS2812(uint16_t LEDsPerStripCount, uint16_t stripCount);
    ~MultiWS2812();

    void show(void);
    int busy(void);
    boolean enableOutputPin(uint32_t bankStringNum, uint32_t pin);

    // Update any parameter that isn't 0
    void setCycleDividersForced(uint32_t cyclesPeriod, uint32_t cyclesT0H, uint32_t cyclesT1H, uint32_t cyclesFrameDelay) {
        timingFrameResetDelay = cyclesFrameDelay != 0 ? cyclesFrameDelay : timingFrameResetDelay;
        timingCyclesPeriod800 = cyclesPeriod != 0 ? cyclesPeriod : timingCyclesPeriod800;
        timingCyclesT0H = cyclesT0H != 0 ? cyclesT0H : timingCyclesT0H;
        timingCyclesT1H = cyclesT1H != 0 ? cyclesT1H : timingCyclesT1H;
    }

    // Apply some logic to the timing constraints (see MultiWS2812::show() for timing diagram)
    void setCycleDividers(uint32_t cyclesPeriod, uint32_t cyclesT0H, uint32_t cyclesT1H, uint32_t cyclesFrameDelay) {
        timingFrameResetDelay = (cyclesFrameDelay > WS2812Timing.FRAME_RESET_MAX) ? WS2812Timing.FRAME_RESET_MAX : cyclesFrameDelay;
        timingFrameResetDelay = (cyclesFrameDelay < WS2812Timing.FRAME_RESET_MIN) ? WS2812Timing.FRAME_RESET_MIN : cyclesFrameDelay;
        timingCyclesPeriod800 = (cyclesPeriod > WS2812Timing.CYCLES_MAX) ? WS2812Timing.CYCLES_MAX : cyclesPeriod;
        timingCyclesPeriod800 = (cyclesPeriod < WS2812Timing.CYCLES_MIN) ? WS2812Timing.CYCLES_MIN : cyclesPeriod;
        timingCyclesT0H = (cyclesT0H > WS2812Timing.CYCLES_T0H_MAX) ? WS2812Timing.CYCLES_T0H_MAX : cyclesT0H;
        timingCyclesT0H = (cyclesT0H < WS2812Timing.CYCLES_T0H_MIN) ? WS2812Timing.CYCLES_T0H_MIN : cyclesT0H;
        timingCyclesT1H = (cyclesT1H > WS2812Timing.CYCLES_T1H_MAX) ? WS2812Timing.CYCLES_T1H_MAX : cyclesT1H;
        timingCyclesT1H = (cyclesT1H < WS2812Timing.CYCLES_T1H_MIN) ? WS2812Timing.CYCLES_T1H_MIN : cyclesT1H;
    }

    void printTimingCycleStats() {
        DebugPrint("CPU Clock time: %u\r\n", F_CPU_ACTUAL);
        DebugPrint("800kHz total cycle duration: %u cycles [%.2fkHz]\r\n", timingCyclesPeriod800, 1.0 / timingCyclesPeriod800 * ((double)F_CPU_ACTUAL / 1000.0));
        DebugPrint("T0H NZR high time duration: %u cycles [%.2fns]\r\n", timingCyclesT0H, timingCyclesT0H * 1.0 / ((double)F_CPU_ACTUAL / 1.0e9));
        DebugPrint("T1H NZR high time duration: %u cycles [%.2fns]\r\n", timingCyclesT1H, timingCyclesT1H * 1.0 / ((double)F_CPU_ACTUAL / 1.0e9));
        DebugPrint("frameBuffer: 0x%08X [32bit alignment: 0x%08X]\r\n", (int)frameBuffer, (int)frameBuffer % 32);
    }

    void begin(void) {
        begun = true;
    }
    void* getBuffer(void) {
        return frameBuffer;
    }
    void setBuffer(uint32_t* newBuf) {
        frameBuffer = newBuf;
    }
    int getNumLEDs() {
        return (maxLEDsPerStrip * numStrips);
    }
    int getNumLEDStrips() {
        return numStrips;
    }
    int getNumLEDsPerStrip() {
        return maxLEDsPerStrip;
    }

private:
    boolean begun;              // true if begin() previously called
    uint16_t maxLEDsPerStrip;   // Max length of strip, sets size of buffers
    uint16_t numStrips;         // Number of strips/output ports enabled
    uint16_t numBytes;         // Number of strips/output ports enabled
    uint32_t* frameBuffer;     // Pixel data arranged in [LED][strip] order

  // Cycle timing tuning:
    Timer timer;
    uint32_t timingCyclesPeriod800;
    uint32_t timingCyclesT1H;
    uint32_t timingCyclesT0H;
    uint32_t timingFrameResetDelay;

    // RES (frame latch) timing:
    volatile uint8_t update_in_progress;
    volatile uint32_t update_completed_at;

    // Port setting registers on GPIO6 ports
    uint32_t port6Count;
    volatile uint32_t* set6;
    volatile uint32_t* clr6;
    uint32_t msk6;      // Mask for setting all outputs on

    // Port setting registers on GPIO7 port
    uint32_t port7Count;
    volatile uint32_t* set7;
    volatile uint32_t* clr7;
    uint32_t msk7;      // Mask for setting all outputs on

    // GPIO usage tracking:
    uint32_t pinNums[MULTIWS2812_MAX_GPIOS];
    uint32_t curPin;
};

#endif // MultiWS2812_H
