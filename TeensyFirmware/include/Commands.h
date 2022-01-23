// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.#ifndef _COMMANDS_H
#define _COMMANDS_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "Vector.h"
#include "Timer.h"
#include "SimpleString.h"
#include "crc32.h"
#include "String.h"
#include "Status.h"

enum class CommandType
{
    None,
    SetColor,
    NeuralDrop,
    CrossFade,
    WaterDrop,
    Rainbow,
    Twinkle,
    Breathe,
    Gradient,
    MovingGradient,
    FullBuffer,
    SpeedTest,
    Status,
    StartRain,
    StopRain,
    Fire
};

// Provides a wrapper on Commands parsed from the Serial port input.
class Command
{
public:
    const char* TAG_HEADER_STRING = "##HEADER##";
    const int TAG_HEADER_LENGTH = 10;
    CommandType type = CommandType::None;
    SimpleString command;
    Vector<Color> colors;
    float seconds = 0; // length of animation or zero means run forever.
    uint32_t iterations = 0; // number of iterations on something like neural drop
    uint32_t size = 0; // size of rainbow
    int strip = 0; // specify a strip
    int index = 0; // specify one led in the strip
    int colorsPerStrip = 0; // can optionally specify different colors per strip in Gradient command.
    SimpleString error;
    // used by EncodedBuffer command.
    uint32_t* pixelBuffer = nullptr;
    uint32_t numStrips = 0;
    uint32_t ledsPerStrip = 0;
    uint32_t pixelsUsed = 0;
    float f1 = 0; // factor 1
    float f2 = 0; // factor 2

    Command()
    {
    }

    ~Command()
    {
        if (pixelBuffer != nullptr) {
            delete[] pixelBuffer;
        }
    }

    Command& operator=(const Command& other)
    {
        this->type = other.type;
        this->command = other.command;
        this->colors = other.colors;
        this->seconds = other.seconds;
        this->iterations = other.iterations;
        this->size = other.size;
        this->strip = other.strip;
        this->index = other.index;
        this->colorsPerStrip = other.colorsPerStrip;
        this->error = other.error;
        this->f1 = other.f1;
        this->f2 = other.f2;
        if (other.pixelsUsed > 0)
        {
            if (other.pixelsUsed > this->pixelsUsed)
            {
                if (pixelBuffer != nullptr)
                {
                    delete [] pixelBuffer;
                    this->pixelsUsed = 0;
                }
                pixelBuffer = new uint32_t[other.pixelsUsed];
                if (pixelBuffer == nullptr)
                {
                    CrashPrint("### Command: out of memory\r\n");
                    return *this;
                }
                this->pixelsUsed = other.pixelsUsed;
            }
            this->numStrips = other.numStrips;
            this->ledsPerStrip = other.ledsPerStrip;
            ::memcpy((void*)pixelBuffer, (void*)other.pixelBuffer, (size_t)(sizeof(uint32_t) * other.pixelsUsed));
        }
        return *this;
    }

    void clearColors()
    {
        colors.clear();
    }

    void addColor(const Color& c)
    {
        colors.push_back(c);
    }

    void reset()
    {
        type = CommandType::None;
        command = "";
        colors.clear();
        seconds = 0;
        iterations = 0;
        size = 0;
        strip = 0;
        index = 0;
        colorsPerStrip = 0;
        error = "";
        // pixelBuffer: leave allocated so we can reuse it, which means we also need to
        // keep numStrips and ledsPerPixel since those tell us the size of the allocated buffer.
        // numStrips = 0;
        // ledsPerStrip = 0;
        pixelsUsed = 0;
        f1 = 0;
        f2 = 0;
    }

    class ReadState
    {
    public:
        uint32_t bufsize = 8000;
        uint8_t buffer[8000];
        int header_pos = 0;
        uint32_t readpos = 0; // we're to read next char
        uint32_t writepos = 0; // we're to add the next chunk
        int readState = 0;
        uint32_t count = 0; // a state specific counter.
        uint32_t crc = 0;
        uint32_t length = 0;
        uint32_t payloadSize = 0;
        uint8_t* payload = nullptr;
        SimpleString name;

        void reset() {
            header_pos = 0;
            readState = 0;
            count = 0;
            crc = 0;
            length = 0;
            name = "";
            // readpos = 0; // not these ones.
            // writepos = 0;
            // payloadSize; // records size of allocated payload
            // payload; // is allocated to hold a kind of high water mark.
        }

