#pragma once

#include "terrain.h"

class R_DOT;

class RAIL : public TERRAIN
{
public:
    RAIL(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent = 0);
    virtual ~RAIL();
    virtual int Action(int command, int directionArgument, int argument2, int argument3);
    void UnBreak(R_DOT* dot);

private:
    R_DOT* m_dotA;
    R_DOT* m_dotB;
};

