#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"
#include "mylib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "zRPGHero.h"
#include "zRPGFrameWork.h"
#include "zRPGImplant.h"
#include "../Inventory/zInventoryItem.h"
#include "zm/Inventory/zInventory3D.h"
#include "../Utils/String/zString.h"

#include "basetype.h"


bool zRPGHero::SaveInv(std::FILE* output, int selector)
{
    iInventory* inventoryHandle = m_pInvServer->GetInv(selector);
    zInventory* inventory = static_cast<zInventory*>(         inventoryHandle->QueryInterface(46));
    inventoryHandle = m_pInvServer->GetInv(selector);
    zInventory3D* inventory3D = static_cast<zInventory3D*>(         inventoryHandle->QueryInterface(48));
    const int itemCount = inventory->m_apItems.begin         ? static_cast<int>(inventory->m_apItems.end-inventory->m_apItems.begin)         : 0;
    for (int index = 0;index<itemCount;++index)
    {
        zItem* item = inventory->GetItem(index);
        if (index != 0)
        {
            (void)std::fprintf(output, " ");
        }
        else
        {
            (void)std::fprintf(output, "%s:%s=", "Inv", inventory->GetName());
        }
        (void)std::fprintf(output, "%s", item->GetName());
        const int numInCell = item->GetNumInCell();
        if (numInCell != item->GetBuyNum())
            (void)std::fprintf(output, "[%d]", numInCell);
        if (inventory3D)
        {
            int x;
            int y;
            (void)inventory3D->GetItemCellPos(item, &x, &y);
            (void)std::fprintf(output, "<%d,%d>", x, y);
        }
        if (item->GetGameObj() == TYPE_IMPLANT)
        {
            iRPGHeroBase* implant = static_cast<iRPGHeroBase*>(item->QueryInterface(3));
            const int srandValue = BitsToSigned32(implant->GetSRandVal(false));
            if (srandValue != 0)
                (void)std::fprintf(output, "(%d)", srandValue);
            if (static_cast<iInvKeeper*>(this)->CanHoldObj(item))
                (void)std::fprintf(output, "{%d}", 777);
        }
        if (item->GetGameObj() == TYPE_ARMOR)
        {
            const int property = item->GetSkillValue(RPGARMOR_HP);
            if (property != RPG_BASE_HP)
                (void)std::fprintf(output, "{%d}", property);
        }
        if (item->GetGameObj() == TYPE_EQUIP)
        {
            const int property = item->GetSkillValue(RPGEQUIP_DroneHp);
            if (property != RPG_BASE_HP)
                (void)std::fprintf(output, "{%d}", property);
        }
    }
    if (itemCount != 0)
        (void)std::fprintf(output, "\n");
    return true;
}

bool zRPGHero::SaveToFile(std::FILE* output)
{
    zRPGObj* const objectLane = static_cast<zRPGObj*>(this);
    iRPGFrameWork* const descriptor = objectLane->GetFW();
    const char* const objectName = objectLane->GetName();
    const OBJ_TYPE objectType = static_cast<OBJ_TYPE>(objectLane->GetGameObj());
    const char* const typeName = descriptor->GetTypeName(objectType);
    (void)std::fprintf(output, "%s=%s\n", typeName, objectName);
    const bool baseResult = zRPGHeroBase::SaveToFile(output);
    const unsigned int sex = static_cast<unsigned int>(objectLane->GetSkillValue(RPGHERO_SEX));
    (void)std::fprintf(output, "%s=%s\n", "Sex", sex == 1u?"Female":"Male");
    const unsigned int srandValue = GetSRandVal(true);
    (void)std::fprintf(output, "%s=%u\n", "Srand", srandValue);
    (void)SaveInv(output, 12);
    (void)SaveInv(output, 13);
    (void)SaveInv(output, 14);
    for (int i = 0;i<19;++i)
        if (i<12 || i>14) (void)SaveInv(output, i);
    if (m_iCurWeapon >= 0)
    {
        iInventory* inventory = m_pInvServer->GetInv(m_iCurWeapon);
        const char* const inventoryName = inventory->GetName();
        (void)std::fprintf(output, "%s=%s\n", "CurWeapon", inventoryName);
    }
    return baseResult;
}


