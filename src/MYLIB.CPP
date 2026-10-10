
#include "mylib.h"
#include "myerror.h"
#include "basetype.h"
#include "registry.h"
#include "stream.h"
#include "sprite.h"
#include "vid.h"
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <windows.h>
#include <mmsystem.h>

int PROFILE::Load(const STRING& path)
{

    fileName = StringGlobals::EmptyText();
    fileName = path;
    return 0;
}

int PROFILE::GetInt(const STRING& section, const STRING& key, int defaultValue)
{
    return static_cast<int>(GetPrivateProfileIntA(
        section.value, key.value, defaultValue, fileName.value));
}

STRING PROFILE::GetString(
    const STRING& section, const STRING& key, const STRING& defaultValue)
{
    char buffer[32768];
    std::memset(buffer, 0, sizeof(unsigned int));
    GetPrivateProfileStringA(
        section.value, key.value, defaultValue.value, buffer, 32767u, fileName.value);
    return STRING(buffer);
}

static unsigned int StringCapacity(unsigned int length)
{
    return (length & ~15u) + 16u;
}

const STRING& STRING::operator=(const STRING& source)
{

    if(this==&source) return *this;

    const char* const sourceText=source.value;
    const unsigned int sourceLength=(unsigned int)strlen(sourceText);
    char* const oldText=value;

    if(oldText==StringEmptySentinel()) {
        if(sourceLength==0u) return *this;
        value=static_cast<char*>(CrtNew(StringCapacity(sourceLength)));
    } else if(sourceLength==0u) {
        CrtFreePointer(oldText);
        value=StringEmptySentinel();
        return *this;
    } else {
        const unsigned int oldLength=(unsigned int)strlen(oldText);
        if(StringCapacity(oldLength)!=StringCapacity(sourceLength)) {
            CrtFreePointer(oldText);
            value=static_cast<char*>(CrtNew(StringCapacity(sourceLength)));
        }
    }

    strncpy(value,source.value,sourceLength+1u);
    return *this;
}

const STRING& STRING::operator=(const char* source)
{
    const unsigned int sourceLength=(unsigned int)strlen(source);
    char* const oldText=value;

    if(oldText==StringEmptySentinel()) {
        if(sourceLength==0u) return *this;
        value=static_cast<char*>(CrtNew(StringCapacity(sourceLength)));
    } else if(sourceLength==0u) {
        CrtFreePointer(oldText);
        value=StringEmptySentinel();
        return *this;
    } else {
        const unsigned int oldLength=(unsigned int)strlen(oldText);
        if(StringCapacity(oldLength)!=StringCapacity(sourceLength)) {
            CrtFreePointer(oldText);
            value=static_cast<char*>(CrtNew(StringCapacity(sourceLength)));
        }
    }

    strncpy(value,source,sourceLength+1u);
    return *this;
}

const STRING& STRING::operator+=(const STRING& suffix)
{
    const char* sourceText=suffix.value;
    if(sourceText[0]==0) return *this;

    char* const oldText=value;
    const unsigned int oldLength=(unsigned int)strlen(oldText);
    const unsigned int sourceLength=(unsigned int)strlen(sourceText);
    const unsigned int combinedLength=oldLength+sourceLength;

    if(oldText==StringEmptySentinel() ||
       StringCapacity(combinedLength)!=StringCapacity(oldLength)) {
        char* const replacement=static_cast<char*>(CrtNew(StringCapacity(combinedLength)));
        value=replacement;
        strncpy(replacement,oldText,oldLength);
        if(oldText!=StringEmptySentinel()) CrtFreePointer(oldText);
    }

    char* const appendPosition=value+oldLength;
    sourceText=suffix.value;
    strncpy(appendPosition,sourceText,sourceLength+1u);
    return *this;
}

const STRING& STRING::operator+=(const char* suffix)
{
    if(suffix[0]==0) return *this;

    char* const oldText=value;
    const unsigned int oldLength=(unsigned int)strlen(oldText);
    const unsigned int sourceLength=(unsigned int)strlen(suffix);
    const unsigned int combinedLength=oldLength+sourceLength;

    if(oldText==StringEmptySentinel() ||
       StringCapacity(combinedLength)!=StringCapacity(oldLength)) {
        char* const replacement=static_cast<char*>(CrtNew(StringCapacity(combinedLength)));
        value=replacement;
        strncpy(replacement,oldText,oldLength);
        if(oldText!=StringEmptySentinel()) CrtFreePointer(oldText);
    }

    strncpy(value+oldLength,suffix,sourceLength+1u);
    return *this;
}

void STRING::Read(std::FILE* file)
{
    *this = StringGlobals::EmptyText();
    char buffer[256];
    unsigned int bufferUsed = 0u;
    for (;;)
    {
        if (bufferUsed == 255u)
        {
            buffer[255] = 0;
            bufferUsed = 0u;
            *this+=buffer;
        }

        int character = std::fgetc(file);
        if (character<0 || character == '\n') character = 0;
        if (character != '\r') buffer[bufferUsed++] = static_cast<char>(character);
        if (character <= 0) break;
    }
    *this+=buffer;
}

void STRING::Read(STREAM* stream)
{
    unsigned int characterBits = 0u;
    *this = StringGlobals::EmptyText();
    char buffer[256];
    unsigned int bufferUsed = 0u;
    for (;;)
    {
        if (bufferUsed == 255u)
        {
            buffer[255] = 0;
            bufferUsed = 0u;
            *this+=buffer;
        }

        const unsigned int readFailed = static_cast<unsigned int>(stream->Read(&characterBits, 1u));
        if (readFailed || characterBits == '\n') characterBits = 0u;
        const int character = static_cast<int>(characterBits);
        if (character != '\r') buffer[bufferUsed++] = static_cast<char>(character);
        if (character <= 0) break;
    }
    *this+=buffer;
}

extern "C" unsigned int __cdecl MylibStaticCleanupRegister()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

extern "C" unsigned int __cdecl MylibStaticCleanupGuard()
{
    const unsigned char guard = CrtStaticInitializationGuards[0];
    if ((guard&1u) == 0u)
        CrtStaticInitializationGuards[0] = static_cast<unsigned char>(guard|1u);
    return MylibStaticCleanupRegister();
}

void* FFileMapping(const STRING& fileName)
{
    const char* const path = fileName.value;
    const HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == 0)
    {
        if (::Error)
        {
            const char* const failedPath = fileName.value;
            const unsigned int errorCode = GetLastError();
            MYERROR* const logger = ::Error;
            (void)logger->Log("!!!ERROR!!! FFileMapping(%s):Can't CreateFile %u", failedPath, errorCode);
        }
        return 0;
    }

    const HANDLE mapping = CreateFileMappingA(file, 0, PAGE_READONLY, 0, 0, 0);
    (void)CloseHandle(file);
    if (!mapping)
    {
        if (::Error)
        {
            const char* const failedPath = fileName.value;
            const unsigned int errorCode = GetLastError();
            MYERROR* const logger = ::Error;
            (void)logger->Log("!!!ERROR!!! FFileMapping(%s):Can't CreateFileMapping %u", failedPath, errorCode);
        }
        return 0;
    }

    void* const view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    (void)CloseHandle(mapping);
    if (!view)
    {
        if (::Error)
        {
            const char* const failedPath = fileName.value;
            const unsigned int errorCode = GetLastError();
            MYERROR* const logger = ::Error;
            (void)logger->Log("!!!ERROR!!! FFileMapping(%s):Can't MapViewOfFile %u", failedPath, errorCode);
        }
    }
    return view;
}



extern "C" void __cdecl MylibNoOp(int)
{
}

