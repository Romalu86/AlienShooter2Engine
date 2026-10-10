#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"
#include "mylib.h"
#include "basetype.h"
#include "zm/IniParser/zIniParser.h"
class iInvServer;
void __cdecl GetNew_zInvServer(iInvServer** outObject);

#include "input.h"
#include "zRPGHero.h"
#include "../../iman.h"
#include "../Utils/zRefObject.h"
#include <new>
#include "zRPGWeapon.h"
#include "zRPGAmmo.h"
#include "zRPGBaseObject.h"
#include "zRPGFrameWork.h"
#include "../Inventory/zInventoryItem.h"
#include "map.h"
#include "dialogSrv.h"


int zRPGHero::m_iOnGetMoney = 0;
int zRPGHero::m_iOnGetExperience = 0;
int zRPGHero::m_iGetExpToGetLevel = 0;
bool zRPGHero::m__abChangeableParams[MAX_RPGPARAMSCOUNT];
bool zRPGHero::m__abBaseParams[MAX_RPGPARAMSCOUNT];
bool zRPGHero::m__abArrayParams[MAX_RPGPARAMSCOUNT];

namespace
{
int experienceByLevel[MAX_HERO_LEVEL] = {};
int maximumLevelExperience = 0;
unsigned char experienceCacheInitialized = 0;
}

void zRPGHeroBase::SetSRandVal(unsigned int value)
{
    m_iSRandVal = BitsToSigned32(value);
}

iInvServer* zRPGHero::GetInvServer()
{
    return m_pInvServer;
}

int zRPGHero::GetUseMedTime()
{
    return m_iUseMedTime;
}

zInventory* zRPGHero::GetCustomInv(M_MODE mode)
{
    return mode == MODE_STAND_FIRM ? &m_customInventory0 : &m_customInventory1;
}

void zRPGHero::SelectBestWeapon()
{
    SelectBestWeapon(-1);
}

void zRPGHero::SetAutoChangeWeapons(bool value)
{
    m_autoChangeWeapons = value;
}

void zRPGHero::SetPlayer(iPlayer* value)
{
    m_player = value;
}

void zRPGHero::SetInvulnerable(bool value)
{
    m_invulnerable = value;
}

void zRPGHero::SetDamageCoeff(float value)
{
    m_damageCoeff = value;
}

bool zRPGHero::CanHoldItm(const zItem* item) const
{
    return CanHoldObj(static_cast<const iRPGObj*>(item));
}

