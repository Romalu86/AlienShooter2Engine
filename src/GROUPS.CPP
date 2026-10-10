#include "mylib.h"

#include "basetype.h"
#include "map.h"
#include "sprite_collector.h"
#include "sprite.h"
#include "stream.h"
#include "resread.h"
#include "graph.h"
#include "groups.h"
#include <new>

GROUP::GROUP(GROUP* owner, SPRITE* sprite)
{
    if (owner)
    {
        m_next = owner->m_next;
        owner->m_next = this;
    }
    else
    {
        m_next = this;
    }
    if (sprite)
        Insert(sprite);
}

GROUP::~GROUP()
{
    GROUP* const next = m_next;
    GROUP* previous = next;
    while (previous->m_next != this)
        previous = previous->m_next;
    previous->m_next = next;
}

void GROUP::DrawNumber(int number) const
{
    for (int i = 0;i<m_count;++i)
    {
        const SPRITE* const sprite = m_data[i];
        const VECTOR2 screen(sprite->X()-Map->ShiftXY().x, sprite->Y()-sprite->Z()-Map->ShiftXY().y);
        Graph->PrintfXY(screen, "%i", static_cast<unsigned int>(number));
    }
}

void GROUP::Insert(SPRITE* value)
{
    if (!value)
        return;
    if (m_count != 0)
    {
        const float scale = 0.5f;
        m_centerX = (m_centerX+value->X())*scale;
        m_centerY = (m_centerY+value->Y())*scale;
    }
    else
    {
        m_centerX = value->X();
        m_centerY = value->Y();
    }
    SPRITE_LIST::Insert(value);
}

GROUPS::GROUPS() : GROUP(0, 0)
{
}

GROUPS::~GROUPS()
{
}

void GROUPS::Save(RESOURCE* resource) const
{
    const int endOfGroup = -1;
    for (const GROUP* node = m_next; node && node != this; node = node->m_next)
    {
        for (int i = 0; i<node->m_count; ++i)
            (void)resource->Write(&node->m_data[i], 4u);
        if (node->m_count)
            (void)resource->Write(&endOfGroup, sizeof(endOfGroup));
    }
    (void)resource->Write(&endOfGroup, sizeof(endOfGroup));
}

void GROUPS::Load(RESOURCE* resource)
{
    for (;;)
    {
        SPRITE* const first = Map->ReadPointer(resource);
        if (PtrToU32(first) == static_cast<unsigned int>(-1))
            break;
        void* const storage = CrtNew(sizeof(GROUP));
        GROUP* group = 0;
        if (storage)
        {
            try
            {
                group = new (storage) GROUP(this, first);
            }
            catch (...)
            {
                CrtFreePointer(storage);
                throw;
            }
        }
        for (;;)
        {
            SPRITE* const next = Map->ReadPointer(resource);
            if (PtrToU32(next) == static_cast<unsigned int>(-1))
                break;
            group->Insert(next);
        }
    }
}

void GROUPS::DeletePointerToSprite(SPRITE* sprite)
{
    GROUP* node = m_next;
    while (node && node != this)
    {
        if (node->Delete(sprite) == 0 && node->m_count == 0)
        {
            GROUP* const victim = node;
            node = node->m_next;
            if (node == this)
                node = 0;
            victim->~GROUP();
            CrtFreePointer(victim);
        }
        else
        {
            node = node->m_next;
            if (node == this)
                break;
        }
    }
}

void GROUPS::DrawNumber() const
{
    unsigned int index = 0;
    for (const GROUP* node = m_next; node && node != this; node = node->m_next)
        node->DrawNumber(static_cast<int>(index++));
}


