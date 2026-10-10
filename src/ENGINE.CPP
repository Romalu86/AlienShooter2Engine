
#include "myerror.h"
#include "railway.h"
#include "texture.h"
#include "engine_globals.h"
#include "basetype.h"
#include "engine.h"
#include "sprite_collector.h"
#include "sprite_list.h"
#include "railway.h"
#include "rail.h"
#include "sprite.h"
#include "vid.h"
#include "graph.h"
#include "region.h"
#include "mylib.h"
#include <new>
#include "map.h"
#include "resread.h"
#include "constant.h"
#include "player.h"
#include <math.h>



StaticObjectStorage<SPRITE_LIST> ENGINE::PathDots;
unsigned char ENGINE::pathDotsDestructorFlags = 0;

unsigned int __cdecl ENGINE::InitializePathDots()
{
    ::new (static_cast<void*>(PathDots.Address())) SPRITE_LIST();
    return RegisterCrtAtExit(&ENGINE::DestroyPathDots);
}

void __cdecl ENGINE::DestroyPathDots()
{
    if ((pathDotsDestructorFlags & 1u) != 0u)
        return;
    pathDotsDestructorFlags = static_cast<unsigned char>(pathDotsDestructorFlags | 1u);
    PathDots.Get().~SPRITE_LIST();
}

void ENGINE::ClearPathDots()
{
    PathDots.Get().DeleteAll();
}

int ENGINE::NoStep = 0;
int ENGINE::NoStepForNotFound = 0;

ENGINE::ENGINE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : UNIT(vid, position, direction, parent)
{
    m_frontPos.m_dot = 0;
    m_frontPos.m_progressFraction = 0u;
    m_backPos.m_dot = 0;
    m_backPos.m_progressFraction = 0u;
    m_trainFlags &= ~1u;
    m_railState = 0u;
    m_repairProgress = -1;
    m_stateFC = 1;
    m_previousPositionZ = 0.0f;
    m_previousPositionY = 0.0f;
    m_previousPositionX = 0.0f;
    m_mineDeploymentReady = 0;
    m_commandTick = 0u;
    m_depotBatchNotificationPending = 0;
    m_depotBatchId = -1;
    m_pathCount = 0;
    m_pathStart = 0;
    m_pathEnd = 0;
    m_targetSpeed = 0.0f;
    m_acceleration = 0;
    m_prevEngine = 0;
    m_nextEngine = 0;
    m_commandDot = 0;
    m_commandOwner = 0;
    if ((Map->m_flags&0x40u) == 0u)
    {
        SetRDot();
        SPRITE* const linked = m_childChain;
        if (linked && linked->Vid() == m_vid->linkVid)
            linked->ChangeDirection(m_direction);
    }
}

void ENGINE::Error(TYPE_ERROR type, const STRING& subject, unsigned long detail) const
{
    if (::Error == 0)
        return;
    const unsigned int ownerId = m_vid?static_cast<unsigned int>(m_vid->nVid):0xFFFFFFFFu;
    STRING objectFormat("ENGINE %i");
    ::Error->Error(objectFormat, type, subject, detail, ownerId);
}

extern "C" void  EngineError(void* self, int type, const void* text, unsigned long detail)
{
    const DIALOG_UNIT* const dialog = static_cast<const DIALOG_UNIT*>(self);
    const UNIT* const unit = static_cast<const UNIT*>(dialog);
    const ENGINE* const engine = static_cast<const ENGINE*>(unit);
    engine->ENGINE::Error(static_cast<TYPE_ERROR>(type), *static_cast<const STRING*>(text), detail);
}


ENGINE::~ENGINE()
{
    SPRITE_COLLECTOR* manager = ::Sprites;
    if (manager->deletingAll == 0)
        (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_UNIT_DESTROY], this, 0, 0);
    SPRITE* const command = m_commandOwner;
    if (command == this)
        SetCommandToTrain(0, 0, 0, 0);
    else if (command)
    {
        (void)command->Release();
        m_commandOwner = 0;
    }
    if (Map->Flagman(Map->m_playerIndex) == this)
        PathDots.Get().DeleteAll();
    manager = ::Sprites;
    if (manager->deletingAll == 0)
        ClearDotBusy();
    if (m_prevEngine)
    {
        m_prevEngine->m_nextEngine = 0;
        m_prevEngine->ReCalcMoveParameters();
    }
    if (m_nextEngine)
    {
        m_nextEngine->m_prevEngine = 0;
        m_nextEngine->ReCalcMoveParameters();
    }
    manager = ::Sprites;
    if (manager->deletingAll == 0)
    {
        if (m_prevEngine && m_nextEngine)
            (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_SPLIT], m_prevEngine, m_nextEngine, 0);
        WEAPON_RUNTIME* const stats = m_vid->weapon;
        const double delta = static_cast<double>(stats->Power)-static_cast<double>(stats->Weight);
        if (delta>1.0)
            (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_DESTROY_POWER], this, 0, 0);
        if (!m_prevEngine && !m_nextEngine)
            (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_DESTROY], this, 0, 0);
        if (m_depotBatchNotificationPending != 0)
        {
            manager = ::Sprites;
            SPRITE* candidate = manager->FirstUnit(&manager->reserved2EC, -1);
            while (candidate)
            {
                if (candidate->Vid()->spriteClass == 21u && ((candidate->RuntimeFlags()^m_runtimeFlags)&0x000E0000u) == 0u)
                {
                    ENGINE* const candidateEngine = static_cast<ENGINE*>(candidate);
                    if (candidateEngine->m_depotBatchId == m_depotBatchId)
                        break;
                }
                manager = ::Sprites;
                candidate = manager->NextUnit(&manager->reserved2EC);
            }
            if (candidate)
                (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_CREATED], candidate, 0, 0);
        }
    }
}

void ENGINE::DeletePointerToSprite(SPRITE* target)
{
    if (m_commandOwner == target)
        SetCommandToTrain(0, 0, 0, 0);
    VID* const targetOwner = target->Vid();
    if (targetOwner->spriteClass == 21u && m_goalSprite == target && (m_runtimeFlags&0x7Cu) == 0x68u)
    {
        ENGINE* const targetEngine = static_cast<ENGINE*>(target);
        SPRITE* linked = targetEngine->m_nextEngine;
        if (!linked)
            linked = targetEngine->m_prevEngine;
        if (linked)
            SetCommandToTrain(26, linked, 0, 0);
    }
    if (m_goalSprite == target)
        SetCommandToTrain(0, 0, 0, 0);
    SPRITE::DeletePointerToSprite(target);
}

void ENGINE::DrawSecondaryInfo()
{
    GRAPH* const graph = Graph;
    float y = graph->ViewYMin();
    const float x = graph->ViewXMin()+22.0f;
    const int goalVid = m_goalSprite?m_goalSprite->Vid()->nVid:0;
    const int ammo = Action(ACT_GET_AMMO, 0, 0, 0);
    graph->PrintfXY( x, y, "Ref=%-3i cmd=%1i ani=%-2i ammo=%-3i hp=%-3i AT=%i goal=%-3i speed=%-3i timer=%i", static_cast<int>(m_referenceCount), static_cast<int>((m_runtimeFlags>>2u)&0x1Fu), m_currentAnimation, ammo, m_hp, static_cast<int>((m_runtimeFlags>>7u)&7u), goalVid, TruncateFloatToInt32(m_speed*1000.0f), static_cast<int>(m_stateTimer));
    y += 12.0f;
    graph->PrintfXY( x, y, "IS_PBJMF(%i%i%i%i%i) Accel=%i MaxSpeed=%i NoStep=%i NoStepNotF=%i", static_cast<int>(m_railState), m_stateFC, static_cast<int>(m_runtimeFlags&1u), m_mineDeploymentReady, static_cast<int>((m_runtimeFlags>>10u)&1u), m_acceleration, TruncateFloatToInt32(m_targetSpeed*1000.0f), NoStep, NoStepForNotFound);
    SPRITE* const child = m_childChain;
    VID* const owner = m_vid;
    if (child && child->Vid() == owner->linkVid)
    {
        y += 12.0f;
        DIALOG_UNIT* const dialogUnit = static_cast<DIALOG_UNIT*>(this);
        SPRITE* linked = dialogUnit->Link();
        if (linked)
        {
            SPRITE* const linkedGoalSprite = linked->Goal();
            const int linkedGoal = linkedGoalSprite?linkedGoalSprite->Vid()->nVid:0;
            const int linkedAmmo = linked->Action(ACT_GET_AMMO, 0, 0, 0);
            graph->PrintfXY( x, y, "Ref=%-3i cmd=%1i ani=%-2i ammo=%-3i hp=%-3i goal=%-3i timer=%i", static_cast<int>(linked->m_referenceCount), static_cast<int>((linked->RuntimeFlags()>>2u)&0x1Fu), linked->Animation(), linkedAmmo, linked->m_hp, linkedGoal, static_cast<int>(linked->StateTimer()));
        }
    }
    if (ActionStack(0).m_count != 0)
    {
        STRING commandText;
        y += 12.0f;
        commandText += Printf("%i - ", ActionStack(0).m_count);
        for (int index = 0; index<ActionStack(0).m_count; ++index)
        {
            const LIST<ACT>& thirdArgumentStack = ActionStack(0);
            const LIST<ACT>& secondArgumentStack = ActionStack(0);
            const LIST<ACT>& firstArgumentStack = ActionStack(0);
            const LIST<ACT>& commandStack = ActionStack(0);
            commandText += Printf("%i(%i,%i,%i)",
                commandStack.m_data[index].command,
                firstArgumentStack.m_data[index].arg1,
                secondArgumentStack.m_data[index].arg2,
                thirdArgumentStack.m_data[index].arg3);
        }
        COLOR color;
        color.value = 0xFFFFFFFFu;
        graph->PutsXY(graph->ViewXMin()+30.0f, y, commandText, color);
    }
    DrawGoalLine();
    SPRITE* const marker = m_commandOwner;
    if (marker)
    {
        const VECTOR2 point = marker->ScreenXY();
        graph->PutBigPixel(point, GRAPH::RED);
    }
}

void ENGINE::DrawGoalLine()
{
    GRAPH* graph = Graph;
    const VECTOR2 pixelOffset(2, 2);
    if (m_pathEnd)
    {
        const VECTOR2 railXY = m_pathEnd->ScreenXY();
        VECTOR2 from = ScreenXY()+pixelOffset;
        VECTOR2 to = railXY+pixelOffset;
        COLOR color;
        color.value = GRAPH::WHITE.value;
        graph->Line(from, to, color);
    }
    if (m_pathStart)
    {
        const VECTOR2 railXY = m_pathStart->ScreenXY();
        VECTOR2 from = ScreenXY()+pixelOffset;
        VECTOR2 to = railXY+pixelOffset;
        COLOR color;
        color.value = GRAPH::WHITE.value;
        graph->Line(from, to, color);
    }
    SPRITE* goal = Goal();
    if (goal)
    {
        VECTOR2 from = ScreenXY();
        VECTOR2 to = goal->ScreenXY();
        COLOR color;
        color.value = GRAPH::GREEN.value;
        graph->Line(from, to, color);
    }
    DIALOG_UNIT* const dialog = static_cast<DIALOG_UNIT*>(this);
    if (dialog->Link() != 0)
    {
        SPRITE* linked = dialog->Link();
        SPRITE* linkedGoal = linked->Goal();
        if (linkedGoal)
        {
            VECTOR2 from = linked->ScreenXY();
            VECTOR2 to = linkedGoal->ScreenXY();
            COLOR color;
            color.value = GRAPH::RED.value;
            graph->Line(from, to, color);
        }
    }
    if (m_commandDot)
    {
        VECTOR2 to = m_commandDot->ScreenXY();
        VECTOR2 from = ScreenXY();
        COLOR color;
        color.value = GRAPH::BLUE.value;
        graph->Line(from, to, color);
    }
}


int ENGINE::IsRepair() const
{
    return m_vid->nVid == 85 ? 1 : 0;
}

ENGINE* ENGINE::FirstEngine()
{
    ENGINE* firstEngine = this;
    while (firstEngine->m_prevEngine)
        firstEngine = firstEngine->m_prevEngine;
    return firstEngine;
}

ENGINE* ENGINE::LastEngine()
{
    ENGINE* lastEngine = this;
    while (lastEngine->m_nextEngine)
        lastEngine = lastEngine->m_nextEngine;
    return lastEngine;
}

static void EngineMovePositionToCloserEndpoint(const ENGINE* engine, R_POS* position)
{
    R_DOT* const node = position->m_dot;
    const unsigned int linkIndex = static_cast<unsigned int>(position->m_linkIndex);
    R_DOT* const linkedDot = node->m_links[linkIndex].dot;
    const VECTOR& enginePosition = engine->Coor();
    const float nodeDx = static_cast<float>(node->m_x)-enginePosition.x;
    const float nodeDy = static_cast<float>(node->m_y)-enginePosition.y;
    const float nodeDz = static_cast<float>(node->m_z)-enginePosition.z;
    const float targetDx = static_cast<float>(linkedDot->m_x)-enginePosition.x;
    const float targetDy = static_cast<float>(linkedDot->m_y)-enginePosition.y;
    const float targetDz = static_cast<float>(linkedDot->m_z)-enginePosition.z;
    const float nodeDistance = static_cast<float>(sqrt(static_cast<double>(nodeDx*nodeDx+nodeDy*nodeDy+nodeDz*nodeDz)));
    const float targetDistance = static_cast<float>(sqrt(static_cast<double>(targetDx*targetDx+targetDy*targetDy+targetDz*targetDz)));
    if (targetDistance<nodeDistance)
    {
        position->m_progress = node->m_links[linkIndex].length-position->m_progress;
        position->m_dot = linkedDot;
        position->m_linkIndex = node->m_links[linkIndex].reverseIndex;
    }
}

void ENGINE::SetRDot()
{
    VID* const engineVid = m_vid;
    const WEAPON_RUNTIME* const weapon = engineVid->weapon;
    const float radius = weapon->Length*(0.5f);
    const int engineX = TruncateFloatToInt32(m_xyz.x);
    const int engineY = TruncateFloatToInt32(m_xyz.y);
    const int engineZ = TruncateFloatToInt32(m_xyz.z);
    R_DOT* nearestDot = RailMap.Get().GetNearestDot(engineX, engineY, engineZ);
    if (!nearestDot || nearestDot->m_linkCount <= 0)
        return;
    const unsigned char baseDirection = m_direction.value;
    int linkIndex = nearestDot->m_linkCount-1;
    for (;linkIndex >= 0;--linkIndex)
    {
        const unsigned char facing = nearestDot->m_links[linkIndex].direction.value;
        const unsigned char forwardTurn = static_cast<unsigned char>(baseDirection-facing);
        const unsigned char reverseTurn = static_cast<unsigned char>(facing-baseDirection);
        const unsigned char turn = forwardTurn<reverseTurn?forwardTurn:reverseTurn;
        if (turn>108u || turn<20u)
            break;
    }
    unsigned char facing = linkIndex >= 0?baseDirection:nearestDot->m_links[0].direction.value;
    R_POS* frontPosition = &m_frontPos;
    const float frontX = m_xyz.x+radius*ANGLE(static_cast<unsigned char>(static_cast<unsigned int>(facing))).Sin();
    const float frontY = m_xyz.y-radius*ANGLE(static_cast<unsigned char>(static_cast<unsigned int>(facing))).Cos();
    nearestDot->SetNearestPos(TruncateFloatToInt32(frontX), TruncateFloatToInt32(frontY), engineZ, frontPosition);
    EngineMovePositionToCloserEndpoint(this, frontPosition);
    facing = static_cast<unsigned char>(facing-128u);
    R_POS* backPosition = &m_backPos;
    const float backX = m_xyz.x+radius*ANGLE(static_cast<unsigned char>(static_cast<unsigned int>(facing))).Sin();
    const float backY = m_xyz.y-radius*ANGLE(static_cast<unsigned char>(static_cast<unsigned int>(facing))).Cos();
    nearestDot->SetNearestPos(TruncateFloatToInt32(backX), TruncateFloatToInt32(backY), engineZ, backPosition);
    EngineMovePositionToCloserEndpoint(this, backPosition);
    PullTail(*frontPosition);
    CalcCoor();
    ForceLink(GetIntersecting());
    SetDotBusy();
}

