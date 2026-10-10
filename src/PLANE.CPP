#include "mylib.h"
#include "engine_globals.h"


#include "plane.h"
#include "map.h"
#include "vid.h"
#include "basetype.h"
#include <new>


PLANE::PLANE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : UNIT(vid, position, direction, parent)
{
    StartMove();
    m_zSpeed = m_vid->maxZSpeed;
}


PLANE::~PLANE()
{
}


int PLANE::Action(int command, int argument1, int argument2, int argument3)
{
    if (command == ACT_NEXT_COMMAND)
    {
        PlaneNextCommand(argument1, argument2);
        return 0;
    }
    if (command == ACT_DAMAGE)
        return TERRAIN::Action(command, argument1, argument2, argument3);
    if (command != ACT_MOVE)
        return UNIT::Action(command, argument1, argument2, argument3);

    const VECTOR2 targetPoint(static_cast<float>(argument2), static_cast<float>(argument1));
    const float groundHeight = Map->GetGroundZ(m_vid, targetPoint, m_direction);
    const float z = groundHeight+(80.0f);
    Move(static_cast<float>(argument2), static_cast<float>(argument1), z);
    return 0;
}


void PLANE::FreeFlight()
{
    const int state = m_currentAnimation;
    if (state == 4 || state == 5)
    {
        const unsigned int branch = static_cast<unsigned int>(SPRITE::Random(2));
        if (branch != 0u)
        {
            unsigned char direction = m_direction.value;
            direction = static_cast<unsigned char>(state == 4?direction-32u:direction+32u);
            ChangeDirection(ANGLE(direction));
        }
        else ChangeAnimation(2);
    }
    else if (state == 2)
    {
        const unsigned int stay = static_cast<unsigned int>(SPRITE::Random(10));
        if (stay == 0u)
        {
            const unsigned int side = static_cast<unsigned int>(SPRITE::Random(1));
            ChangeAnimation(side?4:5);
        }
    }
    else ChangeAnimation(2);

    const unsigned int seek = static_cast<unsigned int>(SPRITE::Random(20));
    if (seek == 0u && (m_behave&1u) != 0u)
    {
        SPRITE* target = SeekEnemy();
        if (target) SetCommand(4, target);
    }
}


void PLANE::FlightToTarget()
{
    SPRITE* linked = m_goalSprite;
    const VECTOR& target = linked->Coor();
    const float dx = target.x-m_xyz.x;
    const float dy = target.y-m_xyz.y;
    const unsigned char direction = Decart2Polar(dx, dy).value;
    const unsigned int elapsed = ::CurrentTime-m_lastAnimationTick;
    if (RotateTact(ANGLE(direction), static_cast<int>(elapsed)).value == 0u) ChangeAnimation(2);
    FlightToTargetAdditionalActions();
}


void PLANE::ZSpeedInitialization()
{
    const VECTOR2 groundPoint(m_xyz.x, m_xyz.y);
    const float desiredZ = Map->GetGroundZ(m_vid, groundPoint, m_direction)+m_vid->topZ;
    const float eps = (10.0f);
    const float speed = m_vid->maxZSpeed;

    if (desiredZ-eps>m_xyz.z)
    {
        m_zSpeed = speed;
        return;
    }
    if (!(desiredZ+eps >= m_xyz.z))
    {
        m_zSpeed = -speed;
        return;
    }
    m_zSpeed = 0.0f;
}


void PLANE::CheckFlightProperties()
{
}


void PLANE::FlightToTargetAdditionalActions()
{
}


int PLANE::WayBlocked()
{
    const ANGLE direction(m_direction.value);
    const float scale = (128.0f);
    VECTOR point;
    point.x = direction.Sin()*scale+m_xyz.x;
    point.y = m_xyz.y-direction.Cos()*scale;
    point.z = m_xyz.z;
    return CanPlace(point) != 0?1:0;
}


void PLANE::FlightIfWayBlocked()
{
    const ANGLE direction(static_cast<unsigned char>(m_direction.value+16u));
    const float dx = direction.Sin()*(128.0f);
    const float dy = direction.Cos()*(128.0f);
    VECTOR point(m_xyz.x+dx, m_xyz.y-dy, m_xyz.z);
    const bool blocked = CanPlace(point) != 0;
    ChangeAnimation(blocked?5:4);
    unsigned char nextDirection = m_direction.value;
    nextDirection = static_cast<unsigned char>(blocked?nextDirection+16u:nextDirection-16u);
    ChangeDirection(ANGLE(nextDirection));
}


void PLANE::PlaneNextCommand(int command, int argument)
{
    (void) command;
    (void) argument;
    const int state = m_currentAnimation;
    if (state >= 15 || state == 12) return;
    if (state >= 7 && state != 10) ChangeAnimation(0);

    ZSpeedInitialization();
    CheckFlightProperties();
    if (m_childBacklink != 0) return;

    if ((m_runtimeFlags&0x400u) == 0u) m_runtimeFlags |= 0x400u;
    if (m_goalSprite != 0) FlightToTargetAdditionalActions();
    else FreeFlight();
}

