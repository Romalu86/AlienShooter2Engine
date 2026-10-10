#include <cstdio>
#include "../../../engine_globals.h"
#include "net.h"
#include "zm/Utils/zLogConsole.h"
#include "zm/Eng/zProfLdr/zProfMngr.h"
#include "constant.h"
#include "irpg.h"
#include "zm/Eng/zProfLdr/zProfLdr.h"
#include "zm/Quest/zQuestServer.h"
#include "basetype.h"
#include "dialogSrv.h"
#include "registry.h"
#include "myerror.h"
#include "zm/Game/RecordDB/RecordDBStreambufControl.h"
#include "mylib.h"
#include "zm/IniParser/zIniParser.h"
#include <new>


using LOG_DLG = zProfMngr::LOG_DLG;

zProfMngr::LOG_DLG_VECTOR::LOG_DLG_VECTOR()
    : begin(0), end(0), capacityEnd(0)
{
}

zProfMngr::LOG_DLG_VECTOR::~LOG_DLG_VECTOR() noexcept(false)
{
    CrtFreePointer(begin);
    begin = 0;
    end = 0;
    capacityEnd = 0;
}

int zProfMngr::LOG_DLG_VECTOR::size() const
{
    return begin ? static_cast<int>(end - begin) : 0;
}

void zProfMngr::LOG_DLG_VECTOR::push_back(const LOG_DLG& value)
{
    LOG_DLG* const position = end;
    const unsigned int available = static_cast<unsigned int>(capacityEnd - end);
    if (available < 1u)
    {
        const unsigned int oldSize = static_cast<unsigned int>(size());
        const unsigned int capacity = oldSize + (oldSize > 1u ? oldSize : 1u);
        const unsigned int allocationCount = BitsToSigned32(capacity) < 0 ? 0u : capacity;
        LOG_DLG* const storage = static_cast<LOG_DLG*>(CrtNew(allocationCount * sizeof(LOG_DLG)));
        LOG_DLG* output = storage;
        for (LOG_DLG* source = begin; source != position; ++source, ++output)
            if (output) ::new(output) LOG_DLG(*source);
        if (output) ::new(output) LOG_DLG(value);
        ++output;
        for (LOG_DLG* source = position, *last = end; source != last; ++source, ++output)
            if (output) ::new(output) LOG_DLG(*source);
        CrtFreePointer(begin);
        capacityEnd = storage + capacity;
        const unsigned int count = static_cast<unsigned int>(size());
        begin = storage;
        end = storage + count + 1u;
    }
    else
    {
        if (position) ::new(position) LOG_DLG(value);
        ++end;
    }
}


zProfMngr::zProfMngr()
    : m_pRPGFrameWork(0), m_loadedFromMax(false)
{
}

void* zProfMngr::QueryInterface(int interfaceId) const
{
    if (interfaceId == 55)
        return const_cast<zProfMngr*>(this);
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 24u);
    return 0;
}

int zProfMngr::iRelease()
{
    if (this)
    {
        this->~zProfMngr();
        CrtFreePointer(this);
    }
    return 0;
}

zProfMngr::~zProfMngr()
{
    if (m_cloneRef.m_object != 0)
    {
        iRPGObj* cloneObject = static_cast<iRPGObj*>(m_cloneRef.m_object->QueryInterface(9));
        (void)m_pRPGFrameWork->ReleaseObject(cloneObject);
    }


}


