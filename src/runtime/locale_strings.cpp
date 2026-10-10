#include "locale_strings.h"
#include "basetype.h"
#include "mylib.h"
#include "exports.h"
#include <cstring>

void* RecordDBStringAssignSubstring(void*, void*, unsigned int, unsigned int);
void* RecordDBStringAssignBytes(void*, const void*, unsigned int);
void* RecordDBStringAppendBytes(void*, const void*, unsigned int);
void* RecordDBStringAppendSubstring(void*, const void*, unsigned int, unsigned int);
void* RecordDBStringAppendFill(void*, unsigned int, unsigned int);
void RecordDBStringFreeze(void*);
void RecordDBStringSplit(void*);

namespace Msvc6
{
    namespace
    {
        unsigned char& WideReference(unsigned short* data)
        {
            return reinterpret_cast<unsigned char*>(data)[-1];
        }

        bool WideShared(unsigned short* data)
        {
            return data != 0 && WideReference(data) != 0 && WideReference(data) != 255;
        }

        unsigned int WideLength(const unsigned short* text)
        {
            unsigned int length = 0;
            while (text[length] != 0) ++length;
            return length;
        }

        // Native empty-string terminators also have a zero reference byte before them.
        char emptyStringStorage[2] = {0, 0};
        unsigned short emptyWideStringStorage[2] = {0, 0};
        unsigned short* const emptyWideString = emptyWideStringStorage + 1;

        void CopyWideStorage(WideStringData* string, unsigned int requested)
        {
            unsigned int capacity = requested | 31u;
            if (capacity > 2147483645u) capacity = requested;
            unsigned short* allocation;
            try
            {
                const unsigned int count = capacity + 2u;
                allocation = static_cast<unsigned short*>(CrtNew(
                    BitsToSigned32(count) < 0 ? 0u : count * sizeof(unsigned short)));
            }
            catch (...)
            {
                capacity = requested;
                const unsigned int count = requested + 2u;
                allocation = static_cast<unsigned short*>(CrtNew(
                    BitsToSigned32(count) < 0 ? 0u : count * sizeof(unsigned short)));
            }
            const unsigned int copied = string->size < capacity ? string->size : capacity;
            for (unsigned int index = 0; index < copied; ++index)
                allocation[index + 1u] = string->data[index];
            const unsigned int previousLength = string->size;
            StringOperations<unsigned short>::Tidy(string, true);
            string->data = allocation + 1;
            WideReference(string->data) = 0;
            string->capacity = capacity;
            string->size = previousLength < capacity ? previousLength : capacity;
            string->data[string->size] = 0;
        }

        void SplitWideStorage(WideStringData* string)
        {
            unsigned short* const previous = string->data;
            if (WideShared(previous))
            {
                StringOperations<unsigned short>::Tidy(string, true);
                StringOperations<unsigned short>::AssignRange(string, previous, WideLength(previous));
            }
        }
    }

    template<> char* StringEmptyData<char>()
    {
        return emptyStringStorage + 1;
    }
    template<> unsigned short* StringEmptyData<unsigned short>()
    {
        return emptyWideString;
    }

    template<> void StringOperations<char>::Tidy(Data* string, bool release)
    {
        ::Tidy(string, static_cast<unsigned char>(release));
    }
    template<> bool StringOperations<char>::Grow(Data* string, unsigned int size, bool trim)
    {
        return ::Grow(string, size, static_cast<unsigned char>(trim)) != 0;
    }
    template<> void StringOperations<char>::Freeze(Data* string)
    {
        RecordDBStringFreeze(string);
    }
    template<> StringOperations<char>::Data* StringOperations<char>::AssignRange(
        Data* string, const char* source, unsigned int count)
    {
        return static_cast<Data*>(RecordDBStringAssignBytes(string, source, count));
    }
    template<> StringOperations<char>::Data* StringOperations<char>::AssignSubstring(
        Data* string, const Data* source, unsigned int offset, unsigned int count)
    {
        return static_cast<Data*>(RecordDBStringAssignSubstring(string, const_cast<Data*>(source), offset, count));
    }
    template<> StringOperations<char>::Data* StringOperations<char>::Append(
        Data* string, unsigned int count, char value)
    {
        return static_cast<Data*>(RecordDBStringAppendFill(string, count, static_cast<unsigned char>(value)));
    }
    template<> StringOperations<char>::Data* StringOperations<char>::AppendRange(
        Data* string, const char* source, unsigned int count)
    {
        return static_cast<Data*>(RecordDBStringAppendBytes(string, source, count));
    }
    template<> StringOperations<char>::Data* StringOperations<char>::AppendSubstring(
        Data* string, const Data* source, unsigned int offset, unsigned int count)
    {
        return static_cast<Data*>(RecordDBStringAppendSubstring(string, source, offset, count));
    }
    template<> StringOperations<char>::Data* StringOperations<char>::Erase(
        Data* string, unsigned int offset, unsigned int count)
    {
        if (offset > string->size) (void)Msvc6RangeError();
        RecordDBStringSplit(string);
        const unsigned int erased = MinUnsigned(string->size - offset, count);
        if (erased != 0)
        {
            std::memmove(string->data + offset, string->data + offset + erased,
                string->size - offset - erased);
            const unsigned int remaining = string->size - erased;
            if (::Grow(string, remaining, 0))
            {
                string->size = remaining;
                string->data[remaining] = 0;
            }
        }
        return string;
    }

