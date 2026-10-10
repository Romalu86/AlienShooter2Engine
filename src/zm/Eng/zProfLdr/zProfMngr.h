#pragma once
#include <stddef.h>
#include "zm/Utils/zRefObject.h"
class iRPGObj;
class iRPGHero;
class iRPGFrameWork;
class iQuestServer;
class DIALOG;
enum SAVE_TYPE : int;

class zProfMngr {
public:

    struct LOG_DLG
    {
        int dialog;
        int phrase;
        bool flags;
        unsigned char padding[3];
    };

    struct LOG_DLG_VECTOR
    {
        unsigned char allocatorState[4];

        LOG_DLG_VECTOR();
        ~LOG_DLG_VECTOR() noexcept(false);
        int size() const;
        void push_back(const LOG_DLG& value);
        LOG_DLG* begin;
        LOG_DLG* end;
        LOG_DLG* capacityEnd;
    };

    iRef<iRPGHero> m_cloneRef;
    iRPGFrameWork* m_pRPGFrameWork;
    iRef<iRPGObj> m_armyHeroRef;
    LOG_DLG_VECTOR m_logs;
    bool m_loadedFromMax;
    unsigned char m_031[3];

    zProfMngr();
    virtual void* QueryInterface(int interfaceId) const;
    virtual int iRelease();
    virtual ~zProfMngr();
    void OnLevelBegin(iRPGObj* _pHero, iQuestServer* questServer, bool loadFromMax);
    void OnLevelWin(iRPGObj* _pHero, iQuestServer* questServer, bool surviveMode);
    void CopyHeroToArmy(iRPGObj* _pBaseHero, int heroColor);
    iRPGObj* GetArmyHero(bool survival, bool keepMissing, bool loadFromMax);
    void ReleaseArmyHero();
    bool AddLog(int dialogId, int phraseIndex, bool flags);
    int GetLogsNum();
    int GetLogFaceDir(int logIndex);
    const char* GetLogText(int logIndex);
    int GetRealLogNum(int logIndex);

private:
    iRPGObj* LoadHeroFromSave(SAVE_TYPE _eSType, bool loadFromMax);
    void ReCreateClone();
    bool CopyObjToHero(iRPGObj* source, iRPGObj* _pHero);
    const DIALOG* GetDialog(int logIndex);
    void ClearLogs();
    void LoadLogs();
    void SaveLogs();
};
