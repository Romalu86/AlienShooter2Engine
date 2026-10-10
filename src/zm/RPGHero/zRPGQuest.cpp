#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"
#include "mylib.h"

#include "zRPGQuest.h"
#include "zRPGBaseObject.h"
#include "../Utils/String/zString.h"
#include "../Inventory/zInventory.h"
#include "../Menu/zSpriteInventory.h"
#include "zm/Game/zGroundItem.h"
#include "basetype.h"
#include "zm/Survive/zSurvive.h"
#include "zm/Quest/zQuestServer.h"
#include "map.h"
#include <cstring>
#include <new>


static int RPGQuestAbsDifference(int left, int right)
{
    unsigned int differenceBits = static_cast<unsigned int>(left)-static_cast<unsigned int>(right);
    const unsigned int signMask = 0u-(differenceBits>>31);
    differenceBits = (differenceBits^signMask)-signMask;
    return BitsToSigned32(differenceBits);
}

bool zRPGQuest::m__abArrayParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGQuest::m__abBaseParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGQuest::m__abChangeableParams[MAX_RPGPARAMSCOUNT] = {};

void zRPGQuest::SetDataArrays()
{
    if (m_changeableParams)
        return;
    m_changeableParams = m__abChangeableParams;
    m_baseParams = m__abBaseParams;
    m_arrayParams = m__abArrayParams;
    for (int paramIndex = 0;paramIndex<MAX_RPGPARAMSCOUNT;++paramIndex)
    {
        m_changeableParams[paramIndex] = false;
        m_baseParams[paramIndex] = false;
        m_arrayParams[paramIndex] = false;
    }
}

void zRPGQuest::SetLevelNum(int levelNum)
{
    m_levelNum = levelNum;
}
void zRPGQuest::SetQuestNum(int questNum)
{
    m_questNum = questNum;
}
void zRPGQuest::SetSubQNum(int subQuestNum)
{
    m_subQuestNum = subQuestNum;
}
int zRPGQuest::GetLevelNum()
{
    return m_levelNum;
}
int zRPGQuest::GetQuestNum()
{
    return m_questNum;
}
int zRPGQuest::GetSubQNum()
{
    return m_subQuestNum;
}
bool zRPGQuest::SetState(int state, bool validateTransition)
{
    const unsigned int newFlags = static_cast<unsigned int>(state);
    unsigned int oldFlags = static_cast<unsigned int>(m_state);
    bool changed = oldFlags != newFlags;
    if (changed && validateTransition)
    {
        iQuestServer* const questServer = static_cast<iQuestServer*>(GetFW()->QueryInterface(24));
        changed = questServer->InCurLevel(static_cast<iRPGQuest*>(this));
        if (!changed)
        {
            iLogConsole* logger = ::gpLogCons;
            (void)logger->LogStr(LT_INFO, "QS queststate %d %d was not set due to level", static_cast<unsigned int>(m_questNum), static_cast<unsigned int>(m_subQuestNum));
        }
    }
    if (changed)
    {
        oldFlags = static_cast<unsigned int>(m_state);
        if ((oldFlags&0x20u) == 0u || newFlags == 0u)
        {
            m_state = static_cast<int>(newFlags);
            if ((newFlags&1u) != 0u && (oldFlags&1u) == 0u)
                OnQuestBegin();
            if ((static_cast<unsigned int>(m_state)&2u) != 0u && (oldFlags&2u) == 0u)
                OnQuestComplete();
        }
        iQuestServer* const questServer = static_cast<iQuestServer*>(GetFW()->QueryInterface(24));
        questServer->SetDirtyState();
    }
    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "QS queststate was%sset %d %d %d", changed?" ":" not ", static_cast<unsigned int>(m_questNum), static_cast<unsigned int>(m_subQuestNum), newFlags);
    return changed;
}
bool zRPGQuest::AddState(int state, bool validateTransition)
{
    return SetState(GetState()|state, validateTransition);
}
int zRPGQuest::GetState()
{
    return m_state;
}
void zRPGQuest::SetTL(int timeline)
{
    m_timeLine = timeline;
}
int zRPGQuest::GetTL()
{
    return m_timeLine;
}
void zRPGQuest::SetBeginNext(bool beginNext)
{
    m_beginNext = beginNext;
}
bool zRPGQuest::GetBeginNext()
{
    return m_beginNext;
}
void zRPGQuest::SetUnactualNext(bool unactualNext)
{
    m_unactualNext = unactualNext;
}
bool zRPGQuest::GetUnactualNext()
{
    return m_unactualNext;
}
int zRPGQuest::GetNumStartQuests()
{
    return m_startQuests.m_begin ? static_cast<int>(m_startQuests.m_end-m_startQuests.m_begin) : 0;
}
int zRPGQuest::GetStartQuest(int questIndex)
{
    return m_startQuests.m_begin[questIndex];
}
int zRPGQuest::GetNumEndQuests()
{
    return m_endQuests.m_begin ? static_cast<int>(m_endQuests.m_end-m_endQuests.m_begin) : 0;
}
int zRPGQuest::GetEndQuest(int questIndex)
{
    return m_endQuests.m_begin[questIndex];
}
iRPGObj* zRPGQuest::AsRPGObj() const
{
    return static_cast<iRPGObj*>(const_cast<zRPGQuest*>(this));
}
int zRPGQuest::GetNamedMonstersDeath()
{
    return m_namedMonstersDeath;
}