int ENGINE::InTrain(const SPRITE* candidateSprite) const
{
    if (!candidateSprite)
        return 0;
    const ENGINE* trainEngine = this;
    while (trainEngine)
    {
        if (trainEngine == candidateSprite)
            return 1;
        trainEngine = trainEngine->m_nextEngine;
    }
    trainEngine = m_prevEngine;
    while (trainEngine)
    {
        if (trainEngine == candidateSprite)
            return 1;
        trainEngine = trainEngine->m_prevEngine;
    }
    return 0;
}

void ENGINE::ReverseTrain()
{
    ENGINE* firstEngine = FirstEngine();
    if (!firstEngine)
        return;
    ENGINE* trainEngine = firstEngine;
    for (;;)
    {
        ENGINE* nextEngine = trainEngine->m_nextEngine;
        ENGINE* previousEngine = trainEngine->m_prevEngine;
        trainEngine->m_nextEngine = previousEngine;
        trainEngine->m_prevEngine = nextEngine;
        const R_POS previousFrontPosition = trainEngine->m_frontPos;
        trainEngine->m_frontPos = trainEngine->m_backPos;
        trainEngine->m_backPos = previousFrontPosition;
        trainEngine->m_trainFlags ^= 1u;
        if (!nextEngine)
        {
            if (trainEngine != firstEngine)
            {
                const unsigned int flags = trainEngine->m_runtimeFlags;
                trainEngine->m_runtimeFlags = (flags&~0x400u)|(firstEngine->m_runtimeFlags&0x400u);
                trainEngine->m_speed = firstEngine->m_speed;
                trainEngine->m_targetSpeed = -firstEngine->m_targetSpeed;
                trainEngine->m_acceleration = firstEngine->m_acceleration;
                const unsigned int pathLength = static_cast<unsigned int>(firstEngine->m_pathCount);
                CopyBytes(trainEngine->m_pathLinks, firstEngine->m_pathLinks, pathLength);
                firstEngine->m_pathCount = 0;
            }
            else
            {
                firstEngine->m_targetSpeed = -firstEngine->m_targetSpeed;
            }
            break;
        }
        trainEngine = nextEngine;
    }
}

ENGINE* ENGINE::GetIntersecting()
{
    ENGINE* attachedEngine = static_cast<ENGINE*>(m_frontPos.m_dot->m_attached);
    if (attachedEngine && IsTouch(attachedEngine, 0))
        return static_cast<ENGINE*>(m_frontPos.m_dot->m_attached);
    R_DOT* currentDot = m_frontPos.m_dot;
    R_DOT* linkedDot = currentDot?currentDot->m_links[static_cast<unsigned int>(m_frontPos.m_linkIndex)].dot:0;
    attachedEngine = static_cast<ENGINE*>(linkedDot->m_attached);
    if (attachedEngine)
    {
        currentDot = m_frontPos.m_dot;
        linkedDot = currentDot?currentDot->m_links[static_cast<unsigned int>(m_frontPos.m_linkIndex)].dot:0;
        if (IsTouch(static_cast<ENGINE*>(linkedDot->m_attached), 0))
        {
            currentDot = m_frontPos.m_dot;
            linkedDot = currentDot?currentDot->m_links[static_cast<unsigned int>(m_frontPos.m_linkIndex)].dot:0;
            return static_cast<ENGINE*>(linkedDot->m_attached);
        }
    }
    attachedEngine = static_cast<ENGINE*>(m_backPos.m_dot->m_attached);
    if (attachedEngine && IsTouch(attachedEngine, 0))
        return static_cast<ENGINE*>(m_backPos.m_dot->m_attached);
    currentDot = m_backPos.m_dot;
    linkedDot = currentDot?currentDot->m_links[static_cast<unsigned int>(m_backPos.m_linkIndex)].dot:0;
    attachedEngine = static_cast<ENGINE*>(linkedDot->m_attached);
    if (attachedEngine)
    {
        currentDot = m_backPos.m_dot;
        linkedDot = currentDot?currentDot->m_links[static_cast<unsigned int>(m_backPos.m_linkIndex)].dot:0;
        if (IsTouch(static_cast<ENGINE*>(linkedDot->m_attached), 0))
        {
            currentDot = m_backPos.m_dot;
            linkedDot = currentDot?currentDot->m_links[static_cast<unsigned int>(m_backPos.m_linkIndex)].dot:0;
            return static_cast<ENGINE*>(linkedDot->m_attached);
        }
    }
    return 0;
}

ENGINE* ENGINE::GetBadIntersecting()
{
    R_DOT* frontDot = m_frontPos.m_dot;
    const unsigned int frontLinkIndex = static_cast<unsigned int>(m_frontPos.m_linkIndex);
    if (frontDot)
    {
        R_DOT::LINK* crossingLink = frontDot->m_links[frontLinkIndex].crossing;
        if (crossingLink)
        {
            ENGINE* attachedEngine = static_cast<ENGINE*>(crossingLink->dot->m_attached);
            if (attachedEngine && !attachedEngine->InTrain(this))
                return static_cast<ENGINE*>(crossingLink->dot->m_attached);
            const unsigned int crossingReverseIndex = static_cast<unsigned int>(crossingLink->reverseIndex);
            crossingLink = &crossingLink->dot->m_links[crossingReverseIndex];
            attachedEngine = static_cast<ENGINE*>(crossingLink->dot->m_attached);
            if (attachedEngine && !attachedEngine->InTrain(this))
                return static_cast<ENGINE*>(crossingLink->dot->m_attached);
        }
    }
    frontDot = m_frontPos.m_dot;
    if (!frontDot)
        return 0;
    R_DOT* linkedDot = frontDot->m_links[frontLinkIndex].dot;
    if (!linkedDot)
        return 0;
    const unsigned int reverseLinkIndex = static_cast<unsigned int>(frontDot->m_links[frontLinkIndex].reverseIndex);
    R_DOT::LINK* crossingLink = linkedDot->m_links[reverseLinkIndex].crossing;
    if (!crossingLink)
        return 0;
    ENGINE* attachedEngine = static_cast<ENGINE*>(crossingLink->dot->m_attached);
    if (attachedEngine && !attachedEngine->InTrain(this))
        return static_cast<ENGINE*>(crossingLink->dot->m_attached);
    const unsigned int crossingReverseIndex = static_cast<unsigned int>(crossingLink->reverseIndex);
    crossingLink = &crossingLink->dot->m_links[crossingReverseIndex];
    attachedEngine = static_cast<ENGINE*>(crossingLink->dot->m_attached);
    return (attachedEngine && !attachedEngine->InTrain(this)) ? static_cast<ENGINE*>(crossingLink->dot->m_attached) : 0;
}

ENGINE* ENGINE::GetForwardIntersecting(int* relation)
{
    ENGINE* attachedEngine = static_cast<ENGINE*>(m_frontPos.m_dot->m_attached);
    if (attachedEngine)
    {
        const int touchRelation = IsTouch(attachedEngine, 1);
        *relation = touchRelation;
        if (touchRelation)
            return static_cast<ENGINE*>(m_frontPos.m_dot->m_attached);
    }
    R_DOT* frontDot = m_frontPos.m_dot;
    R_DOT* linkedDot = frontDot?frontDot->m_links[static_cast<unsigned int>(m_frontPos.m_linkIndex)].dot:0;
    attachedEngine = static_cast<ENGINE*>(linkedDot->m_attached);
    if (attachedEngine)
    {
        const int touchRelation = IsTouch(attachedEngine, 1);
        *relation = touchRelation;
        if (touchRelation)
        {
            frontDot = m_frontPos.m_dot;
            linkedDot = frontDot?frontDot->m_links[static_cast<unsigned int>(m_frontPos.m_linkIndex)].dot:0;
            return static_cast<ENGINE*>(linkedDot->m_attached);
        }
    }
    *relation = 0;
    return 0;
}

int ENGINE::IsTouch(ENGINE* otherEngine, int strictOverlap) const
{
    if (!otherEngine || InTrain(otherEngine))
        return 0;
    R_DOT* const frontDot = m_frontPos.m_dot;
    R_DOT* const backDot = m_backPos.m_dot;
    R_DOT* const otherFrontDot = otherEngine->m_frontPos.m_dot;
    R_DOT* const otherBackDot = otherEngine->m_backPos.m_dot;
    const int frontProgress = m_frontPos.m_progress;
    const int backProgress = m_backPos.m_progress;
    const int otherFrontProgress = otherEngine->m_frontPos.m_progress;
    const int otherBackProgress = otherEngine->m_backPos.m_progress;
    if (frontDot && otherFrontDot && m_frontPos.Dot2() == otherFrontDot && frontDot == otherEngine->m_frontPos.Dot2())
    {
        const int oppositeProgress = SubSigned32Wrap(frontDot->m_links[m_frontPos.m_linkIndex].length, otherFrontProgress);
        if (strictOverlap ? frontProgress>oppositeProgress : frontProgress >= oppositeProgress)
            return 1;
    }
    if (frontDot && otherBackDot && m_frontPos.Dot2() == otherBackDot && frontDot == otherEngine->m_backPos.Dot2())
    {
        const int oppositeProgress = SubSigned32Wrap(frontDot->m_links[m_frontPos.m_linkIndex].length, otherBackProgress);
        if (strictOverlap ? frontProgress>oppositeProgress : frontProgress >= oppositeProgress)
            return 2;
    }
    if (backDot && otherFrontDot && m_backPos.Dot2() == otherFrontDot && backDot == otherEngine->m_frontPos.Dot2())
    {
        const int oppositeProgress = SubSigned32Wrap(backDot->m_links[m_backPos.m_linkIndex].length, otherFrontProgress);
        if (strictOverlap ? backProgress>oppositeProgress : backProgress >= oppositeProgress)
            return 3;
    }
    if (backDot && otherBackDot && m_backPos.Dot2() == otherBackDot && backDot == otherEngine->m_backPos.Dot2())
    {
        const int oppositeProgress = SubSigned32Wrap(backDot->m_links[m_backPos.m_linkIndex].length, otherBackProgress);
        if (strictOverlap ? backProgress>oppositeProgress : backProgress >= oppositeProgress)
            return 4;
    }
    if (!strictOverlap)
    {
        if (frontDot && frontDot == otherFrontDot)
            return 100;
        if (frontDot && frontDot == otherBackDot)
            return 200;
        if (backDot && backDot == otherFrontDot)
            return 300;
        if (backDot && backDot == otherBackDot)
            return 400;
        return 0;
    }
    if (frontDot && frontDot == otherFrontDot)
        return static_cast<unsigned int>(m_frontPos.m_linkIndex) == static_cast<unsigned int>(otherEngine->m_frontPos.m_linkIndex) ? 2 : 1;
    if (frontDot && frontDot == otherBackDot)
        return 200;
    if (backDot && backDot == otherFrontDot)
        return 300;
    if (backDot && backDot == otherBackDot)
        return 400;
    return 0;
}

void ENGINE::ClearDotBusy()
{
    R_DOT* currentDot = m_frontPos.m_dot;
    if (currentDot && currentDot->m_attached == this)
        currentDot->m_attached = 0;
    currentDot = m_backPos.m_dot;
    if (currentDot && currentDot->m_attached == this)
        currentDot->m_attached = 0;
    currentDot = m_frontPos.m_dot;
    if (currentDot)
    {
        R_DOT* linkedDot = currentDot->m_links[static_cast<unsigned int>(m_frontPos.m_linkIndex)].dot;
        if (linkedDot && linkedDot->m_attached == this)
            linkedDot->m_attached = 0;
    }
    currentDot = m_backPos.m_dot;
    if (currentDot)
    {
        R_DOT* linkedDot = currentDot->m_links[static_cast<unsigned int>(m_backPos.m_linkIndex)].dot;
        if (linkedDot && linkedDot->m_attached == this)
            linkedDot->m_attached = 0;
    }
}

void ENGINE::SetDotBusy()
{
    ClearDotBusy();
    R_DOT* frontDot = m_frontPos.m_dot;
    if (frontDot)
        frontDot->m_attached = this;
    R_DOT* backDot = m_backPos.m_dot;
    if (backDot && backDot != frontDot)
        backDot->m_attached = this;
}

static void EngineRailMessage(ENGINE* engine, const char* messageText)
{
    STRING message(messageText);
    static_cast<DIALOG_UNIT*>(engine)->Error(E_ERROR, message, 0u);
}

static int EngineRailRecordLength(const R_DOT* railDot, unsigned int linkIndex)
{
    return railDot?railDot->m_links[linkIndex].length:0;
}

static R_DOT* EngineRailRecordDot(const R_DOT* railDot, unsigned int linkIndex)
{
    return railDot?railDot->m_links[linkIndex].dot:0;
}

static unsigned int EngineRailRecordBackLink(const R_DOT* railDot, unsigned int linkIndex)
{
    return railDot?static_cast<unsigned int>(railDot->m_links[linkIndex].reverseIndex):0u;
}

void ENGINE::PullTail(const R_POS& position)
{
    R_POS* const frontPosition = &m_frontPos;
    R_POS* const backPosition = &m_backPos;
    const int frontProgress = frontPosition->m_progress;
    const VID* const engineVid = m_vid;
    const WEAPON_RUNTIME* const weapon = engineVid?engineVid->weapon:0;
    const float trainLength = weapon?weapon->Length:0.0f;
    const int trainLengthInt = TruncateFloatToInt32(trainLength);
    if (trainLength == trainLength && static_cast<float>(frontProgress)>trainLength)
    {
        R_DOT* const frontDot = frontPosition->m_dot;
        const unsigned int frontLinkIndex = static_cast<unsigned int>(frontPosition->m_linkIndex);
        R_DOT* const nextDot = EngineRailRecordDot(frontDot, frontLinkIndex);
        const int segmentLength = EngineRailRecordLength(frontDot, frontLinkIndex);
        const unsigned int reverseLinkIndex = EngineRailRecordBackLink(frontDot, frontLinkIndex);
        backPosition->m_dot = nextDot;
        backPosition->m_progress = segmentLength-frontProgress;
        backPosition->m_progressFraction = 0u;
        backPosition->m_linkIndex = static_cast<int>(reverseLinkIndex);
        m_backPos.m_progress = trainLengthInt-frontProgress+segmentLength;
        return;
    }
    R_DOT* const frontDot = frontPosition->m_dot;
    R_DOT* backDot = backPosition->m_dot;
    if (backDot == frontDot)
    {
        m_backPos.m_progress = trainLengthInt-frontProgress;
        return;
    }
    R_DOT* const previousDot = position.m_dot;
    const int previousProgress = position.m_progress;
    const unsigned int previousLinkIndex = static_cast<unsigned int>(position.m_linkIndex);
    if (previousDot == frontDot)
    {
        int linkIndex = frontDot?frontDot->GetLink(backDot):-1;
        if (linkIndex<0)
        {
            EngineRailMessage(this, "rail 1");
            const ANGLE opposite(static_cast<unsigned char>(frontPosition->Direct().value+128u));
            linkIndex = frontDot?frontDot->GetLink(opposite):-1;
            backDot = EngineRailRecordDot(frontDot, static_cast<unsigned int>(linkIndex));
            backPosition->m_dot = backDot;
        }
        const int segmentLength = EngineRailRecordLength(frontDot, static_cast<unsigned int>(linkIndex));
        const int accumulatedProgress = frontProgress+segmentLength;
        if (trainLength == trainLength && static_cast<float>(accumulatedProgress)>trainLength)
        {
            if (static_cast<unsigned int>(frontPosition->m_linkIndex) == static_cast<unsigned int>(linkIndex))
            {
                m_backPos.m_progress = trainLengthInt-frontProgress+segmentLength;
                return;
            }
            backPosition->m_dot = frontDot;
            backPosition->m_linkIndex = linkIndex;
            m_backPos.m_progress = trainLengthInt-frontProgress;
            return;
        }
        m_backPos.m_progress = trainLengthInt-segmentLength-frontProgress;
        return;
    }
    if (previousDot == backDot)
    {
        const int segmentLength = EngineRailRecordLength(previousDot, previousLinkIndex);
        const int accumulatedProgress = frontProgress+segmentLength;
        if (trainLength == trainLength && static_cast<float>(accumulatedProgress)>trainLength)
        {
            R_DOT* const nextDot = EngineRailRecordDot(previousDot, previousLinkIndex);
            const int remaining = segmentLength-previousProgress;
            const unsigned int reverseLinkIndex = EngineRailRecordBackLink(previousDot, previousLinkIndex);
            backPosition->m_dot = nextDot;
            backPosition->m_progress = remaining;
            backPosition->m_progressFraction = 0u;
            backPosition->m_linkIndex = static_cast<int>(reverseLinkIndex);
            m_backPos.m_progress = trainLengthInt-frontProgress;
            return;
        }
        m_backPos.m_progress = trainLengthInt-frontProgress-segmentLength;
        return;
    }
    const int sourceSegmentLength = EngineRailRecordLength(previousDot, previousLinkIndex);
    const int accumulatedProgress = frontProgress+sourceSegmentLength;
    if (trainLength == trainLength && static_cast<float>(accumulatedProgress)>trainLength)
    {
        (void)::Error->Log("zmdots5");
        R_DOT* const nextDot = EngineRailRecordDot(previousDot, previousLinkIndex);
        const int remaining = sourceSegmentLength-previousProgress;
        const unsigned int reverseLinkIndex = EngineRailRecordBackLink(previousDot, previousLinkIndex);
        backPosition->m_dot = nextDot;
        backPosition->m_progress = remaining;
        backPosition->m_progressFraction = 0u;
        backPosition->m_linkIndex = static_cast<int>(reverseLinkIndex);
        m_backPos.m_progress = trainLengthInt-frontProgress;
        return;
    }
    (void)::Error->Log("zmdots6");
    const int linkIndex = previousDot?previousDot->GetLink(backDot):-1;
    backPosition->m_linkIndex = linkIndex;
    backPosition->m_dot = previousDot;
    m_backPos.m_progress = trainLengthInt-sourceSegmentLength-frontProgress;
    if (linkIndex<0)
    {
        EngineRailMessage(this, "rail 2");
        const ANGLE opposite(static_cast<unsigned char>(position.Direct().value+128u));
        backPosition->m_linkIndex = previousDot?previousDot->GetLink(opposite):-1;
    }
}