        bool reservePayload(size_t size)
        {
            if (size > payloadSize)
            {
                // high water mark allocation.
                if (payload != nullptr){
                    delete [] payload;
                }
                payload = new uint8_t[size + 50];
                if (payload == nullptr)
                {
                    CrashPrint("### Serial buffer out of memory");
                    payloadSize = 0;
                    return false;
                }
                payloadSize = (uint32_t)size;
            }
            return true;
        }
    };

    ReadState state;

    void resetInput() {
        state.writepos = 0;
        state.readpos = 0;
        state.readState = 0;
        while (Serial.available()){
            // flush the input back to empty so we sync up on the next command sent.
            Serial.readBytes((char*)&state.buffer[0], state.bufsize);
        }
    }

    bool readNextCommand()
    {
        // Option for timout (disabled for now)
        while (Serial.available() || state.readpos < state.writepos)
        {
            // readBytes can return incomplete buffers, so we have a full state machine
            // here that can read in the commands in whatever chunks we get.
            if (state.writepos >= state.bufsize)
            {
                error = "### serial buffer overflow";
                return true;
            }

            if (Serial.available())
            {
                if (state.readpos > 0)
                {
                    if (state.readpos < state.writepos) {
                        ::memmove(state.buffer, &state.buffer[state.readpos], state.writepos - state.readpos);
                        state.writepos -= state.readpos;
                    } else {
                        state.writepos = 0;
                    }
                    state.readpos = 0;
                }

                uint32_t bytesRead = Serial.readBytes((char*)&state.buffer[state.writepos], state.bufsize - state.writepos);
                state.writepos += bytesRead;
            }

            while (state.readpos < state.writepos)
            {
                // each record received starts with "##HEADER##", is followed by 4 byte integer which is the
                // payload size, followed by that many bytes, and finishing with a CRC for the payload.
                // this method is a re-entrant state machine that can work in chunks received.  The state
                // of the loop is stored in ReadState.
                if (state.readState == 0)
                {
                    // state 0  means reset the state machine!
                    // state 1 is parsing the expected ##HEADER##
                    error = "";
                    reset();
                    state.reset();
                    state.readState = 1;
                }

                char ch = state.buffer[state.readpos++];

                // now we always look for the header no matter what state we are in just in
                // case we lose UART bits and a command is truncated by the next command we
                // want to correctly parse that next command.
                if (ch == TAG_HEADER_STRING[state.header_pos])
                {
                    state.header_pos++;
                    if (state.header_pos == TAG_HEADER_LENGTH) {
                        // no matter what state we were in, we found a header
                        // so jump to state 2 !
                        gTeensyStatus.headers++;
                        state.readState = 2;
                        state.count = 0;
                        state.header_pos = 0;
                        state.name = "";
                        continue;
                    }
                } else {
                    state.header_pos = 0;
                }

                switch (state.readState)
                {
                case 0:
                    // searching for ##HEADER## tag.
                    break;

                case 2:
                    // read a null terminated command name.
                    if (ch == '\0')
                    {
                        // done! found the command name, now for the payload length.
                        state.readState = 3;
                        state.count = 0;
                        command = state.name;
                    }
                    else if (state.name.size() > 100)
                    {
                        error = "name too long!";
                        state.readState = 0;
                        state.count = 0;
                        return true;
                    }
                    else
                    {
                        state.name += ch;
                    }
                    break;

                case 3:
                    // read a 4 byte unsigned integer in little endian order
                    state.length = (state.length >> 8) | ((uint32_t)ch << 24);
                    state.count++;
                    if (state.count == 4)
                    {
                        if (state.length < 50000)
                        {
                            if (state.length > 0)
                            {
                                if (!state.reservePayload(state.length))
                                {
                                    error = "### out of payload memory";
                                    state.length = 0;
                                    state.readState = 0;
                                    return true;
                                }
                                state.readState = 4;
                                state.count = 0;
                            } else {
                                //skip payload.
                                state.readState = 5;
                                state.count = 0;
                                state.crc = 0;
                            }
                        }
                        else
                        {
                            error = "### message too long";
                            state.readState = 0;
                            return true;
                        }

                    }
                    break;

                case 4:
                    // read the payload
                    state.payload[state.count++] = ch;
                    if (state.count == state.length)
                    {
                        state.readState = 5;
                        state.crc = 0;
                        state.count = 0;
                    }
                    break;

                case 5:
                    // read a 4 byte unsigned integer CRC value in little endian order
                    state.crc = (state.crc >> 8) | ((uint32_t)ch << 24);
                    state.count++;
                    if (state.count == 4)
                    {
                        // complete buffer !!
                        uint32_t actual_crc = crc32(state.payload, state.length);
                        if (state.crc == actual_crc)
                        {
                            // buffer is good!
                            if (parseCommand(state.name, state.payload, state.length))
                            {
                                state.readState = 0;
                                error = "";
                                return true;
                            }
                            state.readState = 0;
                            state.count = 0;
                            return true;
                        } else {
                            error = "bad crc";
                            state.readState = 0;
                            state.count = 0;
                            return true;
                        }
                    }
                    break;
                }
            }
        }

        if (state.name.size() > 0)
        {
            if (state.name == "?\n" || state.name == "?\r\n")
            {
                type = CommandType::Status;
                return true;
            }
        }
        return false;
    }

