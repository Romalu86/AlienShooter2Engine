#pragma once

#include <stddef.h>
#include "irpg.h"
#include "zm/Utils/String/zString.h"

class zItem;
class iInvKeeper;
enum OBJ_TYPE : int;
enum OBJ_EXTTYPE : int;


class zInventory : public iInventory
{

    friend class zSpriteInventory;
    friend class zInventory3D;
public:
    struct ItemStorage
    {
        unsigned char allocatorState;
        unsigned char padding[3];
        zItem** begin;
        zItem** end;
        zItem** capacity;

        explicit ItemStorage(unsigned char state);
        ~ItemStorage();
    };

    ItemStorage m_apItems;
    unsigned char m_active;
    unsigned char m_activePadding[3];
    int m_inputAction;
    OBJ_TYPE m_objectType;
    OBJ_EXTTYPE m_objectExtType;
    unsigned char m_assertTypeMismatch;
    unsigned char m_dirty;
    unsigned char m_ammoDirty;
    unsigned char m_dirtyPadding;
    iInvKeeper* m_keeper;
    unsigned char m_preserveItemInventory;
    unsigned char m_mergeStacks;
    unsigned char m_searchPadding[2];
    OBJ_TYPE m_searchObjectType;
    OBJ_EXTTYPE m_searchObjectExtType;
    int m_searchIndex;
    zString m_name;
    int m_id;
    unsigned char m_unlimited;
    unsigned char m_bCanPutBaseDebug;
    unsigned char m_inventoryPadding[2];

    explicit zInventory(int inventoryId);
    virtual ~zInventory();
    static void operator delete(void* pointer);

    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    unsigned int GetInpAction() override;
    const char* GetName() const override;
    bool IsActive() override;
    void SetUnlimited(bool enabled) override;
    OBJ_EXTTYPE GetObjectExtType() const override;
    OBJ_TYPE GetObjectType() const override;
    int GetId() const override;

    int GetItemCount() const;
    zItem* GetItem(int index);
protected:
    int GetItemNum(const zItem* item);
public:
    int IfObjectCanBePut(const zItem* item, bool checkCapacity) override;
    int PutItem(zItem* item, zItem** outItem) override;
protected:
    int PutItem(zItem* _pItem, bool mergeStacks, bool checkCapacity, zItem** _pNewItm) override;
public:
    zItem* TakeItem(zItem* _pItem);
    zItem* RemoveItem(int index);
private:
    zItem* TakeItem(int _iItemNum);
protected:
    virtual zItem* DeleteItem(int index) override;
public:
    void RemoveAllItems() override;
    void SetObjectType(OBJ_TYPE objectType, OBJ_EXTTYPE objectExtType, bool debug) override;
    void SetKeeper(iInvKeeper* keeper);
    zItem* FirstItem(OBJ_TYPE objectType, OBJ_EXTTYPE objectExtType);
    zItem* NextItem();
    zItem* GetItemByBaseId(int baseId, int& inOutIndex, bool findLowestRank);
    void SetDirty();
    bool GetDirty();
    bool GetDirtyNotErase();
    void SetAmmoDirty();
    bool GetAmmoDirty();
    bool GetAmmoDirtyNotErase();
protected:
    int CanAddToItems(const zItem* itemToAdd);
private:
    int GetItemByBaseIdCnt(int baseId);
};

