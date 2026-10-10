#include <cstdio>
#include "../../engine_globals.h"
#include "zm/Utils/zLogConsole.h"
#include "mylib.h"
#include "basetype.h"
#include "zm/IniParser/zIniParser.h"
#include "zm/Utils/String/zString.h"
#include <new>
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <windows.h>
#pragma pop_macro("INPUT")

#include "zQuestServer.h"
#include "zm/Eng/zProfLdr/zProfLdr.h"
#include "zm/RPGHero/zRPGFrameWork.h"
#include "zm/RPGHero/zRPGQuest.h"
#include "myerror.h"


void zQuestServer::SetDirtyState()
{
    m_dirtyState = true;
}

bool zQuestServer::SetCurLevel(iRPGQuest* _pCurLevel)
{
    DebugAssert(2, "0",         "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 720);
    if (_pCurLevel == 0)
        DebugAssert(2, "_pCurLevel",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 722);
    return 0;
}

int zQuestServer::GetQuestState(iRPGQuest* _pQuest)
{
    if (_pQuest == 0)
        DebugAssert(5, "_pQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 172);
    return _pQuest->GetState();
}

int zQuestServer::QuestToId(iRPGQuest* _pQuest)
{
    if (_pQuest == 0)
        DebugAssert(5, "_pQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 246);
    const int subQuestNum = _pQuest->GetSubQNum();
    const int questNum = _pQuest->GetQuestNum();
    return QuestToId(questNum, subQuestNum);
}

iRPGQuest* zQuestServer::FirstQuest(OBJ_TYPE typeValue)
{
    m_iterationType = typeValue;
    iRPGObj* cursor = zRPGFrameWork::FirstObject(0x13, 14, 0);
    if (cursor == 0)
        return 0;
    iRPGQuest* quest = static_cast<iRPGQuest*>(cursor->QueryInterface(10));
    if (quest == 0)
        return 0;
    if (m_iterationType == 14 || cursor->GetGameObj() == m_iterationType)
        return quest;
    return NextQuest();
}

iRPGQuest* zQuestServer::NextQuest()
{
    iRPGObj* cursor = zRPGFrameWork::NextObject();
    while (cursor != 0)
    {
        if (m_iterationType == 14 || cursor->GetGameObj() == m_iterationType)
            return static_cast<iRPGQuest*>(cursor->QueryInterface(10));
        cursor = zRPGFrameWork::NextObject();
    }
    return 0;
}

iRPGQuest* zQuestServer::GetQuest(
    int questNum, int subQuestNumOrIndex, bool useIndex, bool assertIfMissing)
{
    int matchingIndex = 0;
    iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST));
    iRPGQuest* ret = 0;
    while (quest != 0)
    {
        if (quest->GetQuestNum() == questNum)
        {
            if (useIndex)
            {
                if (matchingIndex == subQuestNumOrIndex)
                {
                    ret = quest;
                    break;
                }
                ++matchingIndex;
            }
            else if (quest->GetSubQNum() == subQuestNumOrIndex)
            {
                ret = quest;
                break;
            }
        }
        quest = NextQuest();
    }
    if (assertIfMissing && ret == 0)
    {
        iLogConsole* const logger = ::gpLogCons;
        logger->LogStr(LT_WARN, "Invaild Quest %d %d %d!",             questNum, subQuestNumOrIndex, static_cast<unsigned char>(useIndex));
        DebugAssert(4, "ret",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp",             141);
    }
    return ret;
}

iRPGQuest* zQuestServer::GetLevel(int levelIndex)
{
    int candidateLevelIndex = 0;
    iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_LEVEL));
    while (quest != 0)
    {
        if (candidateLevelIndex == levelIndex)
            return quest;
        ++candidateLevelIndex;
        quest = NextQuest();
    }
    DebugAssert(4, "ret",         "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp",         163);
    return 0;
}

zQuestServer::zQuestServer()
    : m_dirtyState(true), m_iterationType(14), m_savedStatePresent(false)
{
    for (unsigned int i = 0;i != 4096u;++i)
        m_neededVids[i] = 0u;
}



zQuestServer::~zQuestServer()
{
}



int zQuestServer::GetLevelsNum()
{
    int levelCount = 0;
    iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_LEVEL));
    while (quest != 0)
    {
        ++levelCount;
        quest = NextQuest();
    }
    return levelCount;
}

