
#include "mylib.h"
#include "../../engine_globals.h"
#include "irpg.h"
#include "zm/Menu/zParamDlgMenu.h"
#include "zm/Menu/zShopDlgMenu.h"
#include "zm/Menu/zShopItemInfo.h"
#include "zHeroInvMenu.h"
#include "sprite.h"
#include "vid.h"
#include "input.h"
#include "graph.h"
#include "map.h"
#include "zm/Inventory/zInventory3D.h"
#include "zm/Inventory/zInventoryItem.h"
#include "zm/Menu/zSpriteInventory.h"
#include <new>

#include "basetype.h"


iRPGObj* zHeroInvMenu::GetSelItem()
{
    return m_selectedRef.m_object;
}


void  zHeroInvMenu::RemTakenItem()
{
    if (m_invServer->GetTakenObj() != 0)
    {
        if ((m_invServer->TryToPutItemBack() & 1) == 0)
            DebugAssert(4u, "buf & ITEM_ADDEDFULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 767u);
        ShowMouseCursor(0);
    }
}

bool  zHeroInvMenu::GetBuyMode()
{
    iRPGObj* const item = GetSelItem();
    if (!item) DebugAssert(5u, "item", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 942u);
    if (item->GetInvId() == 20) return true;
    return item->GetInvId() == 21;
}

void  zHeroInvMenu::TryToFindButtons2()
{
    m_extraButtonCount = 0;
    const int menuSpriteCount = Map->m_menu.m_count;
    MENU& menu = Map->m_menu;
    for (int menuSpriteIndex = 0; menuSpriteIndex<menuSpriteCount; ++menuSpriteIndex)
    {
        SPRITE* const sprite = menu.m_data[menuSpriteIndex];
        const unsigned int nVid = static_cast<unsigned int>(sprite->Vid()->nVid);
        if (nVid == 712u || nVid == 713u)
        {
            if (m_extraButtonCount >= 29) DebugAssert(5u, "m_iNumSkillBtn < MAX_BUTTONS_NUM2-1", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 689u);
            m_extraButtons[m_extraButtonCount++] = sprite;
        }
    }
    Sort(m_extraButtons, m_extraButtonCount);
}

void  zHeroInvMenu::MouseCursorTact(float screenX, float screenY)
{
    if (!m_mouseSprite) DebugAssert(4u, "m_pMouseSprite", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 538u);
    const float targetX = screenX+Map->m_shiftXY.x;
    const float targetY = screenY+Map->m_shiftXY.y;
    float currentX = 0.0f, currentY = 0.0f;
    m_invServer->GetMouseDisplacement(currentX, currentY);
    VECTOR delta(targetX-currentX, targetY-currentY+(4003.0f), 4003.0f);
    if (m_mouseSprite) m_mouseSprite->ChangeCoor(delta);
}

void  zHeroInvMenu::SetRPGHero(iRPGHero* hero)
{
    m_rpgHero = hero;
    m_invServer = hero->GetInvServer();
    m_invServer->SetRPGHero(hero);
    for (unsigned int i = 0; i<22u; ++i)
    {
        if (!m_inventories[i]) continue;
        iInventory* const buf = m_invServer->GetInv(static_cast<int>(i));
        if (!buf) DebugAssert(5u, "buf", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 816u);
        if (buf->QueryInterface(18) != 0)
        {
            m_inventories[i]->SetRPGHero(hero);
            m_inventories[i]->SetInventory(buf);
        }
        else
        {
            m_inventories[i]->iRelease();
            m_inventories[i] = 0;
        }
    }
    iInventory* const selected = m_invServer->GetInv(0);
    for (unsigned int i = 2; i <= 6u; ++i) m_inventories[i]->SetGetDirtyInventory(selected);
    ReloadAll();
}

void  zHeroInvMenu::TryToFindButtons()
{
    m_buttonCount = 0;
    const int menuSpriteCount = Map->m_menu.m_count;
    MENU& menu = Map->m_menu;
    for (int buttonIndex = 0; buttonIndex<menuSpriteCount; ++buttonIndex)
    {
        SPRITE* const sprite = menu.m_data[buttonIndex];
        VID* const info = sprite->Vid();
        if (info->nVid == 722u)
        {
            if (m_buttonCount >= 21) DebugAssert(5u, "m_iLightBtnNum < LIGHT_BUTTONS_NUM - 1", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 650u);
            m_buttons[m_buttonCount] = sprite;
            (void) m_buttons[m_buttonCount]->Action(ACT_SET_INVISIBLE, 1, 0, 0);
            ++m_buttonCount;
        }
    }
    SortByX(m_buttons, m_buttonCount);
    if (!m_rpgHero) DebugAssert(5u, "m_pRPGHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 658u);
    for (int buttonIndex = 0; buttonIndex<m_buttonCount; ++buttonIndex)
    {
        m_buttonInventoryIndex[buttonIndex] = 0;
        SPRITE* const sprite = m_buttons[buttonIndex];
        EX_SPRITE_DATA* const extraData = sprite->ExData();
        if (!extraData) continue;
        if (extraData->nameText.value[0] != '\0')
        {
            STRING name = sprite->GetName();
            m_buttonInventoryIndex[buttonIndex] = m_invServer->GetInvByName(name.m_value);
        }
    }
}

zHeroInvMenu::zHeroInvMenu(iRPGFrameWork* framework)
    : zBaseDlgMenu()
{
    m_itemTakerRef.m_object = 0;
    m_itemTakerRef.m_refObject = 0;
    m_itemTakerRef.m_owner = 0;
    m_buttonCount = 0;
    m_extraButtonCount = 0;
    m_mouseSprite = 0;
    m_hintSprite = 0;
    m_itemInfoWidget = 0;
    m_framework = framework;
    m_shopItemInfo = 0;
    m_shopDlgMenu = 0;
    m_invServer = 0;
    m_rpgHero = 0;
    m_highLightRef.m_object = 0;
    m_highLightRef.m_refObject = 0;
    m_highLightRef.m_owner = 0;
    m_hoverRef.m_object = 0;
    m_hoverRef.m_refObject = 0;
    m_hoverRef.m_owner = 0;
    m_selectedRef.m_object = 0;
    m_selectedRef.m_refObject = 0;
    m_selectedRef.m_owner = 0;
    m_dragHoldTicks = 0;
    for (int inventoryIndex = 0; inventoryIndex<22; ++inventoryIndex) GetNew_zSpriteInventory(&m_inventories[inventoryIndex], m_framework);
    m_inventories[0]->SetDrawCellsType(1);
    m_inventories[19]->SetDrawCellsType(2);
    m_inventories[20]->SetSpriteZ(2800);
    m_inventories[20]->ShowCost(true);
    m_inventories[21]->ShowCost(true);
    m_highLightRef.m_owner = static_cast<iRefOwner*>(this);
}


zHeroInvMenu::~zHeroInvMenu()
{
    for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex)
    {
        if (m_inventories[inventoryIndex]) m_inventories[inventoryIndex]->iRelease();
        m_inventories[inventoryIndex] = 0;
    }
    if (m_itemInfoWidget)
    {
        m_itemInfoWidget->iRelease();
        m_itemInfoWidget = 0;
    }
}

void  zHeroInvMenu::SetRPGFrameWork(iRPGFrameWork* frameWork)
{
    m_framework = frameWork;
}

void  zHeroInvMenu::SetShopItemInfo(iShopItemInfo* shopItemInfo)
{
    m_shopItemInfo = shopItemInfo;
}

void  zHeroInvMenu::SetShopDlgMenu(iShopDlgMenu* shopDialog)
{
    m_shopDlgMenu = shopDialog;
}

bool  zHeroInvMenu::IsShopMode()
{
    return m_shopItemInfo != 0;
}


void  zHeroInvMenu::GetGammaItemDisabled(int& red, int& green, int& blue)
{
    m_framework->GetGammaItemDisabled(&red, &green, &blue);
}

iBaseDlgMenu*  zHeroInvMenu::AsBaseDlgMenu()
{
    return static_cast<iBaseDlgMenu*>(this);
}

void  zHeroInvMenu::DeleteHint()
{
    delete m_hintSprite;
    m_hintSprite = 0;
}

void  zHeroInvMenu::InitOnShow()
{
    FindButtonsIfNeeded();
    ControlInventories(0);
    RefreshItems();
}

void  zHeroInvMenu::OnInvalidate(iRef<iRefObject>* candidate)
{
    if (candidate != reinterpret_cast<iRef<iRefObject>*>(&m_highLightRef))
        return;
    (void) HighLightItem(0);
}


bool  zHeroInvMenu::SetInputActive(bool active)
{
    const bool changed = zBaseDlgMenu::SetInputActive(active);
    if (changed && !active)
        this->RemTakenItem();
    return changed;
}




void  zHeroInvMenu::SetItemTaker(iItemTaker* itemTaker)
{
    m_itemTakerRef.Set(itemTaker);
}

void  zHeroInvMenu::ShowItemInfo(iRPGObj* item)
{
    if (!m_shopItemInfo || !item) return;
    m_shopItemInfo->ShowObjInfo(item, true, static_cast<iBaseDlgMenu*>(this));
    if (item->GetInvId() == 21) return;
    m_shopDlgMenu->ScrollToItem(item->GetBaseId(), false);
}

bool  zHeroInvMenu::Show(bool action)
{
    const bool shown = zBaseDlgMenu::Show(action);
    if (!shown) return false;
    if (action)
    {
        this->InitOnShow();
    }
    else
    {
        this->RemTakenItem();
        this->ReloadAll();
        if (this->m_itemInfoWidget)
            this->m_itemInfoWidget->ShowObjInfo(0, false, 0);
    }
    return shown;
}

const char*  zHeroInvMenu::GetMouseTipsString(float x, float y)
{
    if (!m_enabled) return 0;
    if (m_invServer->GetTakenObj() != 0) return 0;
    const iInventory* const candidate = m_invServer->GetInventoryByPoint(x, y);
    if (m_invServer->GetObjByPoint(x, y) != 0 || !candidate) return 0;
    const char* const name = candidate->GetName();
    if (!name) return 0;
    iTextDB* const textDb = m_framework->AsTextDB();
    const char** const lines = textDb->GetExtInfo(1, name);
    return m_framework->AsTextDB()->GetSpeciallyDecodedString(lines, -1);
}

void  zHeroInvMenu::HighLightInventories(const iRPGObj* item)
{
    if (m_buttonCount == 0) return;
    if (m_hoverRef.m_object == item) return;
    for (int inventoryIndex = 0; inventoryIndex<m_buttonCount; ++inventoryIndex)
    {
        unsigned int enabled = 1u;
        if (item && m_buttonInventoryIndex[inventoryIndex] >= 0 && m_invServer->CanPutToInv(m_buttonInventoryIndex[inventoryIndex], item))
            enabled = 0u;
        (void) m_buttons[inventoryIndex]->Action(ACT_SET_INVISIBLE, static_cast<int>(enabled), 0, 0);
    }
    m_hoverRef.Set(const_cast<iRPGObj*>(item));
}

bool  zHeroInvMenu::HighLightItem(const iRPGObj* target)
{
    iRPGObj* const mutableTarget = const_cast<iRPGObj*>(target);
    if (!m_selectedRef.m_object && target)
    {
        iRPGObj* const resolved = static_cast<iRPGObj*>(mutableTarget->QueryInterface(9));
        m_selectedRef.Set(resolved);
        if (m_shopDlgMenu) m_shopDlgMenu->RefreshAll();
    }
    if (m_highLightRef.m_object == target) return false;
    unsigned int changed = 0u;
    for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex)
        if (m_inventories[inventoryIndex] && m_inventories[inventoryIndex]->HighLightItem(target)) changed |= 1u;
    m_highLightRef.Set(mutableTarget);
    return (changed&0xFFu) != 0u;
}

