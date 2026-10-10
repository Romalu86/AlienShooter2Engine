#include "mylib.h"
#include "engine_globals.h"

#include "basetype.h"
#include "sprite.h"
#include "engine.h"
#include "depo.h"
#include "unit.h"
#include "map.h"
#include "player.h"
#include "vid.h"
#include "sprite_collector.h"
#include "base_graph.h"
#include "graph.h"
#include "constant.h"
#include <new>



DEPO::DEPO(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : UNIT(vid, position, direction, parent)
{
    m_depotSerial = 0u;
    m_buildQueueIndex = 0;
    m_buildQueueCount = 0;
    m_buildQueueCapacity = 10;
    for (unsigned int i = 0u;i<static_cast<unsigned int>(BuildQueueStorageCount);++i)
    {
        m_queueUnit[i] = 0u;
        m_queueBuildTime[i] = 0u;
        m_queueState[i] = 0u;
    }
}

DEPO::~DEPO()
{
}



void DEPO::MoveTact()
{
    const unsigned int tick = ::CurrentTime;
    const unsigned int limit = ::PrevCurrentTime;
    if ((tick&0xFFFFC000u)>limit)
        m_runtimeFlags &= 0xFFFFFFFDu;
    const CONSTANT* const gameConstants = reinterpret_cast<const CONSTANT*>(::Const);
    AddHpPerSecond(gameConstants->DepoAutoAddHpPerSecond);
}

int DEPO::Action(int command, int argument1, int argument2, int argument3)
{
    switch (command)
    {
        case ACT_BUILD_UNIT:
            if (argument1 >= 0)
        {
            MAP* const map = Map;
            if (map->StoredVid(argument1) != 0)
            {
                AddUnitToQueue(argument1);
                BuildNextUnit();
            }
        }
        return 0;
        case ACT_BACKUP_COMMAND:
            if ((m_runtimeFlags&0x7Cu) == 0x40u)
        {
            const unsigned int queuedUnit = m_queueUnit[0];
            AddActionImmediate(0, ACT_BUILD_UNIT, static_cast<int>(queuedUnit), 0, 0);
            (void)SetCommand(0, static_cast<SPRITE*>(0));
            return 0;
        }
        return UNIT::Action(command, argument1, argument2, argument3);
        case ACT_SAVE:
            if (ActionStackHaveCommand(0, ACT_STOP_STACK) == 0) AddActionImmediate(0, ACT_STOP_STACK, 0, 0, 0);
        return UNIT::Action(command, argument1, argument2, argument3);
        case ACT_RESTORE:
            case ACT_LEGACY_RESTORE:
        {
            (void)UNIT::Action(command, argument1, argument2, argument3);
            MAP* map = Map;
            const unsigned int team = (m_runtimeFlags>>17)&7u;
            PLAYER* teamObject = map->Player(static_cast<int>(team));
            if (teamObject)
                teamObject->AddUnitToStateBar(this);
            EX_SPRITE_DATA* const extraData = m_extraData;
            if (extraData)
            {
                static const unsigned int priority[14] =
                {
                    5u, 10u, 20u, 25u, 80u, 85u, 45u, 30u, 35u, 82u, 97u, 90u, 75u, 62u
                }
                ;
                unsigned int sortedPrefixCount = 0u;
                for (unsigned int priorityIndex = 0;priorityIndex<14u;++priorityIndex)
                {
                    const unsigned int count = static_cast<unsigned int>(extraData->items.m_count);
                    for (unsigned int i = sortedPrefixCount;i<count;++i)
                    {
                        int* itemIds = extraData->items.m_data;
                        if (itemIds == 0)
                            break;
                        if (static_cast<unsigned int>(itemIds[i]) != priority[priorityIndex])
                            continue;
                        const int swappedItemId = itemIds[sortedPrefixCount];
                        itemIds[sortedPrefixCount] = itemIds[i];
                        itemIds[i] = swappedItemId;
                        ++sortedPrefixCount;
                    }
                }
            }
            return 0;
        }
        case ACT_DAMAGE:
            if (argument1>0 && (m_runtimeFlags&2u) == 0u && argument2 != 0)
        {
            MAP* const map = Map;
            SPRITE* const other = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument2)));
            const unsigned int ownTeam = (m_runtimeFlags>>17)&7u;
            const unsigned int otherTeam = (other->RuntimeFlags()>>17)&7u;
            PLAYER* relation = map->Player(static_cast<int>(ownTeam));
            if (relation && relation->RelationTo(otherTeam) == 1)
            {
                (void)map->ScriptRun(EvFunctionNumber[MAP_EVENT_DEPO_ATTACKED], this, other, 0);
                m_runtimeFlags |= 2u;
            }
        }
        return UNIT::Action(command, argument1, argument2, argument3);
        case ACT_SET_ARMY:
        {
            MAP* map = Map;
            const unsigned int oldTeam = (m_runtimeFlags>>17)&7u;
            PLAYER* oldTeamObject = map->Player(static_cast<int>(oldTeam));
            if (oldTeamObject)
                oldTeamObject->DeletePointerToSprite(this);
            ChangeArmy(argument1);
            const unsigned int newTeam = (m_runtimeFlags>>17)&7u;
            if (newTeam != oldTeam)
                (void)map->ScriptRun(EvFunctionNumber[MAP_EVENT_BUILDING_CAPTURE], this, 0, 0);
            PLAYER* newTeamObject = map->Player(static_cast<int>(newTeam));
            if (newTeamObject)
                newTeamObject->AddUnitToStateBar(this);
            return 0;
        }
        case ACT_NEXT_COMMAND:
        {
            const int animation = m_currentAnimation;
            if (animation >= 15)
                return 0;
            if (animation == 13)
                ChangeAnimation(0);
            const unsigned int queueIndex = static_cast<unsigned int>(m_buildQueueIndex);
            if (queueIndex != 0u && m_queueState[queueIndex-1u] == 0u)
            {
                if (m_stateTimer == 0u)
                {
                    (void)ActionBuildUnit(argument1, argument2);
                    return 0;
                }
                if (m_currentAnimation == 1)
                    return 0;
                ChangeAnimation(1);
                return 0;
            }
            if (m_currentAnimation != 0)
                ChangeAnimation(0);
            if ((m_runtimeFlags&0x7Cu) != 0u)
                (void)SetCommand(0, static_cast<SPRITE*>(0));
            (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_DEPO_FREE], this, 0, 0);
            return 0;
        }
        default:
            return UNIT::Action(command, argument1, argument2, argument3);
    }
}

