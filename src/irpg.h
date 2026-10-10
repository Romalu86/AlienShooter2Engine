#include <cstdio>
#pragma once

#include "zm/RPGHero/iRPGBaseObjectConst.h"

class iRPGObj;
class iRPGHero;
class zItem;
class iInventory;
class iPlayer;
class iItemTaker;
class iQuestServer;
class iInvServer;
class iInvKeeper;
class iHeroInvMenu;
class iString;
class iTextDB;
class INPUT;
class SPRITE;
class zInventory;

extern bool g_bGlobalRelease;

enum OBJ_TYPE : int;
enum OBJ_EXTTYPE : int;

class iRPGFrameWork
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual iRPGObj* GetNewRPGObj(OBJ_TYPE type, bool clearLoadedFlag) = 0;
    virtual iRPGObj* GetObjByNamee(const char* name, bool logIfMissing) = 0;
    virtual iRPGObj* GetGrndObject(int key0, int key1) = 0;
    virtual iRPGObj* GetObjCopyS(const char* name, bool logIfMissing) = 0;
    virtual iRPGObj* GetObjCopy(iRPGObj* object) = 0;


    virtual int GetSkillValue(iRPGObj* object, int arg) = 0;
    virtual int GetSkillValue(int a, int b) = 0;
    virtual const char** GetHeroInfo(int arg) = 0;
    virtual const char* GetItemName(const iRPGObj* object) = 0;
    virtual iRPGObj* FirstObject(int objectMask, int typeOrClass, int extraId) = 0;
    virtual iRPGObj* NextObject() = 0;
    virtual void SetPerkForAllHeroes(int perkId) = 0;
    virtual bool ReleaseObject(iRPGObj* object) = 0;
    virtual iRPGObj* GetItemById(int key) = 0;
    virtual iTextDB* AsTextDB() = 0;
    virtual void SetGammaItemDisabled(int value5C, int value60, int value64) = 0;
    virtual void GetGammaItemDisabled(int* out5C, int* out60, int* out64) = 0;
    virtual void PlaySFXCurArmy(int sfxId) = 0;
    virtual void ResetScriptFunctions() = 0;
    virtual const char* GetTypeName(OBJ_TYPE type) = 0;
    virtual void SetPlayer(iPlayer* value) = 0;
    virtual int ScriptRun(int scriptId, const char* functionName, int argument1, int argument2, int argument3) = 0;
    virtual int ScriptGetNFunction(const char* functionName) = 0;
    virtual void SetPlayerName(const char* name) = 0;
    virtual const char* GetPlayerName() = 0;
    virtual int GetArcadeMod_() = 0;
    virtual int GetItemsCnt() = 0;
    virtual void WinLevel() = 0;
    virtual int GetNoSprites(int vidIndex, unsigned int army) = 0;
    virtual int CheckHashesDebug() = 0;
    virtual void AddBaseName(const char* name, iRPGObj* object) = 0;
};

class iTextDB
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual const char* GetStringValue(int kind, int index) = 0;
    virtual bool BeginDataStringsReading(const char* name) = 0;
    virtual bool BeginObjectInfoReading(const char* fieldName) = 0;
    virtual void AddDataString(const char* value) = 0;
    virtual void EndDataStringsReading() = 0;
    virtual const char** GetExtInfo(int selector, int firstKeyComponent, int secondKeyComponent) = 0;
    virtual const char** GetExtInfo(int selector, const char* key) = 0;
    virtual const char* GetFirstString(int vid, int direction) = 0;
    virtual const char* GetObjInfo(const iRPGObj* object, bool expand) = 0;
    virtual const char* GetObjInfo(const char* itemName) = 0;
    virtual void SetObjInfo(const iRPGObj* object, const char* text) = 0;
    virtual void SetObjInfo(const char* itemName, const char* text) = 0;
    virtual const char* GetSpeciallyDecodedString(const char** lines, int maxLines) = 0;
    virtual const char* GetFormattedSkill(const iRPGObj* object, int type) = 0;
    virtual void SaveInfosToFile(const char* fileName) = 0;
    virtual const char* GetStrIniString(const char* section, const char* key) = 0;
    virtual void SetFileVer(const char* fileName, int fileValue) = 0;
    virtual int GetFileVer(const char* fileName) = 0;
    virtual int GetCurFileVer() = 0;
};

