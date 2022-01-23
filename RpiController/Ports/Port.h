// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef PORT_H
#define PORT_H
#include <stdint.h>
#include <string>

class Port
{
public:
    virtual ~Port();

	// write to the port, return number of bytes written or -1 if error.
	virtual int write(const uint8_t* ptr, int count) = 0;

	// read a given number of bytes from the port (blocking until the requested bytes are available).
	// return the number of bytes read or -1 if error.
	virtual int read(uint8_t* buffer, int bytesToRead) = 0;

    // read up to next new line char and return null terminatted buffer, or nullptr if
    // there is nothing to read right now.
    virtual const char* readline(int timeoutMilliseconds = 0);

	// close the port.
	virtual void close() = 0;

	virtual bool isClosed() = 0;

    virtual void flush() = 0;

private:
    // readline buffer
    char* readlineBuffer = nullptr;
    int bufferSize = 0;
    int totalBytesRead = 0;
    int totalBytesWritten = 0;

};
#endif // !PORT_H
