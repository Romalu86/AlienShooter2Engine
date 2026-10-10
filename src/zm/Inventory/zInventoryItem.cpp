#include "mylib.h"

#include "zInventoryItem.h"
#include "zInventory.h"
#include "zInventory2D.h"
#include "myerror.h"

#include "basetype.h"
#include <new>

bool zItem::ms_bAddNumToBase = false;
bool zItem::s_bBuyShopNum = false;

void zItem::SetNumInCell(int _iNumInCell)
{
    if (_iNumInCell < 0)
        DebugAssert(4u, "_iNumInCell >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 211u);
    m_numInCell = _iNumInCell;
}


int zItem::GetMaxNumInCell() const
{
    return m_maxNumInCell;
}


int zItem::GetNumInCell() const
{
    return m_numInCell;
}


bool zItem::IsBaseObject() const
{
    return m_pBaseItem == 0;
}


void zItem::SetMaxNumInCell(int maxItemCount)
{
    m_maxNumInCell = maxItemCount;
    if (m_pInventory != 0)
        DebugAssert(4u, "!m_pInventory", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 281u);
}


void zItem::SetBuyNumber(int buyCount)
{
    m_buyNumber = buyCount;
    m_numInCell = buyCount;
    if (buyCount > m_maxNumInCell)
        m_numInCell = m_maxNumInCell;
    if (m_pInventory != 0)
        DebugAssert(4u, "!m_pInventory", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 303u);
}

zItem::zItem(OBJ_TYPE objectType)
    : zRefObject()
{
    m_gameObjectType = static_cast<int>(objectType);
    m_width = 1;
    m_height = 1;
    m_numInCell = 1;
    m_maxNumInCell = 1;
    m_buyNumber = 1;
    m_justCreated = 1;
    m_pBaseItem = 0;
    m_extType = 0;
    m_pInventory = 0;
    m_pInventory2D = 0;
    m_addLevel = 0;
    m_addLevelRandom = 0;
    m_cost = 0;
    m_maxShop = 0;
    m_itemType = 0;
    m_gameObjectExtType = 16;
    m_shopNumber0 = 3;
    m_shopNumber1 = 3;
}


zItem::~zItem()
{
}




void* zItem::QueryInterface(int interfaceId) const
{
    if (interfaceId == 9 || interfaceId == 49)
        return const_cast<zItem*>(this);
    return zRefObject::QueryInterface(interfaceId);
}


OBJ_EXTTYPE zItem::GetExtType() const
{
    return static_cast<OBJ_EXTTYPE>(m_extType);
}

int zItem::GetGameObj()
{
    return m_gameObjectType;
}

int zItem::GetAddMoney()
{
    return m_gameObjectExtType;
}


int zItem::GetAddLevel()
{
    if (m_addLevelRandom != 0 && m_addLevelRandom > m_addLevel)
    {
        const int delta = SubSigned32Wrap(m_addLevelRandom, m_addLevel);
        const int increment = Random(delta);
        m_addLevel = AddSigned32Wrap(m_addLevel, increment);
    }
    const int result = m_addLevel;
    m_addLevelRandom = 0;
    return result;
}


void zItem::GetSize2DCalced(int& outWidth, int& outHeight, iInventory* inventory) const
{
    zInventory2D* target = m_pInventory2D;
    if (inventory)
        target = static_cast<zInventory2D*>(inventory->QueryInterface(47));

    if (target && target->IfItemAlwaysInCenter())
    {
        static_cast<iInventory2D*>(target)->GetSizeInCells(outWidth, outHeight);
        return;
    }

    GetSize2DBase(outWidth, outHeight);
}


void zItem::CopyParametersFrom(zItem* _pBaseItem)
{
    if (m_justCreated == 0 && IsBaseObject() && (GetClone() & 1) == 0)
        DebugAssert(4u, "m_bJustCreated || !IsBaseObject() || GetClone()&CT_CLONE", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 94u);
    if (this == _pBaseItem)
        DebugAssert(5u, "_pBaseItem != this", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 95u);

    m_justCreated = 0;
    m_cost = _pBaseItem->m_cost;
    int width;
    int height;
    _pBaseItem->GetSize2DBase(width, height);
    m_width = width;
    m_height = height;
    m_numInCell = _pBaseItem->GetNumInCell();
    m_maxNumInCell = _pBaseItem->GetMaxNumInCell();
    m_buyNumber = _pBaseItem->GetBuyNum();
    m_addLevel = _pBaseItem->GetAddLevel();
    m_itemType = static_cast<int>(_pBaseItem->GetType());
    zItem* const sourceBaseItem = _pBaseItem->m_pBaseItem;
    m_pBaseItem = sourceBaseItem;
    if (sourceBaseItem == 0)
        m_pBaseItem = _pBaseItem;
}


bool zItem::ParseUnknownCommand(const char* key, int valueCount, const int* values)
{
    if (CStringEqual(key, "Add_level"))
    {
        m_addLevel = values[0];
        return true;
    }
    if (CStringEqual(key, "Add_level_rand"))
    {
        m_addLevelRandom = values[0];
        return true;
    }
    if (CStringEqual(key, "AddMoney"))
    {
        m_cost = SubSigned32Wrap(0, values[0]);
        return true;
    }
    if (CStringEqual(key, "Cost"))
    {
        m_cost = values[0];
        return true;
    }
    if (CStringEqual(key, "Size"))
    {
        m_width = values[0];
        m_height = values[1];
        return true;
    }
    if (CStringEqual(key, "MaxInv"))
    {
        SetMaxNumInCell(values[0]);
        return true;
    }
    if (CStringEqual(key, "MaxBay"))
    {
        SetBuyNumber(values[0]);
        return true;
    }
    if (CStringEqual(key, "MaxShop"))
    {
        m_maxShop = values[0];
        return true;
    }
    if (CStringEqual(key, "ShopNum"))
    {
        m_shopNumber0 = values[0];
        m_shopNumber1 = valueCount > 1 ? values[1] : 0;
        return true;
    }
    return false;
}


bool zItem::ParseUnknownCommand(
    const char* section, int valueCount, const char** values, iRPGFrameWork* framework)
{
    (void) framework;
    if (!CStringEqual(section, "GameObj") || valueCount != 1)
        return false;

    const char* const itemTypeName = values[0];
    int itemType = 0;
    if (CStringEqual(itemTypeName, "ITEM_FLASH")) itemType = 1;
    else if (CStringEqual(itemTypeName, "ITEM_DRONE")) itemType = 2;
    else if (CStringEqual(itemTypeName, "ITEM_DRONE2")) itemType = 3;
    else if (CStringEqual(itemTypeName, "ITEM_NIGHTVISION")) itemType = 4;
    else if (CStringEqual(itemTypeName, "ITEM_INVULNERABLE")) itemType = 5;
    else if (CStringEqual(itemTypeName, "ITEM_GRENADE")) itemType = 6;
    else if (CStringEqual(itemTypeName, "ITEM_RADAR")) itemType = 7;
    else if (CStringEqual(itemTypeName, "ITEM_TANKSHELL")) itemType = 8;
    else if (CStringEqual(itemTypeName, "ITEM_MEDKIT")) itemType = 9;
    else if (CStringEqual(itemTypeName, "ITEM_DYNAMITE")) itemType = 10;
    else if (CStringEqual(itemTypeName, "ITEM_BACKPACK")) itemType = 11;
    else DebugAssert(3u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 197u);

    m_itemType = itemType;
    return true;
}


int zItem::AddNumInCell(int numToAdd)
{
    if (IsBaseObject() && !ms_bAddNumToBase)
        DebugAssert(4u, "!this->IsBaseObject() || ms_bAddNumToBase", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 222u);

    m_numInCell = AddSigned32Wrap(m_numInCell, numToAdd);
    int notAdded = 0;
    if (m_numInCell < 0)
    {
        notAdded = m_numInCell;
        m_numInCell = 0;
    }
    else if (m_pInventory == 0 || m_pInventory->m_unlimited == 0)
    {
        if (m_numInCell > m_maxNumInCell)
        {
            notAdded = SubSigned32Wrap(m_numInCell, m_maxNumInCell);
            m_numInCell = m_maxNumInCell;
        }
    }

    if (m_pInventory != 0)
    {
        if (GetGameObj() != TYPE_AMMO)
            m_pInventory->SetDirty();
        else
            m_pInventory->SetAmmoDirty();
    }

    if (m_numInCell == 0 && m_pInventory != 0)
        (void) m_pInventory->TakeItem(this);
    return notAdded;
}


bool zItem::IsInfinite() const
{
    if (m_cost != 0)
        return false;
    return const_cast<zItem*>(this)->GetGameObj() == TYPE_AMMO;
}


int zItem::TryToAddItem(zItem* other, bool allowPartial)
{
    const int otherBaseId = other->GetBaseId();
    if (otherBaseId != GetBaseId())
        return 0;

    const int NumToAdd = other->GetNumInCell();
    const int itemLimit = GetMaxNumInCell();
    const int room = SubSigned32Wrap(itemLimit, GetNumInCell());
    if (NumToAdd == 0)
        DebugAssert(4u, "NumToAdd", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 320u);

    int transferResult = 0;
    if (NumToAdd <= room || (m_pInventory != 0 && m_pInventory->m_unlimited != 0))
    {
        (void) AddNumInCell(NumToAdd);
        other->SetNumInCell(0);
        transferResult = 3;
    }
    else if (room != 0 && allowPartial)
    {
        (void) AddNumInCell(room);
        ms_bAddNumToBase = true;
        (void) other->AddNumInCell(SubSigned32Wrap(0, room));
        ms_bAddNumToBase = false;
        transferResult = 6;
    }
    else
    {
        return 0;
    }

    if (m_pInventory != 0)
        m_pInventory->SetDirty();
    return transferResult;
}


void zItem::SetInventory(zInventory* inventory)
{
    m_pInventory = inventory;
    m_pInventory2D = inventory
        ? static_cast<zInventory2D*>(inventory->QueryInterface(47)) : 0;
}


bool zItem::IsBetter(zItem* _pItem, bool enforcePairChecks)
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 362u);

    const iInvKeeper* hero = GetKeeper() ? GetKeeper() : _pItem->GetKeeper();
    if (!hero)
    {
        DebugAssert(4u, "hero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 366u);
        return false;
    }

    if (enforcePairChecks)
    {
        if (!hero->CanHoldItm(this)) return false;
        if (!hero->CanHoldItm(_pItem)) return true;
    }
    return IsBetterEx(_pItem);
}