const STRING STRING::SplitRegPath(HKEY__** root) const
{
    struct Prefix
    {
        const char* text;
        HKEY__* rootValue;
    };
    const Prefix prefixes[] = {
        {
            "HKEY_USERS\\", HKEY_USERS
        },
        {"HKEY_CURRENT_USER\\", HKEY_CURRENT_USER},
        {"HKEY_CLASSES_ROOT\\", HKEY_CLASSES_ROOT},
        {"HKEY_CURRENT_CONFIG\\", HKEY_CURRENT_CONFIG},
        {"HKEY_LOCAL_MACHINE\\", HKEY_LOCAL_MACHINE}
    };

    for (unsigned int prefixIndex = 0u;prefixIndex<sizeof(prefixes)/sizeof(prefixes[0]);++prefixIndex)
    {
        const unsigned int prefixLength = static_cast<unsigned int>(strlen(prefixes[prefixIndex].text));
        if (strncmp(value, prefixes[prefixIndex].text, prefixLength) == 0)
        {
            *root = prefixes[prefixIndex].rootValue;
            return After("\\");
        }
    }

    *root = HKEY_LOCAL_MACHINE;
    return STRING(value);
}

void STRING::ToWideChar(unsigned short* output, int outputChars) const
{
    (void)MultiByteToWideChar(CP_ACP, 0, value, -1,
        reinterpret_cast<wchar_t*>(output), outputChars);
}

int STRING::LoadFile(const STRING& fileName)
{
    std::FILE* file = 0;
    {
        const STRING mode("rb");
        if (fileName.value[0] != 0)
            file = std::fopen(fileName.value, mode.value);
    }
    if (!file)
    {
        *this = StringGlobals::EmptyText();
        return 0;
    }

    unsigned int length;
    bool hasPayload;
    try
    {
        length = static_cast<unsigned int>(_filelength(_fileno(file)));
        if (value != StringEmptySentinel())
            CrtFreePointer(value);

        value = static_cast<char*>(CrtNew(StringCapacity(length)));
        if (!value)
        {
            value = StringEmptySentinel();
            hasPayload = false;
        }
        else
        {
            (void)std::fread(value, 1u, length, file);
            value[length] = 0;
            hasPayload = true;
        }
    }
    catch (...)
    {
        (void)std::fclose(file);
        throw;
    }
    (void)std::fclose(file);
    return hasPayload ? BitsToSigned32(length) : 0;
}

int STRING::Replace(const char* oldText, const char* newText)
{
    const unsigned int oldLength = static_cast<unsigned int>(strlen(oldText));
    const unsigned int newLength = static_cast<unsigned int>(strlen(newText));
    char* const match = strstr(value, oldText);
    if (!match) return 0;

    if (BitsToSigned32(oldLength) >= BitsToSigned32(newLength))
    {
        strncpy(match, newText, newLength);
        const unsigned int suffixLength = static_cast<unsigned int>(strlen(match))-oldLength+1u;
        memmove(match+newLength, match+oldLength, suffixLength);
        return 1;
    }

    char* const oldValue = value;
    const unsigned int prefixLength = static_cast<unsigned int>(match-oldValue);
    const unsigned int currentLength = static_cast<unsigned int>(strlen(oldValue));
    const unsigned int finalLength = currentLength+newLength-oldLength;
    value = static_cast<char*>(CrtNew(finalLength+1u));
    strncpy(value, oldValue, prefixLength);
    strncpy(value+prefixLength, newText, newLength);
    const unsigned int suffixLength = static_cast<unsigned int>(strlen(match))-oldLength+1u;
    strncpy(value+prefixLength+newLength, match+oldLength, suffixLength);
    if (oldValue != StringEmptySentinel()) CrtFreePointer(oldValue);
    return 1;
}

void STRING::RemoveEndChars(const char* chars)
{
    char* const first = value;
    if (first[0] == 0) return;

    char* last = first + strlen(first) - 1u;
    for (;;)
    {
        if (strchr(chars, static_cast<int>(static_cast<signed char>(*last))) == 0)
            return;
        *last = 0;
        if (last == value) return;
        --last;
    }
}

void STRING::RemoveBeginChars(const char* chars)
{
    const unsigned int removeCount = static_cast<unsigned int>(strspn(value, chars));
    if (removeCount != 0u)
    {
        char* const text = value;
        char* const begin = text+removeCount;
        const unsigned int remaining = static_cast<unsigned int>(strlen(begin))+1u;
        memmove(text, begin, remaining);
    }
}

int STRING::CountChar(int needle) const
{
    const signed char* cursor = reinterpret_cast<const signed char*>(value);
    int count = 0;
    while (*cursor != 0)
    {
        if (static_cast<int>(*cursor) == needle) ++count;
        ++cursor;
    }
    return count;
}

const STRING STRING::Before(const char* delimiter) const
{
    const char* found = strstr(value, delimiter);
    const char* const sourceText = value;
    const unsigned int length = found
        ? static_cast<unsigned int>(found-sourceText)
        : (sourceText[0] != 0 ? static_cast<unsigned int>(strlen(sourceText)) : 0u);
    char* const copy = length != 0u
        ? static_cast<char*>(CrtNew(StringCapacity(length)))
        : StringEmptySentinel();
    STRING result;
    result.value = copy;
    if (length != 0u)
    {
        memcpy(copy, sourceText, length);
        result.value[length] = 0;
    }
    return result;
}

const STRING STRING::After(const char* delimiter) const
{
    const char* const found = strstr(value, delimiter);
    const char* const begin = found
        ? found + strlen(delimiter)
        : StringGlobals::EmptyText();
    return STRING(begin);
}

const STRING STRING::BeforeLast(const char* delimiter) const
{
    const char* last = 0;
    const char* found = strstr(value, delimiter);
    while (found)
    {
        last = found;
        found = strstr(found+1, delimiter);
    }
    const char* const sourceText = value;
    const unsigned int length = last
        ? static_cast<unsigned int>(last-sourceText)
        : (sourceText[0] != 0 ? static_cast<unsigned int>(strlen(sourceText)) : 0u);
    char* const copy = length != 0u
        ? static_cast<char*>(CrtNew(StringCapacity(length)))
        : StringEmptySentinel();
    STRING result;
    result.value = copy;
    if (length != 0u)
    {
        memcpy(copy, sourceText, length);
        result.value[length] = 0;
    }
    return result;
}

const STRING STRING::AfterLast(const char* delimiter) const
{
    const char* last = 0;
    const char* found = strstr(value, delimiter);
    while (found)
    {
        last = found;
        found = strstr(found+1, delimiter);
    }
    const char* const begin = last
        ? last + strlen(delimiter)
        : StringGlobals::EmptyText();
    return STRING(begin);
}

const STRING STRING::Add(int delta) const
{
    const char* const sourceText = value;
    if (sourceText[0] == 0)
        return STRING(StringGlobals::EmptyText());

    const int lastIndex = BitsToSigned32(static_cast<unsigned int>(strlen(sourceText))-1u);
    const STRING working(sourceText);
    if (delta == 0) return STRING(working.value);

    int digitEnd = SubSigned32Wrap(lastIndex, 1);
    while (digitEnd >= 0 && CrtIsDigit(static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(value[digitEnd])))) == 0u)
        --digitEnd;
    if (digitEnd<0)
        return STRING(StringGlobals::EmptyText());

    int digitBegin = SubSigned32Wrap(digitEnd, 1);
    while (digitBegin >= 0 && CrtIsDigit(static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(value[digitBegin])))) != 0u)
        --digitBegin;
    if (digitBegin<0 || value[digitBegin] != '-') ++digitBegin;

    const int width = AddSigned32Wrap(SubSigned32Wrap(digitEnd, digitBegin), 1);
    const int number = BitsToSigned32(CrtAtoiText(value+digitBegin)+static_cast<unsigned int>(delta));
    char numberText[128];
    std::sprintf(numberText, "%0*i", width, number);
    strncpy(working.value+digitBegin, numberText, static_cast<unsigned int>(width));
    return STRING(working.value);
}

