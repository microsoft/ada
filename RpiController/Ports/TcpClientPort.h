// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef SERIAL_COM_TCPCLIENTPORT_HPP
#define SERIAL_COM_TCPCLIENTPORT_HPP

#include "Port.h"
#include <memory>

class TcpClientPort : public Port
{
public:
	TcpClientPort();
	~TcpClientPort();

	// Connect can set you up two different ways.  Pass 0 for local port to get any free local
	// port. localHost allows you to be specific about which local adapter to use in case you
	// have multiple ethernet adapters.
	void connect(const std::string& localHost, int localPort, const std::string& remoteHost, int remotePort);

    // Connect to server using any available local address and port.
    void connect(const std::string& remoteHost, int remotePort);

	// start listening on the local adapter, and accept one connection request from a remote machine.
	void accept(const std::string& localHost, int localPort);

	// write the given bytes to the port, return number of bytes written or -1 if error.
	virtual int write(const uint8_t* ptr, int count);

	// read some bytes from the port, return the number of bytes read or -1 if error.
	virtual int read(uint8_t* buffer, int bytesToRead);

	// return true if there is something available to read
	bool available();

	// close the port.
	virtual void close();

	virtual void flush() {};

	virtual bool isClosed();

	std::string remoteAddress();
	int remotePort();

    std::string localAddress();
    int localPort();

    static std::string getHostName();

    static std::string getHostByName(const std::string& name);

private:
	class TcpSocketImpl;
	std::unique_ptr<TcpSocketImpl> impl_;
};


#endif // SERIAL_COM_UDPCLIENTPORT_HPP
