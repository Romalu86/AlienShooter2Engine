#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"
#include "mylib.h"
#include "zm/Game/zVidCopier.h"
#include "basetype.h"
#include "map.h"
#include "vid.h"
#include "region.h"
#include <new>

int zVidCopier::heroLegVids[2][4] = {
    {300, 300, 301, 304},
    {302, 302, 303, 304}
};

int zVidCopier::heroHeadVids[2][4] = {
    {-1, -1, 400, -1},
    {-1, -1, 400, -1}
};

int zVidCopier::heroBodyVids[2][40] = {
    {
        310, 311, 312, 313, 314, 315, 316, 317, 318, 319,
        320, 321, 322, 323, 324, 325, 326, 327, 328, 329,
        330, 331, 332, 333, 334, 335, 336, 337, 338, 339,
        340, 341, 342, 343, 344, 345, 346, 347, 348, 349
    },
    {
        350, 351, 352, 353, 354, 355, 356, 357, 358, 359,
        360, 361, 362, 363, 364, 365, 366, 367, 368, 369,
        370, 371, 372, 373, 374, 375, 376, 377, 378, 379,
        340, 341, 342, 343, 344, 345, 346, 347, 348, 349
    }
};

int zVidCopier::maxSpeedPercent = 0;



void* zVidCopier::QueryInterface(int interfaceId) const
{
    if (interfaceId == 35)return const_cast<zVidCopier*>(this);
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zVidCopier.cpp", 51u);
    return 0;
}


int zVidCopier::iRelease()
{
    if (this)
    {
        this->~zVidCopier();
        CrtFreePointer(this);
    }
    return 0;
}

zVidCopier::zVidCopier()
    : m_appliedHeroColor(-1), m_engineSpeedBase(0.0f), m_iArmy(-1), m_weaponState(-1)
{
    for (unsigned int i = 0;i<8u;++i)
    {
        void* const memory = CrtNew(sizeof(COLOR));
        m_heroColor[i] = memory ? new(memory) COLOR : 0;
    }
}


zVidCopier::~zVidCopier()
{

    for (unsigned int i = 0;i<8u;++i)
        CrtFreePointer(this->m_heroColor[i]);
}


bool zVidCopier::SetLegsVid(int tableSet, int tableIndex, int renderIndex, int speedMultiplier)
{
    int vidIndex = heroLegVids[tableSet != 0][tableIndex];
    if (Map->ValidateVid(vidIndex) == 0)
    {
        if (g_iZmDebug == 0u)::gpLogCons->LogStr(LT_WARN, "Vid %i for legs was not found", vidIndex);
        vidIndex = 303;
    }

    VID* const legs = GetDestVid(VC_LEGS);
    unsigned char changed = 0u;
    if (!legs->sourceVid || legs->sourceVid->nVid != vidIndex)
    {
        Map->CopyVid(legs, Map->Vid(vidIndex));
        this->m_appliedHeroColor = -1;
        this->m_engineSpeedBase = legs->maxSpeed;
        ::gpLogCons->LogStr(LT_INFO, "Legs vid %i was changed to %i", legs->nVid, vidIndex);
        changed = 1u;
    }

    if (legs)
    {
        legs->maxHp = renderIndex;
        const float speed = GetEngineSpeed(speedMultiplier);
        if (!FloatEqualOrUnordered(speed, legs->maxSpeed))
        {
            legs->maxSpeed = speed;
            const int speedPercent = TruncateFloatToInt32(speed*(1e+03f));
            ::gpLogCons->LogStr(LT_INFO, "MaxSpeed %i was set to hero", speedPercent);
        }
    }
    return changed;
}


bool zVidCopier::SetBodyVid(int tableSet, int bodyIndex, int variantIndex)
{
    const int tableOffset = AddSigned32Wrap(bodyIndex, MultiplySigned32Wrap(10, variantIndex));
    int vidIndex = heroBodyVids[tableSet != 0][tableOffset];
    if (Map->ValidateVid(vidIndex) == 0)
    {
        if (g_iZmDebug == 0u)::gpLogCons->LogStr(LT_WARN, "Vid %i for body was not found", vidIndex);
        vidIndex = 351;
    }

    VID* const body = GetDestVid(VC_BODY);
    if (!body->sourceVid || body->sourceVid->nVid != vidIndex)
    {
        Map->CopyVid(body, Map->Vid(vidIndex));
        this->m_appliedHeroColor = -1;
        ::gpLogCons->LogStr(LT_INFO, "Body vid %i was changed to %i", body->nVid, vidIndex);
    }

    VID* const legs = GetDestVid(VC_LEGS);
    legs->linkVid = body;
    const int bodyVidNum = GetDestVidNum(VC_BODY);
    GetDestVid(VC_LEGS)->nLinkVid = bodyVidNum;
    return 1u;
}


