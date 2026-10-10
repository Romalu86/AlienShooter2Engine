#pragma once
#include "plane.h"


class BALLOON : public PLANE
{
public:
    BALLOON(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~BALLOON();
    virtual void MoveTact();
    int SetCommand(int command, SPRITE* target);

protected:
    void MoveToNearestBase();
    void ConnectToBase();
    virtual void ZSpeedInitialization();
    int IsBalloonMoveFinished();
    virtual void CheckFlightProperties();
    virtual void FlightToTargetAdditionalActions();

    unsigned int m_attachmentTransitionPending;
    unsigned char m_attachmentPhase;
    unsigned char m_attachmentPadding[3];
};