void zRPGQuest::InitParams()
{
    SetArrayParam(RPGQUEST_STARTQUESTS);
    SetArrayParam(RPGQUEST_ENDQUESTS);
    SetArrayParam(RPGQUEST_ADDITEM);
    SetArrayParam(RPGQUEST_REMITEM);
}

void zRPGQuest::OnQuestComplete()
{
    const unsigned int soundId = static_cast<unsigned int>(GetSkillValue(RPGQUEST_SNDCOMPLETE));
    if (soundId == 0u)
        return;
    iRPGFrameWork* const framework = GetFW();
    framework->PlaySFXCurArmy(static_cast<int>(soundId));
}

bool zRPGQuest::GetStateFlagmanCoord(QUEST_STATE state, int& x, int& y, int& z, int& radius)
{
    const int* record = m_flagmanCoords[static_cast<int>(state)];
    x = record[0];
    y = record[1];
    z = record[2];
    radius = record[3];
    return x != 0 || y != 0 || z != 0 || radius != 0;
}
void zRPGQuest::SetStateFlagmanCoord(QUEST_STATE state, int x, int y, int z, int radius)
{
    int* record = m_flagmanCoords[static_cast<int>(state)];
    record[0] = x;
    record[1] = y;
    record[2] = z;
    record[3] = radius;
}
const char* zRPGQuest::GetStateItemName(QUEST_STATE state, CSI_ACTION action)
{
    const unsigned int stateActionIndex = static_cast<unsigned int>(action)+4u*static_cast<unsigned int>(state);
    zString* entry = m_stateItemNames+stateActionIndex;
    return entry->IsEmpty() ? 0 : entry->m_pBuf;
}
int zRPGQuest::GetStateItemVid(QUEST_STATE state, CSI_ACTION action)
{
    return m_stateItemVids[static_cast<int>(state)][static_cast<int>(action)];
}
void zRPGQuest::SetStateItemName(QUEST_STATE state, CSI_ACTION action, const char* itemName)
{
    const unsigned int stateActionIndex = static_cast<unsigned int>(action)+4u*static_cast<unsigned int>(state);
    m_stateItemNames[stateActionIndex].Set(itemName);
}
int zRPGQuest::GetNumItems(IT_TYPE type)
{
    const int itemType = static_cast<int>(type);
    if (itemType >= 0 && itemType<2)
    {
        const zQuestVector<int>& items = m_items[itemType];
        return items.m_begin ? static_cast<int>(items.m_end-items.m_begin) : 0;
    }
    (void)DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 386u);
    return 0;
}
int zRPGQuest::GetItem(IT_TYPE type, int itemIndex)
{
    const int itemType = static_cast<int>(type);
    if (itemType >= 0 && itemType<2)
        return m_items[itemType].m_begin[itemIndex];
    (void)DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 401u);
    return 0;
}
void zRPGQuest::SetCurTime(int currentTime)
{
    m_curTime = currentTime;
}
int zRPGQuest::GetCurTime()
{
    return m_curTime;
}
bool zRPGQuest::CanBegin(int limit)
{
    iRPGQuest* joinedQuest = GetJoin(static_cast<JOIN_TYPE>(1));
    return !joinedQuest || joinedQuest->GetTL() >= limit;
}


