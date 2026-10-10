#include "mylib.h"
#include "sprite.h"
#include "sprite_list.h"
#include "list.h"
#include <stddef.h>
#include <new>
#include "basetype.h"


SPRITE_LIST::SPRITE_LIST()
{
    m_data = 0;
    m_count = 0;
    m_capacity = 0;
}

SPRITE_LIST::~SPRITE_LIST()
{
}



void SPRITE_LIST::Insert(SPRITE* sprite)
{
    if (!sprite) return;
    sprite->AddReference();
    ExpandForInsert();
    m_data[m_count] = sprite;
    m_count = BitsToSigned32(static_cast<unsigned int>(m_count)+1u);
}


void SPRITE_LIST::InsertSorted(SPRITE* sprite)
{
    if (!sprite) return;
    sprite->AddReference();
    int index = 0;
    const float key = sprite->Z();
    while (index<m_count)
    {
        const float current = m_data[index]->Z();

        if (!(FloatIsNaN(key) || FloatIsNaN(current) || key<current)) break;
        ++index;
    }
    ExpandForInsert();
    const int previousCount = m_count;
    m_count = BitsToSigned32(static_cast<unsigned int>(m_count)+1u);
    for (int moveIndex = previousCount; moveIndex>index; --moveIndex) m_data[moveIndex] = m_data[moveIndex-1];
    m_data[index] = sprite;
}


void SPRITE_LIST::InsertSorted(SPRITE* sprite, float key)
{
    if (!sprite) return;
    sprite->AddReference();
    int index = 0;
    while (index<m_count)
    {
        const float current = m_data[index]->Z();

        if (FloatIsNaN(current) || FloatIsNaN(key) || !(current>key)) break;
        ++index;
    }
    ExpandForInsert();
    const int previousCount = m_count;
    m_count = BitsToSigned32(static_cast<unsigned int>(m_count)+1u);
    for (int moveIndex = previousCount; moveIndex>index; --moveIndex) m_data[moveIndex] = m_data[moveIndex-1];
    m_data[index] = sprite;
}


int SPRITE_LIST::Delete(SPRITE* sprite)
{
    if (!sprite) return 1;
    int index = m_count;
    while (index>0)
    {
        if (m_data[--index] == sprite) break;
    }
    if (index<0 || index >= m_count || m_data[index] != sprite) return 1;
    --m_count;
    m_data[index] = m_data[m_count];
    sprite->Release();
    return 0;
}


int SPRITE_LIST::DeleteSorted(SPRITE* sprite)
{
    if (!sprite) return 1;
    int index = m_count;
    while (index>0)
    {
        if (m_data[--index] == sprite) break;
    }
    if (index<0 || index >= m_count || m_data[index] != sprite) return 1;
    --m_count;
    for (int moveIndex = index; moveIndex<m_count; ++moveIndex) m_data[moveIndex] = m_data[moveIndex+1];
    if (m_count == 0)
    {
        m_capacity = 0;
        m_count = 0;
        if (m_data) ::operator delete(m_data);
        m_data = 0;
    }
    sprite->Release();
    return 0;
}


int SPRITE_LIST::DeleteSpriteNumber(int index)
{
    if (index<0 || index >= m_count) return 1;
    SPRITE* sprite = m_data[index];
    --m_count;
    m_data[index] = m_data[m_count];
    if (sprite->Release() != 0)
    {
        sprite->~SPRITE();
        CrtFreePointer(sprite);
    }
    return 0;
}


void SPRITE_LIST::DeleteAll()
{
    for (int first = 0; first<m_count; ++first)
    {
        for (int scan = m_count-1; scan>first; --scan)
        {
            if (m_data[first] && m_data[first] == m_data[scan])
            {
                m_data[scan]->Release();
                if (scan >= 0 && scan<m_count)
                {
                    --m_count;
                    m_data[scan] = m_data[m_count];
                }
            }
        }
    }
    for (int index = m_count-1; index >= 0; --index) if (m_data[index]) DeleteSpriteNumber(index);
    m_capacity = 0;
    m_count = 0;
    if (m_data) ::operator delete(m_data);
    m_data = 0;
}


void SPRITE_LIST::Release()
{
    for (int index = m_count-1; index >= 0; --index)
    {
        SPRITE* sprite = m_data[index];
        if (sprite && sprite->Release() != 0 && index >= 0 && index<m_count)
        {
            --m_count;
            m_data[index] = m_data[m_count];
        }
    }
    m_capacity = 0;
    m_count = 0;
    if (m_data) ::operator delete(m_data);
    m_data = 0;
}
