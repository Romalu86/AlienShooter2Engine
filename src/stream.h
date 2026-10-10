#include <cstdio>
#pragma once

class STREAM
{
public:
    virtual ~STREAM() noexcept(false);
    virtual int Read(void* buffer,unsigned int count)=0;
    virtual int Write(const void* buffer,unsigned int count)=0;
};

class FSTREAM : public STREAM
{
public:
    FSTREAM();
    virtual ~FSTREAM();
    virtual int Read(void* buffer,unsigned int count);
    virtual int Write(const void* buffer,unsigned int count);

    std::FILE* file;
};