    template<> void StringOperations<unsigned short>::Tidy(Data* string, bool release)
    {
        unsigned short* const data = string->data;
        if (release && data != 0)
        {
            const unsigned char references = WideReference(data);
            if (references == 0 || references == 255)
                CrtFreePointer(data - 1);
            else
                WideReference(data) = static_cast<unsigned char>(references - 1u);
        }
        string->data = 0;
        string->size = 0;
        string->capacity = 0;
    }

    template<> bool StringOperations<unsigned short>::Grow(Data* string, unsigned int size, bool trim)
    {
        if (size > 2147483645u) (void)Msvc6LengthError();
        if (WideShared(string->data))
        {
            if (size == 0)
            {
                --WideReference(string->data);
                Tidy(string, false);
                return false;
            }
            CopyWideStorage(string, size);
        }
        else if (size == 0)
        {
            if (trim) Tidy(string, true);
            else if (string->data != 0)
            {
                string->size = 0;
                string->data[0] = 0;
            }
            return false;
        }
        else if (trim && (string->capacity > 31u || string->capacity < size))
        {
            Tidy(string, true);
            CopyWideStorage(string, size);
        }
        else if (!trim && string->capacity < size)
            CopyWideStorage(string, size);
        return true;
    }

    template<> void StringOperations<unsigned short>::Freeze(Data* string)
    {
        if (WideShared(string->data)) Grow(string, string->size, false);
        if (string->data != 0) WideReference(string->data) = 255;
    }

    template<> StringOperations<unsigned short>::Data* StringOperations<unsigned short>::AssignRange(
        Data* string, const unsigned short* source, unsigned int count)
    {
        if (Grow(string, count, true))
        {
            for (unsigned int index = 0; index < count; ++index) string->data[index] = source[index];
            string->size = count;
            string->data[count] = 0;
        }
        return string;
    }

    template<> StringOperations<unsigned short>::Data* StringOperations<unsigned short>::AssignSubstring(
        Data* string, const Data* source, unsigned int offset, unsigned int count)
    {
        if (offset > source->size) (void)Msvc6RangeError();
        count = MinUnsigned(source->size - offset, count);
        if (string == source)
        {
            Erase(string, offset + count, static_cast<unsigned int>(-1));
            Erase(string, 0, offset);
        }
        else if (count != 0 && count == source->size &&
            WideReference(source->data != 0 ? source->data : const_cast<unsigned short*>(emptyWideString)) < 254)
        {
            Tidy(string, true);
            string->data = source->data != 0 ? source->data : const_cast<unsigned short*>(emptyWideString);
            string->size = source->size;
            string->capacity = source->capacity;
            ++WideReference(string->data);
        }
        else if (Grow(string, count, true))
        {
            const unsigned short* const data = source->data != 0 ? source->data : emptyWideString;
            for (unsigned int index = 0; index < count; ++index)
                string->data[index] = data[offset + index];
            string->size = count;
            string->data[count] = 0;
        }
        return string;
    }

    template<> StringOperations<unsigned short>::Data* StringOperations<unsigned short>::Append(
        Data* string, unsigned int count, unsigned short value)
    {
        const unsigned int previous = string->size;
        if (static_cast<unsigned int>(-1) - previous <= count) (void)Msvc6LengthError();
        if (count != 0 && Grow(string, previous + count, false))
        {
            const unsigned int first = string->size;
            for (unsigned int index = 0; index < count; ++index) string->data[first + index] = value;
            string->size = previous + count;
            string->data[string->size] = 0;
        }
        return string;
    }

    template<> StringOperations<unsigned short>::Data* StringOperations<unsigned short>::AppendSubstring(
        Data* string, const Data* source, unsigned int offset, unsigned int count)
    {
        if (offset > source->size) (void)Msvc6RangeError();
        count = MinUnsigned(source->size - offset, count);
        const unsigned int previous = string->size;
        if (static_cast<unsigned int>(-1) - previous <= count) (void)Msvc6LengthError();
        if (count != 0 && Grow(string, previous + count, false))
        {
            const unsigned short* const data = source->data != 0 ? source->data : emptyWideString;
            const unsigned int first = string->size;
            for (unsigned int index = 0; index < count; ++index)
                string->data[first + index] = data[offset + index];
            string->size = previous + count;
            string->data[string->size] = 0;
        }
        return string;
    }

    template<> StringOperations<unsigned short>::Data* StringOperations<unsigned short>::Erase(
        Data* string, unsigned int offset, unsigned int count)
    {
        if (offset > string->size) (void)Msvc6RangeError();
        SplitWideStorage(string);
        const unsigned int erased = MinUnsigned(string->size - offset, count);
        if (erased != 0)
        {
            std::memmove(string->data + offset, string->data + offset + erased,
                (string->size - offset - erased) * sizeof(unsigned short));
            const unsigned int remaining = string->size - erased;
            if (Grow(string, remaining, false))
            {
                string->size = remaining;
                string->data[remaining] = 0;
            }
        }
        return string;
    }
}
