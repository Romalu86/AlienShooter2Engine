#pragma once
#include "mylib.h"
#include <stddef.h>
#include "basetype.h"
#include "irpg.h"
#include "zm/Utils/zRefObject.h"

class iRPGObj;
class VID;
class COLOR;
class SPRITE;

class iVidCopier
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual bool SetLegsVid(int tableSet,int tableIndex,int renderIndex,int speedMultiplier) = 0;
    virtual bool SetBodyVid(int tableSet,int bodyIndex,int variantIndex) = 0;
    virtual bool SetHeadVid(int tableSet,int fallbackVid,int headMode) = 0;
    virtual bool SetWeaponProperties(const iRPGObj* rpgObject) = 0;
    virtual bool SetHeroColor(int colorIndex) = 0;
    virtual int GetHeroColor() = 0;
    virtual ~iVidCopier() noexcept(false) = 0;
    virtual void ShowInventory(bool show) = 0;
    virtual void InventorySetActive(bool active) = 0;
    virtual int GetDirtyState() = 0;
    virtual int GetVidDirectionsNum(int index) = 0;
    virtual int GetFaceDir(int value) = 0;
    virtual int RecalcCurAmmoClip(bool force) = 0;
    virtual int GetCurCadr(int object) = 0;
    virtual void SetTankCanShot() = 0;
    virtual void CheckRPGHeroChange(bool force) = 0;
    virtual void SetCtrlWheelsCar(SPRITE* sprite) = 0;
    virtual void AddHypnMonst(SPRITE* sprite,int timeOffset) = 0;
    virtual void RemoveFlagmanDialog() = 0;
    virtual int GetCarSpeed(SPRITE* sprite) = 0;
    virtual void ReloadDB() = 0;
};
inline iVidCopier::~iVidCopier() noexcept(false)
{
}

class zVidCopier : public iVidCopier, public iPlayer {
public:
    enum VC_VID_TYPE {
        VC_LEGS=0,
        VC_BODY=1,
        VC_HEAD=2,
        VC_WEAPON=3
    };

    COLOR* m_heroColor[8];
    int m_appliedHeroColor;
    float m_engineSpeedBase;
    int m_iArmy;
    iRef<iRPGHero> m_refLegs;
    iRef<iRPGObj> m_refBody;
    iRef<iRPGObj> m_refHead;
    int m_weaponState;

    zVidCopier();
    virtual ~zVidCopier() override;
    virtual void* QueryInterface(int interfaceId) const override;
    virtual int iRelease() override;
    virtual bool SetLegsVid(int tableSet,int tableIndex,int renderIndex,int speedMultiplier) override;
    virtual bool SetBodyVid(int tableSet,int bodyIndex,int variantIndex) override;
    virtual bool SetHeadVid(int tableSet,int fallbackVid,int headMode) override;
    virtual bool SetWeaponProperties(const iRPGObj* rpgObject) override;
    int ReadColors();
    int PrevHeroColor();
    virtual int GetHeroColor() override;
    int NextHeroColor();
    bool ApplyHeroColor();
    virtual bool SetHeroColor(int colorIndex) override;
    float GetEngineSpeed(int multiplier);
    int GetDestVidNum(VC_VID_TYPE vidType);
    VID* GetDestVid(VC_VID_TYPE vidType);

private:
    static int heroLegVids[2][4];
    static int heroHeadVids[2][4];
    static int heroBodyVids[2][40];
    static int maxSpeedPercent;
};


unsigned int __cdecl GameZvidcopier_StaticCleanupRegister01();
