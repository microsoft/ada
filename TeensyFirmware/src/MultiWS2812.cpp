// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
/*-------------------------------------------------------------------------
  Arduino library to control a wide variety of WS2811- and WS2812-based RGB
  LED devices such as Adafruit FLORA RGB Smart Pixels and NeoPixel strips.
  Currently handles 400 and 800 KHz bitstreams on 8, 12 and 16 MHz ATmega
  MCUs, with LEDs wired for various color orders.  Handles most output pins
  (possible exception with upper PORT registers on the Arduino Mega).

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries,
  contributions by PJRC, Michael Miller and other members of the open
  source community.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  -------------------------------------------------------------------------
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
  -------------------------------------------------------------------------*/


#include "MultiWS2812.h"


  //#define MULTIWS2812_DBG_PRINT_OUTPUTPINS (1)
  //#define MULTIWS2812_PERF_PRINT_CYCLES (1)

#ifdef MULTIWS2812_DBG_PRINT_OUTPUTPINS
static void dbg_printmask(uint32_t value, const char* name) {
    char buf[256];
    sprintf(buf, "   %s: 0x%08x 0b", name, value);
    Serial.print(buf);
    Serial.println(value, BIN);
}
#endif

MultiWS2812::MultiWS2812(uint16_t LEDsPerStripCount, uint16_t stripCount) : begun(false), frameBuffer(NULL),
update_in_progress(0), update_completed_at(0),
port6Count(0), msk6(0),
port7Count(0), msk7(0),
curPin(0)
{
    maxLEDsPerStrip = LEDsPerStripCount;
    numStrips = stripCount;
    numBytes = (LEDsPerStripCount * stripCount);
    timer.start();

    // Get the set/clear system registers for port 6/7
    set6 = (uint32_t*)portSetRegister(0);
    clr6 = (uint32_t*)portClearRegister(0);
    set7 = (uint32_t*)portSetRegister(10);
    clr7 = (uint32_t*)portClearRegister(10);
#ifdef MULTIWS2812_FAST_DBG_GPIO
    dbg_pin1_set = (uint32_t*)portSetRegister(1);
    dbg_pin1_clr = (uint32_t*)portClearRegister(1);
    dbg_pin1_msk = digitalPinToBitMask(1);
#endif
    timingCyclesPeriod800 = WS2812Timing.DEFAULT_CYCLES_800;
    timingCyclesT0H = WS2812Timing.DEFAULT_CYCLES_T0H;
    timingCyclesT1H = WS2812Timing.DEFAULT_CYCLES_T1H;
    timingFrameResetDelay = WS2812Timing.DEFAULT_FRAME_RESET;
}

MultiWS2812::~MultiWS2812()
{
}

// Enable HW control of the appropriate pins and add them to the mask collection
boolean MultiWS2812::enableOutputPin(uint32_t bankStringNum, uint32_t pin)
{
    switch (pin)
    {
        /*
            +-------+----------+------+-------+
            |  Pin  |   Name   | GPIO |  PORT |
            +-------+----------+------+-------+
            | 0     | AD_B0_03 |  1.3 | GPIO6 |
            | 1     | AD_B0_02 |  1.2 | GPIO6 |
            | 14/A0 | AD_B1_02 | 1.18 | GPIO6 |
            | 15/A1 | AD_B1_03 | 1.19 | GPIO6 |
            | 16/A2 | AD_B1_07 | 1.23 | GPIO6 |
            | 17/A3 | AD_B1_06 | 1.22 | GPIO6 |
            | 18/A4 | AD_B1_01 | 1.17 | GPIO6 |
            | 19/A5 | AD_B1_00 | 1.16 | GPIO6 |
            | 20/A6 | AD_B1_10 | 1.26 | GPIO6 |
            | 21/A7 | AD_B1_11 | 1.27 | GPIO6 |
            | 22/A8 | AD_B1_08 | 1.24 | GPIO6 |
            | 23/A9 | AD_B1_09 | 1.25 | GPIO6 |
            +-------+----------+------+-------+
          */
          // case 0: [Not used in this hardware]
          // case 1: [Not used in this hardware]
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        // case 20: [Not used in this hardware]
    case 21:
    case 22:
    case 23:
        msk6 = msk6 | digitalPinToBitMask(pin);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        if (port6Count < bankStringNum)
            port6Count = bankStringNum;
        break;
        /*
          +-------+----------+------+-------+
          |  Pin  |   Name   | GPIO |  PORT |
          +-------+----------+------+-------+
          | 6     | B0_10    | 2.10 | GPIO7 |
          | 7     | B1_01    | 2.17 | GPIO7 |
          | 8     | B1_00    | 2.16 | GPIO7 |
          | 9     | B0_11    | 2.11 | GPIO7 |
          | 10    | B0_00    |  2.0 | GPIO7 |
          | 11    | B0_02    |  2.2 | GPIO7 |
          | 12    | B0_01    |  2.1 | GPIO7 |
          | 13    | B0_03    |  2.3 | GPIO7 |
          +-------+----------+------+-------+
        */
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        // case 13:
        msk7 = msk7 | digitalPinToBitMask(pin);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        if (port7Count < bankStringNum)
            port7Count = bankStringNum;
        break;
    default:
        return false;
        break;
    }
    pinNums[curPin] = pin;
    curPin++;

#ifdef MULTIWS2812_DBG_PRINT_OUTPUTPINS
    Serial.println("----------------------------------");
    char buf[256];
    sprintf(buf, "[%02d] Adding bank string num: %02d, pin number: %02d\n", curPin, bankStringNum, pin);
    Serial.print(buf);
    dbg_printmask((uint32_t)digitalPinToBitMask(pin), " pin");
    dbg_printmask(msk6, "msk6");
    dbg_printmask(msk7, "msk7");
    dbg_printmask(set6, "set6");
    dbg_printmask(set7, "set7");
#endif

    return false;
}