    // Parse the payload
    bool parseCommand(const SimpleString& command, uint8_t* payload, uint32_t length)
    {
        // payload starts with a null terminated command name.
        if (command == "SetColor")
        {
            type = CommandType::SetColor;
            return parseSetColor(payload, length);
        }
        else if (command == "EncodedBuffer")
        {
            type = CommandType::FullBuffer;
            return parseEncodedBuffer(payload, length);
        }
        else if (command == "FullBuffer")
        {
            type = CommandType::FullBuffer;
            return parseFullBuffer(payload, length);
        }
        else if (command == "Breathe")
        {
            type = CommandType::Breathe;
            return parseBreathe(payload, length);
        }
        else if (command == "Gradient")
        {
            type = CommandType::Gradient;
            return parseGradient(payload, length);
        }
        else if (command == "MovingGradient")
        {
            type = CommandType::MovingGradient;
            return parseMovingGradient(payload, length);
        }
        else if (command == "CrossFade")
        {
            type = CommandType::CrossFade;
            return parseCrossFade(payload, length);
        }
        else if (command == "WaterDrop")
        {
            type = CommandType::WaterDrop;
            return parseWaterDrop(payload, length);
        }
        else if (command == "NeuralDrop")
        {
            type = CommandType::NeuralDrop;
            return parseNeuralDrop(payload, length);
        }
        else if (command == "Rainbow")
        {
            type = CommandType::Rainbow;
            return parseRainbow(payload, length);
        }
        else if (command == "Fire")
        {
            type = CommandType::Fire;
            return parseFire(payload, length);
        }
        else if (command == "Twinkle")
        {
            type = CommandType::Twinkle;
            return parseTwinkle(payload, length);
        }
        else if (command == "SpeedTest")
        {
            type = CommandType::SpeedTest;
            return true;
        }
        else if (command == "Status")
        {
            type = CommandType::Status;
            return true;
        }
        else if (command == "StopRain")
        {
            type = CommandType::StopRain;
            return true;
        }
        else if (command == "StartRain")
        {
            type = CommandType::StartRain;
            return parseStartRain(payload, length);
        }
        else
        {
            type = CommandType::None;
            error = "unknown command :";
            error += command;
            return false;
        }
    }

    bool parseSetColor(uint8_t* payload, uint32_t length)
    {
        uint32_t position = 0;
        if (position + 8 <= length) {
            strip = readInt32(&payload[position]);
            index = readInt32(&payload[position+4]);
            position += 8;
            bool rc = parseColors(&payload[position], length - position);
            if (!rc)
            {
                error = "SetColor: missing a color";
            }
            return true;
        }
        else
        {
            error = "SetColor: missing parameters";
        }
        return false;
    }

