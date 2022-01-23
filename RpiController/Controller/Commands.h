// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _COMMANDS_H
#define _COMMANDS_H

#include <string>
#include <vector>
#include "Utils.h"
STRICT_MODE_OFF
#include "Json.h"
STRICT_MODE_ON


class Command
{
public:
    std::string command;
    std::string hash;
    int sequence = 0; // a command sequence number
    std::vector<Color> colors;
    float seconds = 0; // length of animation or zero means run forever.
    float f1 = 0;
    float f2 = 0;
    int iterations = 0; // number of iterations on something like neural drop
    int size = 0; // size of rainbow
    int strip = 0; // specify a strip
    int colorsPerStrip = 0; // specify different gradient colors per strip.
    int index = 0; // specify one led in the strip
    std::vector<int> columns; // for column fade
    std::vector<Pixel> pixels; // for SetPixels command.

    bool operator==(const Command& other)
    {
        if (command == other.command && seconds == other.seconds && iterations == other.iterations && size == other.size && strip == other.strip && index == other.index
            && f1 == other.f1 && f2 == other.f2)
        {
            if (colors.size() == other.colors.size())
            {
                for (size_t i = 0; i < colors.size(); i++)
                {
                    if (colors[i] != other.colors[i])
                        return false;
                }

                if (columns.size() == other.columns.size())
                {
                    for (size_t j = 0; j < columns.size(); j++)
                    {
                        if (columns[j] != other.columns[j])
                            return false;
                    }

                    if (pixels.size() == other.pixels.size())
                    {
                        for (size_t k = 0; k < pixels.size(); k++)
                        {
                            if (pixels[k] != other.pixels[k])
                                return false;
                        }

                    }
                }
                return true;
            }
        }
        return false;
    }

    bool operator!=(const Command& other)
    {
        return !operator==(other);
    }

    int GetInt(nlohmann::json& doc, const std::string& name, int defaultValue = 0)
    {
        const auto it = doc.find(name);
        if (it != doc.end())
        {
            return (*it).get<int>();
        }
        return defaultValue;
    }

    float GetFloat(nlohmann::json& doc, const std::string& name, float defaultValue = 0)
    {
        const auto it = doc.find(name);
        if (it != doc.end())
        {
            return (*it).get<float>();
        }
        return defaultValue;
    }

    std::string GetString(nlohmann::json& doc, const std::string& name)
    {
        const auto it = doc.find(name);
        if (it != doc.end())
        {
            return (*it).get<std::string>();
        }
        return "";
    }

    bool ParseCommand(nlohmann::json& doc)
    {
        // parse the json
        try {

            command = doc["command"].get<std::string>();

            // get an iterastion number from the server to see if this is a command we need to play again.
            // This sequence number makes the raspberry pi resillient to restarts.
            sequence = GetInt(doc, "sequence", 0);

            if (command == "sensei")
            {
                seconds = GetFloat(doc, "seconds", 0);
                auto data = doc["colors"];
                ParseColors(data);
            }
            else if (command == "SetColor")
            {
                strip = GetInt(doc, "strip", 0);
                index = GetInt(doc, "index", 0);
                auto data = doc["colors"];
                ParseColors(data);
            }
            else if (command == "SetPixels")
            {
                auto data = doc["pixels"];
                ParsePixels(data);
            }
            else if (command == "ColumnFade")
            {
                seconds = GetFloat(doc, "seconds", 0);
                auto data = doc["columns"];
                ParseColumns(data);
            }
            else if (command == "CrossFade")
            {
                seconds = GetFloat(doc, "seconds", 0);
                auto data = doc["colors"];
                ParseColors(data);
            }
            else if (command == "Breathe")
            {
                seconds = GetFloat(doc, "seconds", 0);
                f1 = GetFloat(doc, "f1", 10.0f);
                f2 = GetFloat(doc, "f1", 0.2f);
            }
            else if (command == "WaterDrop")
            {
                size = GetInt(doc, "size", 1);
                iterations = GetInt(doc, "drops", 1);
                f1 = GetFloat(doc, "amount", 10.0f);
            }
            else if (command == "Gradient")
            {
                strip = GetInt(doc, "strip", -1);
                colorsPerStrip = GetInt(doc, "cps", 0);
                seconds = GetFloat(doc, "seconds", 0);
                auto data = doc["colors"];
                ParseColors(data);
            }
            else if (command == "MovingGradient")
            {
                seconds = GetFloat(doc, "speed", 0);
                f1 = GetFloat(doc, "direction", 0);
                size = GetInt(doc, "size", 0);
                auto data = doc["colors"];
                ParseColors(data);
            }
			else if (command == "StartRain")
			{
				size = GetInt(doc, "size", 1);
				f1 = GetFloat(doc, "amount", 10.0f);
			}
			else if (command == "StopRain")
			{
			}
            else if (command == "NeuralDrop")
            {
                iterations = GetInt(doc, "iterations", 0);
            }
            else if (command == "Twinkle")
            {
                // takes 2 colors and a seconds.
                // first is a base color, second is the random twinkle highlight color
                // seconds is the speed of the twinkling.
                seconds = GetFloat(doc, "seconds", 0);
                size = GetInt(doc, "density", 1);
                auto data = doc["colors"];
                ParseColors(data);
            }
            else if (command == "Rainbow")
            {
                seconds = GetFloat(doc, "seconds", 0);
                size = GetInt(doc, "length", 157);
            }
            else if (command == "Fire")
            {
                seconds = GetFloat(doc, "seconds", 0);
                f1 = GetFloat(doc, "f1", 55.0f);
                f2 = GetFloat(doc, "f2", 120.0f);
            }
            else if (command == "FirmwareHash")
            {
                hash = GetString(doc, "hash");
            }
            else if (command == "ping")
            {
                // just a heartbeat.
                return true;
            }
            else
            {
                std::cout << "### unrecognized command: " << command << "\n";
            }
        }
        catch (std::exception&)
        {
            std::cout << "### error parsing json command\n";
            return false;
        }
        return true;
    }

