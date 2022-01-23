// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _PIXELBUFFER_H
#define _PIXELBUFFER_H

#include <math.h>
#include "Utils.h"
#include "StreamWriter.h"

// This class abstracts the 16 LED strips as one big pixel buffer that we can setup.
// It provides a "write" method which then sends the buffer to the Teensy.
class TeensyPixelBuffer
{
    const char* header = "##HEADER##";
    uint32_t bufferSize;
    uint32_t* pixelBuffer;
    Port& _port; // teensy serial port
    int numStrips;
    int ledsPerStrip;
    CancelToken& _token;
public:
    TeensyPixelBuffer(Port& port, int numStrips, int ledsPerStrip, CancelToken& token) : _port(port), _token(token)
    {
        this->numStrips = numStrips;
        this->ledsPerStrip = ledsPerStrip;
        bufferSize = sizeof(uint32_t) * (numStrips * ledsPerStrip);
        pixelBuffer = new uint32_t[bufferSize];
        ::memset(pixelBuffer, 0, bufferSize);
    }

    ~TeensyPixelBuffer()
    {
        delete[] pixelBuffer;
    }

    int NumStrips() { return numStrips; }
    int NumLedsPerStrip() { return ledsPerStrip; }

    uint32_t* CopyPixels()
    {
        auto result = new uint32_t[bufferSize];
        ::memcpy(result, pixelBuffer, bufferSize);
        return result;
    }

    // Set color with optional strip/led arguments.  If not defined
    // you set everything, if strip >= 0 you set a column, and if
    // led >= 0 you set one led.  If strip < 0 and led > 0 you set
    // a row.
    void SetColor(Color color, int strip, int led)
    {
        // pack colors according to neopixel format.
        if (strip != -1)
        {
            // then we are only operating on 1 strip.
            if (led != -1)
            {
                // then we are only changging one pixel.
                SetPixel(color, strip, led);
            }
            else
            {
                // then we are setting all pixels in this strip, which is a "set row"
                SetColumn(color, strip);
            }
        }
        else if (led != -1)
        {
            // set a row
            SetRow(color, led);
        }
        else
        {
            SetColor(color);
        }
    }

