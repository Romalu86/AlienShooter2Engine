#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"

#include "zShopDlgMenu.h"
#include "irpg.h"
#include "zHeroInvMenu.h"
#include "zShopItemInfo.h"
#include "sprite.h"
#include "vid.h"
#include "input.h"
#include "zm/Inventory/zInventory.h"
#include "zm/Inventory/zInventory3D.h"
#include "zm/Inventory/zInventory2D.h"
#include "zm/RPGHero/zRPGWeapon.h"
#include "zm/RPGHero/zRPGAmmo.h"
#include "zm/RPGHero/zRPGArmor.h"
#include "zm/RPGHero/zRPGImplant.h"
#include "mylib.h"
#include <new>

#include "basetype.h"

void GetNew_zShopDlgMenu(iShopDlgMenu** outObject);
zShopListNode**  ShopListErase(zShopListLayout* list, zShopListNode** outNext, zShopListNode* node);
zShopListNode**  ShopListInsert(zShopListLayout* list, zShopListNode** outNode, zShopListNode* position, iRPGObj* const* value);


#include "map.h"

void __cdecl GetNew_zShopDlgMenu(iShopDlgMenu** outObject)

{
    void* const storage = CrtNew(sizeof(zShopDlgMenu));
    zShopDlgMenu* object = 0;
    if (storage)
    {
        try
        {
            object = ::new (storage) zShopDlgMenu();
        }
        catch (...)
        {
            CrtFreePointer(storage);
            throw;
        }
    }
    *outObject = object;
}

void  zShopDlgMenu::SetRPGFrameWork(iRPGFrameWork* frameWork)
{
    m_framework = frameWork;
}

void  zShopDlgMenu::SetHeroInvMenu(iHeroInvMenu* heroInventoryMenu)
{
    m_heroInvMenu = heroInventoryMenu;
}

void  zShopDlgMenu::SetPlayer(iPlayer* player)
{
    m_player = player;
}

void  zShopDlgMenu::SetCurLevel(int level)
{
    m_curLevel = level;
}

iBaseDlgMenu*  zShopDlgMenu::AsBaseDlgMenu()
{
    return static_cast<iBaseDlgMenu*>(QueryInterface(11));
}

void  zShopDlgMenu::RefreshAll()
{
    InitLoad();
    RefreshBuyButtons();
}

void  zShopDlgMenu::SetShopItemInfo(iShopItemInfo* info)
{
    m_shopItemInfo = info;
    SetRPGHero(m_rpgHeroRef.m_object);
}

void  zShopDlgMenu::PageUp()
{
    iInventory* inventory = m_invServerRef.m_object->GetInv(20);
    zInventory3D* inventory3D = static_cast<zInventory3D*>(inventory->QueryInterface(48));
    inventory3D->Scroll(0.0f, -100.0f);
}

void  zShopDlgMenu::PageDown()
{
    iInventory* inventory = m_invServerRef.m_object->GetInv(20);
    zInventory3D* inventory3D = static_cast<zInventory3D*>(inventory->QueryInterface(48));
    inventory3D->Scroll(0.0f, 100.0f);
}


void*  zShopDlgMenu::QueryInterface(int id) const
{
    if (id == 14) return const_cast<zShopDlgMenu*>(this);
    return zBaseDlgMenu::QueryInterface(id);
}

int  zShopDlgMenu::iRelease()
{
    if (this)
    {
        this->~zShopDlgMenu();
        zBaseDlgMenu::operator delete(this);
    }
    return 0;
}

bool  zShopDlgMenu::ScrollToItem(OBJ_TYPE objectType, OBJ_EXTTYPE extType)
{
    if (static_cast<int>(objectType) == 7)
    {
        RefreshBuyButtons();
        return false;
    }
    LoadListOfObjects(objectType, extType);
    return true;
}

void  zShopDlgMenu::Control(INPUT* input)
{
    (void) InitLoad();
    zBaseDlgMenu::Control(input);
    RefreshBuyButtons();
}

void  zShopDlgMenu::PageScroll(int scrollPercent)
{
    if (scrollPercent<0) scrollPercent = 0;
    else if (scrollPercent>1000) scrollPercent = 1000;
    iInventory* inventory = m_invServerRef.m_object->GetInv(20);
    zInventory3D* inventory3D = static_cast<zInventory3D*>(inventory->QueryInterface(48));
    inventory3D->ScrollPer(scrollPercent);
}

void  zShopDlgMenu::DeletePointerToSprite(SPRITE* sprite)
{
    int index = 0;
    for (; index<m_categoryButtonCount && m_categoryButtons[index] != sprite; ++index)
    {
    }
    if (index >= m_categoryButtonCount) return;
    m_categoryButtonCount = 0;
    m_filterButtonCount = 0;
    m_buyButtonCount = 0;
    m_objectType = static_cast<OBJ_TYPE>(14);
    m_extType = static_cast<OBJ_EXTTYPE>(16);
    ReloadAll();
    if (m_shopItemInfo) m_shopItemInfo->Initialize();
}