int zQuestServer::GetQuestNum()
{
    int mainQuestCount = 0;
    iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST));
    while (quest != 0)
    {
        if (quest->GetSubQNum() == 0)
            ++mainQuestCount;
        quest = NextQuest();
    }
    return mainQuestCount;
}

int zQuestServer::GetSubQuestNum(int questNum)
{
    int subQuestCount = 0;
    iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST));
    while (quest != 0)
    {
        if (quest->GetQuestNum() == questNum)
            ++subQuestCount;
        quest = NextQuest();
    }
    return subQuestCount;
}

bool zQuestServer::SetQuestState(
    iRPGQuest* _pQuest, int state, bool validateTransition)
{
    if (_pQuest == 0)
        DebugAssert(5, "_pQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp",             182);
    return _pQuest->AddState(state, validateTransition);
}

bool zQuestServer::GetDirtyState()
{
    const bool result = m_dirtyState;
    m_dirtyState = false;
    return result;
}

int zQuestServer::QuestToId(
    int questNum, int subQuestNum)
{
    return AddSigned32Wrap(MultiplySigned32Wrap(questNum, 1000), subQuestNum);
}

iRPGQuest* zQuestServer::IdToQuest(int packedKey)
{
    return GetQuest(packedKey / 1000, packedKey % 1000, false, true);
}

void zQuestServer::ResetAllQuests()
{
    iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(14));
    while (quest != 0)
    {
        (void)quest->SetState(0, false);
        quest->SetCurTime(0);
        iRPGObj* const object = quest->AsRPGObj();
        (void)object->SetSkillValue(RPGQUEST_EXTINFOQUEST, 0);
        quest = NextQuest();
    }
}

const char* zQuestServer::GetSubQuestName(iRPGQuest* _pQuest)
{
    if (_pQuest == 0)
        DebugAssert(5, "_pQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 264);
    iRPGObj* const questObject = static_cast<iRPGObj*>(_pQuest->QueryInterface(9));
    const int key = questObject->GetSkillValue(RPGQUEST_EXTINFOQUEST);
    if (key != 0)
    {
        iRPGQuest* const buff = IdToQuest(key);
        if (buff == 0)
            DebugAssert(5, "buff",                 "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 273);
        const char* const text = static_cast<iTextDB*>(this)->GetObjInfo(buff->AsRPGObj(), true);
        if (text != 0)
            return text;
    }
    if (_pQuest->GetSubQNum() == 0)
        return _pQuest->GetMainQuestInfo();
    return static_cast<iTextDB*>(this)->GetObjInfo(questObject, true);
}

int zQuestServer::GetPrevQuestMaxTimeLine(iRPGQuest* _pQuest)
{
    if (_pQuest == 0)
        DebugAssert(5, "_pQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 594);
    int best = -1;
    for (iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST));quest != 0;quest = NextQuest())
    {
        if (quest->GetJoin(static_cast<JOIN_TYPE>(0)) == _pQuest)
        {
            if (quest->GetTL()>best)
                best = quest->GetTL();
        }
    }
    return best;
}

bool zQuestServer::DeleteQuest(iRPGQuest* _pQuest)
{
    if (_pQuest == 0)
        DebugAssert(5, "_pQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 614);
    for (iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST));quest != 0;quest = NextQuest())
    {
        if (quest->GetJoin(static_cast<JOIN_TYPE>(0)) == _pQuest)
            (void)quest->SetJoin(static_cast<JOIN_TYPE>(0), 0, false);
        if (quest->GetJoin(static_cast<JOIN_TYPE>(1)) == _pQuest)
            (void)quest->SetJoin(static_cast<JOIN_TYPE>(1), 0, false);
    }
    iRPGObj* const object = _pQuest->AsRPGObj();
    static_cast<iTextDB*>(this)->SetObjInfo(object, 0);
    (void)zRPGFrameWork::ReleaseObject(object);
    return true;
}

int zQuestServer::GetLocalNum(iRPGQuest* _pSubQuest)
{
    if (_pSubQuest == 0)
        DebugAssert(5, "_pSubQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 636);
    const int questNum = _pSubQuest->GetQuestNum();
    int matchingQuestIndex = 0;
    for (iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST));quest != 0;quest = NextQuest())
    {
        if (quest->GetQuestNum() != questNum)
            continue;
        if (quest == _pSubQuest)
            break;
        ++matchingQuestIndex;
    }
    return matchingQuestIndex;
}