bool zRPGHero::IfIncreasedByImplant(int _iSkillNum)
{
    if (_iSkillNum<0 || _iSkillNum >= MAX_RPGPARAMSCOUNT)
        (void)DebugAssert(5u, "_iSkillNum >= 0 && _iSkillNum < MAX_RPGPARAMSCOUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1513u);
    return m_implantIncreased[_iSkillNum] != 0;
}

void zRPGHero::IncreasedByImplant(int _iSkillNum, bool value)
{
    if (_iSkillNum<0 || _iSkillNum >= MAX_RPGPARAMSCOUNT)
        (void)DebugAssert(5u, "_iSkillNum >= 0 && _iSkillNum < MAX_RPGPARAMSCOUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1504u);
    m_implantIncreased[_iSkillNum] = value ? 1u : 0u;
}

bool zRPGHero::HasMoneyToBuy(const iRPGObj* obj)
{
    const iRPGObj* const _pObj = obj;
    if (!_pObj)
        DebugAssert(5u, "_pObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1590u);
    const unsigned int type = static_cast<unsigned int>(_pObj->GetInvId());
    if (type == 20u || type == 21u)
        return _pObj->GetBuyCostFull() <= GetMoney();
    return true;
}

bool zRPGHero::SetHp(int value)
{
    if (value>20000)
    {
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 960u);
        value = 20000;
    }
    const int denominator = static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_MAXHP);
    if (denominator == 0)
        return false;
    unsigned int numerator = static_cast<unsigned int>(value);
    numerator *= RPG_BASE_HP;
    return SetHpBase(BitsToSigned32(numerator)/denominator);
}

bool zRPGHero::AddHp(int _iHpToAdd, bool flag)
{
    if (_iHpToAdd<0)
        DebugAssert(4u, "_iHpToAdd >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 985u);
    const int base = GetHp();
    int limit = static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_MAXHP);
    if (flag)
        limit = AddSigned32Wrap(limit, _iHpToAdd);
    int candidate = AddSigned32Wrap(base, _iHpToAdd);
    if (candidate>limit)
        candidate = limit;
    return candidate>base ? SetHp(candidate) : false;
}

int zRPGHero::TryToBuyObj(iRPGObj* _pBaseItem, iRPGObj** outObject, int memberIndex)
{
    if (!_pBaseItem)
        DebugAssert(4u, "_pBaseItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1464u);
    if (!CanBuyItem(_pBaseItem))
        return 0;
    const int ret = BuyItem(_pBaseItem, true, outObject, memberIndex);
    if (ret == 0)
        DebugAssert(4u, "ret != ITEM_ADDEDNOT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1470u);
    return ret;
}

bool zRPGHero::AddMoney(int amount, bool suppressCallback)
{
    int& onGetMoney = m_iOnGetMoney;
    int callback = onGetMoney;
    if (callback == 0)
    {
        callback = m_pRPGFrameWork->ScriptGetNFunction("OnGetMoney");
        onGetMoney = callback;
        if (callback == 0)
            DebugAssert(2u, "m_iOnGetMoney", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 264u);
    }
    iRPGObj* const object = static_cast<zRPGObj*>(this);
    if (amount>0 && !suppressCallback)
    {
        const int extType = static_cast<int>(object->GetExtType());
        const int callbackId = onGetMoney;
        zRPGFrameWork* const framework = m_pRPGFrameWork;
        amount = AddSigned32Wrap(amount,
            framework->ScriptRun(callbackId, "%d%d", extType, amount, 0));
        if (m_player && amount>0)
            m_player->OnAddSkill(RPGHERO_MONEY, amount);
    }
    return object->IncSkillValue(RPGHERO_MONEY, amount);
}

int zRPGHero::GetExpToGetLevel(int level)
{
    if (level >= MAX_HERO_LEVEL)
        level = MAX_HERO_LEVEL;
    unsigned char& cacheInitialized = experienceCacheInitialized;
    if (cacheInitialized == 0)
    {
        for (int index = 0;index != MAX_HERO_LEVEL;++index)
            experienceByLevel[index] = -1;
        cacheInitialized = 1u;
    }
    int& cacheEntry = level == MAX_HERO_LEVEL
        ? maximumLevelExperience : experienceByLevel[level];
    int cached = cacheEntry;
    if (cached != -1)
        return cached;
    int& getExpToGetLevel = m_iGetExpToGetLevel;
    int callback = getExpToGetLevel;
    if (callback == 0)
    {
        callback = m_pRPGFrameWork->ScriptGetNFunction("GetExpToGetLevel");
        getExpToGetLevel = callback;
        if (callback == 0)
            DebugAssert(2u, "m_iGetExpToGetLevel", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 350u);
        callback = getExpToGetLevel;
    }
    cached = m_pRPGFrameWork->ScriptRun(callback, "%d", level, 0, 0);
    cacheEntry = cached;
    return cached;
}

void zRPGHero::AddExperience(int amount, bool notifyPositiveGain)
{
    int& onGetExperience = m_iOnGetExperience;
    int callback = onGetExperience;
    if (callback == 0)
    {
        callback = m_pRPGFrameWork->ScriptGetNFunction("OnGetExperience");
        onGetExperience = callback;
        if (callback == 0)
            DebugAssert(2u, "m_iOnGetExperience", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 285u);
    }
    iRPGObj* const object = static_cast<zRPGObj*>(this);
    zRPGFrameWork* const framework = m_pRPGFrameWork;
    const int extType = static_cast<int>(object->GetExtType());
    const int callbackId = onGetExperience;
    amount = AddSigned32Wrap(amount,
        framework->ScriptRun(callbackId, "%d%d", extType, amount, 0));
    if (m_player && amount>0 && notifyPositiveGain)
        m_player->OnAddSkill(RPGHERO_EXP, amount);
    (void)object->IncSkillValue(RPGHERO_EXP, amount);
}

void zRPGHero::CopyParametersFrom(iRPGObj* _BaseObj)
{
    zRPGObj* const object = static_cast<zRPGObj*>(this);
    const int ownType = object->GetGameObj();
    const int sourceType = _BaseObj->GetGameObj();
    if (ownType != sourceType)
        DebugAssert(5u, "_BaseObj->GetType() == this->GetType()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1522u);
    LoadingHeroParameters = true;
    zRPGHero* const clone = static_cast<zRPGHero*>(_BaseObj->QueryInterface(0));
    if (clone->m_pItemTaker)
        m_pItemTaker = clone->m_pItemTaker;
    if (clone->m_pRPGFrameWork)
        m_pRPGFrameWork = clone->m_pRPGFrameWork;
    if (clone->m_player)
        m_player = clone->m_player;
    if (!m_pRPGFrameWork)
        DebugAssert(5u, "m_pRPGFrameWork", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1535u);
    m_autoChangeWeapons = 0;
    object->zRPGObj::CopyParametersFrom(_BaseObj);
    m_pInvServer->CopyParametersFrom(clone->m_pInvServer);
    m_iCurWeapon = clone->m_iCurWeapon;
    m_autoChangeWeapons = clone->m_autoChangeWeapons;
    const int cloneState = object->GetClone();
    object->SetClone(cloneState|2);
    LoadingHeroParameters = false;
}

int zRPGHero::TryToSellObjj(iRPGObj* _pItem, bool _bFull)
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1481u);
    const int baseValue = _bFull ? _pItem->GetNumInCell() : _pItem->GetBuyNumShop();
    const int WasNotPut = _pItem->AddNumInCell(BitsToSigned32(0u-static_cast<unsigned int>(baseValue)));
    const int adjustedValue = AddSigned32Wrap(baseValue, WasNotPut);
    if (_bFull && WasNotPut != 0)
        DebugAssert(5u, "!_bFull || WasNotPut == 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1487u);
    if (_pItem->GetSellCost() != 0)
    {
        const int factor = _pItem->GetSellCost();
        const int product = MultiplySigned32Wrap(factor, adjustedValue);
        const int denominator = _pItem->GetBuyNumShop();
        const int scaled = product/denominator;
        if (!AddMoney(scaled, true))
            DebugAssert(4u, "buf", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1493u);
    }
    return adjustedValue;
}

void zRPGHero::OnTakeItem(zItem* _pItem, iInventory* _pSender, int flags)
{
    if (!_pItem || !_pSender)
        DebugAssert(5u, "_pItem && _pSender", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 740u);
    zRPGObj* const owner = static_cast<zRPGObj*>(this);
    if (_pItem->GetGameObj() == TYPE_IMPLANT && _pSender->IsActive())
        owner->RemoveParent(static_cast<iRPGObj*>(_pItem), false);
    else         _pItem->RemoveParent(static_cast<iRPGObj*>(owner), false);
    if (_pItem->GetGameObj() == TYPE_EQUIP && _pSender->IsActive())
        SetItemDirty(static_cast<iRPGObj*>(_pItem), 3);
    if (m_inventories[19] == _pSender)
    {
        if (!m_pItemTaker)
            DebugAssert(5u, "m_pItemTaker", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 755u);
        if (flags == 0)
            (void)m_pItemTaker->DeleteGroundSprite(static_cast<iRPGObj*>(_pItem), false);
    }
    m_takenWeaponIndex = -1;
    const int active = m_iCurWeapon;
    if (active >= 0 && m_inventories[active] == _pSender)
    {
        m_takenWeaponIndex = active;
        (void)CheckIfNoAmmo();
    }
}

void zRPGHero::UseObject(iRPGObj* _pObj)
{
    if (!_pObj || _pObj->GetGameObj() != TYPE_EQUIP)
        DebugAssert(5u, "_pObj && _pObj->GetType() == TYPE_EQUIP", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1140u);
    if (_pObj->GetType() != static_cast<OBJ_TYPE>(9))
    {
        SetItemDirty(_pObj, 1);
        return;
    }
    const bool enabled = _pObj->GetSkillValue(RPGEQUIP_IGNOREMAXHP) != 0;
    const int value = _pObj->GetSkillValue(RPGEQUIP_ADDHP);
    if (!AddHp(value, enabled))
        return;
    unsigned int itemId = 0u;
    if (_pObj->GetNumInCell() == 1)
        itemId = static_cast<unsigned int>(_pObj->GetBaseId());
    (void)_pObj->AddNumInCell(-1);
    m_player->PlaySFx(165, true);
    if (m_manRef.m_object)
        m_manRef.m_object->CreateEff(1393);
    if (itemId != 0u)
    {
        iRPGObj* selected = m_pInvServer->GetItemByBaseId(static_cast<int>(itemId), 2);
        if (!selected)
            selected = m_pInvServer->GetEquipEx(9, 2);
        if (selected)
            (void)m_pInvServer->PutToActiveInv(selected);
    }
}

zRPGHero::zRPGHero(iRPGFrameWork* frameWork)
    : zRPGHeroBase(static_cast<OBJ_TYPE>(TYPE_HERO), RPGHERO_PARAMS_COUNT),
      m_iCurWeapon(-1), m_takenWeaponIndex(-1), m_pendingWeaponIndex(0), m_player(0),
      m_pItemTaker(0), m_pInvServer(0),
      m_pRPGFrameWork(static_cast<zRPGFrameWork*>(frameWork)),
      m_dirtyItemAllocatorState(static_cast<unsigned char>(PtrToU32(frameWork))),
      m_aiDirtyItems
{
    0, 0, 0
},
      m_invulnerable(0), m_autoChangeWeapons(1), m_damageCoeff(1.0f), m_hpFraction(0),
      m_manRef
{
    0, 0, 0
}, m_iUseMedTime(0), m_medUseTimestamp(0),
      m_customInventory0(555), m_customInventory1(666), m_unlimitedAmmo(0)
{
    GetNew_zInvServer(&m_pInvServer);
    m_pInvServer->InitializeInventories();
    m_pInvServer->SetRPGHero(static_cast<iRPGHero*>(this));
    for (unsigned int i = 0;i<22u;++i)
        m_inventories[i] = m_pInvServer->GetInv(static_cast<int>(i));
    for (unsigned int i = 0;i<sizeof(m_implantIncreased);++i)
        m_implantIncreased[i] = 0;
}

int zRPGHero::NoSkillsToHold(const iRPGObj* _pObj, int* outRequirements) const
{
    iRPGObj* object = const_cast<iRPGObj*>(_pObj);
    if (!_pObj)
        DebugAssert(5u, "_pObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1306u);
    if (m_pRPGFrameWork->GetArcadeMod_() != 0)
        return 0;
    int local[10];
    if (!outRequirements)
        outRequirements = local;
    zRPGObj* stats = static_cast<zRPGObj*>(const_cast<zRPGHero*>(this));
    int count = 0;
    const unsigned int type = static_cast<unsigned int>(object->GetGameObj());
    if (type == static_cast<unsigned int>(TYPE_WEAPON))
    {
        const unsigned int sub = static_cast<unsigned int>(object->GetAddMoney());
        int requirement = -1;
        bool checkRequirement = true;
        switch (sub)
        {
            case 1u:
                requirement = RPGHERO_GUNS;
            break;
            case 2u:
                requirement = RPGHERO_SHORTGUNS;
            break;
            case 3u:
                requirement = RPGHERO_AUTOGUNS;
            break;
            case 4u:
                requirement = RPGHERO_GRANADEGUNS;
            break;
            case 5u:
                requirement = RPGHERO_ENERGOGUNS;
            break;
            case 0u:
                checkRequirement = false;
            break;
            default:
                DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1343u);
            break;
        }
        if (checkRequirement)
        {
            const int have = stats->GetSkillValue(requirement);
            const int need = object->GetSkillValue(RPGWEAPON_LEVEL);
            if (have<need)
            {
                outRequirements[0] = 2;
                count = 1;
            }
        }
    }
    else if (object->GetGameObj() == TYPE_ARMOR)
    {
        if (object->GetSkillValue(RPGARMOR_HP) == 0)
            outRequirements[count++] = 3;
        const int have = stats->GetSkillValue(RPGHERO_ARMOR);
        const int need = object->GetSkillValue(RPGARMOR_LEVEL);
        if (have<need)
            outRequirements[count++] = 1;
    }
    else if (object->GetGameObj() == TYPE_IMPLANT)
    {
        const int have = stats->GetSkillValue(RPGHERO_INTELLIGENCE);
        const int need = object->GetSkillValue(RPGIMPLANT_LEVEL);
        if (have<need)
        {
            outRequirements[0] = RPGIMPLANT_LEVEL;
            count = 1;
        }
    }
    return count;
}

bool zRPGHero::SwitchToMode(M_MODE mode)
{
    if (mode == MODE_CAREER)
    {
        m_unlimitedAmmo = 0;
        return true;
    }
    m_unlimitedAmmo = mode == MODE_STAND_FIRM ? 1u : 0u;
    for (int i = 0;i<19;++i)
    {
        iInventory* const handle = m_pInvServer->GetInv(i);
        zInventory* const inventory = static_cast<zInventory*>(handle->QueryInterface(46));
        inventory->RemoveAllItems();
    }
    iRPGObj* const stats = static_cast<zRPGObj*>(this);
    const char* const name = stats->GetName();
    iRPGObj* const baseObj = m_pRPGFrameWork->GetObjByNamee(name, true);
    if (!baseObj || !baseObj->IsBaseObject())
        DebugAssert(5u, "baseObj && baseObj->IsBaseObject()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1631u);
    if (!baseObj)
        return false;
    iRPGHero* const baseHero = static_cast<iRPGHero*>(baseObj->QueryInterface(0));
    zInventory* const pCustInv = baseHero->GetCustomInv(mode);
    const int count = pCustInv->m_apItems.begin         ? static_cast<int>(pCustInv->m_apItems.end-pCustInv->m_apItems.begin) : 0;
    if (!pCustInv->m_apItems.begin || count <= 0)
        DebugAssert(4u, "pCustInv->GetItemsCnt() > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1637u);
    for (int i = 0;i<(pCustInv->m_apItems.begin         ? static_cast<int>(pCustInv->m_apItems.end-pCustInv->m_apItems.begin) : 0);++i)
    {
        zItem* const entry = pCustInv->GetItem(i);
        (void)PutItem(entry->GetObjCopy());
    }
    SelectBestWeapon();
    return true;
}

void zRPGHero::ResetScriptFunctions()
{
    int& onGetMoney = m_iOnGetMoney;
    int& onGetExperience = m_iOnGetExperience;
    int& getExpToGetLevel = m_iGetExpToGetLevel;
    onGetMoney = 0;
    onGetExperience = 0;
    getExpToGetLevel = 0;
}


bool zRPGHero::GetDirty()
{
    bool dirty = static_cast<zRPGObj*>(this)->zRPGObj::GetDirty();
    if (!dirty)
        dirty = m_pInvServer->GetDirty();
    return dirty;
}

int zRPGHero::GetNextExpFromExp(int value)
{
    const int level = GetLevelFromExp(value);
    return GetExpToGetLevel(AddSigned32Wrap(level, 1));
}

iRPGObj* zRPGHero::GetCurWpn() const
{
    zRPGWeapon* weapon = GetCurWeapon();
    return weapon ? static_cast<iRPGObj*>(weapon) : 0;
}

const iRPGObj* zRPGHero::GetArmor()
{
    return m_pInvServer->GetArmor();
}

const iRPGObj* zRPGHero::GetEquip(int value)
{
    return m_pInvServer->GetEquip(value);
}

bool zRPGHero::SetHpToMax()
{
    return SetHp(static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_MAXHP));
}

bool zRPGHero::CanHoldObj(const iRPGObj* value) const
{
    return NoSkillsToHold(value, 0) == 0;
}

zRPGWeapon* zRPGHero::GetCurWeapon() const
{
    if (m_iCurWeapon<0)
        return 0;
    iRPGObj* item = m_pInvServer->GetObj(m_iCurWeapon, static_cast<OBJ_TYPE>(TYPE_WEAPON), static_cast<OBJ_EXTTYPE>(16), 1);
    return item ? static_cast<zRPGWeapon*>(item->QueryInterface(50)) : 0;
}

int zRPGHero::GetLevelFromExp(int threshold)
{
    int level = 1;
    while (level<MAX_HERO_LEVEL)
    {
        const int value = GetExpToGetLevel(level);
        if (value>threshold)
            break;
        ++level;
    }
    return level-1;
}

bool zRPGHero::IsBaseObject() const
{
    const zRPGObj* object = static_cast<const zRPGObj*>(this);
    const zItem* item = static_cast<const zItem*>(object);
    if (!item->zItem::IsBaseObject())
        return false;
    return object->GetClone() == 0;
}

int zRPGHero::GetHp() const
{
    const int base = static_cast<const zRPGObj*>(this)->GetSkillValue(RPGHERO_MAXHP);
    const int product = MultiplySigned32Wrap(base, m_hpFraction);
    return AddSigned32Wrap(product, RPG_BASE_HP/2)/RPG_BASE_HP;
}


bool zRPGHero::GetAmmoDirty()
{
    return static_cast<zRPGObj*>(this)->zRPGObj::GetAmmoDirty();
}

bool zRPGHero::CreateGroundSprite(iRPGObj* value) const
{
    if (!m_pItemTaker)
        DebugAssert(5u, "m_pItemTaker", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1572u);
    return m_pItemTaker->CreateGroundSprite(value);
}

bool zRPGHero::SetHpBase(int value)
{
    if (m_hpFraction == value)
        return false;
    iMan* const man = m_manRef.m_object;
    m_hpFraction = value;
    if (man)
        (void)man->SetHpFromHero(GetHp());
    static_cast<zRPGObj*>(this)->SetAmmoDirty();
    return true;
}

void zRPGHero::OnSkillChange(int action, int oldValue, int value)
{
    zRPGObj* const object = static_cast<zRPGObj*>(this);
    object->zRPGObj::OnSkillChange(action, oldValue, value);
    if (action == RPGHERO_ACTIVECELLS)
    {
        m_pInvServer->SetActCellsCnt(value);
        return;
    }
    if (action == RPGHERO_EXP)
    {
        object->SetSkillValue(RPGHERO_LEVEL, GetLevelFromExp(value));
        object->SetSkillValue(RPGHERO_NEXTLEVELEXP, GetNextExpFromExp(value));
    }
}

int zRPGHero::GetItemDirty(int key)
{
    zRPGHero::DIRTY_ITEM* const begin = m_aiDirtyItems.m_begin;
    zRPGHero::DIRTY_ITEM* const end = m_aiDirtyItems.m_end;
    for (zRPGHero::DIRTY_ITEM* entry = begin;entry<end;++entry)
    {
        if (static_cast<int>(entry->m_type) != key)
            continue;
        const int result = entry->m_mode;
        for (zRPGHero::DIRTY_ITEM* next = entry+1;next != end;++next)
            *(next-1) = *next;
        m_aiDirtyItems.m_end = end-1;
        return result;
    }
    return 0;
}

void zRPGHero::SetItemTaker(iItemTaker* _pItemTaker)
{
    if (_pItemTaker == 0)
        (void)DebugAssert(5u, "_pItemTaker", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 849u);
    m_pItemTaker = _pItemTaker;
    if (!m_pInvServer)
        (void)DebugAssert(5u, "m_pInvServer", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 851u);
    m_pInvServer->SetItemTaker(_pItemTaker);
}

int zRPGHero::CanBuyItem(const iRPGObj* object)
{
    iRPGObj* item = const_cast<iRPGObj*>(object);
    const int required = item->GetBuyCost();
    if (required>GetMoney())
        return 0;
    return m_pInvServer->IfObjectCanBePut(item, true);
}

bool zRPGHero::CheckIfNoAmmo()
{
    zRPGWeapon* weapon = GetCurWeapon();
    if (weapon && CanHoldObj(static_cast<iRPGObj*>(weapon)))
    {
        zRPGAmmo* ammo = weapon->GetAmmo();
        if (GetAmmoCnt(ammo) != 0)
            return false;
    }
    if (!SetNextPrevWeapon(false, true, true))
        (void)SetNextPrevWeapon(true, true, true);
    return true;
}

int zRPGHero::Damage(int value)
{
    const int MaxHp = static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_MAXHP);
    if (MaxHp == 0)
        (void)DebugAssert(5u, "MaxHp", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1396u);
    const int scaled = BitsToSigned32(LowMul100000(value))/MaxHp;
    const int result = AddSigned32Wrap(m_hpFraction, BitsToSigned32(0u-static_cast<unsigned int>(scaled)));
    (void)SetHpBase(result);
    return 0;
}

void iRef<iMan>::Set(iMan* target)
{
    iRef<iRefObject>* const relation = reinterpret_cast<iRef<iRefObject>*>(this);
    if (m_object)
        m_refObject->RemObjectRef(relation);
    if (!target)
    {
        m_object = 0;
        m_refObject = 0;
        return;
    }
    m_object = target;
    m_refObject = static_cast<zRefObject*>(target->QueryInterface(2));
    if (m_object)
        m_refObject->AddObjectRef(relation);
}

void zRPGHero::SetMan(iMan* next)
{
    m_manRef.Set(next);
    iMan* const man = m_manRef.m_object;
    if (man)
        (void)man->SetHpFromHero(GetHp());
}

void zRPGHero::SelectBestWeapon(int preferredIndex)
{
    zRPGWeapon* currentWeapon = GetCurWeapon();
    if (preferredIndex >= 0)
    {
        iRPGObj* candidate = m_pInvServer->GetObj(preferredIndex, static_cast<OBJ_TYPE>(TYPE_WEAPON), static_cast<OBJ_EXTTYPE>(16), 3);
        if (candidate == 0)
            return;
        iRPGObj* currentInterface = currentWeapon             ? static_cast<iRPGObj*>(currentWeapon)             : 0;
        if (!IsWpnClassBetter(                       currentInterface, candidate, false))
            return;
    }
    else
    {
        for (int index = 0;index<19;++index)
        {
            if (index == m_iCurWeapon)
                continue;
            iRPGObj* candidate = m_pInvServer->GetObj(index, static_cast<OBJ_TYPE>(TYPE_WEAPON), static_cast<OBJ_EXTTYPE>(16), 3);
            if (candidate == 0)
                continue;
            iRPGObj* currentInterface = currentWeapon                 ? static_cast<iRPGObj*>(currentWeapon)                 : 0;
            if (IsWpnClassBetter(                           currentInterface, candidate, true))
                preferredIndex = index;
        }
    }
    if (preferredIndex >= 0)
        (void)SetCurWeapon(preferredIndex, false);
}

bool zRPGHero::IsWpnClassBetter(iRPGObj* currentInterface, iRPGObj* candidate, bool validateCandidate)
{
    if (!currentInterface)
        return candidate != 0;
    if (!candidate)
        return false;
    if (validateCandidate)
    {
        zRPGWeapon* const weapon = static_cast<zRPGWeapon*>(candidate->QueryInterface(50));
        zRPGAmmo* const ammo = weapon->GetAmmo();
        if (!ammo)
            return false;
        iRPGObj* const ammoObject = static_cast<zRPGObj*>(ammo);
        if (!ammoObject->IsInfinite())
        {
            const int baseId = ammoObject->GetBaseId();
            if (!m_pInvServer->GetItemByBaseId(baseId, 2))
                return false;
        }
    }
    const int currentClass = currentInterface->GetSkillValue(RPGWEAPON_CLASSS);
    const int candidateClass = candidate->GetSkillValue(RPGWEAPON_CLASSS);
    return candidateClass >= currentClass;
}

bool zRPGHero::SetNextPrevWeapon(bool previous, bool requireAmmo, bool allowSlotOne)
{
    const int step = previous ? -1 : 1;
    int candidateIndex = AddSigned32Wrap(m_iCurWeapon, step);
    int minimumIndex = 1;
    if (!allowSlotOne)
    {
        if (static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_PERK_HANDTOHAND) == 0)
            minimumIndex = 2;
    }
    while (candidateIndex >= minimumIndex && candidateIndex<19)
    {
        iRPGObj* candidate = m_pInvServer->GetObj(candidateIndex, static_cast<OBJ_TYPE>(TYPE_WEAPON), static_cast<OBJ_EXTTYPE>(16), 1);
        if (candidate != 0)
        {
            zRPGWeapon* const candidateWeapon = static_cast<zRPGWeapon*>(candidate->QueryInterface(50));
            if (CanHoldObj(candidate))
            {
                bool accepted = true;
                if (requireAmmo)
                {
                    accepted = GetAmmoCnt(candidateWeapon->GetAmmo()) != 0;
                }
                if (accepted)
                {
                    (void)SetCurWeapon(candidateIndex, false);
                    return true;
                }
            }
        }
        candidateIndex = AddSigned32Wrap(candidateIndex, step);
    }
    return false;
}

bool zRPGHero::UseMedkitIfNeed(int* quota)
{
    iRPGObj* medkit = m_pInvServer->GetEquipEx(9, 1);
    if (!medkit)
        return false;
    const int requiredQuota = medkit->GetSkillValue(RPGEQUIP_MaxDamageAutoUse);
    const int threshold = medkit->GetSkillValue(RPGEQUIP_MinHpAutoUse);
    const int healthPercent = static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_HPPERCENT);
    if (healthPercent <= MultiplySigned32Wrap(threshold, 10) && m_iUseMedTime == 0)
    {
        const unsigned int now = static_cast<unsigned int>(m_player->GetCurTime());
        m_medUseTimestamp = BitsToSigned32(now);
        const int delay = medkit->GetSkillValue(RPGEQUIP_AutoUseTime);
        m_iUseMedTime = AddSigned32Wrap(delay, m_medUseTimestamp);
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "2: m_iUseMedTime = %d", m_iUseMedTime);
        m_player->PlaySFx(166, true);
        return false;
    }
    if (!quota)
        return false;
    if (GetHp()>0)
        return false;
    if (requiredQuota == 0 || *quota>requiredQuota)
        return false;
    (void)SetHpBase(0);
    UseObject(medkit);
    *quota = 0;
    return true;
}

