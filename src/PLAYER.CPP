#include "net.h"
#include "engine_globals.h"
#include "basetype.h"
#include "myerror.h"
#include "player.h"
#include "sprite.h"
#include "vid.h"
#include "weapon_runtime.h"
#include "input.h"
#include "groups.h"
#include "map.h"
#include "sprite_list.h"
#include "mylib.h"
#include <new>

int g_iHeroIdForInvuln = 528;

namespace {
const unsigned int playerInitialState[16] = {
    1u, 2u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};
const int playerInitialRelations[4][4] = {
    {2, 1, 0, 0},
    {1, 2, 0, 1},
    {0, 0, 0, 0},
    {0, 1, 0, 2},
};
}

void PLAYER::PutMessage(const STRING&, float, float)
{
}

int PLAYER::GetQuestState(int, int)
{
    return 0;
}

bool PLAYER::SetQuestState(int, int, int)
{
    return true;
}

void PLAYER::InitForLoadMap()
{
}
void PLAYER::DrawSecondaryInfo()
{
}
void PLAYER::Control(INPUT*)
{
}
void PLAYER::StateBarOn()
{
}
void PLAYER::StateBarOff()
{
}
void PLAYER::AddUnitToStateBar(SPRITE*)
{
}

void* PLAYER::ScalarDeletingDestructor(unsigned int flags)
{
    void* self = this;
    this->PLAYER::~PLAYER();
    if (flags&1u) CrtFreePointer(self);
    return self;
}

void PLAYER::SetFlagman(SPRITE* flagman)
{
    if (flagman) ++flagman->m_referenceCount;
    if (m_flagman.m_value) m_flagman.m_value->Release();
    m_flagman.m_value = flagman;
    m_moveHistoryCount = 0u;
}

void PLAYER::DeletePointerToSprite(SPRITE* sprite)
{
    (void)m_spriteList.Delete(sprite);
    if (m_flagman.m_value == sprite)
    {
        if (m_flagman.m_value) m_flagman.m_value->Release();
        m_flagman.m_value = 0;
    }
    if (m_transientSprite.m_value == sprite)
    {
        if (m_transientSprite.m_value) m_transientSprite.m_value->Release();
        m_transientSprite.m_value = 0;
    }
}

PLAYER::PLAYER(unsigned int playerIndex)
    : m_playerIndex(playerIndex)
{
    if (m_flagman.m_value) m_flagman.m_value->Release();
    m_flagman.m_value = 0;
    if (m_transientSprite.m_value) m_transientSprite.m_value->Release();
    m_transientSprite.m_value = 0;
    m_stateF0 = playerInitialState[playerIndex];
    m_money = 1000;
    m_moveHistoryCount = 0u;
    m_moveRollback = 0u;
    m_moveHistoryTime = 0u;
    m_moveFallback = VECTOR(0.0f, 0.0f, 0.0f);
    m_cachedDirection = 0u;
    m_controlPauseState = 0u;
    m_controlMode = 2;
    for (unsigned int relationIndex = 0;relationIndex<8u;++relationIndex)
        m_relation[relationIndex] = (relationIndex<4u && playerIndex<4u)?playerInitialRelations[playerIndex][relationIndex]:0;
}


void PLAYER::Release()
{
    if (m_flagman.m_value) m_flagman.m_value->Release();
    m_flagman.m_value = 0;
    if (m_transientSprite.m_value) m_transientSprite.m_value->Release();
    m_transientSprite.m_value = 0;

    m_money = 1000;
    m_controlMode = 2;
    m_stateF0 = playerInitialState[m_playerIndex];
    m_moveFallback = VECTOR(0.0f, 0.0f, 0.0f);
    m_previousInput = VECTOR2(0.0f, 0.0f);
    m_controlPauseState = 0u;
    m_moveHistoryCount = 0u;
    m_moveRollback = 0u;
    m_moveHistoryTime = 0u;
    m_cachedDirection = 0u;
    m_targetX = 0.0f;
    m_targetY = 0.0f;
    m_targetTime = 0u;

    for (unsigned int relationIndex = 0;relationIndex<8u;++relationIndex)
        m_relation[relationIndex] = (relationIndex<4u && m_playerIndex<4u)?playerInitialRelations[m_playerIndex][relationIndex]:0;
}

