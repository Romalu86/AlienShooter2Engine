#include "mylib.h"
#include "engine_globals.h"

#include "constant.h"
#include "basetype.h"
#include "balloon.h"
#include "map.h"
#include "vid.h"
#include "sprite_collector.h"
#include "script.h"
#include "dialogSrv.h"
#include <new>

PHRASE::~PHRASE()
{
}

extern "C" void*  ScriptStackVectorDeletingDestructorAlias(void* self, unsigned int flags)
{
    return as2::core::vectorDeletingDestructor(self, flags, 12u,
        [](void* item){ static_cast<SCRIPTSTACK*>(item)->~SCRIPTSTACK(); },
        [](void* storage){ CrtFreePointer(storage); });
}

extern "C" void*  PhraseVectorDeletingDestructorAlias(void* self, unsigned int flags)
{
    return as2::core::vectorDeletingDestructor(self, flags, 20u,
        [](void* item){ static_cast<PHRASE*>(item)->~PHRASE(); },
        [](void* storage){ CrtFreePointer(storage); });
}

extern "C" void*  BalloonVectorDeletingDestructorAlias(void* self, unsigned int flags)
{
    BALLOON* object = static_cast<BALLOON*>(self);
    object->BALLOON::~BALLOON();
    if (flags&1u) CrtFreePointer(self);
    return self;
}

extern "C" void*  BalloonAdjustorDeletingDestructorAlias(void* self, unsigned int flags)
{
    BALLOON* const complete = static_cast<BALLOON*>(
        static_cast<UNIT*>(static_cast<DIALOG_UNIT*>(self)));
    return BalloonVectorDeletingDestructorAlias(complete, flags);
}

ANGLE SPRITE::DirectionTo(const SPRITE* target) const
{
    const VECTOR& targetCoor = target->Coor();
    VECTOR2 delta;
    delta.x = targetCoor.x-m_xyz.x;
    delta.y = targetCoor.y-m_xyz.y;
    return Decart2Polar(delta.x, delta.y);
}

int SPRITE::Attack(const VECTOR& target)
{
    void* const storage = static_cast<unsigned char*>(CrtNew(104u));
    SPRITE* const created = storage?new (storage) SPRITE(target, ANGLE(0)):0;
    return Attack(created);
}