int zRPGHero::BuyItem(iRPGObj* source, bool enableRoute, iRPGObj** outObject, int memberIndex)
{
    unsigned int result = 0u;
    iRPGObj* routed = 0;
    const int routeValue = memberIndex != -1 ? source->GetBuyCost() : source->GetBuyCostFull();
    if (AddMoney(BitsToSigned32(0u-static_cast<unsigned int>(routeValue)), false))
    {
        routed = source->GetObjCopy();
        if (memberIndex != -1)
            routed->SetNumInCell(memberIndex);
        if (routed->GetGameObj() == TYPE_WEAPON)
        {
            zRPGWeapon* weapon = static_cast<zRPGWeapon*>(routed->QueryInterface(50));
            weapon->IfIncAmmoOnPut();
        }
        if (enableRoute)
        {
            result = static_cast<unsigned int>(PutItem(routed));
            if (result != 0u && (result&1u) == 0u)
            {
                const int refund = routed->GetBuyCost();
                (void)AddMoney(refund, true);
            }
        }
    }
    if (outObject)
    {
        if (!routed || routed->GetInvId() == -1)
            routed = 0;
        *outObject = routed;
    }
    return BitsToSigned32(result);
}

void zRPGHero::OnPutItem(zItem* _pItem, iInventory* _pSender)
{
    if (!_pItem || !_pSender)
        DebugAssert(5u, "_pItem && _pSender", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 774u);
    zRPGObj* const owner = static_cast<zRPGObj*>(this);
    if (_pItem->GetGameObj() == TYPE_IMPLANT && _pSender->IsActive())
        owner->AddParent(static_cast<iRPGObj*>(_pItem));
    else         _pItem->AddParent(static_cast<iRPGObj*>(owner));
    if (_pItem->GetGameObj() == TYPE_EQUIP && _pSender->IsActive())
        SetItemDirty(static_cast<iRPGObj*>(_pItem), 2);
    if (m_inventories[19] == _pSender)
    {
        if (_pItem->GetNumInCell() <= 0)
            DebugAssert(4u, "_pItem->GetNumInCell() > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 789u);
        if (!m_pItemTaker)
            DebugAssert(5u, "m_pItemTaker", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 790u);
        (void)m_pItemTaker->CreateGroundSprite(static_cast<iRPGObj*>(_pItem));
    }
    const int selected = m_takenWeaponIndex;
    if (selected >= 0)
    {
        if (m_inventories[selected] == _pSender)
            (void)SetCurWeapon(selected, false);
        m_takenWeaponIndex = -1;
    }
    if (_pItem->GetGameObj() == TYPE_WEAPON && m_inventories[19] != _pSender)
    {
        zRPGWeapon* weapon = static_cast<zRPGWeapon*>(_pItem->QueryInterface(50));
        (void)weapon->IfIncAmmoOnPut();
    }
}

void zRPGHero::OnBaseSkillChange(int command, int oldValue, int value)
{
    const bool refresh = (command == RPGHERO_EXP || command == RPGHERO_MONEY || command == RPGHERO_SKILLSTOADD ||                         command == RPGHERO_KILLS || command == RPGHERO_MONSTERDAMAGE || command == RPGHERO_COLOR);
    const bool oldState = GetDirty();
    static_cast<zRPGObj*>(this)->zRPGObj::OnBaseSkillChange(command, oldValue, value);
    if (!oldState && refresh)
    {
        (void)GetDirty();
        static_cast<zRPGObj*>(this)->SetAmmoDirty();
    }
}

int zRPGHero::GetSkillValue(int stat) const
{
    const zRPGObj* const object = static_cast<const zRPGObj*>(this);
    switch (stat)
    {
        case RPGHERO_CURAMMOCNT:
            return GetCurAmmoCnt(false);
        case RPGHERO_GETAMMO1DIR:
            case RPGHERO_GETAMMO2DIR:     case RPGHERO_GETAMMO3DIR:     case RPGHERO_GETAMMO4DIR:     case RPGHERO_GETAMMO5DIR:
        {
            const int slot = stat-(RPGHERO_GETAMMO1DIR-2);
            iRPGObj* selected = m_pInvServer->GetEquip(slot);
            if (!selected)
                return -1;
            zRPGWeapon* weapon = static_cast<zRPGWeapon*>(selected->QueryInterface(50));
            zRPGAmmo* ammo = weapon->GetAmmo();
            const int current = GetAmmoCnt(ammo);
            const iRPGObj* ammoIface = static_cast<const iRPGObj*>(ammo);
            const int maximum = ammoIface->GetMaxNumInCell();
            const int clamped = current<maximum ? current : maximum;
            if (ammoIface->IsInfinite())
                return 1000000;
            const int numerator = MultiplySigned32Wrap(clamped, 1000000);
            return numerator/maximum;
        }
        case RPGHERO__HP:
            return GetHp();
        case RPGHERO_HPPERCENT:
        {
            const int denominator = object->GetSkillValue(RPGHERO_MAXHP);
            if (denominator <= 0)
                return 0;
            const int numerator = MultiplySigned32Wrap(GetHp(), 1000);
            const int scaled = numerator/denominator;
            return scaled>1000 ? 1000 : scaled;
        }
        default:
            return object->zRPGObj::GetSkillValue(stat);
    }
}

int zRPGHero::ActDamage(int _iDamage)
{
    if (_iDamage <= 0)
        DebugAssert(4u, "_iDamage > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1016u);
    int adjusted = TruncateDoubleToInt32(static_cast<double>(_iDamage)*static_cast<double>(m_damageCoeff)+         static_cast<double>(0.5f));
    if (m_invulnerable != 0u)
        adjusted = 0;
    iRPGObj* equipment = m_pInvServer->GetArmor();
    if (equipment && CanHoldObj(equipment))
    {
        const int Defence = equipment->GetSkillValue(RPGARMOR_DEFENCE);
        if (Defence<0 || Defence>100)
            DebugAssert(3u, "Defence >= 0 && Defence <= 100", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1030u);
        if (equipment->Damage(adjusted) == 0)
        {
            static_cast<zRPGObj*>(this)->SetDirty();
            m_player->PlaySFx(38, true);
        }
        adjusted = AddSigned32Wrap(adjusted, -(MultiplySigned32Wrap(Defence, adjusted)/100));
    }
    zRPGObj* stats = static_cast<zRPGObj*>(this);
    const int resist = stats->GetSkillValue(RPGHERO_RESISTANCE);
    if (resist<0 || resist>100)
        DebugAssert(3u, "resist >= 0 && resist <= 100", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1052u);
    adjusted = AddSigned32Wrap(adjusted, -(MultiplySigned32Wrap(resist, adjusted)/100));
    (void)stats->Damage(adjusted);
    if (equipment)
    {
        if (equipment->GetSkillValue(RPGARMOR_HP) <= 0)
        {
            iRPGFrameWork* framework = stats->GetFW();
            if ((framework->GetArcadeMod_()&3) != 0)
            {
                zItem* item = static_cast<zItem*>(equipment->QueryInterface(49));
                if (item->m_pInventory)
                    (void)item->m_pInventory->TakeItem(item);
                (void)stats->GetFW()->ReleaseObject(equipment);
            }
        }
    }
    if (GetHp() <= 0 && m_iUseMedTime != 0)
    {
        m_iUseMedTime = 0;
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "3: m_iUseMedTime = %d", 0);
    }
    return adjusted;
}

void zRPGHero::SetItemDirty(iRPGObj* _pObj, int _iState)
{
    if (_iState == 0)
        DebugAssert(4u, "_iState != DS_NONE", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1187u);
    if (_pObj->GetGameObj() != TYPE_EQUIP)
        DebugAssert(4u, "_pObj->GetType() == TYPE_EQUIP", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1188u);
    DIRTY_ITEM pair;
    pair.m_type = _pObj->GetType();
    pair.m_mode = _iState;
    (void)GetItemDirty(static_cast<int>(pair.m_type));
    const unsigned int dirtyCount = m_aiDirtyItems.m_begin
        ? static_cast<unsigned int>(m_aiDirtyItems.m_end - m_aiDirtyItems.m_begin) : 0u;
    if (dirtyCount >= 10u)
        DebugAssert(4u, "m_aiDirtyItems.size() < 10", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero.cpp", 1197u);

    DIRTY_ITEM* const position = m_aiDirtyItems.m_end;
    const unsigned int available = m_aiDirtyItems.m_capacityEnd == position ? 0u
        : static_cast<unsigned int>(m_aiDirtyItems.m_capacityEnd - position);
    if (available >= 1u)
    {
        if (position)
            new(position) DIRTY_ITEM(pair);
        m_aiDirtyItems.m_end = m_aiDirtyItems.m_end + 1;
        return;
    }

    const unsigned int oldCount = m_aiDirtyItems.m_begin
        ? static_cast<unsigned int>(position - m_aiDirtyItems.m_begin) : 0u;
    const unsigned int growth = oldCount > 1u ? oldCount : 1u;
    const int newCapacity = BitsToSigned32(oldCount + growth);
    const unsigned int allocationCount = newCapacity >= 0
        ? static_cast<unsigned int>(newCapacity) : 0u;
    DIRTY_ITEM* const newBegin = static_cast<DIRTY_ITEM*>(
        CrtNew(allocationCount * sizeof(DIRTY_ITEM)));
    DIRTY_ITEM* write = newBegin;
    for (DIRTY_ITEM* source = m_aiDirtyItems.m_begin; source != position; ++source, ++write)
    {
        if (write)
            new(write) DIRTY_ITEM(*source);
    }
    if (write)
        new(write) DIRTY_ITEM(pair);
    ++write;
    DIRTY_ITEM* const end = m_aiDirtyItems.m_end;
    for (DIRTY_ITEM* source = position; source != end; ++source, ++write)
    {
        if (write)
            new(write) DIRTY_ITEM(*source);
    }
    CrtFreePointer(m_aiDirtyItems.m_begin);
    m_aiDirtyItems.m_capacityEnd = newBegin + newCapacity;
    const unsigned int countAfterRelease = m_aiDirtyItems.m_begin
        ? static_cast<unsigned int>(m_aiDirtyItems.m_end - m_aiDirtyItems.m_begin) : 0u;
    m_aiDirtyItems.m_begin = newBegin;
    m_aiDirtyItems.m_end = newBegin + countAfterRelease + 1u;
}

bool zRPGHero::SetCurWeapon(int requested, bool strictMode)
{
    int next = m_iCurWeapon;
    if (requested >= 0)
    {
        const int searchMode = strictMode?1:3;
        if (m_pInvServer->GetObj(requested, static_cast<OBJ_TYPE>(TYPE_WEAPON), static_cast<OBJ_EXTTYPE>(16), searchMode) != 0)
            next = requested;
    }
    else if (requested == -1)
    {
        next = -1;
    }
    const int current = m_iCurWeapon;
    if (next == current)
        return false;
    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "NewCurWeapon = %d. m_iCurWeapon = %d", next, current);
    zRPGWeapon* outgoingObject = GetCurWeapon();
    zRPGObj* outgoing = outgoingObject ? static_cast<zRPGObj*>(outgoingObject) : 0;
    if (m_player != 0 && outgoing != 0)
    {
        const int volley = m_player->GetNumInVolley();
        (void)outgoing->SetSkillValue(RPGWEAPON_VOLLEY, volley);
        const int reloadTimer = m_player->GetReloadTimer();
        (void)outgoing->SetSkillValue(RPGWEAPON_RELOADTIMER, reloadTimer);
        const int timestamp = m_player->GetCurTime();
        (void)outgoing->SetSkillValue(RPGWEAPON_CHANGETIME, timestamp);
    }
    m_iCurWeapon = next;
    zRPGWeapon* incomingObject = GetCurWeapon();
    zRPGObj* incoming = incomingObject ? static_cast<zRPGObj*>(incomingObject) : 0;
    if (m_player != 0 && incoming != 0)
    {
        const int savedVolley = incoming->GetSkillValue(RPGWEAPON_VOLLEY);
        const int savedReload = incoming->GetSkillValue(RPGWEAPON_RELOADTIMER);
        const int savedTime = incoming->GetSkillValue(RPGWEAPON_CHANGETIME);
        const int now = m_player->GetCurTime();
        const int switchDelay = m_player->GetDelayForRotateAfterFire();
        const int minimumTimer = m_player->GetReloadTimer() != 0 ? switchDelay : 0;
        if (savedTime == 0)
        {
            m_player->SetReloadTimer(minimumTimer);
            m_player->SetNumInVolley(0);
        }
        else if (savedReload>switchDelay)
        {
            const int elapsed = SubSigned32Wrap(now, savedTime);
            if (elapsed<savedReload)
            {
                const int reloadRemaining = AddSigned32Wrap(SubSigned32Wrap(savedTime, now), savedReload);
                m_player->SetReloadTimer(reloadRemaining);
                m_player->SetNumInVolley(savedVolley);
            }
            else
            {
                m_player->SetReloadTimer(0);
                m_player->SetNumInVolley(0);
            }
        }
        else
        {
            m_player->SetReloadTimer(minimumTimer);
            const int volleyDelay = incoming->GetSkillValue(RPGWEAPON_RELOADFULL);
            const int elapsed = SubSigned32Wrap(now, savedTime);
            m_player->SetNumInVolley(elapsed<volleyDelay ? savedVolley : 0);
        }
    }
    static_cast<zRPGObj*>(this)->SetDirty();
    return true;
}

void zRPGHero::Control(INPUT* input)
{
    bool weaponChanged = false;
    if (m_pendingWeaponIndex != 0)
    {
        if (m_player->IsReadyForWeaponChange())
        {
            (void)SetCurWeapon(m_pendingWeaponIndex, true);
            m_pendingWeaponIndex = 0;
            weaponChanged = true;
        }
    }
    else
    {
        for (unsigned int index = 0;index<22u;++index)
        {
            iInventory* binding = m_inventories[index];
            const unsigned int inputId = binding->GetInpAction();
            binding = m_inventories[index];
            if (!binding->IsActive() || inputId == IA_NONE)
                continue;
            if (input->GetAction(inputId) == 0)
                continue;
            iRPGObj* item = m_pInvServer->GetEquip(static_cast<int>(index));
            if (!item)
            {
                if (m_inventories[index]->GetObjectType() != static_cast<OBJ_TYPE>(TYPE_WEAPON))
                    continue;
                (void)m_pInvServer->ChangeWpn_(static_cast<int>(index), 0);
                item = m_pInvServer->GetEquip(static_cast<int>(index));
                if (!item)
                    continue;
            }
            const int itemType = item->GetGameObj();
            if (itemType == TYPE_WEAPON)
            {
                if (!m_player->IsReadyForWeaponChange())
                {
                    m_pendingWeaponIndex = static_cast<int>(index);
                    continue;
                }
                if (m_iCurWeapon != static_cast<int>(index))
                {
                    weaponChanged = SetCurWeapon(static_cast<int>(index), true);
                }
                else
                {
                    weaponChanged = m_pInvServer->ChangeWpn_(static_cast<int>(index), 0);
                    if (m_iCurWeapon != static_cast<int>(index))
                        (void)SetCurWeapon(static_cast<int>(index), true);
                }
            }
            else if (item->GetGameObj() == TYPE_EQUIP)
            {
                UseObject(item);
            }
        }
    }
    if (!weaponChanged && m_player->IsReadyForWeaponChange())
    {
        if (input->GetAction(IA_WPN_PREV) != 0)
            weaponChanged = SetNextPrevWeapon(true, false, false);
        else if (input->GetAction(IA_WPN_NEXT) != 0)
            weaponChanged = SetNextPrevWeapon(false, false, false);
    }
    if (weaponChanged)
        m_player->PlaySFx(79, false);
    (void)UseMedkitIfNeed(0);
    if (m_iUseMedTime == 0)
        return;
    int now = m_player->GetCurTime();
    if (m_medUseTimestamp>now)
        m_iUseMedTime = 0;
    if (m_iUseMedTime == 0)
        return;
    now = m_player->GetCurTime();
    if (now <= m_iUseMedTime)
    {
        m_player->PlaySFx(166, true);
        return;
    }
    m_iUseMedTime = 0;
    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "1: m_iUseMedTime = 0");
    iRPGObj* medkit = m_pInvServer->GetEquipEx(9, 1);
    if (!medkit)
        return;
    const int threshold = medkit->GetSkillValue(RPGEQUIP_MinHpAutoUse);
    const int currentHp = static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_HPPERCENT);
    if (currentHp <= MultiplySigned32Wrap(threshold, 10))
        UseObject(medkit);
}

