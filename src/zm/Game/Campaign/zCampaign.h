#pragma once
#include "mylib.h"
#include <stddef.h>
#include "../../RPGHero/zRPGBaseObject.h"

class iRPGFrameWork;

class iCampaign
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual int GetBonusBaseId(int value) = 0;
};

#if !defined(_MSC_VER) && !defined(__thiscall)
#  define __thiscall
#endif

struct zCampaignValueList
{
    int count;
    int values[99];
};

class zCampaign : public zRPGObj, public iCampaign
{
public:
    unsigned int m_0F0;
    int m_bonusIds[40];
    int m_bonusCount;
    zCampaignValueList m_bonusValues[40];
    zCampaignValueList m_randomWeights[40];
    iRPGFrameWork* m_frameWork;

    zCampaign(OBJ_TYPE _eType, iRPGFrameWork* frameWork);
    ~zCampaign() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;


    bool ParseUnknownCommand(const char* name, int valueCount, const char** values, iRPGFrameWork* frameWork) override;
    void SetDataArrays() override;
    bool GetParams(const char* name, int nameLength, int& outCount, int* outKind) const override;
    void InitParams() override;
    int GetBonusBaseId(int bonusGroupId) override;

private:
    static bool m__abChangeableParams[50];
    static bool m__abBaseParams[50];
    static bool m__abArrayParams[50];
};




