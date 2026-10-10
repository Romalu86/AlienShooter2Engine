#pragma once

#include "creature.h"

class CIV_ROBOT : public CREATURE
{
public:
    CIV_ROBOT(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~CIV_ROBOT();

    int IsRobotBuilding(const SPRITE* sprite);
    SPRITE* FindRobotBuilding();
    void PathIsBlocked();
    virtual void MoveTact();
    virtual void DeletePointerToSprite(SPRITE* sprite);
    void ChangeAnimation(int animation);
    void RotateHead(ANGLE angle);
    virtual int Action(int action, int argument1, int argument2, int argument3);

protected:
    int m_robotState;
    SPRITE* m_robotBuilding;
    unsigned int m_robotFlagC0;
    unsigned int m_robotFlagC4;
};