void PLAYER::Save(STREAM* stream) const
{
    (void)stream->Write(&m_flagman.m_value, 4u);
}

void PLAYER::Load(STREAM* stream)
{
    SPRITE* loadedFlagman = Map->ReadPointer(stream);
    if (loadedFlagman) ++loadedFlagman->m_referenceCount;
    if (m_flagman.m_value) m_flagman.m_value->Release();
    m_flagman.m_value = loadedFlagman;
}


STRING PLAYER::GetMouseTipsString() const
{
    const char* emptyText = StringGlobals::EmptyText();
    if (!m_transientSprite.m_value) return STRING(emptyText);
    STRING fallbackText(emptyText);
    STRING sectionName("Units");
    VID* const spriteVid = m_transientSprite.m_value->m_vid;
    const int vidIndex = spriteVid->nVid;
    STRING vidKey = Int2Str(vidIndex);
    PROFILE* const profile = ::Strings;
    return profile->GetString(sectionName, vidKey, fallbackText);
}


PLAYER::~PLAYER() noexcept(false)
{
    Release();
}

static int PlayerAbsAction(int actionValue)
{


    const unsigned int actionBits = static_cast<unsigned int>(actionValue);
    const unsigned int signBit = actionBits>>31;
    const unsigned int signMask = 0u-signBit;
    return BitsToSigned32((actionBits^signMask)+signBit);
}

static bool PlayerSamePosition(const VECTOR& historyPosition, const SPRITE* actor)
{
    const VECTOR& actorPosition = actor->Coor();
    return FloatEqualOrUnordered(historyPosition.x, actorPosition.x) &&
           FloatEqualOrUnordered(historyPosition.y, actorPosition.y) &&
           FloatEqualOrUnordered(historyPosition.z, actorPosition.z);
}

// Compare before the final float store, as the coordinate-attack path does.
static void ClampFlagmanAttackHeight(const float& actorHeight, float& groundHeight)
{
    const double heightLimit = static_cast<double>(actorHeight) + 80.0;
    if (FloatLessOrUnordered(heightLimit, groundHeight))
        groundHeight = static_cast<float>(static_cast<double>(actorHeight) + 60.0);
}

