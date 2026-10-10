#include "myerror.h"
#include "engine_globals.h"
#include "unit.h"
#include "sprite.h"
#include "sprite_collector.h"
#include "region.h"
#include "resread.h"
#include "vid.h"
#include "list.h"
#include "map.h"
#include "stream.h"
#include "dialogSrv.h"
#include "mylib.h"
#include <new>
#include "basetype.h"


void SPRITE::Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const
{
    const unsigned int vidId = m_vid?static_cast<unsigned int>(m_vid->nVid):0xFFFFFFFFu;
    const STRING objectFormat("SPRITE [%i](%i,%i,%i)");
    const int positionZ = TruncateFloatToInt32(m_xyz.z);
    const int positionY = TruncateFloatToInt32(m_xyz.y);
    const int positionX = TruncateFloatToInt32(m_xyz.x);
    (void)::Error->Error(objectFormat, type, subject, code, vidId, positionX, positionY, positionZ);
}


UNIT::UNIT(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : TERRAIN(vid, position, direction, parent), DIALOG_UNIT()
{
    m_behaviorFlags &= ~3u;
    m_legacyState9C = -1;
    m_turnTimer = 0;

    m_lastCommandStopTick = 0;
    m_ammoFixedPoint = static_cast<int>(static_cast<unsigned int>(m_vid->GetMaxAmmo())<<6u);
    m_turnState = 0;
    m_moveStartTick = 0;
}


UNIT::~UNIT()
{
    Map->DeletePointerToSprite(this);
}

SPRITE* UNIT::Link() const
{
    return SPRITE::Link();
}

void UNIT::Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const
{
    SPRITE::Error(type, subject, code);
}

const VECTOR& UNIT::Coor() const
{
    return SPRITE::Coor();
}

int UNIT::AddLinkToLast(SPRITE* sprite)
{
    return SPRITE::AddLink(sprite);
}

void UNIT::PlaySFX(int sfxId) const
{
    SPRITE::PlaySFX(sfxId);
}


void UNIT::DetectTact()
{
    if (m_currentAnimation >= 14)
        return;
    if (m_currentAnimation == 8 && m_goalSprite)
        return;

    const int detectScript = m_vid->scriptFunction[20];
    if (detectScript >= 0 && Map->ScriptRun(detectScript, this, 0, 0) != 0)
        return;
    if ((m_behave&1u) == 0u)
        return;

    const SPRITE* const linkedSprite = m_childChain;
    if (linkedSprite && linkedSprite->Vid() == m_vid->linkVid)
    {
        const VID* const linkedVid = linkedSprite->Vid();
        if (linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0 &&
            ::CurrentTime>static_cast<unsigned int>(m_lastCommandStopTick)+3000u)
        {
            if (Link()->Animation() >= 14)
                return;
            if (Link()->Animation() == 8 && Link()->Goal())
                return;

            SPRITE* enemyTarget = 0;
            if ((Link()->RuntimeFlags()&0x7Cu) == 0x10u ||
                (Link()->RuntimeFlags()&0x7Cu) == 0x0Cu)
            {
                const VECTOR& goalPosition = Link()->Goal()->Coor();
                const VECTOR2 targetDelta(goalPosition.x-m_xyz.x, goalPosition.y-m_xyz.y);
                const float targetDistance = targetDelta.NearLength();
                if ((Link()->RuntimeFlags()&0x380u) == 0x100u ||
                    targetDistance>Link()->Vid()->weapon->BattleRange ||
                    !(targetDistance >= Link()->Vid()->weapon->DeadZone))
                    enemyTarget = SeekEnemy();
            }
            else if ((Link()->RuntimeFlags()&0x7Cu) != 0x1Cu)
            {
                enemyTarget = SeekEnemy();
            }
            if (enemyTarget)
                Link()->SetCommand(4, enemyTarget);
        }
    }

    if (m_vid->nWeapon == 0 || m_vid->childVid[8] == 0 ||
        ::CurrentTime <= static_cast<unsigned int>(m_lastCommandStopTick)+3000u)
        return;

    const unsigned int movementCommand = m_runtimeFlags&0x7Cu;
    if (movementCommand == 0u || movementCommand == 0x48u)
    {
        SPRITE* const enemyTarget = SeekEnemy();
        if (enemyTarget)
            SetCommand(4, enemyTarget);
        return;
    }

    if (movementCommand == 4u || movementCommand == 0x20u)
    {
        SPRITE* const enemyTarget = SeekEnemy();
        if (!enemyTarget)
            return;

        const unsigned int resumedCommand = m_runtimeFlags&0x7Cu;
        if (resumedCommand == 4u)
        {
            const VECTOR& targetPosition = m_goalSprite->Coor();
            const int targetZ = TruncateFloatToInt32(targetPosition.z);
            const int targetY = TruncateFloatToInt32(targetPosition.y);
            const int targetX = TruncateFloatToInt32(targetPosition.x);
            AddActionImmediate(0, ACT_MOVE, targetX, targetY, targetZ);
        }
        else if (resumedCommand == 0x20u)
        {
            AddActionImmediate(0, ACT_FOLLOW, static_cast<int>(PtrToU32(m_goalSprite)), 0, 0);
        }
        Stop();
        SetCommand(4, enemyTarget);
        return;
    }

    if (movementCommand == 0x10u)
    {
        const VECTOR& goalPosition = m_goalSprite->Coor();
        const double deltaX = static_cast<double>(goalPosition.x)-m_xyz.x;
        const float deltaY = goalPosition.y-m_xyz.y;
        const double absoluteX = fabs(deltaX);
        const double absoluteY = fabs(static_cast<double>(deltaY));
        const float diagonalScale = 0.398000002f;
        const double targetDistance = absoluteX>absoluteY
            ? absoluteX+absoluteY*diagonalScale
            : absoluteY+absoluteX*diagonalScale;
        const WEAPON_RUNTIME* const weapon = m_vid->weapon;
        if ((m_runtimeFlags&0x380u) == 0x100u ||
            targetDistance>weapon->BattleRange || !(targetDistance >= weapon->DeadZone))
        {
            SPRITE* const enemyTarget = SeekEnemy();
            if (enemyTarget)
                SetCommand(4, enemyTarget);
        }
    }
}


static unsigned char UnitCircularDirectionDifference(unsigned char leftDirection, unsigned char rightDirection)
{
    const unsigned char forwardDifference = static_cast<unsigned char>(leftDirection-rightDirection);
    const unsigned char backwardDifference = static_cast<unsigned char>(rightDirection-leftDirection);
    return forwardDifference<backwardDifference?forwardDifference:backwardDifference;
}

static float UnitAbs(float value)
{
    return value<0.0f?-value:value;
}

static bool UnitFloatEqualOrUnordered(float left, float right)
{
    return left == right || left != left || right != right;
}

static bool UnitFloatLessOrUnordered(float left, float right)
{
    return !(left >= right);
}

static unsigned int UnitWeaponFlags(const VID* weaponVid)
{
    return weaponVid->weapon->Property;
}

static bool UnitHasCompleteWeapon(const VID* weaponVid)
{
    return weaponVid->nWeapon != 0 && weaponVid->childVid[8] != 0;
}

static bool UnitCanRotateAsLinkedSprite(const SPRITE* linkedSprite)
{
    const VID* const linkedVid = linkedSprite->Vid();
    if (UnitFloatEqualOrUnordered(linkedVid->rotationSpeed, 0.0f))
        return false;
    if (UnitHasCompleteWeapon(linkedVid))
        return false;
    return linkedVid->spriteClass != 12u;
}

static SPRITE* UnitSelectLinkedRotationSprite(SPRITE* linkedSprite)
{
    SPRITE* const nestedSprite = linkedSprite->ChildChain();
    if (nestedSprite && nestedSprite->Vid() == linkedSprite->Vid()->linkVid &&
        UnitCanRotateAsLinkedSprite(nestedSprite))
        return nestedSprite;

    return UnitCanRotateAsLinkedSprite(linkedSprite)?linkedSprite:0;
}

static SPRITE* UnitRuntimePlayerZero()
{
    MAP* const runtimeMap = ::Map;
    return runtimeMap->Flagman(0);
}

void UNIT::MoveTact()
{
    DialogTact();

    if (m_vid->spriteClass != 2u && m_vid->spriteClass != 4u)
    {
        SPRITE::MoveTact();
        return;
    }

    const unsigned int elapsedTicks =
        ::CurrentTime-::PrevCurrentTime;

    SPRITE* linkedSprite = Link();
    if (linkedSprite && linkedSprite->Vid() == m_vid->linkVid &&
        ((UnitWeaponFlags(m_vid)&0x2000u) != 0u ||
        (UnitWeaponFlags(linkedSprite->Vid())&0x2000u) != 0u))
    {
        SPRITE* const rotationSprite = UnitSelectLinkedRotationSprite(linkedSprite);
        if (rotationSprite)
        {
            SPRITE* const playerSprite = UnitRuntimePlayerZero();
            if (playerSprite)
            {
                SPRITE* const parentSprite = rotationSprite->ChildBacklink();
                bool usePlayerDirection = false;
                const VID* const parentVid = parentSprite->Vid();
                const unsigned int parentCommand = parentSprite->RuntimeFlags()&0x7Cu;

                const bool parentCommitted =
                    UnitHasCompleteWeapon(parentVid) &&
                parentSprite->StateTimer() != 0u &&
                parentCommand != 0x48u;

                if (!parentCommitted)
                {
                    const VECTOR2 toPlayer(
                        playerSprite->X()-m_xyz.x,
                        playerSprite->Y()-m_xyz.y);
                    if (toPlayer.NearLength()<300.0f)
                    {
                        const ANGLE playerDirection(toPlayer);
                        if (UnitCircularDirectionDifference(
                            parentSprite->Direction().value,
                            playerDirection.value)<42u)
                        {
                            (void) rotationSprite->RotateTact(playerDirection, static_cast<int>(elapsedTicks));
                            usePlayerDirection = true;
                        }
                    }
                }

                if (!usePlayerDirection)
                {
                    (void) rotationSprite->RotateTact(parentSprite->Direction(), static_cast<int>(elapsedTicks));
                }
            }
        }
    }

    if (m_turnTimer != 0)
    {
        const unsigned char previousDirection = m_direction.value;
        const ANGLE desiredDirection(static_cast<unsigned char>(
            Direction().value+static_cast<unsigned char>(m_turnTimer)));
        (void) RotateTact(desiredDirection, static_cast<int>(elapsedTicks));

        const unsigned char turnedAngle =
            UnitCircularDirectionDifference(previousDirection, m_direction.value);
        if (m_turnTimer>0)
            m_turnTimer -= static_cast<int>(turnedAngle);
        else
            m_turnTimer += static_cast<int>(turnedAngle);
    }
    else if (m_goalSprite && !UnitFloatEqualOrUnordered(m_speed, 0.0f))
    {
        ANGLE desiredDirection = DirectionTo(m_goalSprite);
        if (UnitFloatLessOrUnordered(m_speed, 0.0f))
            desiredDirection.value = static_cast<unsigned char>(desiredDirection.value+128u);
        RotateForGlide(desiredDirection, static_cast<int>(elapsedTicks));

        if ((m_runtimeFlags&0x03000000u) == 0x03000000u &&
            (m_runtimeFlags&0x7Cu) != 0x20u)
        {
            Stop();
            for (int actionLaneIndex = 0; actionLaneIndex<4; ++actionLaneIndex)
            {
                if (!m_extraData)
                    continue;
                const LIST<ACT>& actionLane = m_extraData->actionLane[actionLaneIndex];
                if (actionLane.m_count == 0)
                    continue;
                if (actionLane.m_data[actionLane.m_count-1].command == ACT_STOP_STACK)
                    continue;
                const LIST<ACT>& currentLane = ActionStack(actionLaneIndex);
                if (currentLane.m_data[currentLane.m_count-1].command == ACT_MOVE)
                {
                    m_runtimeFlags |= 0x00000400u;
                }
            }
        }
    }
    else
    {
        linkedSprite = Link();
        if (linkedSprite && linkedSprite->Goal() && !UnitFloatEqualOrUnordered(m_speed, 0.0f))
        {
            const unsigned int movementCommand = m_runtimeFlags&0x7Cu;
            if (movementCommand != 4u && movementCommand != 0x20u)
            {
                unsigned char desiredDirectionByte = DirectionTo(linkedSprite->Goal()).value;
                if (UnitFloatLessOrUnordered(m_speed, 0.0f))
                    desiredDirectionByte = static_cast<unsigned char>(desiredDirectionByte+128u);
                RotateForGlide(ANGLE(desiredDirectionByte), static_cast<int>(elapsedTicks));
            }
        }
        else
        {
            const unsigned int movementCommand = m_runtimeFlags&0x7Cu;
            if ((movementCommand == 0x48u || (movementCommand == 0u && m_stateTimer == 0u)) &&
                m_turnState == 0)
            {
                const int vidDirectionIndex = m_vid->RealDirection(m_direction.value);
                const unsigned char quantizedDirection = static_cast<unsigned char>(
                    (vidDirectionIndex<<8)/m_vid->noDir);
                if (quantizedDirection != m_direction.value)
                {
                    (void) RotateTact(ANGLE(quantizedDirection), static_cast<int>(elapsedTicks));
                }
            }
        }
    }

    VECTOR nextPosition = m_xyz;
    MoveTactCalcCoor(&nextPosition);

    if (m_moveStartTick != 0)
    {
        if ((m_runtimeFlags&0x7Cu) == 4u && m_goalSprite)
        {
            const unsigned int currentTick = ::CurrentTime;
            if ((currentTick&1u) != 0u &&
                currentTick-static_cast<unsigned int>(m_moveStartTick)>15000u)
            {
                AddActionImmediate(
                    0, ACT_MOVE,
                    TruncateFloatToInt32(m_goalSprite->X()),
                    TruncateFloatToInt32(m_goalSprite->Y()),
                    TruncateFloatToInt32(m_goalSprite->Coor().z));
                m_runtimeFlags &= ~0x10000000u;
                m_stateTimer = 7500u;
                SetCommand(0x12, static_cast<SPRITE*>(0));
            }
        }
        else
        {
            m_moveStartTick = 0;
        }
    }

    const VECTOR& currentPosition = Coor();
    const bool positionChanged =
        !UnitFloatEqualOrUnordered(nextPosition.x, currentPosition.x) ||
    !UnitFloatEqualOrUnordered(nextPosition.y, currentPosition.y) ||
    !UnitFloatEqualOrUnordered(nextPosition.z, currentPosition.z);

    if (positionChanged)
    {
        if (CanPlaceWithCrushAndGlide(&nextPosition) == 0)
        {
            (void) MoveTactMapLimit(nextPosition);
            ChangeCoor(nextPosition);
        }
        else if ((m_behave&0x20u) != 0u && m_turnTimer != 0)
        {
            Stop();
            m_turnTimer = 0;
        }
        else if (m_goalSprite && (m_behaviorFlags&3u) == 0u)
        {
            const ANGLE desiredDirection(VECTOR2(
                m_goalSprite->X()-m_xyz.x,
                m_goalSprite->Y()-m_xyz.y));
            if (desiredDirection.value>m_direction.value)
                m_behaviorFlags |= 2u;
            else if (desiredDirection.value<m_direction.value)
                m_behaviorFlags |= 1u;
            else
            {
                const bool useLocalRandom = (m_vid->runtimeFlags&0x1000u) != 0u;
                const int turnChoice = useLocalRandom? ::Random(1) : ::Random2(1);
                m_behaviorFlags |= (turnChoice == 0)?2u:1u;
            }
        }
        else if (m_turnTimer == 0 && !UnitFloatEqualOrUnordered(m_speed, 0.0f))
        {
            bool suppressObstacleTurn = false;
            if ((m_runtimeFlags&0x2000u) != 0u)
            {
                suppressObstacleTurn = true;
                if (m_goalSprite)
                {
                    const double absoluteGoalDeltaX = ::fabs(static_cast<double>(m_xyz.x)-static_cast<double>(m_goalSprite->X()));
                    const float absoluteGoalDeltaY = UnitAbs(m_xyz.y-m_goalSprite->Y());
                    suppressObstacleTurn = (!(absoluteGoalDeltaX >= 40.0) &&
                        UnitFloatLessOrUnordered(absoluteGoalDeltaY, 40.0f));
                }
            }
            if (!suppressObstacleTurn)
                m_turnTimer = (m_behaviorFlags&1u) != 0u?75:-75;
        }
    }

    linkedSprite = Link();
    if (!linkedSprite)
        return;

    if ((UnitWeaponFlags(linkedSprite->Vid())&0x800u) == 0u ||
        m_vid->noAnimCadr[6] == 0)
        return;

    if (!UnitFloatEqualOrUnordered(m_speed, 0.0f) || (m_runtimeFlags&0x400u) != 0u)
    {
        const unsigned char directionDifference = UnitCircularDirectionDifference(
            m_direction.value, linkedSprite->Direction().value);
        if (directionDifference>64u)
        {
            if (m_currentAnimation == 0 ||
                m_currentAnimation == 2 ||
                m_currentAnimation == 3)
            {
                ChangeAnimation(6);
                return;
            }
            return;
        }
        if ((m_runtimeFlags&0x400u) != 0u)
            return;
    }

    if (m_currentAnimation == 6)
    {
        if (UnitAbs(MaxSpeed())*0.25f>UnitAbs(m_speed))
        {
            ChangeAnimation(0);
            ChangeDirection(ANGLE(static_cast<unsigned char>(
                Direction().value+128u)));
        }
    }

    if (!UnitFloatEqualOrUnordered(m_speed, 0.0f))
        return;

    if (m_turnState != 0)
    {
        const ANGLE directionDifference = RotateTact(linkedSprite->Direction(), static_cast<int>(elapsedTicks));
        if (directionDifference.value<16u)
            m_turnState = 0;
        return;
    }

    m_turnState =
        UnitCircularDirectionDifference(
        m_direction.value, linkedSprite->Direction().value)>55u?1:0;
}

int UNIT::Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter)
{
    switch (actionCode)
    {
    case ACT_GET_DIALOG:
            return GetDialog();

    case ACT_SET_DIALOG:
            SetDialog(::g_pDlgSrv->GetDlgKey(firstParameter, secondParameter));
        return 0;

    case ACT_MOVE:
        {
            m_moveStartTick = static_cast<int>(::CurrentTime);
            void* const spriteStorage = CrtNew(sizeof(SPRITE));
            SPRITE* moveTarget = 0;
            try
            {
                if (spriteStorage) moveTarget = new (spriteStorage) SPRITE(VECTOR(firstParameter, secondParameter, thirdParameter), ANGLE(0));
            }
            catch (...)
            {
                CrtFreePointer(spriteStorage);
                throw;
            }
            Move(moveTarget);
            return 0;
        }

    case ACT_ADD_AMMO:
        {
            const int ammoCapacity = m_vid->GetMaxAmmo();
            if (ammoCapacity == 999999)
            {
                m_ammoFixedPoint = 999999<<6;
                return ammoCapacity;
            }
            const int actionResult = BitsToSigned32(static_cast<unsigned int>(m_ammoFixedPoint)+
                (static_cast<unsigned int>(firstParameter)<<6u));
            m_ammoFixedPoint = actionResult;
            if (actionResult<0)
                m_ammoFixedPoint = 0;

            return actionResult;
        }

    case ACT_GET_AMMO:

            return m_ammoFixedPoint/64;

    case ACT_NEXT_COMMAND:
        {
            if (m_currentAnimation >= 15)
                return 0;

            SetMoveAnimation();

            unsigned int commandModeFlags = m_runtimeFlags;
            const bool hasOwnWeapon = (m_vid->nWeapon != 0 && m_vid->childVid[8] != 0);
            if (!hasOwnWeapon && m_childChain && m_childChain->Vid() == m_vid->linkVid)
            {
                VID* const linkedVid = m_childChain->Vid();
                if (linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0)
                {
                    commandModeFlags = Link()->RuntimeFlags();
                }
            }
            const int commandMode = static_cast<int>((commandModeFlags>>7u)&7u);
            unsigned int movementCommand = m_runtimeFlags&0x7Cu;

            const bool movementCommandHandled =
                (movementCommand == 0x28u || movementCommand == 0x20u) && m_goalSprite;

            if (movementCommand == 0x28u && m_goalSprite)
            {
                const VECTOR2 targetDelta(m_goalSprite->X()-m_xyz.x, m_goalSprite->Y()-m_xyz.y);
                const float targetDistance = targetDelta.NearLength();
                if (targetDistance>10.0f)
                {
                    if (UnitFloatEqualOrUnordered(m_speed, 0.0f))
                        StartMove();
                }
                else if (!UnitFloatEqualOrUnordered(m_speed, 0.0f) && (m_runtimeFlags&0x400u) != 0u)
                {
                    Stop();
                    m_turnTimer = 0;
                }
            }

            else if (movementCommand == ACT_ATTACK && m_goalSprite)
            {
                const VECTOR2 targetDelta(m_goalSprite->X()-m_xyz.x, m_goalSprite->Y()-m_xyz.y);
                const float targetDistance = targetDelta.NearLength();
                if (targetDistance>150.0f && targetDistance<1000.0f)
                {
                    if (UnitFloatEqualOrUnordered(m_speed, 0.0f))
                        StartMove();
                }
                else if (!UnitFloatEqualOrUnordered(m_speed, 0.0f) && (m_runtimeFlags&0x400u) != 0u)
                {
                    Stop();
                    m_turnTimer = 0;
                }
            }

            bool skipCommandTransition = movementCommandHandled;
            if (!skipCommandTransition && commandMode == 4 && !UnitFloatEqualOrUnordered(m_speed, 0.0f))
            {
                skipCommandTransition = true;
                if (m_childChain && m_childChain->Vid() == m_vid->linkVid)
                {
                    VID* const linkedVid = m_childChain->Vid();
                    if (linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0)
                        {
                        SPRITE* const ownGoal = m_goalSprite;
                        skipCommandTransition = (ownGoal == Link()->Goal());
                    }
                }
            }

            if (!skipCommandTransition)
            {
                movementCommand = m_runtimeFlags&0x7Cu;
                const bool persistentCommand = (movementCommand == 0x0Cu || movementCommand == 0x10u || movementCommand == 0x14u);

                if (!persistentCommand && m_childChain && m_childChain->Vid() == m_vid->linkVid)
                {
                    VID* const linkedVid = m_childChain->Vid();
                    if (linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0)
                    {
                        if ((Link()->RuntimeFlags()&0x380u) == 0x180u && (m_runtimeFlags&0x7Cu) == 0x10u)
                        {
                            m_lastCommandStopTick = static_cast<int>(::CurrentTime);
                            Link()->SetCommand(0, static_cast<SPRITE*>(0));
                        }
                        else if (commandMode == 2 || commandMode == 3)
                        {
                            const bool linkedContinue = (m_behave&2u) != 0u ||
                            ((Link()->RuntimeFlags()&0x7Cu) == 0x0Cu);
                            if (!linkedContinue)
                            {
                                if (commandMode == 3)
                                {
                                    m_turnTimer = 0;
                                    Link()->SetCommand(0, static_cast<SPRITE*>(0));
                                }
                            }
                            else if (UnitFloatEqualOrUnordered(m_speed, 0.0f) &&
                                !UnitFloatEqualOrUnordered(MaxSpeed(), 0.0f))
                            {
                                StartMove();
                            }
                        }
                    }
                }
                else if (persistentCommand)
                {
                    if (commandMode == 3 && movementCommand == 0x10u)
                    {
                        m_turnTimer = 0;
                        m_lastCommandStopTick = static_cast<int>(::CurrentTime);
                        SetCommand(0, static_cast<SPRITE*>(0));
                    }
                    else if (commandMode == 2 || commandMode == 3)
                    {
                        if ((m_behave&2u) != 0u || movementCommand == 0x0Cu)
                        {
                            if (UnitFloatEqualOrUnordered(m_speed, 0.0f) &&
                                !UnitFloatEqualOrUnordered(MaxSpeed(), 0.0f))
                                StartMove();
                        }
                        else if (commandMode == 3)
                        {
                            m_turnTimer = 0;
                            SetCommand(0, static_cast<SPRITE*>(0));
                        }
                    }
                }
            }

            movementCommand = m_runtimeFlags&0x7Cu;
            if ((movementCommand == 0x0Cu || movementCommand == 0x10u) &&
                m_childChain && m_childChain->Vid() == m_vid->linkVid)
            {
                VID* const linkedVid = m_childChain->Vid();
                if (linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0 && m_goalSprite)
                {
                    if (!Link()->Goal())
                    {
                        SPRITE* const goal = m_goalSprite;
                        const int command = static_cast<int>((m_runtimeFlags>>2u)&0x1Fu);
                        Link()->SetCommand(command, goal);
                    }
                }
            }

            movementCommand = m_runtimeFlags&0x7Cu;
            if (movementCommand != 0u || m_goalSprite)
                return 0;

            if (Link() && (((Link()->RuntimeFlags()&0x7Cu) != 0u) || Link()->Goal() != 0))
                return 0;

            if ((m_behave&0x20u) == 0u)
            {
                Stop();
                return 0;
            }

            if (Map->Flagman(0))
            {
                SPRITE* const playerSprite = Map->Flagman(0);
                const VECTOR2 playerDelta(playerSprite->X()-m_xyz.x, playerSprite->Y()-m_xyz.y);
                if (UnitFloatLessOrUnordered(playerDelta.NearLength(), 200.0f))
                {
                    SPRITE* const currentPlayerSprite = Map->Flagman(0);
                    const VECTOR2 currentPlayerDelta(currentPlayerSprite->X()-m_xyz.x,
                        currentPlayerSprite->Y()-m_xyz.y);
                    ANGLE awayDirection(currentPlayerDelta);
                    awayDirection.value = static_cast<unsigned char>(awayDirection.value+128u);
                    const float horizontalOffset = awayDirection.Sin()*32.0f;
                    const VECTOR2 position(m_xyz.x+horizontalOffset,
                        static_cast<float>(static_cast<double>(m_xyz.y)-
                            static_cast<double>(awayDirection.Cos())*32.0f));
                    const float height = Map->GetGroundZ(m_vid, position, awayDirection);
                    const VECTOR destination(position.x, position.y, height);
                    void* const spriteStorage = CrtNew(sizeof(SPRITE));
                    SPRITE* moveTarget = 0;
                    try
                    {
                        if (spriteStorage) moveTarget = new (spriteStorage) SPRITE(destination, ANGLE(0));
                    }
                    catch (...)
                    {
                        CrtFreePointer(spriteStorage);
                        throw;
                    }
                    Move(moveTarget);
                    return 0;
                }
            }

            const int idleChoice = Random(2);
            if (idleChoice == 0)
            {
                Stop();
                const int idleDelay = Random(1500);
                m_runtimeFlags &= ~0x10000000u;
                if (idleDelay == 0 && (m_runtimeFlags&0x7Cu) != 0x48u)
                    return 0;
                m_stateTimer = idleDelay;
                SetCommand(idleDelay?0x12:0, static_cast<SPRITE*>(0));
                return 0;
            }

            const int directionChoice = Random(255);
            const ANGLE direction(static_cast<unsigned char>(directionChoice));
            const float horizontalOffset = direction.Sin()*45.0f;
            const VECTOR2 position(m_xyz.x+horizontalOffset,
                static_cast<float>(static_cast<double>(m_xyz.y)-
                    static_cast<double>(direction.Cos())*45.0f));
            const float height = Map->GetGroundZ(m_vid, position, direction);
            const VECTOR destination(position.x, position.y, height);
            void* const spriteStorage = CrtNew(sizeof(SPRITE));
            SPRITE* moveTarget = 0;
            try
            {
                if (spriteStorage) moveTarget = new (spriteStorage) SPRITE(destination, ANGLE(0));
            }
            catch (...)
            {
                CrtFreePointer(spriteStorage);
                throw;
            }
            Move(moveTarget);
            return 0;
        }

    case ACT_DAMAGE:
        {
            const int actionResult = TERRAIN::Action(actionCode, firstParameter, secondParameter, thirdParameter);
            if (firstParameter<0 || (m_runtimeFlags&0x7Cu) != 0u || m_currentAnimation>2 ||
                (m_behave&2u) == 0u || UnitFloatEqualOrUnordered(MaxSpeed(), 0.0f))
                return actionResult;

            for (int attempt = 2; attempt >= 0; --attempt)
            {
                const int directionChoice = Random(255);
                const unsigned int directionIndex = static_cast<unsigned int>(directionChoice)&0xFFu;
                VECTOR destination(
                    m_xyz.x+ANGLE(static_cast<unsigned char>(directionIndex)).Sin()*64.0f,
                    m_xyz.y-ANGLE(static_cast<unsigned char>(directionIndex)).Cos()*64.0f,
                    m_xyz.z);
                if (CanPlace(destination) == 0)
                {
                    destination.z = m_xyz.z;
                    void* const spriteStorage = CrtNew(sizeof(SPRITE));
                    SPRITE* moveTarget = 0;
                    try
                    {
                        if (spriteStorage) moveTarget = new (spriteStorage) SPRITE(destination, ANGLE(0));
                    }
                    catch (...)
                    {
                        CrtFreePointer(spriteStorage);
                        throw;
                    }
                    Move(moveTarget);
                    break;
                }
            }
            return actionResult;
        }

    case ACT_REPAIR:
        {
            m_ammoFixedPoint = BitsToSigned32(static_cast<unsigned int>(m_vid->GetMaxAmmo())<<6u);
            return TERRAIN::Action(actionCode, firstParameter, secondParameter, thirdParameter);
        }

    case ACT_RESTORE:
        {
            (void) TERRAIN::Action(actionCode, firstParameter, secondParameter, thirdParameter);
            STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(firstParameter)));
            const int saveVersion = secondParameter;
            if (saveVersion<19)
                (void) stream->Read(&m_behave, 4u);

            if (saveVersion == 11)
            {
                int itemCount = 0;
                (void) stream->Read(&itemCount, 4u);
                if (itemCount>0)
                {
                    int* const itemIds = static_cast<int*>(CrtNew(static_cast<unsigned int>(itemCount)*4u));
                    if (!itemIds)
                    {
                        STRING allocationErrorFormat("!!!ERROR!!!::LIST: Not enough memory %i");
                            ::Error->LogExit(allocationErrorFormat, itemCount);
                    }
                    (void) stream->Read(itemIds, static_cast<unsigned int>(itemCount)*4u);
                    for (int itemIndex = 0; itemIndex<itemCount; ++itemIndex)
                        InsertItem(itemIds[itemIndex]);
                    CrtFreePointer(itemIds);
                }
            }
            else if (saveVersion<11)
            {
                int itemCount = 0;
                (void) stream->Read(&itemCount, 4u);
                if (itemCount>0)
                {
                    short* const itemIds = static_cast<short*>(CrtNew(static_cast<unsigned int>(itemCount)*2u));
                    if (!itemIds)
                    {
                        STRING allocationErrorFormat("!!!ERROR!!!::LIST: Not enough memory %i");
                            ::Error->LogExit(allocationErrorFormat, itemCount);
                    }
                    (void) stream->Read(itemIds, static_cast<unsigned int>(itemCount)*2u);
                    for (int itemIndex = 0; itemIndex<itemCount; ++itemIndex)
                        InsertItem(static_cast<int>(itemIds[itemIndex]));
                    CrtFreePointer(itemIds);
                }
            }
            return 0;
        }

    case ACT_LEGACY_RESTORE:
        {
            (void) TERRAIN::Action(actionCode, firstParameter, secondParameter, thirdParameter);
            STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(firstParameter)));
            if (secondParameter<7)
            {
                unsigned char loadedArmy = 0;
                (void) stream->Read(&loadedArmy, 1u);
                ChangeArmy(static_cast<int>(loadedArmy));
            }
            (void) stream->Read(&m_behave, 1u);
            return 0;
        }

    default:
            return TERRAIN::Action(actionCode, firstParameter, secondParameter, thirdParameter);
    }
}