void  zShopDlgMenu::BuySellButton1()
{
    if (!m_heroInvMenu)
        DebugAssert(5u, "m_pHeroInvMenu", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1066u);
    if (m_heroInvMenu->GetBuyMode())
    {
        iRPGObj* selected = m_heroInvMenu->GetSelItem();
        if (selected)
            (void) TryToBuyItem(selected);
    }
    else
    {
        (void) TryToSellSelectedItem(0u);
    }
    RefreshAll();
}

void  zShopDlgMenu::BuySellButton2()
{
    if (!m_heroInvMenu)
        DebugAssert(5u, "m_pHeroInvMenu", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1088u);

    iRPGObj* const selected = m_heroInvMenu->GetSelItem();
    if (selected)
    {
        const int objectType = selected->GetGameObj();
        if (objectType == 2)
        {
            zRPGWeapon* const weapon = static_cast<zRPGWeapon*>(selected->QueryInterface(50));
            zRPGAmmo* const weaponAmmo = weapon->GetAmmo();
            if (weaponAmmo)
            {
                zRPGAmmo* shopAmmo = 0;
                zShopListNode* shopAmmoNode = 0;
                for (zShopListNode* node = m_objects.m_sentinel->next;
                    node != m_objects.m_sentinel;
                    node = node->next)
                {
                    iRPGObj* const object = node->value;
                    if (!object)
                        DebugAssert(5u, "*j", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1103u);
                    if (object->GetGameObj() != TYPE_AMMO)
                        continue;

                    zRPGAmmo* const candidate = static_cast<zRPGAmmo*>(object->QueryInterface(45));
                    const int candidateBaseId = static_cast<zRPGObj*>(candidate)->GetBaseId();
                    const int weaponBaseId = static_cast<zRPGObj*>(weaponAmmo)->GetBaseId();
                    if (candidateBaseId == weaponBaseId)
                    {
                        shopAmmo = candidate;
                        shopAmmoNode = node;
                        break;
                    }
                }

                zRPGAmmo* ammoToBuy = shopAmmo;
                if (!ammoToBuy)
                {
                    zInventory3D* const inventory15 =
                        static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(21)->QueryInterface(48));
                    int index = -1;
                    zItem* const item = inventory15->GetItemByBaseId(
                        static_cast<zRPGObj*>(weaponAmmo)->GetBaseId(), index, false);
                    if (item)
                        ammoToBuy = static_cast<zRPGAmmo*>(item->QueryInterface(45));
                }

                if (ammoToBuy)
                {
                    (void) TryToBuyItem(static_cast<iRPGObj*>(static_cast<zRPGObj*>(ammoToBuy)));
                    if (ammoToBuy == shopAmmo && static_cast<zRPGObj*>(ammoToBuy)->GetNumInCell() == 0)
                    {
                        zShopListNode* const next = shopAmmoNode->next;
                        zShopListNode* const previous = shopAmmoNode->previous;
                        previous->next = next;
                        next->previous = previous;
                        CrtFreePointer(shopAmmoNode);
                        --m_objects.m_size;
                    }
                }
            }
        }
        else if (selected->GetGameObj() == TYPE_ARMOR)
        {
            zRPGArmor* const armor = static_cast<zRPGArmor*>(selected->QueryInterface(51));
            iRPGHero* const hero = m_rpgHeroRef.m_object;
            const int repairCost = armor->Repair(hero->GetMoney());
            (void) hero->AddMoney(-repairCost, false);
        }
    }
    RefreshAll();
}

void  zShopDlgMenu::GoHome()
{
    zInventory3D* const inventory =
        static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    float left = 0.0f, top = 0.0f, right = 0.0f, bottom = 0.0f;
    inventory->GetLimits(left, top, right, bottom);
    inventory->SetLimits(left, 0.0f, right, bottom);
}

void  zShopDlgMenu::GoEnd()
{
    zInventory3D* const inventory =
        static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    float left = 0.0f, top = 0.0f, right = 0.0f, bottom = 0.0f;
    inventory->GetLimits(left, top, right, bottom);
    inventory->SetLimits(left, 99999.0f, right, bottom);
}

int  zShopDlgMenu::GetItemsNum()
{
    iInventory* group = m_invServerRef.m_object->GetInv(20);
    zInventory3D* inventory = static_cast<zInventory3D*>(group->QueryInterface(48));
    return inventory->m_gridHeight*inventory->GetCellSizeY();
}

int  zShopDlgMenu::GetListItemsNum()
{
    iInventory* group = m_invServerRef.m_object->GetInv(20);
    zInventory3D* inventory = static_cast<zInventory3D*>(group->QueryInterface(48));
    float left, top, right, bottom;
    inventory->GetLimits(left, top, right, bottom);
    return TruncateFloatToInt32(bottom);
}

