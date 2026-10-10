#pragma once
#include <stddef.h>
#include "zBaseDlgMenu.h"
#include "basetype.h"
#include "zm/Utils/zRefObject.h"

class iShopItemInfo;
class iBaseDlgMenu;
class iRPGFrameWork;
class iRPGHero;
class iHeroInvMenu;
class iInvServer;
class iInvKeeper;
class iPlayer;
class iRPGObj;
enum OBJ_TYPE : int;
enum OBJ_EXTTYPE : int;

class iShopDlgMenu {
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void ReloadAll() = 0;
    virtual void SetRPGFrameWork(iRPGFrameWork* value) = 0;
    virtual void SetRPGHero(iRPGHero* object) = 0;
    virtual void SetShopItemInfo(iShopItemInfo* value) = 0;
    virtual void SetHeroInvMenu(iHeroInvMenu* value) = 0;
    virtual bool ScrollToItem(OBJ_TYPE objectType, OBJ_EXTTYPE extType) = 0;
    virtual bool ScrollToItem(int objectKey, bool unusedArgument) = 0;
    virtual iBaseDlgMenu* AsBaseDlgMenu() = 0;
    virtual void ReleaseListOfObjects() = 0;
    virtual void SetInvServer(iInvServer* object) = 0;
    virtual bool TryToSellSelectedItem(unsigned int mode) = 0;
    virtual bool TryToSellItem(iRPGObj* item, unsigned int mode) = 0;
    virtual void RefreshAll() = 0;
    virtual int GetItemsNum() = 0;
    virtual int GetListItemsNum() = 0;
    virtual int GetFirstShownItemNum() = 0;
    virtual void PageUp() = 0;
    virtual void PageDown() = 0;
    virtual void PageScroll(int value) = 0;
    virtual void SetPlayer(iPlayer* value) = 0;
    virtual void SetCurLevel(int value) = 0;
};

struct zShopListNode { zShopListNode* next; zShopListNode* previous; iRPGObj* value; };

struct zShopListLayout {
    unsigned char m_allocatorState;
    unsigned char m_allocatorPadding[3];
    zShopListNode* m_sentinel;
    unsigned int m_size;

    zShopListLayout();
    ~zShopListLayout();
};

struct zShopObjectPointerVector {
    unsigned char m_allocatorState = 0;
    unsigned char m_allocatorPadding[3];
    iRPGObj** m_begin = 0;
    iRPGObj** m_end = 0;
    iRPGObj** m_capEnd = 0;

    ~zShopObjectPointerVector();
};

class zShopDlgMenu : public iShopDlgMenu, public zBaseDlgMenu {
public:
    zShopDlgMenu();
    ~zShopDlgMenu() override;
    int m_categoryButtonCount;
    SPRITE* m_categoryButtons[9];
    int m_filterButtonCount;
    SPRITE* m_filterButtons[9];
    int m_buyButtonCount;
    SPRITE* m_buyButtons[2];
    int m_buyAnim[2];
    zShopListLayout m_objects;
    zShopObjectPointerVector m_generated;
    bool m_generatedMode;
    unsigned char m_pad8D[3];
    iRPGFrameWork* m_framework;
    iRef<iRPGHero> m_rpgHeroRef;
    iRef<iInvKeeper> m_keeperRef;
    iShopItemInfo* m_shopItemInfo;
    iHeroInvMenu* m_heroInvMenu;
    OBJ_TYPE m_objectType;
    OBJ_EXTTYPE m_extType;
    iRef<iInvServer> m_invServerRef;
    iPlayer* m_player;
    int m_curLevel;


    void* QueryInterface(int id) const override;
    int iRelease() override;
private:
    void Control(INPUT* input) override;
    void ControlInput(INPUT* input) override;
    void DeletePointerToSprite(SPRITE* sprite) override;
    bool ScrollToItem(int objectKey, bool unusedArgument) override;
    bool ScrollToItem(OBJ_TYPE objectType, OBJ_EXTTYPE extType) override;
    void ReleaseListOfObjects() override;
    bool TryToSellSelectedItem(unsigned int mode) override;
    bool TryToSellItem(iRPGObj* _pItem, unsigned int mode) override;
public:
    void SetRPGFrameWork(iRPGFrameWork* frameWork) override;
    void SetHeroInvMenu(iHeroInvMenu* heroInventoryMenu) override;
private:
    void SetPlayer(iPlayer* player) override;
    void SetCurLevel(int level) override;
    iBaseDlgMenu* AsBaseDlgMenu() override;
    void RefreshAll() override;
public:
    void SetShopItemInfo(iShopItemInfo* info) override;
private:
    void PageUp() override;
    void PageDown() override;
    void PageScroll(int scrollPercent) override;
    void BuySellButton1();
    int GetItemsNum() override;
    int GetListItemsNum() override;
    int GetFirstShownItemNum() override;
    void SetInvServer(iInvServer* inventoryServer) override;
    void TryToFindButtons();
public:
    void SetRPGHero(iRPGHero* hero) override;
private:
    int TryToBuyItem(iRPGObj* _pItem);
    bool InitLoad();
public:
    void ReloadAll() override;
private:
    void SetBuyBtnAnim(int secondButtonAnimation, int firstButtonAnimation);
    void RefreshItems();
    void GoHome();
    void GoEnd();
    void BuySellButton2();
    bool GenerateItems();
    void RefreshBuyButtons();
    void LoadListOfObjects(OBJ_TYPE objectType, OBJ_EXTTYPE extType);
};