    // Set entire strip to one color.
    void SetColor(Color color)
    {
        uint32_t value = color.pack();
        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < ledsPerStrip; j++)
            {
                uint32_t* pixel = GetPixelAddress(i, j);
                *pixel = value;
            }
        }
    }

    // Set a pixel on all strips to this color (so it is a row across the strips).
    void SetRow(Color color, int index)
    {
        uint32_t value = color.pack();

        for (int j = 0; j < numStrips; j++)
        {
            uint32_t* pixel = GetPixelAddress(j, index);
            *pixel = value;
        }
    }

    // Set one color for every led in a strip.
    void SetColumn(Color color, int strip)
    {
        uint32_t value = color.pack();
        for (int i = 0; i < ledsPerStrip; i++)
        {
            uint32_t* pixel = GetPixelAddress(strip, i);
            *pixel = value;
        }
    }

    // Set individual colors for every led in a strip.
    void SetColumn(int strip, std::vector<Color>& colors)
    {
        int i = 0;
        for (; i < ledsPerStrip && i < colors.size(); i++)
        {
            Color c = colors[i];
            SetPixel(c, strip, i);
        }
        // turn off everything else that was not specified.
        while (i++ < ledsPerStrip)
        {
            uint32_t* pixel = GetPixelAddress(strip, i);
            *pixel = 0;
        }
    }

    void SendSetColor(Color& color, int strip, int index)
    {
        SetColor(color, strip, index);

        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("SetColor");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(strip);
        writer.WriteInt(index);
        writer.WriteInt(color.pack());
        uint32_t payloadSize = writer.Size() - offset;
        writer.WriteLength(lenOffset, payloadSize);
        writer.WriteCRC(offset);
		Send(writer);
    }

    void QueryStatus()
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("Status");
        writer.WriteByte(0); // null terminate the command string
        writer.WriteInt(0); // length, no payload.
        writer.WriteCRC(0);
		Send(writer);
    }

    void Breathe(float seconds, float f1, float f2)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("Breathe");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteFloat(seconds);
        writer.WriteFloat(f1);
        writer.WriteFloat(f2);
        uint32_t payloadSize = writer.Size() - offset;
        writer.WriteLength(lenOffset, payloadSize);
        writer.WriteCRC(offset);
		Send(writer);
    }

    inline uint32_t* GetPixelAddress(int strip, int led) {
        return (uint32_t*)(pixelBuffer + (led * numStrips) + strip);
    }

    inline void SetPixel(Color& c, int strip, int led)
    {
        uint32_t* pixel = GetPixelAddress(strip, led);
        *pixel = c.pack();
    }

    // Call this method to send our entire buffer to the Teensy.  You usually call this
    // after doing a series of SetColor, SetColumn, SetRow or SetPixel methods to setup the
    // buffer that you want to send.  The seconds provided is a cross-fade time
    void SendEncodedBuffer(float seconds)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("EncodedBuffer");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(this->numStrips);
        writer.WriteInt(this->ledsPerStrip);
        writer.WriteFloat(seconds);

        // simple run-length encoded stream of color values, so it is { N colors } repeated
        // where N is the run-length of that color.  It runs vertically down each strip, then the next strip.
        // this is optimized for contiguous colors being used, the obvious optimization is only 1 length
        // and one color if the pixel buffer is completely homogenous.
        uint32_t length = 0;
        uint32_t current = *pixelBuffer;
        for (int i = 0; i < numStrips; i++)
        {
            for (int j = 0; j < ledsPerStrip; j++)
            {
                uint32_t* pixel = GetPixelAddress(i, j);
                uint32_t value = *pixel;
                if (current == value)
                {
                    length++;
                }
                else if (length > 0)
                {
                    writer.WriteInt(length);
                    writer.WriteInt(current);
                    length = 1;
                    current = value;
                }
            }
        }
        if (length > 0)
        {
            writer.WriteInt(length);
            writer.WriteInt(current);
        }

        uint32_t payloadSize = writer.Size() - offset;

        if (payloadSize > bufferSize)
        {
            // degenerate case, we'd be better off just sending every pixel.
            SendFullBuffer(seconds);
            return;
        }
        else
        {
            writer.WriteLength(lenOffset, payloadSize);
            writer.WriteCRC(offset);
        }
		Send(writer);
    }

    void RunSerialTest(float msdelay)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("SpeedTest");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(0); // empty payload
        uint32_t payloadSize = writer.Size() - offset;
        writer.WriteLength(lenOffset, payloadSize);
        writer.WriteCRC(offset);
		Send(writer);

        std::cout << "running serial test...\n";
        Timer timer;
        timer.start();
        uint8_t buffer[1000];
        uint8_t x = 0;
        for (int i = 0; i < 1000; i++)
        {
            memset(buffer, x, 1000);
            int sent = _port.write(buffer, 1000);
            if (sent != 1000) {
                std::cout << "send only " << sent << " bytes\n";
                break;
            }
            x++;
            if (x == 255) x = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds((int)msdelay));
        }
        timer.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ::memset(buffer, 255, 1000);
        _port.write(buffer, 1000);
        std::cout << "done in " << timer.seconds() << " seconds\n";
        return;
    }

    void SendFullBuffer(float seconds)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("FullBuffer");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(this->numStrips);
        writer.WriteInt(this->ledsPerStrip);
        writer.WriteFloat(seconds);

        writer.WriteIntBuffer(pixelBuffer, numStrips * ledsPerStrip);
        uint32_t payloadSize = writer.Size() - offset;
        writer.WriteLength(lenOffset, payloadSize);
        writer.WriteCRC(offset);
		Send(writer);
    }


    // Set entire buffer to new color, using smooth crossfade over given number of seconds.
    void CrossFadeTo(Color color, float seconds)
    {
        SetColor(color); // record this fact in our own buffer...

        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("CrossFade");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteFloat(seconds);
        writer.WriteInt(color.pack());
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
		Send(writer);
    }

    void Rainbow(int length = 157, float seconds = 10)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("Rainbow");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(length);
        writer.WriteFloat(seconds);
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
		Send(writer);
    }

    void Fire(float cooling = 55, float sparkle = 120, float seconds = 0)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("Fire");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteFloat(cooling);
        writer.WriteFloat(sparkle);
        writer.WriteFloat(seconds);
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
        Send(writer);
    }

    //
    void VerticalGradient(const std::vector<Color>& colors, float seconds = 0, int strip = -1, int colorsPerStrip = 0)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("Gradient");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(strip); // optional strip index, if -1 animate all strips with these colors.
        writer.WriteInt(colorsPerStrip); // optional different colors per strip (so colors array is numStrips * colorsPerStrip)
        writer.WriteFloat(seconds);
        for(const Color& c : colors)
        {
            writer.WriteInt(c.pack());
        }
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
		Send(writer);
    }

    void MovingVerticalGradient(const std::vector<Color>& colors, float speed, float direction, uint32_t size)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("MovingGradient");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteFloat(speed);
        writer.WriteFloat(direction);
        writer.WriteInt(size);
        for (const Color& c : colors)
        {
            writer.WriteInt(c.pack());
        }
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
        Send(writer);
    }

    void Twinkle(Color baseColor, Color twinkle, float speed, int density)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("Twinkle");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(density);
        writer.WriteFloat(speed);
        writer.WriteInt(baseColor.pack());
        writer.WriteInt(twinkle.pack());
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
		Send(writer);
    }

    void NeuralDrop(int drops)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("NeuralDrop");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(drops);
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
		Send(writer);
    }

    void WaterDrop(int size, int drops, float amount)
    {
        StreamWriter writer;
        writer.WriteString(header);
        writer.WriteString("WaterDrop");
        writer.WriteByte(0); // null terminate the command string
        auto lenOffset = writer.Size();
        writer.WriteInt(0); // placeholder for length
        auto offset = writer.Size();
        writer.WriteInt(drops);
        writer.WriteInt(size);
        writer.WriteFloat(amount);
        writer.WriteLength(lenOffset, writer.Size() - offset);
        writer.WriteCRC(offset);
		Send(writer);
    }

	void StartRain(int size, float amount)
	{
		StreamWriter writer;
		writer.WriteString(header);
		writer.WriteString("StartRain");
		writer.WriteByte(0); // null terminate the command string
		auto lenOffset = writer.Size();
		writer.WriteInt(0); // placeholder for length
		auto offset = writer.Size();
		writer.WriteInt(size);
		writer.WriteFloat(amount);
		writer.WriteLength(lenOffset, writer.Size() - offset);
		writer.WriteCRC(offset);
		Send(writer);
	}

	void StopRain()
	{
		StreamWriter writer;
		writer.WriteString(header);
		writer.WriteString("StopRain");
		writer.WriteByte(0); // null terminate the command string
        writer.WriteInt(0); // length, no payload.
        writer.WriteCRC(0);
		Send(writer);
	}