int  zShopDlgMenu::GetFirstShownItemNum()
{
    iInventory* group = m_invServerRef.m_object->GetInv(20);
    zInventory3D* inventory = static_cast<zInventory3D*>(group->QueryInterface(48));
    float left, top, right, bottom;
    inventory->GetLimits(left, top, right, bottom);
    return TruncateFloatToInt32(top);
}

void  zShopDlgMenu::SetInvServer(iInvServer* inventoryServer)
{
    m_invServerRef.Set(inventoryServer);

    ReloadAll();
}

bool  zShopDlgMenu::ScrollToItem(int objectKey, bool unusedArgument)
{
    (void) unusedArgument;
    iRPGObj* NewObj = m_framework->GetItemById(objectKey);
    if (!NewObj)
        DebugAssert(5u, "NewObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1279u);

    const unsigned int firstType = static_cast<unsigned int>(NewObj->GetGameObj());
    if (firstType == 7u)
    {
        RefreshBuyButtons();
        return false;
    }
    const unsigned int detail = static_cast<unsigned int>(NewObj->GetAddMoney());
    const unsigned int secondType = static_cast<unsigned int>(NewObj->GetGameObj());
    LoadListOfObjects(static_cast<OBJ_TYPE>(secondType), static_cast<OBJ_EXTTYPE>(detail));
    return false;
}

bool  zShopDlgMenu::TryToSellSelectedItem(unsigned int mode)
{
    if (!m_shopItemInfo)
        DebugAssert(5u, "m_pShopItemInfo", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1343u);
    iRPGObj* pSelectedItem = m_heroInvMenu->GetSelItem();
    if (!pSelectedItem)
        return false;
    if (pSelectedItem->GetSellCost() == 0)
        return false;
    const int count = pSelectedItem->GetNumInCell();
    if (count <= 0)
        DebugAssert(4u, "pSelectedItem->GetNumInCell() > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1350u);
    return TryToSellItem(pSelectedItem, mode);
}

void  zShopDlgMenu::ReleaseListOfObjects()
{
    zShopListNode* node = m_objects.m_sentinel->next;
    while (node != m_objects.m_sentinel)
    {
        zShopListNode* const current = node;
        node = node->next;
        zShopListNode* next = 0;
        (void) ShopListErase(&m_objects, &next, current);
    }
    m_generated.m_end = m_generated.m_begin;
    if (!m_invServerRef.m_object) return;
    for (int inventoryId = 20; inventoryId <= 21; ++inventoryId)
    {
        iInventory* inventory = m_invServerRef.m_object->GetInv(inventoryId);
        if (inventory)
        {
            zInventory3D* complete = static_cast<zInventory3D*>(inventory->QueryInterface(48));
            complete->RemoveAllItems();
        }
    }
}

void  zShopDlgMenu::TryToFindButtons()
{
    m_categoryButtonCount = 0;
    m_filterButtonCount = 0;
    m_buyButtonCount = 0;
    for (int menuSpriteIndex = 0; menuSpriteIndex<Map->m_menu.m_count; ++menuSpriteIndex)
    {
        SPRITE* sprite = Map->m_menu.m_data[menuSpriteIndex];
        VID* info = sprite->Vid();
        const unsigned int type = static_cast<unsigned int>(info->nVid);
        if (type == 721u) m_categoryButtons[m_categoryButtonCount++] = sprite;
        if (type == 715u) m_filterButtons[m_filterButtonCount++] = sprite;
        if (type == 723u) m_buyButtons[m_buyButtonCount++] = sprite;
    }
    if (m_buyButtonCount != 0 && m_buyButtonCount != 2)
        DebugAssert(4u, "!m_iBuyBtnNum || m_iBuyBtnNum == 2", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 629u);
    Sort(m_categoryButtons, m_categoryButtonCount);
    Sort(m_filterButtons, m_filterButtonCount);
    SortByX(m_buyButtons, m_buyButtonCount);
}

void  zShopDlgMenu::SetRPGHero(iRPGHero* hero)
{
    m_rpgHeroRef.Set(hero);
    iInvKeeper* keeper = 0;
    if (m_rpgHeroRef.m_object)
        keeper = static_cast<iInvKeeper*>(m_rpgHeroRef.m_object->QueryInterface(1));
    m_keeperRef.Set(keeper);
    if (m_shopItemInfo && m_rpgHeroRef.m_object)
        m_shopItemInfo->SetInventoryKeeper(m_keeperRef.m_object);
}

static const OBJ_EXTTYPE categoryExtTypes[9]={static_cast<OBJ_EXTTYPE>(1), static_cast<OBJ_EXTTYPE>(2), static_cast<OBJ_EXTTYPE>(3), static_cast<OBJ_EXTTYPE>(4), static_cast<OBJ_EXTTYPE>(5), static_cast<OBJ_EXTTYPE>(16), static_cast<OBJ_EXTTYPE>(16), static_cast<OBJ_EXTTYPE>(9), static_cast<OBJ_EXTTYPE>(16)};
static const OBJ_TYPE categoryTypes[9]={static_cast<OBJ_TYPE>(14), static_cast<OBJ_TYPE>(14), static_cast<OBJ_TYPE>(14), static_cast<OBJ_TYPE>(14), static_cast<OBJ_TYPE>(14), static_cast<OBJ_TYPE>(4), static_cast<OBJ_TYPE>(5), static_cast<OBJ_TYPE>(6), static_cast<OBJ_TYPE>(3)};

void  zShopDlgMenu::ControlInput(INPUT* input)
{
    SPRITE* sprite = Map->m_menu.SelectedSprite();
    if (sprite && (Map->m_menu.ControlFlags()&1u))
    {
        VID* info = sprite->Vid();
        const unsigned int type = static_cast<unsigned int>(info->nVid);
        if (type == 721u)
        {
            int BtnNum = -1;
            for (int i = 0; i<m_categoryButtonCount; ++i)
                if (m_categoryButtons[i] == sprite)
                {
                    BtnNum = i;
                    break;
                }
            if (BtnNum<0) DebugAssert(5u, "BtnNum >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 198u);
            const OBJ_EXTTYPE ext = categoryExtTypes[BtnNum];
            const OBJ_TYPE typeId = categoryTypes[BtnNum];
            LoadListOfObjects(typeId, ext);
        }
        else if (type == 723u)
        {
            if (m_invServerRef.m_object->GetTakenObj() == 0)
            {
                if (m_buyButtons[0] == sprite) BuySellButton1();
                else if (m_buyButtons[1] == sprite) BuySellButton2();
                else DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 222u);
            }
        }
    }
    unsigned int action = input->GetAction(72u);
    if (!action) return;
    action >>= 8;
    if (action == 33u) PageUp();
    else if (action == 34u) PageDown();
    else if (action == 36u) GoHome();
    else if (action == 35u) GoEnd();
}

bool  zShopDlgMenu::TryToSellItem(iRPGObj* _pItem, unsigned int mode)
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1359u);
    const int soldCount = m_rpgHeroRef.m_object->TryToSellObjj(_pItem, static_cast<unsigned char>(mode) != 0u);
    if (!soldCount)
    {
        m_player->PlaySFx(46, false);
        return false;
    }
    m_player->PlaySFx(45, false);

    iRPGObj* baseObject = _pItem->GetObjCopy();
    baseObject->SetNumInCell(soldCount);

    zShopListNode* node = 0;
    (void) ShopListInsert(&m_objects, &node, m_objects.m_sentinel, &baseObject);
    m_objectType = static_cast<OBJ_TYPE>(14);
    m_extType = static_cast<OBJ_EXTTYPE>(16);
    const unsigned int ext = static_cast<unsigned int>(baseObject->GetAddMoney());
    const unsigned int type = static_cast<unsigned int>(baseObject->GetGameObj());
    LoadListOfObjects(static_cast<OBJ_TYPE>(type), static_cast<OBJ_EXTTYPE>(ext));
    m_heroInvMenu->ResetSelItem();
    return true;
}