bool zItem::IsBetterEx(zItem* _pItem)
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventoryItem.cpp", 397u);

    const int otherBaseId = _pItem->GetBaseId();
    return GetBaseId() > otherBaseId;
}


const iInvKeeper* zItem::GetKeeper() const
{
    return m_pInventory ? m_pInventory->m_keeper : 0;
}


int zItem::GetBaseId() const
{
    return m_pBaseItem ? m_pBaseItem->GetBaseId()
        : static_cast<int>(GetExtType());
}


int zItem::GetInvId() const
{
    return m_pInventory ? m_pInventory->GetId() : -1;
}


int zItem::GetBuyCost() const
{
    const int baseCost = GetBuyNumShop();
    const int product = MultiplySigned32Wrap(baseCost, m_cost);
    return product / m_buyNumber;
}


int zItem::GetSellCost() const
{
    return GetBuyCost();
}


int zItem::GetBuyCostFull() const
{
    s_bBuyShopNum = true;
    const int buyCost = GetBuyCost();
    s_bBuyShopNum = false;
    return buyCost;
}


int zItem::GetSellCostFull() const
{
    s_bBuyShopNum = true;
    const int sellCost = GetSellCost();
    s_bBuyShopNum = false;
    return sellCost;
}


int zItem::GetBuyNumShop() const
{
    if (s_bBuyShopNum)
        return GetNumInCell();
    return m_buyNumber;
}


int zItem::GetMaxShop(int level)
{
    int shopCount = SubSigned32Wrap(level, GetAddLevel());
    if (shopCount > m_maxShop)
        shopCount = m_maxShop;
    return shopCount <= 0 ? 1 : shopCount;
}