zRPGHero::DIRTY_ITEM::DIRTY_ITEM()
    : m_type(static_cast<OBJ_TYPE>(-1)), m_mode(0)
{
}


int zRPGHero::GetMoney()
{
    return static_cast<zRPGObj*>(this)->GetSkillValue(RPGHERO_MONEY);
}

void* zRPGHero::QueryInterface(int code) const
{
    zRPGHero* const self = const_cast<zRPGHero*>(this);
    if (code == 0 || code == 52)
        return static_cast<iRPGHero*>(self);
    if (code == 1)
        return static_cast<iInvKeeper*>(self);
    return self->zRPGHeroBase::QueryInterface(code);
}

bool zRPGHero::TakeItem(int value)
{
    const bool ok = m_pInvServer->TakeItem(value);
    if (ok)
        static_cast<zRPGObj*>(this)->SetDirty();
    return ok;
}

int zRPGHero::iRelease()
{
    if (this)
    {
        this->~zRPGHero();
        CrtFreePointer(this);
    }
    return 0;
}


bool zRPGHero::TakeItem(iRPGObj* value)
{
    const bool ok = m_pInvServer->TakeItem(value);
    if (ok)
        static_cast<zRPGObj*>(this)->SetDirty();
    return ok;
}

int zRPGHero::GetCurAmmoCnt(bool requireCheck) const
{
    zRPGWeapon* candidate = GetCurWeapon();
    if (!candidate)
        return 0;
    if (requireCheck && !CanHoldObj(static_cast<iRPGObj*>(candidate)))
        return 0;
    return GetAmmoCnt(candidate->GetAmmo());
}

