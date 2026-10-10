#include "mylib.h"

#include "zm/RPGHero/zRPGAmmo.h"
#include "zm/Game/zGroundItem.h"
#include <string.h>

bool zRPGAmmo::m__abArrayParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGAmmo::m__abBaseParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGAmmo::m__abChangeableParams[MAX_RPGPARAMSCOUNT] = {};

zRPGAmmo::zRPGAmmo()
    : zRPGObj(static_cast<OBJ_TYPE>(TYPE_AMMO), RPGAMMO_PARAMS_COUNT), m_pEquipOwner(0), m_addAmmo(0)
{
}

zRPGAmmo::~zRPGAmmo()
{
}

int zRPGAmmo::iRelease()
{
    this->~zRPGAmmo();
    CrtFreePointer(this);
    return 0;
}

void* zRPGAmmo::QueryInterface(int interfaceId) const
{
    if (interfaceId == 4 || interfaceId == 45)
        return const_cast<zRPGAmmo*>(this);
    return zGroundItem::QueryInterface(interfaceId);
}

void zRPGAmmo::SetDataArrays()
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

bool zRPGAmmo::GetParams(const char* token, int length, int& count, int* indices) const
{
    count = 1;
    indices[0] = -1;
    if (strncmp("DeathMatchMax", token, static_cast<unsigned int>(length)) == 0)
        indices[0] = RPGAMMO_SURVIVEMAX;
    if (strncmp("DeathMatchGrnd", token, static_cast<unsigned int>(length)) == 0)
        indices[0] = RPGAMMO_DMGRND;
    return indices[0] != -1;
}

bool zRPGAmmo::ParseUnknownCommand(const char* key, int valueCount,
                                   const char** values, iRPGFrameWork* framework)
{
    if (zRPGObj::ParseUnknownCommand(key, valueCount, values, framework))
        return true;
    if (strcmp(key, "AddAmmo") != 0 || valueCount <= 0)
        return false;
    iRPGFrameWork* owner = GetFW();
    m_addAmmo = owner->GetObjByNamee(*values, true);
    return true;
}