bool zQuestServer::SaveQuestsToFile(
    const char* _sFileName, iRPGFrameWork* _pMainFramework)
{
    static const char* source =         "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp";
    if (_sFileName == 0)
        DebugAssert(5, "_sFileName", source, 490);
    if (_pMainFramework == 0)
        DebugAssert(5, "_pMainFramework", source, 491);
    (void)::MoveFileExA(_sFileName, "levels.cfg.bak",
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    std::FILE* stream = std::fopen(_sFileName, "w");
    if (stream == 0)
    {
        iLogConsole* const logger = ::gpLogCons;
        logger->LogStr(LT_WARN, "Can't write to file %s", _sFileName);
        return 0;
    }
    std::fprintf(stream, "%s=%d\n", "FileVer", 2);
    SaveObjsToFile(stream, _pMainFramework,         static_cast<OBJ_TYPE>(TYPE_LEVEL));
    SaveObjsToFile(stream, _pMainFramework,         static_cast<OBJ_TYPE>(TYPE_QUEST));
    std::fclose(stream);
    return 1;
}

iRPGQuest* zQuestServer::CreateNewSubQuest(int levelNum, int questNum)
{
    iRPGObj* const NewQuest = zRPGFrameWork::GetNewRPGObj(static_cast<OBJ_TYPE>(TYPE_QUEST), true);
    if (NewQuest == 0)
        DebugAssert(5, "NewQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 526);
    iRPGQuest* const result = static_cast<iRPGQuest*>(NewQuest->QueryInterface(10));
    const int subQuestCount = GetSubQuestNum(questNum);
    iRPGQuest* const previous = GetQuest(questNum, subQuestCount - 1, true, true);
    (void)previous->QueryInterface(9);
    const int subQuestNum = previous->GetSubQNum() + 1;
    result->SetQuestNum(questNum);
    result->SetSubQNum(subQuestNum);
    zString name;
    name.Format("%d_%d_%d", levelNum, questNum, subQuestNum);
    NewQuest->SetName(name.m_pBuf, true);
    return result;
}

iRPGQuest* zQuestServer::CreateNewQuest(int levelNum)
{
    iRPGObj* const NewQuest = zRPGFrameWork::GetNewRPGObj(static_cast<OBJ_TYPE>(TYPE_QUEST), true);
    if (NewQuest == 0)
        DebugAssert(5, "NewQuest",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 554);
    iRPGQuest* const quest = static_cast<iRPGQuest*>(NewQuest->QueryInterface(10));
    const int current = GetQuestNum();
    quest->SetQuestNum(current);
    quest->SetSubQNum(0);
    zString name;
    name.Format("%d_%d_%d", levelNum, current, 0);
    NewQuest->SetName(name.m_pBuf, true);
    return static_cast<iRPGQuest*>(NewQuest->QueryInterface(10));
}

iRPGQuest* zQuestServer::CreateNewLevel(int levelIndex)
{
    iRPGObj* const NewLevel = zRPGFrameWork::GetNewRPGObj(static_cast<OBJ_TYPE>(TYPE_LEVEL), true);
    if (NewLevel == 0)
        DebugAssert(5, "NewLevel",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 576);
    iRPGQuest* const level = static_cast<iRPGQuest*>(NewLevel->QueryInterface(10));
    level->SetTL(levelIndex);
    zString name;
    name.Format("Level%.3d", AddSigned32Wrap(levelIndex, 1));
    NewLevel->SetName(name.m_pBuf, true);
    return level;
}

void zQuestServer::AddNeededVid(int vid)
{
    m_neededVids[static_cast<unsigned int>(vid)] = 1u;
}




void* zQuestServer::QueryInterface(int interfaceId) const
{
    if (interfaceId == 24)
        return static_cast<iQuestServer*>(const_cast<zQuestServer*>(this));
    return zRPGFrameWork::QueryInterface(interfaceId);
}


int zQuestServer::iRelease()
{
    if (this)
    {
        this->~zQuestServer();
        CrtFreePointer(this);
    }
    return 0;
}


void zQuestServer::CheckNamedMonsters(int namedMonsterId)
{
    const int levelCount = GetQuestNum();
    for (int level = 1; level <= levelCount; ++level)
    {
        iRPGQuest* const quest = GetQuest(level, 0, false, true);
        quest->CheckStateItemNameOrVid(static_cast<CSI_ACTION>(1), 0, 0, namedMonsterId);
    }
}

void zQuestServer::CheckStateItemName(CSI_ACTION action, const char* itemName)
{
    const int levelCount = GetQuestNum();
    for (int level = 1; level <= levelCount; ++level)
    {
        iRPGQuest* const quest = GetQuest(level, 0, false, true);
        quest->CheckStateItemNameOrVid(action, itemName, 0, 0);
    }
}

void zQuestServer::CheckStateItemVid(CSI_ACTION action, int vid)
{
    if (m_neededVids[static_cast<unsigned int>(vid)] == 0u)
        return;
    const int levelCount = GetQuestNum();
    for (int level = 1; level <= levelCount; ++level)
    {
        iRPGQuest* const quest = GetQuest(level, 0, false, true);
        quest->CheckStateItemNameOrVid(action, 0, vid, 0);
    }
}

bool zQuestServer::AreLinked(iRPGQuest* firstQuest, iRPGQuest* secondQuest, bool reverse)
{
    iRPGQuest* current = reverse ? firstQuest : secondQuest;
    iRPGQuest* const target = reverse ? secondQuest : firstQuest;
    current = current->GetJoin(static_cast<JOIN_TYPE>(0));
    while (current != 0)
    {
        if (current == target)
            return true;
        current = current->GetJoin(static_cast<JOIN_TYPE>(0));
    }
    return false;
}

void zQuestServer::CheckFlagmanCoord(int x, int y)
{
    const int levelCount = GetQuestNum();
    for (int level = 1; level <= levelCount; ++level)
    {
        iRPGQuest* const Quest = GetQuest(level, 0, false, true);
        if (Quest == 0)
            DebugAssert(5u, "Quest", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 751u);
        Quest->CheckFlagmanCoord(x, y);
    }
}

void zQuestServer::SetAllQuestsUnActual()
{
    const int groupCount = GetQuestNum();
    for (int group = 1; group <= groupCount; ++group)
    {
        const int questCount = GetSubQuestNum(group);
        for (int subQuestIndex = 0; subQuestIndex < questCount; ++subQuestIndex)
        {
            iRPGQuest* const buf = GetQuest(group, subQuestIndex, true, true);
            if (buf == 0)
                DebugAssert(4u, "buf", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 968u);
            (void)SetQuestState(buf, 0x20, false);
        }
    }
}

bool zQuestServer::InCurLevel(iRPGQuest* _pQuest)
{
    if (_pQuest == 0)
        DebugAssert(5u, "_pQuest", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 979u);
    const int threshold = _pQuest->GetTL();
    zProfLdr* const profiles = ::g_pProfLdr;
    const int requiredCount = profiles->GetCurLevNum(profiles->m_currentProfile, false);
    int eligibleLevelCount = 0;
    for (iRPGQuest* current = FirstQuest(static_cast<OBJ_TYPE>(TYPE_LEVEL)); current != 0; current = NextQuest())
    {
        const int levelTimeline = current->GetTL();
        if (levelTimeline != 0 && levelTimeline <= threshold)
            ++eligibleLevelCount;
    }
    ++eligibleLevelCount;
    return eligibleLevelCount == requiredCount;
}

void zQuestServer::PushCurState(const char* fileName)
{
    std::FILE* fileHandle = 0;
    if (fileName != 0)
    {
        fileHandle = std::fopen(fileName, "w");
        if (fileHandle != 0)
            (void)std::fprintf(fileHandle, ";%s\n", fileName);
    }
    const int groupCount = GetQuestNum();
    for (int group = 1;group <= groupCount;++group)
    {
        const int questCount = GetSubQuestNum(group);
        for (int subQuestIndex = 0;subQuestIndex<questCount;++subQuestIndex)
        {
            iRPGQuest* const quest = GetQuest(group, subQuestIndex, true, true);
            quest->PushCurState(fileHandle);
        }
    }
    if (fileHandle != 0)
        (void)std::fclose(fileHandle);
    m_savedStatePresent = true;
    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_NOTICE, "ResetQuestSystem: PushCurState()");
}

bool zQuestServer::PopCurState(const char* fileName)
{
    if (!m_savedStatePresent && fileName == 0)
        return false;
    bool result = true;
    if (fileName != 0)
    {
        ResetAllQuests();
        iRPGFrameWork* const frameWork = static_cast<zRPGFrameWork*>(this);
        ::g_pParser->SetGlobalFrameWork(frameWork);
        ::g_pParser->SetReadFrameWork(frameWork);
        result = ::g_pParser->AddIniFile(fileName);
    }
    else
    {
        const int groupCount = GetQuestNum();
        for (int group = 1; group <= groupCount; ++group)
        {
            const int questCount =                 GetSubQuestNum(group);
            for (int subQuestIndex = 0; subQuestIndex < questCount; ++subQuestIndex)
            {
                iRPGQuest* const quest = GetQuest(group, subQuestIndex, true, true);
                quest->PopCurState(0);
            }
        }
    }
    m_dirtyState = true;
    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_NOTICE, "ResetQuestSystem: PopCurState()");
    return result;
}