int  zHeroInvMenu::TryProcessItemPut(iRPGObj* item, float x, float y, bool alternate)
{
    const float centerX = Graph->screenWidth*(0.5f);
    const float centerY = Graph->screenHeight*(0.5f);

    const double shopBoundary = static_cast<double>(centerX)+20.0;

    if (!alternate)
    {
        if (IsShopMode())
        {
            if (shopBoundary>x) return 0;
            if (!m_shopDlgMenu->TryToSellItem(item, 1u)) return 0;
            return 1;
        }
        if (IsShopMode()) return 0;
        if (x>static_cast<double>(centerX)-360.0 && x<static_cast<double>(centerX)+360.0 &&
            y>static_cast<double>(centerY)-250.0 && y<static_cast<double>(centerY)+250.0) return 0;
        (void) m_itemTakerRef.m_object->CreateGroundSprite(item);
        return 1;
    }

    if (!(shopBoundary >= x)) return 0;
    const bool accepted = m_invServer->TryToBuyObj(item, x, y) != 0;
    if (accepted)
    {
        m_framework->PlaySFXCurArmy(45);
        m_shopDlgMenu->RefreshAll();
        ResetSelItem();
        return 1;
    }
    m_framework->PlaySFXCurArmy(46);
    return 0;
}

void  zHeroInvMenu::ShowMouseCursor(const iRPGObj* enabled)
{
    SPRITE* oldPreview = m_mouseSprite;
    if (!enabled)
    {
        if (oldPreview)
        {
            delete oldPreview;
            m_mouseSprite = 0;
        }
        return;
    }

    iRPGObj* const selected = const_cast<iRPGObj*>(m_invServer->GetTakenObj());
    if (!selected) return;
    oldPreview = m_mouseSprite;
    if (oldPreview) DebugAssert(4u, "m_pMouseSprite == NULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 510u);

    const int spriteIndex = selected->GetVidNum(2);
    VID* descriptor = ::EmptyVid;
    if (spriteIndex >= 0 && spriteIndex<Map->m_vidCount)
    {
        VID* const candidate = Map->m_vids[spriteIndex];
        if (candidate) descriptor = candidate;
    }

    VECTOR origin(0.0f, 0.0f, 0.0f);
    ANGLE direction(static_cast<unsigned char>(0));
    SPRITE* const preview = Map->CreateSprite(descriptor, origin, direction, 0);
    m_mouseSprite = preview;
    if (!preview) return;

    const unsigned int frame = static_cast<unsigned int>(selected->GetDirNum(2));
    VID* const previewInfo = m_mouseSprite->Vid();
    const unsigned char directionValue = static_cast<unsigned char>((frame<<8)/static_cast<unsigned int>(previewInfo->noDir));
    m_mouseSprite->ChangeDirection(ANGLE(directionValue));

    iInvKeeper* const activeOwner = m_invServer->Keeper();
    if (activeOwner->CanHoldObj(selected)) return;

    int red = 0, green = 0, blue = 0;
    GetGammaItemDisabled(red, green, blue);
    const int values[3] = {red, green, blue};
    unsigned int negative = 0u, positive = 0u;
    for (unsigned int gammaComponent = 0; gammaComponent<3u; ++gammaComponent)
    {
        int value = values[gammaComponent];
        if (value<-255) value = -255;
        else if (value>255) value = 255;
        const unsigned int shift = (2u-gammaComponent)*8u;
        if (value<0) negative |= static_cast<unsigned int>(-value)<<shift;
        else positive |= static_cast<unsigned int>(value)<<shift;
    }
    GAMMA gamma;
    gamma.modulate = negative;
    gamma.add = positive;
    m_mouseSprite->SetGamma(gamma);
}

void  zHeroInvMenu::ShowHint(const char* _sMsg, float x, float y, bool localize)
{
    if (!_sMsg) DebugAssert(5u, "_sMsg", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zHeroInvMenu.cpp", 961u);

    SPRITE* hint = m_hintSprite;
    if (!hint)
    {
        VECTOR origin(0.0f, 0.0f, 0.0f);
        ANGLE direction(static_cast<unsigned char>(0));
        VID* const vid = Map->Vid(3);
        hint = Map->CreateSprite(vid, origin, direction, 0);
        m_hintSprite = hint;
    }

    VECTOR position(
        x+Map->m_shiftXY.x,
        y+Map->m_shiftXY.y+(4000.0f),
        4000.0f);
    hint->ChangeCoor(position);

    STRING text;
    if (!localize)
    {
        text = _sMsg;
        (void) hint->Action(ACT_SET_BEHAVE, 2, 0, 0);
    }
    else
    {
        PROFILE* const profile = ::Strings;
        const STRING fallback(StringGlobals::EmptyText());
        const STRING key(_sMsg);
        const STRING section("messages");
        text = profile->GetString(section, key, fallback);
        (void) hint->Action(ACT_SET_BEHAVE, 4, 0, 0);
    }
    (void) hint->Action(ACT_SET_TEXT, static_cast<int>(PtrToU32(&text)), 0, 0);
}

void __cdecl GetNew_zHeroInvMenu(iHeroInvMenu** outObject, iRPGFrameWork* framework)
{
    void* const storage = CrtNew(sizeof(zHeroInvMenu));
    zHeroInvMenu* object = 0;
    try
    {
        if (storage)
            object = ::new (storage) zHeroInvMenu(framework);
    }
    catch (...)
    {
        zBaseDlgMenu::operator delete(storage);
        throw;
    }
    *outObject = object;
}

void  zHeroInvMenu::Control(INPUT* input)
{
    if (m_enabled == 0u)
    {
        this->HighLightItem(0);
        this->HighLightInventories(0);
        this->ResetSelItem();
        return;
    }

    this->FindButtonsIfNeeded();
    this->ControlInventories(input);
    zBaseDlgMenu::Control(input);

    if (this->m_invServer->GetMouseDirty())
        this->ShowMouseCursor(this->m_invServer->GetTakenObj());

    if (this->m_mouseSprite != 0)
        this->MouseCursorTact(input->ClientMouseXY().x, input->ClientMouseXY().y);

    iRPGObj* const overlay = this->m_highLightRef.m_object;
    if (overlay)
    {
        this->HighLightItem(0);
        if (overlay->HaveInv())
            this->HighLightItem(overlay);
    }

    const iRPGObj* candidate = this->m_invServer->GetTakenObj();
    if (!candidate)
    {
        iRPGObj* const fallback = this->m_selectedRef.m_object;
        if (fallback && fallback->GetInvId() == 20) candidate = this->m_selectedRef.m_object;
    }
    this->HighLightInventories(candidate);
    this->RefreshItems();
}

void  zHeroInvMenu::ControlInput(INPUT* input)
{
    const float mouseX = input->ClientMouseXY().x;
    const float mouseY = input->ClientMouseXY().y;

    if (input->GetAction(35u) != 0)
        this->m_dragHoldTicks = AddSigned32Wrap(this->m_dragHoldTicks, 1);
    else
        this->m_dragHoldTicks = 0;

    if (this->m_invServer->GetTakenObj() == 0 && this->m_dragHoldTicks>3 && input->GetAction(35u) != 0)
    {
        const float dx = INPUT::lClickMouseCoor.x-input->ClientMouseXY().x;
        const float dy = INPUT::lClickMouseCoor.y-input->ClientMouseXY().y;
        const float ax = dx<0.0f?-dx:dx;
        const float ay = dy<0.0f?-dy:dy;
        const float k = (0.398000002f);
        const float metric = ax >= ay?ax+ay*k:ay+ax*k;
        if (metric >= (4.0f) &&
            this->m_invServer->TryToTakeItem(INPUT::lClickMouseCoor.x, INPUT::lClickMouseCoor.y) != 0)
            this->m_framework->PlaySFXCurArmy(79);
    }

    if (input->GetAction(61u) != 0 && this->m_invServer->PutToActiveInv(mouseX, mouseY))
        this->HighLightItem(this->m_selectedRef.m_object);

    if (input->GetAction(55u) != 0 && this->m_invServer->GetTakenObj() != 0)
    {
        this->HighLightItem(0);
        iRef<iRPGObj> heldReference;
        heldReference.Set(const_cast<iRPGObj*>(this->m_invServer->GetTakenObj()));

        const bool accepted = this->m_invServer->TryToPutItem(mouseX, mouseY, true) != 0;
        if (accepted)
            this->m_framework->PlaySFXCurArmy(80);

        iRPGObj* const held = heldReference.m_object;
        if (held && held->HaveInv())
        {
            this->HighLightItem(heldReference.m_object);
        }
        else if (!accepted)
        {
            this->ResetSelItem();
            this->HighLightItem(0);
            this->HighLightInventories(0);
            for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex)
            {
                iSpriteInventory* const child = this->m_inventories[inventoryIndex];
                if (child) child->HideHighLight();
            }
        }
    }

    if (this->m_invServer->GetTakenObj() != 0)
    {
        iRPGObj* const selected = const_cast<iRPGObj*>(this->m_invServer->GetTakenObj());
        zItem* const object = static_cast<zItem*>(selected->QueryInterface(49));

        if (this->m_shopDlgMenu && object->GetGameObj() != TYPE_AMMO && this->m_invServer->IsForBuy())
            this->m_shopDlgMenu->ScrollToItem(object->GetBaseId(), false);

        float currentX = 0.0f, currentY = 0.0f;
        this->m_invServer->GetMouseDisplacement(currentX, currentY);
        zItem* routeObject = 0;
        const int hoverIndex = this->m_invServer->GetInventoryToPutItem(
            object, mouseX-currentX, mouseY-currentY, routeObject);

        for (int inventoryIndex = 0; inventoryIndex<22; ++inventoryIndex)
        {
            if (inventoryIndex == hoverIndex) continue;
            iSpriteInventory* const child = this->m_inventories[static_cast<unsigned int>(inventoryIndex)];
            if (child) child->HideHighLight();
        }
        if (hoverIndex != -1)
        {
            iSpriteInventory* const child = this->m_inventories[static_cast<unsigned int>(hoverIndex)];
            if (child)
            {
                const iRPGObj* const taken = this->m_invServer->GetTakenObj();
                this->m_inventories[static_cast<unsigned int>(hoverIndex)]->ShowPlaceToPut(taken, mouseX-currentX, mouseY-currentY);
            }
        }

        int width = 0, height = 0;
        object->GetSize2DCalced(width, height, 0);
        const float hintX = (mouseX-currentX)+static_cast<float>((width*37)/2);
        const float hintY = (mouseY-currentY)-static_cast<float>((height*37)/2);

        if (hoverIndex == 19)
        {
            this->ShowHint("InvThrowToGrndWarn", hintX, hintY, true);
        }
        else
        {
            const iInventory* const candidate = this->m_invServer->GetInventoryByPoint(mouseX, mouseY);
            if (candidate && (candidate->GetId() == 20 || candidate->GetId() == 21))
            {
                STRING priceText = Printf("$%d", static_cast<unsigned int>(object->GetSellCostFull()));
                this->ShowHint(priceText.CharPtr(), hintX, hintY, false);
            }
            else
            {
                this->DeleteHint();
            }
        }
    }
    else
    {
        this->DeleteHint();
    }

    if (input->GetAction(56u) != 0)
    {
        iRPGObj* const candidate = this->m_invServer->GetObjByPoint(mouseX, mouseY);
        if (candidate) this->m_selectedRef.Set(candidate);

        if (this->IsShopMode())
        {
            if (candidate)
            {
                this->m_shopDlgMenu->RefreshAll();
                this->ShowItemInfo(candidate);
                if (candidate->GetGameObj() == 7)
                    this->HighLightItem(0);
                else
                    this->HighLightItem(candidate);
            }
            else
            {
                const iInventory* const underMouse = this->m_invServer->GetInventoryByPoint(mouseX, mouseY);
                if (underMouse)
                {
                    const unsigned int first = static_cast<unsigned int>(underMouse->GetObjectType());
                    const unsigned int second = static_cast<unsigned int>(underMouse->GetObjectExtType());
                    if (first != 14u || second != 16u)
                        this->m_shopDlgMenu->ScrollToItem(static_cast<OBJ_TYPE>(first), static_cast<OBJ_EXTTYPE>(second));
                }
            }
        }
        else
        {
            this->HighLightItem(candidate);
            this->HighLightInventories(candidate);
            this->ResetSelItem();
        }
    }
    else
    {
        const int wheel = input->GetAction(73u);
        if (wheel != 0)
        {
            const iInventory* const candidate = this->m_invServer->GetInventoryByPoint(mouseX, mouseY);
            if (candidate)
            {
                zInventory3D* const inventory = static_cast<zInventory3D*>(const_cast<iInventory*>(candidate)->QueryInterface(48));
                inventory->Scroll(0.0f, wheel<0?10.0f:-10.0f);
            }
        }
    }

    (void) this->IsShopMode();
    this->ControlItemInfo(this->m_invServer->GetObjByPoint(mouseX, mouseY), input);
}

void  zHeroInvMenu::ControlItemInfo(iRPGObj* selection, const INPUT* input)
{
    static iRPGObj* lastItem = 0;
    static unsigned int lastItemTime = 0;

    if (lastItem != selection)
        lastItemTime = ::CurrentTime;
    lastItem = selection;

    iShopItemInfo* widget = m_itemInfoWidget;
    if (!widget)
    {
        GetNew_zShopItemInfo(&m_itemInfoWidget);
        widget = m_itemInfoWidget;
        widget->SetRPGFrameWork(m_framework);
        widget = m_itemInfoWidget;
        const float scale = (0.5f);
        widget->SetScreenCoordinates(Graph->screenWidth*scale, Graph->screenHeight*scale);
        widget = m_itemInfoWidget;
        widget->SetShowAmmo(false);
    }

    iInvKeeper* const keeper = static_cast<iInvKeeper*>(m_rpgHero->QueryInterface(1));
    widget = m_itemInfoWidget;
    widget->SetInventoryKeeper(keeper);
    widget = m_itemInfoWidget;
    widget->SetMaxCoords(TruncateFloatToInt32(Graph->screenWidth)-230,
        TruncateFloatToInt32(Graph->screenHeight)-340);

    if (m_invServer->GetTakenObj() != 0)
    {
        m_itemInfoWidget->ShowObjInfo(0, false, 0);
        return;
    }
    if (lastItem == 0 || !input)
    {
        m_itemInfoWidget->ShowObjInfo(0, false, 0);
        return;
    }

    const unsigned int now = ::CurrentTime;
    bool active = (now-lastItemTime >= 1500u);
    if (!active && input->GetAction(35u) != 0)
    {
        const float dx = INPUT::lClickMouseCoor.x-input->ClientMouseXY().x;
        const float dy = INPUT::lClickMouseCoor.y-input->ClientMouseXY().y;
        const float ax = dx<0.0f?-dx:dx, ay = dy<0.0f?-dy:dy;
        const float k = (0.398000002f);
        const float metric = ax >= ay?ax+ay*k:ay+ax*k;
        active = metric >= (0.0f);
    }
    if (active)
    {
        m_itemInfoWidget->SetScreenCoordinates(input->ClientMouseXY().x, input->ClientMouseXY().y);
        m_itemInfoWidget->ShowObjInfo(lastItem, true, 0);
        lastItemTime = ::CurrentTime-1500u;
    }
    else
    {
        m_itemInfoWidget->ShowObjInfo(0, false, 0);
    }
}

void  zHeroInvMenu::ControlInventories(INPUT* input)
{
    for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex) if (m_inventories[inventoryIndex]) m_inventories[inventoryIndex]->ControlPlayerMenu(input);
    for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex) if (m_inventories[inventoryIndex]) m_inventories[inventoryIndex]->ClearDirty();
}