STRING STRING::ToBase64(int addCode) const
{
    char alphabet[65];
    memcpy(alphabet, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", sizeof(alphabet));
    const unsigned int length = static_cast<unsigned int>(strlen(value));
    STRING encoded;

    unsigned int sourceOffset = 0u;
    unsigned int encodedLength = 0u;
    while (sourceOffset<length)
    {
        const unsigned int chunkLength = (length-sourceOffset<57u)?(length-sourceOffset):57u;
        unsigned char chunk[57];
        memcpy(chunk, value+sourceOffset, chunkLength);
        memset(chunk+chunkLength, 0, 57u-chunkLength);

        const unsigned char key = static_cast<unsigned char>(addCode);
        for (unsigned int byteIndex = 0u;byteIndex<chunkLength;++byteIndex)
            chunk[byteIndex] = static_cast<unsigned char>(chunk[byteIndex]+key);

        char output[77u];
        unsigned int inputIndex = 0u;
        unsigned int outputIndex = 0u;
        while (inputIndex<chunkLength)
        {
            unsigned int packed = (static_cast<unsigned int>(chunk[inputIndex])<<16) |
                (static_cast<unsigned int>(chunk[inputIndex+1u])<<8) |
                static_cast<unsigned int>(chunk[inputIndex+2u]);
            output[outputIndex+3u] = alphabet[packed&0x3Fu];
            packed >>= 6;
            output[outputIndex+2u] = alphabet[packed&0x3Fu];
            packed >>= 6;
            output[outputIndex+1u] = alphabet[packed&0x3Fu];
            packed >>= 6;
            output[outputIndex+0u] = alphabet[packed&0x3Fu];
            inputIndex += 3u;
            outputIndex += 4u;
        }
        output[outputIndex] = 0;
        encodedLength += outputIndex;
        encoded += output;
        sourceOffset += 57u;
    }

    const unsigned int remainder = length%3u;
    const unsigned int padding = (3u-remainder)%3u;
    for (unsigned int byteIndex = 0u;byteIndex<padding;++byteIndex)
        encoded.value[encodedLength-1u-byteIndex] = '=';

    return STRING(encoded);
}

const STRING Printf(const char* format, ...)
{
    char buffer[4096];
    buffer[0] = *StringGlobals::EmptyText();
    for (unsigned int byteIndex = 1u;byteIndex<4096u;++byteIndex) buffer[byteIndex] = 0;
    va_list arguments;
    va_start(arguments, format);
    vsprintf(buffer, format, arguments);
    va_end(arguments);
    return STRING(buffer);
}

const STRING GetDateString()
{
    __time32_t timeValue = 0;
    char buffer[256];
    (void)_time32(&timeValue);
    const std::tm* const localTime = _localtime32(&timeValue);
    (void)std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", localTime);
    return STRING(buffer);
}

const STRING GetTimeString()
{
    __time32_t timeValue = 0;
    char buffer[256];
    (void)_time32(&timeValue);
    const std::tm* const localTime = _localtime32(&timeValue);
    (void)std::strftime(buffer, sizeof(buffer), "%H:%M:%S", localTime);
    return STRING(buffer);
}

STRING FFirstFile(int* findHandle, const STRING& fileMask, _FILETIME* lastWriteTime)
{
    WIN32_FIND_DATAA findData;
    const HANDLE handle = FindFirstFileA(fileMask.value, &findData);
    *findHandle = static_cast<int>(reinterpret_cast<INT_PTR>(handle));
    if (handle == INVALID_HANDLE_VALUE)
        return STRING(StringGlobals::EmptyText());
    if (lastWriteTime) *lastWriteTime = findData.ftLastWriteTime;
    return STRING(findData.cFileName);
}

STRING FNextFile(int* findHandle, _FILETIME* lastWriteTime)
{
    WIN32_FIND_DATAA findData;
    const HANDLE handle = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(*findHandle));
    if (FindNextFileA(handle, &findData) == 0)
    {
        (void)FindClose(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(*findHandle)));
        return STRING(StringGlobals::EmptyText());
    }
    if (lastWriteTime) *lastWriteTime = findData.ftLastWriteTime;
    return STRING(findData.cFileName);
}

int FFileLength(const STRING& fileName)
{
    std::FILE* file = 0;
    {
        const STRING mode("rb");
        if (fileName.value[0] != 0)
            file = std::fopen(fileName.value, mode.value);
    }
    if (!file) return 0;
    const int length = BitsToSigned32(static_cast<unsigned int>(_filelength(_fileno(file))));
    (void)std::fclose(file);
    return length;
}

