#pragma once

#include <stddef.h>
#include <new>
#include "../RPGHero/zRPGBaseObject.h"

class iRPGFrameWork;
class iRPGHero;
class iSurvive
{
public:
    virtual void* QueryInterface(int interfaceId) const = 0;
    virtual int iRelease() = 0;
    virtual int GetIntrval(int deltaTime, iSurvive*& outInterval, int& outIndex, int* outDepth) = 0;
    virtual int GetMonsterVidsCnt(int _iPeriod) = 0;
    virtual int GetMonstersCount(int _iPeriod, int _iMonstersNum) = 0;
    virtual int GetMonstersVid(int _iPeriod, int _iMonstersNum) = 0;
    virtual int GetPeriodLength(int _iPeriod) = 0;
    virtual int GetPauseLength(int _iPeriod) = 0;
    virtual int GetBossToCrt(int period) = 0;
    virtual int GetBossBonusesCnt(int _iPeriod) = 0;
    virtual int GetBossBonusBaseId(int _iPeriod, int _iNum) = 0;
    virtual int GetPeriodBonusId(int _iPeriod) = 0;
    virtual int GetFullLen() = 0;
    virtual int GetAddWpnCnt(int period) = 0;
    virtual int ResolveWeaponIdForClass(int period, int monsterClass, iRPGHero* hero, iRPGFrameWork* frameWork) = 0;
    virtual int GetAddWpnId(int period, int num, iRPGHero* hero, iRPGFrameWork* frameWork) = 0;
    virtual iSurvive* GetPrevInt() = 0;
    virtual int GetPerCnt() = 0;
    virtual bool IsBossPeriod(int period) = 0;
    virtual int GetBossSize() = 0;
    virtual int GetBossHpCoeff() = 0;
    virtual int GetBossColor(int index) = 0;
    virtual int GetScoresCoeff() = 0;
};

template <class T>
class zSurviveVector
{
public:
    unsigned char m_allocatorState[4];
    T* m_begin;
    T* m_end;
    T* m_capacity;

    zSurviveVector() : m_begin(0), m_end(0), m_capacity(0) {}
    zSurviveVector(const zSurviveVector& other)
    {
        const unsigned int count = other.size();
        const unsigned int allocationCount = BitsToSigned32(count) < 0 ? 0u : count;
        m_begin = static_cast<T*>(CrtNew(allocationCount * sizeof(T)));
        m_end = Copy(other.m_begin, other.m_end, m_begin);
        m_capacity = m_end;
    }
    ~zSurviveVector() noexcept(false)
    {
        Destroy(m_begin, m_end);
        CrtFreePointer(m_begin);
        m_begin = 0;
        m_end = 0;
        m_capacity = 0;
    }
    zSurviveVector& operator=(const zSurviveVector& other)
    {
        if (this == &other)
            return *this;
        const unsigned int count = other.size();
        const unsigned int oldSize = size();
        if (count <= oldSize)
        {
            T* output = m_begin;
            for (const T* source = other.m_begin; source != other.m_end; ++source, ++output)
                *output = *source;
            Destroy(output, m_end);
            m_end = m_begin + count;
        }
        else if (count <= (m_begin ? static_cast<unsigned int>(m_capacity - m_begin) : 0u))
        {
            const T* source = other.m_begin;
            for (T* output = m_begin; output != m_end; ++source, ++output)
                *output = *source;
            Copy(source, other.m_end, m_end);
            m_end = m_begin + count;
        }
        else
        {
            Destroy(m_begin, m_end);
            CrtFreePointer(m_begin);
            const unsigned int allocationCount = BitsToSigned32(other.size()) < 0 ? 0u : other.size();
            m_begin = static_cast<T*>(CrtNew(allocationCount * sizeof(T)));
            m_end = Copy(other.m_begin, other.m_end, m_begin);
            m_capacity = m_end;
        }
        return *this;
    }
    unsigned int size() const
    {
        return m_begin ? static_cast<unsigned int>(m_end - m_begin) : 0u;
    }
    void clear()
    {
        Destroy(m_begin, m_end);
        m_end = m_begin;
    }
    T* erase(T* first, T* last)
    {
        T* output = first;
        T* const oldEnd = m_end;
        for (T* source = last; source != oldEnd; ++source, ++output)
            *output = *source;
        Destroy(output, oldEnd);
        m_end = output;
        return first;
    }
    void insert(T* position, unsigned int count, const T& value)
    {
        const unsigned int available = static_cast<unsigned int>(m_capacity - m_end);
        if (available < count)
        {
            const unsigned int oldSize = size();
            const unsigned int capacity = oldSize + (count < oldSize ? oldSize : count);
            const unsigned int allocationCount = BitsToSigned32(capacity) < 0 ? 0u : capacity;
            T* const storage = static_cast<T*>(CrtNew(allocationCount * sizeof(T)));
            T* output = Copy(m_begin, position, storage);
            output = Fill(output, count, value);
            Copy(position, m_end, output);
            Destroy(m_begin, m_end);
            CrtFreePointer(m_begin);
            m_capacity = storage + capacity;
            const unsigned int currentSize = size();
            m_begin = storage;
            m_end = storage + currentSize + count;
        }
        else if (static_cast<unsigned int>(m_end - position) < count)
        {
            Copy(position, m_end, position + count);
            Fill(m_end, count - static_cast<unsigned int>(m_end - position), value);
            T* const last = m_end;
            for (T* output = position; output != last; ++output)
                *output = value;
            m_end += count;
        }
        else if (count != 0u)
        {
            Copy(m_end - count, m_end, m_end);
            T* source = m_end - count;
            T* output = m_end;
            while (source != position)
                *--output = *--source;
            T* const last = position + count;
            for (output = position; output != last; ++output)
                *output = value;
            m_end += count;
        }
    }

private:
    static T* Copy(const T* first, const T* last, T* output)
    {
        while (first != last)
        {
            if (output) ::new(output) T(*first);
            ++first;
            ++output;
        }
        return output;
    }
    static T* Fill(T* output, unsigned int count, const T& value)
    {
        while (count--)
        {
            if (output) ::new(output) T(value);
            ++output;
        }
        return output;
    }
    static void Destroy(T* first, T* last)
    {
        while (first != last)
        {
            first->~T();
            ++first;
        }
    }
};


