
#include "sprite.h"
#include "engine_globals.h"
#include "sprite_collector.h"
#include "unit.h"
#include "creature.h"
#include "region.h"
#include "vid.h"
#include <new>
#include "basetype.h"
#include "map.h"

extern "C" void*  CreatureVectorDeletingDestructorAlias(void* self, unsigned int flags)
                                            ;
extern "C" void*  CreatureVectorDeletingDestructorAlias(void* self, unsigned int flags)
{
    CREATURE* creature = static_cast<CREATURE*>(self);
    creature->CREATURE::~CREATURE();
    if (flags&1u) CrtFreePointer(self);
    return self;
}

extern "C" void*  CreatureAdjustorDeletingDestructorAlias(void* self, unsigned int flags)
{
    CREATURE* const completeCreature = static_cast<CREATURE*>(
        static_cast<UNIT*>(static_cast<DIALOG_UNIT*>(self)));
    return CreatureVectorDeletingDestructorAlias(completeCreature, flags);
}

VID* SPRITE::Vid() const
{
    return m_vid;
}


VECTOR2& VECTOR::XY()
{
    return *reinterpret_cast<VECTOR2*>(this);
}

const ANGLE ANGLE::operator-(const ANGLE& otherAngle) const
{
    ANGLE differenceAngle;
    differenceAngle.value = static_cast<unsigned char>(value-otherAngle.value);
    return differenceAngle;
}

CREATURE::CREATURE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : UNIT(vid, position, direction, parent)
{
    m_regionFlags |= 1u;
    m_zSpeed = m_vid->maxZSpeed;
    StartMove();
    m_region = FindRegion(m_xyz);
}

CREATURE::~CREATURE()
{
}


void CREATURE::DeletePointerToSprite(SPRITE* sprite)
{
    if (m_region == reinterpret_cast<REGION*>(sprite))
        m_region = 0;
    SPRITE::DeletePointerToSprite(sprite);
}

REGION* CREATURE::FindRegion(const VECTOR& position)
{
    SPRITE_COLLECTOR* const spriteCollector = ::Sprites;
    const float regionScale = 0.5f;
    for (int spriteIndex = spriteCollector->allSprites.m_count-1; spriteIndex >= 0; --spriteIndex)
    {
        if (spriteIndex >= spriteCollector->allSprites.m_count)
            return 0;
        SPRITE* const candidateSprite = spriteCollector->allSprites.m_data[spriteIndex];
        if (!candidateSprite)
            continue;
        if (candidateSprite->Vid()->spriteClass != 23u)
            continue;
        REGION* const region = static_cast<REGION*>(candidateSprite);
        const double regionMinX = static_cast<double>(region->X())-
        static_cast<double>(region->HalfWidth())*regionScale;
        const float lowerHeightExtent = region->HalfHeight()*regionScale;
        const float regionMinY = region->Y()-lowerHeightExtent;
        if (regionMinX>position.x || !(position.y >= regionMinY))
            continue;
        const double regionMaxX = static_cast<double>(region->X())+
        static_cast<double>(region->HalfWidth())*regionScale;
        const float upperHeightExtent = region->HalfHeight()*regionScale;
        const float regionMaxY = region->Y()+upperHeightExtent;
        if (!(regionMaxX >= position.x) || position.y>regionMaxY)
            continue;
        return region;
    }
    return 0;
}

static bool CreatureEqualOrUnordered(float left, float right)
{
    return left == right || left != left || right != right;
}

static bool CreatureLessOrUnordered(float left, float right)
{
    return left<right || left != left || right != right;
}

static int CreatureRandom(const CREATURE* creature, unsigned int range)
{
    const VID* const creatureVid = creature->Vid();
    return (creatureVid->runtimeFlags&0x1000u)
        ? Random(static_cast<int>(range))
        : Random2(static_cast<int>(range));
}

static void CreatureRotateOffset(CREATURE* creature, int turnDelta, unsigned int elapsedTicks)
{
    const unsigned char currentDirection = creature->Direction().value;
    const unsigned char desiredDirection = static_cast<unsigned char>(static_cast<int>(currentDirection)+turnDelta);
    (void) creature->RotateTact(ANGLE(desiredDirection), static_cast<int>(elapsedTicks));
}