int zRPGHero::GetAmmoCnt(const zRPGAmmo* value) const
{
    const iRPGObj* ammo = static_cast<const iRPGObj*>(value);
    if (ammo->IsInfinite())
        return 999999;
    if (m_unlimitedAmmo != 0u)
        return 999999;
    const int baseId = ammo->GetBaseId();
    return m_pInvServer->GetObjectCnt(baseId);
}

void zRPGHero::SetDataArrays()
{
    if (m_changeableParams)
        return;
    m_changeableParams = m__abChangeableParams;
    m_baseParams = m__abBaseParams;
    m_arrayParams = m__abArrayParams;
    for (unsigned int index = 0;index<static_cast<unsigned int>(MAX_RPGPARAMSCOUNT);++index)
    {
        m_changeableParams[index] = false;
        m_baseParams[index] = false;
        m_arrayParams[index] = false;
    }
}

void iRef<iMan>::Invalidate()
{
    if (m_object)
    {
        iRef<iRefObject>* const relation = reinterpret_cast<iRef<iRefObject>*>(this);
        m_refObject->RemObjectRef(relation);
        iRefOwner* const observer = m_owner;
        m_object = 0;
        m_refObject = 0;
        if (observer)
            observer->OnInvalidate(relation);
    }
}

iRef<iMan>::~iRef() noexcept(false)
{
    Invalidate();
}