    bool parseEncodedBuffer(uint8_t* payload, uint32_t length)
    {
        // simple run - length encoded stream of color values, so it is{ N colors } repeated
        // where N is the run-length of that color.  It runs vertically down each strip, then the next strip.
        // this is optimized for contiguous colors being used, the obvious optimization is only 1 length
        // and one color if the pixel buffer is completely homogenous.
        uint32_t position = 0;
        if (position + 12 <= length) {
            uint32_t numStrips = readInt32(&payload[position]);
            uint32_t ledsPerStrip = readInt32(&payload[position+4]);
            seconds = readFloat(&payload[position + 8]);
            position += 12;
            uint32_t numPixels = numStrips * ledsPerStrip;
            if (!allocatePixelBuffer(numStrips, ledsPerStrip))
            {
                return false;
            }

            // set to black.
            ::memset(pixelBuffer, 0, sizeof(uint32_t) * numPixels);
            pixelsUsed = numPixels;

            uint32_t strip = 0;
            uint32_t led = 0;
            bool done = false;

            // now read in the run-length encoded information.
            while (position + 8 <= length && !done)
            {
                uint32_t number = readInt32(&payload[position]);
                position += 4;
                uint32_t color = readInt32(&payload[position]);
                position += 4;

                // Fill in 'number' copies of this color into the pixel buffer.
                for (uint32_t j = 0; j < number && !done; j++)
                {
                    uint32_t* pixel = (uint32_t*)(pixelBuffer + (led * numStrips) + strip);
                    *pixel = color;
                    led++;
                    if (led == ledsPerStrip)
                    {
                        led = 0;
                        strip++;
                        if (strip == numStrips)
                        {
                            done = true;
                        }
                    }
                }
            }
            return true;
        }
        else
        {
            error = "EncodedBuffer: missing parameters";
        }
        return false;
    }

    bool allocatePixelBuffer(uint32_t numStrips, uint32_t ledsPerStrip)
    {
        if (numStrips != this->numStrips || ledsPerStrip != this->ledsPerStrip)
        {
            if (pixelBuffer != nullptr) {
                delete[] pixelBuffer;
            }
            uint32_t numPixels = numStrips * ledsPerStrip;
            pixelBuffer = new uint32_t[numPixels];
            if (pixelBuffer == nullptr)
            {
                error = "out of memory";
                return false;
            }
            this->numStrips = numStrips;
            this->ledsPerStrip = ledsPerStrip;
        }
        return true;
    }

    bool parseFullBuffer(uint8_t* payload, uint32_t length)
    {
        // parse a full buffer of colors.
        uint32_t position = 0;
        if (position + 12 <= length)
        {
            uint32_t numStrips = readInt32(&payload[position]);
            uint32_t ledsPerStrip = readInt32(&payload[position + 4]);
            seconds = readFloat(&payload[position + 8]);
            position += 12;
            if (!allocatePixelBuffer(numStrips, ledsPerStrip))
            {
                return false;
            }
            // set to black.
            uint32_t numPixels = numStrips * ledsPerStrip;
            ::memset(pixelBuffer, 0, sizeof(uint32_t) * numPixels);
            pixelsUsed = numPixels;

            uint32_t remainder = length - position;
            if (remainder > numPixels)
            {
                remainder = numPixels;
            }
            ::memcpy(pixelBuffer, &payload[position], sizeof(uint32_t) * remainder);
            return true;
        }
        else
        {
            error = "EncodedBuffer: missing parameters";
        }
        return false;
    }

    bool parseGradient(uint8_t* payload, uint32_t length)
    {
        // parse seconds
        uint32_t position = 0;
        if (position + 12 <= length) {
            strip = readInt32(&payload[position]);
            position += 4;
            colorsPerStrip = readInt32(&payload[position]);
            if (colorsPerStrip > 0){
                this->command = "GradientCps";
            }
            position += 4;
            seconds = readFloat(&payload[position]);
            position += 4;
            bool rc = parseColors(&payload[position], length - position);
            if (!rc)
            {
                error = "Gradient: missing colors";
            }
            return true;
        }
        else
        {
            error = "Gradient: missing parameters";
            seconds = 0;
        }
        return false;
    }

    bool parseMovingGradient(uint8_t* payload, uint32_t length)
    {
        // parse speed, direction, size and colors.
        uint32_t position = 0;
        if (position + 16 <= length) {
            seconds = readFloat(&payload[position]);
            position += 4;
            f1 = readFloat(&payload[position]);
            position += 4;
            size = readUInt32(&payload[position]);
            position += 4;
            bool rc = parseColors(&payload[position], length - position);
            if (!rc)
            {
                error = "MovingGradient: missing colors";
            }
            return true;
        }
        else
        {
            error = "MovingGradient: missing required parameters";
            seconds = 0;
        }
        return false;
    }