void PLAYER::ControlFlagman(INPUT* input)
{
    const unsigned int controlMode = static_cast<unsigned int>(m_controlMode);
    if (controlMode == 0u) return;

    const VECTOR2& inputMouse = input->MouseXY();
    float inputX = inputMouse.x;
    float inputY = inputMouse.y;

    if (static_cast<unsigned int>(m_controlMode) != 3u)
    {
        if (input->GetAction(IA_MOVE_TO_COORD) != 0)
        {
            if (static_cast<unsigned int>(m_controlMode) == 4u)
            {
                const float aimOffsetX = inputX-m_flagman.m_value->m_xyz.x;
                const float aimOffsetY = inputY-m_flagman.m_value->m_xyz.y;
                const unsigned char aimDirection = Decart2Polar(aimOffsetX, aimOffsetY).value;
                const unsigned char spriteDirection = m_flagman.m_value->m_direction.value;
                const unsigned char clockwiseTurn = static_cast<unsigned char>(aimDirection-spriteDirection);
                const unsigned char counterclockwiseTurn = static_cast<unsigned char>(spriteDirection-aimDirection);
                const unsigned char turnDistance = clockwiseTurn<counterclockwiseTurn?clockwiseTurn:counterclockwiseTurn;
                unsigned int movementFlags = m_flagman.m_value->m_runtimeFlags;
                if (turnDistance>80u) movementFlags |= 0x00004000u;
                else movementFlags &= ~0x00004000u;
                m_flagman.m_value->m_runtimeFlags = movementFlags;
            }
            else
            {
                m_flagman.m_value->m_runtimeFlags &= ~0x00004000u;
            }

            const int requestedX = TruncateFloatToInt32(inputX);
            const int requestedY = TruncateDoubleToInt32(static_cast<double>(m_flagman.m_value->m_xyz.z)+static_cast<double>(inputY));
            const int requestedZ = TruncateFloatToInt32(m_flagman.m_value->m_xyz.z);
            (void)m_flagman.m_value->Action(ACT_MOVE, requestedX, requestedY, requestedZ);
            m_targetX = m_flagman.m_value->m_xyz.x;
            m_targetY = m_flagman.m_value->m_xyz.y;
            m_targetTime = ::CurrentTime;
        }

        if (input->GetAction(IA_MOVE_UP) != 0 || input->GetAction(IA_MOVE_DOWN) != 0 ||
           input->GetAction(IA_MOVE_RIGHT) != 0 || input->GetAction(IA_MOVE_LEFT) != 0)
        {
            if ((m_flagman.m_value->m_runtimeFlags&0x7Cu) == 4u)
                (void)m_flagman.m_value->SetCommandWithoutLink(0, 0);
        }
    }

    const VECTOR2& mouseXY = input->MouseXY();
    VECTOR2 groundProbe = {mouseXY.x, mouseXY.y};
    float probeHeight = Map->GetGroundZScr(groundProbe);
    inputX = input->MouseXY().x;
    inputY = input->MouseXY().y;
    float moveX = inputX;
    float moveY = inputY;

    NET* const network = ::Net;
    if ((network->m_flags&0x10u) != 0u)
    {
        moveX = inputX+inputX-m_previousInput.x;
        moveY = inputY+inputY-m_previousInput.y;
    }

    if (input->GetAction(IA_ATTACK_COORD) != 0 || PlayerAbsAction(input->GetAction(71u))>900 ||
       PlayerAbsAction(input->GetAction(70u))>900)
    {
        ClampFlagmanAttackHeight(m_flagman.m_value->m_xyz.z, probeHeight);
        const int attackZ = TruncateFloatToInt32(probeHeight);
        const int attackY = TruncateFloatToInt32(moveY);
        const int attackX = TruncateFloatToInt32(moveX);
        (void)m_flagman.m_value->Action(ACT_COOR_ATTACK, attackX, attackY, attackZ);
    }

    unsigned char desiredDirection = 0u;
    bool directionalInput = false;

    if (static_cast<unsigned int>(m_controlMode) != 3u)
    {
        if (static_cast<unsigned int>(m_controlMode) == 4u) desiredDirection = m_flagman.m_value->m_direction.value;

        if (PlayerAbsAction(input->GetAction(69u))>100 ||
           PlayerAbsAction(input->GetAction(68u))>100)
        {
            const float analogY = static_cast<float>(input->GetAction(69u));
            const float analogX = static_cast<float>(input->GetAction(68u));
            desiredDirection = static_cast<unsigned char>(desiredDirection+
                Decart2Polar(analogX, analogY).value);
        }
        else
        {
            if (input->GetAction(IA_MOVE_UP) != 0 && input->GetAction(IA_MOVE_RIGHT) != 0) desiredDirection = static_cast<unsigned char>(desiredDirection+36u);
            else if (input->GetAction(IA_MOVE_UP) != 0 && input->GetAction(IA_MOVE_LEFT) != 0) desiredDirection = static_cast<unsigned char>(desiredDirection+216u);
            else if (input->GetAction(IA_MOVE_DOWN) != 0 && input->GetAction(IA_MOVE_RIGHT) != 0) desiredDirection = static_cast<unsigned char>(desiredDirection+88u);
            else if (input->GetAction(IA_MOVE_DOWN) != 0 && input->GetAction(IA_MOVE_LEFT) != 0) desiredDirection = static_cast<unsigned char>(desiredDirection+164u);
            else if (input->GetAction(IA_MOVE_LEFT) != 0) desiredDirection = static_cast<unsigned char>(desiredDirection+192u);
            else if (input->GetAction(IA_MOVE_RIGHT) != 0) desiredDirection = static_cast<unsigned char>(desiredDirection+64u);
            else if (input->GetAction(IA_MOVE_DOWN) != 0) desiredDirection = static_cast<unsigned char>(desiredDirection+128u);
            else if (input->GetAction(IA_MOVE_UP) == 0)
            {
                if ((m_flagman.m_value->m_runtimeFlags&0x7Cu) != 4u)
                    (void)m_flagman.m_value->Stop();
                else if (::CurrentTime-m_targetTime>500u)
                {
                    const double movedX = fabs(static_cast<double>(m_targetX)-m_flagman.m_value->m_xyz.x);
                    const float movedY = static_cast<float>(fabs(static_cast<double>(m_targetY)-m_flagman.m_value->m_xyz.y));
                    if (FloatLessOrUnordered(movedX, 20.0) &&
                       FloatLessOrUnordered(movedY, 20.0))
                        (void)m_flagman.m_value->Stop();
                    else
                    {
                        m_targetX = m_flagman.m_value->m_xyz.x;
                        m_targetY = m_flagman.m_value->m_xyz.y;
                        m_targetTime = ::CurrentTime;
                    }
                }
            }
        }

        if (static_cast<unsigned int>(m_controlMode) == 4u)
        {
            if (input->GetAction(IA_MOVE_DOWN) != 0 || input->GetAction(69u)>100)
            {
                m_flagman.m_value->m_runtimeFlags |= 0x00004000u;
                desiredDirection = static_cast<unsigned char>(desiredDirection+128u);
            }
            else if ((m_flagman.m_value->m_runtimeFlags&0x7Cu) != 4u)
            {
                m_flagman.m_value->m_runtimeFlags &= ~0x00004000u;
            }
        }
        else
        {
            m_flagman.m_value->m_runtimeFlags &= ~0x00004000u;
        }

        directionalInput = input->GetAction(IA_MOVE_UP) != 0 || input->GetAction(IA_MOVE_DOWN) != 0 ||
                           input->GetAction(IA_MOVE_RIGHT) != 0 || input->GetAction(IA_MOVE_LEFT) != 0 ||
                           PlayerAbsAction(input->GetAction(69u))>100 ||
                           PlayerAbsAction(input->GetAction(68u))>100;
        if (directionalInput)
        {
            const unsigned int elapsedTime = ::CurrentTime-::PrevCurrentTime;
            if (static_cast<unsigned int>(m_controlMode) == 4u)
            {
                (void)m_flagman.m_value->RotateTact(ANGLE(desiredDirection), static_cast<int>(elapsedTime));
            }
            else
            {
                ANGLE requestedDirection;
                requestedDirection.value = desiredDirection;
                m_flagman.m_value->RotateForGlide(requestedDirection, static_cast<int>(elapsedTime));
            }

            if (static_cast<unsigned int>(m_controlMode) != 4u || input->GetAction(IA_MOVE_DOWN) != 0 || input->GetAction(IA_MOVE_UP) != 0)
                (void)m_flagman.m_value->StartMove();
        }
    }

    MAP* const activeMap = Map;
    const unsigned int activePlayerIndex = activeMap->m_playerIndex;
    PLAYER* const activePlayer = activeMap->m_players[activePlayerIndex];
    const unsigned int activeControlMode = static_cast<unsigned int>(activePlayer->m_controlMode);
    const bool historyEnabled = (activeControlMode == 3u || activeControlMode == 4u);

    if (!historyEnabled)
    {
        m_moveHistoryCount = 0u;
    }
    else
    {
        unsigned int historyCount = m_moveHistoryCount;
        bool sameAsLast = false;
        if (historyCount != 0u)
        {
            const PLAYER_HISTORY_RECORD& latestHistory = m_moveHistory[historyCount-1u];
            sameAsLast = PlayerSamePosition(latestHistory.position, m_flagman.m_value) && latestHistory.direction == m_flagman.m_value->m_direction.value;
        }
        const bool positionChanged = !sameAsLast && !PlayerSamePosition(m_moveFallback, m_flagman.m_value);

        const unsigned int historyMarker = (input->GetAction(IA_MOVE_DOWN) == 0 &&
            input->GetAction(69u) <= 100 && m_flagman.m_value->m_speed >= 0.0f)?1u:0u;

        if (positionChanged && m_moveRollback == 0u)
        {
            if (BitsToSigned32(m_moveHistoryCount) >= 7)
            {
                --m_moveHistoryCount;
                for (int historyIndex = 0;historyIndex<BitsToSigned32(m_moveHistoryCount);++historyIndex)
                {
                    m_moveHistory[historyIndex].position = m_moveHistory[historyIndex+1].position;
                    m_moveHistory[historyIndex].direction = m_moveHistory[historyIndex+1].direction;
                    m_moveHistory[historyIndex].marker = m_moveHistory[historyIndex+1].marker;
                    m_moveHistory[historyIndex].time = m_moveHistory[historyIndex+1].time;
                }
            }
            m_moveHistory[m_moveHistoryCount].time = ::CurrentTime;
            m_moveHistory[m_moveHistoryCount].position = m_flagman.m_value->m_xyz;
            m_moveHistory[m_moveHistoryCount].direction = m_flagman.m_value->m_direction.value;
            m_moveHistory[m_moveHistoryCount].marker = historyMarker;
            m_moveHistoryTime = ::CurrentTime;
            ++m_moveHistoryCount;
        }
        else
        {
            const bool wantsRollback = input->GetAction(IA_MOVE_DOWN) != 0 ||
                input->GetAction(IA_MOVE_UP) != 0 || PlayerAbsAction(input->GetAction(69u))>100;
            if (wantsRollback && !FloatEqualOrUnordered(m_flagman.m_value->m_speed, 0.0f))
            {
                historyCount = m_moveHistoryCount;
                bool rolledBack = false;
                if (historyCount != 0u)
                {
                    const PLAYER_HISTORY_RECORD& latestHistory = m_moveHistory[historyCount-1u];
                    const unsigned int latestHistoryTime = latestHistory.time;
                    if (latestHistoryTime != ::PrevCurrentTime &&
                       (latestHistory.marker != historyMarker || m_moveRollback != 0u))
                    {
                        --historyCount;
                        m_moveHistoryCount = historyCount;
                        m_moveRollback = 1u;
                        m_flagman.m_value->ChangeDirection1(ANGLE(m_moveHistory[historyCount].direction));
                        m_moveFallback = m_moveHistory[m_moveHistoryCount].position;
                        (void)m_flagman.m_value->ChangeCoor(m_moveFallback);
                        rolledBack = true;
                    }
                }

                if (!rolledBack)
                {
                    m_moveRollback = 0u;
                    if (::CurrentTime-m_moveHistoryTime>2000u)
                    {
                        bool placementFound = false;
                        for (float searchRadius = 5.0f; FloatLessOrUnordered(searchRadius, 45.0) && !placementFound; searchRadius += 5.0f)
                        {
                            for (unsigned int searchDirection = 0u;searchDirection <= 252u;searchDirection += 2u)
                            {
                                const ANGLE searchAngle(static_cast<unsigned char>(searchDirection));
                                const float requestedX = static_cast<float>(
                                    static_cast<double>(searchAngle.Sin())*static_cast<double>(searchRadius)+
                                    static_cast<double>(m_flagman.m_value->m_xyz.x));

                                const float candidateYOffset = static_cast<float>(
                                    -static_cast<double>(searchAngle.Cos())*static_cast<double>(searchRadius));
                                const float requestedY = static_cast<float>(static_cast<double>(candidateYOffset)+
                                                                 static_cast<double>(m_flagman.m_value->m_xyz.y));
                                m_moveFallback.x = requestedX;
                                m_moveFallback.y = requestedY;
                                m_moveFallback.z = m_flagman.m_value->m_xyz.z;
                                if (m_flagman.m_value->CanPlace(m_moveFallback) == 0)
                                {
                                    (void)m_flagman.m_value->ChangeCoor(m_moveFallback);
                                    placementFound = true;
                                    break;
                                }
                            }
                        }
                        (void)::Error->Log("%u ChangePosition", ::CurrentTime);
                    }
                }
            }
        }
    }


    SPRITE* childSprite = m_flagman.m_value->m_childChain;
    if (childSprite && m_flagman.m_value->m_currentAnimation<15)
    {
        SPRITE* aimSprite = m_flagman.m_value->m_childChain;
        const float aimOffsetX = moveX-aimSprite->m_xyz.x;
        const float aimOffsetY = (moveY+probeHeight)-aimSprite->m_xyz.y;
        unsigned char childTargetDirection = Decart2Polar(aimOffsetX, aimOffsetY).value;
        if (FloatEqualOrUnordered(m_flagman.m_value->m_speed, 0.0f) && static_cast<unsigned int>(m_flagman.m_value->m_currentAnimation) == 2u)
            (void)m_flagman.m_value->ChangeAnimation(0);

        aimSprite = m_flagman.m_value->m_childChain;
        const unsigned char cachedDirection = m_cachedDirection;
        if (cachedDirection == aimSprite->m_direction.value)
        {
            bool rotateAllowed = true;
            const unsigned int aimSpriteKind = aimSprite->m_runtimeFlags&0x7Cu;
            if (aimSpriteKind == 0x10u || aimSpriteKind == 0x14u)
            {
                VID* const aimSpriteVid = aimSprite->m_vid;
                WEAPON_RUNTIME* const weaponConfig = aimSpriteVid->weapon;
                rotateAllowed = (weaponConfig->Property&1u) != 0u;
            }
            if (rotateAllowed)
            {
                SPRITE* const rotationTarget = m_flagman.m_value->m_childChain;
                (void)rotationTarget->RotateTact(
                    ANGLE(childTargetDirection), static_cast<int>(::CurrentTime-::PrevCurrentTime));
            }
        }
        childSprite = m_flagman.m_value->m_childChain;
        m_cachedDirection = childSprite->m_direction.value;
    }

    SPRITE* transientSprite = m_transientSprite.m_value;
    if (transientSprite)
    {
        VID* const transientVid = transientSprite->m_vid;
        if ((transientVid->spriteType&4u) != 0u)
        {
            if (m_transientSprite.m_value) m_transientSprite.m_value->Release();
            m_transientSprite.m_value = 0;
        }
    }

    m_previousInput.x = input->MouseXY().x;
    m_previousInput.y = input->MouseXY().y;
}