static float EngineAbsFloat(float value)
{
    return value<0.0f ? -value : value;
}

static int EngineCollisionDamage(const ENGINE* engine, int fallback)
{
    const VID* const engineVid = engine->Vid();
    if (engineVid->nVid == 97)
    {
        const unsigned int armyIndex = (engine->RuntimeFlags()>>17u)&7u;
        return engineVid->maxHpByPlayer[armyIndex]+10;
    }
    return fallback;
}

float ENGINE::Clash(ENGINE* otherEngine, int touchRelation)
{
    TRAIN_INFO selfTrainInfo(this);
    TRAIN_INFO otherTrainInfo(otherEngine);
    const float selfSpeed = EngineAbsFloat(m_speed);
    const float otherSpeed = EngineAbsFloat(otherEngine->m_speed);
    const float denominator = otherTrainInfo.totalLoadScalar+selfTrainInfo.totalLoadScalar;
    float trainSpeed = (otherTrainInfo.totalLoadScalar*otherSpeed+selfTrainInfo.totalLoadScalar*selfSpeed)/denominator;
    if (trainSpeed>0.001f && trainSpeed<0.01f)
        trainSpeed = 0.01f;
    const float impact = (touchRelation == 2 || touchRelation == 3) ? EngineAbsFloat(selfSpeed-otherSpeed) : selfSpeed+otherSpeed;
    if (touchRelation == 1 || touchRelation == 3)
        otherEngine->ReverseTrain();
    for (ENGINE* trainEngine = otherEngine->FirstEngine();trainEngine;trainEngine = trainEngine->m_nextEngine)
        trainEngine->m_speed = (trainEngine->m_trainFlags&1u)?-trainSpeed:trainSpeed;
    const float damageThreshold = Const->SafeClashSpeed;
    bool applyDamage = impact>damageThreshold;
    if (applyDamage)
    {
        const unsigned int selfCommand = m_runtimeFlags&0x7Cu;
        const unsigned int otherCommand = otherEngine->m_runtimeFlags&0x7Cu;
        const bool selfAttackCommand = selfCommand == 0x6Cu && otherEngine->InTrain(m_goalSprite) != 0;
        const bool otherAttackCommand = otherCommand == 0x6Cu && InTrain(otherEngine->Goal()) != 0;
        applyDamage = selfAttackCommand || otherAttackCommand;
    }
    if (!applyDamage)
    {
        static_cast<DIALOG_UNIT*>(this)->PlaySFX(19);
        return 0.0f;
    }
    ChangeAnimation(12);
    static_cast<DIALOG_UNIT*>(this)->PlaySFX(16);
    int damage = TruncateFloatToInt32(impact*1500.0f);
    damage = (damage-(damage>>31))/2;
    ENGINE* trainEngine = otherEngine;
    int currentDamage = damage;
    if (trainEngine)
    {
        trainEngine->Action(ACT_DAMAGE, EngineCollisionDamage(trainEngine, currentDamage), 0, 0);
        trainEngine = trainEngine->m_prevEngine;
        while (trainEngine && currentDamage >= 2)
        {
            currentDamage = (currentDamage-(currentDamage>>31))/2;
            trainEngine->Action(ACT_DAMAGE, currentDamage, 0, 0);
            trainEngine = trainEngine->m_prevEngine;
        }
    }
    trainEngine = this;
    currentDamage = damage;
    trainEngine->Action(ACT_DAMAGE, EngineCollisionDamage(trainEngine, currentDamage), 0, 0);
    trainEngine = trainEngine->m_prevEngine;
    if (!trainEngine)
        trainEngine = m_nextEngine;
    while (trainEngine && currentDamage >= 2)
    {
        currentDamage = (currentDamage-(currentDamage>>31))/2;
        trainEngine->Action(ACT_DAMAGE, currentDamage, 0, 0);
        trainEngine = m_prevEngine?trainEngine->m_prevEngine:trainEngine->m_nextEngine;
    }
    return 0.0f;
}

int ENGINE::GetTrainLengthInRails()
{
    int engineCount = 0;
    ENGINE* trainEngine = FirstEngine();
    if (trainEngine)
    {
        do
        {
            trainEngine = trainEngine->m_nextEngine;
            ++engineCount;
        }
        while (trainEngine);
    }
    const float railLength = static_cast<float>(engineCount)*(1.33000004f) +(0.5f);
    return TruncateFloatToInt32(railLength);
}

void ENGINE::MT_SpeedProcessing(float* currentSpeed)
{
    const float initialTarget = m_targetSpeed;
    const bool initialZeroOrUnordered = (initialTarget == (0.0f)) || (initialTarget != initialTarget);
    if (m_railState != 0u && initialZeroOrUnordered)
    {
        *currentSpeed = 0.035f;
        m_acceleration = 0;
        return;
    }
    const float targetSpeed = initialTarget;
    if (targetSpeed>*currentSpeed)
    {
        if (static_cast<unsigned int>(m_acceleration) == 0u)
            ReCalcMoveParameters();
        const int accelerationStep = m_acceleration;
        if (accelerationStep != 0)
        {
            const unsigned int elapsedTicks = ::CurrentTime-::PrevCurrentTime;
            const unsigned int scaledAcceleration = elapsedTicks*static_cast<unsigned int>(accelerationStep);
            const float unsignedAcceleration = static_cast<float>(static_cast<unsigned long long>(scaledAcceleration));
            *currentSpeed += unsignedAcceleration*(9.99999997e-07f)*2.0f;
        }
        if (*currentSpeed >= targetSpeed)
        {
            *currentSpeed = targetSpeed;
            m_acceleration = 0;
        }
        return;
    }
    const bool targetSpeedUnordered = (targetSpeed != targetSpeed) || (*currentSpeed != *currentSpeed);
    if (targetSpeed<*currentSpeed || targetSpeedUnordered)
    {
        const int accelerationStep = TruncateFloatToInt32((*currentSpeed*(1000.0f)+ (10.0f))*(-0.5f));
        m_acceleration = accelerationStep;
        const unsigned int elapsedTicks = ::CurrentTime-::PrevCurrentTime;
        const int scaledAcceleration = static_cast<int>(elapsedTicks*static_cast<unsigned int>(accelerationStep));
        *currentSpeed += static_cast<float>(scaledAcceleration)*(9.99999997e-07f)*2.0f;
        if (*currentSpeed>targetSpeed)
            return;
        *currentSpeed = targetSpeed;
    }
    m_acceleration = 0;
}

int ENGINE::CanLinkWithEngine(ENGINE* otherEngine)
{
    if (!otherEngine)
        return 0;
    if (otherEngine->InTrain(m_goalSprite) && (m_runtimeFlags&0x7Cu) == 0x68u)
        return 1;
    if (InTrain(otherEngine->m_goalSprite) && (otherEngine->m_runtimeFlags&0x7Cu) == 0x68u)
        return 1;
    if ((m_railState != 0u || otherEngine->m_railState != 0u) && ((otherEngine->m_runtimeFlags^m_runtimeFlags)&0x000E0000u) == 0u)
        return 1;
    return 0;
}

int ENGINE::MT_IntersectingProcessing(const R_POS& position, float* currentSpeed)
{
    int touchRelation = -1;
    ENGINE* otherEngine = GetForwardIntersecting(&touchRelation);
    if (otherEngine)
    {
        m_frontPos = position;
        if (CanLinkWithEngine(otherEngine) == 0)
        {
            *currentSpeed = Clash(otherEngine, touchRelation);
            (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_CLASH], this, otherEngine, 0);
            return 1;
        }
        if (otherEngine->m_nextEngine || touchRelation == 1 || touchRelation == 4)
            otherEngine->ReverseTrain();
        const unsigned int selfFlags = m_runtimeFlags;
        const unsigned int otherFlags = otherEngine->m_runtimeFlags;
        if (((selfFlags^otherFlags)&0x000E0000u) == 0u)
        {
            bool actionOnOther = false;
            if ((selfFlags&0x7Cu) == 0x68u && otherEngine->InTrain(m_goalSprite))
            {
                if ((otherFlags&0x7Cu) != 0x68u || !InTrain(otherEngine->m_goalSprite))
                    actionOnOther = true;
            }
            if (actionOnOther)
                otherEngine->Action(ACT_BACKUP_COMMAND, 0, 0, 0);
            else if (!((selfFlags&0x7Cu) == 0x68u && (otherFlags&0x7Cu) == 0x68u && otherEngine->InTrain(m_goalSprite) && InTrain(otherEngine->m_goalSprite)))
                Action(ACT_BACKUP_COMMAND, 0, 0, 0);
        }
        if (otherEngine->m_nextEngine)
        {
            STRING message("can't link");
            static_cast<DIALOG_UNIT*>(this)->Error(E_ERROR, message, 0u);
        }
        else
        {
            otherEngine->m_nextEngine = this;
            m_prevEngine = otherEngine;
        }
        SetCommandToTrain(0, 0, 0, 0);
        ChangeAnimation(11);
        if (((m_runtimeFlags^otherEngine->m_runtimeFlags)&0x000E0000u) != 0u)
        {
            for (ENGINE* trainEngine = FirstEngine();trainEngine;trainEngine = trainEngine->m_nextEngine)
                if ((trainEngine->m_runtimeFlags&0x000E0000u) != 0u) trainEngine->DeleteAttackToEngine();
            ENGINE* eventEngine = ((otherEngine->m_runtimeFlags&0x000E0000u) == 0x00020000u)?otherEngine:this;
            (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_ENEMY_LINKED], eventEngine, 0, 0);
        }
        return 0;
    }
    if (GetBadIntersecting())
    {
        if (*currentSpeed>0.2f)
            static_cast<DIALOG_UNIT*>(this)->PlaySFX(148);
        m_frontPos = position;
        *currentSpeed = 0.0f;
        m_targetSpeed = -m_targetSpeed;
        return 1;
    }
    return 0;
}

void ENGINE::DeleteAttackToEngine()
{
    SPRITE_COLLECTOR* const collector = ::Sprites;
    int unitCursor = collector->reserved2EC;
    SPRITE* unit = collector->FirstUnit(&unitCursor, -1);
    collector->reserved2EC = unitCursor;
    while (unit)
    {
        SPRITE* commandTarget = unit;
        if (unit->Goal() == this)
        {
            VID* const owner = unit->Vid();
            const unsigned int command = unit->RuntimeFlags()&0x7Cu;
            if (owner->spriteClass == 21u && (command == 0x70u || command == 0x74u))
            {
                static_cast<ENGINE*>(unit)->SetCommandToTrain(0, 0, 0, 0);
                commandTarget = 0;
            }
        }
        else
        {
            SPRITE* const linked = unit->ChildChain();
            if (linked && linked->Goal() == this)
                commandTarget = linked;
            else commandTarget = 0;
        }
        if (commandTarget)
        {
            const unsigned int command = commandTarget->RuntimeFlags()&0x7Cu;
            if (command == 0x14u || command == 0x0Cu || command == 0x10u || (commandTarget != unit && (command == 0x70u || command == 0x74u)))
                (void)commandTarget->SetCommand(0, static_cast<SPRITE*>(0));
        }
        unitCursor = collector->reserved2EC;
        unit = collector->NextUnit(&unitCursor);
        collector->reserved2EC = unitCursor;
    }
}

void ENGINE::CreatePathDots(R_DOT* startDot)
{
    CreatePathDots(startDot, PathDots.Address(), 603);
    for (int markerIndex = 0;markerIndex<PathDots.Get().m_count;++markerIndex)
    {
        const unsigned int command = m_runtimeFlags&0x7Cu;
        unsigned int markerArmy = 0u;
        if (command == 0x70u || command == 0x74u)
            markerArmy = 1u;
        else if (command == 0x6Cu)
            markerArmy = 3u;
        else if (command == 0x68u)
            markerArmy = 2u;
        if (markerArmy)
        {
            SPRITE* const marker = PathDots.Get().m_data[markerIndex];
            marker->ChangeArmy(static_cast<int>(markerArmy));
        }
    }
}

void ENGINE::CreatePathDots(R_DOT* pathDot, SPRITE_LIST* markers, int markerVidId)
{
    NoStepForNotFound = R_DOT::NoStepForNotFound;
    NoStep = R_DOT::NoStep;
    markers->DeleteAll();
    const int pathLength = m_pathCount;
    for (int pathIndex = 0;pathIndex<pathLength;++pathIndex)
    {
        const unsigned int linkIndex = m_pathLinks[static_cast<unsigned int>(pathIndex)];
        if (linkIndex >= static_cast<unsigned int>(pathDot->m_linkCount))
            continue;
        pathDot = pathDot->m_links[linkIndex].dot;
        VECTOR position(static_cast<float>(pathDot->m_x), static_cast<float>(pathDot->m_y), static_cast<float>(pathDot->m_z));
        VID* const vid = Map->Vid(markerVidId);
        SPRITE* const markerSprite = Map->CreateSprite(vid, position, ANGLE(static_cast<unsigned char>(0u)), 0);
        markers->Insert(markerSprite);
    }
}

int ENGINE::IsTailInFindedPath(R_DOT* startDot)
{
    ENGINE* const lastEngine = LastEngine();
    const R_POS& backPosition = lastEngine->m_backPos;
    R_DOT* tailNextDot = 0;
    R_DOT* const backDot = backPosition.m_dot;
    if (backDot)
    {
        const unsigned int linkIndex = static_cast<unsigned int>(backPosition.m_linkIndex);
        tailNextDot = backDot->m_links[linkIndex].dot;
    }
    R_DOT* pathDot = startDot;
    const int pathLength = m_pathCount;
    for (int pathIndex = 0;pathIndex<pathLength;++pathIndex)
    {
        const unsigned int linkIndex = m_pathLinks[static_cast<unsigned int>(pathIndex)];
        if (static_cast<int>(linkIndex)<pathDot->m_linkCount)
        {
            pathDot = pathDot->m_links[linkIndex].dot;
            if (pathDot == tailNextDot)
                return 1;
        }
    }
    return 0;
}

