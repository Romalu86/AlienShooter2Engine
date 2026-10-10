#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "mylib.h"

#include "zm/IniParser/zItemInfo.h"
#include "zm/Utils/String/zString.h"
#include "basetype.h"
#include <new>
#include <windows.h>

int zItemInfoList::FindItemBinarySearch(int id, bool insertionMode)
{
    int result = -1;
    int low = 0;
    int high = m_ItemsNum - 1;

    while (low <= high)
    {

        const int middle = (high + low) / 2;
        const int current = m_ItemIds[middle];

        if (current == id)
        {
            result = middle;
            break;
        }

        if (current > id)
            high = middle - 1;
        else
            low = middle + 1;
    }

    if (result == -1 && insertionMode && high <= low)
    {
        int i = high;
        while (i >= 0 && i < m_ItemsNum && m_ItemIds[i] <= id)
        {
            ++i;
            result = high;
            if (i > low)
                break;
        }
    }

    return result;
}

int zItemInfoList::FindItem(int id)
{
    return FindItemBinarySearch(id, false);
}

int zItemInfoList::FindItemSimpleSearch(int id)
{
    for (int i = 0;i<m_ItemsNum;++i)
        if (m_ItemIds[i] == id)
            return i;
    return -1;
}

int zItemInfo::iRelease()
{
    if (PtrToU32(this) != 0u)
    {
        this->~zItemInfo();
        zItemInfo::operator delete(this);
    }
    return 0;
}

void zItemInfo::SetName(const char* name)
{
    m_Name = name;
}

const char* zItemInfo::GetName()
{
    return static_cast<const char*>(m_Name);
}

zItemInfoList::zItemInfoList()
    : m_aInfos(0), m_ItemsNum(0), m_ItemIds(0), m_iCurInfoPos(-1)
{
}

int zItemInfoList::GetItemId(const char* name)
{
    return static_cast<int>(zutl::GetHashPJW(name));
}

zItemInfo::zItemInfo()
    : m_Data(0), m_DataCount(0), m_Name()
{
}

void* zItemInfoList::QueryInterface(int) const
{
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 114u);
    return 0;
}

int zItemInfoList::iRelease()
{
    if (PtrToU32(this) != 0u)
    {
        this->~zItemInfoList();
        zItemInfoList::operator delete(this);
    }
    return 0;
}

void* zItemInfo::QueryInterface(int id) const
{
    if (id == 20) return const_cast<zItemInfo*>(this);
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 31u);
    return 0;
}