GROUP* GROUPS::Next(const GROUP* group)
{
    if (!group) return 0;
    GROUP* const nextGroup = group->m_next;
    return nextGroup == static_cast<const GROUP*>(this) ? 0 : nextGroup;
}

SPRITE* PLAYER::Flagman() const
{
    return m_flagman.m_value;
}

void PLAYER::GroupsTact()
{


    GROUPS* const groups = &Map->m_groups;
    GROUP* group = groups->m_next;
    if (group == static_cast<GROUP*>(groups)) return;

    while (group)
    {
        const int memberCount = group->m_count;
        SPRITE** const members = group->m_data;
        SPRITE* commandTarget = 0;
        unsigned int commandCode = 0u;
        unsigned int commandArmy = 0u;
        bool hasCommandRecipient = false;

        for (int memberIndex = 0;memberIndex<memberCount;++memberIndex)
        {
            SPRITE* member = members[memberIndex];
            const unsigned int memberFlags = member->m_runtimeFlags;
            const unsigned int memberKind = memberFlags&0x7Cu;
            if (memberKind == 0u) hasCommandRecipient = true;
            if (memberKind == 0x0Cu || memberKind == 0x10u)
            {
                SPRITE* const goalSprite = member->m_goalSprite;
                if (goalSprite && goalSprite->m_vid != ::EmptyVid)
                {
                    commandTarget = goalSprite;
                    commandCode = (memberFlags>>2)&0x1Fu;
                    commandArmy = (memberFlags>>17)&7u;
                }
            }
        }

        if (commandTarget && commandCode && hasCommandRecipient)
        {
            for (int memberIndex = 0;memberIndex<group->m_count;++memberIndex)
            {
                SPRITE* member = group->m_data[memberIndex];
                const unsigned int memberFlags = member->m_runtimeFlags;
                if ((memberFlags&0x7Cu) == 0u && ((memberFlags>>17)&7u) == commandArmy)
                {
                    const unsigned int behaviorFlags = member->m_behave;
                    if (behaviorFlags && (behaviorFlags&0x20u) == 0u)
                        (void)member->SetCommand(static_cast<int>(commandCode), commandTarget);
                }
            }
        }
        group = Map->m_groups.Next(group);
    }
}


