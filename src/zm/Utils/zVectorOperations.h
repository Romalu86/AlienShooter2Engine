#pragma once

#include "basetype.h"
#include "exports.h"

// Scalar and pointer operations for the native three-boundary vector storage.
// The caller owns the control object and preserves its construction/destruction.
namespace VectorOperations
{
template<class T>
T* Copy(const T* first, const T* last, T* output)
{
    while (first != last)
    {
        if (output) *output = *first;
        ++first;
        ++output;
    }
    return output;
}

template<class T>
void Insert(T*& begin, T*& end, T*& capacityEnd,
    T* position, unsigned int count, const T& value)
{
    const unsigned int available = static_cast<unsigned int>(capacityEnd-end);
    if (available<count)
    {
        T* const oldBegin = begin;
        T* const oldEnd = end;
        const unsigned int oldSize = oldBegin
            ? static_cast<unsigned int>(oldEnd-oldBegin) : 0u;
        const unsigned int newCapacity = oldSize+(count<oldSize ? oldSize : count);
        const unsigned int allocationCount = BitsToSigned32(newCapacity)<0
            ? 0u : newCapacity;
        T* const storage = static_cast<T*>(CrtNew(allocationCount*sizeof(T)));
        T* output = Copy(oldBegin, position, storage);
        for (unsigned int remaining = count; remaining; --remaining, ++output)
            if (output) *output = value;
        Copy(position, oldEnd, output);
        CrtFreePointer(oldBegin);
        capacityEnd = storage+newCapacity;
        begin = storage;
        end = storage+oldSize+count;
        return;
    }

    const unsigned int tailCount = static_cast<unsigned int>(end-position);
    if (tailCount<count)
    {
        Copy(position, end, position+count);
        T* output = end;
        for (unsigned int remaining = count-tailCount; remaining; --remaining, ++output)
            if (output) *output = value;
        for (output = position; output != end; ++output)
            *output = value;
        end += count;
    }
    else if (count)
    {
        Copy(end-count, end, end);
        T* source = end-count;
        T* output = end;
        while (source != position)
            *--output = *--source;
        for (output = position; output != position+count; ++output)
            *output = value;
        end += count;
    }
}

template<class T>
void Assign(T*& begin, T*& end, T*& capacityEnd,
    const T* sourceBegin, const T* sourceEnd)
{
    const unsigned int count = sourceBegin
        ? static_cast<unsigned int>(sourceEnd-sourceBegin) : 0u;
    const unsigned int oldSize = begin ? static_cast<unsigned int>(end-begin) : 0u;
    if (count<=oldSize)
    {
        T* output = begin;
        for (const T* source = sourceBegin; source != sourceEnd; ++source, ++output)
            *output = *source;
        end = begin+count;
    }
    else if (count<=(begin ? static_cast<unsigned int>(capacityEnd-begin) : 0u))
    {
        const T* source = sourceBegin;
        T* output = begin;
        for (unsigned int copied = 0; copied != oldSize; ++copied, ++source, ++output)
            *output = *source;
        Copy(source, sourceEnd, end);
        end = begin+count;
    }
    else
    {
        CrtFreePointer(begin);
        const unsigned int allocationCount = BitsToSigned32(count)<0 ? 0u : count;
        begin = static_cast<T*>(CrtNew(allocationCount*sizeof(T)));
        end = Copy(sourceBegin, sourceEnd, begin);
        capacityEnd = end;
    }
}
}
