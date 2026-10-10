#include "mylib.h"
#include "zRPGWeapon.h"
#include "zm/Game/zGroundItem.h"

#include "basetype.h"

bool zRPGWeapon::m__abArrayParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGWeapon::m__abBaseParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGWeapon::m__abChangeableParams[MAX_RPGPARAMSCOUNT] = {};


void* zRPGWeapon::QueryInterface(int interfaceId) const
{
    if (interfaceId == 6 || interfaceId == 50)
        return const_cast<zRPGWeapon*>(this);
    return zGroundItem::QueryInterface(interfaceId);
}

zRPGWeapon::zRPGWeapon()
    : zRPGObj(static_cast<OBJ_TYPE>(TYPE_WEAPON), RPGWEAPON_PARAMS_COUNT), m_pAmmo(0), m_bIncAmmoOnPut(true)
{
}

zRPGWeapon::~zRPGWeapon()
{
}

int zRPGWeapon::iRelease()
{
    this->~zRPGWeapon();
    CrtFreePointer(this);
    return 0;
}

void zRPGWeapon::SetDataArrays()
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

void zRPGWeapon::InitParams()
{
    SetChangeableParam(RPGWEAPON_DAMAGEMIN);
    SetChangeableParam(RPGWEAPON_DAMAGEMAX);
    SetChangeableParam(RPGWEAPON_DAMAGERADIUS);
    SetChangeableParam(RPGWEAPON_RELOAD);
    SetChangeableParam(RPGWEAPON_RELOADFULL);
    SetChangeableParam(RPGWEAPON_AIMRANGE);
    SetBaseParam(RPGWEAPON_CLASSS);
    SetBaseParam(RPGWEAPON_DAMAGEMAX);
}

void zRPGWeapon::SetAmmo(zRPGAmmo* _pAmmo)
{
    if (!_pAmmo)
        DebugAssert(4u, "_pAmmo", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGWeapon.cpp", 63u);
    if (m_pAmmo)
        DebugAssert(4u, "m_pAmmo == 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGWeapon.cpp", 65u);
    m_pAmmo = _pAmmo;
}

zRPGAmmo* zRPGWeapon::GetAmmo()
{
    return m_pAmmo;
}

bool zRPGWeapon::IfIncAmmoOnPut()
{
    const bool incrementAmmo = m_bIncAmmoOnPut;
    m_bIncAmmoOnPut = false;
    return incrementAmmo;
}


void zRPGWeapon::CopyParametersFrom(iRPGObj* source)
{
    zRPGObj::CopyParametersFrom(source);
    zRPGWeapon* const sourceWeapon =
        static_cast<zRPGWeapon*>(source->QueryInterface(50));
    zRPGAmmo* const ammo = sourceWeapon->GetAmmo();
    if (ammo)
        SetAmmo(ammo);
}

bool zRPGWeapon::SetSkillValue(int skill, int value)
{
    const bool result = zRPGObj::SetSkillValue(skill, value);
    const int objectType = GetAddMoney();
    if (objectType == 16 && skill == RPGWEAPON_CLASSS)
    {
        if (static_cast<unsigned int>(value) <= 5u)
            m_gameObjectExtType = value;
        else
        {
            (void)DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGWeapon.cpp", 124u);
            m_gameObjectExtType = objectType;
        }
    }
    return result;
}


