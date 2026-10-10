#pragma once

#include "unit.h"

class REGION;

class CREATURE : public UNIT
{
public:
    CREATURE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~CREATURE();
    virtual void DeletePointerToSprite(SPRITE* sprite);
    virtual void MoveTact();
    REGION* FindRegion(const VECTOR& position);
    virtual int Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter);

protected:
    unsigned int m_regionFlags;
    REGION* m_region;
};

