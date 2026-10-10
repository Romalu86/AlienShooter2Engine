#pragma once
#include "mylib.h"
#include <stddef.h>

class GRAPH;
class zString;
enum LOG_TYPE
{
    LT_INFO=0, LT_NOTICE=1, LT_WARN=2, LT_INPTERR=3,
    LT_ERR=4, LT_CRIT=5, LT_BLINK=6
};

struct LOG_STRUCT
{
    zString* text;
    LOG_TYPE type;
    unsigned int time;
};

struct zLogConsoleRecordVector
{
    unsigned char allocatorState;
    unsigned char padding[3];
    LOG_STRUCT* begin;
    LOG_STRUCT* end;
    LOG_STRUCT* capacityEnd;

    LOG_STRUCT& operator[](unsigned int index);
    void insert(LOG_STRUCT* position, unsigned int count, const LOG_STRUCT& value);
};

class iLogConsole
{
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void SetOutputCoordinates(float left,float right,float top,float bottom) = 0;
    virtual void Show_(int mode) = 0;
    virtual void ToggleShow() = 0;
    virtual void SetScrollTextDelay(int delay) = 0;
    virtual void __cdecl LogStr(LOG_TYPE type,const char* format,...) = 0;
    virtual void Render(GRAPH* graph) = 0;
    virtual void Tact() = 0;
};

class zLogConsole : public iLogConsole
{
public:
    int m_showMode;
    float m_left;
    float m_right;
    float m_top;
    float m_bottom;
    zLogConsoleRecordVector m_records;
    unsigned int m_scrollTextDelay;
    unsigned int m_lastMessageTime;
    unsigned int m_blinkTime;

    zLogConsole();
    virtual void* QueryInterface(int id) const;
    virtual int iRelease();
    virtual void SetOutputCoordinates(float left,float right,float top,float bottom);
    virtual void Show_(int mode);
    virtual void ToggleShow();
    virtual void SetScrollTextDelay(int delay);
    virtual void __cdecl LogStr(LOG_TYPE type,const char* format,...);
    virtual void Render(GRAPH* graph);
    virtual void Tact();
    virtual ~zLogConsole();

    void AddString(LOG_TYPE type,const char* text);
    void RemoveString(int index);
};


#include "basetype.h"


void __cdecl GetNew_zLogConsole(iLogConsole** out);


