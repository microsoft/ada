// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _UTILS_H
#define _UTILS_H

#include <string>
#include <cstdarg>
#include <stdexcept>
#include <memory>
#include <locale>
#include <codecvt>
#include <vector>
#include <string.h>
#include "Color.h"

#if defined(_MSC_VER)
//'=': conversion from 'double' to 'float', possible loss of data
#define STRICT_MODE_OFF                                           \
    __pragma(warning(push))										  \
    __pragma(warning( disable : 4100 4189 4244 4245 4239 4464 4456 4505 4514 4571 4624 4626 4267 4710 4820 5027 5031))
#define STRICT_MODE_ON                                            \
    __pragma(warning(pop))

//TODO: limit scope of below statements required to suppress VC++ warnings
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(disable:4996)
#else
#define STRICT_MODE_OFF
#define STRICT_MODE_ON
#endif

// special way to quiet the warning:  warning: format string is not a string literal
#ifdef __CLANG__
#define IGNORE_FORMAT_STRING_ON                                       \
    _Pragma("clang diagnostic push")                                  \
    _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")

#define IGNORE_FORMAT_STRING_OFF                                      \
    _Pragma("clang diagnostic pop")
#else
#define IGNORE_FORMAT_STRING_ON
#define IGNORE_FORMAT_STRING_OFF
#endif

// A handy class for making long running operations cancellable.
class CancelToken
{
public:
    bool Cancel;
};


// Call this on a function parameter to suppress the unused paramter warning
template <class T> inline
void unused(T const& result) { static_cast<void>(result); }

const double M_PI = 3.14159265358979323846;   // pi

class Utils {
public:

    typedef std::string string;
    typedef std::wstring wstring;

    static string utf8(wstring input)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(input);
    }

    static string stringf(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        string result;

        char* buffer = nullptr;
        vasprintf(&buffer, format, args);
        if (buffer != nullptr) {
            result = string(buffer);
            free(buffer);
        }

        va_end(args);

        return result;
    }

    static bool startsWith(const string& s, const string& prefix)
    {
        if (s.size() > prefix.size())
        {
            if (strncmp(s.c_str(), prefix.c_str(), prefix.size()) == 0)
            {
                return true;
            }
        }
        return false;
    }


    static std::vector<std::string> split(const string& s, const char* splitChars, int numSplitChars)
    {
        auto start = s.begin();
        std::vector<string> result;
        for (auto it = s.begin(); it != s.end(); it++)
        {
            char ch = *it;
            bool split = false;
            for (int i = 0; i < numSplitChars; i++)
            {
                if (ch == splitChars[i]) {
                    split = true;
                    break;
                }
            }
            if (split)
            {
                if (start < it)
                {
                    result.push_back(string(start, it));
                }
                start = it;
                start++;
            }
        }
        if (start < s.end())
        {
            result.push_back(string(start, s.end()));
        }
        return result;
    }

#ifdef _MSC_VER
    static int vasprintf(char** buffer, const char* format, va_list pargs)
    {
        va_list args;
        va_copy(args, pargs);

        IGNORE_FORMAT_STRING_ON
            auto size = _vscprintf(format, args) + 1U;
        IGNORE_FORMAT_STRING_OFF
            * buffer = (char*)malloc(size);

        IGNORE_FORMAT_STRING_ON
            int retval = vsnprintf(*buffer, size, format, args);
        IGNORE_FORMAT_STRING_OFF

            va_end(args);
        return retval;
    }
#endif

};

#endif
