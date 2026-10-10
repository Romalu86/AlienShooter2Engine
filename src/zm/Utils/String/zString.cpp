#include "mylib.h"
#include "basetype.h"

#include "zString.h"
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <new>

bool zString::IsEmpty() const
{
    if (m_pBuf == 0)
        return true;

    return std::strlen(m_pBuf) == 0;
}


char zString::GetAt(unsigned int index)
{
    char result = 0;
    if (m_pBuf != 0)
    {
        if (index < std::strlen(m_pBuf))
            result = m_pBuf[index];
    }
    return result;
}

zString::zString()
    : m_pBuf(0)
{
}


zString::zString(const char* source)
    : m_pBuf(0)
{
    Set(source);
}


zString::~zString()
{
    ReleaseBuffer();
}


void* zString::QueryInterface(int id) const
{
    if (id == 38)
        return static_cast<iString*>(const_cast<zString*>(this));
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Utils\\String\\zString.cpp", 17);
    return 0;
}

int zString::iRelease()
{
    delete this;
    return 0;
}


void zString::ReleaseBuffer()
{
    if (m_pBuf)
    {
        std::free(m_pBuf);
        m_pBuf = 0;
    }
}

void zString::Set(const char* _sStr)
{
    if (m_pBuf != 0 && _sStr == m_pBuf)
        DebugAssert(5, "!m_pBuf || _sStr != m_pBuf",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Utils\\String\\zString.cpp", 79);

    ReleaseBuffer();
    if (_sStr == 0)
        return;

    const unsigned int firstLength = static_cast<unsigned int>(std::strlen(_sStr));
    if (firstLength == 0)
        return;

    char* storage = static_cast<char*>(CrtAllocate(firstLength + 1u));
    m_pBuf = storage;

    const unsigned int secondLength = static_cast<unsigned int>(std::strlen(_sStr)) + 1u;
    std::memcpy(storage, _sStr, secondLength);
}


int zString::GetLength()
{
    return m_pBuf ? static_cast<int>(std::strlen(m_pBuf)) : 0;
}


const char* zString::c_str()
{
    return m_pBuf;
}


void zString::operator=(const char* source)
{
    Set(source);
}


zString::operator const char*() const
{
    return m_pBuf;
}


const char* zString::operator+=(const char* text)
{
    return Concatenate(text);
}


const char* zString::operator+=(char value)
{
    char text[2];
    text[0] = value;
    text[1] = '\0';
    return Concatenate(text);
}


const char* zString::StrNCpy(const char* source, int length)
{
    ReleaseBuffer();
    if (source && length != 0)
    {
        const unsigned int size = static_cast<unsigned int>(length);
        m_pBuf = static_cast<char*>(CrtAllocate(size + 1u));
        (void)std::strncpy(m_pBuf, source, size);
        m_pBuf[size] = '\0';
    }
    return m_pBuf;
}

const char* zString::Format(const char* format, ...)
{
    char text[4096] = "";
    va_list args;
    va_start(args, format);
    const int rc = std::vsprintf(text, format, args);
    va_end(args);
    Set(rc >= 0 ? text : 0);
    return rc >= 0 ? m_pBuf : 0;
}

const char* zString::Concatenate(const char* suffix)
{
    if (m_pBuf == 0)
    {
        Set(suffix);
        return 0;
    }
    if (suffix == 0)
        return 0;

    const unsigned int currentLength = static_cast<unsigned int>(std::strlen(m_pBuf));
    const unsigned int suffixLength = static_cast<unsigned int>(std::strlen(suffix));
    char* const storage = static_cast<char*>(CrtAllocate(currentLength + suffixLength + 1u));
    storage[0] = '\0';

    const unsigned int currentBytes = static_cast<unsigned int>(std::strlen(m_pBuf)) + 1u;
    std::memcpy(storage, m_pBuf, currentBytes);
    const unsigned int suffixBytes = static_cast<unsigned int>(std::strlen(suffix)) + 1u;
    const unsigned int appendPosition = static_cast<unsigned int>(std::strlen(storage));
    std::memcpy(storage + appendPosition, suffix, suffixBytes);

    Set(storage);
    std::free(storage);
    return m_pBuf;
}

