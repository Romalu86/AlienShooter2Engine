#pragma once
#include <stddef.h>
#include "zm/Menu/zBaseDlgMenu.h"
class INPUT;
class SPRITE;
class iRPGFrameWork;


class iBriefingDlgMenu {
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void Control(INPUT* input) = 0;
    virtual void ReloadAll() = 0;
    virtual void DeletePointerToSprite(SPRITE* value) = 0;
    virtual void SetRPGFrameWork(iRPGFrameWork* value) = 0;
};

class zBriefingDlgMenu : public iBriefingDlgMenu, public zBaseDlgMenu {
public:
    zBriefingDlgMenu();
    ~zBriefingDlgMenu() override;

    SPRITE* m_captionSprite;
    SPRITE* m_bodySprite;
    iRPGFrameWork* m_framework;

    void* QueryInterface(int id) const override;
    int iRelease() override;
    void Control(INPUT* input) override;
    void ReloadAll() override;
    void DeletePointerToSprite(SPRITE* spriteToClear) override;
    void SetRPGFrameWork(iRPGFrameWork* frameWork) override;
    void TryToFindButtons();
};

void __cdecl GetNew_zBriefingDlgMenu(iBriefingDlgMenu** outObject);