void UNIT::DrawSecondaryInfo()
{
    SPRITE::DrawSecondaryInfo();
}


int UNIT::AddAmmoTick(int divisor)
{
    const int ammoCapacity = static_cast<int>(static_cast<unsigned int>(m_vid->GetMaxAmmo())<<6u);
    if (ammoCapacity == 0 || ammoCapacity <= m_ammoFixedPoint)
        return 0;

    if (divisor != 0)
    {
        int nextAmmo = BitsToSigned32(static_cast<unsigned int>(m_ammoFixedPoint)+
            static_cast<unsigned int>(ammoCapacity/divisor));
        if (nextAmmo>ammoCapacity) nextAmmo = ammoCapacity;
        m_ammoFixedPoint = nextAmmo;
    }
    else
    {
        m_ammoFixedPoint = ammoCapacity;
    }
    return 1;
}

void UNIT::AddHpPerSecond(int amount)
{
    if ((::CurrentTime&0xFFFFFC00u) <= ::PrevCurrentTime)
        return;
    if (m_hp <= 0)
        return;

    int nextHp = BitsToSigned32(static_cast<unsigned int>(m_hp)+static_cast<unsigned int>(amount));
    const unsigned int playerIndex = (m_runtimeFlags>>17u)&7u;
    const int maximumHp = m_vid->maxHpByPlayer[playerIndex];
    if (nextHp>maximumHp) nextHp = maximumHp;
    ChangeHp(nextHp);
}

template<>
LIST<short>::~LIST()
{
    if (m_data) CrtFreePointer(m_data);
    m_data = 0;
    m_count = 0;

}


ANGLE::ANGLE(const ANGLE& angleValue)
{
    value = angleValue.value;
}


const ANGLE ANGLE::operator+(const ANGLE& angleValue) const
{
    return ANGLE(static_cast<unsigned char>(value+angleValue.value));
}


float SPRITE::Z() const
{
    return m_xyz.z;
}


VECTOR::VECTOR(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue)
{
}


VECTOR2 VECTOR2::operator-(const VECTOR2& vectorValue) const
{
    return VECTOR2(x-vectorValue.x, y-vectorValue.y);
}

int SPRITE::HaveLink() const
{
    const SPRITE* const attached = m_childChain;
    if (!attached) return 0;
    return attached->m_vid == m_vid->linkVid?1:0;
}


float VECTOR2::NearLength() const
{
    const float absX = fabsf(x);
    const float absY = fabsf(y);
    const float diagonalScale = (0.398000002f);
    return absX>absY ? absX+absY*diagonalScale : absY+absX*diagonalScale;
}


ANGLE SPRITE::Direction() const
{
    return m_direction;
}



