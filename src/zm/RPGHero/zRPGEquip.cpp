#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"
#include <cstring>
#include "mylib.h"
#include <cstdlib>
#include "zRPGEquip.h"
#include "zm/Game/zGroundItem.h"
#include "zm/Inventory/zInventory.h"
#include "zm/Inventory/zInventoryItem.h"
#include "zRPGAmmo.h"
#include "map.h"
#include "vid.h"
#include "basetype.h"
#include <new>

bool zRPGEquip::m__abArrayParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGEquip::m__abBaseParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGEquip::m__abChangeableParams[MAX_RPGPARAMSCOUNT] = {};


zRPGEquip::zRPGEquip()
    : zRPGObj(static_cast<OBJ_TYPE>(TYPE_EQUIP), RPGEQUIP_PARAMS_COUNT), m_addAmmo(0), m_bpInventory(0)
{
}

zRPGEquip::~zRPGEquip()
{
}

void* zRPGEquip::QueryInterface(int interfaceId) const
{
    if (interfaceId == 5)
        return const_cast<zRPGEquip*>(this);
    return zGroundItem::QueryInterface(interfaceId);
}
int zRPGEquip::iRelease()
{
    this->~zRPGEquip();
    CrtFreePointer(this);
    return 0;
}

void zRPGEquip::SetDataArrays()
{
    if (m_changeableParams)
        return;
    m_changeableParams = m__abChangeableParams;
    m_baseParams = m__abBaseParams;
    m_arrayParams = m__abArrayParams;
    for (int index = 0; index < MAX_RPGPARAMSCOUNT; ++index)
    {
        m_changeableParams[index] = false;
        m_baseParams[index] = false;
        m_arrayParams[index] = false;
    }
}

iRPGObj* zRPGEquip::GetAddAmmo()
{
    return m_addAmmo;
}

zInventory* zRPGEquip::GetBPInv()
{
    return m_bpInventory;
}

