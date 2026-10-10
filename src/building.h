#pragma once
#include "unit.h"

class BUILDING : public UNIT
{
public:
    BUILDING(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~BUILDING();
    virtual void MoveTact();
    virtual int Action(int action, int argument1, int argument2, int argument3);

private:
    VID* m_buildVid;
    int  m_buildX;
    int  m_buildY;
    int  m_buildRetryCount;
    int  m_buildStateC0;
    int  m_buildStateC4;
};

