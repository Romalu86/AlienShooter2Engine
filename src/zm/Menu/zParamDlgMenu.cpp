
#include "mylib.h"
#include "zm/Inventory/zInventoryItem.h"
#include "zm/Menu/zSpriteInventory.h"
#include "irpg.h"
#include "script.h"
#include "zm/Menu/zParamDlgMenu.h"
#include "sprite.h"
#include "vid.h"
#include "map.h"
#include "menu.h"
#include "zm/Inventory/zInventory.h"
#include "zm/Inventory/zInventory2D.h"
#include "zm/Inventory/zInventory3D.h"
#include "zm/Menu/zShopDlgMenu.h"
#include "zm/Menu/zHeroInvMenu.h"
#include <new>


#include "basetype.h"
class iSpriteInventory;
void __cdecl GetNew_zSpriteInventory(iSpriteInventory** outObject, iRPGFrameWork* constructorArg);


void  zParamDlgMenu::SetRPGFrameWork(iRPGFrameWork* frameWork)
{
    m_framework = frameWork;
}

void  zParamDlgMenu::SetRPGParamDialog(iRPGParamDialog* dialog)
{
    m_paramDialog = dialog;
    ReloadAll();
}


void*  zParamDlgMenu::QueryInterface(int id) const
{
    if (id == 13) return const_cast<zParamDlgMenu*>(this);
    return zBaseDlgMenu::QueryInterface(id);
}

int  zParamDlgMenu::iRelease()
{
    if (this)
    {
        this->~zParamDlgMenu();
        zBaseDlgMenu::operator delete(this);
    }
    return 0;
}


static const int perkButtonParams[10]={22, 23, 24, -1, 26, 27, 28, 29, -1, 31};
static const int heroButtonParams[10]={2, 3, 5, 6, 1, -1, 4, 7, 8, 9};

int  zParamDlgMenu::GetParamFromBtn(int buttonIndex, bool perkButton)
{
    if (perkButton) return perkButtonParams[buttonIndex];
    int ret = heroButtonParams[buttonIndex];
    if (ret == -1) ret = m_paramDialog->GetPerkSkill();
    if (ret<0) DebugAssert(4u, "ret >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zParamDlgMenu.cpp", 549u);
    return ret;
}

void  zParamDlgMenu::ControlHeroScreen(float unused)
{
    (void) unused;
    if (m_buttonCount == 0) TryToFindButtons();
    MENU* const menu = &Map->m_menu;
    if (menu->SelectedSprite() && (menu->ControlFlags()&1u)) RefershHeroScreen();
    if (m_hoverSprite != m_prevHoverSprite) RefershHeroScreen();
    RefreshScriptVars();
    if (m_paramDialog->GetPlayerMenuType() == 0)
        m_spriteInventory->ControlPlayerMenu(0);
}


zParamDlgMenu::~zParamDlgMenu()
{
    m_spriteInventory->iRelease();
    m_spriteInventory = 0;
    m_inventory->iRelease();
    m_inventory = 0;
}
#include "graph.h"

void  zParamDlgMenu::ReloadAll()
{
    m_hoverSprite = 0;
    m_selectedSprite = 0;
    m_buttonCount = 0;

    SPRITE** const owned[3] = {&m_infoSprite, &m_leftIcon, &m_rightIcon};
    for (unsigned int ownedSpriteIndex = 0; ownedSpriteIndex<3u; ++ownedSpriteIndex)
    {
        SPRITE* const sprite = *owned[ownedSpriteIndex];
        if (sprite)
        {
            delete sprite;
            *owned[ownedSpriteIndex] = 0;
        }
    }
    m_spriteInventory->ReloadAll();
}

