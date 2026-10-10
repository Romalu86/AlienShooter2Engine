#pragma once

#include <stddef.h>
#include "irpg.h"
#include "zRPGBaseObject.h"

class zItem;

class zRPGArmor : public iRPGArmor, public zRPGObj
{
public:
    static int m_iFullDmgCost;

    zRPGArmor();
    ~zRPGArmor() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;

private:
    void SetDataArrays() override;

protected:
    void InitParams() override;

public:
    bool GetParams(const char* token, int tokenLength, int& valueCount, int* propertyId) const override;

protected:
    int Damage(int damage) override;
    int GetBuyCost() const override;

public:
    int GetRepairCost() override;
    int Repair(int requestedCost) override;
    int GetSkillValue(int paramIndex) const override;

protected:
    bool IsBetterEx(zItem* _pItem) override;
private:
    static bool m__abArrayParams[MAX_RPGPARAMSCOUNT];
    static bool m__abBaseParams[MAX_RPGPARAMSCOUNT];
    static bool m__abChangeableParams[MAX_RPGPARAMSCOUNT];

};

