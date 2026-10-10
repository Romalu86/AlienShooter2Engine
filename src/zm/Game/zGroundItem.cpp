#include "mylib.h"
#include <cstring>
#include "zm/Game/zGroundItem.h"
#include "basetype.h"

void* zGroundItem::QueryInterface(int interfaceId) const
{
    if (interfaceId == 33)
        return const_cast<zGroundItem*>(this);
    return zItem::QueryInterface(interfaceId);
}


zGroundItem::zGroundItem(OBJ_TYPE objectType)
    : zItem(objectType), m_groundSprite(0), m_throwTime(0)
{
    for (int vidType = 0;vidType<4;++vidType)
    {
        m_vidNum[vidType] = 0;
        m_dirNum[vidType] = 0;
    }
    m_dirNum[0] = -1;
}


zGroundItem::~zGroundItem()
{
}

int zGroundItem::iRelease()
{
    delete this;
    return 0;
}

void zGroundItem::CopyParametersFrom(zGroundItem* source)
{
    zItem::CopyParametersFrom(source);
    for (int vidType = 0;vidType<4;++vidType)
    {
        SetVidNum(vidType, source->GetVidNum(vidType));
        SetDirNum(vidType, source->GetDirNum(vidType));
    }
    SetGrndSpr(source->GetGrndSpr());
}


bool zGroundItem::ParseUnknownCommand(const char* fieldName, int valueCount, const int* values)
{
    int vidType = -1;
    if (std::strcmp(fieldName, "VidGRND") == 0)vidType = 0;
    else if (std::strcmp(fieldName, "VidINVBig") == 0)vidType = 3;
    else if (std::strcmp(fieldName, "VidMENU") == 0)vidType = 1;
    else return false;

    SetVidNum(vidType, values[0]);
    if (vidType != 0 || valueCount>1)
        SetDirNum(vidType, values[1]);
    return true;
}


void zGroundItem::SetVidNum(int _iVidType, int vidNumber)
{
    if (_iVidType<0 || _iVidType >= 4)(void)DebugAssert(5u, "_iVidType >= 0 && _iVidType < GVT_CNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zGroundItem.cpp", 88u);
    m_vidNum[_iVidType] = vidNumber;
}


void zGroundItem::SetDirNum(int _iVidType, int direction)
{
    if (_iVidType<0 || _iVidType >= 4)(void)DebugAssert(5u, "_iVidType >= 0 && _iVidType < GVT_CNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zGroundItem.cpp", 97u);
    m_dirNum[_iVidType] = direction;
}


int zGroundItem::GetVidNum(int _iVidType) const
{
    if (_iVidType<0 || _iVidType >= 4)(void)DebugAssert(5u, "_iVidType >= 0 && _iVidType < GVT_CNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zGroundItem.cpp", 106u);
    if (_iVidType == 2)_iVidType = 1;
    return m_vidNum[_iVidType];
}


int zGroundItem::GetDirNum(int _iVidType) const
{
    if (_iVidType<0 || _iVidType >= 4)(void)DebugAssert(5u, "_iVidType >= 0 && _iVidType < GVT_CNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zGroundItem.cpp", 119u);
    if (_iVidType == 2)_iVidType = 1;
    return m_dirNum[_iVidType];
}


void zGroundItem::SetGrndSpr(SPRITE* sprite)
{
    m_groundSprite = sprite;
}


SPRITE* zGroundItem::GetGrndSpr() const
{
    return m_groundSprite;
}


void zGroundItem::SetThrowTime(int throwTime)
{
    m_throwTime = throwTime;
}


int zGroundItem::GetThrowTime() const
{
    return m_throwTime;
}



