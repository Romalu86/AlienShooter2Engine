#include "vid.h"
#include "engine_globals.h"
#include "menu.h"

#include "message.h"
#include "mylib.h"
#include "sprite.h"
#include "graph.h"
#include "map.h"
#include "basetype.h"
#include <new>


MESSAGE::MESSAGE(int mapIndex, int vidIndex, float baseX, float baseY, int lineCount, unsigned long shiftIntervalMs)
    : m_lineCount(lineCount),
      m_shiftIntervalMs(shiftIntervalMs),
      m_mapIndex(mapIndex),
      m_vidIndex(vidIndex),
      m_baseX(baseX),
      m_baseY(baseY),
      m_rowShift(-1)
{
    m_messageStackList.m_count = 0;
    m_messageStackList.m_capacity = 0;
    m_messageStackList.m_data = 0;

    for (int index = 0; index<m_lineCount; ++index)
    {
        m_primarySprites[index] = 0;
        m_secondarySprites[index] = 0;
    }
}

MESSAGE::~MESSAGE()
{
    Release();

}

void MESSAGE::Release()
{
    for (int index = 0; index<m_lineCount; ++index)
    {
        SPRITE* secondary = m_secondarySprites[index];
        delete secondary;
        m_secondarySprites[index] = 0;

        SPRITE* primary = m_primarySprites[index];
        delete primary;
        m_primarySprites[index] = 0;
    }
}

void MESSAGE::DeletePointerToSprite(SPRITE* sprite)
{
    for (int index = 0; index<m_lineCount; ++index)
    {
        if (m_secondarySprites[index] == sprite) m_secondarySprites[index] = 0;
        if (m_primarySprites[index] == sprite) m_primarySprites[index] = 0;
    }
}

static int MessageVerticalOffsetToInt(float spriteY, float spriteZ, float mapShiftY, float viewTop)
{
    const double offset = ((static_cast<double>(spriteY)-spriteZ)-mapShiftY)-viewTop;
    return TruncateDoubleToInt32(offset);
}

void MESSAGE::Tact()
{
    MAP* game = Map;
    VID* mapObject = game->Vid(m_mapIndex);
    const int mapScale = static_cast<int>(mapObject->vidSizeY)+1;

    LIST<MESSAGE_STACK>& queued = m_messageStackList;
    int index = 0;
    while (index<queued.m_count)
    {
        MESSAGE_STACK& record = queued.m_data[index];
        const unsigned int now = ::CurrentTime;
        const unsigned int frameBase = ::PrevCurrentTime;
        const unsigned int expires = record.m_duration;
        if (now-frameBase >= expires)
        {
            PutToStack(record.m_text, record.m_x, record.m_y);
            if (index >= 0 && index<queued.m_count)
            {
                --queued.m_count;
                for (int shiftIndex = index; shiftIndex<queued.m_count; ++shiftIndex)
                    queued.m_data[shiftIndex] = queued.m_data[shiftIndex+1];
                if (queued.m_count == 0) queued.Release();
            }
            continue;
        }
        record.m_duration = expires-now+::PrevCurrentTime;
        ++index;
    }

    if (::CurrentTime-m_lastShiftTime>m_shiftIntervalMs)
    {
        Shift();
        m_lastShiftTime = ::CurrentTime;
    }

    game = Map;
    if ((game->m_menu.ControlFlags()&1u) == 0u) return;
    if (game->m_menu.NVidUnderCursor() != m_vidIndex) return;

    SPRITE* const view = game->m_menu.SelectedSprite();
    const int vertical = MessageVerticalOffsetToInt(
        view->Y(), view->Z(), game->m_shiftXY.y, Graph->viewTop);
    const int row = vertical/mapScale;
    const unsigned int sentinel = 0xC97423F0u;
    const unsigned int xBits = FloatBitsU32(m_screenX[row]);
    const unsigned int yBits = FloatBitsU32(m_screenY[row]);
    if (xBits == sentinel && yBits == sentinel) return;

    const VECTOR2 point(m_screenX[row], m_screenY[row]);
    game->SetShiftCoor(point, 2);
}