void zQuestServer::AddBonuses(iRPGQuest* _pQuest)
{
    if (!_pQuest)
        DebugAssert(5u, "_pQuest", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 801u);
    if (!m_pPlayer)
        DebugAssert(5u, "m_pPlayer", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 802u);
    iRPGHero* Hero = m_pPlayer->GetPlayerHero();
    if (!Hero)
        DebugAssert(5u, "Hero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 806u);
    iRPGImplant::sb_FromQuestServ = true;
    const int firstCount = _pQuest->GetNumItems(static_cast<IT_TYPE>(0));
    for (int i = 0;i<firstCount;++i)
    {
        const int entry = _pQuest->GetItem(static_cast<IT_TYPE>(0), i);
        iRPGObj* const family = static_cast<iRPGObj*>(Hero->QueryInterface(9));
        iRPGFrameWork* const route = family->GetFW();
        iRPGObj* const resolved = route->GetItemById(entry);
        const int inserted = Hero->PutItem(resolved);
        if (!inserted)
        {
            iRPGObj* const copy = resolved->GetObjCopy();
            (void)Hero->CreateGroundSprite(copy);
            if (m_pPlayer)
                m_pPlayer->SendInfoMessage(4, 0);
        }
        if (m_pPlayer && inserted)
        {
            const int state = resolved->GetNumInCell();
            m_pPlayer->OnAutoTakeItem(resolved, state, 0, 0, 0);
        }
    }
    iRPGImplant::sb_FromQuestServ = false;
    const int secondCount = _pQuest->GetNumItems(static_cast<IT_TYPE>(1));
    for (int i = 0;i<secondCount;++i)
    {
        const int entry = _pQuest->GetItem(static_cast<IT_TYPE>(1), i);
        (void)Hero->TakeItem(entry);
    }
    iRPGObj* active = _pQuest->AsRPGObj();
    int value = active->GetSkillValue(RPGQUEST_ADDEXPERIENCE);
    if (value)
        Hero->AddExperience(value, true);
    active = _pQuest->AsRPGObj();
    if (active->GetGameObj() != TYPE_LEVEL)
    {
        active = _pQuest->AsRPGObj();
        value = active->GetSkillValue(RPGQUEST_ADDMONEY);
        (void)Hero->AddMoney(value, false);
    }
    active = _pQuest->AsRPGObj();
    value = active->GetSkillValue(RPGQUEST_DIALOGNUM);
    if (value)
        m_pPlayer->SetFlagmanDialog(value);
}

bool zQuestServer::OnBeginNewLevel()
{
    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "OnBeginNewLevel()");
    PushCurState(0);
    m_dirtyState = true;
    for (iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST)); quest != 0; quest = NextQuest())
    {
        if (quest->GetUnactualNext())
        {
            const unsigned int state = static_cast<unsigned int>(quest->GetState());
            if ((state & 1u) != 0u && (state & 0x20u) == 0u)
                (void)quest->AddState(0x20, false);
        }
    }
    for (iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST)); quest != 0; quest = NextQuest())
    {
        if (quest->GetBeginNext())
        {
            const unsigned int state = static_cast<unsigned int>(quest->GetState());
            if ((state & 1u) != 0u && (state & 0x20u) == 0u)
            {
                iRPGQuest* const linked = quest->GetJoin(static_cast<JOIN_TYPE>(0));
                if (linked != 0)
                    (void)linked->AddState(1, false);
            }
        }
    }
    iRPGHero* const Hero = m_pPlayer->GetPlayerHero();
    if (Hero == 0)
        DebugAssert(5u, "Hero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Quest\\zQuestServer.cpp", 688u);
    iRPGObj* const family = static_cast<iRPGObj*>(Hero->QueryInterface(9));
    iRPGFrameWork* const service = family->GetFW();
    zQuestVector<iRPGQuest*> candidates;
    for (iRPGQuest* quest = FirstQuest(static_cast<OBJ_TYPE>(TYPE_QUEST)); quest != 0; quest = NextQuest())
    {
        if (quest->GetStateItemName(static_cast<QUEST_STATE>(1), static_cast<CSI_ACTION>(3)) != 0)
        {
            candidates.insert(candidates.m_end, 1, quest);
        }
    }
    for (unsigned int index = 0; index<candidates.size(); ++index)
    {
        if (!InCurLevel(candidates.m_begin[index]))
            continue;
        const char* const key = candidates.m_begin[index]->GetStateItemName(
            static_cast<QUEST_STATE>(1), static_cast<CSI_ACTION>(3));
        iRPGObj* const candidate = service->GetObjByNamee(key, true);
        if (candidate == 0)
            continue;
        iInvServer* const inventory = Hero->GetInvServer();
        const int subKey = candidate->GetBaseId();
        if (inventory->GetItemByBaseId(subKey, 3) != 0)
            (void)candidates.m_begin[index]->AddState(1, false);
    }
    logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "OnBeginNewLevel() - end");
    return true;
}

