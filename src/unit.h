#pragma once

#include "terrain.h"
#include "dialog_unit.h"

class UNIT : public TERRAIN, public DIALOG_UNIT
{
public:
    UNIT(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~UNIT();


    virtual SPRITE* Link() const;
    virtual void Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const;
    virtual const VECTOR& Coor() const;
    virtual int AddLinkToLast(SPRITE* sprite);
    virtual void PlaySFX(int sfxId) const;

    virtual void DetectTact();
    virtual void MoveTact();
    virtual int Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter);
    void LineDraw() const;
    void InverseActive();
    int GetActive() const;
    virtual void DrawSecondaryInfo();
    int AddAmmoTick(int divisor);
    int PercentAmmo() const;
    void AddHpPerSecond(int amount);

protected:
    unsigned int m_behaviorFlags;
    int m_ammoFixedPoint;
    int m_legacyState9C;
    int m_lastCommandStopTick;
    int m_moveStartTick;
    int m_turnState;
    int m_turnTimer;
};

