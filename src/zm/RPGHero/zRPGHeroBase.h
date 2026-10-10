#pragma once

#include <cstdio>

#include <stddef.h>
#include "../../irpg.h"
#include "zRPGBaseObject.h"
#include "zm/Utils/zRefObject.h"

const char* GetRPGHeroParamName(unsigned int id);

class iRPGFrameWork;

class zRPGHeroBase : public iRPGHeroBase, public zRPGObj
{
public:

    iRef<iRPGObj>** m_apChildren;
    int m_iChildrenCount;
    int m_iChildrenCapacity;
    int m_iSRandVal;

    zRPGHeroBase(OBJ_TYPE typeValue, int paramsCount);
    ~zRPGHeroBase() override;

    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    void AddChild(iRPGObj* _pObject) override;
    void RemoveChild(iRPGObj* child) override;
    void RecalcSkills() override;
    bool GetParams(const char* token, int length, int& count, int* indices) const override;
    bool ParseUnknownCommand(const char* token, int valueCount,
                             const char** values, iRPGFrameWork* framework) override;
    bool SaveToFile(std::FILE* output) override;
    unsigned int GetSRandVal(bool enabled) override;
    void SetSRandVal(unsigned int value) override;
};