private:

    void Send(StreamWriter& writer)
    {
        // write the complete record to the serial port.
        std::cout << "writing " << writer.Size() << " bytes to Teensy...";
        int n = _port.write((uint8_t*)writer.GetBuffer(), writer.Size());
        if (n != writer.Size()) {
            std::cout << "errors transmitting data, sent " << n << ", size " << writer.Size() << "\n";
            return;
        }
        else {
            std::cout << "\n";
        }
        bool completed = false;
        bool error = false;
        int retries = 1000;
		while (retries-- > 0) {
			const char* ptr = _port.readline(2000); // max 2 second timeout
			if (ptr != nullptr && *ptr != 0) {
                std::string line = ptr;
				std::cout << line << "\n";
				if (line.find("##COMPLETE##") == 0)
				{
                    completed = true;
				}
                else if (line.find("Status ") >= 0)
                {
                    // ignore these
                }
                else if (line.find("### ") >= 0)
                {
                    // hmmm, the port is out of sync somehow...
                    error = true;
                }
            }

            if (error)
            {
                _port.flush();
                return;
            }
            else if (completed)
            {
                return;
            }
            else
            {
                // max delay of 100*50 = 5000 milliseconds waiting for ##COMPLETE##
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
		}
        std::cout << "### Teensy is not responding with ##COMPLETE##?\n";
    }
};

#endif