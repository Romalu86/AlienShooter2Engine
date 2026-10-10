#pragma once
#include "zm/Game/RecordDB/zRecordDB.h"

namespace Msvc6
{
    struct WideStringData
    {
        unsigned char allocatorState[4];
        unsigned short* data;
        unsigned int size;
        unsigned int capacity;
    };

    template<class Character> struct StringStorage;
    template<> struct StringStorage<char> { typedef AS_RECORD_STRING_CONTROL Type; };
    template<> struct StringStorage<unsigned short> { typedef WideStringData Type; };
    template<class Character> using StringData = typename StringStorage<Character>::Type;

    template<class Character> Character* StringEmptyData();
    template<> char* StringEmptyData<char>();
    template<> unsigned short* StringEmptyData<unsigned short>();

    template<class Character> struct StringOperations
    {
        typedef StringData<Character> Data;
        static void Tidy(Data* string, bool release);
        static bool Grow(Data* string, unsigned int size, bool trim);
        static void Freeze(Data* string);
        static Data* AssignRange(Data* string, const Character* source, unsigned int count);
        static Data* AssignSubstring(Data* string, const Data* source, unsigned int offset, unsigned int count);
        static Data* Append(Data* string, unsigned int count, Character value);
        static Data* AppendRange(Data* string, const Character* source, unsigned int count);
        static Data* AppendSubstring(Data* string, const Data* source, unsigned int offset, unsigned int count);
        static Data* Erase(Data* string, unsigned int offset, unsigned int count);
    };

    template<> void StringOperations<char>::Tidy(AS_RECORD_STRING_CONTROL*, bool);
    template<> bool StringOperations<char>::Grow(AS_RECORD_STRING_CONTROL*, unsigned int, bool);
    template<> void StringOperations<char>::Freeze(AS_RECORD_STRING_CONTROL*);
    template<> AS_RECORD_STRING_CONTROL* StringOperations<char>::AssignRange(AS_RECORD_STRING_CONTROL*, const char*, unsigned int);
    template<> AS_RECORD_STRING_CONTROL* StringOperations<char>::AssignSubstring(AS_RECORD_STRING_CONTROL*, const AS_RECORD_STRING_CONTROL*, unsigned int, unsigned int);
    template<> AS_RECORD_STRING_CONTROL* StringOperations<char>::Append(AS_RECORD_STRING_CONTROL*, unsigned int, char);
    template<> AS_RECORD_STRING_CONTROL* StringOperations<char>::AppendRange(AS_RECORD_STRING_CONTROL*, const char*, unsigned int);
    template<> AS_RECORD_STRING_CONTROL* StringOperations<char>::AppendSubstring(AS_RECORD_STRING_CONTROL*, const AS_RECORD_STRING_CONTROL*, unsigned int, unsigned int);
    template<> AS_RECORD_STRING_CONTROL* StringOperations<char>::Erase(AS_RECORD_STRING_CONTROL*, unsigned int, unsigned int);
    template<> void StringOperations<unsigned short>::Tidy(WideStringData*, bool);
    template<> bool StringOperations<unsigned short>::Grow(WideStringData*, unsigned int, bool);
    template<> void StringOperations<unsigned short>::Freeze(WideStringData*);
    template<> WideStringData* StringOperations<unsigned short>::AssignRange(WideStringData*, const unsigned short*, unsigned int);
    template<> WideStringData* StringOperations<unsigned short>::AssignSubstring(WideStringData*, const WideStringData*, unsigned int, unsigned int);
    template<> WideStringData* StringOperations<unsigned short>::Append(WideStringData*, unsigned int, unsigned short);
    template<> WideStringData* StringOperations<unsigned short>::AppendSubstring(WideStringData*, const WideStringData*, unsigned int, unsigned int);
    template<> WideStringData* StringOperations<unsigned short>::Erase(WideStringData*, unsigned int, unsigned int);
}
