#include <cstdio>
#include "mylib.h"
#include <cstring>

#include "zRPGQuest.h"
#include "zRPGBaseObject.h"
#include <cstdlib>
#include "../Utils/String/zString.h"
#include "basetype.h"
#include "zm/Quest/zQuestServer.h"

namespace {
const char* const kQuestStateNames[8] = {"QS_NOTBEGUN", "QS_BEGUN", "QS_COMPLETED", "", "QS_FAILED", 0, 0, 0};
const char* const kQuestItemActionNames[] = {"CSI_TAKE", "CSI_DEATH", "CSI_BIRTH", "CSI_HAVE"};
}

unsigned int __cdecl RPGQuestDecodeConditionToken(const char* text)
{
    if (std::strcmp(text, kQuestStateNames[0]) == 0)
        return 0u;
    if (std::strcmp(text, kQuestStateNames[1]) == 0)
        return 1u;
    if (std::strcmp(text, kQuestStateNames[2]) == 0)
        return 2u;
    if (std::strcmp(text, kQuestStateNames[4]) == 0)
        return 4u;
    return 16u;
}

unsigned int __cdecl RPGQuestDecodeActionToken(const char* text)
{
    if (std::strcmp(text, kQuestItemActionNames[0]) == 0)
        return 0u;
    if (std::strcmp(text, kQuestItemActionNames[1]) == 0)
        return 1u;
    if (std::strcmp(text, kQuestItemActionNames[2]) == 0)
        return 2u;
    if (std::strcmp(text, kQuestItemActionNames[3]) == 0)
        return 3u;
    return 5u;
}

void zRPGQuest::SetName(const char* name, bool parseQuestNumbers)
{
    zRPGObj::SetName(name, parseQuestNumbers);
    if (GetGameObj() == TYPE_QUEST)
    {
        (void)std::sscanf(name, "%d_%d_%d",                          &m_levelNum, &m_questNum, &m_subQuestNum);
    }
}