class iRPGParamDialog
{
public:
    virtual void* QueryInterface(unsigned int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual void AddSkill(int paramIndex, int stepCount) = 0;
    virtual const char* GetFormattedSkill(int value) = 0;
    virtual void SetRPGFrameWork(iRPGFrameWork* value) = 0;
    virtual void FirstHero(int expectedSex) = 0;
    virtual void PrevHero() = 0;
    virtual void NextHero() = 0;
    virtual iRPGObj* GetCurHero() = 0;
    virtual void SetHeroExternal(iRPGObj* object) = 0;
    virtual void ChangeCurSex() = 0;
    virtual const char* GetPlayerName() = 0;
    virtual const char* GetHeroInfo() = 0;
    virtual const char* GetObjInfo(iRPGObj* object) = 0;
    virtual const char* GetVidDirInfo(int infoKind, int infoIndex) = 0;
    virtual void AssignSkills() = 0;
    virtual bool CanIncSkill(int index) const = 0;
    virtual bool CanDecSkill(int index) const = 0;
    virtual void SetPerkSkill(int value) = 0;
    virtual int GetPerkSkill() const = 0;
    virtual void RemoveSkillIncrease() = 0;
    virtual void ResetScriptFunctions() = 0;
    virtual void SetPlayeR(iPlayer* value) = 0;
    virtual void RecieveNetPlayers() = 0;
    virtual void SetPlayerMenuType(int value) = 0;
    virtual int GetPlayerMenuType() const = 0;
};

class iInventory
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual unsigned int GetInpAction() = 0;
    virtual bool IsActive() = 0;
    virtual OBJ_TYPE GetObjectType() const = 0;
    virtual OBJ_EXTTYPE GetObjectExtType() const = 0;
    virtual void SetObjectType(OBJ_TYPE type, OBJ_EXTTYPE extType, bool debug) = 0;
    virtual const char* GetName() const = 0;
    virtual void SetUnlimited(bool enabled) = 0;
    virtual int GetId() const = 0;
    virtual ~iInventory()
    {
    }
    virtual int PutItem(zItem* item, zItem** outItem) = 0;
    virtual int PutItem(zItem* item, bool mergeStacks, bool checkCapacity, zItem** outItem) = 0;
    virtual void RemoveAllItems() = 0;
    virtual int IfObjectCanBePut(const zItem* item, bool checkCapacity) = 0;
    virtual zItem* DeleteItem(int index) = 0;
};

class iInventory2D
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual bool SetSizeInCells(int width, int height) = 0;
    virtual void GetSizeInCells(int& outWidth, int& outHeight) = 0;
    virtual int GetCellSizeX() = 0;
    virtual int GetCellSizeY() = 0;
};

class iInventory3D
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
};

class iInvServer
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual void InitializeInventories() = 0;
    virtual iInventory* GetInv(int index) = 0;
    virtual iInvKeeper* Keeper() const = 0;
    virtual void SetRPGHero(iRPGHero* hero) = 0;
    virtual int PutItemEx(iRPGObj* object, iRPGObj** outObject) = 0;
    virtual bool TakeItem(iRPGObj* object) = 0;
    virtual bool TakeItem(int value) = 0;
    virtual iRPGObj* GetArmor() = 0;
    virtual iRPGObj* GetEquip(int index) = 0;
    virtual iRPGObj* GetEquipEx(int compareValue, int inventoryMask) = 0;
    virtual iRPGObj* GetObj(int index, OBJ_TYPE type, OBJ_EXTTYPE extType, int flags) const = 0;
    virtual bool DecAmmo(int value, int amount) = 0;
    virtual bool GetDirty() = 0;
    virtual int GetObjectCnt(int itemType) = 0;
    virtual void SetActCellsCnt(int value) = 0;
    virtual bool ChangeWpn_(int sourceIndex, int destinationIndex) = 0;
    virtual const iRPGObj* GetBestWeapon(bool compareArg) const = 0;
    virtual void SetItemTaker(iItemTaker* value) = 0;
    virtual iRPGObj* GetItemByBaseId(int value, int inventoryMask) = 0;
    virtual int PutItemIfBetter(iRPGObj* item) = 0;
    virtual int IfObjectCanBePut(const iRPGObj* object, bool flags) = 0;
    virtual void CopyParametersFrom(iInvServer* source) = 0;
    virtual void SetInventoryCoord(int index, float x, float y) = 0;
    virtual const iRPGObj* GetTakenObj() const = 0;
    virtual iRPGObj* TryToTakeItem(float x, float y) = 0;
    virtual bool TryToTakeObj(iRPGObj* object, bool forBuy) = 0;
    virtual int TryToPutItem(float x, float y, bool action) = 0;
    virtual int TryToPutItemBack() = 0;
    virtual iRPGObj* GetObjByPoint(float x, float y) const = 0;
    virtual const iInventory* GetInventoryByPoint(float x, float y) const = 0;
    virtual bool CanPutToInv(int index, const iRPGObj* object) const = 0;
    virtual bool GetMouseDirty() = 0;
    virtual void GetMouseDisplacement(float& outX, float& outY) const = 0;
    virtual void SetParent(iHeroInvMenu* parent) = 0;
    virtual iRPGObj* TryToBuyObj(iRPGObj* object, float x, float y) = 0;
    virtual bool PutToActiveInv(float x, float y) = 0;
    virtual bool PutToActiveInv(iRPGObj* object) = 0;
    virtual void ControlGrndInventory(unsigned long tactTime) = 0;
    virtual int GetInvByName(const char* name) = 0;
    virtual void SetPlayer(iPlayer* value) = 0;
    virtual int GetInventoryToPutItem(zItem* item, float x, float y, zItem*& outOwner) = 0;
    virtual bool IsForBuy() = 0;
};