int MultiWS2812::busy(void)
{
    if (update_in_progress)
        return 1;

    // Busy for 300us after the done interrupt, for WS2811 reset/frame latch
    if (timer.microseconds() > timingFrameResetDelay)
        return 0;

    return 1;
}



__attribute__((optimize("unroll-loops")))
void MultiWS2812::show(void)
{
    if (frameBuffer == NULL)
        return;
    // Data latch = 300+ microsecond pause in the output stream.  Rather than
    // put a delay at the end of the function, the ending time is noted and
    // the function will simply hold off (if needed) on issuing the
    // subsequent round of data until the latch time has elapsed.  This
    // allows the mainline code to start generating the next frame of data
    // rather than stalling for the latch.
    while (busy());

#if defined(__arm__)
    noInterrupts(); // Need 100% focus on instruction timing

#if defined(ARDUINO_TEENSY40) && (defined(__IMXRT1052__) || defined(__IMXRT1062__))

#if defined(USE_CYCLE_DEFINES)
// Original slower speeds:
//#define CYCLES_800 (F_CPU_ACTUAL / 800000)
//#define CYCLES_800_T0H (F_CPU_ACTUAL / 4000000)
// Optimized speeds:
#define CYCLES_800_T0H (F_CPU_ACTUAL / 4100000)
#define CYCLES_800_T1H (F_CPU_ACTUAL / 1250000)
#define CYCLES_800 (F_CPU_ACTUAL / 840000)
#endif


#ifdef MULTIWS2812_PERF_PRINT_CYCLES
    uint32_t cycPerf, elapseAvg = 0, elapseCount = 0;
    elapseAvg = 0;
    elapseCount = 0;
#endif

    /*
      This code does all the time sensitive work for outputting multiple WS2812 bit
      banged digital signals. The WS2815 protocol is a NZR protocol where 0 and 1 are
      sent via a positive pulse of varying widths.
        0 , is set by a short pulse  [T0H] (220ns -  380ns)
        1 , is set by a longer pulse [T1H] (580ns - 1600ns)

             <----------------[800kHz]-------------->
             +-------------+                         +-----------------+
             |             |                         |
             |    T0H      |            T0L          |
             |[220ns-380ns]|       (580ns-1600ns)    |
         +---+             +-------------------------+

             +-----------------------+               +-----------------+
             |                       |               |
             |          T1H          |      T1L      |
             |     (580ns-1600ns)    | (220ns-420ns) |
         +---+                       +---------------+

      This code works by:
        1) setting everything we're driving high (msk6/msk7)
        2) Computing the T0H bits based on the pixel data (24-bit data per LED)
        3) Waiting until T0H
        4) Setting the T0H bits low after T0H has elapsed
        5) Waiting until T1H
        6) Set all bits low after T1H has elapsed [all outputs are zero]
        7) Waiting until the 800kHz cycle is up
      After setting every LED (each of which is a 24-bit signal). We wait ~300us (via the busy() function)
      to latch the data (this is usually called the RES reset pulse)

      WS2812 protocol format:
      [LED 24bit]  23  22  21  20  19  18  17  16    15  14  13  12  11  10  09  08    07  06  05  04  03  02  01  00
      [Byte index]  7   6   5   4   3   2   1   0     7   6   5   4   3   2   1   0     7   6   5   4   3   2   1   0
      [Color bits] G7  G6  G5  G4  G3  G2  G1  G0    R7  R6  R5  R4  R3  R2  R1  R0    B7  B6  B5  B4  B3  B2  B1  B0

      Pixel storage format:
      [int 32bit]  31  30  29  28  27  26  25  24    23  22  21  20  19  18  17  16    15  14  13  12  11  10  09  08    07  06  05  04  03  02  01  00
      [Byte index]  7   6   5   4   3   2   1   0     7   6   5   4   3   2   1   0     7   6   5   4   3   2   1   0     7   6   5   4   3   2   1   0
      [Color bits]  X   X   X   X   X   X   X   X    G7  G6  G5  G4  G3  G2  G1  G0    R7  R6  R5  R4  R3  R2  R1  R0    B7  B6  B5  B4  B3  B2  B1  B0
     */

    uint32_t* p = frameBuffer;
    uint32_t* end = p + (numBytes);
    uint32_t maskT0H_6, maskT0H_7, cyc, curBit;
    uint32_t bitMask = 0;

    update_in_progress = 1;
    // Ensure that the cycle counter is running:
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
    cyc = ARM_DWT_CYCCNT + timingCyclesPeriod800;

    // Loop through the entire buffer
    while (p < end)
    {
        bitMask = 0x01 << 23; // [0] 24th bit first --> [24] 0th bit

        // Loop through the strips (one 24bit RGB value for each strip):
        for (curBit = 0; curBit < 24; curBit++)
        {
            while (ARM_DWT_CYCCNT - cyc < timingCyclesPeriod800);
            dbg_write(HIGH); // Set GPIO1 HIGH [fast to avoid overhead]
            cyc = ARM_DWT_CYCCNT;
            // Turn on all outputs (NZR protocol)
            *set6 = msk6;
            *set7 = msk7;
            maskT0H_6 = 0;
            maskT0H_7 = 0;

#if 0
            // Flexible method (XXXns, by scope -- 48 cycles for core math, 80ns)
            // With loop unrolling (XXXns, by scope -- 25 cycles for core math, 41.7ns)
#ifdef MULTIWS2812_PERF_PRINT_CYCLES
            cycPerf = ARM_DWT_CYCCNT;
#endif
            uint32_t curCol = 0;
            for (int bank = 0; bank < 2; bank++)
            {
                if (bank == 0) {
                    portCount = &port6Count;
                    maskT0H = &maskT0H_6;
                }
                else {
                    portCount = &port7Count;
                    maskT0H = &maskT0H_7;
                }

                // Create GPIO6/GPIO7 masks for each column
                for (uint32_t curStrip = 0; curStrip <= *portCount; curStrip++) {
                    // If the bitMask value is 0, add it's PinBitMask to the T0H set
                    // otherwise it is in the T1H set which we don't need to compute
                    *maskT0H = *maskT0H | ((!(*(p + curCol) & bitMask)) * digitalPinToBitMask(pinNums[curCol]));
                    curCol++;
                }
            }
#ifdef MULTIWS2812_PERF_PRINT_CYCLES
            elapseAvg += ARM_DWT_CYCCNT - cycPerf;
            elapseCount++;
#endif
#endif

            maskT0H_6 = maskT0H_6 | (!(*p & bitMask) * digitalPinToBitMask(pinNums[0]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 1) & bitMask) * digitalPinToBitMask(pinNums[1]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 2) & bitMask) * digitalPinToBitMask(pinNums[2]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 3) & bitMask) * digitalPinToBitMask(pinNums[3]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 4) & bitMask) * digitalPinToBitMask(pinNums[4]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 5) & bitMask) * digitalPinToBitMask(pinNums[5]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 6) & bitMask) * digitalPinToBitMask(pinNums[6]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 7) & bitMask) * digitalPinToBitMask(pinNums[7]));
            maskT0H_6 = maskT0H_6 | (!(*(p + 8) & bitMask) * digitalPinToBitMask(pinNums[8]));
            maskT0H_7 = maskT0H_7 | (!(*(p + 9) & bitMask) * digitalPinToBitMask(pinNums[9]));
            maskT0H_7 = maskT0H_7 | (!(*(p + 10) & bitMask) * digitalPinToBitMask(pinNums[10]));
            maskT0H_7 = maskT0H_7 | (!(*(p + 11) & bitMask) * digitalPinToBitMask(pinNums[11]));
            maskT0H_7 = maskT0H_7 | (!(*(p + 12) & bitMask) * digitalPinToBitMask(pinNums[12]));
            maskT0H_7 = maskT0H_7 | (!(*(p + 13) & bitMask) * digitalPinToBitMask(pinNums[13]));
            maskT0H_7 = maskT0H_7 | (!(*(p + 14) & bitMask) * digitalPinToBitMask(pinNums[14]));
            maskT0H_7 = maskT0H_7 | (!(*(p + 15) & bitMask) * digitalPinToBitMask(pinNums[15]));


            // Wait for 0 transitioning pins [short T0H], we wait wrt the time we turned all outputs on HIGH (cyc):
            dbg_write(LOW);  // Set GPIO1 LOW [fast to avoid overhead]
            while (ARM_DWT_CYCCNT - cyc < timingCyclesT0H);
            *clr6 = maskT0H_6;
            *clr7 = maskT0H_7;
            bitMask = bitMask >> 1;

            // Wait for 1 transitioning pins [long T1H], we wait wrt the time we turned all outputs on HIGH (cyc):
            // Setting this with the msk (clearing everything) is faster (everything must be 0 now)
            while (ARM_DWT_CYCCNT - cyc < timingCyclesT1H);
            *clr6 = msk6;
            *clr7 = msk7;
        } // Done with all bits

        p = p + numStrips;     // Next set of pixels
    } // Done traversing memory

