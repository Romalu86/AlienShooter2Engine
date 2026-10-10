#include "mylib.h"
#include <cstring>

#include "zRPGWeapon.h"
#include "basetype.h"


bool zRPGWeapon::GetParams(const char* token, int length, int& parameterCount, int* parameterIndices) const
{
    parameterCount = 1;
    parameterIndices[0] = -1;
    const unsigned int tokenLength = static_cast<unsigned int>(length);
    if (std::strncmp("Class", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_CLASSS;
    else if (std::strncmp("Level", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_LEVEL;
    else if (std::strncmp("Damage", token, tokenLength) == 0)
    {
        parameterIndices[0] = RPGWEAPON_DAMAGEMIN;
        parameterIndices[1] = RPGWEAPON_DAMAGEMAX;
        parameterCount = 2;
    }
    else if (std::strncmp("DamageRadius", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_DAMAGERADIUS;
    else if (std::strncmp("Clip", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_CLIP;
    else if (std::strncmp("Reload", token, tokenLength) == 0)
    {
        parameterIndices[0] = RPGWEAPON_RELOAD;
        parameterIndices[1] = RPGWEAPON_RELOADFULL;
        parameterCount = 2;
    }
    else if (std::strncmp("AimRange", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_AIMRANGE;
    else if (std::strncmp("TypeAnimationTors", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_TRUNKANIM;
    else if (std::strncmp("BulletCount", token, tokenLength) == 0)
    {
        parameterIndices[0] = RPGWEAPON_BULLETCOUNT;
        parameterIndices[1] = RPGWEAPON_AMMOUSE;
        parameterCount = 2;
    }
    else if (std::strncmp("VIDBullet", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_BULLETVID;
    else if (std::strncmp("ShotWAV", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_SHOTWAV;
    else if (std::strncmp("ShellVid", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_SHELLVID;
    else if (std::strncmp("FlameVid", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_FLAMEVID;
    else if (std::strncmp("DeathPush", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_DEATHPUSH;
    else if (std::strncmp("Explosive", token, tokenLength) == 0)
        parameterIndices[0] = RPGWEAPON_EXPLOSIVE;
    return parameterIndices[0] != -1;
}

bool zRPGWeapon::ParseUnknownCommand(const char* token, int valueCount,
                                     const char** values, iRPGFrameWork* framework)
{
    if (zRPGObj::ParseUnknownCommand(token, valueCount, values, framework))
        return true;
    if (std::strcmp(token, "AmmoName") != 0)
        return false;
    iRPGObj* candidate = framework->GetObjByNamee(values[0], true);
    if (candidate)
    {
        SetAmmo(static_cast<zRPGAmmo*>(candidate->QueryInterface(45)));
    }
    return true;
}


