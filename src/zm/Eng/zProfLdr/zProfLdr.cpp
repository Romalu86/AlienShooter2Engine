#include "zm/Utils/zLogConsole.h"
#include "../../../engine_globals.h"
#include "mylib.h"
#include "zProfLdr.h"
#include "runtime/locale_state.h"
#include "myerror.h"
#include "registry.h"
#include "map.h"
#include "basetype.h"
#include <new>
#include "zm/Game/RecordDB/RecordDBStreambufControl.h"
#include <typeinfo>
#include <cstring>
#include <direct.h>

unsigned int  RecordDBGetOrAssignGlobalSequenceId(Msvc6::LocaleId* self);
void*  RecordDBCtypeCharCtor(void* self,unsigned int table32,unsigned int deleteTable,unsigned int refs);
struct MsvcLocaleFacetRef;
MsvcLocaleFacetRef* RecordDBSaveCtypeFacet(MsvcLocaleFacetRef* value);

void* zProfLdr::QueryInterface(int interfaceId) const
{
    if (interfaceId == 54)
        return const_cast<zProfLdr*>(this);
    DebugAssert(5, "0",
        "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 23);
    return 0;
}

zProfLdr::zProfLdr()
{
    m_bLoaded = 0;
    m_currentProfile = -1;
    ClearObjPrf();
}

int zProfLdr::iRelease()
{
    if (this != nullptr)
    {
        this->~zProfLdr();
        zProfLdr::operator delete(this);
    }
    return 0;
}

void zProfLdr::operator delete(void* memory)
{
    CrtFreePointer(memory);
}

bool zProfLdr::GetLoadState()
{
    return m_bLoaded != 0;
}

void zProfLdr::ClearObjPrf()
{
    if (m_bLoaded != 0)
        DebugAssert(5, "!m_bLoaded",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 47);
    for (int profileIndex = 0; profileIndex < MAX_PROF_NUM; ++profileIndex)
    {
        m_aProfs[profileIndex] = StringGlobals::EmptyText();
        m_profileChanged[profileIndex] = 0;
    }
}


bool zProfLdr::DeleteProfile2(int _iProfileNum)
{
    if (_iProfileNum<0 || _iProfileNum >= MAX_PROF_NUM)DebugAssert(5u, "_iProfileNum >= 0 && _iProfileNum < MAX_PROF_NUM", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 267u);
    const char* name = m_aProfs[_iProfileNum].m_value;
    if (*name == 0)return false;
    return DeleteProfileEx(_iProfileNum);
}

void zProfLdr::SetCurProf(int _iCurProf)
{
    if ((_iCurProf<0 || _iCurProf >= MAX_PROF_NUM) && _iCurProf != -1)DebugAssert(5u, "_iCurProf >= 0 && _iCurProf < MAX_PROF_NUM || _iCurProf == -1", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 446u);
    if (m_currentProfile != -1)(void)ExpRegToFile(true);
    m_currentProfile = _iCurProf;
}

STRING zProfLdr::GetProfFolderName(int _iProfileNum)
{
    if (_iProfileNum<0 || _iProfileNum >= MAX_PROF_NUM)
        DebugAssert(5u, "_iProfileNum >= 0 && _iProfileNum < MAX_PROF_NUM", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 458u);
    return Printf("Profile%d", _iProfileNum);
}

bool zProfLdr::ExpRegToFile(bool saveProfileOptions)
{
    if (!m_bLoaded)DebugAssert(5u, "m_bLoaded", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 240u);
    const unsigned char result = static_cast<unsigned char>(ExpRegToObj());
    (void)ExpObjToFile();
    if (saveProfileOptions && m_currentProfile != -1)(void)SaveOptions(m_currentProfile);
    return result;
}

zProfLdr::~zProfLdr() noexcept(false)
{
    (void)ExpRegToFile(true);
}


void zProfLdr::CreateFolderIfNeed(int _iProfileNum)
{
    if (_iProfileNum<0 || _iProfileNum >= MAX_PROF_NUM)
        DebugAssert(5u, "_iProfileNum >= 0 && _iProfileNum < MAX_PROF_NUM", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 477u);

    STRING path;
    {
        STRING profilePath;
        {
            STRING savePath = Map->WindowsUserPath();
            profilePath = savePath;
        }
        profilePath += "Saves\\";
        profilePath += GetProfFolderName(_iProfileNum);
        path = profilePath;
    }
    if (!zutl::FileExists(path.m_value))
    {
        const int ret = _mkdir(path.m_value);
        if (ret != 0)
            DebugAssert(5u, "ret == 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 487u);
    }
}

STRING zProfLdr::GetPlayerNameFName(int profile)
{
    STRING path;
    {
        STRING profilePath;
        {
            STRING savePath = Map->WindowsUserPath();
            profilePath = savePath;
        }
        profilePath += "Saves\\";
        profilePath += GetProfFolderName(profile);
        path = profilePath;
    }
    return path+"\\"+"_profilename.txt";
}

bool zProfLdr::SaveOptions(int profile)
{
    STRING fileName = GetOptFName(profile, true);
    STRING folderName = GetProfFolderName(profile);
    iLogConsole* const logger =
        ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "zm_options: SaveOptions %s %s", folderName.m_value, fileName.m_value);
    ::Registry->SaveToFile(folderName, fileName);
    (void)::gpLogCons->LogStr(LT_INFO, "zm_options: end save");
    return true;
}

bool zProfLdr::LoadOptions(int profile)
{
    STRING fileName = GetOptFName(profile, true);
    STRING folderName = GetProfFolderName(profile);
    iLogConsole* const logger =
        ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "zm_options: LoadOptions %s %s", folderName.m_value, fileName.m_value);
    ::Registry->RestoreFromFile(folderName, fileName);
    return true;
}

