#pragma once
#include <cstdio>

#include <stddef.h>
#include "zRPGBaseObject.h"

class iRPGFrameWork;
class iRPGObj;
enum OBJ_TYPE : int;
enum QUEST_STATE : int;
enum IT_TYPE : int;
enum CSI_ACTION : int;
enum JOIN_TYPE : int;

class iRPGQuest
{
public:
    enum INFO_STR : int {};

    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual void SetLevelNum(int value) = 0;
    virtual void SetQuestNum(int value) = 0;
    virtual void SetSubQNum(int value) = 0;
    virtual int GetLevelNum() = 0;
    virtual int GetQuestNum() = 0;
    virtual int GetSubQNum() = 0;
    virtual bool SetState(int state, bool validateTransition) = 0;
    virtual bool AddState(int state, bool validateTransition) = 0;
    virtual int GetState() = 0;
    virtual bool SetJoin(JOIN_TYPE _eType, iRPGQuest* object, bool active) = 0;
    virtual iRPGQuest* GetJoin(JOIN_TYPE _eType) = 0;
    virtual void SetTL(int value) = 0;
    virtual int GetTL() = 0;
    virtual void SetBeginNext(bool value) = 0;
    virtual bool GetBeginNext() = 0;
    virtual void SetUnactualNext(bool value) = 0;
    virtual bool GetUnactualNext() = 0;
    virtual bool GetStateFlagmanCoord(QUEST_STATE state, int& x, int& y, int& z, int& radius) = 0;
    virtual void SetStateFlagmanCoord(QUEST_STATE state, int x, int y, int z, int radius) = 0;
    virtual const char* GetStateItemName(QUEST_STATE state, CSI_ACTION action) = 0;
    virtual int GetStateItemVid(QUEST_STATE state, CSI_ACTION action) = 0;
    virtual void SetStateItemName(QUEST_STATE state, CSI_ACTION action, const char* value) = 0;
    virtual const char* GetStrByIemAction(CSI_ACTION action) = 0;
    virtual const char* GetStrByState(QUEST_STATE state) = 0;
    virtual iRPGObj* AsRPGObj() const = 0;
    virtual void ReleaseItems(IT_TYPE type) = 0;
    virtual void AddItems(IT_TYPE type, int value) = 0;
    virtual void SetCurTime(int value) = 0;
    virtual int GetCurTime() = 0;
    virtual void CheckFlagmanCoord(int worldX, int worldY) = 0;
    virtual void CheckStateItemNameOrVid(CSI_ACTION action, const char* _sItemName, int _iVidNum, int _iNamedMonsterId) = 0;
    virtual int GetNumStartQuests() = 0;
    virtual int GetStartQuest(int index) = 0;
    virtual int GetNumEndQuests() = 0;
    virtual int GetEndQuest(int index) = 0;
    virtual int GetNumItems(IT_TYPE type) = 0;
    virtual int GetItem(IT_TYPE type, int index) = 0;
    virtual int GetNamedMonstersDeath() = 0;
    virtual void PushCurState(std::FILE* stream) = 0;
    virtual void PopCurState(std::FILE* stream) = 0;
    virtual const char* GetMainQuestInfo() = 0;
    virtual void SetInfoStr(const char* text, INFO_STR infoType) = 0;
    virtual const char* GetInfoStr(INFO_STR infoType) = 0;
};

// Storage and insertion contract of the original std::vector<int> and
// std::vector<iRPGQuest*>. The allocator occupies the first four bytes on Win32.
template<class T>
struct zQuestVector
{
    unsigned char m_allocatorState;
    unsigned char m_allocatorPadding[3];
    T* m_begin;
    T* m_end;
    T* m_capEnd;

    zQuestVector() : m_begin(0), m_end(0), m_capEnd(0) {}
    ~zQuestVector() noexcept(false)
    {
        CrtFreePointer(m_begin);
        m_begin = 0;
        m_end = 0;
        m_capEnd = 0;
    }

    unsigned int size() const
    {
        return m_begin ? static_cast<unsigned int>(m_end-m_begin) : 0u;
    }

    void insert(T* position, unsigned int count, const T& value)
    {
        const unsigned int available = static_cast<unsigned int>(m_capEnd-m_end);
        if (available<count)
        {
            const unsigned int oldSize = size();
            const unsigned int growth = oldSize>count ? oldSize : count;
            const unsigned int capacity = oldSize+growth;
            const unsigned int allocationCount = BitsToSigned32(capacity)<0 ? 0u : capacity;
            T* const storage = static_cast<T*>(CrtNew(allocationCount*sizeof(T)));
            T* destination = storage;
            for (T* source = m_begin; source != position; ++source, ++destination)
                if (destination) *destination = *source;
            for (unsigned int index = 0; index<count; ++index, ++destination)
                if (destination) *destination = value;
            T* const oldEnd = m_end;
            for (T* source = position; source != oldEnd; ++source, ++destination)
                if (destination) *destination = *source;
            CrtFreePointer(m_begin);
            m_capEnd = storage+capacity;
            const unsigned int currentSize = size();
            m_begin = storage;
            m_end = storage+currentSize+count;
            return;
        }

        const unsigned int tailSize = static_cast<unsigned int>(m_end-position);
        if (tailSize<count)
        {
            T* destination = position+count;
            T* const oldEnd = m_end;
            for (T* source = position; source != oldEnd; ++source, ++destination)
                if (destination) *destination = *source;
            destination = m_end;
            const unsigned int fillCount = count-static_cast<unsigned int>(m_end-position);
            for (unsigned int index = 0; index<fillCount; ++index, ++destination)
                if (destination) *destination = value;
            T* const fillEnd = m_end;
            for (T* destination = position; destination != fillEnd; ++destination)
                *destination = value;
            m_end+=count;
        }
        else if (count>0)
        {
            T* const oldEnd = m_end;
            T* destination = oldEnd;
            for (T* source = oldEnd-count; source != oldEnd; ++source, ++destination)
                if (destination) *destination = *source;
            T* source = m_end-count;
            destination = m_end;
            while (source != position)
                *--destination = *--source;
            for (unsigned int index = 0; index<count; ++index)
                position[index] = value;
            m_end+=count;
        }
    }
};