#ifdef MULTIWS2812_PERF_PRINT_CYCLES
    Serial.print(" [bit shifting] Cycles avg: ");
    Serial.println(elapseAvg / elapseCount);
#endif

    // Finish the last cycle
    while (ARM_DWT_CYCCNT - cyc < timingCyclesPeriod800);

    // Now we still need to wait for frame latch/reset pulse
    // This must be 300us or longer, no activity is allowed until that time is up
    timer.start();
    update_in_progress = 0;

#endif
    // END ARM ----------------------------------------------------------------
#else
#error Architecture not supported
#endif
    interrupts();
}

#if 0
// Really fast method (XXXns, by scope -- 19 cycles for core math, 31.7ns)
maskT0H_6 = maskT0H_6 | (!(*p & bitMask) * digitalPinToBitMask(pinNums[0]));
maskT0H_6 = maskT0H_6 | (!(*(p + 1) & bitMask) * digitalPinToBitMask(pinNums[1]));
maskT0H_6 = maskT0H_6 | (!(*(p + 2) & bitMask) * digitalPinToBitMask(pinNums[2]));
maskT0H_6 = maskT0H_6 | (!(*(p + 3) & bitMask) * digitalPinToBitMask(pinNums[3]));
maskT0H_6 = maskT0H_6 | (!(*(p + 4) & bitMask) * digitalPinToBitMask(pinNums[4]));
maskT0H_6 = maskT0H_6 | (!(*(p + 5) & bitMask) * digitalPinToBitMask(pinNums[5]));
maskT0H_6 = maskT0H_6 | (!(*(p + 6) & bitMask) * digitalPinToBitMask(pinNums[6]));
maskT0H_6 = maskT0H_6 | (!(*(p + 7) & bitMask) * digitalPinToBitMask(pinNums[7]));
maskT0H_6 = maskT0H_6 | (!(*(p + 8) & bitMask) * digitalPinToBitMask(pinNums[8]));
maskT0H_7 = maskT0H_7 | (!(*(p + 9) & bitMask) * digitalPinToBitMask(pinNums[9]));
maskT0H_7 = maskT0H_7 | (!(*(p + 10) & bitMask) * digitalPinToBitMask(pinNums[10]));
maskT0H_7 = maskT0H_7 | (!(*(p + 11) & bitMask) * digitalPinToBitMask(pinNums[11]));
maskT0H_7 = maskT0H_7 | (!(*(p + 12) & bitMask) * digitalPinToBitMask(pinNums[12]));
maskT0H_7 = maskT0H_7 | (!(*(p + 13) & bitMask) * digitalPinToBitMask(pinNums[13]));
maskT0H_7 = maskT0H_7 | (!(*(p + 14) & bitMask) * digitalPinToBitMask(pinNums[14]));
maskT0H_7 = maskT0H_7 | (!(*(p + 15) & bitMask) * digitalPinToBitMask(pinNums[15]));
elapseAvg += ARM_DWT_CYCCNT - cycPerf;
elapseCount++;