class iPlayer
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual int GetNumInVolley() = 0;
    virtual int GetReloadTimer() = 0;
    virtual int GetCurTime() = 0;
    virtual void SetReloadTimer(int value) = 0;
    virtual void SetNumInVolley(int value) = 0;
    virtual void SetCurAmmoDir(int value) = 0;
    virtual SPRITE* Flagman() = 0;
    virtual void OnAutoTakeItem(iRPGObj* object, int itemCount, int worldX, int worldY, int worldZ) = 0;
    virtual void OnClipReloading() = 0;
    virtual void OnClipReload() = 0;
    virtual void OnAddSkill(int skillType, int value) = 0;
    virtual bool IsReadyForWeaponChange() = 0;
    virtual void GetSpeed(float& speedX, float& speedY) = 0;
    virtual int ScriptRun(int function, const char* script, int scriptArgument1, int scriptArgument2, int scriptArgument3) = 0;
    virtual int ScriptGetNFunction(const char* name) = 0;
    virtual void PlaySFx(int effectId, bool positioned) = 0;
    virtual void StopSFx(int effectId, bool positioned) = 0;
    virtual int GetDelayForRotateAfterFire() = 0;
    virtual iRPGHero* GetPlayerHero() = 0;
    virtual void SetFlagmanDialog(int dialogId) = 0;
    virtual void GetStrIniString(iString* out, const char* keyA, const char* keyB) = 0;
    virtual void SendInfoMessage(int messageId, int timeOffset) = 0;
    virtual int GetArcadeMod_() const = 0;
    virtual int GetArmy() = 0;
    virtual bool IsCurPlayer() = 0;
    virtual void CopyHeroToArmy(iRPGObj* object, int army) = 0;
    virtual void WinLevel() = 0;
    virtual int GetNoSprites(int vidIndex, unsigned int army) = 0;
    virtual void SetHeroPDId(int army, int id, bool force) = 0;
    virtual int GetHeroPDId(int army) = 0;
    virtual void SetHeroPDClr(int army, int color) = 0;
    virtual int GetHeroPDClr(int army) = 0;
};



class iItemTaker
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual bool CreateGroundSprite(iRPGObj* object) const = 0;
    virtual bool DeleteGroundSprite(iRPGObj* object, bool removeSprite) = 0;
    virtual bool DeleteGroundSprite(SPRITE* sprite, bool removeSprite) = 0;
    virtual void DeleteOldItems() = 0;
    virtual void CreateIdsForSprites() = 0;
    virtual void FindIdForSprite(SPRITE* sprite) = 0;
    virtual bool IsItemFarToTake(iRPGObj* object) = 0;
    virtual iRPGObj* FirstItemToTake() = 0;
    virtual iRPGObj* NextItemToTake() = 0;
    virtual SPRITE* CurSpriteToTake() = 0;
    virtual void SetRPGFrameWork(iRPGFrameWork* value) = 0;
    virtual void SetQuestServer(iQuestServer* value) = 0;
    virtual void SetPlayer(iPlayer* value) = 0;
    virtual void KillObjects(int handle) = 0;
    virtual void SetItemTakeDist(float distance) = 0;
    virtual bool IsNeededVid(int vid) = 0;
    virtual void GetGrndSpriteCoor(SPRITE* sprite, int& x, int& y, int& z) = 0;
    virtual void DeletePointerToSprite(SPRITE* sprite) = 0;
    virtual void SetNetRespItem(int index, SPRITE* value) = 0;
    virtual SPRITE* GetNetRespItem(int index) = 0;
    virtual iRPGObj* SetNewItemToSprite(iRPGObj* object, SPRITE* sprite) = 0;
};