zRPGHero::DirtyItemVectorLayout::~DirtyItemVectorLayout()
{
    CrtFreePointer(m_begin);
    m_begin = 0;
    m_end = 0;
    m_capacityEnd = 0;
}

zRPGHero::~zRPGHero()
{
    (void)m_pInvServer->iRelease();
}


void zRPGHero::DecAmmo(int amount)
{
    if (amount == 0)
        return;
    zRPGWeapon* weapon = GetCurWeapon();
    zRPGAmmo* ammoObject = weapon->GetAmmo();
    iRPGObj* ammo = ammoObject ? static_cast<iRPGObj*>(ammoObject) : 0;
    zRPGObj* weaponObject = static_cast<zRPGObj*>(weapon);
    if (!ammo->IsInfinite() && m_unlimitedAmmo == 0u)
    {
        unsigned int count = static_cast<unsigned int>(weaponObject->GetSkillValue(RPGWEAPON_AMMOUSE));
        if (count == 0u)
            count = 1u;
        const int baseId = ammo->GetBaseId();
        if (m_pInvServer->DecAmmo(baseId, BitsToSigned32(count)))
            static_cast<zRPGObj*>(this)->SetAmmoDirty();
    }
    const int damageMax = weaponObject->GetSkillValue(RPGWEAPON_DAMAGEMAX);
    const int damageMin = weaponObject->GetSkillValue(RPGWEAPON_DAMAGEMIN);
    const int sum = AddSigned32Wrap(damageMax, damageMin);
    (void)static_cast<zRPGObj*>(this)->IncSkillValue(RPGHERO_MONSTERDAMAGE, sum/2);
}