bool zProfMngr::CopyObjToHero(iRPGObj* source, iRPGObj* _pHero)
{
    if (!_pHero)DebugAssert(4u, "_pHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 246u);
    if (!source)return false;
    if ((source->GetClone()&2) != 0)
    {
        _pHero->CopyParametersFrom(source);
        iLogConsole* const logger =
            ::gpLogCons;
        logger->LogStr(LT_INFO, "zmclone: CopyCloneToHero");
    }
    return true;
}

const DIALOG* zProfMngr::GetDialog(int logIndex)
{
    const int dialogId = m_logs.begin[logIndex].dialog;
    DialogSrv* dialogs = ::g_pDlgSrv;
    DIALOG* ret = dialogs->GetDialog(dialogId);
    bool invalidDialog = ret == 0;
    if (ret)
    {
        if (ret->m_phrases.m_count == 0)
        {
            const unsigned int portraitSide = ret->m_radio ? 1u : 0u;
            if (ret->m_unitVid[portraitSide] == 0)
                invalidDialog = true;
        }
        if (ret->m_radio == 0)
            invalidDialog = true;
    }
    if (invalidDialog)
        DebugAssert(4u, "ret && ret->IsValid() && ret->GetType() == DT_RADIO", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 388u);
    return ret;
}

const char* zProfMngr::GetLogText(int logIndex)
{
    const DIALOG* dlg = GetDialog(logIndex);
    if (!dlg)
        return 0;
    const int num = m_logs.begin[logIndex].phrase;
    if (num<0 || num >= dlg->m_phrases.m_count)
        DebugAssert(5u, "num >= 0 && num < dlg->NoPhrase()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 377u);
    return dlg->GetPhrase(num).m_value;
}

void zProfMngr::CopyHeroToArmy(iRPGObj* _pBaseHero, int heroColor)
{
    if (!_pBaseHero)DebugAssert(4u, "_pBaseHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 263u);
    iRPGObj* armyHero = m_armyHeroRef.m_object;
    if (!armyHero)
    {
        iRPGObj* const newArmyHero = m_pRPGFrameWork->GetNewRPGObj(static_cast<OBJ_TYPE>(TYPE_HERO), false);
        m_armyHeroRef.Set(newArmyHero);
        armyHero = m_armyHeroRef.m_object;
        armyHero->SetClone(1);
        armyHero = m_armyHeroRef.m_object;
        const char* name = armyHero?armyHero->GetName():0;
        if (!name)name = StringGlobals::EmptyText();
        iLogConsole* const logger =
            ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "m_pArmyHero - SetClone(CT_CLONE). %s", name);
    }
    armyHero = m_armyHeroRef.m_object;
    if (armyHero != _pBaseHero)
    {
        armyHero->CopyParametersFrom(_pBaseHero);
        armyHero = m_armyHeroRef.m_object;
        const char* name = armyHero?armyHero->GetName():0;
        if (!name)name = StringGlobals::EmptyText();
        iLogConsole* const logger =
            ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "m_pArmyHero - CopyParametersFrom(_pBaseHero). %s", name);
    }
    if (heroColor != -1 && m_armyHeroRef.m_object)
        (void)m_armyHeroRef.m_object->SetSkillValue(RPGHERO_COLOR, heroColor);
}

int zProfMngr::GetRealLogNum(int logIndex)
{
    return m_logs.begin[logIndex].dialog;
}

unsigned int __cdecl zProfMngr_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned int __cdecl zProfMngr_StaticCleanupRegister02()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned int __cdecl zProfMngr_StaticCleanupRegister03()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned int __cdecl zProfMngr_StaticCleanupRegister04()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


int zProfMngr::GetLogsNum()
{
    return m_logs.size();
}





int zProfMngr::GetLogFaceDir(int logIndex)
{
    const DIALOG* dialog = GetDialog(logIndex);
    if (!dialog || m_logs.begin[logIndex].phrase != 0)
        return -1;
    return BitsToSigned32(dialog->m_faceDirection);
}








void zProfMngr::ClearLogs()
{
    m_logs.end = m_logs.begin;
}

void zProfMngr::ReleaseArmyHero()
{
    iRPGObj* armyHero = m_armyHeroRef.m_object;
    const char* heroName = StringGlobals::EmptyText();
    if (armyHero)
    {
        const char* const name = armyHero->GetName();
        if (name) heroName = name;
    }
    iLogConsole* logger =
        ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "m_pArmyHero - ReleaseArmyHero(). %s", heroName);
    if (m_armyHeroRef.m_object)
        m_armyHeroRef.m_refObject->RemObjectRef(reinterpret_cast<iRef<iRefObject>*>(&m_armyHeroRef));
    m_armyHeroRef.m_object = 0;
    m_armyHeroRef.m_refObject = 0;
}

void zProfMngr::ReCreateClone()
{
    iRPGHero* const previousHero = m_cloneRef.m_object;
    if (previousHero)
    {
        iRPGObj* const previousObject = static_cast<iRPGObj*>(previousHero->QueryInterface(9));
        (void)m_pRPGFrameWork->ReleaseObject(previousObject);
    }

    iRPGObj* const newObject = m_pRPGFrameWork->GetNewRPGObj(static_cast<OBJ_TYPE>(TYPE_HERO), false);
    newObject->SetName("Clone", false);
    newObject->SetClone(1);
    iRPGHero* const newHero = static_cast<iRPGHero*>(newObject->QueryInterface(0));
    m_cloneRef.Set(newHero);
}