void __cdecl zItemInfo_RegisterSharedCleanupOnce()
{
    unsigned char& cleanupGuard = CrtStaticInitializationGuards[0];
    if ((cleanupGuard&1u) == 0u)cleanupGuard = static_cast<unsigned char>(cleanupGuard|1u);
    (void)RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

int zItemInfoList::GetStringsCnt(const char* name)
{
    const int index = FindItem(GetItemId(name));
    if (index<0 || !m_aInfos[index]) return 0;
    return m_aInfos[index]->m_DataCount;
}

const char** zItemInfoList::GetData(const char* name)
{
    const int index = FindItem(GetItemId(name));
    return index >= 0 && m_aInfos[index] ? m_aInfos[index]->GetData() : 0;
}

void zItemInfoList::ReleaseData(const char* name)
{
    const int index = FindItemBinarySearch(GetItemId(name), false);
    if (index != -1)
    {
        m_aInfos[index]->ReleaseData();
        DeleteItem(index);
    }
}

void zItemInfoList::AddDataString(const char* text)
{
    if (m_iCurInfoPos<0) DebugAssert(5u, "m_iCurInfoPos >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 233u);
    if (!m_aInfos[m_iCurInfoPos]) DebugAssert(5u, "m_aInfos[ m_iCurInfoPos ]", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 234u);
    m_aInfos[m_iCurInfoPos]->AddDataString(text);
}

void zItemInfoList::DeleteItem(int _iPos)
{
    if (_iPos<0 || _iPos >= m_ItemsNum) DebugAssert(5u, "_iPos >= 0 && _iPos < m_ItemsNum", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 365u);
    for (int i = _iPos+1;i<m_ItemsNum;++i)
    {
        m_aInfos[i-1] = m_aInfos[i];
        m_ItemIds[i-1] = m_ItemIds[i];
    }
    --m_ItemsNum;
}

const char* zItemInfoList::GetFirstValuableString(const char* name)
{
    const int index = FindItem(GetItemId(name));
    if (index<0 || !m_aInfos[index]) return 0;
    const char** values = m_aInfos[index]->GetData();
    if (!values) return 0;
    if (*values && **values == '<') ++values;
    return *values;
}

void zItemInfo::ReleaseData()
{
    for (int j = 0;j<m_DataCount;++j)
    {
        if (!m_Data[j]) DebugAssert(5u, "m_Data[ j ]", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 100u);
        std::free(m_Data[j]);
        m_Data[j] = 0;
    }
    m_DataCount = 0;
    std::free(m_Data);
    m_Data = 0;
}

const char** zItemInfo::GetData()
{
    return const_cast<const char**>(m_Data);
}

int zItemInfoList::InsertItem(int id, zItemInfo* info)
{
    const int at = FindItemBinarySearch(id, true)+1;
    const int oldCount = m_ItemsNum++;
    for (int i = oldCount;i>at;--i)
    {
        m_aInfos[i] = m_aInfos[i-1];
        m_ItemIds[i] = m_ItemIds[i-1];
    }
    m_aInfos[at] = info;
    m_ItemIds[at] = id;
    return at;
}

void zItemInfoList::operator delete(void* memory)
{
    std::free(memory);
}

zItemInfoList::~zItemInfoList() noexcept(false)
{
    for (int i = 0;i<m_ItemsNum;++i)
    {
        if (!m_aInfos[i]) DebugAssert(5u, "m_aInfos[ i ]", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 144u);
        zItemInfo* const info = m_aInfos[i];
        if (info)
        {
            info->~zItemInfo();
            zItemInfo::operator delete(info);
        }
        m_aInfos[i] = 0;
    }
    std::free(m_aInfos);
    std::free(m_ItemIds);
    m_aInfos = 0;
    m_ItemIds = 0;
}

void zItemInfo::operator delete(void* memory)
{
    std::free(memory);
}

zItemInfo::~zItemInfo() noexcept(false)
{
    ReleaseData();
}

void zItemInfo::AddDataString(const char* text)
{
    const int count = m_DataCount;
    if (count == 0 || count%8 == 7)
    {
        const unsigned int capacity = count == 0 ? 8u : static_cast<unsigned int>(count)+9u;
        const unsigned int bytes = capacity*static_cast<unsigned int>(sizeof(*m_Data));
        m_Data = static_cast<char**>(std::realloc(m_Data, bytes));
    }
    char* copy = reinterpret_cast<char*>(CrtAllocate(static_cast<unsigned int>(std::strlen(text))+1u));
    m_Data[m_DataCount] = copy;
    const int copyIndex = m_DataCount;
    const unsigned int copyBytes = static_cast<unsigned int>(std::strlen(text))+1u;
    char* destination = m_Data[copyIndex];
    std::memcpy(destination, text, copyBytes);
    m_Data[m_DataCount+1] = 0;
    ++m_DataCount;
}

void zItemInfoList::BeginReading(const char* name)
{
    const int id = GetItemId(name);
    if (FindItemBinarySearch(id, false) != -1) DebugAssert(4u, "this->FindItemBinarySearch( id, false ) == -1", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 171u);

    if (m_ItemsNum%8 == 0)
    {
        const unsigned int infoBytes = (static_cast<unsigned int>(m_ItemsNum)+8u)*static_cast<unsigned int>(sizeof(*m_aInfos));
        m_aInfos = static_cast<zItemInfo**>(std::realloc(m_aInfos, infoBytes));
        const unsigned int idBytes = (static_cast<unsigned int>(m_ItemsNum)+8u)*static_cast<unsigned int>(sizeof(*m_ItemIds));
        m_ItemIds = static_cast<int*>(std::realloc(m_ItemIds, idBytes));
    }

    void* storage = static_cast<unsigned char*>(CrtNew(sizeof(zItemInfo)));
    zItemInfo* info = storage ? new(storage) zItemInfo() : 0;
    m_iCurInfoPos = InsertItem(id, info);
    if (m_iCurInfoPos<0) DebugAssert(5u, "m_iCurInfoPos >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 182u);
    if (!m_aInfos[m_iCurInfoPos]) DebugAssert(5u, "m_aInfos[ m_iCurInfoPos ]", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 183u);
    m_aInfos[m_iCurInfoPos]->SetName(name);
}

void zItemInfoList::SaveInfosToFile(const char* _sFileName)
{
    if (!_sFileName) DebugAssert(4u, "_sFileName", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zItemInfo.cpp", 383u);
    zString backupName;
    const char* const backup = backupName.Format(
        "%s.bak", _sFileName);
    (void)MoveFileExA(_sFileName, backup, MOVEFILE_REPLACE_EXISTING | MOVEFILE_CREATE_HARDLINK);

    std::FILE* const file = std::fopen(_sFileName, "w");
    if (file)
    {
        for (int i = 0;i<m_ItemsNum;++i)
        {
            const char** lines = m_aInfos[i]->GetData();
            if (lines && *lines)
            {
                (void)std::fprintf(file, "<%s>\n", m_aInfos[i]->GetName());
                while (*lines)
                {
                    (void)std::fprintf(file, "%s\n", *lines);
                    ++lines;
                }
            }
        }
        (void)std::fclose(file);
    }
}