    bool parseBreathe(uint8_t* payload, uint32_t length)
    {
        // parse seconds
        uint32_t position = 0;
        if (position + 12 <= length) {
            seconds = readFloat(&payload[position]);
            f1 = readFloat(&payload[position+4]);
            f2 = readFloat(&payload[position+8]);
            return true;
        }
        else
        {
            error = "Breathe: missing parameters";
            seconds = 0;
        }
        return false;
    }

    bool parseStartRain(uint8_t* payload, uint32_t length)
    {
        // parse seconds, and number of colors.
        uint32_t position = 0;
        if (position + 8 <= length) {
            size = readUInt32(&payload[position]);
            f1 = readFloat(&payload[position + 4]);
            if (size > 1000 || f1 > 255)
            {
                error = "StartRain: invalid parameter";
                return false;
            }
            return true;
        }
        else
        {
            error = "StartRain: missing parameters";
        }
        return false;
    }

    bool parseWaterDrop(uint8_t* payload, uint32_t length)
    {
        // parse seconds, and number of colors.
        uint32_t position = 0;
        if (position + 12 <= length) {
            iterations = readUInt32(&payload[position]);
            size = readUInt32(&payload[position + 4]);
            f1 = readFloat(&payload[position + 8]);
            return true;
        }
        else
        {
            error = "WaterDrop: missing parameters";
        }
        return false;
    }


    bool parseCrossFade(uint8_t* payload, uint32_t length)
    {
        // parse seconds, and number of colors.
        uint32_t position = 0;
        if (position + 4 <= length) {
            seconds = readFloat(&payload[position]);
            position += 4;
            bool rc = parseColors(&payload[position], length - position);
            if (!rc)
            {
                error = "CrossFade: missing colors";
            }
            return rc;
        }
        else
        {
            error = "CrossFade: missing parameters";
        }
        return false;
    }

    bool parseNeuralDrop(uint8_t* payload, uint32_t length)
    {
        // parse number of drops
        uint32_t position = 0;
        if (position + 4 <= length) {
            iterations = readUInt32(&payload[position]);
            return true;
        }
        else {
            error = "NeuralDrop: missing parameters";
        }
        return false;
    }

    bool parseRainbow(uint8_t* payload, uint32_t length)
    {
        // int length = 157, int seconds
        uint32_t position = 0;
        if (position + 8 <= length) {
            size = readUInt32(&payload[position]);
            seconds = readFloat(&payload[position + 4]);
            return true;
        }
        else {
            error = "Rainbow: missing parameters";
        }
        return false;
    }

    bool parseFire(uint8_t* payload, uint32_t length)
    {
        uint32_t position = 0;
        f1 = 55;
        f2 = 120;
        seconds = -1;
        if (position + 4 <= length) {
            f1 = readFloat(&payload[position]);
            if (position + 8 <= length) {
                f2 = readFloat(&payload[position + 4]);
                if (position + 12 <= length) {
                    seconds = readFloat(&payload[position + 8]);
                }
            }
        }
        return true;
    }

    bool parseTwinkle(uint8_t* payload, uint32_t length)
    {
        // parse size and seconds and colors.
        uint32_t position = 0;
        if (position + 8 <= length) {
            size = readUInt32(&payload[position]);
            seconds = readFloat(&payload[position + 4]);
            position += 8;
            bool rc = parseColors(&payload[position], length - position);
            if (!rc)
            {
                error = "Twinkle: missing colors";
            }
            return rc;
        }
        else {
            error = "Twinkle: missing parameters";
        }
        return false;
    }

    bool parseStripIndex(uint8_t* payload, uint32_t length)
    {
        uint32_t position = 0;
        if (position + 4 <= length) {
            strip = readUInt32(&payload[position]);
            return true;
        }
        else {
            error = "Disable: missing parameters";
        }
        return false;
    }

    bool parseColors(uint8_t* payload, uint32_t length)
    {
        uint32_t position = 0;
        while (position + 4 <= length)
        {
            uint32_t v = readUInt32(&payload[position]);
            colors.push_back(Color::from(v));
            position += 4;
        }
        return colors.size() > 0;
    }

    uint32_t readUInt32(uint8_t* payload)
    {
        uint32_t* ptr = (uint32_t*)payload;
        return *ptr;
    }

    int readInt32(uint8_t* payload)
    {
        int * ptr = (int*)payload;
        return *ptr;
    }

    float readFloat(uint8_t* payload)
    {
        // let's hope the bytes are in the same order...
        float* ptr = (float*)payload;
        return *ptr;
    }

};

#endif
