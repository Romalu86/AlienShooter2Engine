#include "mylib.h"
#include "basetype.h"
#include <new>

#include "zInventory.h"
#include "zInventoryItem.h"
#include "zm/Utils/String/zString.h"


static int InventoryPointerRangeCount(zItem* const* begin, zItem* const* end)
{
    return begin == end ? 0 : static_cast<int>(end - begin);
}





zInventory::ItemStorage::ItemStorage(unsigned char state)
    : allocatorState(state), begin(0), end(0), capacity(0)
{
}

zInventory::ItemStorage::~ItemStorage()
{
    CrtFreePointer(begin);
    begin = 0;
    end = 0;
    capacity = 0;
}

zInventory::zInventory(int inventoryId)
    : m_apItems(static_cast<unsigned char>(inventoryId)),
      m_active(0), m_inputAction(65535), m_objectType(static_cast<OBJ_TYPE>(14)), m_objectExtType(static_cast<OBJ_EXTTYPE>(16)),
      m_assertTypeMismatch(0), m_dirty(1), m_ammoDirty(1), m_keeper(0),
      m_preserveItemInventory(0), m_mergeStacks(1), m_searchObjectType(static_cast<OBJ_TYPE>(14)), m_searchObjectExtType(static_cast<OBJ_EXTTYPE>(16)), m_searchIndex(-1),
      m_name(), m_id(inventoryId), m_unlimited(0), m_bCanPutBaseDebug(0)
{
}

zInventory::~zInventory()
{
}

void zInventory::operator delete(void* pointer)
{
    CrtFreePointer(pointer);
}

int zInventory::GetItemCount() const
{
    return m_apItems.begin ? InventoryPointerRangeCount(m_apItems.begin, m_apItems.end) : 0;
}

unsigned int zInventory::GetInpAction()
{
    return static_cast<unsigned int>(m_inputAction);
}

int zInventory::GetId() const
{
    return m_id;
}


bool zInventory::IsActive()
{
    return m_active != 0u;
}


void zInventory::SetUnlimited(bool enabled)
{
    m_unlimited = enabled ? 1u : 0u;
}


OBJ_EXTTYPE zInventory::GetObjectExtType() const
{
    return m_objectExtType;
}


OBJ_TYPE zInventory::GetObjectType() const
{
    return m_objectType;
}


zItem* zInventory::GetItem(int index)
{
    if (index < 0 || !m_apItems.begin)
        return 0;
    const int count = InventoryPointerRangeCount(m_apItems.begin, m_apItems.end);
    return static_cast<unsigned int>(index) < static_cast<unsigned int>(count)
        ? m_apItems.begin[index] : 0;
}


int zInventory::GetItemNum(const zItem* item)
{
    if (!m_apItems.begin)
        return -1;
    const unsigned int count = static_cast<unsigned int>(InventoryPointerRangeCount(m_apItems.begin, m_apItems.end));
    for (unsigned int i = 0; i < count; ++i)
        if (m_apItems.begin[i] == item)
            return static_cast<int>(i);
    return -1;
}


void zInventory::SetObjectType(OBJ_TYPE objectType, OBJ_EXTTYPE objectExtType, bool debug)
{
    m_objectType = objectType;
    m_objectExtType = objectExtType;
    m_assertTypeMismatch = debug ? 1u : 0u;
}

#if !defined(_MSC_VER) && !defined(__cdecl)
#define __cdecl
#endif
#include "myerror.h"

zItem* zInventory::TakeItem(zItem* _pItem)
{
    if (_pItem == 0)
        DebugAssert(4, "_pItem",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp",
            226);

    const int index = GetItemNum(_pItem);
    if (index < 0)
        return 0;

    zItem* removed = DeleteItem(index);
    if (removed != 0 && m_keeper != 0)
        m_keeper->OnTakeItem(removed, this, 0);
    return removed;
}


int zInventory::iRelease()
{
    delete this;
    return 0;
}


const char* zInventory::GetName() const
{
    return static_cast<const char*>(m_name);
}


int zInventory::PutItem(zItem* item, zItem** outItem)
{
    return PutItem(item, m_mergeStacks != 0, true, outItem);
}

void* zInventory::QueryInterface(int interfaceId) const
{
    if (interfaceId == 16 || interfaceId == 46)
        return const_cast<zInventory*>(this);
    return 0;
}

void zInventory::SetKeeper(iInvKeeper* keeper)
{
    m_keeper = keeper;
}


void zInventory::RemoveAllItems()
{
    while (m_apItems.begin && m_apItems.begin != m_apItems.end)
        RemoveItem(SubSigned32Wrap(InventoryPointerRangeCount(m_apItems.begin, m_apItems.end), 1));
}






