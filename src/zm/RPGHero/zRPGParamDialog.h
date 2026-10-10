#pragma once
#include <stddef.h>
#include "irpg.h"
#include "zm/Utils/zRefObject.h"

class zRPGParamDialog : public iRPGParamDialog {
public:

    iRef<iRPGObj> m_heroRef;
    iRef<iRPGHero> m_childRef;
    iRPGFrameWork* m_pRPGFrameWork;
    iTextDB* m_pTextDB;
    unsigned int m_024;
    int m_scriptSkillAdd;
    int m_aParamIncrease[RPGHERO_PARAMS_COUNT];
    int m_perkSkill;
    int m_skillIncrease;
    int m_iCurHero;
    struct HeroStorage {
        unsigned char allocatorState;
        unsigned char allocatorPadding[3];
        iRPGObj** begin;
        iRPGObj** end;
        iRPGObj** capacity;

        HeroStorage();
        ~HeroStorage();
        void PushBack(iRPGObj* hero);
    };

    HeroStorage m_apHeroes;
    iPlayer* m_pPlayer;
    int m_playerMenuType;

    zRPGParamDialog();
    virtual ~zRPGParamDialog();
    static void operator delete(void* pointer);

    void* QueryInterface(unsigned int id) const override;
    int iRelease() override;
    void AddSkill(int _iParamNum, int stepCount) override;
    const char* GetFormattedSkill(int paramIndex) override;
    void SetRPGFrameWork(iRPGFrameWork* frameWork) override;
    void FirstHero(int expectedSex) override;
    void PrevHero() override;
    void NextHero() override;
    iRPGObj* GetCurHero() override;
    void SetHeroExternal(iRPGObj* heroObject) override;
    void ChangeCurSex() override;
    const char* GetPlayerName() override;
    const char* GetHeroInfo() override;
    const char* GetObjInfo(iRPGObj* heroObject) override;
    const char* GetVidDirInfo(int infoKind, int infoIndex) override;
    void AssignSkills() override;
    bool CanIncSkill(int _iParamNum) const override;
    bool CanDecSkill(int _iParamNum) const override;
    void SetPerkSkill(int perkSkill) override;
    int GetPerkSkill() const override;
    void RemoveSkillIncrease() override;
    void ResetScriptFunctions() override;
    void SetPlayeR(iPlayer* player) override;
    void RecieveNetPlayers() override;
    void SetPlayerMenuType(int playerMenuType) override;
    int GetPlayerMenuType() const override;
private:
    void SetHero(iRPGObj* _pHero);
    iRPGObj* NextHero(int expectedSex, int step);
    int GetSkillAddValue(int paramIndex);
    void LoadHeroesIfNeed();
};


void __cdecl GetNew_zRPGParamDialog(iRPGParamDialog** outObject);
