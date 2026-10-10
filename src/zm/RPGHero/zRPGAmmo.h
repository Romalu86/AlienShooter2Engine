#pragma once

#include <stddef.h>
#include "irpg.h"
#include "zRPGBaseObject.h"

class iRPGFrameWork;

class zRPGAmmo : public iRPGAmmo, public zRPGObj
{
public:
    iRPGObj* m_pEquipOwner;
    iRPGObj* m_addAmmo;

    zRPGAmmo();
    ~zRPGAmmo() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;

private:
    void SetDataArrays() override;

public:
    bool GetParams(const char* token, int length, int& count, int* indices) const override;
    bool ParseUnknownCommand(const char* key, int valueCount,
                             const char** values, iRPGFrameWork* framework) override;

private:
    static bool m__abArrayParams[MAX_RPGPARAMSCOUNT];
    static bool m__abBaseParams[MAX_RPGPARAMSCOUNT];
    static bool m__abChangeableParams[MAX_RPGPARAMSCOUNT];
};

