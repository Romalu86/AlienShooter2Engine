#pragma once
#include "unit.h"


class PLANE : public UNIT
{
public:
    PLANE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~PLANE();
    virtual int Action(int action, int argument1, int argument2, int argument3);

protected:


    virtual void PlaneNextCommand(int command, int argument);
    virtual void ZSpeedInitialization();
    virtual void CheckFlightProperties();
    virtual int WayBlocked();
    virtual void FlightIfWayBlocked();
    virtual void FlightToTarget();
    virtual void FlightToTargetAdditionalActions();
    virtual void FreeFlight();
};