void  zParamDlgMenu::DeletePointerToSprite(SPRITE* sprite)
{
    if (m_hoverSprite == sprite) m_hoverSprite = 0;
    if (m_prevHoverSprite == sprite) m_prevHoverSprite = 0;
    if (m_selectedSprite == sprite) m_selectedSprite = 0;
    if (m_prevSelectedSprite == sprite) m_prevSelectedSprite = 0;

    SPRITE** const owned[3] = {&m_infoSprite, &m_leftIcon, &m_rightIcon};
    for (unsigned int ownedSpriteIndex = 0; ownedSpriteIndex<3u; ++ownedSpriteIndex)
    {
        if (*owned[ownedSpriteIndex] == sprite)
        {
            *owned[ownedSpriteIndex] = 0;
            m_spriteInventory->DeletePointerToSprite(sprite);
            return;
        }
    }

    if (SpriteInArray(sprite, m_buttons, m_buttonCount)) ReloadAll();
    m_spriteInventory->DeletePointerToSprite(sprite);
}

void  zParamDlgMenu::ControlPerkScreen()
{
    if (m_buttonCount == 0)
        TryToFindButtons();

    MENU* const menu = &Map->m_menu;
    const unsigned int controlFlags = menu->ControlFlags();

    if ((controlFlags&1u) != 0u)
    {
        if (m_prevSelectedSprite)
            m_prevSelectedSprite->ChangeAnimation(0);
        if (m_selectedSprite)
            m_selectedSprite->ChangeAnimation(1);
    }

    if (m_selectedSprite && (menu->ControlFlags()&1u) != 0u)
    {
        int selectedParam = -1;
        for (int buttonIndex = 0; buttonIndex<m_buttonCount; ++buttonIndex)
        {
            SPRITE* const button = m_buttons[buttonIndex];
            if (button != m_selectedSprite)
                continue;

            VID* const vid = button->Vid();
            const int direction = static_cast<int>(vid->RealDirection(button->Direction().value));
            selectedParam = GetParamFromBtn(direction, true);
        }

        if (selectedParam >= 0)
            m_paramDialog->SetPerkSkill(selectedParam);
        else
            m_selectedSprite = m_prevSelectedSprite;

        RefreshPerkScreen();
    }

    if (m_hoverSprite != m_prevHoverSprite)
        RefreshPerkScreen();
}


void  zParamDlgMenu::SetInvHero(iRPGHero* source)
{
    m_inventory->RemoveAllItems();
    iInvKeeper* const keeper = static_cast<iInvKeeper*>(source->QueryInterface(1));
    m_inventory->SetKeeper(keeper);

    const float x = static_cast<float>(static_cast<double>(Graph->screenWidth)*0.5-5.0);
    const float y = static_cast<float>(static_cast<double>(Graph->screenHeight)*0.5-220.0);
    m_inventory->SetCoord(x, y);

    if (!source) return;
    for (unsigned int group = 0; group<19u; ++group)
    {
        if (group == 1u) continue;
        iInvServer* const provider = source->GetInvServer();
        iInventory* const listInterface = provider->GetInv(static_cast<int>(group));
        zInventory* const list = static_cast<zInventory*>(listInterface->QueryInterface(46));
        zItem* item = list->FirstItem(static_cast<OBJ_TYPE>(14), static_cast<OBJ_EXTTYPE>(16));
        while (item)
        {
            iRPGObj* const baseObj = item->GetObjCopy();
            zItem* const child = static_cast<zItem*>(baseObj->QueryInterface(49));
            (void) m_inventory->PutItem(child, 0);
            item = list->NextItem();
        }
    }
}

void __cdecl GetNew_zParamDlgMenu(iParamDlgMenu** outObject, iRPGParamDialog* paramDialog, iHeroInvMenu* heroInvMenu)
{
    void* const storage = CrtNew(sizeof(zParamDlgMenu));
    zParamDlgMenu* object = 0;
    if (storage)
    {
        try
        {
            object = ::new (storage) zParamDlgMenu(paramDialog, heroInvMenu);
        }
        catch (...)
        {
            CrtFreePointer(storage);
            throw;
        }
    }
    *outObject = object;
}