void MESSAGE::PutToStack(const STRING& text, float x, float y)
{
    MAP* game = Map;
    VID* mapObject = game->Vid(m_mapIndex);
    const int mapScale = static_cast<int>(mapObject->vidSizeY)+1;

    if (StrEq(text.m_value, StringGlobals::EmptyText())) return;

    m_lastShiftTime = ::CurrentTime;
    if (m_secondarySprites[m_lineCount-1]) Shift();

    SPRITE* textSprite = m_secondarySprites[m_lineCount-1];
    if (!textSprite)
    {
        const float heroY = Graph->viewTop;
        VECTOR position;
        position.x = m_baseX;
        position.y = static_cast<float>(static_cast<double>(heroY)+MultiplySigned32Wrap(m_lineCount-1, mapScale)+m_baseY+2000.0);
        position.z = 2000.0f;
        VID* const vid = Map->Vid(m_mapIndex);
        textSprite = Map->CreateSprite(vid, position, ANGLE(static_cast<unsigned char>(0u)), 0);
        m_secondarySprites[m_lineCount-1] = textSprite;
    }

    textSprite = m_secondarySprites[m_lineCount-1];
    if (textSprite)
        (void) textSprite->Action(ACT_SET_TEXT, PtrToU32(&text), 0, 0);

    m_screenX[m_lineCount-1] = x;
    m_screenY[m_lineCount-1] = y;
    if ((!(x<-1.0f) && !(x>-1.0f)) && (!(y<-1.0f) && !(y>-1.0f))) return;

    const float heroY = Graph->viewTop;
    VECTOR markerPosition;
    markerPosition.x = m_baseX-10.0f;
    markerPosition.y = static_cast<float>(static_cast<double>(heroY)+MultiplySigned32Wrap(m_lineCount-1, mapScale)+(mapScale/2)+m_baseY+2000.0);
    markerPosition.z = 2000.0f;
    VID* const markerVid = Map->Vid(m_vidIndex);
    SPRITE* const marker = Map->CreateSprite(markerVid, markerPosition, ANGLE(static_cast<unsigned char>(0u)), 0);
    m_primarySprites[m_lineCount-1] = marker;
}

void MESSAGE::Shift()
{
    MAP* game = Map;
    VID* mapObject = game->Vid(m_mapIndex);
    const int mapScale = static_cast<int>(mapObject->vidSizeY)+1;

    delete m_secondarySprites[0];
    m_secondarySprites[0] = 0;
    delete m_primarySprites[0];
    m_primarySprites[0] = 0;

    for (int index = 1; index<m_lineCount; ++index)
    {
        SPRITE* sprite = m_primarySprites[index];
        if (!sprite) continue;
        VECTOR position(sprite->X(),
            static_cast<float>(static_cast<double>(MultiplySigned32Wrap(m_rowShift, mapScale))+sprite->Y()),
            sprite->Z());
        sprite->ChangeCoor(position);
        m_primarySprites[index-1] = m_primarySprites[index];
        m_screenX[index-1] = m_screenX[index];
        m_screenY[index-1] = m_screenY[index];
        m_primarySprites[index] = 0;
    }

    for (int index = 1; index<m_lineCount; ++index)
    {
        SPRITE* sprite = m_secondarySprites[index];
        if (!sprite) continue;
        VECTOR position(sprite->X(),
            static_cast<float>(static_cast<double>(MultiplySigned32Wrap(m_rowShift, mapScale))+sprite->Y()),
            sprite->Z());
        sprite->ChangeCoor(position);
        m_secondarySprites[index-1] = m_secondarySprites[index];
        m_secondarySprites[index] = 0;
    }
}

MESSAGE_STACK& MESSAGE_STACK::operator=(const MESSAGE_STACK& source)
{
    m_text=source.m_text;
    m_x=source.m_x;
    m_y=source.m_y;
    m_duration=source.m_duration;
    return *this;
}

template<>
LIST<MESSAGE_STACK>::~LIST()
{
    unsigned char* data = reinterpret_cast<unsigned char*>(m_data);
    if (data)
    {
        const unsigned int elementCount = *reinterpret_cast<unsigned int*>(data-4);
        as2::core::destroyArray(data, sizeof(MESSAGE_STACK), elementCount, [](void* element){ static_cast<MESSAGE_STACK*>(element)->~MESSAGE_STACK(); });
        CrtFreePointer(data-4);
    }
    m_data = 0;
    m_count = 0;
}

template<>
void LIST<MESSAGE_STACK>::Release()
{
    m_capacity = 0;
    m_count = 0;
    unsigned char* data = reinterpret_cast<unsigned char*>(m_data);
    if (data)
    {
        const unsigned int elementCount = *reinterpret_cast<unsigned int*>(data-4);
        as2::core::destroyArray(data, sizeof(MESSAGE_STACK), elementCount, [](void* element){ static_cast<MESSAGE_STACK*>(element)->~MESSAGE_STACK(); });
        CrtFreePointer(data-4);
    }
    m_data = 0;
}


