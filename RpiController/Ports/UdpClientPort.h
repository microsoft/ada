// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef SERIAL_COM_UDPCLIENTPORT_HPP
#define SERIAL_COM_UDPCLIENTPORT_HPP

#include "Port.h"

class UdpClientPort : public Port
{
public:
	UdpClientPort();
	~UdpClientPort();

	// Connect can set you up two different ways.  Pass 0 for local port to get any free local
	// port and pass a fixed remotePort if you want to send to a specific remote port.
	// Conversely, pass a fix local port to bind to, and 0 for remotePort if you want to
	// allow any remote sender to send to your specific local port.  localHost allows you to
	// be specific about local adapter ip address.
	void connect(const std::string& localHost, int localPort, const std::string& remoteHost, int remotePort);

	// write the given bytes to the port, return number of bytes written or -1 if error.
	virtual int write(const uint8_t* ptr, int count);

	// read some bytes from the port, return the number of bytes read or -1 if error.
	virtual int read(uint8_t* buffer, int bytesToRead);

	// close the port.
	virtual void close();

	virtual bool isClosed();

	virtual void flush() {};

	std::string remoteAddress();
	int remotePort();

private:
	class UdpSocketImpl;
	std::unique_ptr<UdpSocketImpl> impl_;
};


#endif // SERIAL_COM_UDPCLIENTPORT_HPP