zItem* zInventory::TakeItem(int _iItemNum)
{
    bool valid = _iItemNum >= 0 && m_apItems.begin != 0;
    if (valid)
    {
        const int count = InventoryPointerRangeCount(m_apItems.begin, m_apItems.end);
        valid = static_cast<unsigned int>(_iItemNum) < static_cast<unsigned int>(count);
    }
    if (!valid)
        DebugAssert(5u, "_iItemNum >= 0 && _iItemNum < m_apItems.size()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 269u);

    zItem** current = m_apItems.begin + _iItemNum;
    zItem* removed = *current;
    zItem** next = current + 1;
    while (next != m_apItems.end)
    {
        *current = *next;
        ++current;
        ++next;
    }
    --m_apItems.end;

    if (m_preserveItemInventory == 0)
        removed->SetInventory(0);
    SetDirty();
    return removed;
}


zItem* zInventory::DeleteItem(int index)
{
    return TakeItem(index);
}


zItem* zInventory::GetItemByBaseId(int baseId, int& inOutIndex, bool findLowestRank)
{
    const int count = m_apItems.begin ? InventoryPointerRangeCount(m_apItems.begin, m_apItems.end) : 0;
    zItem* best = 0;
    for (int index = AddSigned32Wrap(inOutIndex, 1); index < count; index = AddSigned32Wrap(index, 1))
    {
        zItem* item = GetItem(index);
        if (!item)
            DebugAssert(5u, "item", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 362u);
        if (item->GetBaseId() != baseId)
            continue;
        if (best)
        {
            const int bestRank = best->GetNumInCell();
            const int itemRank = item->GetNumInCell();
            if (itemRank >= bestRank)
                continue;
        }
        best = item;
        inOutIndex = index;
        if (!findLowestRank)
            break;
    }
    return best;
}

zItem* zInventory::NextItem()
{
    const int count = m_apItems.begin ? InventoryPointerRangeCount(m_apItems.begin, m_apItems.end) : 0;
    for (; ;)
    {
        m_searchIndex = AddSigned32Wrap(m_searchIndex, 1);
        const int index = m_searchIndex;
        if (index >= count)
            return 0;
        zItem* item = GetItem(index);
        if (!item)
            DebugAssert(4u, "item", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 333u);
        if (m_searchObjectType != 14 && item->GetGameObj() != m_searchObjectType)
            continue;
        if (m_searchObjectExtType != 16 && item->GetAddMoney() != m_searchObjectExtType)
            continue;
        return item;
    }
}


int zInventory::IfObjectCanBePut(const zItem* item, bool checkCapacity)
{
    (void) checkCapacity;
    bool match = true;
    if (m_objectExtType != 16)
        match = const_cast<zItem*>(item)->GetAddMoney() == m_objectExtType;
    if (m_objectType != 14 && (!match || const_cast<zItem*>(item)->GetGameObj() != m_objectType))
        match = false;
    int result = match ? 1 : 0;
    if (!match || m_assertTypeMismatch == 0)
        return result;
    if (const_cast<zItem*>(item)->GetGameObj() != TYPE_AMMO)
        return result;
    const int existingCount = GetItemByBaseIdCnt(item->GetBaseId());
    const int maximumCount = item->GetMaxInvAmount();
    const int remainingCapacity = SubSigned32Wrap(maximumCount, existingCount);
    if (maximumCount == 0)
        return result;
    if (remainingCapacity <= 0)
        return 0;
    if (remainingCapacity < item->GetNumInCell())
        return 4;
    return result;
}

void zInventory::SetDirty()
{
    m_dirty = 1u;
}

bool zInventory::GetDirtyNotErase()
{
    return m_dirty != 0u;
}


void zInventory::SetAmmoDirty()
{
    m_ammoDirty = 1u;
}


bool zInventory::GetAmmoDirtyNotErase()
{
    return m_ammoDirty != 0u;
}


bool zInventory::GetDirty()
{
    const bool dirty = m_dirty != 0u;
    if (dirty)
        m_dirty = 0u;
    return dirty;
}


bool zInventory::GetAmmoDirty()
{
    const bool dirty = m_ammoDirty != 0u;
    if (dirty)
        m_ammoDirty = 0u;
    return dirty;
}







zItem* zInventory::FirstItem(OBJ_TYPE objectType, OBJ_EXTTYPE objectExtType)
{
    m_searchObjectType = objectType;
    m_searchObjectExtType = objectExtType;
    m_searchIndex = -1;
    return NextItem();
}

zItem* zInventory::RemoveItem(int index)
{
    zItem* item = DeleteItem(index);
    if (item && m_keeper)
        m_keeper->OnTakeItem(item, this, 1);
    return item;
}

int zInventory::GetItemByBaseIdCnt(int baseId)
{
    int cursor = -1;
    int total = 0;
    for (zItem* item = GetItemByBaseId(baseId, cursor, false); item; item = GetItemByBaseId(baseId, cursor, false))
        total = AddSigned32Wrap(total, item->GetNumInCell());
    return total;
}


int zInventory::CanAddToItems(const zItem* itemToAdd)
{
    int remaining = itemToAdd->GetNumInCell();

    for (unsigned int i = 0; m_apItems.begin &&
        i<static_cast<unsigned int>(InventoryPointerRangeCount(m_apItems.begin, m_apItems.end)); ++i)
    {
        zItem* item = GetItem(static_cast<int>(i));
        const int itemBaseId = item->GetBaseId();
        const int targetBaseId = itemToAdd->GetBaseId();
        if (targetBaseId == itemBaseId)
        {
            const int itemLimit = item->GetMaxNumInCell();
            const int itemCount = item->GetNumInCell();
            const int delta = SubSigned32Wrap(itemCount, itemLimit);
            remaining = AddSigned32Wrap(remaining, delta);
            if (remaining <= 0)
                return 3;
        }
    }
    return remaining > 0 ? 0 : 3;
}

int zInventory::PutItem(zItem* _pItem, bool mergeStacks, bool checkCapacity, zItem** _pNewItm)
{
    if (!_pItem || _pItem->GetNumInCell() <= 0)
        DebugAssert(5u, "_pItem && _pItem->GetNumInCell() > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 150u);

    const bool bPutBase = _pItem->IsBaseObject();
    if (bPutBase && !m_bCanPutBaseDebug)
        DebugAssert(5u, "!bPutBase || m_bCanPutBaseDebug", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 153u);

    if (GetItemNum(_pItem) != -1)
        DebugAssert(4u, "ItemWasAlreadyHere = ( GetItemNum( _pItem ) == -1 )", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 156u);

    int result = 0;
    const int originalCount = _pItem->GetNumInCell();
    const int placement = IfObjectCanBePut(_pItem, checkCapacity);
    if (!placement)
        return 0;

    if (placement & 4)
    {
        const int currentByBaseId = GetItemByBaseIdCnt(_pItem->GetBaseId());
        const int allowedByBaseId = _pItem->GetMaxInvAmount();
        const int CntToAdd = SubSigned32Wrap(allowedByBaseId, currentByBaseId);
        if (CntToAdd <= 0 || CntToAdd >= _pItem->GetNumInCell())
            DebugAssert(4u, "CntToAdd > 0 && CntToAdd < _pItem->GetNumInCell()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 172u);
        _pItem->SetNumInCell(CntToAdd);
    }

    if (mergeStacks)
    {
        for (unsigned int index = 0; m_apItems.begin &&
            index < static_cast<unsigned int>(InventoryPointerRangeCount(m_apItems.begin, m_apItems.end)); ++index)
        {
            zItem* existing = GetItem(BitsToSigned32(index));
            result |= existing->TryToAddItem(_pItem, true);
            if (result & 1)
                break;
        }
    }

    if (result & 1)
    {
        result &= ~4;
    }
    else
    {
        zItem* placedItem = _pItem;
        if (bPutBase)
        {
            iRPGObj* copiedObject = _pItem->GetObjCopy();
            placedItem = static_cast<zItem*>(copiedObject->QueryInterface(49));
            if (!_pNewItm)
                DebugAssert(5u, "_pNewItm", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory.cpp", 196u);
            else
                *_pNewItm = placedItem;
        }

        if (!m_preserveItemInventory)
            placedItem->SetInventory(this);

        if (static_cast<unsigned int>(InventoryPointerRangeCount(m_apItems.end, m_apItems.capacity)) < 1u)
        {
            zItem** const oldEnd = m_apItems.end;
            const int oldCount = m_apItems.begin ? InventoryPointerRangeCount(m_apItems.begin, oldEnd) : 0;
            const int growth = static_cast<unsigned int>(oldCount) > 1u ? oldCount : 1;
            const int newCapacity = AddSigned32Wrap(oldCount, growth);
            const int allocationCapacity = newCapacity < 0 ? 0 : newCapacity;
            zItem** newBegin = static_cast<zItem**>(CrtNew(
                static_cast<unsigned int>(allocationCapacity) * sizeof(*newBegin)));
            zItem** destination = newBegin;
            for (zItem** source = m_apItems.begin; source != oldEnd; ++source, ++destination)
                if (destination)
                    *destination = *source;
            if (destination)
                *destination = placedItem;
            ++destination;
            zItem** const tailEnd = m_apItems.end;
            for (zItem** source = oldEnd; source != tailEnd; ++source, ++destination)
                if (destination)
                    *destination = *source;
            CrtFreePointer(m_apItems.begin);
            m_apItems.capacity = newBegin + newCapacity;
            const int liveCount = m_apItems.begin ? InventoryPointerRangeCount(m_apItems.begin, m_apItems.end) : 0;
            m_apItems.begin = newBegin;
            m_apItems.end = newBegin + AddSigned32Wrap(liveCount, 1);
        }
        else
        {
            *m_apItems.end++ = placedItem;
        }

        result |= 1;
        if (m_keeper)
            m_keeper->OnPutItem(placedItem, this);
    }

    if (result)
        SetDirty();
    if (bPutBase)
        _pItem->SetNumInCell(originalCount);
    return result;
}


