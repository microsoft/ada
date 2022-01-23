// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

// One of these must be defined, usually via the Makefile
//#define MACOSX
//#define LINUX
//#define WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdarg.h>
#if defined(MACOSX) || defined(LINUX)
#include <termios.h>
#include <sys/select.h>
#define PORTTYPE int
#define boolean bool
#define BAUD B115200
#if defined(LINUX)
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#elif defined(WINDOWS)
#include <windows.h>
#define PORTTYPE HANDLE
#define BAUD 115200
#elif defined(_WIN32)
#include <windows.h>
#define PORTTYPE HANDLE
#define BAUD 115200
#include <stdint.h> // portable: uint64_t   MSVC: __int64
#pragma warning(disable : 4996)
#else
#error "You must define the operating system\n"
#endif

PORTTYPE open_port_and_set_baud_or_die(const char* name, long baud);
int transmit_bytes(PORTTYPE port, const char* data, int len);
int recieve_bytes(PORTTYPE port, char* dest, int len);
void recvData(const PORTTYPE& port);
void close_port(PORTTYPE port);
void delay(double sec);
void delay_ms(double msec);
#if defined(_WIN32)
#include <sal.h>
void die(_Printf_format_string_ const char* format, ...);
#else
void die(const char* format, ...) __attribute__((format(printf, 1, 2)));
#endif


/**********************************/
/*  Serial Port Functions         */
/**********************************/


PORTTYPE open_port_and_set_baud_or_die(const char* name, long baud)
{
	PORTTYPE fd;
#if defined(MACOSX)
	struct termios tinfo;
	fd = open(name, O_RDWR | O_NONBLOCK);
	if (fd < 0) die("unable to open port %s\n", name);
	if (tcgetattr(fd, &tinfo) < 0) die("unable to get serial parms\n");
	cfmakeraw(&tinfo);
	if (cfsetspeed(&tinfo, baud) < 0) die("error in cfsetspeed\n");
	tinfo.c_cflag |= CLOCAL;
	if (tcsetattr(fd, TCSANOW, &tinfo) < 0) die("unable to set baud rate\n");
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
#elif defined(LINUX)
	struct termios tinfo;
	struct serial_struct kernel_serial_settings;
	int r;
	fd = open(name, O_RDWR);
	if (fd < 0) die("unable to open port %s\n", name);
	if (tcgetattr(fd, &tinfo) < 0) die("unable to get serial parms\n");
	cfmakeraw(&tinfo);
	if (cfsetspeed(&tinfo, baud) < 0) die("error in cfsetspeed\n");
	if (tcsetattr(fd, TCSANOW, &tinfo) < 0) die("unable to set baud rate\n");
	r = ioctl(fd, TIOCGSERIAL, &kernel_serial_settings);
	if (r >= 0) {
		kernel_serial_settings.flags |= ASYNC_LOW_LATENCY;
		r = ioctl(fd, TIOCSSERIAL, &kernel_serial_settings);
		if (r >= 0) printf("set linux low latency mode\n");
	}
	// https://stackoverflow.com/questions/2917881/how-to-implement-a-timeout-in-read-function-call
	tcgetattr(fd, &tinfo);
	tinfo.c_lflag &= ~ICANON; /* Set non-canonical mode */
	tinfo.c_cc[VMIN] = 0; /* Do not block on reads */
	tinfo.c_cc[VTIME] = 0; /* Set timeout of 0 seconds */
	tcsetattr(fd, TCSANOW, &tinfo);
#elif defined(WINDOWS) || defined(_WIN32)
	COMMCONFIG cfg;
	COMMTIMEOUTS timeout;
	DWORD n = 0;
	int num;
	char portname[256];
	if (sscanf(name, "COM%d", &num) == 1) {
		sprintf(portname, "\\\\.\\COM%d", num); // Microsoft KB115831
	}
	else {
		n = (DWORD) strlen(name);
		strncpy(portname, name, sizeof(portname) - 1);
		portname[n - 1] = 0;
	}
#if defined(_WIN32)
	wchar_t portname_wc[256];
	// https://stackoverflow.com/questions/19715144/how-to-convert-char-to-lpcwstr/19717944
	MultiByteToWideChar(CP_ACP, 0, portname, -1, portname_wc, 128);
#else
	char* portname_wc = portname;
#endif
	fd = CreateFile(portname_wc, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING, 0, NULL);
	if (fd == INVALID_HANDLE_VALUE) die("unable to open port %s\n", name);
	GetCommConfig(fd, &cfg, &n);
	//cfg.dcb.BaudRate = baud;
	cfg.dcb.BaudRate = 115200;
	cfg.dcb.fBinary = TRUE;
	cfg.dcb.fParity = FALSE;
	cfg.dcb.fOutxCtsFlow = FALSE;
	cfg.dcb.fOutxDsrFlow = FALSE;
	cfg.dcb.fOutX = FALSE;
	cfg.dcb.fInX = FALSE;
	cfg.dcb.fErrorChar = FALSE;
	cfg.dcb.fNull = FALSE;
	cfg.dcb.fRtsControl = RTS_CONTROL_ENABLE;
	cfg.dcb.fAbortOnError = FALSE;
	cfg.dcb.ByteSize = 8;
	cfg.dcb.Parity = NOPARITY;
	cfg.dcb.StopBits = ONESTOPBIT;
	cfg.dcb.fDtrControl = DTR_CONTROL_ENABLE;
	SetCommConfig(fd, &cfg, n);
	GetCommTimeouts(fd, &timeout);
	timeout.ReadIntervalTimeout = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.ReadTotalTimeoutConstant = 1;
	timeout.WriteTotalTimeoutConstant = 1000;
	timeout.WriteTotalTimeoutMultiplier = 0;
	SetCommTimeouts(fd, &timeout);
#endif
	return fd;

}