bool zVidCopier::SetHeadVid(int tableSet, int fallbackVid, int headMode)
{
    int vidIndex;
    if (headMode == 3)vidIndex = 309;
    else if (headMode == 2)
    {
        vidIndex = heroHeadVids[tableSet != 0][2];
        if (vidIndex <= 0)vidIndex = fallbackVid;
    }
    else vidIndex = fallbackVid;

    if (Map->ValidateVid(vidIndex) == 0)
    {
        if (g_iZmDebug == 0u)::gpLogCons->LogStr(LT_WARN, "Vid %i for head was not found", vidIndex);
        vidIndex = 380;
    }

    VID* const head = GetDestVid(VC_HEAD);
    if (!head->sourceVid || head->sourceVid->nVid != vidIndex)
    {
        Map->CopyVid(head, Map->Vid(vidIndex));
        this->m_appliedHeroColor = -1;
        ::gpLogCons->LogStr(LT_INFO, "Head vid %i was changed to %i", head->nVid, vidIndex);
    }

    VID* const body = GetDestVid(VC_BODY);
    body->linkVid = head;
    const int headVidNum = GetDestVidNum(VC_HEAD);
    GetDestVid(VC_BODY)->nLinkVid = headVidNum;
    return 1u;
}


bool zVidCopier::SetWeaponProperties(const iRPGObj* rpgObject)
{

    VID* const normalDest = GetDestVid(VC_BODY);
    VID* const auxiliaryDest = GetDestVid(VC_WEAPON);

    if (!rpgObject)
    {
        if (normalDest->noChild[8] != 0 || normalDest->sfx[8] != 0)
        {
            normalDest->noChild[8] = 0;
            normalDest->sfx[8] = 0;
            return 1u;
        }
        return 0u;
    }

    const iRPGObj* const weapon = rpgObject;
    int shooterVidIndex = weapon->GetSkillValue(RPGWEAPON_BULLETVID);
    int auxiliaryVidIndex = weapon->GetSkillValue(RPGWEAPON_FLAMEVID);

    if (shooterVidIndex == 0)
    {
        const char* const weaponName = weapon->GetName();
        ::gpLogCons->LogStr(
            LT_INPTERR, "No bullet defined in cfg-file for weapon %s", weaponName);
    }

    bool specialShooterPrepared = false;
    if (shooterVidIndex == 1386)
    {
        VID* const specialVid = Map->Vid(1407);
        if (specialVid)
        {
            specialVid->deathDamageMin = weapon->GetSkillValue(RPGWEAPON_DAMAGEMIN);
            specialVid->deathDamageMax = weapon->GetSkillValue(RPGWEAPON_DAMAGEMAX);
            const int damageRadius = weapon->GetSkillValue(RPGWEAPON_DAMAGERADIUS);
            specialVid->deathRange = static_cast<float>(damageRadius);
            specialShooterPrepared = true;
        }
    }

    if (shooterVidIndex != 0 && Map->ValidateVid(shooterVidIndex) == 0)
        shooterVidIndex = 112;

    if (auxiliaryVidIndex != 0)
    {
        if (Map->ValidateVid(auxiliaryVidIndex) == 0)
            auxiliaryVidIndex = 104;
        Map->CopyVid(auxiliaryDest, Map->Vid(auxiliaryVidIndex));
    }

    WEAPON_RUNTIME* const normalSprite = normalDest->weapon;
    normalSprite->clipSize = weapon->GetSkillValue(RPGWEAPON_CLIP);
    normalSprite->reloadTime = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_RELOAD));
    normalSprite->fullReloadTime = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_RELOADFULL));
    const float aimRange = static_cast<float>(weapon->GetSkillValue(RPGWEAPON_AIMRANGE));
    normalSprite->aimRange = aimRange;

    const int attachedVidIndex = weapon->GetSkillValue(RPGWEAPON_SHELLVID);
    const int bulletCount = weapon->GetSkillValue(RPGWEAPON_BULLETCOUNT);
    const int shotWav = weapon->GetSkillValue(RPGWEAPON_SHOTWAV);

    VID* ShooterVid = normalDest;
    if (auxiliaryVidIndex != 0)
    {


        normalDest->noChild[8] = 1;
        normalDest->childVid[8] = auxiliaryDest;
        normalDest->sfx[8] = 0;

        auxiliaryDest->runtimeFlags |= 0x00001000u;
        auxiliaryDest->noChild[14] = attachedVidIndex != 0?1:0;
        auxiliaryDest->childVid[14] = attachedVidIndex != 0?Map->Vid(attachedVidIndex):0;

        WEAPON_RUNTIME* const auxiliarySprite = auxiliaryDest->weapon;
        auxiliarySprite->aimRange = aimRange;
        ShooterVid = auxiliaryDest;

        normalDest->deathDamageMin = 0;
        normalDest->deathDamageMax = 0;
        normalDest->deathRange = 0.0f;
        normalDest->deathPush = 0.0f;
    }

    if (!ShooterVid)
        DebugAssert(5u, "ShooterVid", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zVidCopier.cpp", 333u);

    VID* const shooterVid = Map->Vid(shooterVidIndex);
    ShooterVid->noChild[8] = bulletCount;
    ShooterVid->childVid[8] = shooterVid;
    ShooterVid->sfx[8] = shotWav;
    if (shooterVid)
        shooterVid->runtimeFlags |= 0x00001000u;

    if (!specialShooterPrepared)
    {
        ShooterVid->deathDamageMin = weapon->GetSkillValue(RPGWEAPON_DAMAGEMIN);
        ShooterVid->deathDamageMax = MultiplySigned32Wrap(-1, weapon->GetSkillValue(RPGWEAPON_DAMAGEMAX));
        const int damageRadius = weapon->GetSkillValue(RPGWEAPON_DAMAGERADIUS);
        ShooterVid->deathRange = static_cast<float>(damageRadius);
        const int scale = weapon->GetSkillValue(RPGWEAPON_DEATHPUSH);
        ShooterVid->deathPush = static_cast<float>(static_cast<double>(scale)*
                                   (0.02f));
    }

    iLogConsole* const log = ::gpLogCons;
    (void)log->LogStr(LT_INFO, " ShooterVid = %d, deathDamageMax = %d ", ShooterVid->nVid, ShooterVid->deathDamageMax);
    return 1u;
}


