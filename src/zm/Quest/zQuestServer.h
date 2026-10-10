#pragma once
#include <cstdio>
#include "basetype.h"
#include "zm/RPGHero/zRPGFrameWork.h"
#include <stddef.h>
#if !defined(_MSC_VER) && !defined(__thiscall)
#define __thiscall
#endif

class iRPGFrameWork;
class iRPGQuest;
enum OBJ_TYPE : int;
enum CSI_ACTION : int;
class iQuestServer
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual int GetQuestState(iRPGQuest* quest) = 0;
    virtual bool SetQuestState(iRPGQuest* quest, int state, bool value) = 0;
    virtual void SetDirtyState() = 0;
    virtual bool GetDirtyState() = 0;
    virtual int GetLevelsNum() = 0;
    virtual int GetQuestNum() = 0;
    virtual int GetSubQuestNum(int questNum) = 0;
    virtual const char* GetSubQuestName(iRPGQuest* quest) = 0;
    virtual void ResetAllQuests() = 0;
    virtual iRPGQuest* GetQuest(int questNum, int subQuestNum, bool useIndex, bool assertIfMissing) = 0;
    virtual iRPGQuest* GetLevel(int index) = 0;
    virtual bool SaveQuestsToFile(const char* _sFileName, iRPGFrameWork* framework) = 0;
    virtual iRPGQuest* CreateNewSubQuest(int questNum, int subQuestNum) = 0;
    virtual iRPGQuest* CreateNewQuest(int questNum) = 0;
    virtual iRPGQuest* CreateNewLevel(int levelNum) = 0;
    virtual int GetPrevQuestMaxTimeLine(iRPGQuest* quest) = 0;
    virtual bool DeleteQuest(iRPGQuest* quest) = 0;
    virtual int GetLocalNum(iRPGQuest* quest) = 0;
    virtual bool SetCurLevel(iRPGQuest* quest) = 0;
    virtual void CheckFlagmanCoord(int x, int y) = 0;
    virtual void CheckStateItemName(CSI_ACTION action, const char* itemName) = 0;
    virtual void CheckStateItemVid(CSI_ACTION action, int vid) = 0;
    virtual void CheckNamedMonsters(int value) = 0;
    virtual void AddBonuses(iRPGQuest* quest) = 0;
    virtual bool AreLinked(iRPGQuest* first, iRPGQuest* second, bool reverse) = 0;


    virtual int QuestToId(int questNum, int subQuestNum) = 0;
    virtual int QuestToId(iRPGQuest* quest) = 0;
    virtual iRPGQuest* IdToQuest(int packedKey) = 0;
    virtual void AddNeededVid(int vid) = 0;
    virtual void PushCurState(const char* fileName) = 0;
    virtual bool PopCurState(const char* fileName) = 0;
    virtual void SetAllQuestsUnActual() = 0;
    virtual bool InCurLevel(iRPGQuest* _pQuest) = 0;
    virtual bool OnBeginNewLevel() = 0;
};

class zQuestServer : public iQuestServer, public zRPGFrameWork {
public:

    bool m_dirtyState;
    unsigned char m_2D5[3];
    unsigned int m_neededVids[4096];
    int m_iterationType;
    bool m_savedStatePresent;
    unsigned char m_savedStatePadding[3];

    zQuestServer();
    ~zQuestServer() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    void SetDirtyState() override;
    iRPGQuest* FirstQuest(OBJ_TYPE typeValue);
    iRPGQuest* NextQuest();
    iRPGQuest* GetQuest(int questNum, int subQuestNumOrIndex, bool useIndex, bool assertIfMissing) override;
    iRPGQuest* GetLevel(int levelIndex) override;
    int GetLevelsNum() override;
    int GetQuestNum() override;
    int GetSubQuestNum(int questNum) override;
    bool SetQuestState(iRPGQuest* _pQuest, int state, bool validateTransition) override;
    bool GetDirtyState() override;
    int QuestToId(int questNum, int subQuestNum) override;
    iRPGQuest* IdToQuest(int packedKey) override;
    void ResetAllQuests() override;
    const char* GetSubQuestName(iRPGQuest* _pQuest) override;
    int GetPrevQuestMaxTimeLine(iRPGQuest* _pQuest) override;
    bool DeleteQuest(iRPGQuest* _pQuest) override;
    int GetLocalNum(iRPGQuest* _pSubQuest) override;


    iRPGQuest* CreateNewSubQuest(int levelNum, int questNum) override;
    iRPGQuest* CreateNewQuest(int levelNum) override;
    iRPGQuest* CreateNewLevel(int levelIndex) override;
    bool SaveObjsToFile(std::FILE* stream, iRPGFrameWork* pMainFramework, OBJ_TYPE objectType);
    bool SaveQuestsToFile(const char* _sFileName, iRPGFrameWork* _pMainFramework) override;

    bool SetCurLevel(iRPGQuest* _pCurLevel) override;
    bool OnBeginNewLevel() override;
    void CheckFlagmanCoord(int x, int y) override;
    void CheckStateItemName(CSI_ACTION action, const char* itemName) override;
    void CheckStateItemVid(CSI_ACTION action, int vid) override;
    void CheckNamedMonsters(int namedMonsterId) override;
    void AddBonuses(iRPGQuest* pQuest) override;
    bool AreLinked(iRPGQuest* firstQuest, iRPGQuest* secondQuest, bool reverse) override;
    void AddNeededVid(int vid) override;
    void PushCurState(const char* fileName) override;
    bool PopCurState(const char* fileName) override;
    void SetAllQuestsUnActual() override;
    bool InCurLevel(iRPGQuest* pQuest) override;

    int GetQuestState(iRPGQuest* _pQuest) override;
    int QuestToId(iRPGQuest* _pQuest) override;
};

void __cdecl GetNew_zQuestServer(iQuestServer** outObject);
