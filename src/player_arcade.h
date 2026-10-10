#pragma once


#include <stddef.h>
#include "player.h"
#include "zm/Utils/zRefObject.h"
#include "zm/Game/ScriptExec/zScriptExec.h"
#include "terrain.h"
#include "message.h"
#include "zm/Utils/zStaticStorage.h"

class SPRITE;
struct PTR_SPRITE;
class iRPGHero;
class iRPGObj;
class iString;
class INPUT;
class iBriefingDlgMenu;

enum CSI_ACTION : int
{
    CSI_ACTION_DEATH  = 1,
    CSI_ACTION_CREATE = 2
};

struct PLAYER_ARCADE_PTR_VECTOR
{
    explicit PLAYER_ARCADE_PTR_VECTOR(unsigned char allocatorValue);
    ~PLAYER_ARCADE_PTR_VECTOR() noexcept(false);
    unsigned char allocatorState[4];
    PTR_SPRITE* begin;
    PTR_SPRITE* end;
    PTR_SPRITE* capacityEnd;
};

struct PLAYER_ARCADE_DWORD_VECTOR
{
    explicit PLAYER_ARCADE_DWORD_VECTOR(unsigned char allocatorValue);
    ~PLAYER_ARCADE_DWORD_VECTOR() noexcept(false);
    unsigned char allocatorState[4];
    unsigned int* begin;
    unsigned int* end;
    unsigned int* capacityEnd;
};

class PLAYER_ARCADE : public PLAYER, public zRefObject, public zScriptExec, public iQuestEvent
{
public:

    unsigned char m_parametersDirty;
    unsigned char m_reserved339[3];
    unsigned int m_prevInventoryAction;
    unsigned int m_lastQuestCoordTick;
    unsigned int m_lastInventoryTick;
    SPRITE* m_pCarToStop;

    PLAYER_ARCADE_PTR_VECTOR m_hypnSprites;
    PLAYER_ARCADE_DWORD_VECTOR m_hypnTimes;

    PLAYER* PlayerSubobject()
{
    return static_cast<PLAYER*>(this);
}
    const PLAYER* PlayerSubobject() const
{
    return static_cast<const PLAYER*>(this);
}
    zRefObject* RefObjectSubobject()
{
    return static_cast<zRefObject*>(this);
}
    const zRefObject* RefObjectSubobject() const
{
    return static_cast<const zRefObject*>(this);
}
    zScriptExec* ScriptExecSubobject()
{
    return static_cast<zScriptExec*>(this);
}
    const zScriptExec* ScriptExecSubobject() const
{
    return static_cast<const zScriptExec*>(this);
}
    iPlayer* PlayerInterface()
{
    return static_cast<iPlayer*>(this);
}
    const iPlayer* PlayerInterface() const
{
    return static_cast<const iPlayer*>(this);
}
    iQuestEvent* QuestEventSubobject()
{
    return static_cast<iQuestEvent*>(this);
}


    virtual void* QueryInterface(int interfaceId) const override;
    virtual int iRelease() override;
    static unsigned int __cdecl InitializeMessage();
    explicit PLAYER_ARCADE(unsigned int mode);
    virtual ~PLAYER_ARCADE() noexcept(false) override;
    void* ScalarDeletingDestructor(unsigned int flags);
    virtual void DeletePointerToSprite(SPRITE* sprite) override;
    virtual void PutMessage(const STRING& text,float x,float y) override;
    virtual void AddHypnMonst(SPRITE* sprite,int timeOffset) override;
    virtual void InitForLoadMap() override;
    virtual void Control(INPUT* input) override;
    virtual void CheckRPGHeroChange(bool force) override;

    void SetInventoryCoord(int inventorySlot,float deltaX,float deltaY);

    void SetCurMapType(int mapType,bool surviveMode,const STRING& campaignName,bool deathMatch);


protected:
    virtual bool IsCurPlayer() override;
    virtual int GetQuestState(int group,int quest) override;
    virtual bool SetQuestState(int group,int quest,int state) override;
    virtual STRING GetMouseTipsString() const override;

private:
    static StaticObjectStorage<MESSAGE> m_oMessage;
    static unsigned char messageDestructorFlags;
    static void __cdecl DestroyMessage();

    static iBriefingDlgMenu* m_pBriefingDlgMenu;
    static int m_iDeHypnMonsters;

    void CreateAll();
    void DrawCarDebug();
    void ReleaseAll();
    virtual void DrawSecondaryInfo() override;
    virtual void ReloadDB() override;
    int OnSpriteAction(const SPRITE* sprite,CSI_ACTION action);
    virtual int OnSpriteCreate(SPRITE* sprite) override;
    virtual int OnSpriteDeath(const SPRITE* sprite) override;
    virtual void GetStrIniString(iString* _pString,const char* keyA,const char* keyB) override;
    virtual void SetRadioEx(int radioId,int recordIndex,bool skipRecordCopy) override;
    virtual void SendInfoMessage(int messageId,int timeOffset) override;
    virtual void OnAutoTakeItem(iRPGObj* _pItem,int itemCount,int worldX,int worldY,int worldZ) override;
    virtual void OnAddSkill(int skillType,int value) override;
    void ChangeParameters();
    void DeHypnMonsters();


    void GetSpeed(SPRITE* _pSpr,float& speedX,float& speedY);
    virtual SPRITE* Flagman() override;
    virtual int GetDelayForRotateAfterFire() override;
    virtual iRPGHero* GetPlayerHero() override;
    virtual int GetArmy() override;
    virtual void CopyHeroToArmy(iRPGObj* object,int army) override;
    virtual void OnClipReloading() override;
    virtual void OnClipReload() override;
    virtual int GetNumInVolley() override;
    virtual int GetReloadTimer() override;
    virtual int GetCurTime() override;
    virtual void SetReloadTimer(int value) override;
    virtual void SetNumInVolley(int value) override;
    virtual void SetCurAmmoDir(int value) override;
    virtual void GetSpeed(float& speedX,float& speedY) override;
    virtual void ShowInventory(bool show) override;
    virtual void InventorySetActive(bool active) override;
    virtual int GetDirtyState() override;
    virtual int GetVidDirectionsNum(int index) override;
    virtual int GetFaceDir(int value) override;
    virtual int RecalcCurAmmoClip(bool force) override;
    virtual int GetCurCadr(int object) override;
    virtual bool IsReadyForWeaponChange() override;
    virtual int ScriptRun(int function,const char* script,int scriptArgument1,int scriptArgument2,int scriptArgument3) override;
    virtual int ScriptGetNFunction(const char* name) override;
    virtual void PlaySFx(int effectId,bool positioned) override;
    virtual void StopSFx(int effectId,bool positioned) override;
    virtual void SetFlagmanDialog(int dialogId) override;
    virtual void RemoveFlagmanDialog() override;
    virtual int GetCarSpeed(SPRITE* sprite) override;
    virtual void SetTankCanShot() override;
    virtual void WinLevel() override;
    virtual int GetNoSprites(int vidIndex,unsigned int army) override;
    virtual void SetFlagman(SPRITE* newReference) override;
    virtual void SetHeroPDId(int army,int id,bool force) override;
    virtual int GetHeroPDId(int army) override;
    virtual void SetHeroPDClr(int army,int color) override;
    virtual int GetHeroPDClr(int army) override;

public:
};