zRPGQuest::zRPGQuest(OBJ_TYPE questType)
    : zRPGObj(questType, RPGQUEST_PARAMS_COUNT),
      m_levelNum(0), m_questNum(0), m_subQuestNum(0), m_state(0), m_timeLine(0)
{
    m_curTime = 0;
    m_beginNext = 0;
    m_unactualNext = 0;
    m_namedMonstersDeath = -1;
    m_iOldState = -1;
    m_iOldExtInfoQuest = -1;
    m_savedCurTime = -1;
    if (static_cast<int>(questType) != TYPE_QUEST && static_cast<int>(questType) != TYPE_LEVEL)
        DebugAssert(5u, "_eType == TYPE_QUEST || _eType == TYPE_LEVEL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 50u);
    m_gameObjectExtType = 14;
    for (int stateIndex = 0;stateIndex<8;++stateIndex)
        for (int componentIndex = 0;componentIndex<4;++componentIndex)
    {
        m_flagmanCoords[stateIndex][componentIndex] = 0;
        m_stateItemVids[stateIndex][componentIndex] = 0;
    }
    m_join[0] = 0;
    m_join[1] = 0;
    m_joinId[0] = 0;
    m_joinId[1] = 0;
}


zRPGQuest::~zRPGQuest()
{
}


void zRPGQuest::OnSkillChange(int selector, int oldValue, int newValue)
{
    zRPGObj::OnSkillChange(selector, oldValue, newValue);
    zQuestVector<int>* vector = 0;
    if (selector == RPGQUEST_STARTQUESTS)
        vector = &m_startQuests;
    else if (selector == RPGQUEST_ENDQUESTS)
        vector = &m_endQuests;
    else if (selector == RPGQUEST_ADDITEM)
        vector = &m_items[0];
    else if (selector == RPGQUEST_REMITEM)
        vector = &m_items[1];
    else return;
    vector->insert(vector->m_end, 1, newValue);
}


void zRPGQuest::OnQuestBegin()
{
    iRPGFrameWork* framework = GetFW();
    if (!framework)
        (void)DebugAssert(5u, "GetFW()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 162u);
    framework = GetFW();
    iQuestServer* const questServer = static_cast<iQuestServer*>(framework->QueryInterface(24));
    iRPGQuest* const questInterface = static_cast<iRPGQuest*>(this);
    questServer->AddBonuses(questInterface);
    const unsigned int beginSound = static_cast<unsigned int>(GetSkillValue(RPGQUEST_SNDBEGUN));
    if (beginSound != 0u)
    {
        framework = GetFW();
        framework->PlaySFXCurArmy(static_cast<int>(beginSound));
    }
    const int infoQuestId = GetSkillValue(RPGQUEST_SETINFOQUEST);
    if (infoQuestId != 0)
    {
        iRPGQuest* infoQuest = questServer->IdToQuest(infoQuestId);
        int infoValue = questServer->QuestToId(questInterface);
        const int overrideValue = GetSkillValue(RPGQUEST_EXTINFOQUEST);
        if (overrideValue != 0)
            infoValue = overrideValue;
        iRPGObj* const infoObject = static_cast<iRPGObj*>(infoQuest->QueryInterface(9));
        (void)infoObject->SetSkillValue(RPGQUEST_EXTINFOQUEST, infoValue);
    }
    if (GetGameObj() == TYPE_LEVEL)
        return;
    const int rowCount = questServer->GetQuestNum();
    for (int questNumber = 1;questNumber <= rowCount;++questNumber)
    {
        const int columnCount = questServer->GetSubQuestNum(questNumber);
        for (int subQuestNumber = 1;subQuestNumber<columnCount;++subQuestNumber)
        {
            iRPGQuest* subQuest = questServer->GetQuest(questNumber, subQuestNumber, true, true);
            if (!subQuest)
                (void)DebugAssert(5u, "buf", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 200u);
            const int stateTimeLine = subQuest->GetTL();
            if (m_timeLine <= stateTimeLine)
                continue;
            if ((static_cast<unsigned int>(subQuest->GetState())&0x20u) != 0u)
                continue;
            if (questServer->AreLinked(questInterface, subQuest, false))
                (void)subQuest->AddState(0x20, false);
        }
    }
    for (int index = 0;index<(m_startQuests.m_begin ?         static_cast<int>(m_startQuests.m_end-m_startQuests.m_begin) : 0);++index)
    {
        const int startQuest = static_cast<int>(m_startQuests.m_begin[index]);
        iRPGQuest* subQuest = questServer->GetQuest(m_questNum, startQuest, false, true);
        if (subQuest)
            (void)subQuest->AddState(1, false);
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_NOTICE, "QS set from OnQuestBegin %d - %d - %d", static_cast<unsigned int>(m_questNum), m_startQuests.m_begin[index], 1u);
    }
    iRPGQuest* subQuest = questServer->GetQuest(m_questNum, 0, false, true);
    if (!subQuest)
        (void)DebugAssert(5u, "MainQuest", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 234u);
    if (m_subQuestNum != 0 && (static_cast<unsigned int>(subQuest->GetState())&1u) == 0u)
        (void)subQuest->AddState(1, false);
    const int timeLine = m_timeLine;
    if (timeLine>subQuest->GetCurTime())
        subQuest->SetCurTime(timeLine);
    if (GetSkillValue(RPGQUEST_UNNEEDED) != 0)
    {
        iRPGQuest* joined = GetJoin(static_cast<JOIN_TYPE>(0));
        if (joined)
            (void)joined->AddState(1, false);
    }
    if (GetSkillValue(RPGQUEST_WINONBEGIN) != 0)
    {
        framework = GetFW();
        framework->WinLevel();
    }
}

void* zRPGQuest::QueryInterface(int interfaceId) const
{
    const unsigned int id = static_cast<unsigned int>(interfaceId);
    if (id == 56u)
        return const_cast<zRPGQuest*>(this);
    if (id == 10u)
        return static_cast<iRPGQuest*>(const_cast<zRPGQuest*>(this));
    return zGroundItem::QueryInterface(static_cast<int>(id));
}

int zRPGQuest::iRelease()
{
    if (this)
    {
        this->~zRPGQuest();
        CrtFreePointer(this);
    }
    return 0;
}

void zRPGQuest::ReleaseItems(IT_TYPE type)
{
    zQuestVector<int>& items = m_items[static_cast<int>(type) == 0 ? 0 : 1];
    items.m_end = items.m_begin;
}
void zRPGQuest::AddItems(IT_TYPE type, int itemId)
{
    zQuestVector<int>& items = m_items[static_cast<int>(type) == 0 ? 0 : 1];
    items.insert(items.m_end, 1, itemId);
}
void zRPGQuest::CheckFlagmanCoord(int worldX, int worldY)
{
    iRPGFrameWork* framework = GetFW();
    if (!framework)
        (void)DebugAssert(5u, "GetFW()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 435u);
    framework = GetFW();
    iQuestServer* const questServer = static_cast<iQuestServer*>(framework->QueryInterface(24));
    const int questNum = m_questNum;
    const int count = questServer->GetSubQuestNum(questNum);
    for (int stateIndex = 0;stateIndex<count;++stateIndex)
    {
        iRPGQuest* SubQuest = questServer->GetQuest(questNum, stateIndex, true, true);
        if (!SubQuest)
            (void)DebugAssert(5u, "SubQuest", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 445u);
        const unsigned int flags = static_cast<unsigned int>(SubQuest->GetState());
        if ((flags&0x20u) != 0u)
            continue;
        if (!CanBegin(m_curTime))
            continue;
        for (unsigned int questState = 0;questState<8u;++questState)
        {
            if ((flags&questState) != 0u)
                continue;
            int x, y, z, radius;
            (void)SubQuest->GetStateFlagmanCoord(static_cast<QUEST_STATE>(questState), x, y, z, radius);
            if (x == 0 && y == 0 && z == 0 && radius == 0)
                continue;
            const int dx = RPGQuestAbsDifference(x, worldX);
            const int dy = RPGQuestAbsDifference(y, worldY);
            const unsigned int metricBits = dx>dy                 ?static_cast<unsigned int>(dx)+static_cast<unsigned int>(dy/2)                 :static_cast<unsigned int>(dy)+static_cast<unsigned int>(dx/2);
            if (BitsToSigned32(metricBits)>radius)
                continue;
            (void)questServer->SetQuestState(SubQuest, static_cast<int>(questState), true);
            iLogConsole* logger = ::gpLogCons;
            (void)logger->LogStr(LT_NOTICE, "QS set from CheckFlagmanCoor %d - %d - %d", flags, static_cast<unsigned int>(stateIndex), questState);
        }
    }
}
void zRPGQuest::CheckStateItemNameOrVid(CSI_ACTION action, const char* _sItemName, int _iVidNum, int _iNamedMonsterId)
{
    if (!_sItemName && _iVidNum == 0 && _iNamedMonsterId == 0)
        DebugAssert(4u, "_sItemName || _iVidNum || _iNamedMonsterId", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 480u);
    iRPGFrameWork* framework = GetFW();
    if (!framework)
        DebugAssert(5u, "GetFW()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 481u);
    framework = GetFW();
    iQuestServer* const questServer = static_cast<iQuestServer*>(framework->QueryInterface(24));
    const int questNum = m_questNum;
    const int stateCount = questServer->GetSubQuestNum(questNum);
    for (int stateIndex = 0;stateIndex<stateCount;++stateIndex)
    {
        iRPGQuest* SubQuest = questServer->GetQuest(questNum, stateIndex, true, true);
        if (!SubQuest)
            DebugAssert(5u, "SubQuest", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 491u);
        if ((static_cast<unsigned int>(SubQuest->GetState())&0x20u) != 0u)
            continue;
        if (!CanBegin(m_curTime))
            continue;
        for (unsigned int questState = 0;questState<8u;++questState)
        {
            bool match = false;
            const char* logFormat = 0;
            if (_sItemName)
            {
                const char* stateName = SubQuest->GetStateItemName(                     static_cast<QUEST_STATE>(questState), action);
                if (stateName && std::strcmp(stateName, _sItemName) == 0)
                {
                    match = true;
                    logFormat = "QS set from CheckStateItemName %d - %d - %d";
                }
            }
            else if (_iVidNum != 0)
            {
                if (SubQuest->GetStateItemVid(static_cast<QUEST_STATE>(questState), action) == _iVidNum)
                {
                    match = true;
                    logFormat = "QS set from CheckStateItemVid %d - %d - %d";
                }
            }
            else
            {
                if (_iNamedMonsterId == 0)
                    DebugAssert(4u, "_iNamedMonsterId", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 519u);
                if (SubQuest->GetNamedMonstersDeath() == _iNamedMonsterId && questState == 1u)
                {
                    match = true;
                    logFormat = "QS set from CheckNamedMonster %d - %d - %d";
                }
            }
            if (!match)
                continue;
            (void)questServer->SetQuestState(SubQuest, static_cast<int>(questState), true);
            iLogConsole* logger = ::gpLogCons;
            (void)logger->LogStr(LT_NOTICE, logFormat, static_cast<unsigned int>(questNum), static_cast<unsigned int>(stateIndex), questState);
        }
    }
}
const char* zRPGQuest::GetMainQuestInfo()
{
    if (m_subQuestNum != 0)
        return 0;
    iRPGFrameWork* const framework = GetFW();
    iQuestServer* const questServer = static_cast<iQuestServer*>(framework->QueryInterface(24));
    const int subQuestCount = questServer->GetSubQuestNum(m_questNum);
    int bestTimeline = -1;
    const char* bestInfo = 0;
    for (int stateIndex = 0;stateIndex<subQuestCount;++stateIndex)
    {
        iRPGQuest* buf = questServer->GetQuest(m_questNum, stateIndex, true, true);
        if (!buf)
            (void)DebugAssert(5u, "buf", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest.cpp", 549u);
        const int timeline = buf->GetTL();
        if (timeline>m_curTime || timeline <= bestTimeline)
            continue;
        const char* info = buf->GetInfoStr(static_cast<iRPGQuest::INFO_STR>(0));
        if (info)
        {
            bestInfo = info;
            bestTimeline = timeline;
        }
    }
    return bestInfo;
}
void zRPGQuest::SetInfoStr(const char* text, iRPGQuest::INFO_STR infoType)
{
    iRPGObj* const questObject = AsRPGObj();
    zString key(questObject->GetName());
    key += static_cast<int>(infoType) != 0?"Description":"Coment2";
    iTextDB* const strings = GetFW()->AsTextDB();
    strings->SetObjInfo(key.m_pBuf, text);
    SetCharValue(static_cast<int>(infoType) != 0?RPG_DESC:RPG_COMENT2, key.m_pBuf);
}
const char* zRPGQuest::GetInfoStr(iRPGQuest::INFO_STR infoType)
{
    zString key(GetCharValue(static_cast<int>(infoType) != 0?RPG_DESC:RPG_COMENT2));
    iTextDB* const strings = GetFW()->AsTextDB();
    return key.IsEmpty()?0:strings->GetObjInfo(key.m_pBuf);
}


