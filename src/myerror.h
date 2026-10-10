#pragma once
#include <cstdio>

class STRING;

enum TYPE_ERROR {
    E_LOCK=0, E_COPY=1, E_MEMORY=2, E_CREATE=3, E_INVALID=4,
    E_LOAD=5, E_SAVE=6, E_OPEN=7, E_SET=8, E_GET=9,
    E_ERROR=10, E_SECTION=11, E_INIT=12, E_MISSING=13, E_UNKNOWN=14
};

struct HWND__;

class MYERROR
{
public:
    MYERROR(const char* baseLogDirectory,int timestampedFile);
    virtual ~MYERROR();

    int  __cdecl Window(STRING text,...);
    void __cdecl LogTmp(STRING text,...);
    void __cdecl Log(STRING text,...);
    void __cdecl Log(const char* text,...);
    void __cdecl LogExit(STRING text,...);
    void __cdecl LogExit(const char* text,...);
    void __cdecl Error(const STRING& objectFormat,TYPE_ERROR type,
                       const STRING& subject,unsigned long errorCode,...);

    HWND__* hwnd;
    FILE* file;
    char filename[1024];
};

extern MYERROR* Error;