class iRPGObj
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual OBJ_EXTTYPE GetExtType() const = 0;
    virtual int GetBaseId() const = 0;
    virtual bool SetSkillValue(int paramIndex, int value) = 0;
    virtual bool IncSkillValue(int paramIndex, int delta) = 0;
    virtual int GetSkillValue(int paramIndex) const = 0;
    virtual int GetSkillValueBase(int paramIndex) const = 0;
    virtual void SetCharValue(int stringParam, const char* value) = 0;
    virtual const char* GetCharValue(int stringParam) const = 0;
    virtual int GetGameObj() = 0;
    virtual int GetAddMoney() = 0;
    virtual void RecalcSkills() = 0;
    virtual bool GetParams(const char* token, int length, int& count, int* indices) const = 0;
    virtual void CopyParametersFrom(iRPGObj* baseObject) = 0;
    virtual bool IsBaseObject() const = 0;
    virtual void SetPerkSkill(int value) = 0;
    virtual int GetPerkSkill() = 0;
    virtual bool GetDirty() = 0;
    virtual bool GetAmmoDirty() = 0;
    virtual void AddParent(iRPGObj* parent) = 0;
    virtual void RemoveParent(iRPGObj* parent, bool noRefresh) = 0;
    virtual void AddChild(iRPGObj* child) = 0;
    virtual void RemoveChild(iRPGObj* child) = 0;
    virtual int GetExtInfoLen() const = 0;
    virtual void OutputExtInfoStr(const char* prefix, int mode, char*& out, int flags) const = 0;
    virtual void Init() = 0;
    virtual int Damage(int value) = 0;
    virtual void BeginArray(int index) = 0;
    virtual bool ParseUnknownCommand(const char* name, int count, const int* values) = 0;
    virtual bool ParseUnknownCommand(const char* name, int count, const char** values, iRPGFrameWork* frameWork) = 0;
    virtual bool IsArrayProperty(int index) = 0;
    virtual void SetRecalcAllSkills(bool value) = 0;
    virtual void SetOwnerFrameWork(iRPGFrameWork* frameWork) = 0;
    virtual iRPGFrameWork* GetFW() = 0;
    virtual bool IfIncreasedByImplant(int paramIndex) = 0;
    virtual bool CanHoldObj(const iRPGObj* object) const = 0;
    virtual bool SaveToFile(std::FILE* output) = 0;
    virtual void SetClone(int clone) = 0;
    virtual int GetClone() const = 0;
    virtual void GetSize2DBase(int& outWidth, int& outHeight) const = 0;
    virtual void GetSize2DCalced(int& outWidth, int& outHeight, iInventory* inventory) const = 0;
    virtual void SetNumInCell(int value) = 0;
    virtual int AddNumInCell(int value) = 0;
    virtual int GetNumInCell() const = 0;
    virtual bool IsInfinite() const = 0;
    virtual const iInvKeeper* GetKeeper() const = 0;
    virtual bool HaveInv() const = 0;
    virtual int GetInvId() const = 0;
    virtual int GetAddLevel() = 0;
    virtual OBJ_TYPE GetType() const = 0;
    virtual int GetMaxNumInCell() const = 0;
    virtual bool ItemIsNotReal() = 0;
    virtual int GetId() const = 0;
    virtual int GetVidNum(int index) const = 0;
    virtual int GetDirNum(int index) const = 0;
    virtual SPRITE* GetGrndSpr() const = 0;
    virtual void SetGrndSpr(SPRITE* sprite) = 0;
    virtual void SetThrowTime(int value) = 0;
    virtual int GetThrowTime() const = 0;
    virtual iRPGObj* GetObjCopy() = 0;
    virtual void SetName(const char* value, bool notifyOwner) = 0;
    virtual const char* GetName() const = 0;
    virtual int GetBuyNum() const = 0;
    virtual int GetGrndTakeNum() const = 0;
    virtual int GetBuyNumShop() const = 0;
    virtual int GetBuyCost() const = 0;
    virtual int GetSellCost() const = 0;
    virtual int GetBuyCostFull() const = 0;
    virtual int GetSellCostFull() const = 0;
    virtual void SetAmmoDirty() = 0;
    virtual int GetShopNum(int shop) = 0;
    virtual int GetMaxShop(int requested) = 0;
    virtual void AddItem(iRPGObj* item) = 0;
    virtual zInventory* GetBPInv() = 0;
    virtual ~iRPGObj() noexcept(false)
    {
    }
    virtual int GetMaxInvAmount() const = 0;
    virtual bool IsBetterEx(zItem* other) = 0;
};