int FGetFileTime(STRING fileName, _FILETIME* creationTime, _FILETIME* accessTime, _FILETIME* writeTime)
{
    const HANDLE handle = CreateFileA(fileName.value, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (handle == INVALID_HANDLE_VALUE) return 1;
    (void)GetFileTime(handle,
        reinterpret_cast<FILETIME*>(creationTime),
        reinterpret_cast<FILETIME*>(accessTime),
        reinterpret_cast<FILETIME*>(writeTime));
    (void)CloseHandle(handle);
    return 0;
}

int FSetFileTime(STRING fileName, _FILETIME* creationTime, _FILETIME* accessTime, _FILETIME* writeTime)
{
    const HANDLE handle = CreateFileA(fileName.value, GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (handle == INVALID_HANDLE_VALUE) return 1;
    (void)SetFileTime(handle,
        reinterpret_cast<const FILETIME*>(creationTime),
        reinterpret_cast<const FILETIME*>(accessTime),
        reinterpret_cast<const FILETIME*>(writeTime));
    (void)CloseHandle(handle);
    return 0;
}

int Random(int maximum)
{
    if (maximum == -1) return 0;
    const int randomValue = std::rand();
    const int divisor = BitsToSigned32(static_cast<unsigned int>(maximum)+1u);
    return randomValue%divisor;
}

float Random(float scale)
{
    const int randomValue = std::rand();
    return static_cast<float>(randomValue)*scale*
        3.051851e-05f;
}

namespace {
int random2Seed = 0;
unsigned char random2InitializationState = 0;
unsigned int startupQuarterTick = 0;
}

static int NextRandom2Value()
{
    volatile int& storedSeed = random2Seed;
    int seed;
    const unsigned char guard = random2InitializationState;
    if ((guard&1u) == 0u)
    {
        random2InitializationState = static_cast<unsigned char>(guard|1u);
        seed = BitsToSigned32(timeGetTime()&0x3FFFFFFFu);
    }
    else
    {
        seed = storedSeed;
    }

    const int quotient = seed/44488;
    const int remainder = seed%44488;
    int next = 48271*remainder-3399*quotient;
    storedSeed = next;
    if (next<0)
    {
        next = AddSigned32Wrap(next, 2147483647);
        storedSeed = next;
    }
    return next;
}

void __cdecl RefreshQuarterTickAtStartup()
{
    startupQuarterTick = ::timeGetTime() >> 2;
}

float ANGLE::SinTable[256] =
{
    0.0f, 0.024541229f, 0.0490676723f, 0.0735645667f, 0.0980171412f, 0.122410677f, 0.146730468f, 0.170961887f,
    0.195090324f, 0.219101235f, 0.242980182f, 0.266712755f, 0.290284663f, 0.313681751f, 0.336889863f, 0.359895051f,
    0.382683426f, 0.405241311f, 0.427555084f, 0.449611336f, 0.471396744f, 0.492898196f, 0.514102757f, 0.534997642f,
    0.555570245f, 0.575808167f, 0.59569931f, 0.615231574f, 0.634393275f, 0.653172851f, 0.671558976f, 0.689540565f,
    0.707106769f, 0.724247098f, 0.740951121f, 0.757208824f, 0.773010433f, 0.78834641f, 0.803207517f, 0.817584813f,
    0.831469595f, 0.84485358f, 0.857728601f, 0.870086968f, 0.881921291f, 0.893224299f, 0.903989315f, 0.914209783f,
    0.923879504f, 0.932992816f, 0.941544056f, 0.949528158f, 0.956940353f, 0.963776052f, 0.970031261f, 0.975702107f,
    0.980785251f, 0.985277653f, 0.989176512f, 0.992479563f, 0.99518472f, 0.997290432f, 0.99879545f, 0.999698818f,
    1.0f, 0.999698818f, 0.99879545f, 0.997290432f, 0.99518472f, 0.992479563f, 0.989176512f, 0.985277653f,
    0.980785251f, 0.975702107f, 0.970031261f, 0.963776052f, 0.956940353f, 0.949528158f, 0.941544056f, 0.932992816f,
    0.923879504f, 0.914209783f, 0.903989315f, 0.893224299f, 0.881921291f, 0.870086968f, 0.857728601f, 0.84485358f,
    0.831469595f, 0.817584813f, 0.803207517f, 0.78834641f, 0.773010433f, 0.757208824f, 0.740951121f, 0.724247098f,
    0.707106769f, 0.689540565f, 0.671558976f, 0.653172851f, 0.634393275f, 0.615231574f, 0.59569931f, 0.575808167f,
    0.555570245f, 0.534997642f, 0.514102757f, 0.492898196f, 0.471396744f, 0.449611336f, 0.427555084f, 0.405241311f,
    0.382683426f, 0.359895051f, 0.336889863f, 0.313681751f, 0.290284663f, 0.266712755f, 0.242980182f, 0.219101235f,
    0.195090324f, 0.170961887f, 0.146730468f, 0.122410677f, 0.0980171412f, 0.0735645667f, 0.0490676723f, 0.024541229f,
    0.0f, -0.0245412271f, -0.0490676723f, -0.0735645667f, -0.0980171412f, -0.122410677f, -0.146730468f, -0.170961887f,
    -0.195090324f, -0.219101235f, -0.242980182f, -0.266712755f, -0.290284663f, -0.313681751f, -0.336889863f, -0.359895051f,
    -0.382683426f, -0.405241311f, -0.427555084f, -0.449611336f, -0.471396744f, -0.492898196f, -0.514102757f, -0.534997642f,
    -0.555570245f, -0.575808167f, -0.59569931f, -0.615231574f, -0.634393275f, -0.653172851f, -0.671558976f, -0.689540565f,
    -0.707106769f, -0.724247098f, -0.740951121f, -0.757208824f, -0.773010433f, -0.78834641f, -0.803207517f, -0.817584813f,
    -0.831469595f, -0.84485358f, -0.857728601f, -0.870086968f, -0.881921291f, -0.893224299f, -0.903989315f, -0.914209783f,
    -0.923879504f, -0.932992816f, -0.941544056f, -0.949528158f, -0.956940353f, -0.963776052f, -0.970031261f, -0.975702107f,
    -0.980785251f, -0.985277653f, -0.989176512f, -0.992479563f, -0.99518472f, -0.997290432f, -0.99879545f, -0.999698818f,
    -1.0f, -0.999698818f, -0.99879545f, -0.997290432f, -0.99518472f, -0.992479563f, -0.989176512f, -0.985277653f,
    -0.980785251f, -0.975702107f, -0.970031261f, -0.963776052f, -0.956940353f, -0.949528158f, -0.941544056f, -0.932992816f,
    -0.923879504f, -0.914209783f, -0.903989315f, -0.893224299f, -0.881921291f, -0.870086968f, -0.857728601f, -0.84485358f,
    -0.831469595f, -0.817584813f, -0.803207517f, -0.78834641f, -0.773010433f, -0.757208824f, -0.740951121f, -0.724247098f,
    -0.707106769f, -0.689540565f, -0.671558976f, -0.653172851f, -0.634393275f, -0.615231574f, -0.59569931f, -0.575808167f,
    -0.555570245f, -0.534997642f, -0.514102757f, -0.492898196f, -0.471396744f, -0.449611336f, -0.427555084f, -0.405241311f,
    -0.382683426f, -0.359895051f, -0.336889863f, -0.313681751f, -0.290284663f, -0.266712755f, -0.242980182f, -0.219101235f,
    -0.195090324f, -0.170961887f, -0.146730468f, -0.122410677f, -0.0980171412f, -0.0735645667f, -0.0490676761f, -0.024541229f
};

float ANGLE::CosTable[256] =
{
    1.0f, 0.999698818f, 0.99879545f, 0.997290432f, 0.99518472f, 0.992479563f, 0.989176512f, 0.985277653f,
    0.980785251f, 0.975702107f, 0.970031261f, 0.963776052f, 0.956940353f, 0.949528158f, 0.941544056f, 0.932992816f,
    0.923879504f, 0.914209783f, 0.903989315f, 0.893224299f, 0.881921291f, 0.870086968f, 0.857728601f, 0.84485358f,
    0.831469595f, 0.817584813f, 0.803207517f, 0.78834641f, 0.773010433f, 0.757208824f, 0.740951121f, 0.724247098f,
    0.707106769f, 0.689540565f, 0.671558976f, 0.653172851f, 0.634393275f, 0.615231574f, 0.59569931f, 0.575808167f,
    0.555570245f, 0.534997642f, 0.514102757f, 0.492898196f, 0.471396744f, 0.449611336f, 0.427555084f, 0.405241311f,
    0.382683426f, 0.359895051f, 0.336889863f, 0.313681751f, 0.290284663f, 0.266712755f, 0.242980182f, 0.219101235f,
    0.195090324f, 0.170961887f, 0.146730468f, 0.122410677f, 0.0980171412f, 0.0735645667f, 0.0490676723f, 0.024541229f,
    0.0f, -0.0245412271f, -0.0490676723f, -0.0735645667f, -0.0980171412f, -0.122410677f, -0.146730468f, -0.170961887f,
    -0.195090324f, -0.219101235f, -0.242980182f, -0.266712755f, -0.290284663f, -0.313681751f, -0.336889863f, -0.359895051f,
    -0.382683426f, -0.405241311f, -0.427555084f, -0.449611336f, -0.471396744f, -0.492898196f, -0.514102757f, -0.534997642f,
    -0.555570245f, -0.575808167f, -0.59569931f, -0.615231574f, -0.634393275f, -0.653172851f, -0.671558976f, -0.689540565f,
    -0.707106769f, -0.724247098f, -0.740951121f, -0.757208824f, -0.773010433f, -0.78834641f, -0.803207517f, -0.817584813f,
    -0.831469595f, -0.84485358f, -0.857728601f, -0.870086968f, -0.881921291f, -0.893224299f, -0.903989315f, -0.914209783f,
    -0.923879504f, -0.932992816f, -0.941544056f, -0.949528158f, -0.956940353f, -0.963776052f, -0.970031261f, -0.975702107f,
    -0.980785251f, -0.985277653f, -0.989176512f, -0.992479563f, -0.99518472f, -0.997290432f, -0.99879545f, -0.999698818f,
    -1.0f, -0.999698818f, -0.99879545f, -0.997290432f, -0.99518472f, -0.992479563f, -0.989176512f, -0.985277653f,
    -0.980785251f, -0.975702107f, -0.970031261f, -0.963776052f, -0.956940353f, -0.949528158f, -0.941544056f, -0.932992816f,
    -0.923879504f, -0.914209783f, -0.903989315f, -0.893224299f, -0.881921291f, -0.870086968f, -0.857728601f, -0.84485358f,
    -0.831469595f, -0.817584813f, -0.803207517f, -0.78834641f, -0.773010433f, -0.757208824f, -0.740951121f, -0.724247098f,
    -0.707106769f, -0.689540565f, -0.671558976f, -0.653172851f, -0.634393275f, -0.615231574f, -0.59569931f, -0.575808167f,
    -0.555570245f, -0.534997642f, -0.514102757f, -0.492898196f, -0.471396744f, -0.449611336f, -0.427555084f, -0.405241311f,
    -0.382683426f, -0.359895051f, -0.336889863f, -0.313681751f, -0.290284663f, -0.266712755f, -0.242980182f, -0.219101235f,
    -0.195090324f, -0.170961887f, -0.146730468f, -0.122410677f, -0.0980171412f, -0.0735645667f, -0.0490676723f, -0.024541229f,
    0.0f, 0.0245412271f, 0.0490676723f, 0.0735645667f, 0.0980171412f, 0.122410677f, 0.146730468f, 0.170961887f,
    0.195090324f, 0.219101235f, 0.242980182f, 0.266712755f, 0.290284663f, 0.313681751f, 0.336889863f, 0.359895051f,
    0.382683426f, 0.405241311f, 0.427555084f, 0.449611336f, 0.471396744f, 0.492898196f, 0.514102757f, 0.534997642f,
    0.555570245f, 0.575808167f, 0.59569931f, 0.615231574f, 0.634393275f, 0.653172851f, 0.671558976f, 0.689540565f,
    0.707106769f, 0.724247098f, 0.740951121f, 0.757208824f, 0.773010433f, 0.78834641f, 0.803207517f, 0.817584813f,
    0.831469595f, 0.84485358f, 0.857728601f, 0.870086968f, 0.881921291f, 0.893224299f, 0.903989315f, 0.914209783f,
    0.923879504f, 0.932992816f, 0.941544056f, 0.949528158f, 0.956940353f, 0.963776052f, 0.970031261f, 0.975702107f,
    0.980785251f, 0.985277653f, 0.989176512f, 0.992479563f, 0.99518472f, 0.997290432f, 0.99879545f, 0.999698818f
};

float ANGLE::SinYTable[256] =
{
    0.0f, 0.024541229f, 0.0490676723f, 0.0735645667f, 0.0980171412f, 0.122410677f, 0.146730468f, 0.170961887f,
    0.195090324f, 0.219101235f, 0.242980182f, 0.266712755f, 0.290284663f, 0.313681751f, 0.336889863f, 0.359895051f,
    0.382683426f, 0.405241311f, 0.427555084f, 0.449611336f, 0.471396744f, 0.492898196f, 0.514102757f, 0.534997642f,
    0.555570245f, 0.575808167f, 0.59569931f, 0.615231574f, 0.634393275f, 0.653172851f, 0.671558976f, 0.689540565f,
    0.707106769f, 0.724247098f, 0.740951121f, 0.757208824f, 0.773010433f, 0.78834641f, 0.803207517f, 0.817584813f,
    0.831469595f, 0.84485358f, 0.857728601f, 0.870086968f, 0.881921291f, 0.893224299f, 0.903989315f, 0.914209783f,
    0.923879504f, 0.932992816f, 0.941544056f, 0.949528158f, 0.956940353f, 0.963776052f, 0.970031261f, 0.975702107f,
    0.980785251f, 0.985277653f, 0.989176512f, 0.992479563f, 0.99518472f, 0.997290432f, 0.99879545f, 0.999698818f,
    1.0f, 0.999698818f, 0.99879545f, 0.997290432f, 0.99518472f, 0.992479563f, 0.989176512f, 0.985277653f,
    0.980785251f, 0.975702107f, 0.970031261f, 0.963776052f, 0.956940353f, 0.949528158f, 0.941544056f, 0.932992816f,
    0.923879504f, 0.914209783f, 0.903989315f, 0.893224299f, 0.881921291f, 0.870086968f, 0.857728601f, 0.84485358f,
    0.831469595f, 0.817584813f, 0.803207517f, 0.78834641f, 0.773010433f, 0.757208824f, 0.740951121f, 0.724247098f,
    0.707106769f, 0.689540565f, 0.671558976f, 0.653172851f, 0.634393275f, 0.615231574f, 0.59569931f, 0.575808167f,
    0.555570245f, 0.534997642f, 0.514102757f, 0.492898196f, 0.471396744f, 0.449611336f, 0.427555084f, 0.405241311f,
    0.382683426f, 0.359895051f, 0.336889863f, 0.313681751f, 0.290284663f, 0.266712755f, 0.242980182f, 0.219101235f,
    0.195090324f, 0.170961887f, 0.146730468f, 0.122410677f, 0.0980171412f, 0.0735645667f, 0.0490676723f, 0.024541229f,
    0.0f, -0.0245412271f, -0.0490676723f, -0.0735645667f, -0.0980171412f, -0.122410677f, -0.146730468f, -0.170961887f,
    -0.195090324f, -0.219101235f, -0.242980182f, -0.266712755f, -0.290284663f, -0.313681751f, -0.336889863f, -0.359895051f,
    -0.382683426f, -0.405241311f, -0.427555084f, -0.449611336f, -0.471396744f, -0.492898196f, -0.514102757f, -0.534997642f,
    -0.555570245f, -0.575808167f, -0.59569931f, -0.615231574f, -0.634393275f, -0.653172851f, -0.671558976f, -0.689540565f,
    -0.707106769f, -0.724247098f, -0.740951121f, -0.757208824f, -0.773010433f, -0.78834641f, -0.803207517f, -0.817584813f,
    -0.831469595f, -0.84485358f, -0.857728601f, -0.870086968f, -0.881921291f, -0.893224299f, -0.903989315f, -0.914209783f,
    -0.923879504f, -0.932992816f, -0.941544056f, -0.949528158f, -0.956940353f, -0.963776052f, -0.970031261f, -0.975702107f,
    -0.980785251f, -0.985277653f, -0.989176512f, -0.992479563f, -0.99518472f, -0.997290432f, -0.99879545f, -0.999698818f,
    -1.0f, -0.999698818f, -0.99879545f, -0.997290432f, -0.99518472f, -0.992479563f, -0.989176512f, -0.985277653f,
    -0.980785251f, -0.975702107f, -0.970031261f, -0.963776052f, -0.956940353f, -0.949528158f, -0.941544056f, -0.932992816f,
    -0.923879504f, -0.914209783f, -0.903989315f, -0.893224299f, -0.881921291f, -0.870086968f, -0.857728601f, -0.84485358f,
    -0.831469595f, -0.817584813f, -0.803207517f, -0.78834641f, -0.773010433f, -0.757208824f, -0.740951121f, -0.724247098f,
    -0.707106769f, -0.689540565f, -0.671558976f, -0.653172851f, -0.634393275f, -0.615231574f, -0.59569931f, -0.575808167f,
    -0.555570245f, -0.534997642f, -0.514102757f, -0.492898196f, -0.471396744f, -0.449611336f, -0.427555084f, -0.405241311f,
    -0.382683426f, -0.359895051f, -0.336889863f, -0.313681751f, -0.290284663f, -0.266712755f, -0.242980182f, -0.219101235f,
    -0.195090324f, -0.170961887f, -0.146730468f, -0.122410677f, -0.0980171412f, -0.0735645667f, -0.0490676761f, -0.024541229f
};

float ANGLE::CosYTable[256] =
{
    1.0f, 0.999698818f, 0.99879545f, 0.997290432f, 0.99518472f, 0.992479563f, 0.989176512f, 0.985277653f,
    0.980785251f, 0.975702107f, 0.970031261f, 0.963776052f, 0.956940353f, 0.949528158f, 0.941544056f, 0.932992816f,
    0.923879504f, 0.914209783f, 0.903989315f, 0.893224299f, 0.881921291f, 0.870086968f, 0.857728601f, 0.84485358f,
    0.831469595f, 0.817584813f, 0.803207517f, 0.78834641f, 0.773010433f, 0.757208824f, 0.740951121f, 0.724247098f,
    0.707106769f, 0.689540565f, 0.671558976f, 0.653172851f, 0.634393275f, 0.615231574f, 0.59569931f, 0.575808167f,
    0.555570245f, 0.534997642f, 0.514102757f, 0.492898196f, 0.471396744f, 0.449611336f, 0.427555084f, 0.405241311f,
    0.382683426f, 0.359895051f, 0.336889863f, 0.313681751f, 0.290284663f, 0.266712755f, 0.242980182f, 0.219101235f,
    0.195090324f, 0.170961887f, 0.146730468f, 0.122410677f, 0.0980171412f, 0.0735645667f, 0.0490676723f, 0.024541229f,
    0.0f, -0.0245412271f, -0.0490676723f, -0.0735645667f, -0.0980171412f, -0.122410677f, -0.146730468f, -0.170961887f,
    -0.195090324f, -0.219101235f, -0.242980182f, -0.266712755f, -0.290284663f, -0.313681751f, -0.336889863f, -0.359895051f,
    -0.382683426f, -0.405241311f, -0.427555084f, -0.449611336f, -0.471396744f, -0.492898196f, -0.514102757f, -0.534997642f,
    -0.555570245f, -0.575808167f, -0.59569931f, -0.615231574f, -0.634393275f, -0.653172851f, -0.671558976f, -0.689540565f,
    -0.707106769f, -0.724247098f, -0.740951121f, -0.757208824f, -0.773010433f, -0.78834641f, -0.803207517f, -0.817584813f,
    -0.831469595f, -0.84485358f, -0.857728601f, -0.870086968f, -0.881921291f, -0.893224299f, -0.903989315f, -0.914209783f,
    -0.923879504f, -0.932992816f, -0.941544056f, -0.949528158f, -0.956940353f, -0.963776052f, -0.970031261f, -0.975702107f,
    -0.980785251f, -0.985277653f, -0.989176512f, -0.992479563f, -0.99518472f, -0.997290432f, -0.99879545f, -0.999698818f,
    -1.0f, -0.999698818f, -0.99879545f, -0.997290432f, -0.99518472f, -0.992479563f, -0.989176512f, -0.985277653f,
    -0.980785251f, -0.975702107f, -0.970031261f, -0.963776052f, -0.956940353f, -0.949528158f, -0.941544056f, -0.932992816f,
    -0.923879504f, -0.914209783f, -0.903989315f, -0.893224299f, -0.881921291f, -0.870086968f, -0.857728601f, -0.84485358f,
    -0.831469595f, -0.817584813f, -0.803207517f, -0.78834641f, -0.773010433f, -0.757208824f, -0.740951121f, -0.724247098f,
    -0.707106769f, -0.689540565f, -0.671558976f, -0.653172851f, -0.634393275f, -0.615231574f, -0.59569931f, -0.575808167f,
    -0.555570245f, -0.534997642f, -0.514102757f, -0.492898196f, -0.471396744f, -0.449611336f, -0.427555084f, -0.405241311f,
    -0.382683426f, -0.359895051f, -0.336889863f, -0.313681751f, -0.290284663f, -0.266712755f, -0.242980182f, -0.219101235f,
    -0.195090324f, -0.170961887f, -0.146730468f, -0.122410677f, -0.0980171412f, -0.0735645667f, -0.0490676723f, -0.024541229f,
    0.0f, 0.0245412271f, 0.0490676723f, 0.0735645667f, 0.0980171412f, 0.122410677f, 0.146730468f, 0.170961887f,
    0.195090324f, 0.219101235f, 0.242980182f, 0.266712755f, 0.290284663f, 0.313681751f, 0.336889863f, 0.359895051f,
    0.382683426f, 0.405241311f, 0.427555084f, 0.449611336f, 0.471396744f, 0.492898196f, 0.514102757f, 0.534997642f,
    0.555570245f, 0.575808167f, 0.59569931f, 0.615231574f, 0.634393275f, 0.653172851f, 0.671558976f, 0.689540565f,
    0.707106769f, 0.724247098f, 0.740951121f, 0.757208824f, 0.773010433f, 0.78834641f, 0.803207517f, 0.817584813f,
    0.831469595f, 0.84485358f, 0.857728601f, 0.870086968f, 0.881921291f, 0.893224299f, 0.903989315f, 0.914209783f,
    0.923879504f, 0.932992816f, 0.941544056f, 0.949528158f, 0.956940353f, 0.963776052f, 0.970031261f, 0.975702107f,
    0.980785251f, 0.985277653f, 0.989176512f, 0.992479563f, 0.99518472f, 0.997290432f, 0.99879545f, 0.999698818f
};

float FSin[256] =
{
    0.0f, 0.024541229f, 0.0490676723f, 0.0735645667f, 0.0980171412f, 0.122410677f, 0.146730468f, 0.170961887f,
    0.195090324f, 0.219101235f, 0.242980182f, 0.266712755f, 0.290284663f, 0.313681751f, 0.336889863f, 0.359895051f,
    0.382683426f, 0.405241311f, 0.427555084f, 0.449611336f, 0.471396744f, 0.492898196f, 0.514102757f, 0.534997642f,
    0.555570245f, 0.575808167f, 0.59569931f, 0.615231574f, 0.634393275f, 0.653172851f, 0.671558976f, 0.689540565f,
    0.707106769f, 0.724247098f, 0.740951121f, 0.757208824f, 0.773010433f, 0.78834641f, 0.803207517f, 0.817584813f,
    0.831469595f, 0.84485358f, 0.857728601f, 0.870086968f, 0.881921291f, 0.893224299f, 0.903989315f, 0.914209783f,
    0.923879504f, 0.932992816f, 0.941544056f, 0.949528158f, 0.956940353f, 0.963776052f, 0.970031261f, 0.975702107f,
    0.980785251f, 0.985277653f, 0.989176512f, 0.992479563f, 0.99518472f, 0.997290432f, 0.99879545f, 0.999698818f,
    1.0f, 0.999698818f, 0.99879545f, 0.997290432f, 0.99518472f, 0.992479563f, 0.989176512f, 0.985277653f,
    0.980785251f, 0.975702107f, 0.970031261f, 0.963776052f, 0.956940353f, 0.949528158f, 0.941544056f, 0.932992816f,
    0.923879504f, 0.914209783f, 0.903989315f, 0.893224299f, 0.881921291f, 0.870086968f, 0.857728601f, 0.84485358f,
    0.831469595f, 0.817584813f, 0.803207517f, 0.78834641f, 0.773010433f, 0.757208824f, 0.740951121f, 0.724247098f,
    0.707106769f, 0.689540565f, 0.671558976f, 0.653172851f, 0.634393275f, 0.615231574f, 0.59569931f, 0.575808167f,
    0.555570245f, 0.534997642f, 0.514102757f, 0.492898196f, 0.471396744f, 0.449611336f, 0.427555084f, 0.405241311f,
    0.382683426f, 0.359895051f, 0.336889863f, 0.313681751f, 0.290284663f, 0.266712755f, 0.242980182f, 0.219101235f,
    0.195090324f, 0.170961887f, 0.146730468f, 0.122410677f, 0.0980171412f, 0.0735645667f, 0.0490676723f, 0.024541229f,
    0.0f, -0.0245412271f, -0.0490676723f, -0.0735645667f, -0.0980171412f, -0.122410677f, -0.146730468f, -0.170961887f,
    -0.195090324f, -0.219101235f, -0.242980182f, -0.266712755f, -0.290284663f, -0.313681751f, -0.336889863f, -0.359895051f,
    -0.382683426f, -0.405241311f, -0.427555084f, -0.449611336f, -0.471396744f, -0.492898196f, -0.514102757f, -0.534997642f,
    -0.555570245f, -0.575808167f, -0.59569931f, -0.615231574f, -0.634393275f, -0.653172851f, -0.671558976f, -0.689540565f,
    -0.707106769f, -0.724247098f, -0.740951121f, -0.757208824f, -0.773010433f, -0.78834641f, -0.803207517f, -0.817584813f,
    -0.831469595f, -0.84485358f, -0.857728601f, -0.870086968f, -0.881921291f, -0.893224299f, -0.903989315f, -0.914209783f,
    -0.923879504f, -0.932992816f, -0.941544056f, -0.949528158f, -0.956940353f, -0.963776052f, -0.970031261f, -0.975702107f,
    -0.980785251f, -0.985277653f, -0.989176512f, -0.992479563f, -0.99518472f, -0.997290432f, -0.99879545f, -0.999698818f,
    -1.0f, -0.999698818f, -0.99879545f, -0.997290432f, -0.99518472f, -0.992479563f, -0.989176512f, -0.985277653f,
    -0.980785251f, -0.975702107f, -0.970031261f, -0.963776052f, -0.956940353f, -0.949528158f, -0.941544056f, -0.932992816f,
    -0.923879504f, -0.914209783f, -0.903989315f, -0.893224299f, -0.881921291f, -0.870086968f, -0.857728601f, -0.84485358f,
    -0.831469595f, -0.817584813f, -0.803207517f, -0.78834641f, -0.773010433f, -0.757208824f, -0.740951121f, -0.724247098f,
    -0.707106769f, -0.689540565f, -0.671558976f, -0.653172851f, -0.634393275f, -0.615231574f, -0.59569931f, -0.575808167f,
    -0.555570245f, -0.534997642f, -0.514102757f, -0.492898196f, -0.471396744f, -0.449611336f, -0.427555084f, -0.405241311f,
    -0.382683426f, -0.359895051f, -0.336889863f, -0.313681751f, -0.290284663f, -0.266712755f, -0.242980182f, -0.219101235f,
    -0.195090324f, -0.170961887f, -0.146730468f, -0.122410677f, -0.0980171412f, -0.0735645667f, -0.0490676761f, -0.024541229f
};

void ANGLE::InitializeYTables(float scale)
{
    for (unsigned int index = 0; index < 256; ++index)
    {
        SinYTable[index] = SinTable[index] * scale;
        CosYTable[index] = CosTable[index] * scale;
    }
}

float ANGLE::SinY() const
{
    return SinYTable[value];
}

float ANGLE::CosY() const
{
    return CosYTable[value];
}

float ANGLE::Sin() const
{
    return SinTable[value];
}

float ANGLE::Cos() const
{
    return CosTable[value];
}

ANGLE Decart2Polar(float x, float y)
{
    const double angle = CartesianHeadingRadians(x, y)*static_cast<double>(40.7436676f);
    return ANGLE(static_cast<unsigned char>(TruncateDoubleToInt32(angle)));
}
ANGLE Decart2Polar(float x, float y, float* length)
{
    const ANGLE direction = Decart2Polar(x, y);
    const float absX = x<0.0f?-x:x;
    const float absY = y<0.0f?-y:y;
    if (!(absX >= absY)) *length = -(y/direction.Cos());
    else *length = x/direction.Sin();
    return direction;
}

int Random2(int maximum)
{
    if (maximum == -1) return 0;
    const int divisor = BitsToSigned32(static_cast<unsigned int>(maximum)+1u);
    return NextRandom2Value()%divisor;
}

float Random2(float scale)
{
    return static_cast<float>(
        static_cast<double>(NextRandom2Value())*static_cast<double>(scale)*
        4.656612875245797e-10);
}

int Sqrt(int value)
{
    int remainder = value;
    unsigned int root = 0u;
    unsigned int bit = 0x40000000u;
    do
    {
        const unsigned int trial = root+bit;
        if (remainder >= static_cast<int>(trial))
        {
            remainder-=static_cast<int>(trial);
            root = (root>>1)|bit;
        }
        else
        {
            root>>=1;
        }
        bit>>=2;
    }
    while (bit != 0u);
    return static_cast<int>(root);
}

namespace {
const unsigned int crc32Table[256] = {
    0x00000000u, 0x77073096u, 0xEE0E612Cu, 0x990951BAu, 0x076DC419u, 0x706AF48Fu, 0xE963A535u, 0x9E6495A3u,
    0x0EDB8832u, 0x79DCB8A4u, 0xE0D5E91Eu, 0x97D2D988u, 0x09B64C2Bu, 0x7EB17CBDu, 0xE7B82D07u, 0x90BF1D91u,
    0x1DB71064u, 0x6AB020F2u, 0xF3B97148u, 0x84BE41DEu, 0x1ADAD47Du, 0x6DDDE4EBu, 0xF4D4B551u, 0x83D385C7u,
    0x136C9856u, 0x646BA8C0u, 0xFD62F97Au, 0x8A65C9ECu, 0x14015C4Fu, 0x63066CD9u, 0xFA0F3D63u, 0x8D080DF5u,
    0x3B6E20C8u, 0x4C69105Eu, 0xD56041E4u, 0xA2677172u, 0x3C03E4D1u, 0x4B04D447u, 0xD20D85FDu, 0xA50AB56Bu,
    0x35B5A8FAu, 0x42B2986Cu, 0xDBBBC9D6u, 0xACBCF940u, 0x32D86CE3u, 0x45DF5C75u, 0xDCD60DCFu, 0xABD13D59u,
    0x26D930ACu, 0x51DE003Au, 0xC8D75180u, 0xBFD06116u, 0x21B4F4B5u, 0x56B3C423u, 0xCFBA9599u, 0xB8BDA50Fu,
    0x2802B89Eu, 0x5F058808u, 0xC60CD9B2u, 0xB10BE924u, 0x2F6F7C87u, 0x58684C11u, 0xC1611DABu, 0xB6662D3Du,
    0x76DC4190u, 0x01DB7106u, 0x98D220BCu, 0xEFD5102Au, 0x71B18589u, 0x06B6B51Fu, 0x9FBFE4A5u, 0xE8B8D433u,
    0x7807C9A2u, 0x0F00F934u, 0x9609A88Eu, 0xE10E9818u, 0x7F6A0DBBu, 0x086D3D2Du, 0x91646C97u, 0xE6635C01u,
    0x6B6B51F4u, 0x1C6C6162u, 0x856530D8u, 0xF262004Eu, 0x6C0695EDu, 0x1B01A57Bu, 0x8208F4C1u, 0xF50FC457u,
    0x65B0D9C6u, 0x12B7E950u, 0x8BBEB8EAu, 0xFCB9887Cu, 0x62DD1DDFu, 0x15DA2D49u, 0x8CD37CF3u, 0xFBD44C65u,
    0x4DB26158u, 0x3AB551CEu, 0xA3BC0074u, 0xD4BB30E2u, 0x4ADFA541u, 0x3DD895D7u, 0xA4D1C46Du, 0xD3D6F4FBu,
    0x4369E96Au, 0x346ED9FCu, 0xAD678846u, 0xDA60B8D0u, 0x44042D73u, 0x33031DE5u, 0xAA0A4C5Fu, 0xDD0D7CC9u,
    0x5005713Cu, 0x270241AAu, 0xBE0B1010u, 0xC90C2086u, 0x5768B525u, 0x206F85B3u, 0xB966D409u, 0xCE61E49Fu,
    0x5EDEF90Eu, 0x29D9C998u, 0xB0D09822u, 0xC7D7A8B4u, 0x59B33D17u, 0x2EB40D81u, 0xB7BD5C3Bu, 0xC0BA6CADu,
    0xEDB88320u, 0x9ABFB3B6u, 0x03B6E20Cu, 0x74B1D29Au, 0xEAD54739u, 0x9DD277AFu, 0x04DB2615u, 0x73DC1683u,
    0xE3630B12u, 0x94643B84u, 0x0D6D6A3Eu, 0x7A6A5AA8u, 0xE40ECF0Bu, 0x9309FF9Du, 0x0A00AE27u, 0x7D079EB1u,
    0xF00F9344u, 0x8708A3D2u, 0x1E01F268u, 0x6906C2FEu, 0xF762575Du, 0x806567CBu, 0x196C3671u, 0x6E6B06E7u,
    0xFED41B76u, 0x89D32BE0u, 0x10DA7A5Au, 0x67DD4ACCu, 0xF9B9DF6Fu, 0x8EBEEFF9u, 0x17B7BE43u, 0x60B08ED5u,
    0xD6D6A3E8u, 0xA1D1937Eu, 0x38D8C2C4u, 0x4FDFF252u, 0xD1BB67F1u, 0xA6BC5767u, 0x3FB506DDu, 0x48B2364Bu,
    0xD80D2BDAu, 0xAF0A1B4Cu, 0x36034AF6u, 0x41047A60u, 0xDF60EFC3u, 0xA867DF55u, 0x316E8EEFu, 0x4669BE79u,
    0xCB61B38Cu, 0xBC66831Au, 0x256FD2A0u, 0x5268E236u, 0xCC0C7795u, 0xBB0B4703u, 0x220216B9u, 0x5505262Fu,
    0xC5BA3BBEu, 0xB2BD0B28u, 0x2BB45A92u, 0x5CB36A04u, 0xC2D7FFA7u, 0xB5D0CF31u, 0x2CD99E8Bu, 0x5BDEAE1Du,
    0x9B64C2B0u, 0xEC63F226u, 0x756AA39Cu, 0x026D930Au, 0x9C0906A9u, 0xEB0E363Fu, 0x72076785u, 0x05005713u,
    0x95BF4A82u, 0xE2B87A14u, 0x7BB12BAEu, 0x0CB61B38u, 0x92D28E9Bu, 0xE5D5BE0Du, 0x7CDCEFB7u, 0x0BDBDF21u,
    0x86D3D2D4u, 0xF1D4E242u, 0x68DDB3F8u, 0x1FDA836Eu, 0x81BE16CDu, 0xF6B9265Bu, 0x6FB077E1u, 0x18B74777u,
    0x88085AE6u, 0xFF0F6A70u, 0x66063BCAu, 0x11010B5Cu, 0x8F659EFFu, 0xF862AE69u, 0x616BFFD3u, 0x166CCF45u,
    0xA00AE278u, 0xD70DD2EEu, 0x4E048354u, 0x3903B3C2u, 0xA7672661u, 0xD06016F7u, 0x4969474Du, 0x3E6E77DBu,
    0xAED16A4Au, 0xD9D65ADCu, 0x40DF0B66u, 0x37D83BF0u, 0xA9BCAE53u, 0xDEBB9EC5u, 0x47B2CF7Fu, 0x30B5FFE9u,
    0xBDBDF21Cu, 0xCABAC28Au, 0x53B39330u, 0x24B4A3A6u, 0xBAD03605u, 0xCDD70693u, 0x54DE5729u, 0x23D967BFu,
    0xB3667A2Eu, 0xC4614AB8u, 0x5D681B02u, 0x2A6F2B94u, 0xB40BBE37u, 0xC30C8EA1u, 0x5A05DF1Bu, 0x2D02EF8Du,
};
}

unsigned int CRC32::Add(const void* data, unsigned int size)
{
    unsigned int cursor = PtrToU32(data);
    const unsigned int end = cursor+size;
    if (cursor != 0u)
    {
        while (cursor<end)
        {
            const unsigned int previous = value;
            const unsigned int index = (previous&0xFFu)^static_cast<unsigned int>(*U32ToPtr(cursor));
            const unsigned int next = crc32Table[index]^(previous>>8);
            ++cursor;
            value = next;
        }
    }
    return value;
}

void CRC32::AddFile(STRING fileName)
{
    const unsigned char* const mappedBytes = static_cast<const unsigned char*>(FFileMapping(fileName));
    if (!mappedBytes) return;

    const unsigned int length = static_cast<unsigned int>(FFileLength(fileName));
    (void)Add(mappedBytes, length);
    (void)UnmapViewOfFile(mappedBytes);
}

int CRC32::CheckListFromFile(STRING fileName)
{
    std::FILE* file = 0;
    {
        const STRING mode("r");
        if (fileName.value[0] != 0)
            file = std::fopen(fileName.value, mode.value);
    }
    if (!file) return 1;

    char line[256];
    for (;;)
    {
        if (std::feof(file) != 0)
        {
            (void)std::fclose(file);
            return 1;
        }
        if (std::fgets(line, sizeof(line), file) == 0 || line[0] == 0)
            continue;

        STRING entry(line);
        entry.RemoveEndChars(" \n\r\t");

        const bool hasDot = strstr(entry.value, ".") != 0;
        bool terminal = false;
        if (!hasDot)
        {
            const unsigned int first = static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(entry.value[0])));
            terminal = CrtIsDigit(first) != 0u;
            if (!terminal && entry.value[0] == '-')
            {
                const unsigned int second = static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(entry.value[1])));
                terminal = CrtIsDigit(second) != 0u;
            }
        }

        if (terminal)
        {
            (void)std::fclose(file);
            int expected = BitsToSigned32(PtrToU32(this));
            if (entry.value[1] != 'x') expected = BitsToSigned32(CrtAtoiText(entry.value));
            else (void)std::sscanf(entry.value, "%i", &expected);
            return value != static_cast<unsigned int>(expected) ? 1 : 0;
        }

        const bool mapRelativePath =
            strstr(entry.value, ".lgc") != 0 ||
            strstr(entry.value, ".LGC") != 0 ||
            strstr(entry.value, ".map") != 0 ||
            strstr(entry.value, ".MAP") != 0;

        if (mapRelativePath)
        {
            STRING path = "maps\\"+entry;
            AddFile(path);
        }
        else
        {
            AddFile(entry);
        }
    }
}