void __cdecl RPGHeroParseInventoryDescriptor(const char* source,
                                                int* countOut,
                                                int* xOut,
                                                int* yOut,
                                                zString* nameOut,
                                                int* srandOut,
                                                int* propertyOut)
{
    nameOut->Set(source);
    const char* openSquare = std::strchr(source, '[');
    const char* closeSquare = std::strchr(source, ']');
    const char* openAngle = std::strchr(source, '<');
    const char* closeAngle = std::strchr(source, '>');
    const char* comma = std::strchr(source, ',');
    const char* openParen = std::strchr(source, '(');
    const char* closeParen = std::strchr(source, ')');
    const char* openBrace = std::strchr(source, '{');
    const char* closeBrace = std::strchr(source, '}');
    const char* firstDelimiter = 0;
    const char* const delimiters[4] =
    {
        openSquare, openAngle, openParen, openBrace
    }
    ;
    for (int delimiterIndex = 0;delimiterIndex<4;++delimiterIndex)
        if (delimiters[delimiterIndex] && (!firstDelimiter || delimiters[delimiterIndex]<firstDelimiter))             firstDelimiter = delimiters[delimiterIndex];
    if (firstDelimiter)
        nameOut->StrNCpy(source, static_cast<int>(firstDelimiter-source));
    zString scratch;
    if (countOut && openSquare && closeSquare)
    {
        const int length = static_cast<int>(closeSquare-openSquare)-1;
        scratch.StrNCpy(openSquare+1, length);
        if (scratch.GetLength() != 0)
            *countOut = BitsToSigned32(CrtAtoiText(scratch.m_pBuf));
    }
    if (xOut && yOut && openAngle && comma && closeAngle)
    {
        const int xLength = static_cast<int>(comma-openAngle)-1;
        scratch.StrNCpy(openAngle+1, xLength);
        if (scratch.GetLength() != 0)
            *xOut = BitsToSigned32(CrtAtoiText(scratch.m_pBuf));
        const int yLength = static_cast<int>(closeAngle-comma)-1;
        scratch.StrNCpy(comma+1, yLength);
        if (scratch.GetLength() != 0)
            *yOut = BitsToSigned32(CrtAtoiText(scratch.m_pBuf));
    }
    if (srandOut && openParen && closeParen)
    {
        const int length = static_cast<int>(closeParen-openParen)-1;
        scratch.StrNCpy(openParen+1, length);
        if (scratch.GetLength() != 0)
            *srandOut = BitsToSigned32(CrtAtoiText(scratch.m_pBuf));
    }
    if (propertyOut)
    {
        if (openBrace && closeBrace)
        {
            const int length = static_cast<int>(closeBrace-openBrace)-1;
            scratch.StrNCpy(openBrace+1, length);
            if (scratch.GetLength() != 0)
                *propertyOut = BitsToSigned32(CrtAtoiText(scratch.m_pBuf));
        }
        else
        {
            *propertyOut = -1;
        }
    }
}

