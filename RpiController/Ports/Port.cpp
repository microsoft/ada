// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Port.h"
#include "Timer.h"
#include <iostream>
#include <thread>
#include <string.h>

Port::~Port()
{
    if (readlineBuffer != nullptr)
    {
        delete[] readlineBuffer;
    }
}

const char*
Port::readline(int timeoutMilliseconds)
{
    if (readlineBuffer == nullptr)
    {
        bufferSize = 8000;
        readlineBuffer = new char[bufferSize];
        ::memset(readlineBuffer, 0, bufferSize);
    }
    Timer timer;
    timer.start();

    do 
    {
        if (totalBytesRead > 0)
        {
            // shift buffer down.
            int pos = 0;
            for (int j = totalBytesRead; j < totalBytesWritten; j++)
            {
                readlineBuffer[pos++] = readlineBuffer[j];
            }
            totalBytesRead = 0;
            totalBytesWritten = pos;
        }
        readlineBuffer[totalBytesWritten] = '\0';

        for (int i = totalBytesRead; i < totalBytesWritten; i++)
        {
            if (readlineBuffer[i] == '\n')
            {
                readlineBuffer[i] = '\0';
                if (i > 0 && readlineBuffer[i - 1] == '\r')
                {
                    // Arduino println seems to return '\r\n'
                    // this trims the '\r' from the buffer also.
                    readlineBuffer[i - 1] = '\0';
                }
                
                totalBytesRead = i + 1;
                return readlineBuffer;
            }            
        }
        int len = this->read((uint8_t*)&readlineBuffer[totalBytesWritten], bufferSize - totalBytesWritten - 1);
        if (len < 0)
        {
            return nullptr;
        }
        else if (len == 0) 
        {
            // no data retruned, so try again up to the timeout.
            if (timer.milliseconds() > timeoutMilliseconds)
            {
                // return what we have so far because we don't seem to be getting any more.
                readlineBuffer[totalBytesWritten] = '\0';
                totalBytesRead = totalBytesWritten;
                return readlineBuffer;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        totalBytesWritten += len;
        readlineBuffer[totalBytesWritten] = '\0';
    } while (totalBytesWritten < bufferSize && !isClosed());

    // no new lines, and buffer is full, so return everything so far...
    totalBytesRead = totalBytesWritten;
    return readlineBuffer;
}