class zSurvive : public zRPGObj, public iSurvive
{
public:
    zSurviveVector<zSurviveVector<int> > m_aiMonstVids;
    zSurviveVector<zSurviveVector<int> > m_aiMonstCnt;
    zSurviveVector<int> m_aiPeriodsLen;
    zSurviveVector<int> m_aiPauseLen;
    zSurviveVector<int> m_withoutMenu;
    zSurviveVector<bool> m_bossPeriodFlags;
    zSurviveVector<int> m_aiBonus;
    zSurviveVector<int> m_aiBonusRand;
    zSurviveVector<zSurviveVector<int> > m_addedWeaponLists;

    iRPGFrameWork* m_frameWork;
    int m_iBossSize;
    int m_iBossHpCoeff;
    int m_iPauseBoss;
    int m_iBossColor[3];
    zSurviveVector<int> m_aiBossBonus;
    int m_iScoresCoeff;

    zSurvive(OBJ_TYPE _eType,iRPGFrameWork* frameWork);
    ~zSurvive() override;

    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    void SetDataArrays() override;
    void InitParams() override;
    bool GetParams(const char* name, int nameLength, int& outCount, int* outKind) const override;
    bool ParseUnknownCommand(const char* name, int count, const char** values, iRPGFrameWork* frameWork) override;
    void SetCharValue(int stringParam, const char* textValue) override;
    void OnSkillChange(int skillSelector, int oldValue, int newValue) override;
    void BeginArray(int kind) override;
    int GetIntrval(int deltaTime, iSurvive*& outInterval, int& outIndex, int* outDepth) override;
    int GetMonsterVidsCnt(int _iPeriod) override;
    int GetMonstersCount(int _iPeriod, int _iMonstersNum) override;
    int GetMonstersVid(int _iPeriod, int _iMonstersNum) override;
    int GetPeriodLength(int _iPeriod) override;
    int GetPauseLength(int _iPeriod) override;
    int GetBossToCrt(int period) override;
    int GetBossBonusesCnt(int _iPeriod) override;
    int GetBossBonusBaseId(int _iPeriod, int _iNum) override;
    int GetPeriodBonusId(int _iPeriod) override;
    int GetFullLen() override;
    int GetAddWpnCnt(int period) override;
    int ResolveWeaponIdForClass(int period, int monsterClass, iRPGHero* hero, iRPGFrameWork* frameWork) override;
    int GetAddWpnId(int period, int num, iRPGHero* hero, iRPGFrameWork* frameWork) override;
    iSurvive* GetPrevInt() override;
    int GetPerCnt() override;
    bool IsBossPeriod(int period) override;
    int GetBossSize() override;
    int GetBossHpCoeff() override;
    int GetBossColor(int index) override;
    int GetScoresCoeff() override;
    zSurvive* GetNextInt();
    int GetWpnIdForClass(int period, int num, iRPGHero* hero, iRPGFrameWork* frameWork);
    int GetAbsPeriodNum(int value);
    int GetBonusBaseId(int _iNum);
    int GetWithoutMenu(int index);
    static void ResetMonst2Create();
    static int ms_MonstCntCoeff;
    static int ms_EmptyBonusWeightCoeff;
    static unsigned int __cdecl RegisterMonsterCountCache();
    static unsigned int __cdecl RegisterMonsterTimeCache();
    int GetMonst2Create(int period, int tactTime, int monstNum);

private:
    static bool m__abChangeableParams[50];
    static bool m__abBaseParams[50];
    static bool m__abArrayParams[50];
    static zSurvive* m_pM2CInterval;
    static int m_iM2CPeriod;
    static zSurviveVector<int>& m_aM2CMonst;
    static zSurviveVector<int>& m_aM2CLastTime;
    static unsigned char m_destroyedCacheFlags;
    static void DestroyMonsterCountCache();
    static void DestroyMonsterTimeCache();

    int GetMonst2Create_type1(int period, int tactTime, int monstNum);
    int GetMonst2Create_type2(int period, int tactTime, int monstNum);
};


