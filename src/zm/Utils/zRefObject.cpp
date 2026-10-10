#include "mylib.h"

#include "zm/Utils/zRefObject.h"
#include "basetype.h"
#include <new>

zRefObject::zRefObject()
    : m_pObjects(0), m_iObjectsNum(0), m_iCapacity(0)
{
}


zRefObject::~zRefObject() noexcept(false)
{
    InvalidateAllRefs();
    if (m_pObjects != 0)
    {
        std::free(m_pObjects);
        m_pObjects = 0;
    }
}


void zRefObject::operator delete(void* pointer)
{
    CrtFreePointer(pointer);
}


int zRefObject::iRelease()
{
    this->~zRefObject();
    zRefObject::operator delete(this);
    return 0;
}


void* zRefObject::QueryInterface(int id) const
{
    if (id == 2)
        return const_cast<zRefObject*>(this);
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Utils\\zRefObject.cpp", 9);
    return 0;
}


void zRefObject::AddObjectRef(iRef<iRefObject>* reference)
{
    if (m_iObjectsNum == m_iCapacity)
    {
        m_iCapacity = AddSigned32Wrap(m_iCapacity, 8);
        m_pObjects = static_cast<iRef<iRefObject>**>(
            std::realloc(m_pObjects, static_cast<unsigned int>(m_iCapacity) * sizeof(*m_pObjects)));
    }
    m_pObjects[m_iObjectsNum] = reference;
    m_iObjectsNum = AddSigned32Wrap(m_iObjectsNum, 1);
}


void zRefObject::InvalidateAllRefs()
{
    while (m_iObjectsNum != 0)
    {
        m_pObjects[SubSigned32Wrap(m_iObjectsNum, 1)]->Invalidate();
    }
}


void zRefObject::RemObjectRef(iRef<iRefObject>* reference)
{
    for (int referenceIndex = SubSigned32Wrap(m_iObjectsNum, 1); referenceIndex >= 0;
         referenceIndex = SubSigned32Wrap(referenceIndex, 1))
    {
        if (m_pObjects != 0 && m_pObjects[referenceIndex] == reference)
        {
            m_pObjects[referenceIndex] = m_pObjects[SubSigned32Wrap(m_iObjectsNum, 1)];
            m_iObjectsNum = SubSigned32Wrap(m_iObjectsNum, 1);
            return;
        }
    }
    DebugAssert(4, "success", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Utils\\zRefObject.cpp", 69);
}


