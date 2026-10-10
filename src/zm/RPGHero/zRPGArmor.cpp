#include "mylib.h"
#include <cstring>
#include "zRPGArmor.h"
#include "zm/Game/zGroundItem.h"
#include "zm/Inventory/zInventoryItem.h"
#include "basetype.h"

bool zRPGArmor::m__abArrayParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGArmor::m__abBaseParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGArmor::m__abChangeableParams[MAX_RPGPARAMSCOUNT] = {};
int zRPGArmor::m_iFullDmgCost = 0;


zRPGArmor::zRPGArmor() : zRPGObj(static_cast<OBJ_TYPE>(TYPE_ARMOR), RPGARMOR_PARAMS_COUNT)
{
}

zRPGArmor::~zRPGArmor()
{
}

void* zRPGArmor::QueryInterface(int interfaceId) const
{
    if (interfaceId == 7 || interfaceId == 51)
        return const_cast<zRPGArmor*>(this);
    return zGroundItem::QueryInterface(interfaceId);
}

int zRPGArmor::iRelease()
{
    this->~zRPGArmor();
    CrtFreePointer(this);
    return 0;
}

void zRPGArmor::SetDataArrays()
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

void zRPGArmor::InitParams()
{
    SetSkillValue(RPGARMOR_HP, RPG_BASE_HP);
    SetChangeableParam(RPGARMOR_DEFENCE);
    SetBaseParam(RPGARMOR_LEVEL);
    SetBaseParam(RPGARMOR_DEFENCE);
}

bool zRPGArmor::GetParams(const char* token, int tokenLength, int& valueCount, int* propertyId) const
{
    valueCount = 1;
    *propertyId = -1;
    const unsigned int comparisonLength = static_cast<unsigned int>(tokenLength);
    if (std::strncmp("Level", token, comparisonLength) == 0)
        *propertyId = RPGARMOR_LEVEL;
    else if (std::strncmp("Hp", token, comparisonLength) == 0)
        *propertyId = RPGARMOR_MAXHP;
    else if (std::strncmp("TypeAnimationArmor", token, comparisonLength) == 0)
        *propertyId = RPGARMOR_ANIMTYPE;
    else if (std::strncmp("Defence", token, comparisonLength) == 0)
        *propertyId = RPGARMOR_DEFENCE;
    return *propertyId != -1;
}

int zRPGArmor::Damage(int damage)
{
    int health = GetSkillValue(RPGARMOR_HP);
    const int MaxHp = GetSkillValue(RPGARMOR_MAXHP);
    if (MaxHp == 0)
        DebugAssert(3u, "MaxHp", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGArmor.cpp", 93u);
    health = AddSigned32Wrap(health, MultiplySigned32Wrap(-RPG_BASE_HP, damage)/MaxHp);
    if (health<0)
        health = 0;
    if (health>RPG_BASE_HP)
        health = RPG_BASE_HP;
    SetSkillValue(RPGARMOR_HP, health);
    return AddSigned32Wrap(MultiplySigned32Wrap(100, health), (RPG_BASE_HP-1))/RPG_BASE_HP;
}

int zRPGArmor::GetBuyCost() const
{
    int healthPercent = MultiplySigned32Wrap(GetSkillValue(RPGARMOR_HP), 100)/RPG_BASE_HP;
    if (healthPercent != 100)
    {
        const int minimumCostPercent = m_iFullDmgCost;
        healthPercent = AddSigned32Wrap(minimumCostPercent, MultiplySigned32Wrap(SubSigned32Wrap(100, minimumCostPercent), healthPercent)/100);
    }
    const int baseCost = zItem::GetBuyCost();
    return MultiplySigned32Wrap(baseCost, healthPercent)/100;
}

int zRPGArmor::GetRepairCost()
{
    const int damagedCost = GetBuyCost();
    const int fullCost = zItem::GetBuyCost();
    return SubSigned32Wrap(fullCost, damagedCost);
}

int zRPGArmor::Repair(int requestedCost)
{
    int acceptedCost = 0;
    int repairCost = GetRepairCost();
    int CurHp = RPG_BASE_HP;
    bool partialRepair = false;
    if (repairCost != 0)
    {
        if (requestedCost<repairCost)
        {
            acceptedCost = requestedCost;
            partialRepair = true;
        }
        else acceptedCost = repairCost;
        const int oldHealth = GetSkillValue(RPGARMOR_HP);
        CurHp = AddSigned32Wrap(oldHealth, MultiplySigned32Wrap(
            SubSigned32Wrap(RPG_BASE_HP, oldHealth), acceptedCost)/repairCost);
        if (partialRepair && CurHp>RPG_BASE_HP)
        {
            (void)DebugAssert(4u, "CurHp <= RPG_BASE_HP", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGArmor.cpp", 163u);
            CurHp = RPG_BASE_HP;
        }
        if (!partialRepair)
            CurHp = RPG_BASE_HP;
    }
    SetSkillValue(RPGARMOR_HP, CurHp);
    if (GetSkillValue(RPGARMOR_HP) != CurHp)
        (void)DebugAssert(5u, "GetSkillValue( RPGARMOR_HP ) == CurHp", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGArmor.cpp", 168u);
    return acceptedCost;
}

int zRPGArmor::GetSkillValue(int paramIndex) const
{
    if (paramIndex != RPGARMOR_HPCALCED)
        return zRPGObj::GetSkillValue(paramIndex);
    const int health = GetSkillValue(RPGARMOR_HP);
    const int maxHealth = GetSkillValue(RPGARMOR_MAXHP);
    return MultiplySigned32Wrap(health, maxHealth)/RPG_BASE_HP;
}

bool zRPGArmor::IsBetterEx(zItem* _pItem)
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGArmor.cpp", 198u);
    const int selfDefence = GetSkillValue(RPGARMOR_DEFENCE), otherDefence = _pItem->GetSkillValue(RPGARMOR_DEFENCE);
    const int selfHealth = GetSkillValue(RPGARMOR_HP), selfMaxHealth = GetSkillValue(RPGARMOR_MAXHP);
    const int otherHealth = _pItem->GetSkillValue(RPGARMOR_HP), otherMaxHealth = _pItem->GetSkillValue(RPGARMOR_MAXHP);
    const int selfHealthProduct = MultiplySigned32Wrap(selfHealth, selfMaxHealth);
    const int otherHealthProduct = MultiplySigned32Wrap(otherHealth, otherMaxHealth);
    const int otherProtectedHealth = MultiplySigned32Wrap(otherHealthProduct/RPG_BASE_HP, otherDefence)/100;
    const int selfProtectedHealth = MultiplySigned32Wrap(selfHealthProduct/RPG_BASE_HP, selfDefence)/100;
    return selfProtectedHealth>otherProtectedHealth;
}


