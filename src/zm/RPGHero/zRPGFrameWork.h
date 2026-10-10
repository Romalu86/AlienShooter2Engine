#pragma once

#include <stddef.h>
#include "irpg.h"
#include "zm/Utils/String/zString.h"

class iPlayer;
class zItemInfo;
class zItemInfoList;
enum OBJ_TYPE : int;

template<class Value>
struct zFWTreeNode;

template<class Value>
struct zFWTreeMap
{
    unsigned char m_compareState;
    unsigned char m_allocatorState;
    unsigned char m_reserved02[2];
    zFWTreeNode<Value>* m_head;
    unsigned char m_multi;
    unsigned char m_reserved09[3];
    unsigned int m_size;

    static zFWTreeNode<Value>* _Nil;
    static unsigned int _Nilrefs;

    zFWTreeMap();
    ~zFWTreeMap() noexcept(false);
};

class zRPGFrameWork : public iRPGFrameWork, public iTextDB
{
public:
    zRPGFrameWork();
    virtual ~zRPGFrameWork() noexcept(false);
    static void operator delete(void* pointer);
    iRPGObj** m_apItems;
    int m_iItemsNum;
    int m_itemsCapacity;
    zFWTreeMap<iRPGObj*> m_NamesMap;
    zString m_textScratch;
    char** m_infoLines;
    int m_readingGroup;
    zItemInfo* m_ranks;
    zItemInfo* m_heroSkillNames;
    zItemInfoList* m_heroInfo;
    zItemInfoList* m_objectInfo;
    zItemInfoList* m_VidDirs;
    zItemInfoList* m_mouseTips;
    int m_objectIteratorIndex;
    int m_objectFilterMask;
    int m_objectFilterValue;
    int m_objectFilterDirection;
    int m_disabledGammaRed;
    int m_disabledGammaGreen;
    int m_disabledGammaBlue;
    zString m_playerName;
    zString m_loadedIniFileNames[50];
    int m_aIniFileVersions[50];
    int m_iIniFilesCnt;
    iPlayer* m_pPlayer;

    iTextDB* AsTextDB() override;
    void SetPlayer(iPlayer* player) override;
    int GetItemsCnt() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    iRPGObj* GetNewRPGObj(OBJ_TYPE type, bool clearLoadedFlag) override;
    iRPGObj* GetObjCopyS(const char* name, bool logIfMissing) override;
    iRPGObj* GetObjCopy(iRPGObj* sourceObject) override;
    int GetSkillValue(int objectId, int skill) override;
    int GetSkillValue(iRPGObj* object, int skill) override;
    const char* GetStringValue(int kind, int index) override;
    const char** GetHeroInfo(int objectId) override;
    void SetGammaItemDisabled(int red, int green, int blue) override;
    void GetGammaItemDisabled(int* red, int* green, int* blue) override;
    iRPGObj* GetObjByNamee(const char* name, bool logIfMissing) override;

    void SetPlayerName(const char* name) override;
    const char* GetPlayerName() override;
    iRPGObj* FirstObject(int objectMask, int typeOrClass, int extraId) override;
    int GetArcadeMod_() override;
    void WinLevel() override;
    void PlaySFXCurArmy(int sfxId) override;
    int ScriptGetNFunction(const char* functionName) override;
    int ScriptRun(int scriptId, const char* functionName, int argument1, int argument2, int argument3) override;
    int GetNoSprites(int vidIndex, unsigned int army) override;
    iRPGObj* GetItemById(int objectId) override;
    int GetCurFileVer() override;
    void SaveInfosToFile(const char* fileName) override;
    void SetPerkForAllHeroes(int perkId) override;
    void SetObjInfo(const iRPGObj* object, const char* text) override;
    void SetObjInfo(const char* itemName, const char* text) override;
    const char* GetObjInfo(const iRPGObj* object, bool expand) override;
    const char* GetObjInfo(const char* itemName) override;
    const char* GetStrIniString(const char* section, const char* key) override;
    const char* GetFirstString(int vid, int direction) override;
    const char** GetExtInfo(int selector, const char* key) override;
    const char** GetExtInfo(int selector, int firstKeyComponent, int secondKeyComponent) override;
    int GetFileVer(const char* fileName) override;
    bool ReleaseObject(iRPGObj* object) override;
    int GetItemInfo(const iRPGObj* object, char** outSlots, bool expand);
    const char* GetItemName(const iRPGObj* object) override;
    bool ReleaseObject(int objectIndex);
    const char* GetFormattedSkill(const iRPGObj* object, int type) override;
    int CheckHashesDebug() override;
    void AddBaseName(const char* name, iRPGObj* object) override;
    iRPGObj* GetGrndObject(int vidNumber, int wantedDirection) override;
    const char* GetTypeName(OBJ_TYPE type) override;
    void ResetScriptFunctions() override;
    bool BeginDataStringsReading(const char* name) override;
    bool BeginObjectInfoReading(const char* fieldName) override;
    void AddDataString(const char* value) override;
    void EndDataStringsReading() override;
    const char* GetSpeciallyDecodedString(const char** lines, int maxLines) override;
    iRPGObj* NextObject() override;
    int GetItemPosById(int objectId);
    void SetFileVer(const char* fileName, int fileVersion) override;
};