int  zShopDlgMenu::TryToBuyItem(iRPGObj* _pItem)
{
    if (!_pItem || !m_rpgHeroRef.m_object)
        DebugAssert(5u, "_pItem && m_pRPGHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1021u);

    iRPGObj* moved = 0;
    const int amount = _pItem->GetBuyNumShop();
    const int buyResult = m_rpgHeroRef.m_object->TryToBuyObj(_pItem, &moved, amount);
    if (buyResult)
    {
        m_player->PlaySFx(45, false);
        _pItem->AddNumInCell(-amount);
        if (moved)
        {
            if (_pItem->GetNumInCell() == 0 && m_heroInvMenu->GetSelItem() == _pItem)
            {
                m_heroInvMenu->ResetSelItem();
                m_heroInvMenu->HighLightItem(moved);
            }
            return buyResult;
        }
        m_heroInvMenu->ResetSelItem();
        return buyResult;
    }

    m_player->PlaySFx(46, false);
    if (!m_player) return buyResult;
    iRPGHero* const moneyHero = m_rpgHeroRef.m_object;
    const int requiredMoney = _pItem->GetBuyCost();
    const int money = moneyHero->GetMoney();
    if (requiredMoney>money)
    {
        m_player->SendInfoMessage(3, 0);
        return buyResult;
    }
    if (!m_rpgHeroRef.m_object->CanBuyItem(_pItem))
    {
        m_player->SendInfoMessage(2, 0);
        return buyResult;
    }
    m_player->SendInfoMessage(1, 0);
    return buyResult;
}

zShopDlgMenu::zShopDlgMenu()
    : zBaseDlgMenu(),
      m_categoryButtonCount(0), m_filterButtonCount(0), m_buyButtonCount(0),
      m_generatedMode(0), m_framework(0), m_shopItemInfo(0), m_heroInvMenu(0),
      m_objectType(static_cast<OBJ_TYPE>(14)), m_extType(static_cast<OBJ_EXTTYPE>(16)),
      m_player(0), m_curLevel(0)
{
    m_rpgHeroRef.m_object = 0;
    m_rpgHeroRef.m_refObject = 0;
    m_rpgHeroRef.m_owner = 0;
    m_keeperRef.m_object = 0;
    m_keeperRef.m_refObject = 0;
    m_keeperRef.m_owner = 0;
    m_invServerRef.m_object = 0;
    m_invServerRef.m_refObject = 0;
    m_invServerRef.m_owner = 0;
    ReloadAll();
}

zShopListLayout::zShopListLayout()
    : m_allocatorState(0),
      m_sentinel(static_cast<zShopListNode*>(CrtNew(sizeof(zShopListNode)))),
      m_size(0)
{
    m_sentinel->next = m_sentinel;
    m_sentinel->previous = m_sentinel;
}

zShopObjectPointerVector::~zShopObjectPointerVector()
{
    CrtFreePointer(m_begin);
    m_begin = m_end = m_capEnd = 0;
}

zShopListLayout::~zShopListLayout()
{
    zShopListNode* const root = m_sentinel;
    zShopListNode* iterator = root->next;
    while (iterator != root)
    {
        zShopListNode* const node = iterator;
        iterator = node->next;
        zShopListNode* const next = node->next;
        zShopListNode* const previous = node->previous;
        previous->next = next;
        next->previous = previous;
        CrtFreePointer(node);
        --m_size;
    }
    CrtFreePointer(root);
    m_sentinel = 0;
    m_size = 0;
}

zShopDlgMenu::~zShopDlgMenu()
{
    ReleaseListOfObjects();
}

bool  zShopDlgMenu::InitLoad()
{
    if (m_categoryButtonCount != 0 || m_filterButtonCount != 0 || m_buyButtonCount != 0)
        return false;
    TryToFindButtons();
    if (m_categoryButtonCount == 0)
        return false;
    LoadListOfObjects(categoryTypes[0], categoryExtTypes[0]);
    RefreshItems();
    return true;
}

void  zShopDlgMenu::ReloadAll()
{
    m_buyAnim[0] = -1;
    m_buyAnim[1] = -1;
}


void  zShopDlgMenu::SetBuyBtnAnim(int secondButtonAnimation, int firstButtonAnimation)
{
    if (m_buyAnim[1] != secondButtonAnimation)
        m_buyButtons[1]->ChangeAnimation(secondButtonAnimation);
    m_buyAnim[1] = secondButtonAnimation;
    if (m_buyAnim[0] != firstButtonAnimation)
        m_buyButtons[0]->ChangeAnimation(firstButtonAnimation);
    m_buyAnim[0] = firstButtonAnimation;
}


void  zShopDlgMenu::RefreshItems()
{
    RefreshBuyButtons();
}

zShopListNode**  ShopListErase(
    zShopListLayout* list, zShopListNode** outNext, zShopListNode* erased)
{
    zShopListNode* const next = erased->next;
    zShopListNode* const previous = erased->previous;
    previous->next = next;
    next->previous = previous;
    CrtFreePointer(erased);
    --list->m_size;
    *outNext = next;
    return outNext;
}

zShopListNode**  ShopListInsert(
    zShopListLayout* list, zShopListNode** outNode, zShopListNode* next,
    iRPGObj* const* itemReference)
{
    zShopListNode* const previous = next->previous;
    zShopListNode* const node = static_cast<zShopListNode*>(CrtNew(sizeof(zShopListNode)));
    node->next = next?next:node;
    node->previous = previous?previous:node;
    next->previous = node;
    node->previous->next = node;
    node->value = *itemReference;
    ++list->m_size;
    *outNode = node;
    return outNode;
}
static void ShopGeneratedPushBack(zShopDlgMenu* shop, iRPGObj* object)
{
    zShopObjectPointerVector& vector = shop->m_generated;
    iRPGObj** begin = vector.m_begin;
    iRPGObj** end = vector.m_end;
    iRPGObj** capacity = vector.m_capEnd;
    if (end != capacity)
    {
        if (end) *end = object;
        vector.m_end = end+1;
        return;
    }

    int oldCount = begin?static_cast<int>(end-begin):0;
    const int grow = oldCount>1?oldCount:1;
    const int newCapacity = AddSigned32Wrap(oldCount, grow);
    const unsigned int allocationCount = newCapacity < 0 ? 0 : newCapacity;
    iRPGObj** newBegin = static_cast<iRPGObj**>(CrtNew(allocationCount*sizeof(iRPGObj*)));
    iRPGObj** destination = newBegin;
    for (iRPGObj** source = begin; source != end; ++source, ++destination)
        if (destination) *destination = *source;
    if (destination) *destination = object;
    ++destination;
    CrtFreePointer(begin);
    vector.m_begin = newBegin;
    vector.m_end = destination;
    vector.m_capEnd = newBegin+newCapacity;
}

static void ShopListPushBack(zShopListLayout* list, iRPGObj* const* object)
{
    zShopListNode* node = 0;
    (void) ShopListInsert(list, &node, list->m_sentinel, object);
}

namespace
{
class ShopAmmoKeys
{
    struct Node {
        Node* next;
        Node* previous;
        int key;
    };
    Node* m_head;

public:
    ShopAmmoKeys()
        : m_head(static_cast<Node*>(CrtNew(sizeof(Node))))
    {
        m_head->next = m_head;
        m_head->previous = m_head;
    }

    ~ShopAmmoKeys()
    {
        Node* node = m_head->next;
        while (node != m_head)
        {
            Node* const next = node->next;
            node->previous->next = next;
            next->previous = node->previous;
            CrtFreePointer(node);
            node = next;
        }
        CrtFreePointer(m_head);
    }

    void PushBack(int key)
    {
        Node* const node = static_cast<Node*>(CrtNew(sizeof(Node)));
        node->next = m_head;
        node->previous = m_head->previous;
        m_head->previous = node;
        node->previous->next = node;
        node->key = key;
    }

    bool Contains(int key) const
    {
        for (Node* node = m_head->next; node != m_head; node = node->next)
            if (node->key == key) return true;
        return false;
    }
};
}

bool  zShopDlgMenu::GenerateItems()
{
    iRPGObj** generatedBegin = m_generated.m_begin;
    iRPGObj** generatedEnd = m_generated.m_end;
    if (generatedBegin && generatedEnd != generatedBegin)
        DebugAssert(5u, "m_aItemsGen.empty()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1395u);
    if (!m_rpgHeroRef.m_object)
        DebugAssert(5u, "m_pRPGHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 1396u);

    iRPGHeroBase* const seedSource = static_cast<iRPGHeroBase*>(m_rpgHeroRef.m_object->QueryInterface(3));
    unsigned int seed = seedSource->GetSRandVal(true);
    seed += static_cast<unsigned int>(m_curLevel);
    iRPGImplant::SeedImplantRand(BitsToSigned32(seed));

    iRPGObj* baseItem = m_framework->FirstObject(1, 0, 0);
    while (baseItem)
    {
        if (baseItem->GetGameObj() == TYPE_IMPLANT && baseItem->GetBuyCost()>0)
        {
            const int generatedItemCount = baseItem->GetMaxShop(m_curLevel);
            if (generatedItemCount>0)
            {
                for (int generatedItemIndex = 0; generatedItemIndex<generatedItemCount; ++generatedItemIndex)
                {
                    iRPGObj* const baseObject = baseItem->GetObjCopy();
                    ShopGeneratedPushBack(this, baseObject);
                }
            }
        }
        baseItem = m_framework->NextObject();
    }
    iRPGImplant::SeedImplantRand(0);
    return true;
}

void  zShopDlgMenu::RefreshBuyButtons()
{
    if (m_categoryButtonCount == 0) return;

    const auto setText = [](const char* name, const STRING& value)
    {
        Map->SetScriptVariable(STRING(name), value);
    };
    const auto localize = [](const char* key)->STRING
    {
        PROFILE* const profile = ::Strings;
        return profile->GetString(STRING("menu"), STRING(key), STRING(""));
    };

    iRPGObj* selected = m_heroInvMenu->GetSelItem();
    if (!selected)
    {
        const STRING empty("");
        setText("BuyAmmo", empty);
        setText("Price_Ammo", empty);
        setText("BuyWeapon", empty);
        setText("Price_Weapon", empty);
        SetBuyBtnAnim(6, 6);
        return;
    }

    iRPGHero* const hero = m_rpgHeroRef.m_object;
    const int money = hero->GetMoney();
    const bool buyMode = m_heroInvMenu->GetBuyMode();

    bool isWeapon = selected->GetGameObj() == TYPE_WEAPON;
    bool isRepairable = selected->GetGameObj() == TYPE_ARMOR && !buyMode;
    const bool isSpecial = selected->GetGameObj() == 7;

    STRING weaponPrice("");
    if (!isSpecial)
    {
        const int price = buyMode?selected->GetBuyCost():selected->GetSellCost();
        if (buyMode && price>money) weaponPrice = Printf("<Color=0xFF0000/>$%d", price);
        else weaponPrice = Printf("$%d", price);
    }
    setText("Price_Weapon", weaponPrice);

    const char* action = "";
    if (!isSpecial)
    {
        if (buyMode) action = isWeapon?"BuyWeapon":"Buy";
        else action = "Sell";
    }
    setText("BuyWeapon", localize(action));

    STRING secondAction("");
    STRING secondPrice("");
    if (isWeapon)
    {
        zRPGWeapon* const weapon = static_cast<zRPGWeapon*>(selected->QueryInterface(50));
        zRPGAmmo* const ammo = weapon->GetAmmo();
        if (ammo)
        {
            iRPGObj* const ammoObject = static_cast<zRPGObj*>(ammo);
            if (ammoObject->IsInfinite())
            {
                isWeapon = false;
            }
            else
            {
                secondAction = localize("BuyAmmo");
                const int ammoPrice = ammoObject->GetBuyCost();
                const int buyCount = ammoObject->GetBuyNumShop();
                if (buyMode && ammoPrice>money)
                    secondPrice = Printf("<Color=0xFF0000/>%d/$%d", buyCount, ammoPrice);
                else
                    secondPrice = Printf("%d/$%d", buyCount, ammoPrice);
            }
        }
    }
    else if (isRepairable)
    {
        iRPGArmor* const repair = static_cast<iRPGArmor*>(selected->QueryInterface(51));
        const int repairCost = repair->GetRepairCost();
        if (repairCost == 0)
        {
            isRepairable = false;
        }
        else
        {
            secondAction = localize("Repair");
            if (repairCost>money) secondPrice = Printf("<Color=0xFF0000/>%d$", repairCost);
            else secondPrice = Printf("%d$", repairCost);
        }
    }

    setText("BuyAmmo", secondAction);
    setText("Price_Ammo", secondPrice);
    SetBuyBtnAnim((isWeapon || isRepairable)?0:6, isSpecial?6:0);
}

void  zShopDlgMenu::LoadListOfObjects(OBJ_TYPE objectTypeEnum, OBJ_EXTTYPE extTypeEnum)
{
    unsigned int objectType = static_cast<unsigned int>(objectTypeEnum);
    const unsigned int extType = static_cast<unsigned int>(extTypeEnum);

    if (objectType == 2u && extType != 16u) objectType = 14u;
    if (objectType == 14u && extType == 16u)
        DebugAssert(5u, "_eObjType != TYPE_UNKNOWN || _eObjExtType != EXTTYPE_UNKNOWN", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 644u);

    if (static_cast<unsigned int>(m_objectType) == objectType)
    {
        const unsigned int oldExt = static_cast<unsigned int>(m_extType);
        if (oldExt == extType) return;
        if (oldExt >= 9u && oldExt <= 13u && extType >= 9u && extType <= 13u) return;
    }

    (void) ::gpLogCons->LogStr(LT_INFO, "LoadListOfObjects-begin");

    iRPGObj** const generatedBegin = m_generated.m_begin;
    if (!generatedBegin || m_generated.m_end == generatedBegin) GenerateItems();

    m_objectType = static_cast<OBJ_TYPE>(objectType);
    m_extType = static_cast<OBJ_EXTTYPE>(extType);
    ReloadAll();
    m_generatedMode = objectType == TYPE_IMPLANT;

    zShopListLayout* const shopList = &m_objects;
    if (m_objects.m_size == 0u)
    {
        for (unsigned int i = 0u; m_generated.m_begin &&
            i<static_cast<unsigned int>(m_generated.m_end-m_generated.m_begin); ++i)
        {
            iRPGObj* const object = m_generated.m_begin[i];
            if (object->GetAddLevel() <= m_curLevel)
                ShopListPushBack(shopList, &m_generated.m_begin[i]);
        }

        iRPGObj* object = m_framework->FirstObject(1, 0, 0);
        while (object)
        {
            if (object->GetBuyCost()>0 &&
                object->GetAddLevel() <= m_curLevel &&
                !object->ItemIsNotReal() &&
                object->GetGameObj() != TYPE_IMPLANT)
            {
                if (object->GetGameObj() != TYPE_AMMO)
                {
                    const int saved = object->GetNumInCell();
                    object->SetNumInCell(object->GetMaxNumInCell());
                    int count = object->GetMaxShop(m_curLevel);
                    if (count>0)
                    {
                        do
                        {
                            iRPGObj* const copy = object->GetObjCopy();
                            ShopListPushBack(shopList, &copy);
                            --count;
                        }
                        while (count != 0);
                    }
                    object->SetNumInCell(saved);
                }
                else
                {
                    const int randomRange = object->GetShopNum(1);
                    const int randomCount = Random(randomRange);
                    int count = AddSigned32Wrap(randomCount, object->GetShopNum(0));
                    if (count>0)
                    {
                        do
                        {
                            iRPGObj* const baseObject = object->GetObjCopy();
                            baseObject->SetNumInCell(baseObject->GetMaxNumInCell());
                            ShopListPushBack(shopList, &baseObject);
                            --count;
                        }
                        while (count != 0);
                    }
                }
            }
            object = m_framework->NextObject();
        }
    }

    iHeroInvMenu* const heroMenu = m_heroInvMenu;
    iRPGObj* const selected = heroMenu->GetSelItem();
    if (selected &&
        (selected->GetInvId() == 20 || selected->GetInvId() == 21))
        m_heroInvMenu->ResetSelItem();

    zInventory3D* inventory14 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    int inventoryCount = inventory14->GetItemCount();
    for (int i = 0; i<inventoryCount; ++i)
    {
        iRPGObj* const item = inventory14->GetItem(i);
        ShopListPushBack(shopList, &item);
    }
    inventory14->RemoveAllItems();

    zInventory3D* inventory15 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(21)->QueryInterface(48));
    inventoryCount = inventory15->GetItemCount();
    for (int i = 0; i<inventoryCount; ++i)
    {
        iRPGObj* const item = inventory15->GetItem(i);
        ShopListPushBack(shopList, &item);
    }
    inventory15->RemoveAllItems();

    inventory14 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    ShopAmmoKeys weaponAmmoKeys;
    zShopListNode* sentinel = m_objects.m_sentinel;
    zShopListNode* node = sentinel->next;
    while (node != sentinel)
    {
        if (!node->value) DebugAssert(5u, "*j != NULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 745u);

        bool matches = false;
        if (static_cast<unsigned int>(m_extType) != 16u)
        {
            const int detail = node->value->GetAddMoney();
            if (static_cast<unsigned int>(detail) == static_cast<unsigned int>(m_extType)) matches = true;
        }
        if (!matches && static_cast<unsigned int>(m_objectType) != 14u)
        {
            const int type = node->value->GetGameObj();
            if (static_cast<unsigned int>(type) == static_cast<unsigned int>(m_objectType)) matches = true;
        }
        if (matches)
        {
            iRPGObj* transferObject = node->value;
            if (transferObject->GetGameObj() != TYPE_IMPLANT) transferObject = node->value->GetObjCopy();
            zItem* const item = static_cast<zItem*>(transferObject->QueryInterface(49));
            (void) inventory14->PutItem(item, static_cast<zItem**>(0));

            if (node->value->GetGameObj() == TYPE_WEAPON)
            {
                zRPGWeapon* const weapon = static_cast<zRPGWeapon*>(node->value->QueryInterface(50));
                zRPGAmmo* const ammo = weapon->GetAmmo();
                iRPGObj* const ammoObject = static_cast<zRPGObj*>(ammo);
                if (!ammoObject->IsInfinite())
                    weaponAmmoKeys.PushBack(ammoObject->GetBaseId());
            }

            zShopListNode* const next = node->next;
            node->previous->next = next;
            next->previous = node->previous;
            CrtFreePointer(node);
            --m_objects.m_size;
            node = next;
            continue;
        }
        node = node->next;
    }

    sentinel = m_objects.m_sentinel;
    node = sentinel->next;
    inventory15 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(21)->QueryInterface(48));
    while (node != sentinel)
    {
        if (!node->value) DebugAssert(5u, "*j != NULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopDlgMenu.cpp", 778u);

        const int key = node->value->GetBaseId();
        if (node->value->GetGameObj() == TYPE_AMMO)
        {
            if (weaponAmmoKeys.Contains(key))
            {
                iRPGObj* const baseObject = node->value->GetObjCopy();
                zItem* const item = static_cast<zItem*>(baseObject->QueryInterface(49));
                (void) inventory15->PutItem(item, static_cast<zItem**>(0));

                zShopListNode* const next = node->next;
                node->previous->next = next;
                next->previous = node->previous;
                CrtFreePointer(node);
                --m_objects.m_size;
                node = next;
                continue;
            }
        }
        node = node->next;
    }

    int cellX = 37;
    int cellY = 37;
    int rows = 7;
    if (extType == 1u || objectType == 5u || objectType == 6u || objectType == 3u)
    {
        cellX = 50;
        cellY = 50;
        rows = 5;
    }
    else if (extType == 2u || extType == 3u || extType == 4u || extType == 5u)
    {
        cellX = 41;
        cellY = 50;
        rows = 6;
    }
    else if (objectType == 4u)
    {
        cellX = 42;
        cellY = 50;
        rows = 6;
    }

    inventory14 = static_cast<zInventory3D*>(m_invServerRef.m_object->GetInv(20)->QueryInterface(48));
    inventory14->SetCellSize(cellX, cellY);
    zInventory2D* const grid2D = inventory14;
    (void) static_cast<iInventory2D*>(grid2D)->SetSizeInCells(rows, grid2D->m_gridHeight);
    GoHome();
    RefreshBuyButtons();
    (void) ::gpLogCons->LogStr(LT_INFO, "LoadListOfObjects-end");
}