// Fairly fast method (240ns, by scope -- 103 cycles for core math, 171ns)
cycPerf = ARM_DWT_CYCCNT;
maskT0H_6 = maskT0H_6 | (!(*p & 0x1) * digitalPinToBitMask(pinNums[0]));
*p = (*p) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 1)) & 0x1) * digitalPinToBitMask(pinNums[1]));
*(p + 1) = (*(p + 1)) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 2)) & 0x1) * digitalPinToBitMask(pinNums[2]));
*(p + 2) = (*(p + 2)) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 3)) & 0x1) * digitalPinToBitMask(pinNums[3]));
*(p + 3) = (*(p + 3)) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 4)) & 0x1) * digitalPinToBitMask(pinNums[4]));
*(p + 4) = (*(p + 4)) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 5)) & 0x1) * digitalPinToBitMask(pinNums[5]));
*(p + 5) = (*(p + 5)) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 6)) & 0x1) * digitalPinToBitMask(pinNums[6]));
*(p + 6) = (*(p + 6)) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 7)) & 0x1) * digitalPinToBitMask(pinNums[7]));
*(p + 7) = (*(p + 7)) >> 1;
maskT0H_6 = maskT0H_6 | (!((*(p + 8)) & 0x1) * digitalPinToBitMask(pinNums[8]));
*(p + 8) = (*(p + 8)) >> 1;
maskT0H_7 = maskT0H_7 | (!((*(p + 9)) & 0x1) * digitalPinToBitMask(pinNums[9]));
*(p + 9) = (*(p + 9)) >> 1;
maskT0H_7 = maskT0H_7 | (!((*(p + 10)) & 0x1) * digitalPinToBitMask(pinNums[10]));
*(p + 10) = (*(p + 10)) >> 1;
maskT0H_7 = maskT0H_7 | (!((*(p + 11)) & 0x1) * digitalPinToBitMask(pinNums[11]));
*(p + 11) = (*(p + 11)) >> 1;
maskT0H_7 = maskT0H_7 | (!((*(p + 12)) & 0x1) * digitalPinToBitMask(pinNums[12]));
*(p + 12) = (*(p + 12)) >> 1;
maskT0H_7 = maskT0H_7 | (!((*(p + 13)) & 0x1) * digitalPinToBitMask(pinNums[13]));
*(p + 13) = (*(p + 13)) >> 1;
maskT0H_7 = maskT0H_7 | (!((*(p + 14)) & 0x1) * digitalPinToBitMask(pinNums[14]));
*(p + 14) = (*(p + 14)) >> 1;
maskT0H_7 = maskT0H_7 | (!((*(p + 15)) & 0x1) * digitalPinToBitMask(pinNums[15]));
*(p + 15) = (*(p + 15)) >> 1;
elapseAvg += ARM_DWT_CYCCNT - cycPerf;
elapseCount++;





