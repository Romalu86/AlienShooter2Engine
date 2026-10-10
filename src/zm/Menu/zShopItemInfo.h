#pragma once
#include "basetype.h"
#include <stddef.h>
#include "zBaseDlgMenu.h"
#include "zm/Utils/zRefObject.h"
class SPRITE;
class INPUT;
class iBaseDlgMenu;
class iInvKeeper;
class iRPGFrameWork;
class iRPGObj;
class iRPGAmmo;
class iShopItemInfo {
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void Control(INPUT* input) = 0;
    virtual void SetScreenCoordinates(float x, float y) = 0;
    virtual void SetRPGFrameWork(iRPGFrameWork* value) = 0;
    virtual void DeletePointerToSprite(SPRITE* value) = 0;
    virtual void ShowObjInfo(iRPGObj* object, bool notify, iBaseDlgMenu* dialog) = 0;
    virtual iRPGObj* GetItemForPut() = 0;
    virtual void SetMaxCoords(int maxX, int maxY) = 0;
    virtual void SetInventoryKeeper(iInvKeeper* value) = 0;
    virtual void SetShowAmmo(bool value) = 0;
    virtual void Refresh() = 0;
    virtual void PageUp() = 0;
    virtual void PageDown() = 0;
    virtual int GetCurPageNum() = 0;
    virtual int GetPagesCnt() = 0;
    virtual void Initialize() = 0;
};


class zShopItemInfo : public iShopItemInfo, public zBaseDlgMenu {
public:
    zShopItemInfo();
    ~zShopItemInfo() override;
    SPRITE* m_sprites[5];
    bool m_limitCoords;
    unsigned char m_pad21[3];
    iRPGFrameWork* m_framework;
    iInvKeeper* m_inventoryKeeper;
    iBaseDlgMenu* m_dialog;
    iRef<iRPGObj> m_currentItem;
    iRef<iRPGAmmo> m_ammoRef;
    iRef<iRPGObj> m_itemForPut;
    int m_screenX;
    int m_screenY;
    int m_maxX;
    int m_maxY;
    bool m_showAmmo;
    unsigned char m_pad65[3];
    int m_curPage;
    int m_pagesCount;
    float m_pageTop;
    unsigned int m_074;
private:
    void SetShowAmmo(bool showAmmo) override;


    void SetInventoryKeeper(iInvKeeper* inventoryKeeper) override;
    void SetRPGFrameWork(iRPGFrameWork* frameWork) override;
    int GetCurPageNum() override;
    void ShowObjInfo(iRPGObj* object, bool notify, iBaseDlgMenu* dialog) override;
    void Refresh() override;
public:
    void* QueryInterface(int id) const override;
    int iRelease() override;
private:
    iRPGObj* GetItemForPut() override;
    void SetMaxCoords(int maxX, int maxY) override;
    void PageUp() override;
    void DeletePointerToSprite(SPRITE* spriteToClear) override;
    void PageDown() override;
    void Initialize() override;
    void SetScreenCoordinates(float x, float y) override;
    int GetPagesCnt() override;
    void ShowObjInfo(iRPGObj* item, bool notify, iBaseDlgMenu* dialog, bool force);
    void Control(INPUT* input) override;
    void GetCoordToOutput(int& x, int& y);
    void DeleteSprites();
    void ShowAmmo(int x, int y, bool releaseOnly);
    void ShowImage(int x, int y, bool releaseOnly);
    void Draw();
};

void __cdecl GetNew_zShopItemInfo(iShopItemInfo** outObject);