int ENGINE::ReachTheTarget()
{
    SPRITE* const goal = m_goalSprite;
    if (!goal)
        return 2;
    const unsigned int kind = m_runtimeFlags&0x7Cu;
    if (kind != 0x70u && kind != 0x74u)
        return 2;
    const float margin = (10.0f);
    SPRITE* const selected = m_commandOwner;
    if (selected)
    {
        VID* owner = selected->Vid();
        SPRITE* attached = selected->ChildChain();
        bool linked = false;
        if (attached)
        {
            VID* const attachedOwner = attached->Vid();
            linked = (attachedOwner == owner->linkVid && attachedOwner->nWeapon != 0 && attachedOwner->childVid[8] != 0);
        }
        if (linked)
        {
            SPRITE* const object = static_cast<DIALOG_UNIT*>(static_cast<ENGINE*>(selected))->Link();
            VECTOR2 delta(goal->X()-object->X(), goal->Y()-object->Y());
            const float distance = delta.NearLength();
            owner = object->Vid();
            const float limit = owner->weapon->BattleRange-margin;
            return (limit == limit && distance == distance && limit >= distance)?1:0;
        }
        if (owner->nVid != 35)
            return 2;
        VECTOR2 delta(goal->X()-selected->X(), goal->Y()-selected->Y());
        const float distance = delta.NearLength();
        const float limit = owner->weapon->BattleRange-margin;
        return (limit == limit && distance == distance && limit >= distance)?1:0;
    }
    int result = 2;
    ENGINE* node = FirstEngine();
    while (node)
    {
        VID* owner = node->m_vid;
        SPRITE* attached = node->m_childChain;
        bool linked = false;
        if (attached)
        {
            VID* const attachedOwner = attached->Vid();
            linked = (attachedOwner == owner->linkVid && attachedOwner->nWeapon != 0 && attachedOwner->childVid[8] != 0);
        }
        if (linked)
        {
            SPRITE* const object = static_cast<DIALOG_UNIT*>(node)->Link();
            VECTOR2 delta(goal->X()-object->X(), goal->Y()-object->Y());
            const float distance = delta.NearLength();
            owner = object->Vid();
            const float limit = owner->weapon->BattleRange-margin;
            if (!(limit == limit && distance == distance && limit >= distance))
                return 0;
            result = 1;
        }
        else if (owner->nVid == 35)
        {
            VECTOR2 delta(goal->X()-node->X(), goal->Y()-node->Y());
            const float distance = delta.NearLength();
            const float limit = owner->weapon->BattleRange-margin;
            if (distance == distance && limit == limit && distance>limit)
                return 0;
            result = 1;
        }
        node = node->m_nextEngine;
    }
    return result;
}

void ENGINE::MoveEngineTact()
{
    R_POS oldPosition = m_frontPos;
    R_POS* const position = &m_frontPos;
    R_DOT* root = position->m_dot;
    if (!root || !m_backPos.m_dot || m_prevEngine)
        return;
    const int ownerType = m_vid->nVid;
    if (ownerType != 85)
    {
        R_DOT* const next = position->Dot2();
        const unsigned int direction = (m_runtimeFlags>>17u)&7u;
        if (next && next->m_armyRailClass-4 == static_cast<int>(direction))
        {
            const unsigned int link = static_cast<unsigned int>(position->m_linkIndex);
            const int segmentLength = root->m_links[link].length;
            const int halfLength = (segmentLength-(segmentLength>>31))/2;
            if (m_frontPos.m_progress>halfLength)
            {
                Stop();
                m_speed = 0.0f;
                ReverseTrain();
                return;
            }
        }
    }
    int anyBit0 = 0;
    ENGINE* trainNode = this;
    while (trainNode)
    {
        R_DOT* const nodeRoot = reinterpret_cast<R_DOT*>(trainNode->m_frontPos.m_dot);
        if (nodeRoot && nodeRoot->m_selectedLink == trainNode->m_frontPos.m_linkIndex && nodeRoot->m_010 != 0u)
        {
            for (ENGINE* node = this;node;node = node->m_nextEngine)
                node->m_railState = 1u;
            break;
        }
        bool reciprocal = false;
        if (nodeRoot)
        {
            const unsigned int index = static_cast<unsigned int>(trainNode->m_frontPos.m_linkIndex);
            R_DOT* const peer = nodeRoot->m_links[index].dot;
            if (peer)
            {
                const unsigned int reverseRecord = static_cast<unsigned int>(nodeRoot->m_links[index].reverseIndex);
                reciprocal = peer->m_selectedLink == static_cast<int>(reverseRecord);
            }
        }
        if (reciprocal)
        {
            const float currentMove = m_speed;
            if (currentMove != 0.0f && currentMove == currentMove)
            {
                unsigned int flags = m_runtimeFlags;
                const unsigned int kind = flags&0x7Cu;
                if (kind == 0x5Cu)
                {
                    bool issueStopCommand = static_cast<unsigned int>(m_pathCount) == 0u;
                    if (!issueStopCommand)
                    {
                        R_DOT* const currentNext = position->Dot2();
                        root = position->m_dot;
                        const unsigned int pathIndex = m_pathLinks[0];
                        issueStopCommand = root && root->m_links[pathIndex].dot == currentNext;
                    }
                    if (issueStopCommand)
                        SetCommandToTrain(0, 0, 0, 0);
                    else m_runtimeFlags = flags&~0x400u;
                }
                else
                {
                    m_runtimeFlags = flags&~0x400u;
                }
                m_speed = 0.0f;
                static_cast<DIALOG_UNIT*>(this)->PlaySFX(148);
            }
            if (m_targetSpeed>0.0f)
                m_targetSpeed = -m_targetSpeed;
            ReverseTrain();
            return;
        }
        trainNode->m_railState = 0u;
        anyBit0 |= static_cast<int>(trainNode->m_runtimeFlags&1u);
        trainNode = trainNode->m_nextEngine;
    }
    if (!trainNode && anyBit0)
    {
        const unsigned int direction = (m_runtimeFlags>>17u)&7u;
        for (ENGINE* node = this;node;node = node->m_nextEngine)
        {
            if ((node->m_runtimeFlags&1u) == 0u)
                continue;
            node->m_runtimeFlags &= ~1u;
            VID* const nodeOwner = node->m_vid;
            WEAPON_RUNTIME* const stats = nodeOwner->weapon;
            const float activity = stats->Power;
            if (activity != 0.0f && activity == activity)
            {
                PLAYER* const handler = Map->m_players[direction];
                handler->AddUnitToStateBar(node);
            }
            if (static_cast<unsigned int>(node->m_depotBatchNotificationPending) != 0u)
            {
                ::Map->ScriptRun( EvFunctionNumber[MAP_EVENT_TRAIN_CREATED], node, 0, 0);
                node->m_depotBatchNotificationPending = 0;
            }
        }
    }
    if (m_railState == 0u)
    {
        unsigned int flags = m_runtimeFlags;
        unsigned int kind = flags&0x7Cu;
        const float currentMove = m_speed;
        if ((flags&0x400u) == 0u && (currentMove == 0.0f || currentMove != currentMove))
        {
            if (kind == 0x5Cu || kind == 0x68u || kind == 0x6Cu || kind == 0x64u)
                StartMove();
            flags = m_runtimeFlags;
            kind = flags&0x7Cu;
            if (kind == 0x60u)
            {
                ENGINE* node = FirstEngine();
                while (node)
                {
                    if (node->m_vid->nVid == 85 && static_cast<unsigned int>(node->m_mineDeploymentReady) == 0u)
                    {
                        StartMove();
                        break;
                    }
                    node = node->m_nextEngine;
                }
            }
        }
        flags = m_runtimeFlags;
        kind = flags&0x7Cu;
        if ((flags&0x400u) == 0u && (kind == 0x70u || kind == 0x74u))
        {
            const bool localRandom = (m_vid->runtimeFlags&0x1000u) != 0u;
            const int choice = localRandom ? ::Random(4) : ::Random2(4);
            if (choice == 0 && ReachTheTarget() != 1)
                StartMove();
        }
        const float currentTargetSpeed = m_targetSpeed;
        if ((currentTargetSpeed == 0.0f || currentTargetSpeed != currentTargetSpeed) && (m_runtimeFlags&0x400u) != 0u)
        {
            ReCalcMoveParameters();
            const unsigned int currentKind = m_runtimeFlags&0x7Cu;
            const float recalculatedSpeed = m_targetSpeed;
            const float recalculatedMove = m_speed;
            if ((recalculatedSpeed == 0.0f || recalculatedSpeed != recalculatedSpeed) && (recalculatedMove == 0.0f || recalculatedMove != recalculatedMove) && (currentKind == 0x5Cu || currentKind == 0x68u || currentKind == 0x6Cu || currentKind == 0x64u))
                SetCommandToTrain(0, 0, 0, 0);
        }
    }
    SPRITE* const movementTarget = m_commandDot ? 0 : m_goalSprite;
    float speed = m_speed;
    if (speed<0.0f)
        speed = -speed;
    MT_SpeedProcessing(&speed);
    if (speed<0.0f || speed != speed)
    {
        m_speed = 0.0f;
        ReverseTrain();
        ReCalcMoveParameters();
        return;
    }
    root = position->m_dot;
    if (root->m_broken != 0u)
    {
        const float limit = Const->RailRepairSpeed;
        if (speed>limit)
        {
            if (m_vid->nVid != 85)
                speed = limit;
            DIALOG_UNIT* const embedded = static_cast<DIALOG_UNIT*>(this);
            const VECTOR& spawnPosition = embedded->Coor();
            VID* const effectVid = Map->Vid(588);
            (void)Map->CreateSprite(effectVid, spawnPosition, ANGLE(static_cast<unsigned char>(0u)), this);
        }
    }
    const int tickDelta = static_cast<int>(::CurrentTime-::PrevCurrentTime);
    int fraction = static_cast<int>(m_frontPos.m_progressFraction)+TruncateFloatToInt32(static_cast<float>(tickDelta)*speed*64000.0f);
    m_frontPos.m_progressFraction = static_cast<unsigned int>(fraction);
    if (fraction<0)
    {
        m_frontPos.m_progressFraction = 0u;
    }
    else if (fraction>65535)
    {
        m_frontPos.m_progress += fraction>>16;
        m_frontPos.m_progressFraction = static_cast<unsigned int>(fraction)&0xFFFFu;
    }
    root = position->m_dot;
    const unsigned int currentLink = static_cast<unsigned int>(position->m_linkIndex);
    const int segmentLength = root ? root->m_links[currentLink].length : 0;
    if (m_frontPos.m_progress>segmentLength)
    {
        if (root->m_broken != 0u)
        {
            SPRITE_COLLECTOR* const collector = ::Sprites;
            const float x = static_cast<float>(root->m_x);
            const float y = static_cast<float>(root->m_y);
            for (SPRITE* candidate = collector->FirstHashInBox(x-100.0f, y-60.0f, x+100.0f, y+60.0f); candidate;candidate = collector->NextHashInBox())
            {
                if (candidate->Vid()->spriteClass == 22)
                    reinterpret_cast<RAIL*>(candidate)->UnBreak(root);
            }
            root->m_broken = 0u;
        }
        R_DOT* nextDot = position->Dot2();
        if (nextDot->m_linkCount<2)
        {
            speed = 0.0f;
            m_frontPos.m_progress = segmentLength;
            const unsigned int kind = m_runtimeFlags&0x7Cu;
            if (kind == 0x5Cu)
            {
                bool issueCommand = static_cast<unsigned int>(m_pathCount) == 0u;
                if (!issueCommand)
                {
                    root = position->m_dot;
                    const unsigned int pathIndex = m_pathLinks[0];
                    issueCommand = root->m_links[pathIndex].dot == nextDot;
                }
                if (issueCommand)
                    SetCommandToTrain(0, 0, 0, 0);
                else Stop();
            }
            else
            {
                Stop();
            }
        }
        else
        {
            const int stepResult = position->DoStep( m_commandDot, movementTarget, this);
            MAP* const world = Map;
            SPRITE* probe = world->Flagman(world->m_playerIndex);
            if (InTrain(probe))
            {
                probe = world->Flagman(world->m_playerIndex);
                if ((probe->RuntimeFlags()&0x000E0000u) == 0u)
                    CreatePathDots(position->m_dot);
            }
            unsigned int kind = m_runtimeFlags&0x7Cu;
            if (kind != 0x6Cu)
            {
                ENGINE* const occupant = static_cast<ENGINE*>(position->m_dot->m_attached);
                if (occupant)
                    (void)occupant->InTrain(m_goalSprite);
            }
            nextDot = position->Dot2();
            ENGINE* const nextOccupant = static_cast<ENGINE*>(nextDot->m_attached);
            const bool skipStepSign = nextOccupant && nextOccupant->InTrain(m_goalSprite) != 0;
            if (!skipStepSign && kind == 0x6Cu && stepResult<0 && m_targetSpeed>0.0f)
                m_targetSpeed = -m_targetSpeed;
            {
                if (m_vid->nVid != 85 && nextDot && nextDot->m_armyRailClass-4 == static_cast<int>((m_runtimeFlags>>17u)&7u))
                {
                    speed *= 0.5f;
                    Stop();
                }
                const bool targetReached = (m_commandDot != 0u && m_commandDot == nextDot) || stepResult == 0 || nextDot == R_DOT::FindedDot;
                if (targetReached)
                {
                    kind = m_runtimeFlags&0x7Cu;
                    if (kind == 0x60u)
                    {
                        for (ENGINE* node = this;node;node = node->m_nextEngine)
                        {
                            if (node->m_vid->nVid == 85)
                            {
                                node->m_mineDeploymentReady = 1;
                                Stop();
                            }
                        }
                        if ((m_runtimeFlags&0x400u) != 0u)
                            SetCommandToTrain(0, 0, 0, 0);
                    }
                    else if (kind == 0x5Cu)
                    {
                        const bool protectedDirection = m_vid->nVid == 85 && nextDot && nextDot->m_armyRailClass-4 == static_cast<int>((m_runtimeFlags>>17u)&7u);
                        if (!protectedDirection)
                        {
                            SetCommandToTrain(0, 0, 0, 0);
                            ::Map->ScriptRun( EvFunctionNumber[MAP_EVENT_TRAIN_ARRIVE], this, 0, 0);
                        }
                    }
                    else if (kind == 0x64u)
                    {
                        Stop();
                        StartMove();
                    }
                    else if ((kind == 0x70u || kind == 0x74u) && ReachTheTarget() == 1)
                    {
                        Stop();
                    }
                }
                else if (kind == 0x70u || kind == 0x74u)
                {
                    if (ReachTheTarget() == 1)
                        Stop();
                }
                else
                {
                    root = position->m_dot;
                    if (root->m_linkCount<2 || nextDot->m_linkCount<2)
                    {
                        if (kind == 0x5Cu)
                        {
                            bool issueCommand = static_cast<unsigned int>(m_pathCount) == 0u;
                            if (!issueCommand)
                            {
                                const unsigned int pathIndex = m_pathLinks[0];
                                issueCommand = root->m_links[pathIndex].dot == nextDot;
                            }
                            if (issueCommand)
                                SetCommandToTrain(0, 0, 0, 0);
                            else Stop();
                        }
                        else
                        {
                            Stop();
                        }
                    }
                    else
                    {
                        ENGINE* const occupant = static_cast<ENGINE*>(nextDot->m_attached);
                        if (occupant && occupant->InTrain(m_goalSprite) == 0)
                            Stop();
                    }
                }
            }
        }
    }
    (void)MT_IntersectingProcessing(oldPosition, &speed);
    for (ENGINE* node = FirstEngine();node;node = node->m_nextEngine)
        node->ClearDotBusy();
    MT_PullCoordinates(&oldPosition, speed, m_acceleration);
}

TRAIN_INFO::TRAIN_INFO(const ENGINE* engine)
{
    flags00 &= ~3u;
    maxWeaponRange = 0.0f;
    minWeaponRange = 999999.0f;
    totalDriveScalar = 0.0f;
    totalLoadScalar = 0.0f;
    poweredLoadScalar = 0.0f;
    maxSpeedThousandths = 10000;
    engineCount = 0;
    currentHp = 0;
    maxHp = 0;
    fireDamage = 0;
    buildTime = 0;
    ammoEngineCount = 0;
    ammoCount = 0;
    ammoCapacity = 0;
    averageAmmoPercent = 0;
    const ENGINE* node = engine;
    while (node)
    {
        AddEngine(node);
        node = node->m_nextEngine;
    }
    node = engine->m_prevEngine;
    while (node)
    {
        AddEngine(node);
        node = node->m_prevEngine;
    }
    averageAmmoPercent = (ammoEngineCount != 0)?(averageAmmoPercent/ammoEngineCount):100;
    if (FloatBitsU32(minWeaponRange) == 0x497423F0u)
        minWeaponRange = 0.0f;
    const float denominator = totalDriveScalar-poweredLoadScalar;
    if (denominator == denominator && denominator != 0.0f)
    {
        const float numerator = denominator-(totalLoadScalar-poweredLoadScalar);
        maxSpeedThousandths = TruncateFloatToInt32((numerator*static_cast<float>(maxSpeedThousandths))/denominator);
        if (maxSpeedThousandths<5)
            maxSpeedThousandths = 0;
    }
    if (maxSpeedThousandths == 10000)
        maxSpeedThousandths = 0;
}