bool zRPGHero::ParseUnknownCommand(const char* name, int valueCount,
                                   const char** values, iRPGFrameWork* frameWork)
{
    if (zRPGHeroBase::ParseUnknownCommand(name, valueCount, values, frameWork))
        return true;
    if (std::strcmp(name, "StandFirmInv") == 0)
    {
        for (int i = 0;i<valueCount;++i)
        {
            iRPGObj* obj = frameWork->GetObjCopyS(values[i], true);
            if (obj)
            {
                zItem* item = static_cast<zItem*>(obj->QueryInterface(49));
                (void)m_customInventory0.PutItem(item, 0);
            }
        }
        return true;
    }
    if (std::strcmp(name, "DeathMatchInv") == 0)
    {
        for (int i = 0;i<valueCount;++i)
        {
            iRPGObj* obj = frameWork->GetObjCopyS(values[i], true);
            if (obj)
            {
                zItem* item = static_cast<zItem*>(obj->QueryInterface(49));
                (void)m_customInventory1.PutItem(item, 0);
            }
        }
        return true;
    }
    if (std::strncmp(name, "Inv", 3) == 0)
    {
        int inventoryIndex = -1;
        if (SubSigned32Wrap(BitsToSigned32(static_cast<unsigned int>(std::strlen(name))), 3)>0)
            inventoryIndex = m_pInvServer->GetInvByName(name+4);
        for (int i = 0;i<valueCount;++i)
        {
            int count = -1;
            int x = -1;
            int y = -1;
            int srandValue = 0;
            int propertyValue = 0;
            zString objectName;
            RPGHeroParseInventoryDescriptor(values[i], &count, &x, &y, &objectName, &srandValue, &propertyValue);
            if (srandValue != 0)
                (void)iRPGImplant::SeedImplantRand(srandValue);
            iRPGObj* obj = frameWork->GetObjCopyS(objectName.m_pBuf, true);
            if (srandValue != 0)
                (void)iRPGImplant::SeedImplantRand(0);
            if (obj)
            {
                if (srandValue != 0 && obj->GetGameObj() != TYPE_IMPLANT)
                    (void)DebugAssert(4u, "!SrandVal || obj->GetType() == TYPE_IMPLANT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHero_loadsave.cpp", 153u);
                const int maxInCell = obj->GetMaxNumInCell();
                while (count>maxInCell && g_iZmDebug != 0u)
                {
                    count = SubSigned32Wrap(count, maxInCell);
                    iRPGObj* copy = obj->GetObjCopy();
                    copy->SetNumInCell(maxInCell);
                    (void)PutItem(copy);
                }
                if (count>0)
                    obj->SetNumInCell(count);
                if (obj->GetGameObj() == TYPE_ARMOR && propertyValue != -1)
                    (void)obj->SetSkillValue(RPGARMOR_HP, propertyValue);
                else if (obj->GetGameObj() == TYPE_EQUIP && propertyValue != -1)
                    (void)obj->SetSkillValue(RPGEQUIP_DroneHp, propertyValue);
                else if (obj->GetGameObj() == TYPE_IMPLANT && propertyValue == 777)
                    (void)obj->SetSkillValue(RPGIMPLANT_ENABLED, 1);
                unsigned int putResult = 0u;
                zInventory3D* inventory3D = 0;
                if (inventoryIndex >= 0)
                {
                    iInventory* inventory = m_pInvServer->GetInv(inventoryIndex);
                    inventory3D = static_cast<zInventory3D*>(inventory->QueryInterface(48));
                }
                if (!inventory3D)
                {
                    putResult = static_cast<unsigned int>(PutItem(obj));
                }
                else
                {
                    zItem* item = static_cast<zItem*>(obj->QueryInterface(49));
                    if (x >= 0 && y >= 0)
                        putResult = inventory3D->PutItemToCellEx(item, x, y);
                    else                         putResult = inventory3D->PutItem(item, 0);
                }
                if ((putResult&1u) == 0u)
                {
                    typedef void (__cdecl *LogStrFunction)(iLogConsole*, LOG_TYPE, const char*, ...);
                    iLogConsole* logger = ::gpLogCons;
                    LogStrFunction* const logFunctions = *reinterpret_cast<LogStrFunction**>(logger);
                    const char* const heroName = static_cast<zRPGObj*>(this)->GetName();
                    logger = ::gpLogCons;
                    logFunctions[6](logger, LT_WARN, "Object %s was not put to hero %s!", values[i], heroName);
                }
            }
        }
        return true;
    }
    if (std::strcmp(name, "Sex") == 0)
    {
        const bool female = (std::strcmp(values[0], "Female") == 0);
        (void)static_cast<zRPGObj*>(this)->SetSkillValue(RPGHERO_SEX, female?1:0);
        return true;
    }
    if (std::strcmp(name, "CurWeapon") == 0)
    {
        const int index = GetInvServer()->GetInvByName(values[0]);
        if (index >= 0)
            SelectBestWeapon(index);
        return true;
    }
    return false;
}


