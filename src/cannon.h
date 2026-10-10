#pragma once
#include "sprite.h"


class CANNON : public SPRITE
{
public:
    CANNON(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~CANNON();
    virtual void MoveTact();
    virtual int Action(int action, int argument1, int argument2, int argument3);
    virtual void DeletePointerToSprite(SPRITE* sprite);

private:
    int m_cannonMotionFlags;
};