extern "C" void*  ConstructTrainInfoAtAddress(void* self, const ENGINE* engine)
{
    ::new(self) TRAIN_INFO(engine);
    return self;
}

void TRAIN_INFO::AddEngine(const ENGINE* engine)
{
    if (engine->Vid()->weapon->Power != 0.0f && engine->Vid()->weapon->Power == engine->Vid()->weapon->Power)
    {
        const double candidate = static_cast<double>(engine->MaxSpeed())*1000.0;
        if (static_cast<double>(maxSpeedThousandths)>candidate)
            maxSpeedThousandths = TruncateDoubleToInt32(static_cast<double>(engine->MaxSpeed())*1000.0);
    }
    totalDriveScalar += engine->Vid()->weapon->Power;
    totalLoadScalar += engine->Vid()->weapon->Weight;
    if (engine->Vid()->weapon->Power>0.0f)
        poweredLoadScalar += engine->Vid()->weapon->Weight;
    if (engine->Vid()->nVid != 45)
        flags00 |= 1u;
    else flags00 |= 2u;
    currentHp = AddSigned32Wrap(currentHp, engine->m_hp);
    const int ammoCount = engine->m_ammoFixedPoint/64;
    const VID* engineVid = engine->Vid();
    const VID* linkedVid = engineVid->linkVid;
    int ammoCapacity;
    if (linkedVid && linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0)
        ammoCapacity = linkedVid->weapon->MaxAmmo;
    else ammoCapacity = engineVid->weapon->MaxAmmo;
    if (ammoCount>0)
    {
        float weaponRange = engineVid->weapon->BattleRange;
        const SPRITE* const child = engine->m_childChain;
        if (child && child->Vid() == linkedVid && linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0 && (weaponRange == 0.0f || weaponRange != weaponRange))
            weaponRange = linkedVid->weapon->BattleRange;
        if (weaponRange>maxWeaponRange)
            maxWeaponRange = weaponRange;
        if (weaponRange != 0.0f && weaponRange == weaponRange && weaponRange<minWeaponRange)
            minWeaponRange = weaponRange;
    }
    if (ammoCapacity != 0 && ammoCapacity != 999999 && engine->Vid()->nVid != 85)
    {
        this->ammoCapacity = AddSigned32Wrap(this->ammoCapacity, ammoCapacity);
        ammoEngineCount = AddSigned32Wrap(ammoEngineCount, 1);
        this->ammoCount = AddSigned32Wrap(this->ammoCount, ammoCount);
        averageAmmoPercent = AddSigned32Wrap(averageAmmoPercent, MultiplySigned32Wrap(ammoCount, 100)/ammoCapacity);
    }
    unsigned int armyIndex = (engine->m_runtimeFlags>>17u)&7u;
    maxHp = AddSigned32Wrap(maxHp, engine->Vid()->maxHpByPlayer[armyIndex]);
    buildTime = AddSigned32Wrap(buildTime, engine->Vid()->GetBuildTime());
    int fireDamage = 0;
    if (ammoCount != 0)
    {
        if (engine->Vid()->nVid == 82)
        {
            MAP* const world = Map;
            const VID* fireVid = world->VidCount()>70?world->StoredVid(70):0;
            if (!fireVid)
                fireVid = ::EmptyVid;
            fireDamage = fireVid->GetFireDamageMax();
        }
        else fireDamage = engine->GetFireDamage();
    }
    this->fireDamage = AddSigned32Wrap(this->fireDamage, fireDamage);
    engineVid = engine->Vid();
    linkedVid = engineVid->linkVid;
    if (linkedVid)
    {
        armyIndex = (engine->m_runtimeFlags>>17u)&7u;
        maxHp = AddSigned32Wrap(maxHp, linkedVid->maxHpByPlayer[armyIndex]);
        const SPRITE* const child = engine->m_childChain;
        if (!child || child->Vid() != engine->Vid()->linkVid)
        {
            armyIndex = (engine->m_runtimeFlags>>17u)&7u;
            engineVid = engine->Vid();
            linkedVid = engineVid->linkVid;
            if (static_cast<int>(linkedVid->spriteCountsByPlayer[armyIndex]) >= static_cast<int>(engineVid->spriteCountsByPlayer[armyIndex]))
                currentHp = AddSigned32Wrap(currentHp, linkedVid->maxHpByPlayer[armyIndex]);
        }
    }
    const SPRITE* const child = engine->m_childChain;
    if (child && child->Vid() == engine->Vid()->linkVid)
    {
        const DIALOG_UNIT* const dialog = static_cast<const DIALOG_UNIT*>(engine);
        totalLoadScalar += dialog->Link()->Vid()->weapon->Weight;
        if (dialog->Link()->Vid()->spriteClass != 9)
            currentHp = AddSigned32Wrap(currentHp, dialog->Link()->m_hp);
        if (engine->Vid()->weapon->Power>0.0f)
            poweredLoadScalar += dialog->Link()->Vid()->weapon->Weight;
    }
    engineCount = AddSigned32Wrap(engineCount, 1);
}

void ENGINE::CheckPrevNextEngine()
{
    if (m_prevEngine && m_prevEngine->m_nextEngine != this)
    {
        {
            STRING text("PrevEngine->NextEngine!=this");
            DIALOG_UNIT* const channel = static_cast<DIALOG_UNIT*>(this);
            channel->Error(E_INVALID, text, PtrToU32(m_prevEngine->m_nextEngine));
        }
        m_prevEngine->m_nextEngine = this;
    }
    if (m_nextEngine && m_nextEngine->m_prevEngine != this)
    {
        {
            STRING text("NextEngine->PrevEngine!=this");
            DIALOG_UNIT* const channel = static_cast<DIALOG_UNIT*>(this);
            channel->Error(E_INVALID, text, PtrToU32(m_nextEngine->m_prevEngine));
        }
        m_nextEngine->m_prevEngine = this;
    }
}

int ENGINE::IsCommandToAllTrain() const
{
    VID* const owner = m_vid;
    if (owner->nVid == 45)
        return 1;
    const WEAPON_RUNTIME* const stats = owner->weapon;
    const float value = stats->Power;
    const float zero = (0.0f);
    return (value == value && value != zero)?1:0;
}

void ENGINE::AddAmmoTact()
{
    ENGINE* reloadEngine = 0;
    int highestNeed = 0;
    ENGINE* trainEngine = FirstEngine();
    while (trainEngine)
    {
        if (((trainEngine->m_runtimeFlags^m_runtimeFlags)&0x000E0000u) == 0u)
        {
            const int ammoNeed = trainEngine->NeedAddAmmo();
            if (ammoNeed>highestNeed && trainEngine->m_vid->nVid != 85)
            {
                highestNeed = ammoNeed;
                reloadEngine = trainEngine;
            }
        }
        trainEngine = trainEngine->m_nextEngine;
    }
    if (reloadEngine)
    {
        (void)reinterpret_cast<UNIT*>(reloadEngine)->AddAmmoTick(Const->AmmoReloadTime);
        if (m_currentAnimation != 6)
            ChangeAnimation(6);
    }
    else if (m_currentAnimation == 6)
    {
        ChangeAnimation(0);
    }
}

int ENGINE::RepairByRepair(ENGINE* repairTarget)
{
    if (!repairTarget)
        return 0;
    const int repairAmount = -Const->RepairSpeed;
    const int repairAccepted = repairTarget->Action(ACT_DAMAGE, repairAmount, 0, 0) == 0?1:0;
    if (repairAccepted)
        return 1;
    VID* const targetVid = repairTarget->m_vid;
    VID* const linkedVid = targetVid->linkVid;
    if (!linkedVid)
        return 0;
    SPRITE* const linkedSprite = repairTarget->m_childChain;
    if (linkedSprite && linkedSprite->Vid() == linkedVid)
        return 0;
    int repairProgress = repairTarget->m_repairProgress;
    if (repairProgress<0)
    {
        repairProgress = 0;
        repairTarget->m_repairProgress = 0;
    }
    repairProgress += Const->RepairSpeed;
    repairTarget->m_repairProgress = repairProgress;
    const unsigned int armyIndex = (repairTarget->m_runtimeFlags>>17)&7u;
    if (repairProgress >= linkedVid->maxHpByPlayer[armyIndex])
    {
        (void)repairTarget->Repair(1);
        m_repairProgress = -1;
    }
    return 0;
}

void ENGINE::RepairTact()
{
    ENGINE* repairTarget = 0;
    int highestNeed = 0;
    ENGINE* trainEngine = FirstEngine();
    while (trainEngine)
    {
        if (((trainEngine->m_runtimeFlags^m_runtimeFlags)&0x000E0000u) == 0u)
        {
            const int repairNeed = trainEngine->NeedRepairByRepair();
            if (repairNeed>highestNeed)
            {
                highestNeed = repairNeed;
                repairTarget = trainEngine;
            }
        }
        trainEngine = trainEngine->m_nextEngine;
    }
    if (repairTarget)
    {
        (void)RepairByRepair(repairTarget);
        if (m_currentAnimation != 6)
            ChangeAnimation(6);
    }
    else if (m_currentAnimation == 6)
    {
        ChangeAnimation(0);
    }
}

int ENGINE::NeedRepairByRepair()
{
    const unsigned int armyIndex = (m_runtimeFlags>>17)&7u;
    VID* const owner = m_vid;
    const int maximumHp = owner->maxHpByPlayer[armyIndex];
    const int currentHp = m_hp;
    VID* const linkedOwner = owner->linkVid;
    if (currentHp<maximumHp)
    {
        if (linkedOwner)
        {
            SPRITE* linkedSprite = m_childChain;
            if (!linkedSprite || linkedSprite->Vid() != linkedOwner)
            {
                const int denominator = maximumHp+linkedOwner->maxHpByPlayer[armyIndex];
                return 100-(currentHp*100/denominator);
            }
        }
        return 100-(currentHp*100/maximumHp);
    }
    if (linkedOwner)
    {
        SPRITE* linkedSprite = m_childChain;
        if (!linkedSprite || linkedSprite->Vid() != linkedOwner)
            return owner->nVid != 35?50:0;
    }
    SPRITE* linkedSprite = m_childChain;
    if (!linkedSprite || linkedSprite->Vid() != linkedOwner)
        return 0;
    DIALOG_UNIT* const dialog = static_cast<DIALOG_UNIT*>(this);
    SPRITE* classProbe = dialog->Link();
    if (classProbe->Vid()->spriteClass == 9)
        return 0;
    SPRITE* hpProbe = dialog->Link();
    const int linkedHp = hpProbe->m_hp;
    SPRITE* armyProbe = dialog->Link();
    const unsigned int linkedArmyIndex = (armyProbe->RuntimeFlags()>>17)&7u;
    const int linkedMaximumHp = armyProbe->Vid()->maxHpByPlayer[linkedArmyIndex];
    if (linkedHp >= linkedMaximumHp)
        return 0;
    return 100-((currentHp+linkedHp)*100/(maximumHp+linkedMaximumHp));
}

int ENGINE::NeedAddAmmo()
{
    VID* const engineVid = m_vid;
    VID* const linkedVid = engineVid->linkVid;
    VID* weaponVid = engineVid;
    if (linkedVid && linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0)
        weaponVid = linkedVid;
    const WEAPON_RUNTIME* const weapon = weaponVid->weapon;
    const int ammoCapacity = weapon->MaxAmmo;
    if (ammoCapacity == 0)
        return 0;
    const int ammoFixedPoint = m_ammoFixedPoint;
    const int ammoCount = (ammoFixedPoint+(ammoFixedPoint<0?63:0))>>6;
    return 100-(ammoCount*100/ammoCapacity);
}

int ENGINE::TrainWeaponRange()
{
    if (!m_goalSprite)
        return 0;
    const unsigned int command = m_runtimeFlags&0x7Cu;
    if (command != 0x70u && command != 0x74u)
        return 0;
    const float noRange = (10000.0f);
    float minimumRange = noRange;
    ENGINE* candidate = reinterpret_cast<ENGINE*>(m_commandOwner);
    if (candidate)
    {
        SPRITE* linkedSprite = candidate->ChildChain();
        if (!linkedSprite)
            minimumRange = 0.0f;
        else
        {
            VID* const owner = candidate->Vid();
            VID* const linkedOwner = linkedSprite->Vid();
            if (linkedOwner != owner->linkVid || linkedOwner->nWeapon == 0 || linkedOwner->childVid[8] == 0)
            {
                minimumRange = 0.0f;
            }
            else
            {
                const int ammoFixedPoint = candidate->m_ammoFixedPoint;
                const int ammoCount = (ammoFixedPoint+(ammoFixedPoint<0?63:0))>>6;
                if (ammoCount <= 0)
                {
                    minimumRange = 0.0f;
                }
                else if (owner->nVid == 35)
                {
                    const WEAPON_RUNTIME* const stats = owner->weapon;
                    const float range = stats->BattleRange;
                    minimumRange = range<noRange?range:0.0f;
                }
                else
                {
                    SPRITE* linkedWeaponSprite = static_cast<DIALOG_UNIT*>(candidate)->Link();
                    const WEAPON_RUNTIME* stats = linkedWeaponSprite->Vid()->weapon;
                    const float range = stats->BattleRange;
                    if (range<noRange)
                    {
                        linkedWeaponSprite = static_cast<DIALOG_UNIT*>(candidate)->Link();
                        stats = linkedWeaponSprite->Vid()->weapon;
                        minimumRange = stats->BattleRange;
                    }
                    else minimumRange = 0.0f;
                }
            }
        }
    }
    else
    {
        candidate = FirstEngine();
        while (candidate)
        {
            SPRITE* const linkedSprite = candidate->m_childChain;
            if (linkedSprite)
            {
                VID* const owner = candidate->m_vid;
                VID* const linkedOwner = linkedSprite->Vid();
                if (linkedOwner == owner->linkVid && linkedOwner->nWeapon != 0 && linkedOwner->childVid[8] != 0)
                {
                    const int ammoFixedPoint = candidate->m_ammoFixedPoint;
                    const int ammoCount = (ammoFixedPoint+(ammoFixedPoint<0?63:0))>>6;
                    if (ammoCount>0)
                    {
                        if (owner->nVid == 35u)
                        {
                            const WEAPON_RUNTIME* const stats = owner->weapon;
                            const float range = stats->BattleRange;
                            if (range<minimumRange)
                                minimumRange = range;
                        }
                        else
                        {
                            SPRITE* linkedWeaponSprite = static_cast<DIALOG_UNIT*>(candidate)->Link();
                            const WEAPON_RUNTIME* stats = linkedWeaponSprite->Vid()->weapon;
                            const float range = stats->BattleRange;
                            if (range<minimumRange)
                            {
                                linkedWeaponSprite = static_cast<DIALOG_UNIT*>(candidate)->Link();
                                stats = linkedWeaponSprite->Vid()->weapon;
                                minimumRange = stats->BattleRange;
                            }
                        }
                    }
                }
            }
            candidate = candidate->m_nextEngine;
        }
    }
    if (minimumRange == noRange)
        minimumRange = 0.0f;
    return TruncateFloatToInt32(minimumRange);
}

void ENGINE::ForceStop()
{
    ENGINE* firstEngine = this;
    while (firstEngine->m_prevEngine)
        firstEngine = firstEngine->FirstEngine();
    float absoluteSpeed = firstEngine->m_speed;
    if (absoluteSpeed<0.0f)
        absoluteSpeed = -absoluteSpeed;
    CONSTANT* constantsAddress = ::Const;
    if (absoluteSpeed>Const->SafeClashSpeed)
    {
        ENGINE* trainEngine = firstEngine;
        while (trainEngine)
        {
            (void)trainEngine->Action(ACT_DAMAGE, 2, 0, 0);
            trainEngine = trainEngine->m_nextEngine;
        }
    }
    firstEngine->m_targetSpeed = 0.0f;
    firstEngine->m_speed = 0.0f;
}