iRPGObj* zProfMngr::GetArmyHero(bool survival, bool keepMissing, bool loadFromMax)
{
    if (loadFromMax && (!m_loadedFromMax || !m_armyHeroRef.m_object))
    {
        iRPGObj* const loadedHero = LoadHeroFromSave(static_cast<SAVE_TYPE>(survival?1:0), loadFromMax);
        m_armyHeroRef.Set(loadedHero);
        const char* name = m_armyHeroRef.m_object ? m_armyHeroRef.m_object->GetName() : 0;
        if (!name)name = StringGlobals::EmptyText();
        iLogConsole* const logger = ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "m_pArmyHero - _bLoadFromMax. %s", name);
    }
    m_loadedFromMax = loadFromMax;
    if (!m_armyHeroRef.m_object && !keepMissing)
    {
        iRPGObj* const loadedHero = LoadHeroFromSave(static_cast<SAVE_TYPE>(survival?1:0), false);
        m_armyHeroRef.Set(loadedHero);
        const char* name = m_armyHeroRef.m_object ? m_armyHeroRef.m_object->GetName() : 0;
        if (!name)name = StringGlobals::EmptyText();
        iLogConsole* const logger = ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "m_pArmyHero - LoadHeroFromSave. %s", name);
    }
    if (!m_armyHeroRef.m_object)
    {
        iRPGObj* const loadedHero = m_pRPGFrameWork->FirstObject(0x21, 0, 0);
        CopyHeroToArmy(loadedHero, -1);
        const char* name = m_armyHeroRef.m_object ? m_armyHeroRef.m_object->GetName() : 0;
        if (!name)name = StringGlobals::EmptyText();
        iLogConsole* const logger = ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "m_pArmyHero - FirstObject. %s", name);
    }
    return m_armyHeroRef.m_object;
}

void zProfMngr::SaveLogs()
{
    zProfLdr* loader = ::g_pProfLdr;
    STRING fileName = loader->GetCurSaveName2(static_cast<SAVE_TYPE>(4), false);

    if (fileName.m_value[0] == 0)
        return;

    RecordDBOfstreamControl output(fileName.m_value, 0x12);
    if ((output.State()&6u) == 0u)
    {
        for (int logIndex = 0; logIndex < GetLogsNum(); ++logIndex)
        {
            const LOG_DLG* const logRecord = m_logs.begin + logIndex;
            const int phrase = logRecord->phrase;
            const int dialog = logRecord->dialog;
            void* written = U32ToPtr(Msvc6BasicOstreamInsertInt(&output, dialog));
            written = RecordDBOstreamWriteChar(written, static_cast<unsigned int>(','));
            written = U32ToPtr(Msvc6BasicOstreamInsertInt(written, phrase));
            (void)N_6std(written, "\n");
        }
        output.Close();
    }

}

void zProfMngr::LoadLogs()
{
    (void)ClearLogs();

    zProfLdr* loader = ::g_pProfLdr;
    STRING fileName = loader->GetCurSaveName2(static_cast<SAVE_TYPE>(4), false);

    if (fileName.m_value[0] == 0)
        return;

    RecordDBIfstreamControl input(fileName.m_value, 1);
    if ((input.State()&6u) == 0u)
    {
        RecordDBLineString line;
        for (;;)
        {
            input.PrepareLineRead();
            RecordDBIfstreamControl* const returned = reinterpret_cast<RecordDBIfstreamControl*>(
                U32ToPtr(RecordDBGetlineDelimited(&input, &line, static_cast<unsigned int>('\n'))));
            if ((returned->State()&6u) != 0u)
                break;
            int dialogId;
            int phraseIndex;
            const char* const text = line.data ? line.data :
                "";
            if (sscanf(text, "%i,%i", &dialogId, &phraseIndex) == 2)
                (void)AddLog(dialogId, phraseIndex, false);
        }
        input.Close();
    }

}

