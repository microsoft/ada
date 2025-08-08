// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
/*
  This driver uses the high CPU speed of the Teensy 4.0 to output WS2815/WS2811/WS2812 timed
  signals for driving many parallel strings of LEDs. Optimized for use with WS2815 LEDs at WS2811_800kH
  400kHz code has been removed for cleanliness

  Data is passed in using the Teensy 4.0 USBSERIAL interace from a Raspberry Pi with an assumed data
  rate of around 1MB/s. To maintain a higher refresh rate the program will attempt to read in data
  while the CPU is waiting for a frame lock/reset pulse to be sent.

  Valid output pins are on ports GPIO6 and GPIO7:
  +-------+----------+------+-------+
  |  Pin  |   Name   | GPIO |  PORT |
  +-------+----------+------+-------+
  | 0     | AD_B0_03 |  1.3 | GPIO6 |
  | 1     | AD_B0_02 |  1.2 | GPIO6 |
  | 6     | B0_10    | 2.10 | GPIO7 |
  | 7     | B1_01    | 2.17 | GPIO7 |
  | 8     | B1_00    | 2.16 | GPIO7 |
  |  9    | B0_11    | 2.11 | GPIO7 |
  | 10    | B0_00    |  2.0 | GPIO7 |
  | 11    | B0_02    |  2.2 | GPIO7 |
  | 12    | B0_01    |  2.1 | GPIO7 |
  | 13    | B0_03    |  2.3 | GPIO7 |
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

 Based off of
    OctoWS2811 Rainbow.ino - Rainbow Shifting Test
    http://www.pjrc.com/teensy/td_libs_OctoWS2811.html
    Copyright (c) 2013 Paul Stoffregen, PJRC.COM, LLC

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
  and
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
*/

#include "MultiWS2812.h"
//#include <Adafruit_NeoPixel.h>
#include "PixelBuffer.h"
#include "Commands.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "SimpleString.h"
#include "Controller.h"
#include "Status.h"
#include "Timer.h"

TeensyStatus gTeensyStatus = {0,0,0};

const int LED_PIN = 13;
#define ANIM_EXIT_ON_FIRST_RECV_BYTE (1)
#define NEURAL_ANIMATE_START_DELAY (5e6)
#define RECV_FRAME_TIME_OUT (5e6)
#define RECV_FRAME_FADE_OUT_DELAY (5e5)
//#define DEBUG_ADA_PRINT_FPS
#define PRINT_PREVIOUS_TO_CURRENT_BUFFER_STATS
#define PRINTBUFFER_STATS

/*****************************************************************************
 * LED Strip Layout
 *****************************************************************************/
static const uint32_t LEDSPerStrip = 392;
static const uint32_t NUM_LEDStrips = 16;

// Global PixelBuffer object for writing to all the strips.
Controller controller(NUM_LEDStrips, LEDSPerStrip);

// Timer statusTimer;

/*****************************************************************************
 * Setup()
 *****************************************************************************/
void setup()
{
    Serial.begin(115200);
    delay(500);

    DebugPrint("Setting up system\r\n");

    pinMode(LED_PIN, OUTPUT); // LED Pin

    Timer::init(); // initialize GPT1
    // statusTimer.start();

    // Print some stats about the current configuration:
    auto msg = stringf("LED max length: %d x %d cols\r\n", LEDSPerStrip, NUM_LEDStrips);
    DebugPrint(msg.c_str());

    // setup GPIO pins for strips
    controller.GetBuffer().setOutputPins();
    controller.Initialize();
    DebugPrint("WS2812 setup complete...\r\n");

    controller.PrintStatus();

    // start default infinite Neural drop pattern
    DebugPrint("Starting neural drop\r\n");
    controller.StartNeuralDrop(0);
}

/*****************************************************************************
 * Main loop()
 *****************************************************************************/
void loop() {
    DebugPrint("# INFO: Starting main loop\r\n");

    Command cmd;

    // Main control loop:
    // Recv buffers and send them out to the strips once they're complete
    // If new buffer data is not received for 5 seconds the system slowly fades to black
    // If on the first reboot no buffer data is received for 5 seconds animateNeuralSequence until data arrives
    while (true) {
        if (Serial.available())
        {
            // digitalWrite(LED_PIN, HIGH); // show we are reading serial
            bool read_something = cmd.readNextCommand();
            // digitalWrite(LED_PIN, LOW);
            if (read_something)
            {
                gTeensyStatus.commands++;
                if (cmd.error.size() > 0)
                {
                    // flush input so we can sync up on the next command.
                    cmd.resetInput();
                    DebugPrint("##COMPLETE##: %s at %d bps\r\n", cmd.error.c_str(), Serial.baud());
                    cmd.error = "";
                }
                else
                {
                    // new command received!
                    controller.StartCommand(cmd);
                    DebugPrint("##COMPLETE##: %s\r\n", cmd.command.c_str());
                }
            }
        }

        if (controller.HasAnimation())
        {
            if (controller.RunAnimation()) {
                // done!
            }
        }

        // This is handy for debugging...
        // if (statusTimer.microseconds() > 1000000){
        //     DebugPrint("Status draws=%d, headers=%d, commands=%d\r\n", gTeensyStatus.draws, gTeensyStatus.headers, gTeensyStatus.commands);
        //     gTeensyStatus.draws = 0;
        //     gTeensyStatus.commands = 0;
        //     gTeensyStatus.headers = 0;
        //     statusTimer.start();
        // }

    } // Should never get here
}
