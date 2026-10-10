#include <cstdio>
#pragma once

#include "zRPGHeroBase.h"
#include "../../irpg.h"
#include "../Inventory/zInventory.h"
#include <stddef.h>
#include "../Utils/zRefObject.h"

class zItemTaker;
class zInvServer;
class zRPGFrameWork;
class iRPGFrameWork;
class iRPGObj;
class zRPGWeapon;
class zRPGAmmo;
class zItem;
class iInventory;
class iPlayer;
class iMan;
class zRefObject;
class iRefOwner;
class iItemTaker;
class iInvServer;
class INPUT;

template <>
class iRef<iMan>
{
public:
    iMan* m_object;
    zRefObject* m_refObject;
    iRefOwner* m_owner;

    ~iRef() noexcept(false);
    void Set(iMan* target);
    void Invalidate();
};

class zRPGHero : public iRPGHero, public iInvKeeper, public zRPGHeroBase
{
public:

    struct DIRTY_ITEM
    {
        OBJ_TYPE m_type;
        int m_mode;

        DIRTY_ITEM();
    };

    struct DirtyItemVectorLayout
    {
        DIRTY_ITEM* m_begin;
        DIRTY_ITEM* m_end;
        DIRTY_ITEM* m_capacityEnd;

        ~DirtyItemVectorLayout();
    };

    static int m_iOnGetMoney;
    static int m_iOnGetExperience;
    static int m_iGetExpToGetLevel;
    static bool m__abChangeableParams[MAX_RPGPARAMSCOUNT];
    static bool m__abBaseParams[MAX_RPGPARAMSCOUNT];
    static bool m__abArrayParams[MAX_RPGPARAMSCOUNT];

    unsigned char m_implantIncreased[MAX_RPGPARAMSCOUNT];
    unsigned char m_implantPadding[2];
    iInventory* m_inventories[22];
    int m_iCurWeapon;
    int m_takenWeaponIndex;
    int m_pendingWeaponIndex;
    iPlayer* m_player;
    iItemTaker* m_pItemTaker;
    iInvServer* m_pInvServer;
    zRPGFrameWork* m_pRPGFrameWork;
    unsigned char m_dirtyItemAllocatorState;
    unsigned char m_dirtyVectorPadding[3];
    DirtyItemVectorLayout m_aiDirtyItems;
    unsigned char m_invulnerable;
    unsigned char m_autoChangeWeapons;
    unsigned char m_heroFlagsPadding[2];
    float m_damageCoeff;
    int m_hpFraction;
    iRef<iMan> m_manRef;
    int m_iUseMedTime;
    int m_medUseTimestamp;
    zInventory m_customInventory0;
    zInventory m_customInventory1;
    unsigned char m_unlimitedAmmo;
    unsigned char m_inventoryModePadding[3];

    explicit zRPGHero(iRPGFrameWork* frameWork);
    ~zRPGHero();

    iInvServer* GetInvServer() override;
    int GetUseMedTime() override;
    zInventory* GetCustomInv(M_MODE mode) override;
    void SetAutoChangeWeapons(bool value) override;
    void SetPlayer(iPlayer* value) override;
    void SetInvulnerable(bool value) override;
    void SetDamageCoeff(float value) override;

    bool SetCurWeapon(int requested, bool strictMode);
    int PutItem(iRPGObj* object) override;
    void SelectBestWeapon(int preferredIndex);

    void* QueryInterface(int code) const override;
    int iRelease() override;
    zRPGWeapon* GetCurWeapon() const;
    int GetCurAmmoCnt(bool requireCheck) const;
    int GetAmmoCnt(const zRPGAmmo* value) const;
    bool SetHpBase(int value);
    bool SetHp(int value);
    bool SetNextPrevWeapon(bool previous, bool requireAmmo, bool allowSlotOne);
    bool CheckIfNoAmmo() override;
    int ActDamage(int _iDamage);
    void SetItemDirty(iRPGObj* _pObj, int _iState);
    bool IsWpnClassBetter(iRPGObj* currentInterface, iRPGObj* candidate, bool validateCandidate);
    bool CanHoldItm(const zItem* item) const override;
    int TryToBuyObj(iRPGObj* _pBaseItem, iRPGObj** outObject, int memberIndex) override;
    int TryToSellObjj(iRPGObj* _pItem, bool _bFull) override;
    void SetMan(iMan* next);
    bool AddMoney(int amount, bool suppressCallback) override;
    void AddExperience(int amount, bool notifyPositiveGain) override;
    bool GetDirty() override;
    bool GetAmmoDirty() override;
    int GetExpToGetLevel(int level);
    static void ResetScriptFunctions();
    void SelectBestWeapon() override;
    void IncreasedByImplant(int _iSkillNum, bool value) override;
    bool IfIncreasedByImplant(int _iSkillNum) override;
    bool HasMoneyToBuy(const iRPGObj* obj) override;
    bool AddHp(int _iHpToAdd, bool flag) override;
    void CopyParametersFrom(iRPGObj* _BaseObj) override;
    void OnTakeItem(zItem* _pItem, iInventory* _pSender, int flags) override;
    void UseObject(iRPGObj* _pObj);
    int NoSkillsToHold(const iRPGObj* _pObj, int* outRequirements) const override;
    bool SwitchToMode(M_MODE mode) override;
    int GetNextExpFromExp(int value);
    iRPGObj* GetCurWpn() const override;
    const iRPGObj* GetArmor() override;
    const iRPGObj* GetEquip(int value) override;
    bool SetHpToMax() override;
    bool CanHoldObj(const iRPGObj* value) const override;
    int GetLevelFromExp(int threshold);
    bool IsBaseObject() const override;
    int GetHp() const override;
    bool CreateGroundSprite(iRPGObj* value) const override;
    void OnSkillChange(int action, int oldValue, int value) override;
    int GetItemDirty(int key) override;
    void SetItemTaker(iItemTaker* _pItemTaker) override;
    int CanBuyItem(const iRPGObj* object) override;
    int Damage(int value) override;
    bool UseMedkitIfNeed(int* quota);
    int BuyItem(iRPGObj* source, bool enableRoute, iRPGObj** outObject, int memberIndex) override;
    void OnPutItem(zItem* _pItem, iInventory* _pSender) override;
    void OnBaseSkillChange(int command, int oldValue, int value) override;
    int GetSkillValue(int stat) const override;
    void Control(INPUT* input) override;
    int GetMoney() override;
    bool TakeItem(int value) override;
    bool TakeItem(iRPGObj* value) override;
    void SetDataArrays() override;
    void InitParams() override;
    void DecAmmo(int amount);

    bool ParseUnknownCommand(const char* name, int valueCount, const char** values, iRPGFrameWork* frameWork) override;
    bool SaveInv(std::FILE* output, int selector);
    bool SaveToFile(std::FILE* output) override;
};


