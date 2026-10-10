#pragma once
#include "mylib.h"
#include <stddef.h>

#if !defined(_MSC_VER) && !defined(__thiscall)
#define __thiscall
#endif

class iRefObject;
class zRefObject;
template <class T> class iRef;

class iRefOwner
{
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void OnInvalidate(iRef<iRefObject>* value) = 0;
};

template <class T> class iRef
{
public:
    T* m_object = 0;
    zRefObject* m_refObject = 0;
    iRefOwner* m_owner = 0;

    ~iRef() noexcept(false) { Invalidate(); }
    void Set(T* target);
    void Invalidate();
};

class zRefObject
{
public:
    iRef<iRefObject>** m_pObjects;
    int m_iObjectsNum;
    int m_iCapacity;

    zRefObject();
    ~zRefObject() noexcept(false);
    static void operator delete(void* pointer);

    virtual void* QueryInterface(int id) const;
    virtual int iRelease();
    virtual void AddObjectRef(iRef<iRefObject>* reference);
    virtual void RemObjectRef(iRef<iRefObject>* reference);

    void InvalidateAllRefs();
};

template <class T>
void iRef<T>::Set(T* target)
{
    iRef<iRefObject>* const relation = reinterpret_cast<iRef<iRefObject>*>(this);
    if (m_object)
        m_refObject->RemObjectRef(relation);
    if (!target)
    {
        m_object = 0;
        m_refObject = 0;
        return;
    }
    m_object = target;
    m_refObject = static_cast<zRefObject*>(target->QueryInterface(2));
    if (m_object)
        m_refObject->AddObjectRef(relation);
}

template <class T>
void iRef<T>::Invalidate()
{
    if (m_object)
    {
        iRef<iRefObject>* const relation = reinterpret_cast<iRef<iRefObject>*>(this);
        m_refObject->RemObjectRef(relation);
        iRefOwner* const owner = m_owner;
        m_object = 0;
        m_refObject = 0;
        if (owner)
            owner->OnInvalidate(relation);
    }
}