    void ParseColumns(nlohmann::json& data)
    {
        columns.clear();
        colors.clear();

        if (data.is_array())
        {
            for (auto it = data.begin(); it != data.end(); ++it)
            {
                auto row = it.value();
                if (row.is_object())
                {
                    int i = GetInt(row, "index", 0);
                    auto c = row["color"];
                    if (c.is_array())
                    {
                        std::vector<uint8_t> rgb;
                        for (auto ii = c.begin(); ii != c.end(); ++ii)
                        {
                            rgb.push_back((uint8_t)* ii);
                        }
                        if (rgb.size() >= 3)
                        {
                            uint8_t r = rgb[0];
                            uint8_t g = rgb[1];
                            uint8_t b = rgb[2];
                            colors.push_back(Color{ r, g, b });
                            columns.push_back(i);
                        }
                    }
                }
            }
        }
    }



    struct Range {
        int start;
        int end;
    };

    bool ParseNextRange(const char* buffer, int& pos, Range& range) {
        // parses a string like this into a sequence of Range objects:
        //  "1, 2-10, 30, 40 - 100";
        // returns:
        //   Range{1,1}
        //   Range{2,10}
        //   Range{30,30}
        //   Range{40,100}

        if (*buffer == '\0') {
            return false;
        }
        const char* start = &buffer[pos];
        if (*start == '\0')
            return false;
        char* end = nullptr;
        long i = strtol(start, &end, 10);

        if (end == buffer) {
            return false;
        }
        range.start = i;
        range.end = i;
        while (*end == ' ' && *end != '\0')
            end++;

        if (*end == '-') {
            char* ptr = end + 1;
            long e = strtol(ptr, &end, 10);
            if (ptr == end) {
                return false; // badly formed dangling '-'
            }
            range.end = e;

            while (*end == ' ' && *end != '\0')
                end++;
        }

        if (*end == ',') {
            // consume it!
            end++;
        }

        long long len = end - buffer;
        pos = (int)len;
        return true;
    }

    void ParsePixels(nlohmann::json& data)
    {
        pixels.clear();
        if (data.is_array())
        {
            for (auto it = data.begin(); it != data.end(); ++it)
            {
                auto row = it.value();
                if (row.is_object())
                {
                    int s = GetInt(row, "s", 0); // strip
                    auto ledrange = GetString(row, "l");
                    auto c = row["color"];
                    if (c.is_array())
                    {
                        std::vector<uint8_t> rgb;
                        for (auto ii = c.begin(); ii != c.end(); ++ii)
                        {
                            rgb.push_back((uint8_t)* ii);
                        }
                        if (rgb.size() >= 3)
                        {
                            uint8_t r = rgb[0];
                            uint8_t g = rgb[1];
                            uint8_t b = rgb[2];

                            Range range;
                            int pos = 0;
                            const char* buffer = ledrange.c_str();
                            while (ParseNextRange(buffer, pos, range)) {
                                for (int l = range.start; l <= range.end; l++) {
                                    pixels.push_back(Pixel{ s, l, Color{r, g, b} });
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    void ParseColors(nlohmann::json& data)
    {
        colors.clear();
        if (data.is_array())
        {
            for (auto it = data.begin(); it != data.end(); ++it)
            {
                auto v = it.value();
                if (v.is_array())
                {
                    std::vector<uint8_t> rgb;
                    for (auto ii = v.begin(); ii != v.end(); ++ii)
                    {
                        rgb.push_back((uint8_t)* ii);
                    }
                    if (rgb.size() >= 3)
                    {
                        uint8_t r = rgb[0];
                        uint8_t g = rgb[1];
                        uint8_t b = rgb[2];
                        colors.push_back(Color{ r, g, b });
                    }
                }
            }
        }
    }

};

#endif
