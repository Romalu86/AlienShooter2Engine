#pragma once
#include <stddef.h>
#include "zm/Menu/zBaseDlgMenu.h"
class INPUT;
class SPRITE;
class iRPGFrameWork;
class iRPGParamDialog;
class iRPGHero;
class iHeroInvMenu;
class iSpriteInventory;
class zInventory3D;

class iParamDlgMenu {
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void ControlPlayerMenu(INPUT* input, bool suppressDispatch) = 0;
    virtual void ReloadAll() = 0;
    virtual void SetRPGParamDialog(iRPGParamDialog* dialog) = 0;
    virtual void DeletePointerToSprite(SPRITE* sprite) = 0;
    virtual void SetRPGFrameWork(iRPGFrameWork* value) = 0;
    virtual void SetInvHero(iRPGHero* source) = 0;
    virtual void ControlPerkScreen() = 0;
    virtual void ControlHeroScreen(float value) = 0;
    virtual void RefreshPerkScreen() = 0;
    virtual void RefershHeroScreen() = 0;
};

class zParamDlgMenu : public iParamDlgMenu, public zBaseDlgMenu {
public:
    zParamDlgMenu(iRPGParamDialog* source, iHeroInvMenu* heroInvMenu);
    ~zParamDlgMenu() override;
    iSpriteInventory* m_spriteInventory;
    zInventory3D* m_inventory;
    SPRITE* m_buttons[50];
    int m_buttonCount;
    unsigned int m_0E0;
    SPRITE* m_hoverSprite;
    SPRITE* m_prevHoverSprite;
    SPRITE* m_selectedSprite;
    SPRITE* m_prevSelectedSprite;
    int m_selectedParam;
    SPRITE* m_infoSprite;
    SPRITE* m_leftIcon;
    SPRITE* m_rightIcon;
    iRPGParamDialog* m_paramDialog;
    iRPGFrameWork* m_framework;
    iHeroInvMenu* m_heroInvMenu;


    void SetRPGFrameWork(iRPGFrameWork* frameWork) override;
    void SetRPGParamDialog(iRPGParamDialog* dialog) override;
    void* QueryInterface(int id) const override;
    int iRelease() override;
    int GetParamFromBtn(int buttonIndex, bool perkButton);
    void ControlHeroScreen(float unused) override;
    void ReloadAll() override;
    void DeletePointerToSprite(SPRITE* sprite) override;
    void ControlPerkScreen() override;
    void SetInvHero(iRPGHero* source) override;
    bool IsSelectableBtn(SPRITE* buttonSprite);

    void ControlPlayerMenu(INPUT* input, bool suppressDispatch) override;
    void TryToFindButtons();
    void RefreshPerkScreen() override;
    void CreateInfoTextAndIcons();
    void RefershHeroScreen() override;
    void RefreshScriptVars();
};
