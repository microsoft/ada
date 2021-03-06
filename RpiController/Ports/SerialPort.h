// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef SERIAL_COM_SERIALPORT_HPP
#define SERIAL_COM_SERIALPORT_HPP

#include <string>
#include <memory>
#include <vector>

#include "Port.h"

enum Parity {
	Parity_None = (0x0100),
	Parity_Odd = (0x0200),
	Parity_Even = (0x0400),
	Parity_Mark = (0x0800),
	Parity_Space = (0x1000)
};

enum StopBits
{
	StopBits_None = 0,
	StopBits_10 = (0x0001),
	StopBits_15 = (0x0002),
	StopBits_20 = (0x0004)
};

enum Handshake
{
	Handshake_None,
	Handshake_XonXoff,
	Handshake_RequestToSend,
	Handshake_RequestToSendXonXoff
};

struct SerialPortInfo
{
    std::wstring displayName;
    std::wstring portName;
    int vid;
    int pid;
};

class SerialPort : public Port
{
public:
	SerialPort();
	~SerialPort();

	// open the serial port
	virtual int connect(const char* portName, int baudRate);

	// write to the serial port
	virtual int write(const uint8_t* ptr, int count);

	// read a given number of bytes from the port.
	virtual int read(uint8_t* buffer, int bytesToRead);

	// close the port.
	virtual void close();

	virtual bool isClosed();

    static std::vector<SerialPortInfo> FindSerialPorts(int vid, int pid);

	virtual void flush();
private:
	int setAttributes(int baud_rate, Parity parity, int data_bits, StopBits bits, Handshake hs, int readTimeout, int writeTimeout);


	class serialport_impl;
	std::unique_ptr<serialport_impl> impl_;
};

#endif