void CREATURE::MoveTact()
{
    if ((m_regionFlags&1u) != 0u && !m_region)
    {
        m_regionFlags &= ~1u;
        m_region = FindRegion(Coor());
    }

    VECTOR nextPosition(0.0f, 0.0f, 0.0f);
    MoveTactCalcCoor(&nextPosition);

    int turnCountdown = m_turnTimer;
    if (turnCountdown != 0)
    {
        if ((turnCountdown&1) != 0)
        {
            const int animation = m_currentAnimation;
            bool turnNegative = false;
            if (animation == 4) turnNegative = true;
            else if (animation != 5 && CreatureRandom(this, 1u) == 0) turnNegative = true;
            const unsigned int elapsedTicks = ::CurrentTime-::PrevCurrentTime;
            CreatureRotateOffset(this, turnNegative?-64:+64, elapsedTicks);
        }
        turnCountdown += (turnCountdown<0)?1:-1;
        m_turnTimer = turnCountdown;
    }
    else if (m_goalSprite && !CreatureEqualOrUnordered(m_speed, 0.0f))
    {
        const unsigned char reverseDirectionOffset = (m_speed<0.0f)?128u:0u;
        unsigned char targetDirection = DirectionTo(m_goalSprite).value;
        targetDirection = static_cast<unsigned char>(targetDirection+reverseDirectionOffset);
        const unsigned int elapsedTicks = ::CurrentTime-::PrevCurrentTime;
        RotateForGlide(ANGLE(targetDirection), static_cast<int>(elapsedTicks));
        if ((m_runtimeFlags&0x01000000u) != 0u && (m_runtimeFlags&0x02000000u) != 0u)
            Stop();
    }

    if ((m_vid->property&0x200u) != 0u)
    {
        (void) Map->GetGroundZ(m_vid, nextPosition.XY(), m_direction);
    }

    if (CanPlaceWithCrush(nextPosition) != 0)
    {
        m_speed = 0.0f;
        if (m_turnTimer != 0) return;
        m_turnTimer = 10;
        return;
    }

    if (m_region)
    {
        const float scale = 0.5f;
        const float halfRegionWidth = m_region->HalfWidth()*scale;
        const float halfRegionHeight = m_region->HalfHeight()*scale;
        const VECTOR& regionCoordinates = m_region->Coor();
        const float regionMinX = regionCoordinates.x-halfRegionWidth;
        const float regionMinY = regionCoordinates.y-halfRegionHeight;
        const float regionMaxX = regionCoordinates.x+halfRegionWidth;
        const float regionMaxY = regionCoordinates.y+halfRegionHeight;
        const bool outsideRegion = CreatureLessOrUnordered(nextPosition.x, regionMinX) ||
        CreatureLessOrUnordered(nextPosition.y, regionMinY) ||
        nextPosition.x>regionMaxX || nextPosition.y>regionMaxY;
        if (outsideRegion)
        {
            REGION* const nextRegion = FindRegion(nextPosition);
            if (!nextRegion || m_region->Vid() != nextRegion->Vid())
            {
                m_speed = 0.0f;
                if (m_turnTimer != 0) return;
                m_turnTimer = 10;
                return;
            }
            m_region = nextRegion;
        }
    }

    (void) MoveTactMapLimit(nextPosition);
    ChangeCoor(nextPosition);
}

