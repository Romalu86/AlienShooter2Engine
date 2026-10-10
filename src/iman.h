#pragma once

class iMan
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual bool SetHpFromHero(int hp) = 0;
    virtual void CreateEff(int effectId) = 0;
};

