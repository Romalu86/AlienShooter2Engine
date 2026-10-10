#pragma once
#include "mylib.h"

#include "myerror.h"
#include "sprite_list.h"

class INPUT;
class STRING;
class SPRITE;

class MENU : public SPRITE_LIST
{
    friend class FRAME;
public:
    MENU();
    void Control(INPUT* input);
    int Load(const STRING& fileName);
    int Save(const STRING& fileName);
    int DeleteFromFile(const STRING& fileName);
    int NVidUnderCursor() const;
    int NDirUnderCursor() const;
    SPRITE* SpriteWithName(const STRING& name) const;
    void Error(TYPE_ERROR type, const STRING& subject, unsigned long code);

    unsigned ControlFlags() const
    {
        return m_controlFlags;
    }
    SPRITE* SelectedSprite() const
    {
        return m_selectedSprite;
    }

private:
    unsigned int m_controlFlags;
    SPRITE* m_selectedSprite;
};


#include "basetype.h"


VECTOR2*  MenuComputeViewportOffsetTo(const void* self, VECTOR2* out, const VECTOR* value);