void  zHeroInvMenu::DeletePointerToSprite(SPRITE* sprite)
{
    for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex)
        if (this->m_inventories[inventoryIndex]) this->m_inventories[inventoryIndex]->DeletePointerToSprite(sprite);
    if (this->m_mouseSprite == sprite)
    {
        this->m_mouseSprite = 0;
    }
    else
    {
        bool found = SpriteInArray(sprite, this->m_buttons, this->m_buttonCount) != 0;
        if (!found) found = SpriteInArray(sprite, this->m_extraButtons, this->m_extraButtonCount) != 0;
        if (found) this->ReloadAll();
    }
    if (this->m_shopItemInfo)
        this->m_shopItemInfo->DeletePointerToSprite(sprite);
    if (this->m_itemInfoWidget)
        this->m_itemInfoWidget->DeletePointerToSprite(sprite);
    if (this->m_hintSprite == sprite) this->m_hintSprite = 0;
}

void  zHeroInvMenu::FindButtonsIfNeeded()
{
    if (m_buttonCount == 0)
    {
        TryToFindButtons();
        HighLightInventories(0);
    }
    if (m_extraButtonCount == 0) TryToFindButtons2();
}

void  zHeroInvMenu::ReloadAll()
{
    for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex) if (m_inventories[inventoryIndex]) m_inventories[inventoryIndex]->ReloadAll();
    m_buttonCount = 0;
    m_extraButtonCount = 0;
}

void  zHeroInvMenu::RefreshItems()
{
    for (unsigned int inventoryIndex = 0; inventoryIndex<22u; ++inventoryIndex) if (m_inventories[inventoryIndex]) m_inventories[inventoryIndex]->RefreshItems();
}

void  zHeroInvMenu::ResetSelItem()
{
    m_selectedRef.Set(0);
}

void*  zHeroInvMenu::QueryInterface(int interfaceId) const
{
    if (static_cast<unsigned int>(interfaceId) == 12u)
        return const_cast<zHeroInvMenu*>(this);
    return zBaseDlgMenu::QueryInterface(interfaceId);
}

int  zHeroInvMenu::iRelease()
{
    if (this)
    {
        this->~zHeroInvMenu();
        zBaseDlgMenu::operator delete(this);
    }
    return 0;
}