zParamDlgMenu::zParamDlgMenu(iRPGParamDialog* source, iHeroInvMenu* heroInvMenu)
    : zBaseDlgMenu(), m_spriteInventory(0), m_inventory(0), m_buttonCount(0), m_0E0(0),
      m_hoverSprite(0), m_prevHoverSprite(0), m_selectedSprite(0), m_prevSelectedSprite(0),
      m_selectedParam(-1), m_infoSprite(0), m_leftIcon(0), m_rightIcon(0),
      m_paramDialog(0), m_framework(0), m_heroInvMenu(heroInvMenu)
{
    void* const widget = CrtNew(sizeof(zInventory3D));
    if (widget)
    {
        try
        {
            m_inventory = ::new (widget) zInventory3D(-1);
        }
        catch (...)
        {
            CrtFreePointer(widget);
            throw;
        }
    }
    m_inventory->m_mergeStacks = 0u;
    m_inventory->m_preserveItemInventory = 1u;
    m_inventory->m_active = 0u;
    m_inventory->SetCellSize(37, 37);
    (void) m_inventory->SetActiveCellsCnt(100);
    (void) m_inventory->SetSizeInCells(2, 20);
    m_inventory->SetItemAlwaysInCenter(false);
    GetNew_zSpriteInventory(&m_spriteInventory, 0);
    m_spriteInventory->SetInventory(m_inventory);
    SetRPGParamDialog(source);
}

void  zParamDlgMenu::ControlPlayerMenu(INPUT* input, bool suppressDispatch)
{
    if (m_buttonCount == 0) return;
    MENU* const menu = &Map->m_menu;
    SPRITE* const picked = menu->SelectedSprite();
    if ((menu->ControlFlags()&1u) != 0u && IsSelectableBtn(picked))
    {
        if (picked != m_selectedSprite)
        {
            m_prevSelectedSprite = m_selectedSprite;
            m_selectedSprite = picked;
        }
    }
    m_prevHoverSprite = m_hoverSprite;
    if (m_hoverSprite != picked && (!picked || IsSelectableBtn(picked))) m_hoverSprite = picked;
    if (m_hoverSprite != m_prevHoverSprite && (!m_hoverSprite || IsSelectableBtn(m_hoverSprite)))
    {
        if (m_prevHoverSprite && m_prevHoverSprite->Vid()->nVid == 713 && m_prevHoverSprite != m_selectedSprite)
            m_prevHoverSprite->ChangeAnimation(0);
        if (m_hoverSprite && m_hoverSprite->Vid()->nVid == 713)
            m_hoverSprite->ChangeAnimation(1);
    }
    if (m_paramDialog->GetPlayerMenuType() == 0 && !suppressDispatch)
    {
        zItem* const found = m_inventory->GetItemByPoint(input->ClientMouseXY().x, input->ClientMouseXY().y);
        iRPGObj* resolved = 0;
        if (found) resolved = static_cast<iRPGObj*>(found->QueryInterface(9));
        m_heroInvMenu->ControlItemInfo(resolved, input);
    }
}

unsigned int __cdecl zParamDlgMenu_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


bool  zParamDlgMenu::IsSelectableBtn(SPRITE* buttonSprite)
{
    if (!buttonSprite) return false;
    const VID* buttonVid = buttonSprite->Vid();
    const unsigned int vidId = static_cast<unsigned int>(buttonVid->nVid);
    return vidId == 712u || vidId == 713u;
}

void  zParamDlgMenu::TryToFindButtons()
{
    m_buttonCount = 0;
    MENU* const menu = &Map->m_menu;
    const int menuSpriteCount = menu->m_count;
    for (int menuSpriteIndex = 0; menuSpriteIndex<menuSpriteCount; ++menuSpriteIndex)
    {
        SPRITE* const sprite = menu->m_data[menuSpriteIndex];
        const int vidId = sprite->Vid()->nVid;
        if (vidId == 712 || vidId == 713) m_buttons[m_buttonCount++] = sprite;
    }
    Sort(m_buttons, m_buttonCount);
}