void DEPO::AddUnitToQueue(int unitId)
{
    int count = m_buildQueueCount;
    if (count >= m_buildQueueCapacity)
        return;
    MAP* map = Map;
    const unsigned int team = (m_runtimeFlags>>17)&7u;
    PLAYER* teamObject = map->Player(static_cast<int>(team));
    if (!teamObject)
        return;
    const int available = teamObject->GetMoney();
    VID* vid = map->Vid(unitId);
    const int buildTime = vid->GetBuildTime();
    if (available<buildTime)
        return;
    map = Map;
    vid = map->Vid(unitId);
    teamObject = map->Player(static_cast<int>(team));
    teamObject->SetMoney(teamObject->GetMoney()-vid->GetBuildTime());
    count = m_buildQueueCount;
    m_queueUnit[count] = static_cast<unsigned short>(unitId);
    map = Map;
    vid = map->Vid(unitId);
    const int queuedBuildTime = vid->GetBuildTime();
    const CONSTANT* const globals = reinterpret_cast<const CONSTANT*>(::Const);
    m_queueBuildTime[count] = static_cast<unsigned int>(queuedBuildTime*globals->DepoMillisecondsInSecond);
    m_queueState[count] = 0u;
    m_buildQueueCount = count+1;
}

void DEPO::BuildNextUnit()
{
    if (m_buildQueueIndex != 0)
        return;
    m_buildQueueIndex = 1;
    if (m_queueState[0] != 0u)
    {
        while (m_buildQueueIndex <= m_buildQueueCapacity)
        {
            ++m_buildQueueIndex;
            if (m_queueState[m_buildQueueIndex-1] == 0u)
                break;
        }
    }
    const int index = m_buildQueueIndex;
    if (index <= m_buildQueueCount)
    {
        (void)SetCommand(16, static_cast<SPRITE*>(0));
        const int currentIndex = m_buildQueueIndex;
        const int unitId = static_cast<int>(m_queueUnit[currentIndex-1]);
        const unsigned int cached = m_queueBuildTime[currentIndex-1];
        if (cached)
        {
            m_stateTimer = cached;
            return;
        }
        VID* vid = Map->Vid(unitId);
        const int buildTime = vid->GetBuildTime();
        const CONSTANT* const globals = reinterpret_cast<const CONSTANT*>(::Const);
        m_stateTimer = static_cast<unsigned int>(buildTime*globals->DepoMillisecondsInSecond);
        return;
    }
    m_buildQueueIndex = 0;
    (void)SetCommand(0, static_cast<SPRITE*>(0));
    ChangeAnimation(0);
}