class zRPGQuest : public zRPGObj, public iRPGQuest
{
public:
    int m_levelNum;
    int m_questNum;
    int m_subQuestNum;
    int m_state;
    int m_timeLine;
    zQuestVector<int> m_startQuests;
    zQuestVector<int> m_endQuests;
    zQuestVector<int> m_items[2];
    int m_flagmanCoords[8][4];
    zString m_stateItemNames[32];
    int m_stateItemVids[8][4];
    int m_curTime;
    bool m_beginNext;
    bool m_unactualNext;
    unsigned char m_pad34A[2];
    int m_namedMonstersDeath;
    int m_iOldState;
    int m_iOldExtInfoQuest;
    int m_savedCurTime;
    iRPGQuest* m_join[2];
    int m_joinId[2];

    explicit zRPGQuest(OBJ_TYPE questType);
    ~zRPGQuest() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;

    void SetDataArrays() override;
    void SetLevelNum(int levelNum) override;
    void SetQuestNum(int questNum) override;
    void SetSubQNum(int subQuestNum) override;
    int GetLevelNum() override;
    int GetQuestNum() override;
    int GetSubQNum() override;
    bool SetState(int state, bool validateTransition) override;
    bool AddState(int state, bool validateTransition) override;
    int GetState() override;
    void SetTL(int timeline) override;
    int GetTL() override;
    void SetBeginNext(bool beginNext) override;
    bool GetBeginNext() override;
    void SetUnactualNext(bool unactualNext) override;
    bool GetUnactualNext() override;
    int GetNumStartQuests() override;
    int GetStartQuest(int questIndex) override;
    int GetNumEndQuests() override;
    int GetEndQuest(int questIndex) override;
    iRPGObj* AsRPGObj() const override;
    int GetNamedMonstersDeath() override;
    void InitParams() override;
    void OnSkillChange(int selector, int oldValue, int newValue) override;
    void OnQuestBegin();
    void OnQuestComplete();
    bool GetStateFlagmanCoord(QUEST_STATE state, int& x, int& y, int& z, int& radius) override;
    void SetStateFlagmanCoord(QUEST_STATE state, int x, int y, int z, int radius) override;
    const char* GetStateItemName(QUEST_STATE state, CSI_ACTION action) override;
    int GetStateItemVid(QUEST_STATE state, CSI_ACTION action) override;
    void SetStateItemName(QUEST_STATE state, CSI_ACTION action, const char* itemName) override;
    void ReleaseItems(IT_TYPE type) override;
    void AddItems(IT_TYPE type, int itemId) override;
    int GetNumItems(IT_TYPE type) override;
    int GetItem(IT_TYPE type, int itemIndex) override;
    void SetCurTime(int currentTime) override;
    int GetCurTime() override;
    void CheckFlagmanCoord(int worldX, int worldY) override;
    void CheckStateItemNameOrVid(CSI_ACTION action, const char* _sItemName, int _iVidNum, int _iNamedMonsterId) override;
    bool CanBegin(int limit);

    void SetName(const char* name, bool parseQuestNumbers) override;
    bool GetParams(const char* text, int length, int& outPresent, int* outToken) const override;
    bool ParseUnknownCommand(const char* name, int count, const char** values,
                             iRPGFrameWork* framework) override;
    void PushCurState(std::FILE* stream) override;
    void PopCurState(std::FILE* stream) override;
    const char* GetStrByIemAction(CSI_ACTION action) override;
    const char* GetStrByState(QUEST_STATE state) override;
    const char* GetMainQuestInfo() override;
    void SetInfoStr(const char* text, iRPGQuest::INFO_STR infoType) override;
    const char* GetInfoStr(iRPGQuest::INFO_STR infoType) override;
    void SetJoin(JOIN_TYPE _eType, int joinedQuestId);
    bool SetJoin(JOIN_TYPE _eType, iRPGQuest* joinedQuest, bool active) override;
    iRPGQuest* GetJoin(JOIN_TYPE _eType) override;

private:
    static bool m__abArrayParams[MAX_RPGPARAMSCOUNT];
    static bool m__abBaseParams[MAX_RPGPARAMSCOUNT];
    static bool m__abChangeableParams[MAX_RPGPARAMSCOUNT];
};


