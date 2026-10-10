#include "mylib.h"
#include "../../engine_globals.h"
#include "basetype.h"
#include "myerror.h"
#include "zm/Utils/zLogConsole.h"
#include "zm/Utils/String/zString.h"
#include "script.h"
#include "graph.h"
#include "region.h"
#include <new>


void zLogConsole::Show_(int value)
{
    m_showMode = value;
}

int zLogConsole::iRelease()
{
    delete this;
    return 0;
}


void zLogConsole::SetOutputCoordinates(float left, float /*right*/, float top, float bottom)
{

    m_left = left;
    m_right = left;
    m_top = top;
    m_bottom = bottom;
}

void zLogConsole::ToggleShow()
{
    m_showMode = BitsToSigned32(static_cast<unsigned int>(m_showMode)+1u)%3;
}

void* zLogConsole::QueryInterface(int id) const
{
    if (id == 22) return const_cast<zLogConsole*>(this);
    DebugAssert(LT_CRIT, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Utils\\zLogConsole.cpp", 36);
    return 0;
}

zLogConsole::zLogConsole()
{
    m_showMode = 0;
    m_left = m_right = m_top = m_bottom = 0.0f;
    m_records.begin = m_records.end = m_records.capacityEnd = 0;
    m_lastMessageTime = 0u;
    m_blinkTime = 0u;
}

zLogConsole::~zLogConsole()
{
    CrtFreePointer(m_records.begin);
    m_records.begin = m_records.end = m_records.capacityEnd = 0;
}

void __cdecl GetNew_zLogConsole(iLogConsole** out)
{
    void* storage = static_cast<unsigned char*>(CrtNew(sizeof(zLogConsole)));
    zLogConsole* result = 0;
    if (storage)
    {
        try { result = new (storage) zLogConsole(); }
        catch (...) { CrtFreePointer(storage); throw; }
    }
    *out = result;
}

void zLogConsole::Tact()
{
    static const unsigned int lifetimeSeconds[] = {0u, 0u, 15u, 15u, 60u, 60u, 8u};
    unsigned int recordIndex = 0;
    for (;;)
    {
        LOG_STRUCT* begin = m_records.begin;
        if (!begin) break;
        LOG_STRUCT* end = m_records.end;
        const int count = static_cast<int>(end-begin);
        if (recordIndex >= static_cast<unsigned int>(count)) break;
        LOG_STRUCT& record = begin[recordIndex];
        unsigned int lifetimeMilliseconds = lifetimeSeconds[record.type]*1000u;
        if (lifetimeMilliseconds && static_cast<unsigned int>(::RealCurrentTime-record.time) >= lifetimeMilliseconds)
        {
            RemoveString(static_cast<int>(recordIndex));
            continue;
        }
        ++recordIndex;
    }
}

#include <stdarg.h>


void __cdecl zLogConsole::LogStr(LOG_TYPE type, const char* format, ...)
{
    char buffer[4096];
    buffer[0] = static_cast<char>(*reinterpret_cast<const unsigned char*>(StringGlobals::EmptyText()));
    for (unsigned int byteIndex = 1;byteIndex<4096u;++byteIndex)
        buffer[byteIndex] = 0;

    va_list arguments;
    va_start(arguments, format);
    const int formattedLength = std::vsprintf(buffer, format, arguments);
    va_end(arguments);
    if (formattedLength >= 0)
        AddString(type, buffer);
}


LOG_STRUCT& zLogConsoleRecordVector::operator[](unsigned int index)
{
    return begin[index];
}


void zLogConsole::RemoveString(int index)
{
    LOG_STRUCT* begin = m_records.begin;
    delete begin[index].text;

    begin = m_records.begin;
    LOG_STRUCT* const removedRecord = begin+index;
    LOG_STRUCT* const end = m_records.end;
    for (LOG_STRUCT* source = removedRecord+1;source != end;++source)
        source[-1] = *source;
    --m_records.end;
}


void zLogConsole::Render(GRAPH* graph)
{
    const int x = TruncateDoubleToInt32(static_cast<double>(graph->viewLeft)+10.0);
    int y = TruncateDoubleToInt32(static_cast<double>(graph->viewTop)+40.0);
    unsigned int index = 0u;
    for (;;)
    {
        const LOG_STRUCT* const begin = m_records.begin;
        if (!begin)
            return;
        const int count = static_cast<int>(m_records.end-begin);
        if (index >= static_cast<unsigned int>(count))
            return;

        const LOG_STRUCT* const records =
            begin;
        const int type = static_cast<int>(records[index].type);
        unsigned int color = GRAPH::GREEN.value;
        if (type == LT_WARN || type == LT_INPTERR)
            color = GRAPH::YELLOW.value;
        else if (type == LT_ERR || type == LT_CRIT)
            color = GRAPH::RED.value;

        if (m_showMode == 2 || (type >= LT_WARN && m_showMode == 1))
        {
            int drawX = x;
            int drawY = y;
            bool draw = true;
            if (type == LT_BLINK)
            {
                drawX = TruncateDoubleToInt32(static_cast<double>(graph->viewLeft)+150.0);
                drawY = TruncateDoubleToInt32(static_cast<double>(graph->screenHeight)*0.5+
                    static_cast<double>(graph->viewTop)-30.0);
                const unsigned int now = ::RealCurrentTime;
                const int elapsed = BitsToSigned32(now-m_blinkTime);
                if (elapsed >= 2000 || elapsed<0)
                    m_blinkTime = now;
                else if (elapsed >= 1800)
                    draw = false;
            }
            if (draw)
            {
                const LOG_STRUCT* const currentRecords =
                    m_records.begin;
                COLOR drawColor;
                drawColor.value = color;
                graph->PutsXY(static_cast<float>(drawX), static_cast<float>(drawY),
                    currentRecords[index].text->c_str(), drawColor);
                if (type != LT_BLINK)
                    y+=14;
            }
        }
        ++index;
    }
}


void zLogConsole::AddString(LOG_TYPE category, const char* text)
{
    static const char* const typeNames[] = {
        "   LT_INFO", " LT_NOTICE", "   LT_WARN", "LT_INPTERR",
        "    LT_ERR", "   LT_CRIT", "  LT_BLINK"
    };
    zLogConsoleRecordVector* const records = &m_records;
    LOG_STRUCT* begin = m_records.begin;
    LOG_STRUCT* end = m_records.end;
    if (begin && static_cast<unsigned int>(static_cast<int>(end-begin)) >= 10u)
    {
        const unsigned int count = static_cast<unsigned int>(static_cast<int>(end-begin));
        for (unsigned int i = 0u;i<count;++i)
        {
            if (static_cast<int>(begin[i].type)<LT_WARN)
            {
                RemoveString(static_cast<int>(i));
                break;
            }
        }
    }

    zString line(zutl::GetTimeStr());
    (void)(line += ":");
    (void)(line += text);

    zString* owned = 0;
    void* const memory = static_cast<unsigned char*>(CrtNew(sizeof(zString)));
    if (memory) owned = new(memory) zString();
    owned->Set(line);

    const LOG_STRUCT record = {owned, category, ::RealCurrentTime};
    records->insert(m_records.end, 1u, record);
    const unsigned int now = ::RealCurrentTime;
    m_lastMessageTime = now?now:1u;

    line.Set(zutl::GetTimeStrFull());
    (void)(line += ":");
    (void)(line += typeNames[category]);
    (void)(line += ": ");
    (void)(line += text);
    MYERROR* const log = ::Error;
    (void)log->Log("%s", line.c_str());

}

void zLogConsoleRecordVector::insert(LOG_STRUCT* position, unsigned int count, const LOG_STRUCT& value)
{
    zLogConsoleRecordVector* const records = this;
    LOG_STRUCT* const begin = records->begin;
    LOG_STRUCT* const end = records->end;
    LOG_STRUCT* const insertionPoint = position;
    const LOG_STRUCT* const insertedRecord = &value;
    const unsigned int availableRecords = static_cast<unsigned int>(
        records->capacityEnd-records->end);

    if (availableRecords<count)
    {
        const unsigned int size = begin?static_cast<unsigned int>(
            records->end-records->begin):0u;
        const unsigned int growth = size>count?size:count;
        const unsigned int capacity = size+growth;
        const unsigned int allocationCount = BitsToSigned32(capacity)<0?0u:capacity;
        LOG_STRUCT* const storage = static_cast<LOG_STRUCT*>(CrtNew(allocationCount*sizeof(LOG_STRUCT)));
        LOG_STRUCT* destination = storage;
        for (LOG_STRUCT* source = begin;source != insertionPoint;++source, ++destination)
            if (destination) *destination = *source;
        for (unsigned int index = 0u;index<count;++index, ++destination)
            if (destination) *destination = *insertedRecord;
        for (LOG_STRUCT* source = insertionPoint;source != end;++source, ++destination)
            if (destination) *destination = *source;

        CrtFreePointer(begin);
        records->capacityEnd = storage+capacity;
        const unsigned int oldSize = records->begin?static_cast<unsigned int>(
            records->end-records->begin):0u;
        records->begin = storage;
        records->end = storage+oldSize+count;
        return;
    }

    const unsigned int tailRecords = static_cast<unsigned int>(
        records->end-position);
    if (tailRecords<count)
    {
        LOG_STRUCT* destination = insertionPoint+count;
        for (LOG_STRUCT* source = insertionPoint;source != end;++source, ++destination)
            if (destination) *destination = *source;
        destination = end;
        for (unsigned int index = 0u;index<count-tailRecords;++index, ++destination)
            if (destination) *destination = *insertedRecord;
        for (LOG_STRUCT* destination = insertionPoint;destination != end;++destination)
            *destination = *insertedRecord;
        records->end+=count;
    }
    else if (count>0u)
    {
        LOG_STRUCT* destination = end;
        for (LOG_STRUCT* source = end-count;source != end;++source, ++destination)
            if (destination) *destination = *source;
        LOG_STRUCT* source = end-count;
        destination = end;
        while (source != insertionPoint)
            *--destination = *--source;
        for (unsigned int index = 0u;index<count;++index)
            insertionPoint[index] = *insertedRecord;
        records->end+=count;
    }
}



void zLogConsole::SetScrollTextDelay(int delay)
{
    m_scrollTextDelay = static_cast<unsigned int>(delay);
}