// Computing T0H masks is unnecessary and not free
for (uint32_t curStrip = 0; curStrip < 8; curStrip++, p++) {
    if (*p & 0x1)
        maskT1H_6 = maskT1H_6 | digitalPinToBitMask(pinNums[curStrip]);
    else
        maskT0H_6 = maskT0H_6 | digitalPinToBitMask(pinNums[curStrip]);
    *p = (*p) >> 1;
}
maskT0H_7 = 0;
maskT1H_7 = 0;
for (uint32_t curStrip = 9; curStrip < 17; curStrip++, p++) {
    if (*p & 0x1)
        maskT1H_7 = maskT1H_7 | digitalPinToBitMask(pinNums[curStrip]);
    else
        maskT0H_7 = maskT0H_7 | digitalPinToBitMask(pinNums[curStrip]);
    *p = (*p) >> 1;
}

// Naive method for making this more flexible (too slow)
  // Generate TOH and T1H clear masks[6 then 7]:
for (int bank = 0; bank < 2; bank++)
{
    digitalWrite(1, HIGH);
    if (bank == 0) {
        p = pNext;
        portCount = &port6Count;
        maskT1H = &maskT1H_6;
        maskT0H = &maskT0H_6;
        pmskSet = &(pmsk6[0]);
    }
    else {
        p = pNext + port6Count;
        portCount = &port7Count;
        maskT1H = &maskT1H_7;
        maskT0H = &maskT0H_7;
        pmskSet = &(pmsk7[0]);
    }

    for (uint32_t curStrip = 0; curStrip <= *portCount; curStrip++)
    {
        if (*p & 0x01)
            * maskT1H = *maskT1H | *(pmskSet + curStrip);
        else
            *maskT0H = *maskT0H | *(pmskSet + curStrip);
        // Destructively shift the pixels we just read
        *p = *p >> 1;
        p++;
    }
}

