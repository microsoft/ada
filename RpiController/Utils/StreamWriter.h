// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#include <string.h>
#include <stdint.h>
#include "crc32.h"

class StreamWriter
{
    char* buffer;
    int pos = 0;
    int length;

    void ReAllocate(int len)
    {
        if (pos + len >= length)
        {
            int newSize = (pos + len + 10) * 2;
            char* newBuffer = new char[newSize];
            if (buffer != nullptr) {
                ::memcpy(newBuffer, buffer, pos);
                delete[] buffer;
            }
            buffer = newBuffer;
            length = newSize;
        }
    }

    inline void CheckAllocate(int len)
    {
        if (pos + len >= length)
        {
            ReAllocate(len);
        }
    }

public:
    StreamWriter()
    {
        length = 1000;
        buffer = new char[length];
    }
    ~StreamWriter()
    {
        if (buffer != nullptr) {
            delete[] buffer;
        }
    }

    void WriteString(const char* ptr)
    {
        int len = (int)strlen(ptr);
        CheckAllocate(len);
        strcpy(&buffer[pos], ptr);
        pos += len;
    }

    void WriteByte(uint8_t x)
    {
        CheckAllocate(1);
        buffer[pos++] = x;
    }

    // patch the length field at offset with the current size - offset
    void WriteLength(uint32_t offset, uint32_t len)
    {
        // write in little endian order
        for (size_t j = 0; j < 4; j++)
        {
            buffer[j + offset] = len & 0xff;
            len >>= 8;
        }
    }

    inline void WriteInt(uint32_t i)
    {
        CheckAllocate(4);
        // write out in little endian order
        for (size_t j = 0; j < 4; j++)
        {
            WriteByte(i & 0xff);
            i >>= 8;
        }
    }

    void WriteIntBuffer(uint32_t* data, uint32_t len)
    {
        int bytes = len * sizeof(uint32_t);
        CheckAllocate(bytes);
        ::memcpy(&buffer[pos], data, bytes);
        pos += bytes;
    }

    void WriteFloat(float f)
    {
        CheckAllocate(4);
        float* ptr = (float*)& buffer[pos];
        *ptr = f;
        pos += 4;
    }

    void WriteCRC(uint32_t offset)
    {
        if (offset == 0)
        {
            uint32_t crc = crc32(nullptr, 0);
            WriteInt(crc);
        }
        else
        {
            uint32_t crc = crc32((uint8_t*)& buffer[offset], pos - offset);
            WriteInt(crc);
        }
    }

    char* GetBuffer() { return buffer; }

    int Size()
    {
        return pos;
    }

    void Clear()
    {
        pos = 0;
    }
};