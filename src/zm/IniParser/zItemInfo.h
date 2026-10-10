#pragma once
#include "mylib.h"
#include "zm/Utils/String/zString.h"

#include <stddef.h>

class zItemInfo
{
public:
    char** m_Data;
    int    m_DataCount;
    zString m_Name;

    zItemInfo();

    virtual void* QueryInterface(int id) const;
    virtual int iRelease();
    virtual void AddDataString(const char* text);
    virtual const char** GetData();
    virtual ~zItemInfo() noexcept(false);
    static void operator delete(void* memory);

    void SetName(const char* name);
    const char* GetName();
    void ReleaseData();
};

class zItemInfoList
{
    friend class zRPGFrameWork;
public:
    zItemInfo** m_aInfos;
    int m_ItemsNum;
    int* m_ItemIds;
    int m_iCurInfoPos;

    zItemInfoList();

    virtual void* QueryInterface(int id) const;
    virtual int iRelease();
    virtual void BeginReading(const char* name);
    virtual void ReleaseData(const char* name);
    virtual void AddDataString(const char* text);
    virtual const char** GetData(const char* name);
    virtual const char* GetFirstValuableString(const char* name);
    virtual int GetStringsCnt(const char* name);

private:
    virtual void SaveInfosToFile(const char* _sFileName);

public:
    virtual ~zItemInfoList() noexcept(false);
    static void operator delete(void* memory);

private:
    int GetItemId(const char* name);
    int InsertItem(int id, zItemInfo* info);
    int FindItemBinarySearch(int id, bool insertionMode);
    int FindItem(int id);
    int FindItemSimpleSearch(int id);
    void DeleteItem(int _iPos);
};


void __cdecl zItemInfo_RegisterSharedCleanupOnce();