void ENGINE::Move(const VECTOR& targetPosition, int , int screenCoordinates)
{
    R_DOT* targetDot = 0;
    const int targetZ = TruncateFloatToInt32(targetPosition.z);
    const int targetY = TruncateFloatToInt32(targetPosition.y);
    const int targetX = TruncateFloatToInt32(targetPosition.x);
    if (screenCoordinates != 0)
        targetDot = RailMap.Get().GetNearestDot(targetX, targetY-targetZ);
    else targetDot = RailMap.Get().GetNearestDot(targetX, targetY, targetZ);
    SetCommandToTrain(23, 0, targetDot, 0);
}

static bool EngineHasUsableLinkedWeapon(const ENGINE* engine, SPRITE** childOut, VID** childOwnerOut)
{
    SPRITE* const child = engine->ChildChain();
    if (!child)
        return false;
    VID* const engineVid = engine->Vid();
    VID* const linkedVid = child->Vid();
    if (!engineVid || !linkedVid || linkedVid != engineVid->linkVid || linkedVid->nWeapon == 0 || linkedVid->childVid[8] == 0)
        return false;
    if (childOut)
        *childOut = child;
    if (childOwnerOut)
        *childOwnerOut = linkedVid;
    return true;
}

void ENGINE::SetCommandToTrain(int command, SPRITE* targetSprite, R_DOT* firstDot, R_DOT* lastDot)
{
    bool suppressLinkedWeapon = false;
    if (command == 30)
    {
        command = 0;
        suppressLinkedWeapon = true;
    }
    if (!targetSprite && !firstDot && command != 29)
        command = 0;
    if (command == 24)
    {
        ENGINE* repairEngine = FirstEngine();
        while (repairEngine)
        {
            VID* const owner = repairEngine->Vid();
            if (owner && owner->nVid == 85 && repairEngine->m_ammoFixedPoint/64>0)
                break;
            repairEngine = repairEngine->m_nextEngine;
        }
        if (!repairEngine)
        {
            command = 0;
            targetSprite = 0;
            firstDot = 0;
        }
    }
    R_DOT* pathEnd = 0;
    R_DOT* pathStart = 0;
    if (command == 25)
    {
        if (!lastDot)
        {
            lastDot = RailMap.Get().GetNearestDot( TruncateFloatToInt32(X()), TruncateFloatToInt32(Y()), TruncateFloatToInt32(Z()));
        }
        pathEnd = lastDot;
        pathStart = firstDot;
    }
    for (ENGINE* trainEngine = FirstEngine();trainEngine;trainEngine = trainEngine->m_nextEngine)
    {
        SPRITE* previousCommandOwner = trainEngine->m_commandOwner;
        if (previousCommandOwner)
        {
            previousCommandOwner->Release();
            trainEngine->m_commandOwner = 0;
        }
        if ((command == 28 || command == 29) && !IsCommandToAllTrain())
        {
            trainEngine->m_commandOwner = this;
            ++m_referenceCount;
        }
        trainEngine->SetCommandWithoutLink(command, targetSprite);
        trainEngine->m_commandDot = firstDot;
        trainEngine->m_targetSpeed = 0.0f;
        trainEngine->m_pathEnd = pathEnd;
        trainEngine->m_pathStart = pathStart;
        trainEngine->m_commandTick = 0u;
        trainEngine->m_mineDeploymentReady = 0;
        SPRITE* child = 0;
        VID* childOwner = 0;
        if (!suppressLinkedWeapon && EngineHasUsableLinkedWeapon(trainEngine, &child, &childOwner))
        {
            if ((!IsCommandToAllTrain() && trainEngine != this) || !CanAttackThisSprite(targetSprite))
            {
                trainEngine->Link()->SetCommandWithoutLink(0, 0);
            }
            else if (command == 28)
            {
                trainEngine->Link()->SetCommandWithoutLink(3, targetSprite);
            }
            else if (command == 29)
            {
                WEAPON_RUNTIME* weapon = 0;
                SPRITE* weaponChild = 0;
                VID* weaponChildOwner = 0;
                if (EngineHasUsableLinkedWeapon(trainEngine, &weaponChild, &weaponChildOwner))
                    weapon = weaponChildOwner->weapon;
                else
                {
                    VID* const owner = trainEngine->m_vid;
                    if (owner)
                        weapon = owner->weapon;
                }
                const unsigned int weaponType = weapon?static_cast<unsigned int>(weapon->SpriteType):0u;
                if (weaponType == 0x10u && targetSprite)
                {
                    void* targetStorage = static_cast<unsigned char*>(CrtNew(104u));
                    SPRITE* elevatedTarget = 0;
                    if (targetStorage)
                    {
                        const VECTOR position(targetSprite->X(), targetSprite->Y()+70.0f, targetSprite->Z()+70.0f);
                        elevatedTarget = ::new(targetStorage) SPRITE(position, ANGLE(static_cast<unsigned char>(0)));
                    }
                    trainEngine->Link()->SetCommandWithoutLink(4, elevatedTarget);
                }
                else
                {
                    trainEngine->Link()->SetCommandWithoutLink(4, targetSprite);
                }
            }
            else
            {
                trainEngine->Link()->SetCommandWithoutLink(0, 0);
            }
        }
    }
    if (command == 23 || command == 26 || command == 27 || command == 25)
    {
        FirstEngine()->StartMove();
    }
    else if (command == 24)
    {
        if (m_frontPos.m_dot != m_commandDot)
        {
            FirstEngine()->StartMove();
        }
        else
        {
            FirstEngine()->Stop();
            for (ENGINE* trainEngine = FirstEngine();trainEngine;trainEngine = trainEngine->m_nextEngine)
            {
                VID* const owner = trainEngine->m_vid;
                if (owner && owner->nVid == 85u && m_ammoFixedPoint/64 != 0)
                    trainEngine->m_mineDeploymentReady = 1;
            }
        }
    }
    else if (command == 0)
    {
        FirstEngine()->Stop();
    }
}

static bool EngineHasWeaponDescriptor(const VID* owner)
{
    return owner && owner->nWeapon != 0 && owner->childVid[8] != 0;
}

static bool EngineSameArmy(const SPRITE* lhs, const SPRITE* rhs)
{
    if (!lhs || !rhs)
        return false;
    return ((lhs->RuntimeFlags()^rhs->RuntimeFlags())&0x000E0000u) == 0u;
}

static float EngineApproxXYDistance(const SPRITE* from, const SPRITE* to)
{
    float deltaX = to->X()-from->X();
    if (deltaX<0.0f)
        deltaX = -deltaX;
    float deltaY = to->Y()-from->Y();
    if (deltaY<0.0f)
        deltaY = -deltaY;
    const float diagonalWeight = (0.398000002f);
    return deltaX <= deltaY ? deltaY+deltaX*diagonalWeight : deltaX+deltaY*diagonalWeight;
}

void ENGINE::ActNextCommandFighter()
{
    SPRITE* child = 0;
    if (HaveLink())
    {
        child = Link();
        VID* const childOwner = child?child->Vid():0;
        if (EngineHasWeaponDescriptor(childOwner))
        {
            SPRITE* target = child->Goal();
            if (target && EngineSameArmy(this, target))
            {
                const unsigned int childCommand = child->RuntimeFlags()&0x7Cu;
                if (childCommand == 0x0Cu || childCommand == 0x10u)
                    (void)child->SetCommand(0, static_cast<SPRITE*>(0));
            }
        }
    }
    VID* const engineVid = m_vid;
    if (!engineVid)
        return;
    if (engineVid->nVid == 35u)
    {
        child = Link();
        VID* const childOwner = child?child->Vid():0;
        if (!child || childOwner != engineVid->linkVid || !EngineHasWeaponDescriptor(childOwner))
            return;
        const int linkedAmmo = child->Action(ACT_GET_AMMO, 0, 0, 0);
        const WEAPON_RUNTIME* const childWeapon = childOwner->weapon;
        if (!childWeapon || linkedAmmo<childWeapon->MaxAmmo)
            return;
    }
    if ((m_runtimeFlags&1u) != 0u)
        return;
    const unsigned int trainCommand = m_runtimeFlags&0x7Cu;
    if (trainCommand != 0x70u && trainCommand != 0x74u)
    {
        child = Link();
        VID* const childOwner = child?child->Vid():0;
        if (!child || childOwner != engineVid->linkVid || !EngineHasWeaponDescriptor(childOwner))
            return;
        SPRITE* target = child->Goal();
        if (!target)
            return;
        const float distance = EngineApproxXYDistance(this, target);
        const WEAPON_RUNTIME* const childWeapon = childOwner->weapon;
        if (childWeapon && distance>childWeapon->DetectRange)
            (void)child->SetCommandWithoutLink(0, static_cast<SPRITE*>(0));
        return;
    }
    SPRITE* const commandOwner = m_commandOwner;
    if (commandOwner && commandOwner != this)
        return;
    SPRITE* target = Goal();
    child = Link();
    if (!target || !child || target == child->Goal())
        return;
    VID* const nullVid = ::EmptyVid;
    if (target->Vid() != nullVid && !CanAttackThisSprite(target))
        return;
    const float distance = EngineApproxXYDistance(this, target);
    float range = 0.0f;
    const WEAPON_RUNTIME* const ownWeapon = engineVid->weapon;
    if (ownWeapon)
        range = ownWeapon->BattleRange;
    VID* const childOwner = child->Vid();
    if (childOwner == engineVid->linkVid && EngineHasWeaponDescriptor(childOwner) && ownWeapon && ownWeapon->BattleRange == (0.0f))
    {
        const WEAPON_RUNTIME* const childWeapon = childOwner->weapon;
        if (childWeapon)
            range = childWeapon->BattleRange;
    }
    if (distance<range)
        (void)child->SetCommandWithoutLink(4, target);
}

void ENGINE::CalcCoor()
{
    R_DOT* primary = m_frontPos.m_dot;
    const unsigned int primaryIndex = static_cast<unsigned int>(m_frontPos.m_linkIndex);
    R_DOT* primaryTail = primary?primary->m_links[primaryIndex].dot:0;
    const int primaryDistance = primary?primary->m_links[primaryIndex].length:0;
    const int primaryProgress = m_frontPos.m_progress;
    int start = primary->m_x;
    int finish = primaryTail->m_x;
    unsigned int wrapped = (static_cast<unsigned int>(finish)-static_cast<unsigned int>(start));
    wrapped *= static_cast<unsigned int>(primaryProgress);
    wrapped <<= 8;
    const float primaryXFixed = static_cast<float>(static_cast<int>(wrapped)/primaryDistance)+static_cast<float>(start)*256.0f;
    start = primary->m_y;
    finish = primaryTail->m_y;
    wrapped = (static_cast<unsigned int>(finish)-static_cast<unsigned int>(start));
    wrapped *= static_cast<unsigned int>(primaryProgress);
    wrapped <<= 8;
    const float primaryYFixed = static_cast<float>(static_cast<int>(wrapped)/primaryDistance)+static_cast<float>(start)*256.0f;
    start = primary->m_z;
    finish = primaryTail->m_z;
    wrapped = (static_cast<unsigned int>(finish)-static_cast<unsigned int>(start));
    wrapped *= static_cast<unsigned int>(primaryProgress);
    wrapped <<= 8;
    const float primaryZFixed = static_cast<float>(static_cast<int>(wrapped)/primaryDistance)+static_cast<float>(start)*256.0f;
    R_DOT* secondary = m_backPos.m_dot;
    const int secondaryCount = secondary->m_linkCount;
    int secondaryIndex = m_backPos.m_linkIndex;
    if (secondaryIndex >= secondaryCount)
    {
        STRING text("\360\345\353\374\361\373 \355\345\357\360\340\342\350\353\374\355\373\345 link>nolink");
        static_cast<DIALOG_UNIT*>(this)->Error(E_ERROR, text, 0u);
        secondaryIndex = secondaryCount-1;
        m_backPos.m_linkIndex = secondaryIndex;
    }
    secondary = m_backPos.m_dot;
    R_DOT* secondaryTail = secondary?secondary->m_links[static_cast<unsigned int>(secondaryIndex)].dot:0;
    if (!secondaryTail)
    {
        STRING text("\360\345\353\374\361\373 \355\345\357\360\340\342\350\353\374\355\373\345 \355\345\362 tail.Dot2()");
        static_cast<DIALOG_UNIT*>(this)->Error(E_ERROR, text, 0u);
        return;
    }
    const int secondaryDistance = secondary->m_links[static_cast<unsigned int>(secondaryIndex)].length;
    const int secondaryProgress = m_backPos.m_progress;
    start = secondary->m_x;
    finish = secondaryTail->m_x;
    wrapped = (static_cast<unsigned int>(finish)-static_cast<unsigned int>(start));
    wrapped *= static_cast<unsigned int>(secondaryProgress);
    wrapped <<= 8;
    const float secondaryXFixed = static_cast<float>(static_cast<int>(wrapped)/secondaryDistance)+static_cast<float>(start)*256.0f;
    start = secondary->m_y;
    finish = secondaryTail->m_y;
    wrapped = (static_cast<unsigned int>(finish)-static_cast<unsigned int>(start));
    wrapped *= static_cast<unsigned int>(secondaryProgress);
    wrapped <<= 8;
    const float secondaryYFixed = static_cast<float>(static_cast<int>(wrapped)/secondaryDistance)+static_cast<float>(start)*256.0f;
    start = secondary->m_z;
    finish = secondaryTail->m_z;
    wrapped = (static_cast<unsigned int>(finish)-static_cast<unsigned int>(start));
    wrapped *= static_cast<unsigned int>(secondaryProgress);
    wrapped <<= 8;
    const float secondaryZFixed = static_cast<float>(static_cast<int>(wrapped)/secondaryDistance)+static_cast<float>(start)*256.0f;
    VECTOR position;
    position.x = (primaryXFixed+secondaryXFixed)*0.001953125f;
    position.y = (primaryYFixed+secondaryYFixed)*0.001953125f;
    position.z = (primaryZFixed+secondaryZFixed)*0.001953125f;
    ChangeCoor(position);
    float directionX;
    float directionY;
    if ((m_trainFlags&1u) != 0u)
    {
        directionY = (secondaryYFixed-primaryYFixed+128.0f)*3.0f*0.001953125f;
        directionX = (secondaryXFixed-primaryXFixed+128.0f)*0.00390625f;
    }
    else
    {
        directionY = (primaryYFixed-secondaryYFixed+128.0f)*3.0f*0.001953125f;
        directionX = (primaryXFixed-secondaryXFixed+128.0f)*0.00390625f;
    }
    const ANGLE direction = Decart2Polar(directionX, directionY);
    ChangeDirection(direction);
}

