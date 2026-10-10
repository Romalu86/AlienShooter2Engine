#pragma once

#include <stddef.h>
#include "irpg.h"
#include "zm/Utils/zRefObject.h"

#if !defined(_MSC_VER) && !defined(__thiscall)
#define __thiscall
#endif

class zInventory;
class zInventory2D;
class iInventory;
class iInvKeeper;
class iRPGFrameWork;
enum OBJ_TYPE : int;
enum OBJ_EXTTYPE : int;


class zItem : public iRPGObj, public zRefObject
{
public:
    int m_gameObjectType;
    int m_gameObjectExtType;
    zItem* m_pBaseItem;
    int m_extType;
    zInventory* m_pInventory;
    zInventory2D* m_pInventory2D;
    int m_addLevel;
    int m_addLevelRandom;
    int m_cost;
    int m_width;
    int m_height;
    int m_numInCell;
    int m_maxNumInCell;
    int m_buyNumber;
    unsigned char m_justCreated;
    unsigned char m_itemPadding[3];
    int m_shopNumber0;
    int m_shopNumber1;
    int m_maxShop;
    int m_itemType;

    explicit zItem(OBJ_TYPE objectType);
    virtual ~zItem();

    void* QueryInterface(int interfaceId) const override;
    OBJ_EXTTYPE GetExtType() const override;
    OBJ_TYPE GetType() const override;
    int GetId() const override;
protected:
    int GetShopNum(int shop) override;
public:
    int GetAddLevel() override;
    int GetAddMoney() override;
    int GetGameObj() override;
    void GetSize2DCalced(int& outWidth, int& outHeight, iInventory* inventory) const override;
protected:
    bool ParseUnknownCommand(const char* key, int valueCount, const int* values) override;
    bool ParseUnknownCommand(const char* section, int valueCount, const char** values, iRPGFrameWork* framework) override;
public:
    void SetNumInCell(int _iNumInCell) override;
    int AddNumInCell(int value) override;
    int GetNumInCell() const override;
    int GetBuyNum() const override;
    int GetGrndTakeNum() const override;
    bool HaveInv() const override;
    void GetSize2DBase(int& outWidth, int& outHeight) const override;
    bool IsInfinite() const override;
    int GetMaxNumInCell() const override;
protected:
    bool IsBetterEx(zItem* _pItem) override;
    const iInvKeeper* GetKeeper() const override;
public:
    int GetBaseId() const override;
    bool IsBaseObject() const override;
    int GetInvId() const override;
    int GetBuyCost() const override;
    int GetSellCost() const override;
    int GetBuyCostFull() const override;
    int GetSellCostFull() const override;
    int GetBuyNumShop() const override;
protected:
    int GetMaxShop(int level) override;


    void CopyParametersFrom(zItem* _pBaseItem);
private:
    static bool ms_bAddNumToBase;
    static bool s_bBuyShopNum;

    void SetMaxNumInCell(int maxItemCount);
    void SetBuyNumber(int buyCount);
public:
    int TryToAddItem(zItem* other, bool allowPartial);
    void SetInventory(zInventory* inventory);
    bool IsBetter(zItem* _pItem, bool enforcePairChecks);
};

