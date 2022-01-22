#include "ArduinoMock.h"
#include <chrono>
#include <thread>

SerialInput Serial;;

void digitalWrite(int pin, int value)
{

}

void pinMode(int pin, int mode)
{

}

void delay(float seconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds((long long)(seconds * 1000)));
}