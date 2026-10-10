#pragma once
#include "basetype.h"

#include <stddef.h>

#if !defined(_MSC_VER) && !defined(__thiscall)
#define __thiscall
#endif

class iString
{
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void operator=(const char* source) = 0;
    virtual operator const char*() const = 0;


    virtual const char* operator+=(const char* text) = 0;
    virtual const char* operator+=(char value) = 0;
    virtual const char* Format(const char* format, ...) = 0;
    virtual bool IsEmpty() const = 0;
    virtual char GetAt(unsigned int index) = 0;
    virtual const char* StrNCpy(const char* source, int length) = 0;
};


class zString : public iString
{
public:
    char* m_pBuf;

    zString();
    explicit zString(const char* source);
    virtual ~zString();

    void* QueryInterface(int id) const override;
    int iRelease() override;
    void operator=(const char* source) override;
    operator const char*() const override;
    const char* operator+=(const char* text) override;
    const char* operator+=(char value) override;
    const char* Format(const char* format, ...) override;
    bool IsEmpty() const override;
    char GetAt(unsigned int index) override;
    const char* StrNCpy(const char* source, int length) override;

    void Set(const char* _sStr);
    int GetLength();
    const char* c_str();

private:
    void ReleaseBuffer();
    const char* Concatenate(const char* suffix);
};