int ENGINE::ForceLink(ENGINE* otherEngine)
{
    if (!otherEngine || InTrain(otherEngine))
        return 1;
    if (!otherEngine->m_prevEngine && !otherEngine->m_nextEngine)
    {
        R_DOT* frontEndpoint = otherEngine->m_frontPos.Dot2();
        R_DOT* backEndpoint = otherEngine->m_backPos.Dot2();
        const float engineX = m_xyz.x, engineY = m_xyz.y, engineZ = m_xyz.z;
        const float frontDeltaX = static_cast<float>(frontEndpoint->m_x)-engineX;
        const float frontDeltaY = static_cast<float>(frontEndpoint->m_y)-engineY;
        const float frontDeltaZ = static_cast<float>(frontEndpoint->m_z)-engineZ;
        const float backDeltaX = static_cast<float>(backEndpoint->m_x)-engineX;
        const float backDeltaY = static_cast<float>(backEndpoint->m_y)-engineY;
        const float backDeltaZ = static_cast<float>(backEndpoint->m_z)-engineZ;
        const float firstDistance2 = frontDeltaX*frontDeltaX+frontDeltaY*frontDeltaY+frontDeltaZ*frontDeltaZ;
        const float secondDistance2 = backDeltaX*backDeltaX+backDeltaY*backDeltaY+backDeltaZ*backDeltaZ;
        if (secondDistance2>firstDistance2)
            otherEngine->ReverseTrain();
    }
    else
    {
        ENGINE* frontEndpoint = otherEngine->FirstEngine();
        ENGINE* last = otherEngine->LastEngine();
        const float firstDeltaX = frontEndpoint->m_xyz.x-m_xyz.x;
        const float firstDeltaY = frontEndpoint->m_xyz.y-m_xyz.y;
        const float lastDeltaX = last->m_xyz.x-m_xyz.x;
        const float lastDeltaY = last->m_xyz.y-m_xyz.y;
        if (lastDeltaX*lastDeltaX+lastDeltaY*lastDeltaY>firstDeltaX*firstDeltaX+firstDeltaY*firstDeltaY)
            otherEngine->ReverseTrain();
    }
    ENGINE* lastEngine = otherEngine->LastEngine();
    lastEngine->m_nextEngine = this;
    m_prevEngine = lastEngine;
    R_POS* const sourcePosition = &lastEngine->m_backPos;
    R_POS* const frontPosition = &m_frontPos;
    R_DOT* const sourceDot = sourcePosition->m_dot;
    const unsigned int sourceLinkIndex = static_cast<unsigned int>(sourcePosition->m_linkIndex);
    const R_DOT::LINK& sourceLink = sourceDot->m_links[sourceLinkIndex];
    R_DOT* const destinationDot = sourceLink.dot;
    const int sourceLinkLength = sourceLink.length;
    const int reverseLinkIndex = sourceLink.reverseIndex;
    frontPosition->m_dot = destinationDot;
    frontPosition->m_progress = sourceLinkLength-sourcePosition->m_progress;
    frontPosition->m_progressFraction = 0u;
    frontPosition->m_linkIndex = reverseLinkIndex;
    const ANGLE sourceDirection = sourcePosition->Direct();
    const int outgoingLinkIndex = destinationDot->GetLink(sourceDirection);
    R_DOT* const backDot = destinationDot->m_links[static_cast<unsigned int>(outgoingLinkIndex)].dot;
    m_backPos.m_dot = backDot;
    m_backPos.m_linkIndex = backDot->GetLink(sourceDirection);
    const unsigned char currentDirection = m_direction.value;
    const unsigned char railDirection = frontPosition->Direct().value;
    const unsigned char forwardTurn = static_cast<unsigned char>(currentDirection-railDirection);
    const unsigned char reverseTurn = static_cast<unsigned char>(railDirection-currentDirection);
    const unsigned char turn = forwardTurn<reverseTurn?forwardTurn:reverseTurn;
    if (turn>127u)
        m_trainFlags |= 1u;
    PullTail(*frontPosition);
    CalcCoor();
    return 0;
}

#include "unit.h"

void*  Vector2IntCtorAbiBridge(void* self, int x, int y)
{
    return new(self) VECTOR2(static_cast<float>(x), static_cast<float>(y));
}

VECTOR2 VECTOR::ViewCoor() const
{
    return VECTOR2(x, y-z);
}

void*  ANGLE_Ctor(void* self, unsigned int value)
{
    return new(self) ANGLE(static_cast<unsigned char>(value));
}

float GRAPH::ViewXMin() const
{
    return viewLeft;
}

float GRAPH::ViewXMax() const
{
    return viewRight;
}

const VECTOR2& GRAPH::ViewMax() const
{
    return *reinterpret_cast<const VECTOR2*>(&viewRight);
}

const VECTOR2& GRAPH::SizeXY() const
{
    return *reinterpret_cast<const VECTOR2*>(&screenWidth);
}

const char* STRING::CharPtr() const
{
    return m_value;
}






VECTOR2 VECTOR2::operator+(const VECTOR2& value) const
{
    return VECTOR2(x+value.x, y+value.y);
}

VECTOR2 VECTOR2::operator*(const VECTOR2& value) const
{
    return VECTOR2(x*value.x, y*value.y);
}

VECTOR2 SPRITE::ScreenXY() const
{
    return VECTOR2(Map->ToScreenX(m_xyz.x), Map->ToScreenY(m_xyz.y-m_xyz.z));
}

VECTOR2 R_DOT::ScreenXY() const
{
    const int vertical = m_y-m_z;
    return VECTOR2(Map->ToScreenX(static_cast<float>(m_x)), Map->ToScreenY(static_cast<float>(vertical)));
}



void*  LookupPointerA28(const void* self, int index)
{
    const MAP* const map = static_cast<const MAP*>(self);
    if (index >= 0 && index<map->VidCount())
    {
        VID* const value = map->StoredVid(index);
        if (value)
            return value;
    }
    return reinterpret_cast<unsigned char*>(::EmptyVid);
}

ANGLE R_POS::Direct() const
{
    ANGLE result;
    R_DOT* const base = m_dot;
    result.value = base?base->m_links[static_cast<unsigned int>(m_linkIndex)].direction.value:0u;
    return result;
}








void*  UACT_G_LIST(void* self, unsigned int flags)
{
    static_cast<LIST<ACT>*>(self)->LIST<ACT>::~LIST();
    if (flags&1u)
        CrtFreePointer(self);
    return self;
}


void GRAPH::PutsXY(float x, float y, const STRING& text, COLOR color)
{
    PutsXY(x, y, text.CharPtr(), color);
}

int SPRITE::IsCross(const VID* otherVid, const VECTOR& otherCoor, ANGLE otherDirection) const
{
    if (!IsXYCross(otherVid, otherCoor, otherDirection))
        return 0;
    const float selfZ = m_xyz.z;
    const float otherZ = otherCoor.z;
    const float selfHeight = m_vid->sizeXYZ.z;
    const double selfTop = static_cast<double>(selfHeight)+static_cast<double>(selfZ);
    const double otherTop = static_cast<double>(otherZ)+static_cast<double>(otherVid->sizeXYZ.z);
    if (selfTop<otherZ || selfTop != selfTop || otherZ != otherZ)
        return 0;
    if (otherTop<selfZ || otherTop != otherTop || selfZ != selfZ)
        return 0;
    return 1;
}

void ENGINE::MoveTact()
{
    MoveEngineTact();
    unsigned int currentTick = ::CurrentTime;
    const unsigned int previousTick = ::PrevCurrentTime;
    if ((currentTick&0xFFFFC000u)>previousTick)
    {
        m_runtimeFlags &= ~2u;
        currentTick = ::CurrentTime;
    }
    if ((currentTick&0xFFFFFC00u)>previousTick && m_prevEngine == 0)
    {
        TRAIN_INFO info(this);
        MAP* worldMap = Map;
        if (info.averageAmmoPercent <= 10)
        {
            (void)worldMap->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_NOT_AMMO], this, 0, 0);
        }
        if (info.currentHp<(info.maxHp/2))
        {
            worldMap = Map;
            (void)worldMap->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_DAMAGE], this, 0, 0);
        }
        currentTick = ::CurrentTime;
    }
    if ((currentTick&0xFFFFFC00u)>::PrevCurrentTime)
    {
        const int type = m_vid?m_vid->nVid:0;
        if (type == 85)
            RepairTact();
        else if (type == 45)
            AddAmmoTact();
        else if (type == 35)
            m_stateFC = 0;
    }
}

void ENGINE::Stop()
{
    ENGINE* node = this;
    for (;;)
    {
        MAP* world = Map;
        SPRITE* probe = world->Flagman(world->m_playerIndex);
        if (node->InTrain(probe))
            PathDots.Get().DeleteAll();
        if (node->m_prevEngine == 0)
            break;
        node = node->FirstEngine();
    }
    unsigned int flags = node->m_runtimeFlags;
    if ((flags&0x7Cu) == 0x64u && (flags&0x400u) != 0u)
    {
        R_DOT* const c4 = node->m_pathEnd;
        R_DOT* const c8 = node->m_pathStart;
        node->m_commandDot = c4;
        node->m_pathEnd = c8;
        node->m_runtimeFlags = flags|0x400u;
        node->m_pathStart = node->m_commandDot;
        node->m_targetSpeed = 0.0f;
        node->m_acceleration = 0;
        for (ENGINE* linked = node->m_nextEngine; linked; linked = linked->m_nextEngine)
        {
            linked->m_commandDot = node->m_commandDot;
            linked->m_pathEnd = node->m_pathEnd;
            linked->m_pathStart = node->m_pathStart;
            linked->m_runtimeFlags |= 0x400u;
            linked->m_targetSpeed = 0.0f;
            linked->m_acceleration = 0;
        }
        return;
    }
    const float trainSpeed = node->m_speed;
    const bool zeroOrUnordered = (trainSpeed == 0.0f) || (trainSpeed != trainSpeed);
    for (ENGINE* linked = node;linked;linked = linked->m_nextEngine)
    {
        linked->m_targetSpeed = 0.0f;
        linked->m_runtimeFlags &= ~0x400u;
        if (!zeroOrUnordered)
        {
            int brakingMagnitude = TruncateFloatToInt32(trainSpeed*(1000.0f));
            if (brakingMagnitude<0)
                brakingMagnitude = -brakingMagnitude;
            brakingMagnitude = (brakingMagnitude+10)/2;
            linked->m_acceleration = -brakingMagnitude;
        }
    }
}

int ENGINE::Repair(int trigger)
{
    ChangeHp(m_vid->maxHpByPlayer[(m_runtimeFlags>>17u)&7u]);
    VID* const owner = m_vid;
    VID* const target = owner->linkVid;
    SPRITE* const current = m_childChain;
    bool dispatch = target != 0 && (!current || current->Vid() != target) && trigger != 0;
    if (dispatch && owner->nVid == 35)
    {
        VID* const fallback = ::EmptyVid;
        const int mapState = Map->m_vidCount;
        VID* left = fallback;
        VID* right = fallback;
        if (mapState>40 && Map->m_vids[40])
            left = Map->m_vids[40];
        if (mapState>35 && Map->m_vids[35])
            right = Map->m_vids[35];
        const unsigned int direction = (m_runtimeFlags>>17u)&7u;
        if (left->spriteCountsByPlayer[direction] >= right->spriteCountsByPlayer[direction])
            dispatch = false;
    }
    if (dispatch)
    {
        CreateLink();
        DIALOG_UNIT* const embedded = static_cast<DIALOG_UNIT*>(this);
        const VECTOR& position = embedded->Coor();
        VID* const mapped = Map->Vid(590);
        (void)Map->CreateSprite(mapped, position, ANGLE(static_cast<unsigned char>(0u)), this);
        return 1;
    }
    if (current && current->Vid() == target)
    {
        DIALOG_UNIT* const embedded = static_cast<DIALOG_UNIT*>(this);
        SPRITE* const object = embedded->Link();
        (void)object->Action(ACT_REPAIR, 0, 0, 0);
    }
    return 1;
}

void ENGINE::ReCalcMoveParameters()
{
    ENGINE* firstEngine = this;
    while (firstEngine->m_prevEngine != 0)
        firstEngine = firstEngine->FirstEngine();
    TRAIN_INFO trainInfo(firstEngine);
    const float zero = (0.0f);
    const float initialSpeed = firstEngine->m_targetSpeed;
    const bool speedZeroOrUnordered = (initialSpeed == zero) || (initialSpeed != initialSpeed);
    const float railLength = trainInfo.totalLoadScalar;
    const bool railLengthZeroOrUnordered = (railLength == zero) || (railLength != railLength);
    if (speedZeroOrUnordered && !railLengthZeroOrUnordered)
    {
        const int acceleration = TruncateFloatToInt32( (trainInfo.totalDriveScalar/railLength)*(8.0f));
        const unsigned int flags = firstEngine->m_runtimeFlags;
        SPRITE_COLLECTOR* const collector = ::Sprites;
        if (acceleration>7 && (flags&0x400u) != 0u && collector->deletingAll == 0)
        {
            const unsigned int targetDotAddress = PtrToU32(firstEngine->m_commandDot);
            const unsigned int targetSpriteAddress = targetDotAddress?0u:PtrToU32(firstEngine->m_goalSprite);
            const unsigned int searchCommand = (flags>>2u)&0x1Fu;
            const int railStep = firstEngine->m_frontPos.NoStepToTarget( reinterpret_cast<R_DOT*>(U32ToPtr(targetDotAddress)), reinterpret_cast<SPRITE*>(U32ToPtr(targetSpriteAddress)), searchCommand, firstEngine);
            MAP* worldMap = Map;
            SPRITE* flagman = worldMap->Flagman(worldMap->m_playerIndex);
            if (firstEngine->InTrain(flagman))
            {
                worldMap = Map;
                flagman = worldMap->Flagman(worldMap->m_playerIndex);
                if ((flagman->RuntimeFlags()&0x000E0000u) == 0u)
                {
                    R_DOT* const frontDot = firstEngine->m_frontPos.m_dot;
                    R_DOT* nextDot = 0;
                    if (frontDot)
                    {
                        const unsigned int frontLinkIndex = static_cast<unsigned int>(firstEngine->m_frontPos.m_linkIndex);
                        nextDot = frontDot->m_links[frontLinkIndex].dot;
                    }
                    firstEngine->CreatePathDots(nextDot);
                }
            }
            *reinterpret_cast<unsigned int*>(&firstEngine->m_targetSpeed) = railStep<0?0xBA83126Fu:0x3A83126Fu;
        }
    }
    const float speed = firstEngine->m_targetSpeed;
    if (speed<zero || speed != speed)
    {
        const int maxSpeed = trainInfo.maxSpeedThousandths;
        firstEngine->m_targetSpeed = -static_cast<float>(maxSpeed)*(0.00100000005f);
        return;
    }
    if (!(speed>zero))
        return;
    const int maxSpeed = trainInfo.maxSpeedThousandths;
    firstEngine->m_targetSpeed = static_cast<float>(maxSpeed)*(0.00100000005f);
    const float denominator = trainInfo.totalLoadScalar;
    int acceleration = 0;
    if (denominator != zero && denominator == denominator)
        acceleration = TruncateFloatToInt32( (trainInfo.totalDriveScalar/denominator)*(8.0f));
    firstEngine->m_acceleration = acceleration;
    if (acceleration == 0)
        firstEngine->m_runtimeFlags &= ~0x400u;
}

void ENGINE::MT_PullCoordinates(R_POS* position, float speed, int movementState)
{
    ENGINE* const original = this;
    for (ENGINE* node = this;node;node = node->m_nextEngine)
    {
        node->ClearDotBusy();
        node->PullTail(*position);
        ENGINE* linked = node->m_nextEngine;
        if (linked)
        {
            *position = linked->m_frontPos;
            R_DOT* const recordBase = node->m_backPos.m_dot;
            const unsigned int recordIndex = static_cast<unsigned int>(node->m_backPos.m_linkIndex);
            const int recordBias = node->m_backPos.m_progress;
            const R_DOT::LINK& record = recordBase->m_links[recordIndex];
            linked->m_frontPos.m_dot = record.dot;
            linked->m_frontPos.m_progress = record.length-recordBias;
            linked->m_frontPos.m_progressFraction = 0u;
            linked->m_frontPos.m_linkIndex = record.reverseIndex;
        }
        node->CalcCoor();
        const float oldX = node->m_previousPositionX;
        const float oldY = node->m_previousPositionY;
        if (oldX != (0.0f) || oldY != (0.0f))
        {
            float dx = oldX-node->m_xyz.x;
            if (dx<0.0f)
                dx = -dx;
            if (dx>(30.0f))
                (void)::Error->Log("\361\365\353\340\357\373\342\340\355\350\345 \342\340\343\356\355\356\342 zm-error - kawabanga - begin");
        }
        node->m_previousPositionX = node->m_xyz.x;
        node->m_previousPositionY = node->m_xyz.y;
        node->m_previousPositionZ = node->m_xyz.z;
        node->SetDotBusy();
        float nodeSpeed = speed;
        if ((node->m_trainFlags&1u) != 0u)
            nodeSpeed = -nodeSpeed;
        node->m_speed = nodeSpeed;
        node->m_acceleration = movementState;
    }
    if (speed == (0.0f))
        return;
    SPRITE_COLLECTOR* const collector = ::Sprites;
    VID* const owner = original->Vid();
    const float rx = owner->halfSizeXY.x;
    const float ry = owner->halfSizeXY.y;
    const float x = original->X();
    const float y = original->Y();
    SPRITE* candidate = collector->FirstHashInBox( x-rx, y-ry, x+rx, y+ry);
    while (candidate)
    {
        SPRITE* const probe = candidate;
        const ANGLE direction = original->Direction();
        if (probe != original && probe->IsCross(owner, original->Coor(), direction))
        {
            VID* const probeOwner = probe->Vid();
            if ((probeOwner->property&0x4000u) != 0u)
                (void)probe->Action(ACT_DAMAGE, 5, 0, 0);
        }
        candidate = collector->NextHashInBox();
    }
}