bool zRPGQuest::GetParams(const char* text, int length, int& parameterCount, int* parameterIndices) const
{
    parameterCount = 1;
    *parameterIndices = -1;
    const unsigned int tokenLength = static_cast<unsigned int>(length);
    if (std::strncmp("AddMoneyMAIN", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_ADDMONEY;
    else if (std::strncmp("AddMoneyALTERNATIVE", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_ADDMONEY2;
    else if (std::strncmp("AddExperienceMAIN", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_ADDEXPERIENCE;
    else if (std::strncmp("AddExperienceALTERNATIVE", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_ADDEXPERIENCE2;
    else if (std::strncmp("DialogNumMAIN", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_DIALOGNUM;
    else if (std::strncmp("DialogNumALTERNATIVE", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_DIALOGNUM2;
    else if (std::strncmp("StartingQuests", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_STARTQUESTS;
    else if (std::strncmp("EndQuests", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_ENDQUESTS;
    else if (std::strncmp("Unneeded", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_UNNEEDED;
    else if (std::strncmp("SndBegun", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_SNDBEGUN;
    else if (std::strncmp("SndComplete", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_SNDCOMPLETE;
    else if (std::strncmp("SetInfoQuest", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_SETINFOQUEST;
    else if (std::strncmp("Accomplished", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_ACCOMPLISHED;
    else if (std::strncmp("WinOnBegin", text, tokenLength) == 0)
        *parameterIndices = RPGQUEST_WINONBEGIN;
    return *parameterIndices != -1;
}

bool zRPGQuest::ParseUnknownCommand(const char* name, int count, const char** values,
                                    iRPGFrameWork* framework)
{
    const bool baseHandled = zRPGObj::ParseUnknownCommand(         name, count, values, framework);
    iRPGFrameWork* const provider = GetFW();
    iQuestServer* const questServer = static_cast<iQuestServer*>(provider->QueryInterface(24));
    if (baseHandled)
        return true;
    if (strcmp(name, "LevelNum") == 0)
    {
        m_levelNum = BitsToSigned32(CrtAtoiText(values[0]));
        return true;
    }
    if (strcmp(name, "TimeLine") == 0)
    {
        m_timeLine = BitsToSigned32(CrtAtoiText(values[0]));
        return true;
    }
    if (strcmp(name, "NextQuests") == 0)
    {
        iRPGFrameWork* const joinProvider = GetFW();
        iTextDB* const joinResolver = joinProvider->AsTextDB();
        int questId = BitsToSigned32(CrtAtoiText(values[0]));
        if (joinResolver->GetCurFileVer() == 0)
        {
            const int questNumber = BitsToSigned32(CrtAtoiText(values[0]));
            questId = questServer->QuestToId(m_questNum, questNumber);
        }
        SetJoin(static_cast<JOIN_TYPE>(0), questId);
        return true;
    }
    if (strcmp(name, "LineQuests") == 0)
    {
        if (count != 2)
            return false;
        if (BitsToSigned32(CrtAtoiText(values[0])) <= 0 || BitsToSigned32(CrtAtoiText(values[0])) >= count)
            DebugAssert(5u, "atoi( _Params[ 0 ] ) > 0 && atoi( _Params[ 0 ] ) < JT_COUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest_loadsave.cpp", 180u);
        const int joinValue = BitsToSigned32(CrtAtoiText(values[1]));
        const JOIN_TYPE joinType = static_cast<JOIN_TYPE>(BitsToSigned32(CrtAtoiText(values[0])));
        SetJoin(joinType, joinValue);
        return true;
    }
    if (strcmp(name, "AddItemMAIN") == 0)
    {
        for (int i = 0;i<count;++i)
        {
            iRPGObj* object = framework->GetObjByNamee(values[i], true);
            if (object)
                SetSkillValue(RPGQUEST_ADDITEM, object->GetBaseId());
        }
        return true;
    }
    if (strcmp(name, "RemItem") == 0)
    {
        for (int i = 0;i<count;++i)
        {
            iRPGObj* object = framework->GetObjByNamee(values[i], true);
            if (object)
                SetSkillValue(RPGQUEST_REMITEM, object->GetBaseId());
        }
        return true;
    }
    if (strcmp(name, "ChangeStateItem") == 0)
    {
        if (count != 3)
            return false;
        const unsigned int condition = RPGQuestDecodeConditionToken(values[0]);
        const unsigned int ItemAction = RPGQuestDecodeActionToken(values[1]);
        if (condition == 16u || ItemAction == 5u)
            return false;
        const int numericValue = BitsToSigned32(CrtAtoiText(values[2]));
        const unsigned int matrixIndex = ItemAction+condition*4u;
        if (numericValue)
        {
            questServer->AddNeededVid(numericValue);
            m_stateItemVids[condition][ItemAction] = numericValue;
        }
        else if (strlen(values[2]) == 2u && values[2][0] == 'm' &&                  values[2][1] >= '0' && values[2][1] <= '9')
        {
            if (ItemAction != 1u)
                DebugAssert(4u, "ItemAction == CSI_DEATH", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest_loadsave.cpp", 229u);
            m_namedMonstersDeath = values[2][1]-'0';
        }
        zString* stateString = m_stateItemNames+matrixIndex;
        stateString->Set(values[2]);
        return true;
    }
    if (strcmp(name, "SetFlagmanCoord") == 0)
    {
        if (count != 5)
            return false;
        const unsigned int condition = RPGQuestDecodeConditionToken(values[0]);
        if (condition == 16u)
            return false;
        int* coord = m_flagmanCoords[condition];
        for (int i = 0;i<4;++i)
            coord[i] = BitsToSigned32(CrtAtoiText(values[i+1]));
        return true;
    }
    const bool isComment = strcmp(name, "Comment") == 0;
    if (isComment || strcmp(name, "Description") == 0)
    {
        if (count>0)
        {
            zString joined;
            joined.Set(values[0]);
            for (int valueIndex = 1;valueIndex<count;++valueIndex)
            {
                (void)(joined += " ");
                (void)(joined += values[valueIndex]);
            }
            SetCharValue(isComment?RPG_COMMENT:RPG_DESC, joined.m_pBuf);
        }
        return true;
    }
    if (strcmp(name, "Coment2") == 0)
    {
        if (count>0)
        {
            iRPGFrameWork* const infoProvider = GetFW();
            iTextDB* const info = infoProvider->AsTextDB();
            if (info->GetCurFileVer()>1)
            {
                SetCharValue(RPG_COMENT2, values[0]);
            }
            else
            {
                zString joined;
                joined.Set(values[0]);
                for (int valueIndex = 1;valueIndex<count;++valueIndex)
                {
                    (void)(joined += " ");
                    (void)(joined += values[valueIndex]);
                }
                SetInfoStr(joined.m_pBuf, static_cast<iRPGQuest::INFO_STR>(0));
            }
        }
        return true;
    }
    if (strcmp(name, "State") == 0)
    {
        m_state = BitsToSigned32(CrtAtoiText(values[0]));
        return true;
    }
    if (strcmp(name, "ExtInfoQuest") == 0)
    {
        SetSkillValue(RPGQUEST_EXTINFOQUEST, BitsToSigned32(CrtAtoiText(values[0])));
        return true;
    }
    if (strcmp(name, "CurTime") == 0)
    {
        m_curTime = BitsToSigned32(CrtAtoiText(values[0]));
        return true;
    }
    if (strcmp(name, "BeginNext") == 0)
    {
        if (BitsToSigned32(CrtAtoiText(values[0])) != 0)
            SetBeginNext(true);
        return true;
    }
    if (strcmp(name, "UnactualNext") == 0)
    {
        if (BitsToSigned32(CrtAtoiText(values[0])) != 0)
            SetUnactualNext(true);
        return true;
    }
    return false;
}

void zRPGQuest::PushCurState(std::FILE* stream)
{
    m_iOldState = m_state;
    m_iOldExtInfoQuest = GetSkillValue(RPGQUEST_EXTINFOQUEST);
    m_savedCurTime = m_curTime;
    if (!stream)
        return;
    if (m_iOldState || m_iOldExtInfoQuest || m_savedCurTime)
    {
        iRPGFrameWork* const fw = GetFW();
        const char* const objectName = GetName();
        const char* const typeName = fw->GetTypeName(static_cast<OBJ_TYPE>(GetGameObj()));
        (void)std::fprintf(stream, "%s=%s\n", typeName, objectName);
        const int oldState = m_iOldState;
        if (oldState)
            (void)std::fprintf(stream, "%s=%d\n", "State", oldState);
        const int oldExt = m_iOldExtInfoQuest;
        if (oldExt)
            (void)std::fprintf(stream, "%s=%d\n", "ExtInfoQuest", oldExt);
        const int oldCurTime = m_savedCurTime;
        if (oldCurTime)
            (void)std::fprintf(stream, "%s=%d\n", "CurTime", oldCurTime);
        (void)std::fprintf(stream, "\n");
    }
}

void zRPGQuest::PopCurState(std::FILE* stream)
{
    (void)stream;
    if (m_iOldState<0)
        DebugAssert(4u, "m_iOldState >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest_loadsave.cpp", 374u);
    if (m_iOldExtInfoQuest<0)
        DebugAssert(4u, "m_iOldExtInfoQuest >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest_loadsave.cpp", 375u);
    m_state = m_iOldState;
    SetSkillValue(RPGQUEST_EXTINFOQUEST, m_iOldExtInfoQuest);
    m_curTime = m_savedCurTime;
}

void zRPGQuest::SetJoin(JOIN_TYPE _eType, int joinedQuestId)
{
    const int joinIndex = static_cast<int>(_eType);
    if (joinIndex<0 || joinIndex >= 2)
        DebugAssert(5u, "_eType >= JT_NEXT && _eType < JT_COUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest_loadsave.cpp", 387u);
    m_joinId[joinIndex] = joinedQuestId;
}

bool zRPGQuest::SetJoin(JOIN_TYPE _eType, iRPGQuest* joinedQuest, bool active)
{
    const int joinIndex = static_cast<int>(_eType);
    if (joinIndex<0 || joinIndex >= 2)
        DebugAssert(5u, "_eType >= JT_NEXT && _eType < JT_COUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest_loadsave.cpp", 395u);
    if (active && joinedQuest && (joinIndex == 0 || joinIndex == 1))
    {
        const int joinedTimeline = joinedQuest->GetTL();
        const int currentTimeline = GetTL();
        if ((joinIndex == 0 && currentTimeline >= joinedTimeline) || (joinIndex == 1 && currentTimeline <= joinedTimeline))
            return false;
    }
    m_join[joinIndex] = joinedQuest;
    m_joinId[joinIndex] = 0;
    return true;
}

iRPGQuest* zRPGQuest::GetJoin(JOIN_TYPE _eType)
{
    const int joinIndex = static_cast<int>(_eType);
    if (joinIndex<0 || joinIndex >= 2)
        DebugAssert(5u, "_eType >= JT_NEXT && _eType < JT_COUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGQuest_loadsave.cpp", 420u);
    iRPGQuest** slot = &m_join[joinIndex];
    if (!*slot && m_joinId[joinIndex])
    {
        iRPGFrameWork* const provider = GetFW();
        iQuestServer* const registry = static_cast<iQuestServer*>(provider->QueryInterface(24));
        *slot = registry->IdToQuest(m_joinId[joinIndex]);
    }
    return *slot;
}


const char* zRPGQuest::GetStrByIemAction(CSI_ACTION action)
{
    return kQuestItemActionNames[static_cast<unsigned int>(action)];
}

const char* zRPGQuest::GetStrByState(QUEST_STATE state)
{
    return kQuestStateNames[static_cast<unsigned int>(state)];
}



