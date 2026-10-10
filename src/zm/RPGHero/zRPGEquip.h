#pragma once

#include <stddef.h>
#include "irpg.h"
#include "zRPGBaseObject.h"

class zInventory;
class zItem;
class iRPGFrameWork;

class zRPGEquip : public iRPGEquip, public zRPGObj
{
public:
    iRPGObj* m_addAmmo;
    zInventory* m_bpInventory;

    zRPGEquip();
    ~zRPGEquip() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;

private:
    void SetDataArrays() override;
    iRPGObj* GetAddAmmo() override;

protected:
    zInventory* GetBPInv() override;

public:
    bool GetParams(const char* token, int length, int& count, int* outIndex) const override;

protected:
    bool SetSkillValue(int skill, int value) override;

private:
    bool ParseUnknownCommand(const char* key, int valueCount, const char** values, iRPGFrameWork* framework) override;

protected:
    void CopyParametersFrom(iRPGObj* _BaseObj) override;
    int GetBuyCost() const override;
    bool IsBetterEx(zItem* _pItem) override;
    void AddItem(iRPGObj* source) override;

private:
    static bool m__abArrayParams[MAX_RPGPARAMSCOUNT];
    static bool m__abBaseParams[MAX_RPGPARAMSCOUNT];
    static bool m__abChangeableParams[MAX_RPGPARAMSCOUNT];

};

