#pragma once

#include <stddef.h>
#include "irpg.h"
#include "zRPGBaseObject.h"

class iRPGFrameWork;
class zRPGAmmo;

class zRPGWeapon : public iRPGWeapon, public zRPGObj
{
public:
    zRPGAmmo* m_pAmmo;
    bool m_bIncAmmoOnPut;

    zRPGWeapon();
    ~zRPGWeapon() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    zRPGAmmo* GetAmmo();
    void CopyParametersFrom(iRPGObj* source) override;
    bool IfIncAmmoOnPut();
    bool GetParams(const char* token, int length, int& count, int* codes) const override;
    bool ParseUnknownCommand(const char* key, int valueCount,
                             const char** values, iRPGFrameWork* framework) override;

private:
    void SetAmmo(zRPGAmmo* _pAmmo);
    void SetDataArrays() override;
    void InitParams() override;
    bool SetSkillValue(int skill, int value) override;
private:
    static bool m__abArrayParams[MAX_RPGPARAMSCOUNT];
    static bool m__abBaseParams[MAX_RPGPARAMSCOUNT];
    static bool m__abChangeableParams[MAX_RPGPARAMSCOUNT];

};