char StringGlobals::EmptyTextCharacter = '\0';

char STRING::EMPTY[1] = {0};

char* StringGlobals::EmptySentinel()
{
    return STRING::EMPTY;
}

char* StringEmptySentinel()
{
    return StringGlobals::EmptySentinel();
}

STRING::STRING(const char* source)
{
    if (source && source[0])
    {
        const unsigned int length = static_cast<unsigned int>(strlen(source));
        value = static_cast<char*>(CrtNew(StringCapacity(length)));
        memcpy(value, source, length);
        value[length] = 0;
    }
    else
    {
        value = StringEmptySentinel();
    }
}

STRING::STRING(const STRING& source)
{
    const char* const sourceText = source.value;
    if (sourceText[0])
    {
        const unsigned int length = static_cast<unsigned int>(strlen(sourceText));
        value = static_cast<char*>(CrtNew(StringCapacity(length)));
        memcpy(value, sourceText, length);
        value[length] = 0;
    }
    else
    {
        value = StringEmptySentinel();
    }
}

STRING::STRING(const char* left, const char* right)
{
    const unsigned int leftLength = static_cast<unsigned int>(strlen(left));
    const unsigned int rightLength = static_cast<unsigned int>(strlen(right));
    value = static_cast<char*>(CrtNew(StringCapacity(leftLength+rightLength)));
    if (leftLength != 0u) memcpy(value, left, leftLength);
    memcpy(value+leftLength, right, rightLength+1u);
}