bool zProfMngr::AddLog(int dialogId, int phraseIndex, bool flags)
{
    if (phraseIndex != 0)return true;

    for (int logIndex = 0;logIndex<GetLogsNum();++logIndex)
    {
        const LOG_DLG* records = m_logs.begin;
        if (records[logIndex].dialog == dialogId)return false;
    }

    DialogSrv* dialogs = ::g_pDlgSrv;
    DIALOG* dialog = dialogs->GetDialog(dialogId);
    if (!dialog)return true;

    for (int currentPhrase = dialog->m_phrases.m_count-1;currentPhrase >= 0;--currentPhrase)
    {
        LOG_DLG logRecord;
        logRecord.dialog = dialogId;
        logRecord.phrase = currentPhrase;
        logRecord.flags = flags;
        m_logs.push_back(logRecord);
    }
    return true;
}

void zProfMngr::OnLevelBegin(iRPGObj* _pHero, iQuestServer* questServer, bool loadFromMax)
{
    const NET* const network =
        ::Net;
    if ((network->m_flags&0x08u) != 0u)
        return;

    if (_pHero == 0)
        DebugAssert(4u, "_pHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 107u);

    const bool survival = loadFromMax;
    iRPGObj* savedHero = LoadHeroFromSave(static_cast<SAVE_TYPE>(survival?1:0), false);
    if (savedHero != 0)
    {
        (void)CopyObjToHero(savedHero, _pHero);
    }
    else
    {
        iRPGHero* cloneHero = m_cloneRef.m_object;
        iRPGObj* cloneObject = cloneHero?static_cast<iRPGObj*>(cloneHero->QueryInterface(9)):0;
        const unsigned char copiedClone = CopyObjToHero(cloneObject, _pHero);
        iLogConsole* const logger =
            ::gpLogCons;
        const char* const text = copiedClone
            ? "zmclone: zmload-load from clone"
            : "zmclone: LoadHeroOnLevelInit - hero was not cloned!";
        (void)logger->LogStr(LT_INFO, text);
    }

    ReCreateClone();
    m_cloneRef.m_object->CopyParametersFrom(_pHero);
    m_cloneRef.m_object->SelectBestWeapon();

    if (questServer != 0)
    {
        if (!survival)
        {
            zProfLdr* loader = ::g_pProfLdr;
            STRING questPath = loader->GetCurSaveName2(static_cast<SAVE_TYPE>(2), false);
            if (questPath.m_value[0] != 0)
            {
                const char* const savedQuestPath = questPath.m_value;
                const bool restored = questServer->PopCurState(questPath.m_value);
                iLogConsole* const logger = ::gpLogCons;
                (void)logger->LogStr(LT_NOTICE, "zmquest: try load from %s", savedQuestPath);
                if (!restored)
                {
                    const bool emptyRestored = questServer->PopCurState(0);
                    if (!emptyRestored)
                        questServer->ResetAllQuests();
                }
            }
            else
            {
                const bool emptyRestored = questServer->PopCurState(0);
                if (!emptyRestored)
                    questServer->ResetAllQuests();
            }
            questServer->PushCurState(0);
        }
    }
    if (!survival)
        LoadLogs();

    int listMismatch;
    {
        zProfLdr* loader = ::g_pProfLdr;
        STRING crcListPath = loader->GetCurSaveName2(static_cast<SAVE_TYPE>(5+(survival?1:0)), survival);
        CRC32 crc32State;
        crc32State.value = 0u;
        listMismatch = crc32State.CheckListFromFile(crcListPath);
    }
    if (listMismatch != 0)
        (void)_pHero->IncSkillValue(RPGHERO_Cheater2, 1);
}

iRPGObj* zProfMngr::LoadHeroFromSave(SAVE_TYPE _eSType, bool loadFromMax)
{
    const unsigned int rawSaveType = static_cast<unsigned int>(_eSType);
    if (rawSaveType>1u)
        DebugAssert(4u, "_eSType == ST_NORMAL || _eSType == ST_SURVIVE", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 58u);

    ::g_pParser->SetGlobalFrameWork(m_pRPGFrameWork);
    ::g_pParser->SetReadFrameWork(m_pRPGFrameWork);

    iRPGObj* const loadTarget = m_pRPGFrameWork->GetNewRPGObj(static_cast<OBJ_TYPE>(TYPE_HERO), false);
    loadTarget->SetClone(1);

    STRING fileName("");
    unsigned char loaded = 0u;

    if (loadFromMax != 0u)
    {
        fileName = ::g_pProfLdr->GetMaxLevSaveName(_eSType);
        loaded = ::g_pParser->
            LoadObjFromFile(loadTarget, fileName.m_value);
    }

    if (!loaded)
    {
        fileName = ::g_pProfLdr->GetCurSaveName2(_eSType, rawSaveType == 1u);
        loaded = ::g_pParser->
            LoadObjFromFile(loadTarget, fileName.m_value);
    }

    if (!loaded)
    {
        CONSTANT* application = ::Const;
        const bool canLoadLastSave = application->DebugMode != 0 &&
            g_iZmDebug == 0u &&
            (::Net->m_flags&0x08u) == 0u;
        if (canLoadLastSave)
        {
            if (rawSaveType>1u)DebugAssert(4u, "_eSType == ST_NORMAL || _eSType == ST_SURVIVE", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 50u);
            STRING lastSaveKey(rawSaveType ? "LastSaveNameSurv" : "LastSaveName");
            STRING defaultFileName("");
            REGISTRY* registry = ::Registry;
            fileName = registry->GetString(lastSaveKey, defaultFileName);
            loaded = ::g_pParser->
                LoadObjFromFile(loadTarget, fileName.m_value);
            if (loaded)
            {
                iLogConsole* const logger = ::gpLogCons;
                (void)logger->LogStr(LT_BLINK, "LOADED FROM LAST SAVE  %s", fileName.m_value);
            }
        }
    }

    if (loaded)
    {
        iLogConsole* const logger = ::gpLogCons;
        (void)logger->LogStr(LT_NOTICE, "zmclone: zmload-load from file %s", fileName.m_value);
    }

    return loaded?loadTarget:0;
}

void zProfMngr::OnLevelWin(iRPGObj* _pHero, iQuestServer* questServer, bool surviveMode)
{
    const NET* const network =
        ::Net;
    if ((network->m_flags&0x08u) != 0u)
        return;

    if (!_pHero)
        DebugAssert(5u, "_pHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfMngr.cpp", 169u);

    STRING saveFiles[2];
    STRING& savePath = saveFiles[0];
    STRING& questPath = saveFiles[1];
    const unsigned int survival = surviveMode?1u:0u;
    zProfLdr* profileLoader = ::g_pProfLdr;
    profileLoader->SetMaxLevNum(survival != 0u);

    savePath = ::g_pProfLdr->GetCurSaveName2(static_cast<SAVE_TYPE>(survival), survival != 0u);

    std::FILE* const saveFile = std::fopen(savePath.m_value, "w");
    if (saveFile)
    {
        (void)std::fprintf(saveFile, ";%s\n", savePath.m_value);
        (void)_pHero->SaveToFile(saveFile);
        (void)std::fclose(saveFile);
    }

    ReCreateClone();
    iRPGHero* const cloneHero = m_cloneRef.m_object;
    cloneHero->CopyParametersFrom(_pHero);
    iLogConsole* logger =
        ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "zmclone: SaveHeroOnLevelWin");

    {
        STRING lastSaveKey(survival ? "LastSaveNameSurv" : "LastSaveName");
        REGISTRY* const registry =
            ::Registry;
        registry->SetString(lastSaveKey, savePath);
    }

    if (questServer && !survival)
    {
        questPath = ::g_pProfLdr->GetCurSaveName2(static_cast<SAVE_TYPE>(2), false);
        if (questPath.m_value[0] != 0)
        {
            logger = ::gpLogCons;
            (void)logger->LogStr(LT_NOTICE, "zmquest: try save to %s", questPath.m_value);
            questServer->PushCurState(questPath.m_value);
        }

        questServer->PushCurState(0);
    }

    STRING crcListPath = ::g_pProfLdr->GetCurSaveName2(static_cast<SAVE_TYPE>(5+static_cast<int>(survival)), survival != 0u);
    RecordDBOfstreamControl output(crcListPath.m_value, 0x12);
    if ((output.State()&6u) == 0u)
    {
        CRC32 fileChecksum;
        fileChecksum.value = 0u;
        for (unsigned int fileIndex = 0;fileIndex<2u;++fileIndex)
        {
            const STRING& saveFileName = saveFiles[fileIndex];
            if (saveFileName.m_value[0] == 0)
                continue;
            void* const written = N_6std(&output, saveFileName.m_value);
            (void)N_6std(written, "\n");
            fileChecksum.AddFile(saveFileName);
        }
        void* const written = U32ToPtr(Msvc6BasicOstreamInsertUnsignedInt(&output, fileChecksum.value));
        (void)N_6std(written, "\n");
        output.Close();
    }

    if (!survival)
        SaveLogs();

}