void zRPGHero::InitParams()
{
    (void)SetHp(777);
    zRPGObj* const object = static_cast<zRPGObj*>(this);
    object->SetChangeableParam(RPGHERO_MAXHP);
    object->SetChangeableParam(RPGHERO_ACTIVECELLS);
    object->SetChangeableParam(RPGHERO_RESISTANCE);
    object->SetBaseParam(RPGHERO_HEALTH);
    object->SetBaseParam(RPGHERO_STRENGTH);
    object->SetBaseParam(RPGHERO_ARMOR);
    object->SetBaseParam(RPGHERO_SPEED);
    object->SetBaseParam(RPGHERO_ACCURACY);
    object->SetBaseParam(RPGHERO_INTELLIGENCE);
    object->SetBaseParam(RPGHERO_GUNS);
    object->SetBaseParam(RPGHERO_SHORTGUNS);
    object->SetBaseParam(RPGHERO_AUTOGUNS);
    object->SetBaseParam(RPGHERO_GRANADEGUNS);
    object->SetBaseParam(RPGHERO_ENERGOGUNS);
    object->SetBaseParam(RPGHERO_PERK_QUICKSHOTRELOAD);
    object->SetBaseParam(RPGHERO_PERK_QUICKCLIPRELOAD);
    object->SetBaseParam(RPGHERO_PERK_RESISTANCE);
    object->SetBaseParam(RPGHERO_PERK_HANDTOHAND);
}

int zRPGHero::PutItem(iRPGObj* object)
{
    iRPGObj* acceptedObject = 0;
    int result = m_pInvServer->PutItemEx(object, &acceptedObject);
    if (acceptedObject &&        !acceptedObject->IsBaseObject() &&        m_autoChangeWeapons != 0u &&        !acceptedObject->ItemIsNotReal() &&        acceptedObject->GetNumInCell()>0 &&        acceptedObject->GetGameObj() != TYPE_IMPLANT)
    {
        const int slot = m_pInvServer->PutItemIfBetter(acceptedObject);
        if (slot >= 0)
        {
            if (result == 0)
                result = 1;
            if (acceptedObject->GetGameObj() == TYPE_WEAPON)
                SelectBestWeapon(slot);
        }
        if (slot == -2)
        {
            result = 1;
            static_cast<zRPGObj*>(this)->SetDirty();
            return result;
        }
    }
    if (result != 0)
        static_cast<zRPGObj*>(this)->SetDirty();
    return result;
}


