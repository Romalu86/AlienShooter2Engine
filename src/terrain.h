#pragma once

#include "sprite.h"

class iQuestEvent
{
public:
    virtual int OnSpriteCreate(SPRITE* sprite) = 0;
    virtual int OnSpriteDeath(const SPRITE* sprite) = 0;
    virtual void SetRadioEx(int radioId, int recordIndex, bool skipRecordCopy) = 0;
};

class TERRAIN : public SPRITE
{
public:
    TERRAIN(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent = 0);
    virtual ~TERRAIN();
    virtual int Action(int action, int argument1, int argument2, int argument3);
};

extern iQuestEvent* gpQuestEvent;

