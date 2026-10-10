#pragma once
#include <cstdio>
#include "basetype.h"
#include "zm/Utils/zRefObject.h"

#include <stddef.h>

class iRPGFrameWork;
class iRPGObj;
class iTextDB;

enum OBJ_TYPE : int;

class iIniParser
{
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void SetGlobalFrameWork(iRPGFrameWork* value) = 0;
    virtual void SetReadFrameWork(iRPGFrameWork* value) = 0;
    virtual bool AddIniFile(const char* fileName) = 0;
    virtual bool LoadObjFromFile(iRPGObj* object, const char* fileName) = 0;
    virtual void ConvertObjStr(const char* source, const iRPGObj* context, char* output, int flags) = 0;
    virtual ~iIniParser() noexcept(false)
    {
    }
};

class zIniParser : public iIniParser
{
public:
    iRef<iRPGObj>  m_CurRPGObj;
    iRPGFrameWork* m_globalFrameWork;
    iRPGFrameWork* m_readFrameWork;
    iTextDB*       m_textDB;
    int            m_eCurReadMode;

    zIniParser();
    virtual ~zIniParser() noexcept(false) override;
    static void operator delete(void* memory);
    virtual void* QueryInterface(int id) const override;
    virtual int iRelease() override;
    virtual void SetGlobalFrameWork(iRPGFrameWork* value) override;
    virtual void SetReadFrameWork(iRPGFrameWork* value) override;
    virtual bool AddIniFile(const char* fileName) override;
    virtual bool LoadObjFromFile(iRPGObj* _pObj, const char* fileName) override;
    virtual void ConvertObjStr(const char* source, const iRPGObj* context, char* output, int flags) override;

    void EndDataReading();

private:
    bool AddIniFileEx(const char* fileName, bool keyValueMode);
    bool AddIniFileEx(std::FILE* reader, bool _bLoadObj, const char* sourceName);
    char* PrepareIniString(std::FILE* reader);
    bool AddIniString(const char* sourceName, char* _sStr);
    bool TryBeginHeader(char* line);
    bool AddObjParams(char* ParamName, char* valueText);
    bool AddObjParamsArr(char* key, char* valueText);
    OBJ_TYPE GetRPGTypeByName(const char* name);
    bool AddParameter(const char* key, int count, const char** tokens);
    void OutputSkillValue(const char* text, int length, char*& output, const iRPGObj* _pObj, int flags);
};

extern char* IniLineBuffer;
extern bool LoadingHeroParameters;

void __cdecl zIniParser_RegisterSharedCleanupOnce();
void __cdecl GetNew_zIniParser(iIniParser** out);
bool __cdecl DevideStringIntoParts(char* text, char*& outLeft, char*& outRight, char delimiter);
