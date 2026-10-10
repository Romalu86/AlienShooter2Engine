#pragma once

#include "irpg.h"
#include "zRPGHeroBase.h"
#include <stddef.h>

class zRPGObj;
class iRPGObj;
class iTextDB;

class zRPGImplant : public iRPGImplant, public zRPGHeroBase
{
public:
    iTextDB* m_pTextDB;

    explicit zRPGImplant(iTextDB* textDb = 0);
    ~zRPGImplant() override;

    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;

private:
    void SetDataArrays() override;

public:
    void CopyParametersFrom(iRPGObj* source) override;
    int GetExtInfoLen() const override;
    void OutputExtInfoStr(const char* token, int length, char*& ioCursor, int ordinal) const override;

private:
    int ImplantRand();

protected:
    void GenerateRandomSkills();
    void GenerateName();
    void GenerateCost();
    bool SetSkillValue(int skill, int value) override;

public:
    bool GetParams(const char* token, int length, int& count, int* codes) const override;
private:
    static bool m__abArrayParams[MAX_RPGPARAMSCOUNT];
    static bool m__abBaseParams[MAX_RPGPARAMSCOUNT];
    static bool m__abChangeableParams[MAX_RPGPARAMSCOUNT];

};
