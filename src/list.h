#pragma once

#include "mylib.h"
#include "exports.h"

template<class T>
class LIST
{
public:
    virtual ~LIST();

    int No() const;
    void Push(const T& value);
    void Insert(T value);
    void InsertFirst(T value);
    void InsertBefore(int index,T value);
    int DeleteNumber(int index);
    int Location(const T& value) const;
    void Release();
    void Expand(int requestedCapacity);
    void ExpandForInsert();
    T NextIterateNon0(int* index) const;

    T* Data()
    {
        return m_data;
    }
    const T* Data() const
    {
        return m_data;
    }
    int Capacity() const
    {
        return m_capacity;
    }

public:

int m_count;
    int m_capacity;
    T*  m_data;
};

template<class T>
struct NAMED_LIST_STRUCT
{
    int m_hash;
    STRING m_name;
    T m_value;

    NAMED_LIST_STRUCT();
    NAMED_LIST_STRUCT& operator=(const NAMED_LIST_STRUCT& source);
    ~NAMED_LIST_STRUCT();
};

template<class T>
class NAMED_LIST : public LIST<NAMED_LIST_STRUCT<T> >
{
public:
    int Location(const STRING& name) const;
    void Insert(STRING name,T value);
    STRING Name(int index) const;
    static int GetHash(const char* name);

    T& Value(int index)
    {
        return this->m_data[index].m_value;
    }
    const T& Value(int index) const
    {
        return this->m_data[index].m_value;
    }
    NAMED_LIST_STRUCT<T>& Record(int index)
    {
        return this->m_data[index];
    }
    const NAMED_LIST_STRUCT<T>& Record(int index) const
    {
        return this->m_data[index];
    }
};

struct ACT;
template<> LIST<ACT>::~LIST();

template<> LIST<int>::~LIST();
template<> void LIST<int>::Expand(int requestedCapacity);

class SPRITE;

template<> LIST<SPRITE*>::~LIST();
template<> void LIST<SPRITE*>::Expand(int requestedCapacity);

template<> SPRITE* LIST<SPRITE*>::NextIterateNon0(int* index) const;


#include "basetype.h"

void  Msvc6BasicStreambufInit(void* self);
void  Msvc6BasicStreambufSetg(void* self,unsigned int begin,unsigned int middle,unsigned int end);
void  Msvc6BasicStreambufSetp(void* self,unsigned int begin,unsigned int end);

namespace Msvc6Runtime
{
    static inline void StaticLockConstruct(void* lockObject)
    {
        (void)CrtStaticLockConstruct(PtrToU32(lockObject));
    }
    static inline void StaticLockDestruct(void* lockObject)
    {
        (void)CrtStaticLockDestruct(PtrToU32(lockObject));
    }

static inline void BasicIosClear(void* ios, unsigned int state, unsigned int rethrow)
    {
        (void)Msvc6BasicIosClear(ios, state, rethrow);
    }

    static inline void BasicIosInit(void* ios)
    {
        (void)Msvc6BasicIosInit(ios);
    }
    static inline void RegisterStream(void* stream)
    {
        (void)Msvc6StreamRegistration(stream);
    }

    static inline bool ExceptionInFlight(void* context)
    {
        (void)context;
        return Msvc6ExceptionInFlight() != 0u;
    }

    static inline unsigned int GlobalLocale()
    {
        return Msvc6GlobalLocale();
    }

static inline unsigned int LocaleGetFacet(void* locale, unsigned int id, unsigned int create)
    {
        return Msvc6LocaleGetFacet(locale, id, create);
    }

    static inline bool LocaleCanSupplyFacet(void* locale)
    {
        return Msvc6LocaleCanSupplyFacet(locale) != 0u;
    }

    static inline unsigned int LocaleAddFacet(void* locale, unsigned int facet, unsigned int id, unsigned int replace)
    {
        return Msvc6LocaleAddFacet(locale, facet, id, replace);
    }

static inline void LocaleConstruct(void* locale, unsigned int sourceName)
    {
        (void)Msvc6LocaleConstruct(locale, sourceName);
    }

    static inline void LocaleDestruct(void* locale)
    {
        (void)Msvc6LocaleDestruct(locale);
    }

}