void  MAP::SetScriptVariable(const STRING& name, STRING source)
{
    m_script.SetVariableStr(name, source);
}


void  zParamDlgMenu::RefreshPerkScreen()
{
    if (m_buttonCount == 0)
    {
        TryToFindButtons();
        if (m_buttonCount == 0) return;
    }
    const int selected = m_paramDialog->GetPerkSkill();
    if (selected != -1)
    {
        for (int buttonIndex = 0; buttonIndex<m_buttonCount; ++buttonIndex)
        {
            SPRITE* const sprite = m_buttons[buttonIndex];
            if (!sprite) continue;
            const int slot = static_cast<int>(sprite->Vid()->RealDirection(sprite->Direction().value));
            if (selected == perkButtonParams[slot])
            {
                sprite->ChangeAnimation(1);
                m_selectedSprite = m_buttons[buttonIndex];
                break;
            }
        }
    }
    STRING text;
    CreateInfoTextAndIcons();
    if (!m_selectedSprite) DebugAssert(5u, "m_oSpriteSelected", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zParamDlgMenu.cpp", 240u);
    SPRITE* probe = m_hoverSprite?m_hoverSprite:m_selectedSprite;
    if (probe)
    {
        int slot = static_cast<int>(probe->Vid()->RealDirection(probe->Direction().value));
        if (slot != -1)
        {
            const char* const id = m_paramDialog->GetVidDirInfo(713, slot);
            if (id && m_infoSprite)
            {
                text = id;
                (void) m_infoSprite->Action(ACT_SET_TEXT, static_cast<int>(PtrToU32(&text)), 0, 0);
                (void) m_infoSprite->Action(ACT_SET_BEHAVE, 0, 0, 0);
            }
        }
    }
    m_selectedParam = m_selectedSprite
        ? static_cast<int>(m_selectedSprite->Vid()->RealDirection(m_selectedSprite->Direction().value)) : -1;
}

void  zParamDlgMenu::CreateInfoTextAndIcons()
{
    const int centerX = TruncateFloatToInt32(Graph->screenWidth)/2;
    const int centerY = TruncateFloatToInt32(Graph->screenHeight)/2;
    if (!m_infoSprite && m_paramDialog->GetPlayerMenuType() == 0)
    {
        STRING name("PERK_TEXT");
        m_infoSprite = Map->m_menu.SpriteWithName(name);
    }
    if (!m_infoSprite && m_paramDialog->GetPlayerMenuType() == 0)
    {
        const VECTOR position(static_cast<float>(static_cast<double>(centerX)-163.0),
            static_cast<float>(static_cast<double>(centerY)+3028.0), 3003.0f);
        m_infoSprite = Map->CreateSprite(Map->Vid(3), position, ANGLE(static_cast<unsigned char>(0u)), 0);
    }

    const float overlayX = static_cast<float>(static_cast<double>(centerX)-323.0);
    const float overlayY = static_cast<float>(static_cast<double>(centerY)+35.0);
    const VECTOR overlayPosition(overlayX, overlayY+3003.0f, 3003.0f);
    if (!m_leftIcon && m_paramDialog->GetPlayerMenuType() == 0)
        m_leftIcon = Map->CreateSprite(Map->Vid(712), overlayPosition, ANGLE(static_cast<unsigned char>(0u)), 0);
    if (!m_rightIcon && m_paramDialog->GetPlayerMenuType() == 0)
        m_rightIcon = Map->CreateSprite(Map->Vid(713), overlayPosition, ANGLE(static_cast<unsigned char>(0u)), 0);
    if (m_leftIcon)(void) m_leftIcon->Action(ACT_SET_INVISIBLE, 1, 0, 0);
    if (m_rightIcon)(void) m_rightIcon->Action(ACT_SET_INVISIBLE, 1, 0, 0);
}


void  zParamDlgMenu::RefershHeroScreen()
{
    if (m_buttonCount == 0)
    {
        TryToFindButtons();
        if (m_buttonCount == 0) return;
    }

    STRING text;
    CreateInfoTextAndIcons();

    if (m_leftIcon)(void) m_leftIcon->Action(ACT_SET_INVISIBLE, 1, 0, 0);
    if (m_rightIcon)(void) m_rightIcon->Action(ACT_SET_INVISIBLE, 1, 0, 0);

    SPRITE* overlay = 0;
    SPRITE* const hover = m_hoverSprite;
    if (hover && hover != m_leftIcon && hover != m_rightIcon)
    {
        VID* const hoverVid = hover->Vid();
        const int vidIndex = hoverVid->nVid;
        const int realDirection = hoverVid->RealDirection(hover->Direction().value);
        const char* const value = m_paramDialog->GetVidDirInfo(vidIndex, realDirection);
        if (value) text = value;

        if (vidIndex == 712) overlay = m_leftIcon;
        else if (vidIndex == 713) overlay = m_rightIcon;
        else (void) DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zParamDlgMenu.cpp", 348u);

        if (overlay)
        {
            VID* const overlayVid = overlay->Vid();
            const unsigned int directionCount = static_cast<unsigned int>(overlayVid->noDir);
            overlay->ChangeDirection(ANGLE(static_cast<unsigned char>((static_cast<unsigned int>(realDirection)<<8)/directionCount)));
            (void) overlay->Action(ACT_SET_INVISIBLE, 0, 0, 0);
        }
    }
    else
    {
        const char* const value = m_paramDialog->GetHeroInfo();
        if (value) text = value;
    }

    const int centerX = TruncateFloatToInt32(Graph->screenWidth)/2;
    const int centerY = TruncateFloatToInt32(Graph->screenHeight)/2;
    const float infoX = static_cast<float>(static_cast<double>(centerX)-323.0-17.0-2.0);
    double infoY = static_cast<double>(centerY)+35.0-6.0-8.0;
    if (overlay)
    {
        VID* const overlayVid = overlay->Vid();
        infoY += static_cast<double>(overlayVid->sizeXYZ.y)-(-8.0);
    }

    SPRITE* const info = m_infoSprite;
    if (info)
    {
        const VECTOR position(infoX+Map->m_shiftXY.x,
            static_cast<float>(infoY+3003.0+Map->m_shiftXY.y), 3003.0f);
        info->ChangeCoor(position);
        (void) m_infoSprite->Action(ACT_SET_TEXT, static_cast<int>(PtrToU32(&text)), 0, 0);
        (void) m_infoSprite->Action(ACT_SET_BEHAVE, 0, 0, 0);
    }

    if (m_paramDialog->GetPlayerMenuType() != 1 && m_selectedParam >= 0)
    {
        SPRITE* const selector = m_buttons[5];
        VID* const selectorVid = selector->Vid();
        const unsigned int directionCount = static_cast<unsigned int>(selectorVid->noDir);
        selector->ChangeDirection(ANGLE(static_cast<unsigned char>((static_cast<unsigned int>(m_selectedParam)<<8)/directionCount)));
    }

    RefreshScriptVars();
}


void  zParamDlgMenu::RefreshScriptVars()
{
    static const char* const names[8] = {
        "MENU_PlayerSex", "MENU_PlayerName", "MENU_Health", "MENU_Rank",
        "MENU_EXP", "MENU_LEVEL", "MENU_Money", "MENU_MONEYBASE"
    };
    static const int selectors[8] = {14, 0, 16, 17, 15, 37, 18, 18};

    for (unsigned int variableIndex = 0; variableIndex<8u; ++variableIndex)
    {
        STRING name(names[variableIndex]);
        const char* const text = (variableIndex == 1u)?m_paramDialog->GetPlayerName():m_paramDialog->GetFormattedSkill(selectors[variableIndex]);
        Map->SetScriptVariable(name, STRING(text));
    }
}