STRING::~STRING()
{
    char* const text = value;
    if (text != StringEmptySentinel())
        CrtFreePointer(text);
}

void STRING::Copy(const char* source, int length)
{
    value = static_cast<char*>(CrtNew(StringCapacity(static_cast<unsigned int>(length))));
    if (length != 0) memcpy(value, source, static_cast<unsigned int>(length));
    value[length] = 0;
}

const STRING STRING::SubString(unsigned int begin, unsigned int end) const
{
    const unsigned int length = end - begin + 1u;
    const char* const source = value + begin;
    STRING result;
    if (length != 0u)
    {
        char* const copy = static_cast<char*>(CrtNew(StringCapacity(length)));
        result.value = copy;
        memcpy(copy, source, length);
        result.value[length] = 0;
    }
    return result;
}

void ReportLiveSpriteReference(void* object)
{
    if (!object) return;
    const STRING subject("PTR_SPRITE with this sprite not clear");
    const SPRITE* const sprite = static_cast<const SPRITE*>(object);
    const VID* const link = sprite->Vid();
    const int id = link?link->nVid:-1;
    const STRING objectFormat("SPRITE [%i](%i,%i,%i)");
    MYERROR* sink = ::Error;
    sink->Error(objectFormat, E_ERROR, subject, 0, id, TruncateFloatToInt32(sprite->Coor().x), TruncateFloatToInt32(sprite->Coor().y), TruncateFloatToInt32(sprite->Coor().z));
}