int transmit_bytes(PORTTYPE port, const char* data, int len)
{
#if defined(MACOSX) || defined(LINUX)
	return write(port, data, len);
#elif defined(WINDOWS) || defined(_WIN32)
	DWORD n;
	BOOL r;
	r = WriteFile(port, data, len, &n, NULL);
	if (!r) return 0;
	return n;
	/*
	DWORD n=0, acc = 0;
	while(acc < len ){
		int transmitLen = len;
		if (transmitLen > 22000)
			transmitLen = 22000;
		r = WriteFile(port, data+acc, transmitLen, &n, NULL);
		if (!r) return 0;
		acc += n;
	}
	return acc;
	*/
#endif
}

// https://stackoverflow.com/questions/6036716/serial-comm-using-writefile-readfile
int recieve_bytes(PORTTYPE port, char* dest, int len)
{
#if defined(MACOSX) || defined(LINUX)
	return read(port, dest, len);
#elif defined(WINDOWS) || defined(_WIN32)
	DWORD n;
	BOOL r;
	r = ReadFile(port, dest, len, &n, NULL);
	if (!r) return 0;
	return n;
#endif
}


void close_port(PORTTYPE port)
{
#if defined(MACOSX) || defined(LINUX)
	close(port);
#elif defined(WINDOWS)
	CloseHandle(port);
#endif
}

const int writeDataSz = LEDSPerStrip * NUM_LEDStrips * sizeof(uint32_t);
void recvData(const PORTTYPE& port, const int numBytesToRead, const int timeout) {
	uint8_t recvBuffer[writeDataSz + 2];
	int startTime = millis();
	if (numBytesToRead > writeDataSz)
		die("# ERROR: Asked to read more bytes (%d) than we allocate buffer (%d) for\n", numBytesToRead, writeDataSz);
	int readCount = 0;
	while (millis()-startTime <= timeout && readCount < numBytesToRead) {
		int x = millis() - startTime;
		int recvCount = recieve_bytes(port, (char*) & (recvBuffer[0]), numBytesToRead);
		readCount += recvCount;
		if (recvCount > 0) {
			recvBuffer[recvCount + 1] = '\0';
			fwrite(recvBuffer, 1, recvCount, stdout);
			//printf("%s", recvBuffer);
		}
	}
	if (millis() - startTime > timeout)
		printf("# WARN: Readback tmied out\n");
}

void recvData(const PORTTYPE& port) {
	uint8_t recvBuffer[writeDataSz + 2];
	int recvCount = recieve_bytes(port, (char*)&(recvBuffer[0]), writeDataSz);
	if (recvCount > 0) {
		recvBuffer[recvCount + 1] = '\0';
		fwrite(recvBuffer, 1, recvCount, stdout);
		//printf("%s", recvBuffer);
	}
}

/**********************************/
/*  Misc. Functions               */
/**********************************/

void delay(double sec)
{
#if defined(MACOSX) || defined(LINUX)
	usleep(sec * 1000000);
#elif defined(WINDOWS) || defined(_WIN32)
	Sleep( (DWORD) (sec * 1000));
#endif
}

void delay_ms(double msec)
{
#if defined(MACOSX) || defined(LINUX)
	usleep(msec * 1000);
#elif defined(WINDOWS) || defined(_WIN32)
	Sleep( (DWORD) msec);
#endif
}

#if defined(_WIN32)
void die(_Printf_format_string_ const char* format, ...)
#else
void __attribute__((format(printf, 1, 2))) die(const char* format, ...)
#endif
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	exit(1);
}
