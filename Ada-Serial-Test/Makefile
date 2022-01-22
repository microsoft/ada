#OS = LINUX
#OS = MACOSX
OS = LINUX

ifeq ($(OS), LINUX)
CC = g++
CFLAGS = -Wall -O2 -DLINUX
TARGET = receive_test

else ifeq ($(OS), MACOSX)
CC = gcc
CFLAGS = -Wall -O2 -DMACOSX
TARGET = receive_test

else ifeq ($(OS), WINDOWS)
CC = i586-mingw32msvc-gcc
CFLAGS = -Wall -O2 -DWINDOWS
TARGET = receive_test.exe
endif
all: $(TARGET)

$(TARGET): Ada\ SerialTest.cpp Adafruit_NeoPixel.cpp Adafruit_NeoPixel.h LED_Helper.h Serial_Functions.h Ada_Central_Cone.h Makefile
	$(CC) $(CFLAGS) -pthread -o $(TARGET) Ada\ SerialTest.cpp Adafruit_NeoPixel.cpp Adafruit_NeoPixel.h LED_Helper.h Serial_Functions.h Ada_Central_Cone.h 

clean:
	rm -f receive_test receive_test.exe

