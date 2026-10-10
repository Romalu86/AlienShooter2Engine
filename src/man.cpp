#include "mylib.h"
#include "engine_globals.h"
#include "man.h"
#include "unit.h"
#include "sprite.h"
#include "vid.h"
#include "weapon_runtime.h"
#include "sprite_collector.h"
#include "zm/RPGHero/zRPGWeapon.h"
#include "zm/RPGHero/zRPGAmmo.h"
#include "zm/RPGHero/zRPGHero.h"
#include "base_graph.h"
#include "zm/Game/Vehicles/zCarControl.h"
#include "zm/Utils/zRefObject.h"
#include "zm/Utils/zStaticStorage.h"
#include "region.h"
#include <new>

StaticObjectStorage<COLOR> manBlackColorStorage;
StaticObjectStorage<COLOR> manMagentaColorStorage;
StaticObjectStorage<COLOR> manYellowColorStorage;
StaticObjectStorage<COLOR> manRedColorStorage;
StaticObjectStorage<COLOR> manGreenColorStorage;
StaticObjectStorage<COLOR> manBlueColorStorage;
StaticObjectStorage<COLOR> manSecondaryYellowColorStorage;
StaticObjectStorage<COLOR> manCyanColorStorage;

extern "C" void*  ManScalarDeletingDestructor(void* self, unsigned int flags);


float VECTOR::NearLength() const
{
    const float diagonalScale = (0.398000002f);
    const float absoluteX = AbsFloat(x);
    const float absoluteY = AbsFloat(y);
    const float absoluteZ = AbsFloat(z);
    const float horizontalLength = (absoluteX>absoluteY)?(absoluteX+absoluteY*diagonalScale):(absoluteY+absoluteX*diagonalScale);
    return (horizontalLength>absoluteZ)?(horizontalLength+absoluteZ*diagonalScale):(absoluteZ+horizontalLength*diagonalScale);
}


#include "graph.h"
#include "map.h"

extern "C" void*  ManScalarDeletingDestructor(void* self, unsigned int flags)
{
    static_cast<MAN*>(self)->~MAN();
    if (flags&1u) CrtFreePointer(self);
    return self;
}

int MAN::iRelease()
{
    (void) ManScalarDeletingDestructor(this, 1u);
    return 0;
}

void MAN::DeletePointerToSprite(SPRITE* sprite)
{
    SPRITE::DeletePointerToSprite(sprite);
    if (m_pHeadTarget == sprite)
    {
        if (m_pHeadTarget) m_pHeadTarget->Release();
        m_pHeadTarget = 0;
    }
}

bool MAN::SetHpFromHero(int hp)
{
    if (m_suppressHpMirror == 0)
        static_cast<SPRITE*>(this)->ChangeHp(hp);
    return false;
}


void* MAN::QueryInterface(int interfaceId) const
{
    MAN* self = const_cast<MAN*>(this);
    const unsigned int id = static_cast<unsigned int>(interfaceId);
    if (id == 42u)
        return static_cast<iMan*>(self);
    return self->zRefObject::QueryInterface(static_cast<int>(id));
}


