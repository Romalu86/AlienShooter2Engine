#include <cstdio>
#include "mylib.h"
#include "zRPGBaseObject.h"
#include "zm/Inventory/zInventoryItem.h"
#include "zm/Game/zGroundItem.h"

#include "myerror.h"
#include "basetype.h"

bool zRPGObj::ParseUnknownCommand(const char* command, int valueCount, const int* values)
{
    if (zItem::ParseUnknownCommand(command, valueCount, values))
        return true;
    return zGroundItem::ParseUnknownCommand(command, valueCount, values);
}

bool zRPGObj::ParseUnknownCommand(const char* command, int valueCount, const char** values, iRPGFrameWork* frameWork)
{
    return zItem::ParseUnknownCommand(command, valueCount, values, frameWork);
}

bool zRPGObj::SaveToFile(std::FILE*)
{
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject_loadsave.cpp", 37);
    return true;
}