int zVidCopier::ReadColors()
{
    int lastColor = 0;
    for (int hero = 0;hero<8;++hero)
    {
        int components[3];
        for (int component = 0;component<3;++component)
        {
            const STRING functionName("GetHeroColor");
            components[component] = Map->ScriptRun(functionName, "%d%d%d", component, 0, hero);
        }
        const unsigned int r = ClampByteInt(components[0]);
        const unsigned int g = ClampByteInt(components[1]);
        const unsigned int b = ClampByteInt(components[2]);
        lastColor = static_cast<int>(0xFF000000u|(r<<16)|(g<<8)|b);
        m_heroColor[hero]->value = static_cast<unsigned int>(lastColor);
    }
    return lastColor;
}


int zVidCopier::PrevHeroColor()
{
    if (GetHeroColor() == 0)
        (void)SetHeroColor(7);
    else
        (void)SetHeroColor(AddSigned32Wrap(GetHeroColor(), -1));
    return GetHeroColor();
}


int zVidCopier::GetHeroColor()
{
    iRPGObj* const colorObject = m_refBody.m_object;
    return colorObject?colorObject->GetSkillValue(RPGHERO_COLOR):0;
}


int zVidCopier::NextHeroColor()
{
    if (GetHeroColor() == 7)
        (void)SetHeroColor(0);
    else
        (void)SetHeroColor(AddSigned32Wrap(GetHeroColor(), 1));
    return GetHeroColor();
}


bool zVidCopier::ApplyHeroColor()
{
    const int current = GetHeroColor();
    if (m_appliedHeroColor == current || m_iArmy == -1)return 0u;

    const int colorIndex = GetHeroColor();
    const COLOR color = *m_heroColor[colorIndex];
    VID* const legs = GetDestVid(VC_LEGS);
    legs->SetReColorForArmy(color);
    const int bodyColorIndex = GetHeroColor();
    const COLOR bodyColor = *m_heroColor[bodyColorIndex];
    VID* const body = GetDestVid(VC_BODY);
    body->SetReColorForArmy(bodyColor);
    m_appliedHeroColor = GetHeroColor();
    return 1u;
}


bool zVidCopier::SetHeroColor(int colorIndex)
{
    if (colorIndex >= 8)colorIndex = 0;
    else if (colorIndex<0)colorIndex = 7;
    iRPGObj* const colorObject = m_refBody.m_object;
    if (colorObject)(void)colorObject->SetSkillValue(RPGHERO_COLOR, colorIndex);
    return ApplyHeroColor();
}


float zVidCopier::GetEngineSpeed(int multiplier)
{
    const float engineSpeedBase = m_engineSpeedBase;
    if (maxSpeedPercent == 0)
    {
        const STRING functionName("GetMaxSpeedPercent");
        maxSpeedPercent = Map->ScriptRun(functionName, "", 0, 0, 0);
    }
    const float factor = 5e-05f;
    const float bias = 1.0f;

    const double scaled = static_cast<double>(maxSpeedPercent)*
                        static_cast<double>(multiplier);
    return static_cast<float>((scaled*factor+bias)*engineSpeedBase);
}


int zVidCopier::GetDestVidNum(VC_VID_TYPE vidType)
{
    int base = 0;
    switch (vidType)
    {
    case VC_LEGS:
        base = 250;
    break;
    case VC_BODY:
        base = 251;
    break;
    case VC_HEAD:
        base = 252;
    break;
    case VC_WEAPON:
        base = 254;
    break;
    default:
        break;
    }
    if (m_iArmy<0)DebugAssert(5u, "m_iArmy >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zVidCopier.cpp", 479u);
    return AddSigned32Wrap(MultiplySigned32Wrap(m_iArmy, 5), base);
}


VID* zVidCopier::GetDestVid(VC_VID_TYPE vidType)
{
    return Map->Vid(GetDestVidNum(vidType));
}


unsigned int __cdecl GameZvidcopier_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