bool zRPGEquip::GetParams(const char* token, int length, int& count, int* outIndex) const
{
    count = 1;
    *outIndex = -1;
    const unsigned int tokenLength = static_cast<unsigned int>(length);
    if (std::strncmp("AddHp", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_ADDHP;
    else if (std::strncmp("Class", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_CLASS;
    else if (std::strncmp("RestoreEnergy", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_ENERGYRESTORE;
    else if (std::strncmp("SpotRadius", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_SPOTRADIUS;
    else if (std::strncmp("SpotDistance", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_SPOTDISTANCE;
    else if (std::strncmp("AutoUse", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_AUTOUSE;
    else if (std::strncmp("IgnoreMaxHp", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_IGNOREMAXHP;
    else if (std::strncmp("AddExistingAmmo", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_ADDEXISTINGAMMO;
    else if (std::strncmp("AddLife", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_ADDLIFE;
    else if (std::strncmp("MaxDamageAutoUse", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_MaxDamageAutoUse;
    else if (std::strncmp("MinHpAutoUse", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_MinHpAutoUse;
    else if (std::strncmp("AutoUseTime", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_AutoUseTime;
    else if (std::strncmp("DroneHp", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_DroneHp;
    else if (std::strncmp("DroneMaxHp", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_DroneMaxHp;
    else if (std::strncmp("RepairArmor", token, tokenLength) == 0)
        *outIndex = RPGEQUIP_RepairArmor;
    return *outIndex != -1;
}

bool zRPGEquip::SetSkillValue(int skill, int value)
{
    const bool result = zRPGObj::SetSkillValue(skill, value);
    if (GetAddMoney() == 16 && skill == RPGEQUIP_CLASS)
        m_gameObjectExtType = value >= 1 && value <= 5 ? value + 8 : 16;
    return result;
}

bool zRPGEquip::ParseUnknownCommand(const char* key, int valueCount, const char** values, iRPGFrameWork* framework)
{
    if (zRPGObj::ParseUnknownCommand(key, valueCount, values, framework))
        return true;
    if ((std::strcmp(key, "Energy") == 0) && valueCount == 1)
    {
        const char* text = *values;
        unsigned int energy = 999999;
        if (!(std::strcmp(text, "UNLIMITED_ENERGY") == 0))
            energy = CrtAtoiText(text);
        SetSkillValue(RPGEQUIP_MAXENERGY, BitsToSigned32(energy));
        return true;
    }
    if ((std::strcmp(key, "AddAmmo") == 0) && valueCount == 1)
    {
        iRPGObj* object = framework->GetObjByNamee(*values, true);
        if (object)
        {
            m_addAmmo = object;
            SetSkillValue(RPGEQUIP_AUTOUSE, 1);
            if (object->GetGameObj() == TYPE_AMMO)
            {
                zRPGAmmo* const ammo =
                    static_cast<zRPGAmmo*>(object->QueryInterface(45));
                ammo->m_pEquipOwner = static_cast<zRPGObj*>(this);
            }
            return true;
        }
    }
    return false;
}

void zRPGEquip::CopyParametersFrom(iRPGObj* _BaseObj)
{
    const int sourceType = _BaseObj->GetGameObj();
    if (sourceType != GetGameObj())
        DebugAssert(5u, "_BaseObj->GetType() == this->GetType()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGEquip.cpp", 191u);
    zRPGObj::CopyParametersFrom(_BaseObj);
    iRPGEquip* const clone = static_cast<iRPGEquip*>(_BaseObj->QueryInterface(5));
    m_addAmmo = clone->GetAddAmmo();
    if (GetSkillValue(RPGEQUIP_AUTOUSE) == 0 && GetType() == 0)
    {
        const char* const name = GetName();
        iLogConsole* log = ::gpLogCons;
        (void)log->LogStr(LT_INPTERR, "Error!!! No item type in weapon.cfg for %s", name);
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGEquip.cpp", 201u);
    }
}

int zRPGEquip::GetBuyCost() const
{
    const int maxHealth = GetSkillValue(RPGEQUIP_DroneMaxHp);
    const int health = GetSkillValue(RPGEQUIP_DroneHp);
    if (maxHealth && health)
        return MultiplySigned32Wrap(zItem::GetBuyCost(), health)/maxHealth;
    return zItem::GetBuyCost();
}

bool zRPGEquip::IsBetterEx(zItem* _pItem)
{
    if (!Map)
        return false;
    if (!_pItem)
        (void)DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGEquip.cpp", 224u);
    const int selfType = static_cast<int>(GetType());
    const int otherType = static_cast<int>(_pItem->GetType());
    if ((selfType != 2 && selfType != 3) || (otherType != 2 && otherType != 3))
        return false;
    const int selfDroneIndex = 290+(selfType == 3?5:0);
    const int otherDroneIndex = 290+(otherType == 3?5:0);
    const int selfDamageIndex = 294+(selfType == 3?3:0);
    const int otherDamageIndex = 294+(otherType == 3?3:0);
    VID* const defaultVid = ::EmptyVid;
    const MAP* const map = Map;
    VID* selfDroneVid = map->StoredVid(selfDroneIndex);
    if (!selfDroneVid)
        selfDroneVid = defaultVid;
    VID* otherDroneVid = map->StoredVid(otherDroneIndex);
    if (!otherDroneVid)
        otherDroneVid = defaultVid;
    VID* selfDamageVid = map->StoredVid(selfDamageIndex);
    if (!selfDamageVid)
        selfDamageVid = defaultVid;
    VID* otherDamageVid = map->StoredVid(otherDamageIndex);
    if (!otherDamageVid)
        otherDamageVid = defaultVid;
    if (!selfDroneVid || !otherDroneVid || !selfDamageVid || !otherDamageVid ||
        selfDroneVid == defaultVid || otherDroneVid == defaultVid ||
        selfDamageVid == defaultVid || otherDamageVid == defaultVid)
        return false;
    int selfHealth = GetSkillValue(RPGEQUIP_DroneHp);
    int otherHealth = _pItem->GetSkillValue(RPGEQUIP_DroneHp);
    if (!selfHealth)
        selfHealth = selfDroneVid->maxHpByPlayer[0];
    if (!otherHealth)
        otherHealth = otherDroneVid->maxHpByPlayer[0];
    const int otherAverageDamage = SignedHalf(AddSigned32Wrap(
        otherDamageVid->deathDamageMax, otherDamageVid->deathDamageMin));
    const int selfAverageDamage = SignedHalf(AddSigned32Wrap(
        selfDamageVid->deathDamageMax, selfDamageVid->deathDamageMin));
    return MultiplySigned32Wrap(selfAverageDamage, selfHealth)>MultiplySigned32Wrap(otherAverageDamage, otherHealth);
}

void zRPGEquip::AddItem(iRPGObj* source)
{
    if (!m_bpInventory)
    {
        zInventory* inventory = static_cast<zInventory*>(CrtNew(sizeof(zInventory)));
        if (inventory)
        {
            try
            {
                inventory = new(inventory) zInventory(832);
            }
            catch (...)
            {
                CrtFreePointer(inventory);
                throw;
            }
        }
        m_bpInventory = inventory;
    }
    zItem* const item = static_cast<zItem*>(source->QueryInterface(49));
    (void)m_bpInventory->PutItem(item, nullptr);
}