STRING zProfLdr::GetOptFName(int profile, bool createFolder)
{
    if (createFolder)
        CreateFolderIfNeed(profile);
    STRING optionsFileName;
    {
        STRING profilePath;
        {
            STRING savePath = Map->WindowsUserPath();
            profilePath = savePath;
        }
        profilePath += "Saves\\";
        profilePath += GetProfFolderName(profile);
        optionsFileName = profilePath;
    }
    optionsFileName += "\\ProfileOptions";
    return STRING(optionsFileName);
}

bool zProfLdr::ExpFileToReg(bool loadProfileOptions)
{
    if (m_bLoaded)return true;
    for (unsigned int profileIndex = 0u;profileIndex < MAX_PROF_NUM;++profileIndex)
    {
        STRING profileFolder = GetProfFolderName(static_cast<int>(profileIndex));
        ::Registry->DeleteKey(profileFolder);
    }
    const unsigned char result = static_cast<unsigned char>(ExpFileToObj());
    (void)ExpObjToReg();
    m_bLoaded = 1u;
    if (loadProfileOptions)for (unsigned int profileIndex = 0u;profileIndex < MAX_PROF_NUM;++profileIndex)if (m_aProfs[profileIndex].m_value[0] != 0)(void)LoadOptions(static_cast<int>(profileIndex));
    return result;
}

bool zProfLdr::ExpObjToReg()
{
    REGISTRY profileRegistry(::Registry->path+
        "\\Profiles");
    for (int profileIndex = 0;profileIndex < MAX_PROF_NUM;++profileIndex)
    {
        STRING valueName = GetProfFolderName(profileIndex);
        profileRegistry.SetString(valueName, m_aProfs[profileIndex]);
    }
    return true;
}