bool MAN::IsUnitSeenByDistance(SPRITE* _pUnit, float* outDistance)
{
    if (!_pUnit) DebugAssert(4u, "_pUnit", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\man.cpp", 509u);
    const VECTOR& selfPosition = Coor();
    const VECTOR& targetPosition = _pUnit->Coor();
    float absoluteDeltaX = targetPosition.x-selfPosition.x;
    float absoluteDeltaY = targetPosition.y-selfPosition.y;
    if (absoluteDeltaX<0) absoluteDeltaX = -absoluteDeltaX;
    if (absoluteDeltaY<0) absoluteDeltaY = -absoluteDeltaY;
    const float diagonalScale = (0.398000002f);
    const float distance = (absoluteDeltaX>absoluteDeltaY)?(absoluteDeltaX+absoluteDeltaY*diagonalScale):(absoluteDeltaY+absoluteDeltaX*diagonalScale);
    const bool withinSightRange = !(distance>(400.0f));
    if (outDistance) *outDistance = distance;
    return withinSightRange;
}

MAN::ANGLE_SEEN_TYPE MAN::IsUnitSeenByAngle(SPRITE* _pUnit)
{
    if (!_pUnit) DebugAssert(4u, "_pUnit", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\man.cpp", 524u);
    SPRITE* weaponSprite = Link();
    unsigned char angleDifference = 0;
    if (weaponSprite)
    {
        const VECTOR& unitPosition = _pUnit->Coor();
        const VECTOR& posePosition = weaponSprite->Coor();
        const VECTOR2 vectorToUnit(unitPosition.x-posePosition.x, unitPosition.y-posePosition.y);
        angleDifference = ANGLE(vectorToUnit).value;
        unsigned char facingDirection = weaponSprite->Direction().value;
        unsigned char forwardDifference = static_cast<unsigned char>(facingDirection-angleDifference);
        unsigned char backwardDifference = static_cast<unsigned char>(angleDifference-facingDirection);
        angleDifference = (forwardDifference<backwardDifference)?forwardDifference:backwardDifference;
    }
    if (angleDifference>60u) return ANGLE_SEEN_BACK;
    if (angleDifference>50u) return ANGLE_SEEN_SIDE;
    return ANGLE_SEEN_FRONT;
}


void MAN::CreateEff(int effectId)
{
    const VECTOR& position = Coor();
    const VECTOR effectPosition(position);
    MAP* runtimeMap = Map;
    void* const* const mapVtable = *reinterpret_cast<void* const* const*>(runtimeMap);
    const ANGLE effectDirection(static_cast<unsigned char>(0u));
    runtimeMap = Map;
    VID* const effectVid = runtimeMap->Vid(effectId);
    runtimeMap = Map;
    typedef SPRITE* (__thiscall *CreateSpriteCall)(
        MAP*, VID*, const VECTOR&, ANGLE, SPRITE*);
    const CreateSpriteCall createEffectSprite = reinterpret_cast<CreateSpriteCall>(mapVtable[8]);
    (void) createEffectSprite(runtimeMap, effectVid, effectPosition, effectDirection, this);
}


MAN::MAN(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : UNIT(vid, position, direction, parent),
      m_pRPGHero(0), m_pPlayer(0), m_pHeadTarget(0),
      m_tankCanShoot(1), m_suppressHpMirror(0), m_pRPGFrameWork(0),
      m_reloadInProgress(0), m_returnHeadToBody(0), m_lastHeadTargetSearchTick(0)
{
}

MAN::~MAN()
{
    if (m_pHeadTarget)
        ReportLiveSpriteReference(m_pHeadTarget);
}

static bool ManUsesRpgWeaponControl(const MAN* actor)
{
    VID* const actorVid = const_cast<MAN*>(actor)->Vid();
    if (actorVid->spriteClass != 7u) return false;
    VID* const linkedVid = actorVid->linkVid;
    if (!linkedVid) return false;
    WEAPON_RUNTIME* const weapon = linkedVid->weapon;
    return (weapon->Property&0x800u) != 0u;
}

static SPRITE* ManCurrentPose(MAN* actor)
{
    return actor->Link();
}


void MAN::SetRPGHero(iRPGHero* hero)
{
    if (m_pRPGHero != hero)
    {
        void* heroInterface = hero ? hero->QueryInterface(52) : 0;
        m_pRPGHero = static_cast<iRPGHero*>(heroInterface);
    }

    void* const currentHero = m_pRPGHero;
    if (currentHero)
    {
        iMan* manInterface = 0;
        VID* const actorVid = m_vid;
        if (actorVid->spriteClass == 7u)
        {
            VID* const linkedVid = actorVid->linkVid;
            if (linkedVid)
            {
                WEAPON_RUNTIME* const weapon = linkedVid->weapon;
                if ((weapon->Property&0x800u) != 0u)
                    manInterface = static_cast<iMan*>(this);
            }
        }
        static_cast<zRPGHero*>(currentHero)->SetMan(manInterface);
        RecalcCurAmmoClip(true);
    }
}


bool MAN::WeaponNotNeedReload()
{
    VID* const actorVid = m_vid;
    const unsigned int actorVidId = static_cast<unsigned int>(actorVid->nVid);
    if (actorVidId == 3000u) return true;
    if (actorVidId == 1200u) return false;

    zRPGHero* const hero = static_cast<zRPGHero*>(m_pRPGHero);
    if (!hero) return false;
    zRPGWeapon* const weapon = hero->GetCurWeapon();
    if (!weapon) return false;

    iRPGObj* const weaponObjectInterface = static_cast<iRPGObj*>(weapon);
    if (weaponObjectInterface == 0) return false;
    if (weaponObjectInterface->GetSkillValue(RPGWEAPON_RELOADFULL) == 0u)
        return true;

    zRPGWeapon* const queriedWeapon = static_cast<zRPGWeapon*>(weaponObjectInterface->QueryInterface(50));
    zRPGAmmo* const ammo = queriedWeapon->GetAmmo();
    if (!ammo) return false;
    return static_cast<iRPGObj*>(ammo)->IsInfinite();
}

unsigned int MAN::RecalcCurAmmoClipResult(bool forceComplete)
{
    int reloadPercent = 0;
    if (WeaponNotNeedReload())
        reloadPercent = 100;
    else
    {
        const int ammoCount = GetAmmoCnt();
        if (ammoCount>0 && m_pRPGHero)
        {
            if (forceComplete)
                reloadPercent = 100;
            else
            {
                SPRITE* weaponSprite = ManCurrentPose(this);
                if (!weaponSprite)
                    reloadPercent = 100;
                else
                {
                    weaponSprite = ManCurrentPose(this);
                    EX_SPRITE_DATA* weaponData = weaponSprite->ExtraData();
                    if (!weaponData)
                        reloadPercent = 100;
                    else
                    {
                        weaponSprite = ManCurrentPose(this);
                        VID* weaponVid = weaponSprite->Vid();
                        WEAPON_RUNTIME* weapon = weaponVid->weapon;
                        const int clipCapacity = weapon->clipSize;
                        const int loadedAmmo = static_cast<int>(weaponData->tacticalCounter);

                        bool reloadStarted = false;
                        if (loadedAmmo == 0)
                        {
                            weaponSprite = ManCurrentPose(this);
                            reloadStarted = weaponSprite->StateTimer()>5000u ||
                            (weaponSprite->Animation() == 8 &&
                                weaponSprite->CurrentFrame() <= weaponSprite->CurrentFrameEnd());
                        }
                        if (reloadStarted)
                        {
                            reloadPercent = 0;
                        }
                        else if (clipCapacity != 0)
                        {
                            int reloadableAmmo = BitsToSigned32(static_cast<unsigned int>(clipCapacity)-
                                static_cast<unsigned int>(loadedAmmo));
                            if (reloadableAmmo>ammoCount) reloadableAmmo = ammoCount;
                            reloadPercent = BitsToSigned32(static_cast<unsigned int>(reloadableAmmo)*100u)/clipCapacity;
                        }
                    }
                }
            }
        }
    }

    const int ammoBarDirection = BitsToSigned32(static_cast<unsigned int>(reloadPercent)*100000u)/100;
    m_pPlayer->SetCurAmmoDir(ammoBarDirection);
    return static_cast<unsigned int>(ammoBarDirection);
}


void MAN::RecalcCurAmmoClip(bool forceComplete)
{
    (void) RecalcCurAmmoClipResult(forceComplete);
}


void MAN::ReloadAmmo()
{
    SPRITE* weaponSprite = ManCurrentPose(this);
    if (!weaponSprite) return;
    EX_SPRITE_DATA* weaponData = weaponSprite->ExtraData();
    if (!weaponData || static_cast<int>(weaponData->tacticalCounter) <= 0) return;

    weaponData->tacticalCounter = 0u;
    VID* weaponVid = weaponSprite->Vid();
    WEAPON_RUNTIME* weapon = weaponVid->weapon;
    const unsigned int reloadDuration = weapon->fullReloadTime;
    weaponSprite->SetStateTimer(reloadDuration+5000u);
}


int MAN::GetReloadBarDir()
{
    int reloadPercent = 100;
    if (WeaponNotNeedReload())
        return reloadPercent;

    SPRITE* weaponSprite = ManCurrentPose(this);
    if (weaponSprite)
    {
        EX_SPRITE_DATA* weaponData = weaponSprite->ExtraData();
        if (weaponData)
        {
            const bool reloadWindow = (weaponData->tacticalCounter == 0u) &&
            (weaponSprite->StateTimer()>5000u ||
                (weaponSprite->Animation() == 8 &&
                weaponSprite->CurrentFrame() <= weaponSprite->CurrentFrameEnd()));

            if (reloadWindow)
            {
                VID* spriteVid = weaponSprite->Vid();
                WEAPON_RUNTIME* weapon = spriteVid->weapon;
                const unsigned int reloadDuration = weapon?weapon->fullReloadTime:0u;
                if (reloadDuration != 0u)
                {
                    reloadPercent = static_cast<int>(((reloadDuration+5000u-weaponSprite->StateTimer())*100u)/reloadDuration);
                    if (m_pRPGHero)
                    {
                        iPlayer* playerInterface = m_pPlayer;
                        playerInterface->OnClipReloading();
                    }
                    m_reloadInProgress = 1u;
                }
            }
            else if (m_reloadInProgress != 0u)
            {
                VID* spriteVid = m_vid;
                const unsigned int spriteVidId = static_cast<unsigned int>(spriteVid->nVid);
                if (m_pRPGHero && spriteVidId != 1206u && spriteVidId != 3000u)
                {
                    iPlayer* playerInterface = m_pPlayer;
                    playerInterface->OnClipReload();
                }
                m_reloadInProgress = 0u;
            }
        }
    }
    return reloadPercent*1000;
}


SPRITE* MAN::SeekAnyUnit()
{
    SPRITE_COLLECTOR* spriteCollector = ::Sprites;

    int unitIterator = 0;
    for (SPRITE* candidate = spriteCollector->FirstUnit(&unitIterator, -1); candidate; candidate = spriteCollector->NextUnit(&unitIterator))
    {
        if (candidate == static_cast<SPRITE*>(this)) continue;
        float candidateDistance = 0.0f;
        if (IsUnitSeenByDistance(candidate, &candidateDistance) &&
            candidateDistance<(999999.0f) &&
            IsUnitSeenByAngle(candidate) == ANGLE_SEEN_FRONT)
            return candidate;
    }
    return 0;
}


void MAN::RotateHeadTact()
{
    SPRITE* weaponSprite = ManCurrentPose(this);
    SPRITE* headSprite = weaponSprite?weaponSprite->ChildChain():0;
    if (!headSprite) return;

    bool usesRpgWeapon = false;
    if (weaponSprite && headSprite)
        usesRpgWeapon = ManUsesRpgWeaponControl(this);
    SPRITE* headTarget = m_pHeadTarget;

    if (weaponSprite && headSprite && usesRpgWeapon)
    {
        const unsigned int lastSearchTick = m_lastHeadTargetSearchTick;
        if (!headTarget && (lastSearchTick == 0u || ::CurrentTime-lastSearchTick>1000u))
        {
            SPRITE* nextTarget = SeekAnyUnit();
            if (nextTarget) ++nextTarget->m_referenceCount;
            headTarget = m_pHeadTarget;
            if (headTarget) headTarget->Release();
            headTarget = nextTarget;
            m_pHeadTarget = headTarget;
            m_lastHeadTargetSearchTick = ::CurrentTime;
        }

        if (weaponSprite->StateTimer()>5000u || weaponSprite->Animation() == 8)
        {
            headTarget = m_pHeadTarget;
            if (headTarget) headTarget->Release();
            headTarget = 0;
            m_pHeadTarget = 0;
            m_returnHeadToBody = 0u;
        }

        headTarget = m_pHeadTarget;
        if (headTarget)
        {
            bool keepTarget = false;
            bool returnToBody = true;
            if (IsUnitSeenByDistance(headTarget, 0))
            {
                const ANGLE_SEEN_TYPE seen = IsUnitSeenByAngle(headTarget);
                keepTarget = seen == ANGLE_SEEN_FRONT;
                returnToBody = seen != ANGLE_SEEN_BACK;
            }
            if (!keepTarget)
            {
                headTarget->Release();
                headTarget = 0;
                m_pHeadTarget = 0;
                m_returnHeadToBody = returnToBody?1u:0u;
            }
        }

        headTarget = m_pHeadTarget;
        if (headTarget)
        {
            const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
            unsigned char desiredDirection = headSprite->DirectionTo(headTarget).value;
            (void) headSprite->RotateTact(ANGLE(desiredDirection), static_cast<int>(elapsed));
        }
    }

    if (!weaponSprite || !headSprite || m_pHeadTarget ||
        !ManUsesRpgWeaponControl(this))
        return;

    if (m_returnHeadToBody != 0u)
    {
        const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
        const ANGLE remainingTurn = headSprite->RotateTact(weaponSprite->Direction(), static_cast<int>(elapsed));
        if (remainingTurn.value == 0u)
            m_returnHeadToBody = 0u;
    }
    else
    {
        headSprite->ChangeDirection(weaponSprite->Direction());
        m_returnHeadToBody = 0u;
    }
}


void MAN::SetPlayer(iPlayer* player, iRPGFrameWork* framework)
{
    m_pPlayer = player;
    m_pRPGFrameWork = framework;
}


unsigned int __cdecl SrcMan_StaticCleanupRegister02()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}
#include "basetype.h"


extern unsigned int*  GraphPackOpaqueRgb(unsigned int* outColor, int red, int green, int blue);


unsigned int __cdecl ManInitGlobalColorPalette()
{
    (new(manBlackColorStorage.Address()) COLOR)->value = 0xFF000000u;
    (new(manMagentaColorStorage.Address()) COLOR)->value = 0xFFFF00FFu;
    (new(manYellowColorStorage.Address()) COLOR)->value = 0xFFFFFF00u;
    (new(manRedColorStorage.Address()) COLOR)->value = 0xFFFF0000u;
    (new(manGreenColorStorage.Address()) COLOR)->value = 0xFF00FF00u;
    (new(manBlueColorStorage.Address()) COLOR)->value = 0xFF0000FFu;
    (new(manSecondaryYellowColorStorage.Address()) COLOR)->value = 0xFFFFFF00u;
    COLOR* const cyan = new(manCyanColorStorage.Address()) COLOR;
    GraphPackOpaqueRgb(&cyan->value, 0, 255, 255);
    return 0u;
}


unsigned int __cdecl ManStaticInitGlobalColorPalette()
{
    return ManInitGlobalColorPalette();
}

int MAN::GetAmmoCnt()
{
    void* hero = m_pRPGHero;
    if (!hero) return 0u;
    VID* actorVid = m_vid;
    if (actorVid->spriteClass == 7u)
    {
        VID* linkedVid = actorVid->linkVid;
        if (linkedVid)
        {
            WEAPON_RUNTIME* weapon = linkedVid->weapon;
            if ((weapon->Property&0x800u) != 0u) return static_cast<int>(static_cast<zRPGHero*>(hero)->GetCurAmmoCnt(1u));
        }
    }
    return m_tankCanShoot?999999:0;
}

int MAN::Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter)
{
    const unsigned int actionBits = static_cast<unsigned int>(actionCode);

    const unsigned int dispatchAction = actionBits;
    const unsigned int firstParameterBits = static_cast<unsigned int>(firstParameter);
    const unsigned int secondParameterBits = static_cast<unsigned int>(secondParameter);
    const unsigned int thirdParameterBits = static_cast<unsigned int>(thirdParameter);

    switch (dispatchAction)
    {
    case ACT_COOR_ATTACK:
        {
            DIALOG_UNIT* const dialog = static_cast<DIALOG_UNIT*>(this);
            SPRITE* weaponSprite = dialog->Link();
            if (!weaponSprite) return 0;

            weaponSprite = dialog->Link();
            if (weaponSprite->Animation() >= 15) return 0;

            weaponSprite = dialog->Link();
            VID* weaponVid = weaponSprite->Vid();
            if (!static_cast<unsigned int>(weaponVid->nWeapon) || !weaponVid->childVid[8])
                return 0;

            weaponSprite = dialog->Link();
            if ((weaponSprite->RuntimeFlags()&0x7Cu) != 0u) return 0;

            weaponSprite = dialog->Link();
            if (weaponSprite->StateTimer() >= 5000u) return 0;

            float targetZ = static_cast<float>(thirdParameter);
            const float currentZ = m_xyz.z;
            if (!(static_cast<double>(currentZ)+80.0 >= static_cast<double>(targetZ)))
                targetZ = static_cast<float>(static_cast<double>(currentZ)+60.0);

            const double destinationY = static_cast<double>(secondParameter)+static_cast<double>(targetZ);

            VECTOR targetDelta;
            targetDelta.x = static_cast<float>(static_cast<double>(firstParameter)-static_cast<double>(m_xyz.x));
            targetDelta.y = static_cast<float>(destinationY-static_cast<double>(m_xyz.y));
            targetDelta.z = static_cast<float>(static_cast<double>(targetZ)-static_cast<double>(currentZ));

            weaponSprite = dialog->Link();
            weaponVid = weaponSprite->Vid();
            WEAPON_RUNTIME* const weaponInfo = weaponVid->weapon;
            if (targetDelta.NearLength()>weaponInfo->BattleRange)
                return 0;

            VECTOR destination;
            destination.x = static_cast<float>(firstParameter);
            destination.y = static_cast<float>(destinationY);
            destination.z = targetZ;
            weaponSprite = dialog->Link();
            (void) weaponSprite->SetCommand(4, destination);
            return 0;
        }

    case ACT_ADD_ITEM:
        case ACT_CHANGE_VID:

            (void) DebugAssert(2u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\man.cpp", 290u);
        return 0;

    case ACT_DAMAGE:
        {
            if (m_currentAnimation >= 15)
                return 0;

            VID* actorVid = m_vid;
            bool usesRpgWeapon = false;
            if (actorVid->spriteClass == 7u)
            {
                VID* linkedVid = actorVid->linkVid;
                if (linkedVid)
                {
                    WEAPON_RUNTIME* const linkedWeapon = linkedVid->weapon;
                    usesRpgWeapon = (linkedWeapon->Property&0x00000800u) != 0u;
                }
            }

            void* rpgHero = m_pRPGHero;
            if (!usesRpgWeapon)
            {
                if (rpgHero)
                    return UNIT::Action(static_cast<int>(actionBits), static_cast<int>(firstParameterBits), static_cast<int>(secondParameterBits), static_cast<int>(thirdParameterBits));

                iPlayer* playerInterface = m_pPlayer;
                if (!playerInterface || playerInterface->Flagman() != this)
                    return 0;
                return UNIT::Action(static_cast<int>(actionBits), static_cast<int>(firstParameterBits), static_cast<int>(secondParameterBits), static_cast<int>(thirdParameterBits));
            }

            if (!rpgHero || firstParameter == 0)
                return 0;

            (void) static_cast<zRPGHero*>(rpgHero)->GetHp();
            m_suppressHpMirror = 1u;
            const unsigned int appliedDamage = static_cast<unsigned int>(reinterpret_cast<zRPGHero*>(m_pRPGHero)->ActDamage(static_cast<int>(firstParameterBits)));
            m_suppressHpMirror = 0u;
            if (appliedDamage)
                (void) SPRITE::Action(
                    static_cast<int>(actionBits), static_cast<int>(appliedDamage),
                    static_cast<int>(secondParameterBits), static_cast<int>(thirdParameterBits));

            const int currentSpriteHp = m_hp;
            const int heroHp = reinterpret_cast<zRPGHero*>(m_pRPGHero)->GetHp();
            if (currentSpriteHp != heroHp && heroHp>0)
            {
                iMan* const manInterface = static_cast<iMan*>(this);
                (void) manInterface->SetHpFromHero(reinterpret_cast<zRPGHero*>(m_pRPGHero)->GetHp());
            }
            return 0;
        }

    case ACT_SET_HP:
        {
            VID* actorVid = m_vid;
            bool usesRpgWeapon = false;
            if (actorVid->spriteClass == 7u)
            {
                VID* linkedVid = actorVid->linkVid;
                if (linkedVid)
                {
                    WEAPON_RUNTIME* const linkedWeapon = linkedVid->weapon;
                    usesRpgWeapon = (linkedWeapon->Property&0x00000800u) != 0u;
                }
            }
            if (usesRpgWeapon)
            {
                (void) DebugAssert(2u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\man.cpp", 283u);
                return 0;
            }
            return UNIT::Action(static_cast<int>(actionBits), static_cast<int>(firstParameterBits), static_cast<int>(secondParameterBits), static_cast<int>(thirdParameterBits));
        }

    case ACT_GET_AMMO:
            if (m_pRPGHero)
            return GetAmmoCnt();
        return UNIT::Action(static_cast<int>(actionBits), static_cast<int>(firstParameterBits), static_cast<int>(secondParameterBits), static_cast<int>(thirdParameterBits));

    case ACT_ADD_AMMO:
        {
            void* rpgHero = m_pRPGHero;
            VID* actorVid = m_vid;
            bool usesRpgWeapon = false;
            if (rpgHero && actorVid->spriteClass == 7u)
            {
                VID* linkedVid = actorVid->linkVid;
                if (linkedVid)
                {
                    WEAPON_RUNTIME* const linkedWeapon = linkedVid->weapon;
                    usesRpgWeapon = (linkedWeapon->Property&0x00000800u) != 0u;
                }
            }
            if (usesRpgWeapon)
                static_cast<zRPGHero*>(rpgHero)->DecAmmo(BitsToSigned32(0u-static_cast<unsigned int>(firstParameter)));
            return UNIT::Action(ACT_ADD_AMMO, static_cast<int>(firstParameterBits), 0, 0);
        }

    case ACT_NEXT_COMMAND:
        {
            if (m_currentAnimation >= 15)
                return 0;

            VID* actorVid = m_vid;
            if (actorVid->spriteClass == 7u)
            {
                VID* linkedVid = actorVid->linkVid;
                if (linkedVid)
                {
                    const WEAPON_RUNTIME* const linkedWeapon = linkedVid->weapon;
                    if ((linkedWeapon->Property&0x00000800u) != 0u)
                    {
                        SetMoveAnimation();
                        return 0;
                    }
                }
            }

            const unsigned int actorVidId = static_cast<unsigned int>(actorVid->nVid);
            if (actorVidId == 1206u || actorVidId == 3000u || actorVidId == 1203u)
                return 0;
            return UNIT::Action(static_cast<int>(actionBits), static_cast<int>(firstParameterBits), static_cast<int>(secondParameterBits), static_cast<int>(thirdParameterBits));
        }

    default:
            return UNIT::Action(static_cast<int>(actionBits), static_cast<int>(firstParameterBits), static_cast<int>(secondParameterBits), static_cast<int>(thirdParameterBits));
    }
}

void MAN::SetTankCanShot()
{
    m_tankCanShoot = 1u;
}

static unsigned char ManDirectionDifference(unsigned char leftDirection, unsigned char rightDirection)
{
    const unsigned char forwardDifference = static_cast<unsigned char>(leftDirection-rightDirection);
    const unsigned char backwardDifference = static_cast<unsigned char>(rightDirection-leftDirection);
    return forwardDifference<backwardDifference?forwardDifference:backwardDifference;
}
static float ManAbsFloat(float value)
{
    return value<0.0f?-value:value;
}

void MAN::MoveTact()
{
    DIALOG_UNIT* const dialog = static_cast<DIALOG_UNIT*>(this);
    SPRITE* const sprite = static_cast<SPRITE*>(this);

    dialog->DialogTact();

    VID* actorVid = m_vid;
    const unsigned int actorVidId = static_cast<unsigned int>(actorVid->nVid);
    if (actorVidId == 1206u || actorVidId == 3000u)
    {
        iPlayer* playerInterface = m_pPlayer;
        const unsigned int isCurrentPlayerBits = playerInterface && playerInterface->IsCurPlayer()?1u:0u;
        static_cast<zCarControl*>(this)->MoveVehicle(sprite, isCurrentPlayerBits != 0u);
        return;
    }

    const float previousZ = m_xyz.z;
    VECTOR nextPosition = {m_xyz.x, m_xyz.y, previousZ};
    static_cast<SPRITE*>(this)->MoveTactCalcCoor(&nextPosition);

    const VECTOR* dialogPosition = &dialog->Coor();
    const bool positionChanged =
        !FloatEqualOrUnordered(nextPosition.x, dialogPosition->x) ||
    !FloatEqualOrUnordered(nextPosition.y, dialogPosition->y) ||
    !FloatEqualOrUnordered(nextPosition.z, dialogPosition->z);

    if (positionChanged)
    {
        const VECTOR2 nextPositionXY(nextPosition.x, nextPosition.y);
        const VECTOR2 worldOrigin(0, 0);
        const VECTOR2& worldSize = Map->SizeXY();
        if (nextPositionXY >= worldOrigin && nextPositionXY<worldSize)
        {
            SPRITE* blockingSprite = sprite->CanPlaceWithCrushAndGlide(&nextPosition);
            if (blockingSprite == 0)
                (void) sprite->ChangeCoor(nextPosition);
            else
                m_runtimeFlags |= 0x00001000u;
        }
    }

    SPRITE* goalSprite = m_goalSprite;
    unsigned int runtimeFlags = m_runtimeFlags;
    if (goalSprite && (runtimeFlags&0x7Cu) == 0x04u)
    {
        SPRITE* const directionGoal = sprite->Goal();
        unsigned char desiredDirection = sprite->DirectionTo(directionGoal).value;
        if (FloatLessOrUnordered(m_speed, 0.0f))
            desiredDirection = static_cast<unsigned char>(desiredDirection+128u);

        ANGLE glideDirection;
        glideDirection.value = desiredDirection;
        const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
        sprite->RotateForGlide(glideDirection, static_cast<int>(elapsed));

        runtimeFlags = m_runtimeFlags;
        bool mustStop = (runtimeFlags&0x01000000u) != 0u && (runtimeFlags&0x02000000u) != 0u;
        if (!mustStop)
        {
            goalSprite = m_goalSprite;
            VID* linkedVid = goalSprite->Vid();
            const VECTOR* goalPosition = &goalSprite->Coor();
            ANGLE goalDirection;
            goalDirection = goalSprite->Direction();
            mustStop = sprite->IsXYCross(linkedVid, *goalPosition, goalDirection) != 0;
        }

        if (mustStop)
            sprite->Stop();
        else
        {
            const float nextZ = m_xyz.z;
            if (!FloatEqualOrUnordered(nextZ, previousZ))
            {
                goalSprite = m_goalSprite;
                const float heightChange = nextZ-previousZ;
                const VECTOR& goalCoordinates = goalSprite->Coor();
                VECTOR goalFollowPosition = {goalCoordinates.x, goalCoordinates.y+heightChange, goalCoordinates.z+heightChange};
                goalSprite->ChangeCoor(goalFollowPosition);
            }
        }
    }

    SPRITE* weaponSprite = dialog->Link();
    if (weaponSprite)
    {
        weaponSprite = dialog->Link();
        VID* const weaponVid = weaponSprite->Vid();
        WEAPON_RUNTIME* const weaponInfo = weaponVid->weapon;
        actorVid = m_vid;
        if ((weaponInfo->Property&0x00000800u) != 0u &&
            actorVid->noAnimCadr[6] != 0)
        {
            const float movementSpeed = m_speed;
            runtimeFlags = m_runtimeFlags;
            const bool checkFacing = !FloatEqualOrUnordered(movementSpeed, 0.0f) ||
            ((runtimeFlags&0x00000400u) != 0u);
            if (checkFacing)
            {
                weaponSprite = dialog->Link();
                const unsigned char directionDifference = ManDirectionDifference(
                    m_direction.value, weaponSprite->Direction().value);
                if (directionDifference>64u)
                {
                    const unsigned int animation = static_cast<unsigned int>(m_currentAnimation);
                    if (animation == 0u || animation == 2u || animation == 3u)
                    {
                        sprite->ChangeAnimation(6);
                        RotateHeadTact();
                        return;
                    }
                    RotateHeadTact();
                    return;
                }
                if ((runtimeFlags&0x00000400u) != 0u)
                {
                    RotateHeadTact();
                    return;
                }
            }

            if (static_cast<unsigned int>(m_currentAnimation) == 6u)
            {
                const float minimumTurnSpeed = ManAbsFloat(sprite->MaxSpeed())*0.25f;
                if (minimumTurnSpeed>ManAbsFloat(movementSpeed))
                {
                    sprite->ChangeAnimation(0);
                    sprite->ChangeDirection(ANGLE(static_cast<unsigned char>(m_direction.value+128u)));
                }
            }

            if (FloatEqualOrUnordered(movementSpeed, 0.0f))
            {
                if (static_cast<unsigned int>(m_turnState) != 0u)
                {
                    const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
                    weaponSprite = dialog->Link();
                    const ANGLE remainingTurn = sprite->RotateTact(
                        ANGLE(weaponSprite->Direction().value), static_cast<int>(elapsed));
                    if (remainingTurn.value<16u)
                    {
                        m_turnState = 0;
                        RotateHeadTact();
                        return;
                    }
                }
                else
                {
                    weaponSprite = dialog->Link();
                    const unsigned char directionDifference = ManDirectionDifference(
                        m_direction.value, weaponSprite->Direction().value);
                    m_turnState = directionDifference>55u?1:0;
                }
            }
        }
    }

    RotateHeadTact();
}