static unsigned int EngineActionArmy(const SPRITE* unit)
{
    return (unit->RuntimeFlags()>>17u)&7u;
}

static WEAPON_RUNTIME* EngineActionWeapon(const VID* owner)
{
    return owner?owner->weapon:0;
}

static bool EngineActionHasWeaponDescriptor(const VID* owner)
{
    return owner && owner->nWeapon != 0 && owner->childVid[8] != 0;
}

static int EngineActionAmmoCapacity(const VID* owner)
{
    if (!owner)
        return 0;
    const VID* const linked = owner->linkVid;
    const VID* const source = (EngineActionHasWeaponDescriptor(linked)?linked:owner);
    const WEAPON_RUNTIME* weapon = EngineActionWeapon(source);
    return weapon?weapon->MaxAmmo:0;
}

PLAYER* ENGINE::ActionPlayer(unsigned int army)
{
    MAP* const application = ::Map;
    return application->m_players[army&7u];
}

VID* ENGINE::ActionVidSlot70()
{
    MAP* const application = ::Map;
    if (application->m_vidCount>70)
    {
        VID* value = application->m_vids[70];
        if (value)
            return value;
    }
    return ::EmptyVid;
}

static bool EngineActionSameArmy(const SPRITE* firstSprite, const SPRITE* secondSprite)
{
    return firstSprite && secondSprite && (((firstSprite->RuntimeFlags()^secondSprite->RuntimeFlags())&0x000E0000u) == 0u);
}

void ENGINE::RemoveFromPlayer()
{
    PLAYER* const player = ActionPlayer(EngineActionArmy(this));
    player->DeletePointerToSprite(this);
}

void ENGINE::AddToStateBar()
{
    PLAYER* const player = ActionPlayer(EngineActionArmy(this));
    player->AddUnitToStateBar(this);
}

static void EngineActionSpawnLoopEffect(ENGINE* engine)
{
    const VECTOR current = engine->Coor();
    VID* effectVid = Map->Vid(588);
    (void)Map->CreateSprite(effectVid, current, ANGLE(static_cast<unsigned char>(0u)), engine);
}

int ENGINE::Action(int action, int argument1, int argument2, int argument3)
{
    const unsigned int op = static_cast<unsigned int>(action)&0xFFu;
    switch (op)
    {
        case ACT_REPAIR:
        {
            VID* const owner = m_vid;
            if (!owner || owner->nVid != 82)
                return UNIT::Action(action, argument1, argument2, argument3);
            const int capacity = EngineActionAmmoCapacity(owner);
            const int selfAmmoWhole = (m_ammoFixedPoint+(m_ammoFixedPoint<0?63:0))/64;
            if (selfAmmoWhole >= capacity)
                return UNIT::Action(action, argument1, argument2, argument3);
            const unsigned int army = EngineActionArmy(this);
            const int selfCount = owner->spriteCountsByPlayer[army];
            VID* const slot70 = ActionVidSlot70();
            int available = slot70?static_cast<int>(slot70->spriteCountsByPlayer[army]):0;
            SPRITE_COLLECTOR* collector = ::Sprites;
            int cursor = 0;
            SPRITE* candidate = collector->FirstUnit(&cursor, -1);
            while (candidate)
            {
                VID* candidateVid = candidate->Vid();
                if (candidateVid && candidateVid->nVid == 82 && EngineActionSameArmy(this, candidate))
                {
                    ENGINE* const candidateEngine = static_cast<ENGINE*>(candidate);
                    const int fixed = candidateEngine->m_ammoFixedPoint;
                    available += (fixed+(fixed<0?63:0))/64;
                }
                candidate = collector->NextUnit(&cursor);
            }
            const int required = selfCount*EngineActionAmmoCapacity(owner);
            if (available >= required)
                return TERRAIN::Action(action, argument1, argument2, argument3);
            return UNIT::Action(action, argument1, argument2, argument3);
        }
        case ACT_UNDO_REMOVE:
            (void)SPRITE::Action(action, argument1, argument2, argument3);
        ClearDotBusy();
        if (m_prevEngine)
            m_prevEngine->m_nextEngine = 0;
        if (m_nextEngine)
            m_nextEngine->m_prevEngine = 0;
        return 0;
        case ACT_UNDO_INSERT:
            (void)SPRITE::Action(action, argument1, argument2, argument3);
        SetDotBusy();
        if (m_prevEngine)
            m_prevEngine->m_nextEngine = this;
        if (m_nextEngine)
            m_nextEngine->m_prevEngine = this;
        return 0;
        case ACT_SET_ARMY:
            RemoveFromPlayer();
        ChangeArmy(argument1);
        {
            VID* const owner = m_vid;
            WEAPON_RUNTIME* weapon = EngineActionWeapon(owner);
            if (weapon && weapon->Power != (0.0f))
                AddToStateBar();
        }
        return 0;
        case ACT_GET_GOAL:
        {
            ENGINE* first = FirstEngine();
            return first?static_cast<int>(PtrToU32(first->Goal())):0;
        }
        case ACT_RESTORE_COMMAND:
            SetCommandToTrain(static_cast<int>((static_cast<unsigned int>(action)>>8u)&0xFFu), reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1))), reinterpret_cast<R_DOT*>(U32ToPtr(static_cast<unsigned int>(argument2))), reinterpret_cast<R_DOT*>(U32ToPtr(static_cast<unsigned int>(argument3))));
        if (argument1)
            (void)reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)))->Release();
        return 0;
        case ACT_BACKUP_COMMAND:
        {
            const int command = static_cast<int>(((m_runtimeFlags>>2u)&0x1Fu)<<8u)+ACT_RESTORE_COMMAND;
            AddActionImmediate(0, command, static_cast<int>(PtrToU32(Goal())), static_cast<int>(PtrToU32(m_commandDot)), static_cast<int>(PtrToU32(m_pathEnd)));
            SPRITE* goal = Goal();
            if (goal)
                ++goal->m_referenceCount;
            return 0;
        }
        case ACT_NEXT_COMMAND:
        {
            const int animation = Animation();
            if (animation >= 15)
                return 0;
            const int accel = m_acceleration;
            if (accel>0)
            {
                if (animation != 3)
                    ChangeAnimation(3);
            }
            else if (accel<0)
            {
                if (animation != 1)
                    ChangeAnimation(1);
            }
            else if (m_speed == (0.0f))
            {
                if (animation != 0)
                    ChangeAnimation(0);
            }
            else if (animation != 2)
                ChangeAnimation(2);
            VID* const owner = m_vid;
            bool fighter = EngineActionHasWeaponDescriptor(owner);
            if (!fighter)
            {
                SPRITE* child = Link();
                VID* const childOwner = child?child->Vid():0;
                fighter = childOwner && childOwner == owner->linkVid && EngineActionHasWeaponDescriptor(childOwner);
            }
            if (fighter)
                ActNextCommandFighter();
            if ((m_runtimeFlags&0x7Cu) != 0x60u || !owner || owner->nVid != 85 || static_cast<unsigned int>(m_mineDeploymentReady) == 0u || m_speed != (0.0f))
                return 0;
            const unsigned int currentTick = ::CurrentTime;
            const unsigned int started = m_commandTick;
            if (started == 0u)
                m_commandTick = currentTick;
            else
            {
                CONSTANT* gameConstants = ::Const;
                if (gameConstants && static_cast<int>(currentTick-started)>gameConstants->RepairSettingMineTime)
                {
                    const int ammoWhole = (m_ammoFixedPoint+(m_ammoFixedPoint<0?63:0))/64;
                    if (ammoWhole>0)
                    {
                        if (m_commandDot)
                        {
                            VECTOR targetPosition(static_cast<float>(m_commandDot->m_x), static_cast<float>(m_commandDot->m_y), static_cast<float>(m_commandDot->m_z));
                            VID* markerVid = Map->Vid(595);
                            (void)Map->CreateSprite(markerVid, targetPosition, ANGLE(static_cast<unsigned char>(0u)), this);
                        }
                        const VECTOR current = Coor();
                        VID* shotVid = Map->Vid(86);
                        SPRITE* created = Map->CreateSprite(shotVid, current, Direction(), this);
                        if (created)
                        {
                            (void)created->Action(ACT_SET_BEHAVE, 0, 0, 0);
                            (void)Action(ACT_ADD_AMMO, -1, 0, 0);
                        }
                    }
                    SetCommandToTrain(0, 0, 0, 0);
                }
            }
            EngineActionSpawnLoopEffect(this);
            return 0;
        }
        case ACT_DAMAGE:
        {
            SPRITE* attacker = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument2)));
            if (argument1>0)
            {
                ENGINE* first = FirstEngine();
                if (first && (first->m_runtimeFlags&2u) == 0u && attacker)
                {
                    const unsigned int ownArmy = EngineActionArmy(this);
                    const unsigned int attackerArmy = EngineActionArmy(attacker);
                    PLAYER* const player = ActionPlayer(ownArmy);
                    if (player && player->m_relation[attackerArmy] == 1)
                    {
                        if (Map)
                            (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_TRAIN_ATTACKED], this, attacker, 0);
                        first->m_runtimeFlags |= 2u;
                    }
                }
            }
            SPRITE* child = Link();
            VID* owner = m_vid;
            VID* childOwner = child?child->Vid():0;
            if (child && owner && childOwner == owner->linkVid)
            {
                const unsigned int childArmy = EngineActionArmy(child);
                if (childOwner->maxHpByPlayer[childArmy] != 0)
                    return child->Action(action, argument1, argument2, argument3);
            }
            return SPRITE::Action(action, argument1, argument2, argument3);
        }
        case ACT_SAVE:
        {
            RESOURCE* resource = reinterpret_cast<RESOURCE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            (void)UNIT::Action(action, argument1, argument2, argument3);
            if (resource)
            {
                m_frontPos.Write(reinterpret_cast<STREAM*>(resource));
                m_backPos.Write(reinterpret_cast<STREAM*>(resource));
                const unsigned int previous = PtrToU32(m_prevEngine);
                const unsigned int next = PtrToU32(m_nextEngine);
                (void)resource->Write(&previous, 4u);
                (void)resource->Write(&next, 4u);
            }
            return 0;
        }
        case ACT_RESTORE:
            case ACT_LEGACY_RESTORE:
        {
            RESOURCE* resource = reinterpret_cast<RESOURCE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            (void)UNIT::Action(action, argument1, argument2, argument3);
            if (resource && argument2 >= 6)
            {
                R_POS* primary = &m_frontPos;
                R_POS* secondary = &m_backPos;
                primary->Read(reinterpret_cast<STREAM*>(resource));
                secondary->Read(reinterpret_cast<STREAM*>(resource));
                if (Map)
                {
                    m_prevEngine = reinterpret_cast<ENGINE*>(Map->ReadPointer(reinterpret_cast<STREAM*>(resource)));
                    m_nextEngine = reinterpret_cast<ENGINE*>(Map->ReadPointer(reinterpret_cast<STREAM*>(resource)));
                }
                SetDotBusy();
                R_DOT* node = primary->m_dot;
                unsigned char facing = 0u;
                if (node && static_cast<unsigned int>(primary->m_linkIndex)<static_cast<unsigned int>(node->m_linkCount))
                    facing = node->m_links[primary->m_linkIndex].direction.value;
                const unsigned char direction = m_direction.value;
                const unsigned char forwardDirectionDifference = static_cast<unsigned char>(direction-facing);
                const unsigned char reverseDirectionDifference = static_cast<unsigned char>(facing-direction);
                const unsigned char delta = forwardDirectionDifference<reverseDirectionDifference?forwardDirectionDifference:reverseDirectionDifference;
                if (delta>127u)
                    m_trainFlags |= 1u;
            }
            else if (argument2<6)
                SetRDot();
            VID* const owner = m_vid;
            WEAPON_RUNTIME* weapon = EngineActionWeapon(owner);
            if (weapon && weapon->Power != (0.0f))
                AddToStateBar();
            return 0;
        }
        case ACT_COOR_ATTACK:
        {
            VID* owner = m_vid;
            const int nvid = owner?owner->nVid:0;
            R_MAP* railMap = RailMap.Address();
            if (nvid == 85)
            {
                SetCommandToTrain(24, 0, railMap->GetNearestDot(argument1, argument2), 0);
                return 0;
            }
            if (nvid == 97 && !m_nextEngine && !m_prevEngine)
            {
                SetCommandToTrain(27, 0, railMap->GetNearestDot(argument1, argument2), 0);
                return 0;
            }
            const float x = static_cast<float>(argument1);
            const float y = static_cast<float>(argument2);
            float ground = 0.0f;
            if (Map)
            {
                const VECTOR2 xy(x, y);
                ground = Map->GetGroundZ(xy)+(19.0f);
            }
            const int groundInt = TruncateFloatToInt32(ground);
            const int helperY = groundInt+argument2-19;
            const VECTOR position(x, static_cast<float>(helperY), ground);
            void* const storage = static_cast<unsigned char*>(CrtNew(104u));
            SPRITE* const marker = storage?new (storage) SPRITE(position, ANGLE(0)):0;
            SetCommandToTrain(29, marker, 0, 0);
            return 0;
        }
        case ACT_MOVE:
        {
            const VECTOR point(static_cast<float>(argument1), static_cast<float>(argument2), 0.0f);
            Move(point, 0, 0);
            return 0;
        }
        case ACT_ATTACK:
        {
            SPRITE* target = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            if (!target)
                return 0;
            VID* owner = m_vid;
            if (owner && owner->nVid == 97u && !m_nextEngine && !m_prevEngine)
                SetCommandToTrain(27, target, 0, 0);
            else SetCommandToTrain(28, target, 0, 0);
            return 0;
        }
        case ACT_LINK_ENGINE:
            case ACT_FORCELINK_ENGINE:
        {
            SPRITE* target = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            if (!target || !target->Vid() || target->Vid()->spriteClass != 21u || InTrain(target))
                return 0;
            SetCommandToTrain(26, target, 0, 0);
            return 0;
        }
        case ACT_CLASH_ENGINE:
        {
            SPRITE* target = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            if (!target || !target->Vid() || target->Vid()->spriteClass != 21u || InTrain(target))
                return 0;
            VID* owner = m_vid;
            SPRITE* child = Link();
            if (owner && owner->nVid == 35u && child && child->Vid() == owner->linkVid)
                (void)child->SetCommand(9, target);
            else SetCommandToTrain(27, target, 0, 0);
            return 0;
        }
        case ACT_TRAIN_BEHAVE:
        {
            for (ENGINE* node = FirstEngine();node;node = node->m_nextEngine)
            {
                if (argument1 == 1)
                {
                    node->m_behave = 0u;
                    continue;
                }
                node->m_behave = 1u;
                const int capacity = EngineActionAmmoCapacity(node->m_vid);
                if (capacity <= 5 && (argument1 == 2 || argument1 == 4))
                    node->m_behave = 0u;
                if (argument1 == 4 || argument1 == 5)
                    node->m_behave |= 2u;
                if (argument1 == 3 || argument1 == 5)
                    node->m_behave |= 0x10u;
            }
            return 0;
        }
        case ACT_STOP:
            if (argument1) ForceStop();
        SetCommandToTrain(0, 0, 0, 0);
        return 0;
        case ACT_IS_TRAIN:
        {
            WEAPON_RUNTIME* weapon = EngineActionWeapon(m_vid);
            if (!weapon)
                return 0;
            return (weapon->Power-weapon->Weight>(1.0f))?1:0;
        }
        case ACT_FIRST_ENGINE:
            return static_cast<int>(PtrToU32(FirstEngine()));
        case ACT_LAST_ENGINE:
            return static_cast<int>(PtrToU32(LastEngine()));
        case ACT_NEXT_ENGINE:
            return static_cast<int>(PtrToU32(m_nextEngine));
        case ACT_IS_FIRST:
            return m_prevEngine?0:1;
        case ACT_IN_TRAIN:
            return InTrain(reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1))));
        default:
            return UNIT::Action(action, argument1, argument2, argument3);
    }
}

