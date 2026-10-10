#pragma once

#include <stddef.h>
#include "../../irpg.h"
#include "../Utils/zRefObject.h"

#if !defined(_MSC_VER) && !defined(__thiscall)
#define __thiscall
#endif

class iInvKeeper;
class iInvServer;
class iRPGHero;
class iRPGObj;
class iInventory;
class iPlayer;
class iItemTaker;
class iRefObject;
template <class T> class iRef;
class iHeroInvMenu;
class zItem;
class zInventory;
enum OBJ_TYPE : int;
enum OBJ_EXTTYPE : int;


class zInvServer : public iInvServer, public iRefOwner, public zRefObject
{

public:


    iRef<iRPGHero> m_heroRef;
    zInventory* m_apInv[22];
    class zInventory3D* m_inventory3D[22];
    float m_takeOriginX;
    float m_takeOriginY;
    iItemTaker* m_pItemTaker;
    iRef<zItem> m_takenRef;
    float m_mouseOffsetX;
    float m_mouseOffsetY;
    unsigned char m_mouseDirty;
    unsigned char m_mousePadding[3];
    iHeroInvMenu* m_parentMenu;
    unsigned char m_bForBuy;
    unsigned char m_failedBuyPlacement;
    unsigned char m_playerPadding[2];
    iPlayer* m_player;

    zInvServer();
    virtual ~zInvServer();

    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
private:
    iInvKeeper* Keeper() const override;
    bool IsForBuy() override;
    const zItem* GetTakenItem() const;
    const iRPGObj* GetTakenObj() const override;
    void SetInventoryCoord(int _iNum, float x, float y) override;
    iRPGObj* TryToTakeItem(float x, float y) override;
    int GetInventoryToPutItem(zItem* item, float x, float y, zItem*& outOwner) override;
    int TryToPutItemBack() override;
    zItem* GetItemByPoint(float x, float y) const;
    iRPGObj* GetObjByPoint(float x, float y) const override;
    const iInventory* GetInventoryByPoint(float x, float y) const override;
    bool CanPutToInv(int _iInvNum, const zItem* value) const;
    bool CanPutToInv(int index, const iRPGObj* object) const override;
    int TryToPutItem(float x, float y, bool action) override;

    iRPGObj* TryToBuyObj(iRPGObj* object, float x, float y) override;
    zItem* TryToBuyItem(zItem* _pItem, float x, float y);
    bool TryToTakeObj(iRPGObj* object, bool forBuy) override;
    bool TryToTakeItem(zItem* _pItem, bool forBuy);
    bool PutToActiveInv(float x, float y) override;
    bool PutToActiveInv(iRPGObj* object) override;
    iRPGObj* GetObj(int index, OBJ_TYPE type, OBJ_EXTTYPE extType, int flags) const override;
    zItem* GetItem(int _iInvNum, OBJ_TYPE type, OBJ_EXTTYPE extType, int flags) const;
private:
    bool ChangeWpn_(int _iInvNum1, int _iInvNum2) override;
    bool ChangeWpn_(int _iInvNum1, zItem* item);
    bool ChangeItems(zItem* _pItem1, zItem* _pItem2, bool _bGrnd);
public:
private:
    void OnInvalidate(iRef<iRefObject>* reference) override;
    void SetItemTaker(iItemTaker* _pItemTaker) override;
    void ControlGrndInventory(unsigned long tactTime) override;
    int PutItemEx(iRPGObj* _pObj, iRPGObj** outObject) override;
    bool TakeItem(iRPGObj* _pObj) override;
    bool TakeItem(int baseId) override;
    int GetInvByName(const char* _sName) override;


    void CloneInv(zInventory* _pInvDest, zInventory* _pInvSrc);
    void CopyParametersFrom(iInvServer* source) override;
    iRPGObj* GetArmor() override;
    iRPGObj* GetEquip(int _iInvNum) override;
    bool GetDirty() override;
    int GetObjectCnt(int baseId) override;
    void SetActCellsCnt(int activeCellPercent) override;
    const iRPGObj* GetBestWeapon(bool compareArg) const override;
    iRPGObj* GetEquipEx(int compareValue, int _iInvType) override;
    bool DecAmmo(int baseId, int amount) override;
    iRPGObj* GetItemByBaseId(int baseId, int _iInvType) override;
    int PutItemIfBetter(iRPGObj* _pItem) override;
    int IfObjectCanBePut(const iRPGObj* object, bool flags) override;

    int AddExistingAmmo();

    bool GetMouseDirty() override;
    void GetMouseDisplacement(float& outX, float& outY) const override;
    void SetParent(iHeroInvMenu* parent) override;
    void SetPlayer(iPlayer* player) override;
    iInventory* GetInv(int index) override;
    void SetRPGHero(iRPGHero* hero) override;
    void InitializeInventories() override;
};

