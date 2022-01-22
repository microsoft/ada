#pragma once
#include <stdint.h>
#include <string>
#include <fstream>
#include <iostream>

class Bitmap
{
    int width = 0;
    int height = 0;
    uint8_t* buffer = nullptr;
public:
    Bitmap(int width, int height) 
    {
        this->width = width;
        this->height = height;
        buffer = new uint8_t[width * height * 3];
    }

    Bitmap(const Bitmap& other)
    {
        width = other.width;
        height = other.height;
        auto size = width * height * 3;
        buffer = new uint8_t[size];
        ::memcpy(buffer, other.buffer, size);
    }

    ~Bitmap() {
        delete[] buffer;
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
    {
        int stride = width * 3;
        int pos = (stride * y) + (x * 3);
        // stored in blue green red order for easy serialization to .bmp
        buffer[pos++] = b;
        buffer[pos++] = g;
        buffer[pos++] = r;
    }

    void SaveBitmap(const std::string& filename) const
    {
        std::ofstream file;
        file.open(filename, std::ios::out | std::ios::binary);
        if (file.fail())
            throw std::ios_base::failure(std::strerror(errno));
        SaveBitmap(file);
        file.close();
    }

    void SaveBitmap(std::ostream& stream) const
    {
        stream << "BM";
        
        int padding = ((width * 3) % 4) * height;
        int size = (width * height * 3) + padding + 54; // 54 byte header
        stream.write((const char*)&size, 4);

        int reserved = 0;
        stream.write((const char*)& reserved, 2); // reserved
        stream.write((const char*)& reserved, 2); // reserved
        
        short offset = 54;
        stream.write((const char*)& offset, 2); // to the end of header
        stream.write((const char*)& reserved, 2); // reserved

        int headerSize = 40;
        stream.write((const char*)& headerSize, 4); // size of this header

        stream.write((const char*)& width, 4); // width of bitmap
        stream.write((const char*)& height, 4); // height of bitmap

        short colorPlanes = 1;
        stream.write((const char*)& colorPlanes, 2); // color planes

        short bitsPerPixel = 24; 
        stream.write((const char*)& bitsPerPixel, 2); // bits per pixel.

        int compression = 0;
        stream.write((const char*)& compression, 4); // no compression

        int dummysize = 0; // a dummy 0 can be given for BI_RGB bitmaps.
        stream.write((const char*)& dummysize, 4); // image size

        int resolution = 0xec3;
        stream.write((const char*)& resolution, 4); // horizontal resolution
        stream.write((const char*)& resolution, 4); // vertical resolution
        
        int colors = 0;
        stream.write((const char*)& colors, 4); // colors in the color palette
        stream.write((const char*)& colors, 4); // number of important colors used.

        // write the pixels, padded to 4 byte boundary on each row.
        size_t dataSize = width * height * 3;
        for (size_t i = 0; i < dataSize; i += (width * 3))
        {
            char* raw = (char*)& buffer[i];
            stream.write(raw, width * 3);
            int row = width * 3;
            int remainder = row % 4;
            int zero = 0;
            stream.write((const char*)& zero, remainder); // pad the row
        }

    }

};