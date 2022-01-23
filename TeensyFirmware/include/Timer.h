// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _TIMER_H
#define _TIMER_H

#include <chrono>

#if (ARDUINO_TEENSY40)
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif
#if defined(__AVR__) || defined(KINETISK) || defined(__MKL26Z64__)
#error "Sorry, this code only works on 32 bit Teensy boards, Teensy 4.0 specifically.  AVR isn't supported."
#endif

class Timer
{
private:
    uint32_t start_time;
    uint32_t end_time;
    bool running;

public:
    static void init() {
        // Very accurate timing for uS scale time periods (used for determining frame latch pulse delay)
        // https://github.com/manitou48/teensy4/blob/master/gpt_micros.ino
        CCM_CCGR1 |= CCM_CCGR1_GPT(CCM_CCGR_ON) |
                    CCM_CCGR1_GPT1_SERIAL(CCM_CCGR_ON);  // enable GPT1 module
        GPT1_CR = 0;
        GPT1_PR = 23;   // prescale+1
        GPT1_SR = 0x3F; // clear all prior status
        GPT1_CR = GPT_CR_EN | GPT_CR_CLKSRC(1) | GPT_CR_FRR;// 1 ipg 24mhz  4 32khz
    }

    void start()
    {
        start_time = now();
        running = true;
    }

    void stop() {
        end_time = now();
        running = false;
    }

    uint32_t microseconds() {
        auto end = get_end();
        return end - start_time;
    }

    float milliseconds() {
        auto end = get_end();
        return (float)(end - start_time) / 1000.0;
    }

    float seconds() {
        auto end = get_end();
        return (float)(end - start_time) / 1000000.0;
    }

    uint32_t now()
    {
        return GPT1_CNT;
    }

    uint32_t get_end() {
        if (running) {
            return now();
        } else {
            return end_time;
        }
    }

};


#endif