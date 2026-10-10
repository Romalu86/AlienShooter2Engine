#pragma once
#include <stddef.h>


struct AS_RECORD_STRING_CONTROL
{
    static const unsigned int npos = static_cast<unsigned int>(-1);
    unsigned char allocatorState[4];
    char* data;
    unsigned int size;
    unsigned int capacity;
};

struct AS_RECORD_DWORD_VECTOR
{
    unsigned char allocatorState[4];
    int* begin;
    int* end;
    int* capacityEnd;

    void CopyConstruct(const AS_RECORD_DWORD_VECTOR& other);
    AS_RECORD_DWORD_VECTOR& operator=(const AS_RECORD_DWORD_VECTOR& other);
    void insert(int* position, unsigned int count, const int& value);
    int* insert(int* position, const int& value);
};

struct AS_RECORD
{
    AS_RECORD_STRING_CONTROL m_playerName;
    int m_totalScore;
    int m_monsterKills;
    int m_elapsedSeconds;
    unsigned int m_checksum;
    AS_RECORD_STRING_CONTROL m_recordTimestamp;
    AS_RECORD_DWORD_VECTOR m_weaponScores;
    AS_RECORD_DWORD_VECTOR m_weaponKills;
    unsigned char m_cheater;
    unsigned char m_051[3];

    AS_RECORD() noexcept;
    AS_RECORD(const AS_RECORD& other);
    ~AS_RECORD() noexcept(false);
    AS_RECORD& operator=(const AS_RECORD& other);


    int GetHash(int mode);
};

struct AS_RECORD_VECTOR
{
    unsigned char allocatorState[4];
    AS_RECORD* begin;
    AS_RECORD* end;
    AS_RECORD* capacityEnd;
};

struct AS_RECORD_PTR_VECTOR
{
    unsigned char allocatorState[4];
    AS_RECORD** begin;
    AS_RECORD** end;
    AS_RECORD** capacityEnd;

    void insert(AS_RECORD** position, unsigned int count, AS_RECORD* const& value);
};

class zRecordDB
{
public:


    unsigned char m_DataFlags[4];
    AS_RECORD_VECTOR m_Data[3];
    unsigned char m_OutFlags[4];
    AS_RECORD_PTR_VECTOR m_OutData[3];


    virtual void* QueryInterface(int interfaceId) const;
    virtual int iRelease();
    zRecordDB();
    virtual ~zRecordDB() noexcept(false);
    static void operator delete(void* memory);
    void AddRecord(int iType, const AS_RECORD& record);
    int GetRecCnt(int iType);
    const AS_RECORD* GetRecData(int iType, int index);
    void ClearTable(int iType);

private:
    bool LoadIfNeed(int iType);
    bool LoadFromFile(int iType);
    bool SaveToFile(int iType);
    void BuildDataIfNeed(int iType);
};

extern zRecordDB* RecordDatabase;