STRING zProfLdr::GetSaveName2(int profile, SAVE_TYPE _eSType, int _iLevNum)
{
    const int rawType = static_cast<int>(_eSType);
    if (_iLevNum<0 && rawType != 3)
        (void)DebugAssert(4u, "_iLevNum >= 0 || _eSType == ST_INPUT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 360u);
    CreateFolderIfNeed(profile);
    STRING path;
    {
        STRING profilePath;
        {
            STRING savePath = Map->WindowsUserPath();
            profilePath = savePath;
        }
        profilePath += "Saves\\";
        profilePath += GetProfFolderName(profile);
        path = profilePath;
    }
    path += "\\";
    if (rawType != 3)
        path += Printf("%.2d", _iLevNum);
    const char* suffix = 0;
    switch (rawType)
    {
        case 1:
            suffix = "_surv";
        break;
        case 2:
            suffix = "_quest";
        break;
        case 4:
            suffix = "_log";
        break;
        case 3:
            suffix = "input";
        break;
        case 5:
            suffix = "_hashcmpgn";
        break;
        case 6:
            suffix = "_hashsurv";
        break;
        default:
            break;
    }
    if (suffix) path += suffix;
    path = path+".cfg";
    return STRING(path);
}

int zProfLdr::GetCurLevNum(int profile, bool survival)
{
    if (m_currentProfile<0)return 0;
    const STRING* profileFolder;
    REGISTRY profileRegistry((profileFolder = &static_cast<const STRING&>(GetProfFolderName(profile)),
        (::Registry->path+"\\")+*profileFolder));
    STRING key(survival ? "giCurLevSurv" : "giCurLevCmpgn");
    return profileRegistry.GetInt(key, -1);
}

STRING zProfLdr::GetCurSaveName2(SAVE_TYPE saveType, bool survival)
{
    if (m_currentProfile<0)
        return STRING(StringGlobals::EmptyText());
    const int level = GetCurLevNum(m_currentProfile, survival);
    STRING path(StringGlobals::EmptyText());
    if (level >= 0 || static_cast<int>(saveType) == 3)
        path = GetSaveName2(m_currentProfile, saveType, level);
    return STRING(path);
}

STRING zProfLdr::GetMaxLevSaveName(SAVE_TYPE _eSType)
{
    if (m_currentProfile<0)
        return STRING(StringGlobals::EmptyText());
    const int rawType = static_cast<int>(_eSType);
    if (rawType != 0 && rawType != 1)
        (void)DebugAssert(5u, "_eSType == ST_NORMAL || _eSType == ST_SURVIVE", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 343u);
    const int level = GetMaxLevNum(m_currentProfile, rawType == 1);
    STRING path(StringGlobals::EmptyText());
    if (level >= 0)
        path = GetSaveName2(m_currentProfile, _eSType, level);
    return STRING(path);
}

int zProfLdr::GetMaxLevNum(int _iProfNum, bool survival)
{
    if (_iProfNum == -1)return 0;
    if (_iProfNum<0 || _iProfNum >= MAX_PROF_NUM)
        (void)DebugAssert(5u, "_iProfNum >= 0 && _iProfNum < MAX_PROF_NUM", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 436u);
    const STRING* profileFolder;
    REGISTRY profileRegistry((profileFolder = &static_cast<const STRING&>(GetProfFolderName(_iProfNum)),
        (::Registry->path+"\\")+*profileFolder));
    STRING key(survival ? "giMaxLevSurv" : "giMaxLevCmpgn");
    return profileRegistry.GetInt(key, 0);
}

bool zProfLdr::ExpRegToObj()
{
    REGISTRY profileRegistry(::Registry->path+
        "\\Profiles");

    for (unsigned int profileIndex = 0u;profileIndex < MAX_PROF_NUM;++profileIndex)
    {
        const STRING* fallback;
        STRING profileName = (fallback = &static_cast<const STRING&>(STRING(
            StringGlobals::EmptyText())),
            profileRegistry.GetString(GetProfFolderName(static_cast<int>(profileIndex)), *fallback));

        if (std::strcmp(profileName.m_value, m_aProfs[profileIndex].m_value) != 0)
        {
            m_aProfs[profileIndex] = profileName;
            m_profileChanged[profileIndex] = 1u;
        }
    }

    return true;
}

bool zProfLdr::ExpFileToObj()
{
    ClearObjPrf();

    for (int profileIndex = 0;profileIndex < MAX_PROF_NUM;++profileIndex)
    {
        STRING fileName = GetPlayerNameFName(profileIndex);
        RecordDBIfstreamControl input(fileName.m_value, 1);
        if ((input.State()&6u) == 0u)
        {
            RecordDBLineString profileName;
            input.PrepareLineRead();
            (void)RecordDBGetlineDelimited(&input, &profileName, static_cast<unsigned int>('\n'));
            m_aProfs[profileIndex] = profileName.data ? profileName.data : "";
            input.Close();
        }
    }
    return true;
}

bool zProfLdr::ExpObjToFile()
{
    for (int profileIndex = 0;profileIndex < MAX_PROF_NUM;++profileIndex)
    {
        if (m_profileChanged[profileIndex] == 0u)
            continue;

        STRING fileName = GetPlayerNameFName(profileIndex);
        if (m_aProfs[profileIndex].Length() > 0)
        {
            CreateFolderIfNeed(profileIndex);
            RecordDBOfstreamControl output(fileName.m_value, 0x12);
            if ((output.State()&6u) == 0u)
            {
                void* const written = N_6std(&output, m_aProfs[profileIndex].m_value);
                (void)N_6std(written, "\n");
                output.Close();
            }
        }
        else
        {
            zutl::MoveToTrash(fileName.m_value);
        }

    }
    return true;
}

void zProfLdr::SetMaxLevNum(bool survival)
{
    const int profileIndex = m_currentProfile;
    if (profileIndex == -1)return;

    const STRING* profileFolder;
    REGISTRY profileRegistry((profileFolder = &static_cast<const STRING&>(GetProfFolderName(profileIndex)),
        (::Registry->path+"\\")+*profileFolder));
    int currentLevel;
    {
        STRING currentLevelKey(survival ? "giCurLevSurv" : "giCurLevCmpgn");
        currentLevel = profileRegistry.GetInt(currentLevelKey, -1);
    }
    int maximumLevel;
    {
        STRING maxLevelKey(survival ? "giMaxLevSurv" : "giMaxLevCmpgn");
        maximumLevel = profileRegistry.GetInt(maxLevelKey, 0);
    }

    bool updateMaximum = false;
    if (survival)
    {
        updateMaximum = currentLevel >= 0 && currentLevel <= 4 && (maximumLevel<currentLevel || currentLevel == 0);
    }
    else
    {
        updateMaximum = (maximumLevel<currentLevel || currentLevel == 1);
    }

    if (updateMaximum)
    {
        STRING maxLevelKey(survival ? "giMaxLevSurv" : "giMaxLevCmpgn");
        STRING levelText = Int2Str(currentLevel);
        profileRegistry.SetString(maxLevelKey, levelText);
    }

}

void* __cdecl ProfileUseCtypeFacet(void* localeOwner, void*          , unsigned char createIfMissing)
{
    unsigned int lockStorage;
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    void* facet;
    try
    {
        const unsigned int facetId =
            RecordDBGetOrAssignGlobalSequenceId(&Msvc6::FacetState<Msvc6::CharCtype>::id);
        facet = U32ToPtr(Msvc6Runtime::LocaleGetFacet(localeOwner, facetId, 1u));
        if (!facet)
        {
            if (!createIfMissing || !Msvc6Runtime::LocaleCanSupplyFacet(localeOwner))
                throw std::bad_cast();

            facet = const_cast<Msvc6::FacetData*>(Msvc6::FacetState<Msvc6::CharCtype>::_Psave);
            if (!facet)
            {
                void* const storage = CrtNew(24u);
                if (storage)
                {
                    try
                    {
                        (void)RecordDBCtypeCharCtor(storage, 0u, 0u, 0u);
                    }
                    catch (...)
                    {
                        CrtFreePointer(storage);
                        throw;
                    }
                }
                facet = RecordDBSaveCtypeFacet(static_cast<MsvcLocaleFacetRef*>(storage));
                Msvc6::FacetState<Msvc6::CharCtype>::_Psave = static_cast<const Msvc6::FacetData*>(facet);
            }
        }
    }
    catch (...)
    {
        Msvc6Runtime::StaticLockDestruct(&lockStorage);
        throw;
    }
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
    return facet;
}

bool zProfLdr::DeleteProfileEx(int _iProfileNum)
{
    if (m_aProfs[_iProfileNum].Length() <= 0)
        DebugAssert(5u, "m_aProfs[_iProfileNum].Length() > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Eng\\zProfLdr\\zProfLdr.cpp", 278u);

    STRING folderName = GetProfFolderName(_iProfileNum);

    iLogConsole* const logger =
        ::gpLogCons;
    (void)logger->LogStr(LT_NOTICE, "Deleting profile %s", folderName.m_value);

    STRING fileName = GetOptFName(_iProfileNum, false);
    zutl::MoveToTrash(fileName.m_value);

    STRING registryFile;
    registryFile = GetOptFName(_iProfileNum, true)+
        ".bak";

    ::Registry->SaveToFile(folderName, registryFile);
    ::Registry->DeleteKey(folderName);

    static const int saveTypesToDelete[4] = {0, 2, 4, 1};
    for (int level = 0;level<50;++level)
    {
        for (unsigned int saveTypeIndex = 0;saveTypeIndex<4u;++saveTypeIndex)
        {
            fileName = GetSaveName2(_iProfileNum, static_cast<SAVE_TYPE>(saveTypesToDelete[saveTypeIndex]), level);
            zutl::MoveToTrash(fileName.m_value);
        }
    }

    zutl::MoveToTrash(GetSaveName2(_iProfileNum, static_cast<SAVE_TYPE>(3), 0).m_value);

    if (m_currentProfile == static_cast<int>(_iProfileNum))
        m_currentProfile = -1;
    m_aProfs[_iProfileNum] = StringGlobals::EmptyText();

    zutl::MoveToTrash(GetPlayerNameFName(_iProfileNum).m_value);

    (void)ExpObjToReg();
    return true;
}


