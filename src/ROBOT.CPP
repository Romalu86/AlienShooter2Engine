#include "mylib.h"
#include "engine_globals.h"

#include "basetype.h"
#include "sprite.h"
#include "sprite_collector.h"
#include "unit.h"
#include "robot.h"
#include "region.h"
#include "vid.h"
#include "map.h"
#include "resread.h"
#include <new>


CIV_ROBOT::CIV_ROBOT(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : CREATURE(vid, position, direction, parent)
{
    m_robotState = 0;
    m_robotBuilding = 0;
    m_robotFlagC0 = 0u;

}

CIV_ROBOT::~CIV_ROBOT()
{
    if (m_robotBuilding) ReportLiveSpriteReference(m_robotBuilding);
}

PTR_SPRITE::~PTR_SPRITE() noexcept(false)
{
    if (m_value) ReportLiveSpriteReference(m_value);
}

extern "C" void* __cdecl PtrSpriteDtorRobotNative(void* self)
{
    static_cast<PTR_SPRITE*>(self)->~PTR_SPRITE();
    return self;
}

namespace {
const int robotBuildingVidIds[] =
{
    170, 803, 805, 806, 807, 809, 810, 813,
    846, 848, 861, 863, 864, 874, 875, 895
};
}

int CIV_ROBOT::IsRobotBuilding(const SPRITE* sprite)
{
    if (!sprite)
        return 0;
    const int vidId = sprite->Vid()->nVid;
    for (unsigned int i = 0; i<sizeof(robotBuildingVidIds)/sizeof(robotBuildingVidIds[0]); ++i)
        if (robotBuildingVidIds[i] == vidId)
            return 1;
    return 0;
}

SPRITE* CIV_ROBOT::FindRobotBuilding()
{
    const float x = m_xyz.x;
    const float y = m_xyz.y;
    SPRITE* selectedBuilding = 0;
    SPRITE* candidateSprite = ::Sprites->FirstHashInBox(
        x-200.0f, y-133.333328f, x+200.0f, y+133.333328f);
    while (candidateSprite)
    {
        VID* const candidateVid = candidateSprite->Vid();
        if ((candidateVid->spriteType&4u) != 0u && candidateVid->spriteClass == 1u && IsRobotBuilding(candidateSprite))
        {
            bool compatible = true;
            if (m_region)
            {
                REGION* peer = FindRegion(candidateSprite->Coor());
                compatible = peer && m_region->Vid() == peer->Vid();
            }
            if (compatible)
            {
                selectedBuilding = candidateSprite;
                const unsigned int randomValue = (m_vid->runtimeFlags&0x1000u)
                    ?static_cast<unsigned int>(::Random(3)):static_cast<unsigned int>(::Random2(3));
                if (randomValue == 0u) break;
            }
        }
        candidateSprite = ::Sprites->NextHashInBox();
    }
    return selectedBuilding;
}

void CIV_ROBOT::PathIsBlocked()
{
    m_speed = 0.0f;
    if (m_turnTimer == 0)
    {
        int (*const randomFunction)(int) = (m_vid->runtimeFlags&0x1000u)
            ?static_cast<int (*)(int)>(&::Random):static_cast<int (*)(int)>(&::Random2);
        const unsigned int result = static_cast<unsigned int>(randomFunction(1));
        m_turnTimer = result?10:-10;
    }
    if ((m_runtimeFlags&0x7Cu) == 4u)
    {
        int (*const randomFunction)(int) = (m_vid->runtimeFlags&0x1000u)
            ?static_cast<int (*)(int)>(&::Random):static_cast<int (*)(int)>(&::Random2);
        if (randomFunction(8) == 0)
        {
            Stop();
            m_robotState = 0;
        }
    }
}

void CIV_ROBOT::DeletePointerToSprite(SPRITE* sprite)
{
    if (m_robotBuilding == sprite)
    {
        if (m_robotBuilding)
            (void) m_robotBuilding->Release();
        m_robotBuilding = 0;
    }
    CREATURE::DeletePointerToSprite(sprite);
}

void CIV_ROBOT::ChangeAnimation(int animation)
{
    if (!m_childChain || m_childChain->Vid() != m_vid->linkVid)
        return;
    if (Link()->Animation() == animation)
        return;
    const int frame = Link()->CurrentFrame();
    if (frame >= Link()->CurrentFrameEnd())
        Link()->ChangeAnimation(animation);
}

void CIV_ROBOT::RotateHead(ANGLE angle)
{
    if (!m_childChain || m_childChain->Vid() != m_vid->linkVid)
        return;
    const int frame = Link()->CurrentFrame();
    if (frame >= Link()->CurrentFrameEnd())
    {
        const unsigned int elapsed = ::CurrentTime-m_lastAnimationTick;
        (void) Link()->RotateTact(angle, static_cast<int>(elapsed));
    }
}

extern unsigned int  UnitFindType17AtPoint(void* self, const float* position);

void CIV_ROBOT::MoveTact()
{
    unsigned int regionFlags = m_regionFlags;
    if ((regionFlags&1u) != 0u && m_region == 0)
    {
        m_regionFlags = regionFlags&~1u;
        m_region = FindRegion(Coor());
    }

    VECTOR next;
    MoveTactCalcCoor(&next);

    if ((m_vid->property&0x200u) != 0u)
    {
        next.z = Map->GetGroundZ(
            m_vid,
            next.XY(),
            ANGLE(m_direction.value));
    }

    const float currentZ = m_xyz.z;
    if ((static_cast<double>(next.z)-currentZ)>m_vid->forMoveUpZ ||
        (static_cast<double>(currentZ)-next.z)>m_vid->forMoveDownZ)
    {
        PathIsBlocked();
        return;
    }

    REGION* const region = m_region;
    if (region)
    {
        const float halfX = region->HalfWidth()*0.5f;
        const float halfY = region->HalfHeight()*0.5f;
        const float minX = region->X()-halfX;
        const float minY = region->Y()-halfY;
        const float maxX = region->X()+halfX;
        const float maxY = region->Y()+halfY;

        const bool aboveMinimum = (next.x >= minX && next.y >= minY);
        const bool notAboveMaximum = (!(next.x>maxX) && !(next.y>maxY));
        if (!aboveMinimum || !notAboveMaximum)
        {
            REGION* const replacement = FindRegion(next);
            if (!replacement || replacement->Vid() != region->Vid())
            {
                PathIsBlocked();
                return;
            }
            m_region = replacement;
        }
    }

    if (!(next.x >= 0.0f) || !(next.y >= 0.0f))
    {
        PathIsBlocked();
        return;
    }
    const VECTOR2& nextXY = next.XY();
    const VECTOR2& mapMaximum = Map->SizeXY();
    if (!(nextXY<mapMaximum))
    {
        PathIsBlocked();
        return;
    }

    const VECTOR probe(next.x, next.y, m_xyz.z);
    SPRITE* const candidate = CanPlaceWithCrush(probe);
    if (!candidate)
    {
        ChangeCoor(next);
        return;
    }

    VID* const candidateOwner = candidate->Vid();
    const VECTOR& candidatePoint = candidate->Coor();
    const unsigned int candidateDirection = static_cast<unsigned int>(candidate->Direction().value);
    ANGLE candidateAngle;
    candidateAngle.value = static_cast<unsigned char>(candidateDirection);
    if (this->IsXYCross(
        candidateOwner, candidatePoint, candidateAngle) != 0)
    {
        const float candidateZ = candidate->Z();
        const double selfTop = static_cast<double>(m_vid->sizeXYZ.z)+m_xyz.z;
        const double candidateTop = static_cast<double>(candidateZ)+candidate->Vid()->sizeXYZ.z;

        if (selfTop >= candidateZ && candidateTop >= m_xyz.z)
        {
            ChangeCoor(next);
            return;
        }
    }

    if (this->IsRobotBuilding(candidate) != 0 && m_robotState == 14)
    {
        const float dx = m_xyz.x-candidate->X();
        const float dy = m_xyz.y-candidate->Y();
        const unsigned char computedDirection = Decart2Polar(dx, dy).value;
        const unsigned char desiredDirection = static_cast<unsigned char>(candidate->Direction().value+128u);
        if (AbsDiffU8(computedDirection, desiredDirection)<30u)
        {
            ChangeCoor(next);
            return;
        }
    }

    if (candidate->Vid() == m_vid &&
        (m_robotState == 15 || static_cast<CIV_ROBOT*>(candidate)->m_robotState == 15))
    {
        ChangeCoor(next);
        return;
    }

    PathIsBlocked();
}

void SPRITE::Move(const VECTOR& point)
{
    void* storage = static_cast<unsigned char*>(CrtNew(104u));
    SPRITE* target = storage?::new (storage) SPRITE(point, ANGLE(0)):0;
    Move(target);
}

int SPRITE::Random(int maximum) const
{
    return (m_vid->runtimeFlags&0x1000u) != 0u
        ? ::Random(maximum)
        : ::Random2(maximum);
}

int VECTOR2::operator>=(const VECTOR2& vectorValue) const
{ return (x>=vectorValue.x && y>=vectorValue.y)?1:0; }

int VECTOR2::operator<=(const VECTOR2& vectorValue) const
{ return (!(x>vectorValue.x) && !(y>vectorValue.y))?1:0; }

ANGLE::ANGLE(const VECTOR2& vectorValue)
{
    value = Decart2Polar(vectorValue.x, vectorValue.y).value;
}

extern "C" void*  AngleVector2CtorNative(void* self, const VECTOR2* vectorValue)
{
        ::new (self) ANGLE(*vectorValue);
    return self;
}

const ANGLE ANGLE::GetInversed() const
{
    return ANGLE(static_cast<unsigned char>(value+128u));
}


static bool RobotAbi_RobotLessOrUnordered(float a, float b)
{
    return !(a >= b);
}
static unsigned char RobotAbi_RobotAngleFromDelta(float dx, float dy)
{
    return Decart2Polar(dx, dy).value;
}

int CIV_ROBOT::Action(int command, int argument1, int argument2, int argument3)
{
    const auto elapsed = [this]() -> unsigned int
    {
        return ::CurrentTime-m_lastAnimationTick;
    };
    const auto randomValue = [this](int bound) -> int
    {
        return Random(bound);
    };
    const auto childMatchesOwner = [this]() -> bool
    {
        return m_childChain && m_vid && m_childChain->Vid() == m_vid->linkVid;
    };
    const auto actionStackEndsWithStop = [this]() -> bool
    {
        if (!m_extraData) return true;
        const LIST<ACT>& lane = m_extraData->actionLane[0];
        if (lane.m_count == 0) return true;
        const ACT* const records = lane.m_data;
        return records[lane.m_count-1].command == ACT_STOP_STACK;
    };
    const auto rotateHeadCurrent = [this]()
    {
        RotateHead(m_direction);
    };
    const auto stopIfFlag400 = [this]()
    {
        if ((m_runtimeFlags&0x400u) != 0u) Stop();
    };

    const auto refreshReference = [this]()
    {
        m_robotFlagC4 = 0u;
        SPRITE_COLLECTOR* const collector = ::Sprites;
        const VECTOR2 point(m_xyz.x, m_xyz.y);
        SPRITE* reference = collector->FindNearestSprite(0x30000015, point, 350.0f, 0);
        if (reference) reference->AddReference();
        if (m_robotBuilding) (void) m_robotBuilding->Release();
        m_robotBuilding = reference;
    };

    const auto chooseState = [&]()
    {
        int state = m_robotState;
        const unsigned int flags = m_runtimeFlags;

        if (state == 14)
        {
            if ((flags&0x400u) == 0u)
            {
                if (!actionStackEndsWithStop()) goto choose_done;
                state = 15;
                m_robotState = state;
            }
            goto choose_done;
        }

        if (state == 15 && randomValue(4) == 0)
        {
            m_robotState = 5;
            Move(m_xyz.x, m_xyz.y+(50.0f), m_xyz.z);
            goto choose_done;
        }

        if (m_robotState == 7 && randomValue(1) != 0)
            goto choose_done;

        if (m_robotFlagC0 != 0u)
        {
            if (randomValue(4) != 0)
            {
                m_robotState = 7;
                goto choose_done;
            }
            m_robotState = randomValue(2) != 0?12:10;
            goto choose_done;
        }

        if (m_robotBuilding)
        {
            if (m_robotState == 9 || randomValue(2) != 0)
            {
                if (randomValue(2) == 0)
                {
                    m_robotState = 9;
                    goto choose_done;
                }
            }
            else
            {
                m_robotState = 9;
                goto choose_done;
            }
            if (m_robotBuilding && randomValue(1) == 0)
            {
                m_robotState = 11;
                goto choose_done;
            }
        }

        if (m_robotState == 5 && randomValue(2) != 0)
        {
            m_robotState = 5;
            goto choose_done;
        }
        if (m_robotState == 1 && randomValue(5) != 0)
        {
            m_robotState = 1;
            goto choose_done;
        }
        if (m_robotState == 4 && randomValue(1) == 0)
        {
            m_robotState = 4;
            goto choose_done;
        }
        if (randomValue(6) == 0)
        {
            m_robotState = 4;
            goto choose_done;
        }

        if (randomValue(5) == 0)
        {
            PlaySFX(129);
            m_robotState = 3;
            goto choose_done;
        }
        if (randomValue(4) == 0)
        {
            PlaySFX(129);
            m_robotState = 2;
            goto choose_done;
        }
        m_robotState = randomValue(4) != 0?5:14;

        choose_done:
            m_robotFlagC0 = 0u;
    };

    const auto runState = [&](int state) -> int
    {
        SPRITE* const child = m_childChain;
        const unsigned int flags = m_runtimeFlags;
        const unsigned int frameElapsed = elapsed();

        switch (state)
        {
        case 0:
                stopIfFlag400();
            rotateHeadCurrent();
            return 0;

        case 13:
                stopIfFlag400();
            ChangeDirection(ANGLE(static_cast<unsigned char>(m_direction.value-32u)));
            rotateHeadCurrent();
            return 0;

        case 12:
                stopIfFlag400();
            ChangeAnimation(7);
            return 0;

        case 11:
                stopIfFlag400();
            if (!m_robotBuilding) return 0;
            RotateHead(ANGLE(RobotAbi_RobotAngleFromDelta(
                m_robotBuilding->Coor().x-m_xyz.x,
                m_robotBuilding->Coor().y-m_xyz.y)));
            return 0;

        case 2:
            case 3:
                stopIfFlag400();
            if (!childMatchesOwner()) return 0;
            RotateHead(ANGLE(static_cast<unsigned char>(
                state == 2?child->Direction().value-32u:child->Direction().value+32u)));
            return 0;

        case 4:
                stopIfFlag400();
            if (!childMatchesOwner()) return 0;
            (void) child->RotateTact(m_direction, static_cast<int>(frameElapsed));
            ChangeAnimation(12);
            return 0;

        case 1:
            {
                SPRITE* const target = m_goalSprite;
                if (!target) return 0;
                const VECTOR2 delta(target->Coor().x-m_xyz.x, target->Coor().y-m_xyz.y);
                const float distance = delta.NearLength();
                const float radius = (150.0f);
                if (!RobotAbi_RobotLessOrUnordered(distance, radius))
                {
                    Move(target);
                    return 0;
                }

                (void) SetCommand(0, static_cast<SPRITE*>(0));
                if (randomValue(2) == 0)
                    (void) RotateTact(ANGLE(static_cast<unsigned char>(randomValue(255))), static_cast<int>(frameElapsed));
                if (randomValue(9) == 0)
                {
                    ChangeAnimation(11);
                    return 0;
                }
                if (randomValue(9) == 0)
                {
                    ChangeAnimation(9);
                    return 0;
                }
                if (randomValue(9) == 0)
                {
                    ChangeAnimation(6);
                    return 0;
                }
                ChangeAnimation(0);
                return 0;
            }

        case 5:
                if ((flags&0x400u) == 0u) StartMove();
            if (m_currentAnimation == 2)
            {
                if (randomValue(2) == 0)
                {
                    (void) RotateTact(ANGLE(static_cast<unsigned char>(m_direction.value-32u)), static_cast<int>(frameElapsed));
                    rotateHeadCurrent();
                    return 0;
                }
                if (m_currentAnimation == 2 && randomValue(2) == 0)
                {
                    (void) RotateTact(ANGLE(static_cast<unsigned char>(m_direction.value+32u)), static_cast<int>(frameElapsed));
                    rotateHeadCurrent();
                    return 0;
                }
            }
            ChangeAnimation(2);
            rotateHeadCurrent();
            return 0;

        case 14:
            {
                if (!m_goalSprite && actionStackEndsWithStop())
                {
                    SPRITE* const building = FindRobotBuilding();
                    if (!building)
                    {
                        m_robotFlagC4 = 1u;
                        return 0;
                    }
                    const unsigned int direction = building->Direction().value;
                    const unsigned int tableOffset = (direction&0xFFu)<<2;
                    const float scale = (32.0f);
                    const VECTOR position(
                        building->Coor().x-ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin()*scale,
                        building->Coor().y+ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos()*scale,
                        building->Coor().z);
                    Move(position);
                    AddAction(0, ACT_MOVE_TO, static_cast<int>(PtrToU32(building)), 0, 0);
                    return 0;
                }
                if ((flags&0x400u) == 0u && actionStackEndsWithStop())
                    m_robotState = 15;
                rotateHeadCurrent();
                return 0;
            }

        case 8:
                if ((flags&0x400u) == 0u) StartMove();
            if (m_speed >= m_vid->maxSpeed) m_speed = m_vid->maxSpeed*2.0f;
            if (!childMatchesOwner()) return 0;
            (void) child->RotateTact(m_direction, static_cast<int>(frameElapsed));
            ChangeAnimation(11);
            return 0;

        case 7:
                if ((flags&0x400u) == 0u) StartMove();
            if (m_speed >= m_vid->maxSpeed) m_speed = m_vid->maxSpeed*2.0f;
            if (m_robotBuilding)
            {
                ANGLE direction = DirectionTo(m_robotBuilding);
                direction.value = static_cast<unsigned char>(direction.value+128u);
                (void) RotateTact(direction, static_cast<int>(frameElapsed));
            }
            if (!childMatchesOwner()) return 0;
            (void) child->RotateTact(m_direction, static_cast<int>(frameElapsed));
            ChangeAnimation(11);
            return 0;

        case 9:
            case 10:
            {
                if ((flags&0x400u) != 0u) Stop();
                if (!childMatchesOwner()) return 0;
                const ANGLE desired = m_robotBuilding?DirectionTo(m_robotBuilding):m_direction;
                (void) child->RotateTact(desired, static_cast<int>(frameElapsed));
                if (state == 10)
                {
                    ChangeAnimation(6);
                    return 0;
                }
                ChangeAnimation(randomValue(2) == 0?11:9);
                return 0;
            }

        case 6:
            default:
                return 0;
        }
    };

    switch (command)
    {
    case 9:
            if (m_currentAnimation == 8) return 0;
        if (childMatchesOwner())
        {
            m_childChain->ChangeAnimation(9);
            return 0;
        }
        ChangeAnimation(9);
        return 0;

    case ACT_MOVE_TO:
        {
            SPRITE* const anchor = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            if (!anchor) return 0;
            VID* const owner = anchor->Vid();
            const unsigned int tableOffset = (static_cast<unsigned int>(anchor->Direction().value)&0xFFu)<<2;
            const float scale = (4.0f);
            const VECTOR position(
                anchor->Coor().x-ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos()*scale,
                anchor->Coor().y+ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin()*scale,
                anchor->Coor().z+owner->forMoveUpZ);
            Move(position);
            return 0;
        }

    case ACT_DAMAGE:
            if (argument1>0)
        {
            SPRITE_COLLECTOR* const collector = ::Sprites;
            const float radius = (150.0f);
            const float ySpan = (300.0f);
            SPRITE* candidate = reinterpret_cast<SPRITE*>(collector->FirstHashInBox(
                m_xyz.x-radius, m_xyz.y-ySpan, m_xyz.x+radius, m_xyz.y+ySpan));
            while (candidate)
            {
                VID* const candidateOwner = candidate->Vid();
                if (candidateOwner && candidateOwner->spriteClass == 20)
                {
                    const VECTOR2 delta(candidate->Coor().x-m_xyz.x, candidate->Coor().y-m_xyz.y);
                    if (RobotAbi_RobotLessOrUnordered(delta.NearLength(), radius))
                    {
                        CIV_ROBOT* const robot = static_cast<CIV_ROBOT*>(candidate);
                        robot->m_robotFlagC0 = 1u;
                        robot->m_robotFlagC4 = 1u;
                    }
                }
                candidate = collector->NextHashInBox();
            }
        }
        return UNIT::Action(ACT_DAMAGE, argument1, argument2, argument3);

    case ACT_NEXT_COMMAND:
        {
            if (m_currentAnimation >= 15) return 0;

            if ((m_regionFlags&1u) != 0u && !m_region)
            {
                m_regionFlags &= ~1u;
                m_region = FindRegion(Coor());
            }

            if ((m_runtimeFlags&0x01000000u) != 0u && (m_runtimeFlags&0x02000000u) != 0u)
                Stop();

            if (m_turnTimer != 0)
            {
                const ANGLE desired(static_cast<unsigned char>(
                    m_turnTimer>0?m_direction.value-32u:m_direction.value+32u));
                (void) RotateTact(desired, static_cast<int>(elapsed()));
                m_turnTimer += (m_turnTimer<0)?1:-1;
            }
            else if (m_goalSprite && (m_runtimeFlags&0x7Cu) == 0x04u)
            {
                unsigned char desired = RobotAbi_RobotAngleFromDelta(
                    m_goalSprite->Coor().x-m_xyz.x, m_goalSprite->Coor().y-m_xyz.y);
                if (RobotAbi_RobotLessOrUnordered(m_speed, (0.0f)))
                    desired = static_cast<unsigned char>(desired+128u);
                if (RotateTact(ANGLE(desired), static_cast<int>(elapsed())).value == 0u)
                    ChangeAnimation(2);
            }

            if (m_currentAnimation == 13) ChangeAnimation(0);
            const unsigned int mode = m_runtimeFlags&0x7Cu;
            if (mode == 0x04u || mode == 0x0Cu) return 0;

            const unsigned int now = ::CurrentTime;
            const unsigned int aligned = now-(now&0x7FFu);
            if (aligned>m_lastAnimationTick) m_robotFlagC4 = 1u;

            if (m_robotFlagC4 != 0u)
            {
                refreshReference();
                chooseState();
            }

            if (m_robotState<0 || m_robotState>14) return 0;
            return runState(m_robotState);
        }

    default:
            return CREATURE::Action(command, argument1, argument2, argument3);
    }
}