int CREATURE::Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter)
{
    if (actionCode != ACT_NEXT_COMMAND && actionCode != ACT_DAMAGE)
        return UNIT::Action(actionCode, firstParameter, secondParameter, thirdParameter);

    if (actionCode == ACT_DAMAGE)
    {
        if (firstParameter>0)
        {
            SPRITE_COLLECTOR* const spriteCollector = ::Sprites;
            const float radius = 1.5e+02f;
            const float minX = m_xyz.x-radius;
            const float minY = m_xyz.y-radius;
            const float maxX = m_xyz.x+radius;
            const float maxY = m_xyz.y+radius;
            SPRITE* candidateSprite = spriteCollector->FirstHashInBox(minX, minY, maxX, maxY);
            while (candidateSprite)
            {
                VID* const candidateVid = candidateSprite->Vid();
                if (candidateVid->spriteClass == 25)
                {
                    VECTOR2 candidateDelta(m_xyz.x-candidateSprite->X(),
                        m_xyz.y-candidateSprite->Y());
                    const float candidateDistance = candidateDelta.NearLength();
                    if (CreatureLessOrUnordered(candidateDistance, radius))
                        (void) candidateSprite->StartMove();
                }
                candidateSprite = ::Sprites->NextHashInBox();
            }
        }
        return UNIT::Action(ACT_DAMAGE, firstParameter, secondParameter, thirdParameter);
    }

    int animation = m_currentAnimation;
    if (animation >= 15) return 0;
    if (animation == 8 || animation == 13) (void) ChangeAnimation(0);

    unsigned int regionFlags = m_regionFlags;
    if ((regionFlags&1u) != 0u && m_region == 0)
    {
        m_regionFlags = regionFlags&~1u;
        m_region = FindRegion(Coor());
    }

    if (m_goalSprite && (m_runtimeFlags&0x7Cu) == 0x04u)
    {
        SetMoveAnimation();
    }

    unsigned int runtimeFlags = m_runtimeFlags;
    const unsigned int commandMode = runtimeFlags&0x7Cu;
    const unsigned int commandTick = ::CurrentTime;
    bool mayResetCommand = (commandMode == 0x0Cu || commandMode == 0x10u);
    if (!mayResetCommand)
    {
        const unsigned int alignedTick = commandTick-(commandTick&0x7FFu);
        mayResetCommand = alignedTick>m_lastAnimationTick;
    }
    if (mayResetCommand && (runtimeFlags&0x380u) == 0x100u &&
        CreatureEqualOrUnordered(m_speed,
        0.0f))
    {
        bool hasTarget = false;
        SPRITE* const linkedSprite = m_childChain;
        if (linkedSprite)
        {
            VID* const actorVid = m_vid;
            VID* const linkedVid = linkedSprite->Vid();
            if (linkedVid == actorVid->linkVid && linkedVid->nWeapon != 0 && linkedVid->childVid[8])
            {
                SPRITE* const linked = Link();
                if (linked->Goal()) hasTarget = true;
            }
        }
        if (!hasTarget && m_goalSprite) hasTarget = true;
        if (hasTarget) (void) SetCommand(0, static_cast<SPRITE*>(0));
    }

    runtimeFlags = m_runtimeFlags;
    const unsigned int movementMode = runtimeFlags&0x7Cu;
    if (movementMode != 0x04u && movementMode != 0x0Cu && movementMode != 0x10u)
    {
        animation = m_currentAnimation;
        if (animation == 4 && CreatureRandom(this, 2u) != 0)
            CreatureRotateOffset(this, -32, ::CurrentTime-m_lastAnimationTick);
        if (m_currentAnimation == 5 && CreatureRandom(this, 2u) != 0)
            CreatureRotateOffset(this, +32, ::CurrentTime-m_lastAnimationTick);

        const unsigned int currentTick = ::CurrentTime;
        const unsigned int alignedTick = currentTick-(currentTick&0x7FFu);
        if (alignedTick>m_lastAnimationTick)
        {
            if (CreatureRandom(this, 3u) == 0)
            {
                (void) StartMove();
            }
            else
            {
                const float zeroSpeed = 0.0f;
                if (CreatureEqualOrUnordered(m_speed, zeroSpeed) && CreatureRandom(this, 2u) == 0)
                {
                    VID* const animationVid = m_vid;
                    if (animationVid->noAnimCadr[12] != 0 || animationVid->childVid[12] || animationVid->sfx[12] != 0)
                    {
                        (void) ChangeAnimation(12);
                        goto creature_action_finish;
                    }
                }
                if (CreatureRandom(this, 3u) != 0)
                {
                    (void) Stop();
                }
                else if (CreatureRandom(this, 1u) != 0)
                {
                    CreatureRotateOffset(this, -32, ::CurrentTime-m_lastAnimationTick);
                }
                else
                {
                    CreatureRotateOffset(this, +32, ::CurrentTime-m_lastAnimationTick);
                }
            }
        }
    }

    creature_action_finish:
        animation = m_currentAnimation;
    if (animation == 13 || animation == 9) (void) ChangeAnimation(0);
    return 0;
}


