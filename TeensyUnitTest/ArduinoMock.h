// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "TcpClientPort.h"

const int HIGH = 1;
const int LOW = 0;

void digitalWrite(int pin, int value);

const int INPUT = 0;
const int OUTPUT = 1;

void pinMode(int pin, int mode);

void delay(float seconds);

// mocks the Arduino Serial library
class SerialInput
{
public:
    void setTimeout(int t) {

    }

    bool available()
	{
		if (connected) {
			return client.available();
		}
        return position < size;
    }

	void connect(const std::string& ipaddress, int port)
	{
		client.accept(ipaddress, port);
		connected = true;
	}

    int readBytes(char* buffer, int length)
    {
		if (connected)
		{
			int rc =  client.read((uint8_t*)buffer, length);
			if (rc < 0)
			{
				return 0;
			}
			return rc;
		}
		else if (position < size)
        {
            int available = size - position;
            if (available < length)
            {
                length = available;
            }
            ::memcpy(buffer, &this->buffer[position], length);
            position += length;
            return length;
        }
        return 0;
    }

    void setBuffer(char* buffer, int length)
    {
        this->buffer = buffer;
        this->size = length;
        this->position = 0;
    }

	void print(const char* message)
	{
		if (connected)
		{
			int len = (int)strlen(message);
			client.write((const uint8_t*)message, len);
		}
		else
		{
			std::cout << message << "\n";
		}
	}

	void flush() {}

private:
    char* buffer;
    int size;
    int position;
	TcpClientPort client;
	bool connected;
};

extern SerialInput Serial;