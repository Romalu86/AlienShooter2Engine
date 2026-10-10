
#include "mylib.h"
#include "../../engine_globals.h"
#include "String/zString.h"
#include "zStaticStorage.h"
#include "basetype.h"
#include <ctime>
#include <io.h>
#include <new>
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <windows.h>
#pragma pop_macro("INPUT")
#include <mmsystem.h>

namespace
{
    StaticObjectStorage<zString> fullTimeText;
    unsigned char fullTimeTextInitialized = 0;
    char timeTextBuffer[256] = {};

    zString& FullTimeString()
    {
        return fullTimeText.Get();
    }

    unsigned char& FullTimeStringInitialized()
    {
        return fullTimeTextInitialized;
    }

    char* TimeTextBuffer()
    {
        return timeTextBuffer;
    }

    void __cdecl DestroyFullTimeString()
    {
        FullTimeString().~zString();
    }

}

bool zutl::FileExists(const char* fileName)
{
    _finddata32_t findData;
    const intptr_t handle = CrtFindFirst(fileName, &findData);
    (void)CrtFindClose(handle);
    return handle != -1;
}

void zutl::MoveToTrash(const char* fileName)
{
    if (!fileName || !FileExists(fileName))
        return;

    const STRING backup = STRING(fileName) + ".bak";
    if (FileExists(backup.value))
        (void)::DeleteFileA(backup.value);
    (void)::MoveFileA(fileName, backup.value);
}

const char* zutl::GetTimeStr()
{
    __time32_t stamp;
    (void)_time32(&stamp);
    const std::tm* const broken = _localtime32(&stamp);
    char* const text = TimeTextBuffer();
    (void)std::strftime(text, 256, "%H:%M:%S", broken);
    return text;
}

const char* zutl::GetTimeStrFull()
{
    const char* const timeText = GetTimeStr();
    unsigned char& guard = FullTimeStringInitialized();
    if ((guard & 1u) == 0u)
    {
        guard = static_cast<unsigned char>(guard | 1u);
        (void)new (&FullTimeString()) zString();
        (void)RegisterCrtAtExit(&DestroyFullTimeString);
    }

    const unsigned int random = ::timeGetTime() % 10000u;
    (void)FullTimeString().Format(
        "%s(%.4d,%d)",
        timeText, random, ::CurrentTime);
    return static_cast<const char*>(FullTimeString());
}

int zutl::GetCurTime()
{
    return static_cast<int>(::CurrentTime);
}

unsigned int zutl::GetHashPJW(const char* text)
{
    unsigned int hash = 0u;
    while (*text)
    {
        const int character = static_cast<signed char>(*text);
        hash = (hash << 4) + static_cast<unsigned int>(character);
        const unsigned int high = hash & 0xF0000000u;
        if (high)
        {
            hash &= 0x0FFFFFFFu;
            hash ^= high >> 24;
        }
        ++text;
    }
    return hash;
}

int zutl::GetHashNb(const char* text)
{
    unsigned int a = 1u;
    unsigned int b = 0u;
    while (*text)
    {
        a = (a + static_cast<unsigned char>(*text++)) % 65521u;
        b = (b + a) % 65521u;
    }
    return static_cast<int>((b << 16) + a);
}



