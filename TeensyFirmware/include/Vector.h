// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#include "SimpleString.h"

template <typename T>
class Vector
{
    T* items = nullptr;
    size_t allocated = 0;
    size_t used = 0;
public:
    Vector()
    {
    }

    Vector(const Vector& other)
    {
        for (size_t i = 0; i < other.used; i++)
        {
            push_back(other[i]);
        }
    }

    ~Vector()
    {
        if (items != nullptr)
        {
            delete[] items;
        }
    }

    Vector& operator=(const Vector& other)
    {
        clear();
        for (size_t i = 0; i < other.used; i++)
        {
            push_back(other[i]);
        }
        return *this;
    }

    void reserve(size_t newSize)
    {
        if (allocated < newSize) {

            T* newBuffer = new T[newSize];
            if (newBuffer == nullptr)
            {
                CrashPrint("### Vector: out of memory!\r\n");
                return;
            }
            for (size_t i = 0; i < used; i++)
            {
                newBuffer[i] = items[i];
            }
            if (items != nullptr) {
                delete[] items;
            }
            items = newBuffer;
            allocated = newSize;
        }
    }

    void push_back(const T& x)
    {
        if (used == allocated)
        {
            size_t newSize = (allocated * 2) + 10;
            reserve(newSize);
        }
        items[used++] = x;
    }

    size_t reserved() const
    {
        return allocated;
    }

    size_t size() const
    {
        return used;
    }

    T& operator[](size_t index) const
    {
        return items[index];
    }

    void clear()
    {
        used = 0;
    }
};