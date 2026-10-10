#pragma once

#include <stddef.h>
#include "unit.h"
#include "iman.h"
#include "zm/Game/Vehicles/zCarControl.h"
#include "zm/Utils/zRefObject.h"
#include "region.h"

class iRPGHero;
class iPlayer;
class iRPGFrameWork;
class PLAYER_ARCADE;

struct ManInterfaceRecord
{
    unsigned int m_00;
    unsigned int m_04;
    unsigned int m_08;
    unsigned char m_0C;
    unsigned char m_pad0D[3];
    unsigned int m_10;
};

struct ManInterface
{
    void** m_vftable;
    ManInterfaceRecord m_records[2];
};

class MAN : public UNIT, public zCarControl, public zRefObject, public iMan
{
public:
    enum ANGLE_SEEN_TYPE
    {
        ANGLE_SEEN_FRONT = 2,
        ANGLE_SEEN_SIDE = 3,
        ANGLE_SEEN_BACK = 4
    };

    MAN(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent);
    virtual ~MAN();
    using SPRITE::operator delete;

    virtual void* QueryInterface(int interfaceId) const;
    virtual int iRelease();
    virtual bool SetHpFromHero(int hp);
    virtual void CreateEff(int effectId);

    void SetTankCanShot();
    virtual void MoveTact();
    virtual int Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter);
    void SetRPGHero(iRPGHero* hero);
    void SetPlayer(iPlayer* player, iRPGFrameWork* framework);
    int GetAmmoCnt();
    void RecalcCurAmmoClip(bool forceComplete);
    void ReloadAmmo();
    int GetReloadBarDir();
    virtual void DeletePointerToSprite(SPRITE* value);

private:
    friend class PLAYER_ARCADE;

    iRPGHero* m_pRPGHero;
    iPlayer* m_pPlayer;
    SPRITE* m_pHeadTarget;
    unsigned char m_tankCanShoot;
    unsigned char m_suppressHpMirror;
    unsigned char m_pad13A[2];
    iRPGFrameWork* m_pRPGFrameWork;
    unsigned char m_reloadInProgress;
    unsigned char m_returnHeadToBody;
    unsigned char m_pad142[2];
    unsigned int m_lastHeadTargetSearchTick;

    unsigned int RecalcCurAmmoClipResult(bool forceComplete);
    bool WeaponNotNeedReload();
    SPRITE* SeekAnyUnit();
    bool IsUnitSeenByDistance(SPRITE* _pUnit, float* distance);
    ANGLE_SEEN_TYPE IsUnitSeenByAngle(SPRITE* _pUnit);
    void RotateHeadTact();
};