// Somewhat efficient, but still a bit too slow
/*
      for( curStrip=0; curStrip<8; curStrip++,p++  ) {
        if( !(*p & 0x1) )
          maskT0H_6 = maskT0H_6 | digitalPinToBitMask(pinNums[curStrip]);
        *p = (*p)>>1;
      }
      for( curStrip=9; curStrip<17; curStrip++,p++ ) {
        if( !(*p & 0x1) )
          maskT0H_7 = maskT0H_7 | digitalPinToBitMask(pinNums[curStrip]);
        *p = (*p)>>1;
      }
*/


// Splitting saves very little time [240ns]
maskT0H_6 = maskT0H_6 | (!(*p & 0x1) * digitalPinToBitMask(pinNums[0]));
maskT0H_6 = maskT0H_6 | (!((*(p + 1)) & 0x1) * digitalPinToBitMask(pinNums[1]));
maskT0H_6 = maskT0H_6 | (!((*(p + 2)) & 0x1) * digitalPinToBitMask(pinNums[2]));
maskT0H_6 = maskT0H_6 | (!((*(p + 3)) & 0x1) * digitalPinToBitMask(pinNums[3]));
maskT0H_6 = maskT0H_6 | (!((*(p + 4)) & 0x1) * digitalPinToBitMask(pinNums[4]));
maskT0H_6 = maskT0H_6 | (!((*(p + 5)) & 0x1) * digitalPinToBitMask(pinNums[5]));
maskT0H_6 = maskT0H_6 | (!((*(p + 6)) & 0x1) * digitalPinToBitMask(pinNums[6]));
maskT0H_6 = maskT0H_6 | (!((*(p + 7)) & 0x1) * digitalPinToBitMask(pinNums[7]));
maskT0H_6 = maskT0H_6 | (!((*(p + 8)) & 0x1) * digitalPinToBitMask(pinNums[8]));
maskT0H_7 = maskT0H_7 | (!((*(p + 9)) & 0x1) * digitalPinToBitMask(pinNums[9]));
maskT0H_7 = maskT0H_7 | (!((*(p + 10)) & 0x1) * digitalPinToBitMask(pinNums[10]));
maskT0H_7 = maskT0H_7 | (!((*(p + 11)) & 0x1) * digitalPinToBitMask(pinNums[11]));
maskT0H_7 = maskT0H_7 | (!((*(p + 12)) & 0x1) * digitalPinToBitMask(pinNums[12]));
maskT0H_7 = maskT0H_7 | (!((*(p + 13)) & 0x1) * digitalPinToBitMask(pinNums[13]));
maskT0H_7 = maskT0H_7 | (!((*(p + 14)) & 0x1) * digitalPinToBitMask(pinNums[14]));
maskT0H_7 = maskT0H_7 | (!((*(p + 15)) & 0x1) * digitalPinToBitMask(pinNums[15]));

// Now wait for 0 transitioning pins:
digitalWrite(1, LOW);
while (ARM_DWT_CYCCNT - cyc < CYCLES_800_T0H);
*clr6 = maskT0H_6;
*clr7 = maskT0H_7;

// [80ns]
digitalWrite(1, HIGH);
*p = (*p) >> 1;
*(p + 1) = (*(p + 1)) >> 1;
*(p + 2) = (*(p + 2)) >> 1;
*(p + 3) = (*(p + 3)) >> 1;
*(p + 4) = (*(p + 4)) >> 1;
*(p + 5) = (*(p + 5)) >> 1;
*(p + 6) = (*(p + 6)) >> 1;
*(p + 7) = (*(p + 7)) >> 1;
*(p + 8) = (*(p + 8)) >> 1;
*(p + 9) = (*(p + 9)) >> 1;
*(p + 10) = (*(p + 10)) >> 1;
*(p + 11) = (*(p + 11)) >> 1;
*(p + 12) = (*(p + 12)) >> 1;
*(p + 13) = (*(p + 13)) >> 1;
*(p + 14) = (*(p + 14)) >> 1;
*(p + 15) = (*(p + 15)) >> 1;
p = p + 16;
digitalWrite(1, LOW);
#endif