bool zQuestServer::SaveObjsToFile(
    std::FILE* stream, iRPGFrameWork* pMainFramework, OBJ_TYPE objectType)
{
    for (iRPGQuest* quest = FirstQuest(objectType);          quest != 0; quest = NextQuest())
    {
        iRPGObj* const rpgObject = static_cast<iRPGObj*>(quest->QueryInterface(9));
        std::fprintf(stream, ";************\n");
        const char* objectName = rpgObject->GetName();
        const int rpgType = rpgObject->GetGameObj();
        std::fprintf(stream, "%s=%s\n", GetTypeName(static_cast<OBJ_TYPE>(rpgType)), objectName);
        const int levelNum = quest->GetLevelNum();
        if (levelNum > 0)
            std::fprintf(stream, "%s=%d\n", "LevelNum", levelNum);
        std::fprintf(stream, "%s=%d\n", "TimeLine", quest->GetTL());
        struct QuestRpgValueLine
        {
            int parameter;
            const char* name;
        }
        ;
        static const QuestRpgValueLine preItemValues[] =
        {
            {
                RPGQUEST_ADDMONEY, "AddMoneyMAIN"
            }
            ,
            {
                RPGQUEST_ADDMONEY2, "AddMoneyALTERNATIVE"
            }
            ,
            {
                RPGQUEST_ADDEXPERIENCE, "AddExperienceMAIN"
            }
            ,
            {
                RPGQUEST_ADDEXPERIENCE2, "AddExperienceALTERNATIVE"
            }
        }
        ;
        for (unsigned int i = 0; i < sizeof(preItemValues) / sizeof(preItemValues[0]); ++i)
        {
            const int value = rpgObject->GetSkillValue(preItemValues[i].parameter);
            if (value > 0)
                std::fprintf(stream, "%s=%d\n", preItemValues[i].name, value);
        }
        for (int itemAction = 0; itemAction < 2; ++itemAction)
        {
            const int itemCount = quest->GetNumItems(static_cast<IT_TYPE>(itemAction));
            if (itemCount <= 0)
                continue;
            std::fprintf(stream, "%s=", itemAction == 0 ? "AddItemMAIN" : "RemItem");
            bool first = true;
            for (int index = 0; index < itemCount; ++index)
            {
                if (!first)
                    std::fprintf(stream, " ");
                first = false;
                const int itemId = quest->GetItem(static_cast<IT_TYPE>(itemAction), index);
                iRPGObj* const item = pMainFramework->GetItemById(itemId);
                if (item != 0)
                    std::fprintf(stream, "%s", item->GetName());
            }
            std::fprintf(stream, "\n");
        }
        const int dialogMain = rpgObject->GetSkillValue(RPGQUEST_DIALOGNUM);
        if (dialogMain > 0)
            std::fprintf(stream, "%s=%d\n", "DialogNumMAIN", dialogMain);
        const int dialogAlternative = rpgObject->GetSkillValue(RPGQUEST_DIALOGNUM2);
        if (dialogAlternative > 0)
            std::fprintf(stream, "%s=%d\n", "DialogNumALTERNATIVE", dialogAlternative);
        const int startingCount = quest->GetNumStartQuests();
        if (startingCount > 0)
        {
            std::fprintf(stream, "%s=", "StartingQuests");
            bool first = true;
            for (int index = 0; index < startingCount; ++index)
            {
                if (!first)
                    std::fprintf(stream, " ");
                first = false;
                std::fprintf(stream, "%d", quest->GetStartQuest(index));
            }
            std::fprintf(stream, "\n");
        }
        iRPGQuest* const nextQuest = quest->GetJoin(static_cast<JOIN_TYPE>(0));
        if (nextQuest != 0)
            std::fprintf(stream, "%s=%d\n", "NextQuests", QuestToId(nextQuest));
        iRPGQuest* const lineQuest = quest->GetJoin(static_cast<JOIN_TYPE>(1));
        if (lineQuest != 0)
            std::fprintf(stream, "%s=%d %d\n", "LineQuests", 1, QuestToId(lineQuest));
        for (int state = 0; state < 8; ++state)
        {
            for (int action = 0; action < 4; ++action)
            {
                const char* itemName = quest->GetStateItemName(static_cast<QUEST_STATE>(state), static_cast<CSI_ACTION>(action));
                if (itemName == 0)
                    continue;
                const char* itemActionName = quest->GetStrByIemAction(static_cast<CSI_ACTION>(action));
                const char* stateName = quest->GetStrByState(static_cast<QUEST_STATE>(state));
                std::fprintf(stream, "ChangeStateItem=%s %s %s\n", stateName, itemActionName, itemName);
            }
        }
        for (int state = 0; state < 8; ++state)
        {
            int x1;
            int y1;
            int x2;
            int y2;
            if (quest->GetStateFlagmanCoord(static_cast<QUEST_STATE>(state), x1, y1, x2, y2))
            {
                std::fprintf(stream, "SetFlagmanCoord=%s %i %i %i %i\n", quest->GetStrByState(static_cast<QUEST_STATE>(state)), x1, y1, x2, y2);
            }
        }
        const int unneeded = rpgObject->GetSkillValue(RPGQUEST_UNNEEDED);
        if (unneeded > 0)
            std::fprintf(stream, "%s=%d\n", "Unneeded", unneeded);
        const int accomplished = rpgObject->GetSkillValue(RPGQUEST_ACCOMPLISHED);
        if (accomplished > 0)
            std::fprintf(stream, "%s=%d\n", "Accomplished", accomplished);
        const int winOnBegin = rpgObject->GetSkillValue(RPGQUEST_WINONBEGIN);
        if (winOnBegin > 0)
            std::fprintf(stream, "%s=%d\n", "WinOnBegin", winOnBegin);
        if (quest->GetBeginNext())
            std::fprintf(stream, "%s=%d\n", "BeginNext", 1);
        if (quest->GetUnactualNext())
            std::fprintf(stream, "%s=%d\n", "UnactualNext", 1);
        const int sndBegun = rpgObject->GetSkillValue(RPGQUEST_SNDBEGUN);
        if (sndBegun > 0)
            std::fprintf(stream, "%s=%d\n", "SndBegun", sndBegun);
        const int sndComplete = rpgObject->GetSkillValue(RPGQUEST_SNDCOMPLETE);
        if (sndComplete > 0)
            std::fprintf(stream, "%s=%d\n", "SndComplete", sndComplete);
        const int infoQuest = rpgObject->GetSkillValue(RPGQUEST_SETINFOQUEST);
        if (infoQuest > 0)
            std::fprintf(stream, "%s=%d\n", "SetInfoQuest", infoQuest);
        const char* comment = rpgObject->GetCharValue(RPG_COMMENT);
        if (comment != 0 && comment[0] != '\0')
            std::fprintf(stream, "%s=%s\n", "Comment", comment);
        const char* comment2 = rpgObject->GetCharValue(RPG_COMENT2);
        if (comment2 != 0 && comment2[0] != '\0')
            std::fprintf(stream, "%s=%s\n", "Coment2", comment2);
    }
    return true;
}

void __cdecl GetNew_zQuestServer(iQuestServer** outObject)
{
    zQuestServer* object = static_cast<zQuestServer*>(CrtNew(sizeof(zQuestServer)));
    if (object)
    {
        try
        {
            new (object) zQuestServer();
        }
        catch (...)
        {
            CrtFreePointer(object);
            throw;
        }
    }
    *outObject = static_cast<iQuestServer*>(object);
}


