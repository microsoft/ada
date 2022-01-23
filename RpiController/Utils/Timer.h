// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _TIMER_H
#define _TIMER_H

#include <chrono>

class Timer
{
private:
    typedef std::chrono::high_resolution_clock clock;
    unsigned long long start_time;
    unsigned long long end_time;
    bool running;

public:
    void start()
    {
        start_time = now();
        running = true;
    }

    void stop() {
        end_time = now();
        running = false;
    }

    double microseconds() {
        auto end = end_time;
        if (running) {
            end = now();
        }
        return (double)(end - start_time) / 1000.0;
    }
    double milliseconds() {
        auto end = end_time;
        if (running) {
            end = now();
        }
        return (double)(end - start_time) / 1000000.0;
    }
    double seconds() {
        auto end = end_time;
        if (running) {
            end = now();
        }
        return (double)(end - start_time) / 1000000000.0;
    }

    unsigned long long now()
    {
        return clock::now().time_since_epoch().count();
    }
};


#endif