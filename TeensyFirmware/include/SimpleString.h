#ifndef _SIMPLESTRING_H
#define _SIMPLESTRING_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

void DebugPrint(const char* format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    Serial.print(buf);
}

void CrashPrint(const char* format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    Serial.print(buf);
    Serial.flush();
    Timer timer;
    timer.start();
    while (timer.milliseconds() < 1000);
}

class SimpleString
{
private:
    char* ptr = nullptr;
    size_t allocated = 0;
    size_t used = 0;
public:
    SimpleString() {}
    SimpleString(const char* s)
    {
        auto size = strlen(s);
        reserve(size);
        if (ptr != nullptr) {
            strcpy(ptr, s);
            used = size;
        }
    }


    SimpleString(const SimpleString& other)
    {
        if (other.ptr != nullptr)
        {
            auto size = strlen(other.c_str());
            reserve(size);

            if (ptr != nullptr)
            {
                strcpy(ptr, other.c_str());
                used = size;
            }
        }
    }

    SimpleString(SimpleString&& moved)
    {
        ptr = moved.ptr;
        allocated = moved.allocated;
        used = moved.used;
        moved.ptr = nullptr;
        moved.used = 0;
        moved.allocated = 0;
    }

    ~SimpleString() {
        if (ptr != nullptr) {
            delete[] ptr;
        }
    }

    size_t size() const {
        return used;
    }

    void reserve(size_t size)
    {
        if (size + 1 >= allocated )
        {
            char* newptr = new char[size + 1];
            if (newptr == nullptr)
            {
                CrashPrint("### SimpleString: out of memory!\r\n");
                return;
            }
            if (used > 0)
            {
                ::memcpy(newptr, ptr, used + 1);
            }
            else
            {
                newptr[0] = 0;
            }
            if (ptr != nullptr) {
                delete[] ptr;
            }
            ptr = newptr;
            allocated = size + 1;
        }
    }

    SimpleString& operator=(const char* s)
    {
        auto size = strlen(s);
        reserve(size);
        strcpy(ptr, s);
        used = size;
        return *this;
    }

    SimpleString& operator=(const SimpleString& s)
    {
        return operator=(s.c_str());
    }

    bool operator==(const char* s) const {
        if (s == nullptr) {
            return ptr == nullptr;
        }
        else if (ptr == nullptr) {
            return false;
        }
        return strcmp(ptr, s) == 0;
    }

    bool operator==(const SimpleString& s) const {
        return operator==(s.c_str());
    }

    SimpleString& operator+(char ch)
    {
        if (used + 1 >= allocated) {
            reserve(allocated + 100);
        }
        ptr[used++] = ch;
        ptr[used] = '\0';
        return *this;
    }

    SimpleString& operator+(const char* s1)
    {
        auto size = used + strlen(s1);
        reserve(size);
        strcat(ptr, s1);
        used = size;
        return *this;
    }

    SimpleString& operator+=(char ch)
    {
        return operator+(ch);
    }

    SimpleString& operator+(const SimpleString& s1)
    {
        return operator+(s1.c_str());
    }

    SimpleString& operator+=(const SimpleString& s1)
    {
        return operator+(s1.c_str());
    }

    SimpleString& operator+=(const char* s1)
    {
        return operator+(s1);
    }

    const char* c_str() const { return ptr; }

    void clear(){
        used = 0;
    }
};

static SimpleString stringf(const char* format, ...)
{
   SimpleString result;
   result.reserve(1024);
   va_list ap;
   va_start(ap, format);
   vsnprintf((char*)result.c_str(), 1024, format, ap);
   return result;
}

#endif