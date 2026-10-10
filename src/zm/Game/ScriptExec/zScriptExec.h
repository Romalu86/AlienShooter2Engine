#pragma once
#include <stddef.h>
#include "mylib.h"
#include "basetype.h"
#include "zm/Utils/String/zString.h"
#include "zm/Eng/zProfLdr/zProfMngr.h"
#include "zm/Game/zVidCopier.h"
#include "zm/Game/RecordDB/zRecordDB.h"
class iRPGObj;
class iRPGHero;
class PLAYER_ARCADE;
class SPRITE;
class zString;
class iQuestServer;
class iShopDlgMenu;
class iShopItemInfo;
class iHeroInvMenu;
class iParamDlgMenu;
class iSurvive;
class iCampaign;

struct zScriptExecIntVector : AS_RECORD_DWORD_VECTOR
{
    zScriptExecIntVector() noexcept;
    ~zScriptExecIntVector() noexcept(false);
};

struct ScriptExecMapNode
{
    ScriptExecMapNode* left;
    ScriptExecMapNode* parent;
    ScriptExecMapNode* right;
    int key;
    int value;
    unsigned int color;

    ScriptExecMapNode*& LeftLink() { return left; }
    ScriptExecMapNode*& ParentLink() { return parent; }
    ScriptExecMapNode*& RightLink() { return right; }
    unsigned int& ColorValue() { return color; }
};

struct ScriptExecMapControl
{
    static ScriptExecMapNode* _Nil;
    static unsigned int _Nilrefs;

    unsigned char allocatorState[4];
    ScriptExecMapNode* head;
    unsigned char policyState[4];
    unsigned int size;

    ScriptExecMapControl();
    ~ScriptExecMapControl() noexcept(false);
    void insert(int key, int value);
    int find(int key) const;
};

struct zScriptExecByteVector
{
    zScriptExecByteVector() noexcept;
    ~zScriptExecByteVector() noexcept(false);
    unsigned char allocatorState[4];
    unsigned char* begin;
    unsigned char* end;
    unsigned char* capacityEnd;
};

class zScriptExec : public zVidCopier
{
public:

    static iRPGFrameWork* m_pRPGFrameWork;
    static iQuestServer* m_pQuestServer;
    static iShopDlgMenu* m_pShopDlgMenu;
    static iShopItemInfo* m_pShopItemInfo;
    static iHeroInvMenu* m_pHeroInvMenu;
    static iParamDlgMenu* m_pParamDlgMenu;

    iRef<iRPGObj> m_selectedEquipmentRef;
    unsigned int m_modeFlags;
    AS_RECORD m_hallRecord;
    zScriptExecIntVector m_C0Vector;
    ScriptExecMapControl m_lifetimeValues;
    unsigned char m_loadHeroFromMaxSave;
    unsigned char m_E1[3];
    iItemTaker* m_itemTaker;
    int m_curAmmoDir;
    unsigned int m_reloadBarDirection;
    unsigned char m_flagmanControlEnabled;
    unsigned char m_F1[3];
    zString m_debugText;
    unsigned int m_messageAvailableTick;
    unsigned int m_messageId;
    unsigned int m_mapType;
    zString m_pendingText;
    unsigned int m_radioIconDirection;
    unsigned int m_radioIconVid;
    unsigned int m_radioFontVid;
    unsigned char m_radioMessageDirty;
    unsigned char m_11D[3];
    zScriptExecIntVector m_flyIconDwords[6];
    zScriptExecByteVector m_flyIconTags;
    SPRITE* m_ctrlWheelsCar;
    unsigned int m_lastFlyIconTick;
    zProfMngr m_profileManager;
    iRPGParamDialog* m_paramDialog;
    unsigned char m_showSurvivalHero;
    unsigned char m_1D1[3];

    static unsigned int __cdecl RegisterSurviveReference();
    static unsigned int __cdecl RegisterCampaignReference();

    zScriptExec();
    virtual ~zScriptExec() override;

    friend class PLAYER_ARCADE;

private:
    static iRef<iSurvive>& m_pSurvive;
    static iRef<iCampaign>& m_pCampaign;
    static void __cdecl DestroySurviveReference();
    static void __cdecl DestroyCampaignReference();

    int DirFromRPGObj(const iRPGObj* source);
    void SwitchHeroCS(bool enabled);
    iRPGHero* GetHeroForShow();
    void CreateContSurvHero();
    zProfMngr* ProfMngr()
    {
        return &m_profileManager;
    }
    const zProfMngr* ProfMngr() const
    {
        return &m_profileManager;
    }

protected:
    void SetDeathMatchMode(bool enabled);
    void SetSurviveMode(bool enabled, const char* name);
    void AddFlyIcon(int vid, int direction, int count, int x, int y, int z, bool tag);
    void ClearFlyIcons();
    void GetHotKey(STRING& hotKeyText, int actionIndex, int bindingIndex);
    virtual void SetCtrlWheelsCar(SPRITE* sprite) override;
    virtual int GetArcadeMod_() const override;

public:
    virtual void* QueryInterface(int interfaceId) const override;
    bool ExecCommand(int command, int argument1, int argument2, int argument3, int& outResult, STRING& outText, SPRITE*& outSprite);
    void ExecCommandC(int command, const char* text, int& outResult, STRING& outText);
    bool ExecCommandA(int command, int argument1, int argument2, int argument3, int& outResult, STRING& outText);
    void ReleaseArmyHero();

};


unsigned int ScriptexecZscriptexec_StaticCleanupRegister01();

unsigned int* __cdecl zScriptExec_DwordRangeCopy(
    const unsigned int* sourceBegin, const unsigned int* sourceEnd, unsigned int* destination);


void  Tidy(void* self, unsigned char releasePayload);
void  StdString_CopyStorage(void* self, unsigned int requested);
unsigned char  Grow(void* self, unsigned int requested, unsigned char releaseShared);