int DEPO::ActionBuildUnit(int , int )
{
    SPRITE* created = 0;
    for (;;)
    {
        const ANGLE direction = Direction();
        const float buildZ = Z();
        const float buildY = Y();
        const float buildX = X();
        const unsigned int queueIndex = static_cast<unsigned int>(m_buildQueueIndex);
        const unsigned int unitId = m_queueUnit[queueIndex-1u];
        VID* vid = Map->Vid(static_cast<int>(unitId));
        SPRITE_COLLECTOR* collector = ::Sprites;
        SPRITE* probe = collector->CanPlace( vid, buildX, buildY, buildZ, direction);
        if (!probe || probe == Mouse)
            break;
        delete probe;
    }
    const ANGLE direction = Direction();
    const VECTOR& position = Coor();
    const unsigned int queueIndex = static_cast<unsigned int>(m_buildQueueIndex);
    const unsigned int unitId = m_queueUnit[queueIndex-1u];
    VID* vid = Map->Vid(static_cast<int>(unitId));
    MAP* createMap = Map;
    created = createMap->CreateSprite(vid, position, direction, this);
    if (!created)
    {
        const STRING message("Depo can't create unit");
        const unsigned int failedIndex = static_cast<unsigned int>(m_buildQueueIndex);
        const unsigned int failedUnit = m_queueUnit[failedIndex-1u];
        static_cast<DIALOG_UNIT*>(static_cast<UNIT*>(this))->Error(E_ERROR, message, failedUnit);
        return 0;
    }
    VID* const owner = created->Vid();
    const bool isEngine = owner->spriteClass == 21u;
    if (isEngine)
        static_cast<ENGINE*>(created)->m_depotBatchId = BitsToSigned32(m_depotSerial);
    m_buildQueueCount = SubSigned32Wrap(m_buildQueueCount, 1);
    int from = m_buildQueueIndex-1;
    while (from<m_buildQueueCount)
    {
        m_queueUnit[from] = m_queueUnit[from+1u];
        m_queueBuildTime[from] = m_queueBuildTime[from+1u];
        m_queueState[from] = m_queueState[from+1u];
        ++from;
    }
    m_buildQueueIndex = 0;
    created->AddRuntimeFlags(1u);
    (void)Map->ScriptRun(EvFunctionNumber[MAP_EVENT_UNIT_CREATED], created, 0, 0);
    created->PlaySFX(105);
    bool finish = (m_buildQueueCount == 0);
    if (finish)
    {
        EX_SPRITE_DATA* const actionStack = m_extraData;
        if (actionStack)
        {
            const LIST<ACT>& lane = actionStack->actionLane[0];
            const unsigned int actionCount = static_cast<unsigned int>(lane.m_count);
            if (actionCount != 0u)
            {
                const ACT* const records = lane.m_data;
                finish = records[actionCount-1u].command == ACT_STOP_STACK;
            }
        }
    }
    if (!finish)
    {
        BuildNextUnit();
        return 0;
    }
    if (created->Vid()->spriteClass == 21u)
        static_cast<ENGINE*>(created)->m_depotBatchNotificationPending = 1;
    ++m_depotSerial;
    return 0;
}

float SPRITE::X() const
{
    return m_xyz.x;
}

float SPRITE::Y() const
{
    return m_xyz.y;
}

VID* MAP::Vid(int index) const
{
    if (index >= 0 && index<m_vidCount)
    {
        VID* const vid = m_vids[static_cast<unsigned int>(index)];
        if (vid)
            return vid;
    }
    return ::EmptyVid;
}