class iRPGAmmo
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
};

class iRPGWeapon
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
};

class iRPGArmor
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual int GetRepairCost() = 0;
    virtual int Repair(int requested) = 0;
};

class iRPGEquip
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual iRPGObj* GetAddAmmo() = 0;
};

class iRPGImplant
{
public:
    static int SeedImplantRand(int value);
    static int GetImplantSeed();
    static unsigned int generationSequence;
    static bool sb_FromGround;
    static bool sb_FromQuestServ;
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;

protected:
    static int g_iSrandVal;
};

class iRPGHeroBase
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual void SetSRandVal(unsigned int value) = 0;
    virtual unsigned int GetSRandVal(bool enabled) = 0;
};

class iInvKeeper
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual void OnTakeItem(zItem* item, iInventory* sender, int flags) = 0;
    virtual void OnPutItem(zItem* item, iInventory* sender) = 0;
    virtual bool CanHoldObj(const iRPGObj* object) const = 0;
    virtual bool CanHoldItm(const zItem* item) const = 0;
    virtual int NoSkillsToHold(const iRPGObj* object, int* outRequirements) const = 0;
    virtual bool CreateGroundSprite(iRPGObj* object) const = 0;
};

class iRPGHero
{
public:
    enum M_MODE : int
    {
        MODE_STAND_FIRM = 0,
        MODE_DEATH_MATCH = 1,
        MODE_CAREER = 2
    };
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual int GetMoney() = 0;
    virtual bool HasMoneyToBuy(const iRPGObj* object) = 0;
    virtual int CanBuyItem(const iRPGObj* object) = 0;
    virtual int BuyItem(iRPGObj* source, bool enableRoute, iRPGObj** outObject, int memberIndex) = 0;
    virtual int PutItem(iRPGObj* object) = 0;
    virtual bool AddMoney(int amount, bool suppressCallback) = 0;
    virtual bool AddHp(int amount, bool flag) = 0;
    virtual int GetHp() const = 0;
    virtual bool SetHpToMax() = 0;
    virtual bool TakeItem(iRPGObj* value) = 0;
    virtual bool TakeItem(int value) = 0;
    virtual iRPGObj* GetCurWpn() const = 0;
    virtual const iRPGObj* GetArmor() = 0;
    virtual const iRPGObj* GetEquip(int value) = 0;
    virtual void AddExperience(int amount, bool notifyPositiveGain) = 0;
    virtual void Control(INPUT* input) = 0;
    virtual void SetPlayer(iPlayer* value) = 0;
    virtual void SetItemTaker(iItemTaker* value) = 0;
    virtual iInvServer* GetInvServer() = 0;
    virtual int GetItemDirty(int key) = 0;
    virtual void SetInvulnerable(bool value) = 0;
    virtual void SetAutoChangeWeapons(bool value) = 0;
    virtual int TryToBuyObj(iRPGObj* object, iRPGObj** outObject, int memberIndex) = 0;


    virtual int TryToSellObjj(iRPGObj* item, bool fullValue) = 0;
    virtual bool CheckIfNoAmmo() = 0;
    virtual void SelectBestWeapon() = 0;
    virtual void CopyParametersFrom(iRPGObj* baseObject) = 0;
    virtual void SetDamageCoeff(float value) = 0;
    virtual int GetUseMedTime() = 0;
    virtual bool CreateGroundSprite(iRPGObj* object) const = 0;
    virtual bool SwitchToMode(M_MODE mode) = 0;
    virtual zInventory* GetCustomInv(M_MODE mode) = 0;
};