BALLOON::BALLOON(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : PLANE(vid, position, direction, parent),
      m_attachmentTransitionPending(0u), m_attachmentPhase(0u)
{
}

BALLOON::~BALLOON()
{
}

static float BalloonWeightedDistance(const SPRITE* firstSprite, const SPRITE* secondSprite)
{
    float dx = firstSprite->Coor().x-secondSprite->Coor().x;
    float dy = firstSprite->Coor().y-secondSprite->Coor().y;
    if (dx<0.0f) dx = -dx;
    if (dy<0.0f) dy = -dy;
    const float diagonal = (0.398000002f);

    return (dx>dy) ? (dx+dy*diagonal) : (dy+dx*diagonal);
}

static bool BalloonReservationMatches(const SPRITE* self, const SPRITE* object)
{
    if (!object) return false;
    const VID* const objectVid = object->Vid();
    return objectVid && objectVid->linkVid == self->Vid() &&
    (((object->RuntimeFlags()^self->RuntimeFlags())&0x000E0000u) == 0u);
}

void BALLOON::MoveToNearestBase()
{
    SPRITE_COLLECTOR* manager = ::Sprites;
    int* list = &manager->reserved2EC;
    SPRITE* candidate = manager->FirstUnit(list, -1);
    SPRITE* best = 0;

    while (candidate)
    {
        VID* const candidateVid = candidate->Vid();
        VID* const group = candidateVid?candidateVid->linkVid:0;
        if (group == m_vid && (((candidate->RuntimeFlags()^m_runtimeFlags)&0x000E0000u) == 0u))
        {
            SPRITE* const child = candidate->ChildChain();
            if ((!child || child->Vid() != group) && ReadU32(candidate, 252u) == 0u)
            {
                if (!best || FloatLessOrUnordered(
                    BalloonWeightedDistance(candidate, this),
                    BalloonWeightedDistance(best, this)))
                    best = candidate;
            }
        }
        candidate = manager->NextUnit(list);
    }

    if (best)
    {
        WriteU32(best, 252u, 1u);
        Move(best);
    }
}

void BALLOON::ConnectToBase()
{
    SPRITE* target = m_goalSprite;
    if (!target)
    {
        ChangeAnimation(15);
        return;
    }

    VID* const targetVid = target->Vid();
    VID* const group = targetVid?targetVid->linkVid:0;
    if (group != m_vid || (((target->RuntimeFlags()^m_runtimeFlags)&0x000E0000u) != 0u))
    {
        ChangeAnimation(15);
        return;
    }

    SPRITE* const child = target->ChildChain();
    if (child && child->Vid() == group)
    {
        (void) SetCommand(0, static_cast<SPRITE*>(0));
        m_attachmentPhase = 1u;
        return;
    }

    m_zSpeed = 0.0f;
    VECTOR point(
        targetVid->linkXYZ.x+target->Coor().x,
        targetVid->linkXYZ.y+target->Coor().y,
        targetVid->linkXYZ.z+target->Coor().z);
    ChangeCoor(point);
    m_behave = target->Behave();
    (void) target->AddLinkToLast(this);
    WriteU32(target, 252u, 1u);
    (void) SetCommand(0, static_cast<SPRITE*>(0));
}

void BALLOON::ZSpeedInitialization()
{
    const VECTOR2 groundPoint(m_xyz.x, m_xyz.y);
    const float ground = Map->GetGroundZ(m_vid, groundPoint, m_direction);

    if (m_childBacklink)
    {
        m_attachmentPhase = 0u;
    }
    else if (m_goalSprite && BalloonReservationMatches(this, m_goalSprite))
    {
        WriteU32(m_goalSprite, 252u, 1u);
    }

    const unsigned int phase = static_cast<unsigned int>(m_attachmentPhase);
    if (phase == 2u)
    {
        VID* selfVid = m_vid;
        m_runtimeFlags &= ~0x00000400u;
        m_speed = 0.0f;
        m_zSpeed = -selfVid->maxZSpeed;

        SPRITE* target = m_goalSprite;
        if (target && BalloonReservationMatches(this, target) && target->HaveLink() == 0)
        {
            const VECTOR p(target->Coor().x, target->Coor().y, m_xyz.z);
            ChangeCoor(p);
            const unsigned int elapsed = ::CurrentTime-m_lastAnimationTick;
            (void) RotateTact(target->Direction(), static_cast<int>(elapsed));

            VID* const targetVid = target->Vid();
            const float connectHeight = target->Coor().z+targetVid->linkXYZ.z;

            if (!FloatLessOrUnordered(connectHeight, m_xyz.z))
            {
                ConnectToBase();
                return;
            }
        }
        return;
    }

    if (phase == 1u)
    {
        VID* selfVid = m_vid;
        m_zSpeed = selfVid->maxZSpeed;
        m_runtimeFlags &= ~0x00000400u;
        m_speed = 0.0f;
        const float transitionHeight = ground+selfVid->topZ;

        if (!(transitionHeight>m_xyz.z))
        {
            m_runtimeFlags |= 0x00000400u;
            m_attachmentPhase = 0u;
        }
        return;
    }

    if (phase != 0u)
        return;

    if (m_childBacklink)
    {
        m_zSpeed = 0.0f;
        return;
    }

    VID* selfVid = m_vid;
    const float desiredHeight = ground+selfVid->topZ;
    const float epsilon = (10.0f);

    if ((desiredHeight-epsilon)>m_xyz.z)
    {
        m_zSpeed = selfVid->maxZSpeed;
        return;
    }
    if (FloatLessOrUnordered(desiredHeight+epsilon, m_xyz.z))
    {
        m_zSpeed = -selfVid->maxZSpeed;
        return;
    }
    m_zSpeed = 0.0f;
}

int BALLOON::IsBalloonMoveFinished()
{
    SPRITE* linked = m_goalSprite;
    if (linked)
    {
        const float limit = (10.0f);
        const float dx = AbsFloat(linked->Coor().x-m_xyz.x);
        const float dy = AbsFloat(linked->Coor().y-m_xyz.y);
        if (FloatLessOrUnordered(dx, limit) && FloatLessOrUnordered(dy, limit))
            return 1;
    }
    return ((m_runtimeFlags&0x01000000u) != 0u &&
        (m_runtimeFlags&0x02000000u) != 0u) ? 1 : 0;
}

void BALLOON::CheckFlightProperties()
{
    SPRITE* active = m_childBacklink;
    if (active)
    {
        unsigned int tick = ::CurrentTime;
        if ((tick&0xFFFFFC00u)>m_lastAnimationTick)
        {
            CONSTANT* configuration = ::Const;
            (void) AddAmmoTick(configuration->DirijbanAmmoReloadTime);
            tick = ::CurrentTime;
        }

        SPRITE* target = m_goalSprite;
        bool refreshTick = false;
        if (target)
        {
            const unsigned int activeType = active->RuntimeFlags()&0x7Cu;
            if (active->Goal() != target || (activeType != 0x6Cu && activeType != 0x68u))
            {
                const VECTOR2 delta(
                    target->Coor().x-active->Coor().x,
                    target->Coor().y-active->Coor().y);
                const float distance = delta.NearLength();
                VID* const activeVid = active->Vid();
                const WEAPON_RUNTIME* const state = activeVid->weapon;
                if (FloatLessOrUnordered(distance, state->BattleRange) && m_stateTimer == 0u)
                {
                    if ((m_runtimeFlags&0x7Cu) == 0x24u)
                    {
                        (void) SetCommand(3, target);
                        m_attachmentTransitionPending = 1u;
                    }
                    (void) DestroyLink(activeVid->childVid[13]);
                    WriteU32(active, 24u, 0u);
                    WriteU32(active, 252u, 0u);

                    target = m_goalSprite;
                    if (target && target->Vid() == ::EmptyVid)
                    {
                        (void) active->SetCommand(0, static_cast<SPRITE*>(0));
                        (void) Attack(target->Coor());
                    }

                    ChangeAnimation(2);
                    m_attachmentPhase = 1u;
                    m_childBacklink = 0;
                    m_runtimeFlags &= ~0x00000400u;
                    return;
                }

                refreshTick = true;
            }
        }

        if (refreshTick)
            tick = ::CurrentTime;
        const unsigned int elapsed = tick-m_lastAnimationTick;
        if (RotateTact(active->Direction(), static_cast<int>(elapsed)).value != 0u)
            return;
        ChangeAnimation(0);
        return;
    }

    const int ammo = m_ammoFixedPoint/64;
    if (ammo <= 0 || !m_goalSprite)
    {
        SPRITE* target = m_goalSprite;
        bool keep = false;
        if (target && BalloonReservationMatches(this, target))
        {
            VID* const group = target->Vid()->linkVid;
            SPRITE* const child = target->ChildChain();
            if (!child || child->Vid() == group)
                keep = true;
        }
        if (!keep)
        {
            if (target)
                (void) SetCommand(0, static_cast<SPRITE*>(0));
            MoveToNearestBase();
        }
    }

    SPRITE* target = m_goalSprite;
    if (target && BalloonReservationMatches(this, target))
    {
        VID* const group = target->Vid()->linkVid;
        SPRITE* const child = target->ChildChain();
        if (child && child->Vid() == group)
        {
            (void) SetCommand(0, static_cast<SPRITE*>(0));
            MoveToNearestBase();
        }
    }

    if (m_attachmentPhase == 2u)
        return;
    if ((m_runtimeFlags&0x7Cu) != 0x04u)
        return;
    if (!IsBalloonMoveFinished())
        return;

    target = m_goalSprite;
    if (!target || !BalloonReservationMatches(this, target))
        return;
    {
        VID* const group = target->Vid()->linkVid;
        SPRITE* const child = target->ChildChain();
        if (child && child->Vid() == group)
            return;
    }
    m_attachmentPhase = 2u;
    ZSpeedInitialization();
}

void BALLOON::FlightToTargetAdditionalActions()
{
    SPRITE* target = m_goalSprite;
    if (m_attachmentTransitionPending && target)
    {
        const float epsilon = (10.0f);
        const float dx = AbsFloat(target->Coor().x-m_xyz.x);
        const float dy = AbsFloat(target->Coor().y-m_xyz.y);
        if (FloatLessOrUnordered(dx, epsilon) && FloatLessOrUnordered(dy, epsilon))
        {
            ChangeAnimation(15);
            return;
        }
    }

    target = m_goalSprite;
    if (!target)
        return;
    if (!BalloonReservationMatches(this, target))
        return;
    if (m_ammoFixedPoint/64 <= 0)
        return;
    if ((static_cast<unsigned char>(m_behave)&1u) == 0u)
        return;

    SPRITE* enemy = SeekEnemy();
    if (enemy)
        (void) SetCommand(4, enemy);
}

int BALLOON::SetCommand(int command, SPRITE* target)
{
    m_attachmentTransitionPending = 0u;
    SPRITE* old = m_goalSprite;
    if (BalloonReservationMatches(this, old))
        WriteU32(old, 252u, 0u);
    if (BalloonReservationMatches(this, target))
        WriteU32(target, 252u, 1u);
    return SPRITE::SetCommand(command, target);
}

static bool BalloonFloatEqualOrUnordered(float left, float right)
{
    return FloatEqualOrUnordered(left, right);
}

void BALLOON::MoveTact()
{
    VECTOR next;
    MoveTactCalcCoor(&next);

    if (m_attachmentPhase != 0u)
    {
        const VECTOR constrained(m_xyz.x, m_xyz.y, next.z);
        ChangeCoor(constrained);
        return;
    }

    SPRITE* target = m_goalSprite;
    const float zero = (0.0f);
    if (target && !BalloonFloatEqualOrUnordered(m_speed, zero))
    {
        const unsigned char halfTurn = FloatLessOrUnordered(m_speed, zero)?128u:0u;
        ANGLE requestedDirection = DirectionTo(target)+ANGLE(halfTurn);
        const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
        RotateForGlide(requestedDirection, static_cast<int>(elapsed));
    }

    const bool xSame = BalloonFloatEqualOrUnordered(next.x, m_xyz.x);
    const bool ySame = BalloonFloatEqualOrUnordered(next.y, m_xyz.y);
    if (!xSame || !ySame)
    {
        if (CanPlaceWithCrushAndGlide(&next) == 0)
        {
            (void) MoveTactMapLimit(next);
            ChangeCoor(next);
        }
    }
}
