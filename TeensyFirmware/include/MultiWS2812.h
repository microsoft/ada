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

    // Datasheet says 800khz is the max.
    const uint32_t DEFAULT_CYCLES_800 = 600e6 / 800000; // 750 cycles [1.25us, 800kHz]

    // Datasheet says T0H should be 0.4 us
    const uint32_t DEFAULT_CYCLES_T0H = 240; // 400 ns
    // Datasheet says T0H should be 0.8 us
    const uint32_t DEFAULT_CYCLES_T1H = 480; // 800 ns

    // Wait a bit longer than 300us to avoid any timing issues
    // Datasheet says frame reset has to be >= 50 us.
    const uint32_t DEFAULT_FRAME_RESET = 300; // us of delay
} WS2812Timing;

extern void DebugPrint(const char* format, ...);

class MultiWS2812 {
public:
    MultiWS2812(uint16_t LEDsPerStripCount, uint16_t stripCount);
    ~MultiWS2812();

    void show(void);
    int busy(void);
    boolean enableOutputPin(uint32_t bankStringNum, uint32_t pin);

    void printTimingCycleStats() {
        DebugPrint("CPU Clock time: %u\r\n", F_CPU_ACTUAL);
        DebugPrint("800kHz total cycle duration: %u cycles [%.2fkHz]\r\n", timingCyclesPeriod800, 1.0 / timingCyclesPeriod800 * ((double)F_CPU_ACTUAL / 1000.0));
        DebugPrint("T0H NZR high time duration: %u cycles\r\n", timingCyclesT0H);
        DebugPrint("T1H NZR high time duration: %u cycles\r\n", timingCyclesT1H);
        DebugPrint("Reset delay %u cycles\r\n", timingFrameResetDelay);
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
