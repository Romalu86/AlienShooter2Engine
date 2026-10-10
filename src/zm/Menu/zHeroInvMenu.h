#pragma once
#include <stddef.h>
#include "zBaseDlgMenu.h"
#include "zm/Utils/zRefObject.h"

class INPUT;
class SPRITE;
class iBaseDlgMenu;
class iItemTaker;
class iInvServer;
class iRPGFrameWork;
class iRPGHero;
class iRPGObj;
class iShopDlgMenu;
class iShopItemInfo;
class iSpriteInventory;


class iHeroInvMenu {
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void SetRPGFrameWork(iRPGFrameWork* framework) = 0;
    virtual void SetShopItemInfo(iShopItemInfo* info) = 0;
    virtual void SetShopDlgMenu(iShopDlgMenu* menu) = 0;
    virtual void SetItemTaker(iItemTaker* taker) = 0;
    virtual void SetRPGHero(iRPGHero* hero) = 0;
    virtual iBaseDlgMenu* AsBaseDlgMenu() = 0;
    virtual void ReloadAll() = 0;
    virtual int TryProcessItemPut(iRPGObj* item, float x, float y, bool alternate) = 0;
    virtual const char* GetMouseTipsString(float x, float y) = 0;
    virtual iRPGObj* GetSelItem() = 0;
    virtual void ResetSelItem() = 0;
    virtual bool GetBuyMode() = 0;
    virtual bool HighLightItem(const iRPGObj* item) = 0;
    virtual void ControlItemInfo(iRPGObj* selection, const INPUT* input) = 0;
    virtual void InitOnShow() = 0;
};

class zHeroInvMenu : public iHeroInvMenu, public zBaseDlgMenu, public iRefOwner {
public:
    explicit zHeroInvMenu(iRPGFrameWork* framework);
    ~zHeroInvMenu() override;

    void* QueryInterface(int id) const override;
    int iRelease() override;
private:
    void OnInvalidate(iRef<iRefObject>* value) override;

public:
    iSpriteInventory* m_inventories[22];
    iRef<iItemTaker> m_itemTakerRef;
    int m_buttonCount;
    SPRITE* m_buttons[22];
    int m_buttonInventoryIndex[22];
    SPRITE* m_extraButtons[30];
    int m_extraButtonCount;
    SPRITE* m_mouseSprite;
    SPRITE* m_hintSprite;
    iShopItemInfo* m_itemInfoWidget;
    iRPGFrameWork* m_framework;
    iShopItemInfo* m_shopItemInfo;
    iShopDlgMenu* m_shopDlgMenu;
    iInvServer* m_invServer;
    iRPGHero* m_rpgHero;
    iRef<iRPGObj> m_highLightRef;
    iRef<iRPGObj> m_hoverRef;
    iRef<iRPGObj> m_selectedRef;
    int m_dragHoldTicks;
private:
    iRPGObj* GetSelItem() override;
    void RemTakenItem();
    bool GetBuyMode() override;
    void TryToFindButtons2();
    void MouseCursorTact(float screenX, float screenY);
    void SetRPGHero(iRPGHero* hero) override;
    void TryToFindButtons();
    void SetRPGFrameWork(iRPGFrameWork* frameWork) override;
    void SetShopItemInfo(iShopItemInfo* shopItemInfo) override;
    void SetShopDlgMenu(iShopDlgMenu* shopDialog) override;
    bool IsShopMode();
    void GetGammaItemDisabled(int& red, int& green, int& blue);
    iBaseDlgMenu* AsBaseDlgMenu() override;
    void DeleteHint();
    void InitOnShow() override;
    void FindButtonsIfNeeded();
    void ControlInventories(INPUT* input);
    void RefreshItems();
    bool SetInputActive(bool active) override;
    void SetItemTaker(iItemTaker* itemTaker) override;
    void ShowItemInfo(iRPGObj* item);
    bool Show(bool show) override;
    const char* GetMouseTipsString(float x, float y) override;
    void HighLightInventories(const iRPGObj* item);
    bool HighLightItem(const iRPGObj* item) override;
    int TryProcessItemPut(iRPGObj* item, float x, float y, bool alternate) override;
    void ShowMouseCursor(const iRPGObj* item);
    void ShowHint(const char* _sMsg, float x, float y, bool localize);
    void Control(INPUT* input) override;
    void ControlInput(INPUT* input) override;
    void ControlItemInfo(iRPGObj* selection, const INPUT* input) override;
    void DeletePointerToSprite(SPRITE* sprite) override;
    void ReloadAll() override;
    void ResetSelItem() override;
};
