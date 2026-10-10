#include <cstdio>
#include "../../engine_globals.h"
#include "zm/Utils/zLogConsole.h"
#include "map.h"
#include "vid.h"
#include "mylib.h"
#include "basetype.h"
#include "zIniParser.h"
#include "irpg.h"
#include "zm/Utils/String/zString.h"
#include "zm/RPGHero/iRPGBaseObjectConst.h"

#include <cstring>
#include <cstdlib>
#include <new>

char* IniLineBuffer = 0;
bool LoadingHeroParameters = false;

void zIniParser::SetGlobalFrameWork(iRPGFrameWork* value)
{
    m_globalFrameWork = value;
}

static void StripIniComment(char* text)
{
    const int length = static_cast<int>(std::strlen(text));
    for (int i = 0; i < length; ++i)
    {
        if (text[i] == ';')
        {
            text[i] = '\0';
            return;
        }
    }
}

int zIniParser::iRelease()
{
    if (PtrToU32(this) != 0u)
    {
        this->~zIniParser();
        zIniParser::operator delete(this);
    }
    return 0;
}

zIniParser::zIniParser()
    : m_globalFrameWork(0), m_readFrameWork(0), m_textDB(0), m_eCurReadMode(0)
{
    m_CurRPGObj.m_object = 0;
    m_CurRPGObj.m_refObject = 0;
    m_CurRPGObj.m_owner = 0;
}

bool zIniParser::AddIniFile(const char* fileName)
{
    return AddIniFileEx(fileName, false);
}

void* zIniParser::QueryInterface(int id) const
{
    if (id == 40) return const_cast<zIniParser*>(this);
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 132u);
    return 0;
}

static bool IsHorizontalSpace(char character)
{
    return character == ' ' || character == '\t';
}

void __cdecl zIniParser_RegisterSharedCleanupOnce()
{
    unsigned char& cleanupGuard = CrtStaticInitializationGuards[0];
    if ((cleanupGuard&1u) == 0u)cleanupGuard = static_cast<unsigned char>(cleanupGuard|1u);
    (void)RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

void zIniParser::EndDataReading()
{
    m_textDB->EndDataStringsReading();
    m_eCurReadMode = 0;
    m_CurRPGObj.Set(0);
}

void zIniParser::SetReadFrameWork(iRPGFrameWork* value)
{
    m_readFrameWork = value;
    if (value)
        m_textDB = static_cast<iTextDB*>(value->QueryInterface(28));
}

void zIniParser::operator delete(void* memory)
{
    std::free(memory);
}

zIniParser::~zIniParser() noexcept(false)
{
}

static void SkipLeadingWhitespace(char*& cursor)
{
    char* current = cursor;
    while (*current)
    {
        if (!IsHorizontalSpace(*current))break;
        ++current;
        cursor = current;
    }
}

static void TrimTrailingWhitespace(char*& text)
{
    char* characters = text;
    int length = 0;
    while (characters[length])++length;
    for (int characterIndex = length-1;characterIndex >= 0;--characterIndex)
    {
        if (!IsHorizontalSpace(characters[characterIndex]))break;
        characters[characterIndex] = 0;
    }
}

bool zIniParser::AddObjParamsArr(char* key, char* valueText)
{
    char* currentValue = valueText;
    for (;;)
    {
        char* delimiter = std::strchr(currentValue, '|');
        if (!delimiter)break;
        *delimiter = 0;
        (void)AddObjParams(key, currentValue);
        currentValue = delimiter+1;
    }
    return AddObjParams(key, currentValue);
}

bool zIniParser::AddIniFileEx(const char* fileName, bool keyValueMode)
{
    if (!fileName || !*fileName) return false;
    std::FILE* const file = std::fopen(fileName, "r");
    if (!file) return false;
    const bool result = AddIniFileEx(file, keyValueMode, fileName);
    (void)std::fclose(file);
    return result;
}

OBJ_TYPE zIniParser::GetRPGTypeByName(const char* name)
{
    for (int type = 0;type<13;++type)
    {
        const char* candidate = m_globalFrameWork->GetTypeName(static_cast<OBJ_TYPE>(type));
        const char* left = name;
        const char* right = candidate;
        while (*left == *right)
        {
            if (!*left) return static_cast<OBJ_TYPE>(type);
            ++left;
            ++right;
        }
    }
    return static_cast<OBJ_TYPE>(14);
}

void __cdecl GetNew_zIniParser(iIniParser** out)
{
    void* storage = static_cast<unsigned char*>(CrtNew(sizeof(zIniParser)));
    zIniParser* result = storage?new(storage) zIniParser():0;
    *out = static_cast<iIniParser*>(result);
}

char* zIniParser::PrepareIniString(std::FILE* stream)
{
    char* buffer = IniLineBuffer;
    char* line = std::fgets(buffer, 1000, stream);
    if (!line) return 0;

    buffer = IniLineBuffer;
    unsigned int length = static_cast<unsigned int>(std::strlen(buffer));
    while (length && (buffer[length-1] == '\n' || buffer[length-1] == '\r'))
        buffer[--length] = '\0';

    const bool preserveText = (m_eCurReadMode == 3);
    if (!preserveText)
    {
        StripIniComment(line);
        SkipLeadingWhitespace(line);
        TrimTrailingWhitespace(line);
    }
    return (*line || preserveText)?line:0;
}

bool __cdecl DevideStringIntoParts(char* text, char*& outLeft, char*& outRight, char delimiter)
{
    char* split = std::strchr(text, delimiter);
    if (!split)return 0;
    char* openingDelimiter = std::strchr(text, '[');
    char* closingDelimiter = std::strchr(text, ']');
    if (!openingDelimiter && !closingDelimiter)
    {
        openingDelimiter = std::strchr(text, '<');
        closingDelimiter = std::strchr(text, '>');
    }
    if (openingDelimiter && closingDelimiter && openingDelimiter<split && split<closingDelimiter)return 0;
    *split++ = 0;
    outLeft = text;
    outRight = split;
    SkipLeadingWhitespace(outLeft);
    SkipLeadingWhitespace(outRight);
    TrimTrailingWhitespace(outLeft);
    TrimTrailingWhitespace(outRight);
    return 1;
}

bool zIniParser::LoadObjFromFile(iRPGObj* _pObj, const char* text)
{
    if (!_pObj)DebugAssert(4u, "_pObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 560u);
    if (!text)return false;
    if (_pObj->GetGameObj() == TYPE_HERO) LoadingHeroParameters = true;
    m_CurRPGObj.Set(_pObj);
    m_eCurReadMode = 2;
    const bool ok = AddIniFileEx(text, true);
    if (ok)_pObj->SetClone(_pObj->GetClone()|2);
    LoadingHeroParameters = false;
    return ok;
}

bool zIniParser::AddIniFileEx(std::FILE* reader, bool _bLoadObj, const char* sourceName)
{
    zIniParser* parser = this;
    if (_bLoadObj && (!parser->m_CurRPGObj.m_object || parser->m_eCurReadMode != 2))
        DebugAssert(4u, "!_bLoadObj || ( m_pCurRPGObj && m_eCurReadMode == INI_READ_OBJPARAMS )", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 609u);

    iTextDB* ctx = parser->m_textDB;
    ctx->SetFileVer(sourceName, 0);
    unsigned int lineNo = 0u;

    while (std::feof(reader) == 0)
    {
        char* line = PrepareIniString(reader);
        ++lineNo;

        if (std::feof(reader) != 0) break;
        if (!line) continue;

        bool ok = false;
        if (!_bLoadObj)
        {
            ok = AddIniString(sourceName, line);
        }
        else
        {
            char* lhs = 0;
            char* rhs = 0;
            if (!DevideStringIntoParts(line, lhs, rhs, '=')) continue;
            ok = AddObjParamsArr(lhs, rhs);
        }

        if (!ok)
        {
            iLogConsole* log = ::gpLogCons;
            (void)log->LogStr(LT_INPTERR, "String %d in %s was not parsed!", BitsToSigned32(lineNo), sourceName);
        }
    }

    parser->EndDataReading();
    return true;
}

bool zIniParser::AddIniString(const char* sourceName, char* _sStr)
{
    if (!_sStr) DebugAssert(4u, "_sStr", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 167u);
    if (TryBeginHeader(_sStr)) return true;
    const unsigned int mode = static_cast<unsigned int>(m_eCurReadMode);
    if (mode == 4u) return true;
    if (mode == 1u || mode == 3u)
    {
        iTextDB* sink = m_textDB;
        sink->AddDataString(_sStr);
        return true;
    }
    char* left = _sStr;
    char* right = 0;
    if (DevideStringIntoParts(_sStr, left, right, '='))
    {
        if (mode == 2u)
        {
            AddObjParamsArr(left, right);
            return true;
        }
        if (mode == 0u && std::strcmp(left, "FileVer") == 0)
        {
            const int value = static_cast<int>(CrtAtoiText(right));
            iTextDB* sink = m_textDB;
            sink->SetFileVer(sourceName, value);
            return true;
        }
    }
    iLogConsole* log = ::gpLogCons;
    (void)log->LogStr(LT_INPTERR, "String %s is unparsable!!!", _sStr);
    return true;
}

void zIniParser::ConvertObjStr(const char* source, const iRPGObj* context, char* output, int flags)
{
    const char* tokenStart = 0;
    const char* separator = 0;
    char* writeCursor = output;
    if (!source)
    {
        *writeCursor = 0;
        return;
    }

    const char* cursor = source;
    for (;;++cursor)
    {
        const char character = *cursor;
        if (character == 0)
        {
            *writeCursor = 0;
            return;
        }

        if (character == '<' || character == '[')
        {
            tokenStart = cursor;
        }
        else if (character == '=')
        {
            separator = cursor;
        }
        else if (character == '>' || character == ']')
        {
            if (tokenStart && (!separator || separator<tokenStart))
            {
                const char* first = tokenStart+1;
                const unsigned int tokenLength = static_cast<unsigned int>(cursor-first);
                OutputSkillValue(first, static_cast<int>(tokenLength), writeCursor, context, flags);
                tokenStart = 0;
                separator = 0;
                continue;
            }

            for (const char* literalCursor = tokenStart;PtrToU32(literalCursor)<PtrToU32(cursor);++literalCursor) *writeCursor++ = *literalCursor;
            *writeCursor++ = '/';
            *writeCursor++ = character;
            tokenStart = 0;
            separator = 0;
            continue;
        }

        if (!tokenStart) *writeCursor++ = character;
    }
}

bool zIniParser::AddObjParams(char* ParamName, char* valueText)
{
    SkipLeadingWhitespace(valueText);
    TrimTrailingWhitespace(valueText);

    unsigned int capacity = 16u;
    unsigned int count = 0u;
    const char** tokens = reinterpret_cast<const char**>(
        CrtAllocate(capacity*sizeof(*tokens)));

    SkipLeadingWhitespace(valueText);
    char* CurStr = valueText;
    if (*CurStr)
    {
        tokens[0] = CurStr;
        count = 1u;
    }

    for (;;)
    {
        char* split = std::strchr(CurStr, ' ');
        if (!split)break;

        if (count == capacity)
        {
            capacity += 16u;
            tokens = static_cast<const char**>(std::realloc(tokens, capacity*sizeof(*tokens)));
        }

        *split = 0;
        CurStr = split+1;
        SkipLeadingWhitespace(CurStr);
        TrimTrailingWhitespace(CurStr);
        if (!*CurStr)DebugAssert(4u, "strlen( CurStr ) > 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 275u);
        tokens[count++] = CurStr;
    }

    if (!ParamName)DebugAssert(4u, "ParamName", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 284u);

    const int kind = static_cast<int>(GetRPGTypeByName(ParamName));
    unsigned char result;
    iRPGObj* object = m_CurRPGObj.m_object;
    if (count == 1u && kind != 14u && object && object->GetGameObj() == kind)
    {

        m_CurRPGObj.m_object->SetName(tokens[0], count != 0u);
        result = static_cast<unsigned char>(count);
    }
    else
    {
        result = AddParameter(ParamName, static_cast<int>(count), tokens)?1u:0u;
    }

    std::free(tokens);
    return result;
}

bool zIniParser::TryBeginHeader(char* line)
{
    unsigned char handled = 0u;
    unsigned int originalLength = 0u;
    if (line) originalLength = static_cast<unsigned int>(std::strlen(line));

    char* equalPos = std::strchr(line, '=');
    char* open = std::strchr(line, '[');
    char* close = std::strchr(line, ']');
    if (!open && !close)
    {
        open = std::strchr(line, '<');
        close = std::strchr(line, '>');
    }

    char* left = 0;
    char* right = 0;
    if (DevideStringIntoParts(line, left, right, '='))
    {
        const int kind = static_cast<int>(GetRPGTypeByName(left));
        if (kind != 14)
        {
            zIniParser* parser = this;
            parser->m_CurRPGObj.Set(0);

            iRPGFrameWork* resolver = parser->m_readFrameWork;
            iRPGObj* target = resolver->GetObjByNamee(right, false);
            parser->m_CurRPGObj.Set(target);

            if (!parser->m_CurRPGObj.m_object)
            {
                target = parser->m_readFrameWork->GetNewRPGObj(static_cast<OBJ_TYPE>(kind), true);
                parser->m_CurRPGObj.Set(target);
                parser->m_CurRPGObj.m_object->SetName(right, true);
            }

            parser->m_eCurReadMode = 2;
            handled = 1u;
        }
    }
    else if (open && close && open<close && !equalPos)
    {
        *close = '\0';
        ++open;
        zIniParser* parser = this;
        iTextDB* sink = parser->m_textDB;
        if (sink->BeginDataStringsReading(open))
        {
            handled = 1u;
            parser->m_eCurReadMode = 1;
            *close = '>';
        }
        else if (parser->m_textDB->BeginObjectInfoReading(open))
        {
            handled = 1u;
            parser->m_eCurReadMode = 3;
            *close = '>';
        }
        else
        {
            if (open == line+1)
            {
                parser->m_eCurReadMode = 4;
                iLogConsole* logger = ::gpLogCons;
                (void)logger->LogStr(LT_INPTERR, "!Obj %s was deleted, but Weapon.txt contain it's description", open);
            }
            handled = 1u;
            *close = '>';
        }
    }

    if (equalPos) *equalPos = '=';
    for (unsigned int i = 0u;i<originalLength;++i)
        if (line[i] == '\0') line[i] = ' ';
    return handled;
}

bool zIniParser::AddParameter(const char* key, int count, const char** tokens)
{
    zIniParser* parser = this;
    iRPGObj* object = parser->m_CurRPGObj.m_object;
    if (!object) return false;

    int* values = reinterpret_cast<int*>(
        CrtAllocate(static_cast<unsigned int>(count)*sizeof(*values)));
    if (count>0)
    {
        for (int i = 0;i<count;++i)
            values[i] = static_cast<int>(CrtAtoiText(tokens[i]));
    }

    int expectedCount;
    int parameterIds[10];
    int mismatchCount = 0;
    unsigned char result = 0u;

    const unsigned int keyLength = static_cast<unsigned int>(std::strlen(key));
    if (parser->m_CurRPGObj.m_object->GetParams(key, static_cast<int>(keyLength), expectedCount,
            parameterIds))
    {
        const bool sharedDescriptor = parser->m_CurRPGObj.m_object->IsArrayProperty(parameterIds[0]);

        if (count != expectedCount)
        {
            if (!sharedDescriptor)
            {
                DebugAssert(3u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 458u);
                mismatchCount = count;
            }
            else
            {
                parser->m_CurRPGObj.m_object->BeginArray(parameterIds[0]);
            }
        }
        else if (sharedDescriptor)
        {
            parser->m_CurRPGObj.m_object->BeginArray(parameterIds[0]);
        }

        const int dispatchCount = sharedDescriptor?count:expectedCount;
        if (dispatchCount>0)
        {
            for (int i = 0;i<dispatchCount;++i)
            {
                const int descriptor = sharedDescriptor?
                    parameterIds[0]:parameterIds[i];
                (void)parser->m_CurRPGObj.m_object->SetSkillValue(descriptor, values[i]);
            }
        }
        result = 1u;
    }
    else
    {
        result = parser->m_CurRPGObj.m_object->ParseUnknownCommand(key, count, values)?1u:0u;
        if (!result)
        {
            result = parser->m_CurRPGObj.m_object->ParseUnknownCommand(
                key, count, tokens,
                parser->m_globalFrameWork)?1u:0u;
        }
    }

    std::free(values);

    if (mismatchCount != 0)
    {
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_INPTERR, "%d parameters expected for command %s", mismatchCount, key);
        return false;
    }
    if (!result)
    {
        const char* const objectName = parser->m_CurRPGObj.m_object->GetName();
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_INPTERR, "Command %s for object %s is unknown!!!", key, objectName);
        return false;
    }
    return true;
}

void zIniParser::OutputSkillValue(const char* text, int length, char*& output, const iRPGObj* _pObj, int flags)
{
    iRPGObj* const currentObject = const_cast<iRPGObj*>(_pObj);
    if (!_pObj)DebugAssert(4u, "_pObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 774u);

    iInvKeeper* const keeper = const_cast<iInvKeeper*>(_pObj->GetKeeper());
    iRPGHero* const hero = keeper
        ? static_cast<iRPGHero*>(keeper->QueryInterface(0)) : 0;

    int valueCount = 0;
    int parameterIds[10];
    const bool hasParameters = _pObj->GetParams(
        text, static_cast<int>(length), valueCount, parameterIds);
    if (!hasParameters)
    {
        const int objectType = currentObject->GetGameObj();
        if (objectType == TYPE_WEAPON && _strnicmp(text, "Wp_acc", 6u) == 0)
        {
            const int accuracy = _pObj->GetSkillValueBase(RPGWEAPON_AIMRANGE);
            int band = 0;
            if (accuracy>5)band = (accuracy <= 15)?1:((accuracy <= 25)?2:3);

            zString key;
            key.Format("Aimrange%d", band);
            iRPGFrameWork* framework = m_globalFrameWork;
            iTextDB* itemText = framework->AsTextDB();
            const char* resolved = itemText->GetStrIniString(
                "items", static_cast<const char*>(key));
            if (resolved)
            {
                std::strcpy(output, resolved);
                output += std::strlen(resolved);
            }
        }

        if (currentObject->GetGameObj() == TYPE_EQUIP &&
           _strnicmp(text, "Damage", 6u) == 0)
        {
            int weaponIndex = 0;
            if (static_cast<int>(_pObj->GetType()) == 2)weaponIndex = 294;
            else if (static_cast<int>(_pObj->GetType()) == 3)weaponIndex = 297;
            else DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\IniParser\\zIniParser.cpp", 823u);

            if (weaponIndex != 0)
            {
                MAP* const table = ::Map;
                VID* const lowRecord = table->Vid(weaponIndex);
                VID* const highRecord = table->Vid(weaponIndex);

                zString damage;
                damage.Format("%d-%d",
                    lowRecord->deathDamageMin, highRecord->deathDamageMax);
                std::strcpy(output, static_cast<const char*>(damage));
                output += damage.GetLength();
            }
        }

        if (flags >= 0)
        {
            _pObj->OutputExtInfoStr(text, static_cast<int>(length), output, flags);
            return;
        }

        if (std::strncmp(text, "PlayerName", 10) == 0)
        {
            iRPGFrameWork* framework = m_globalFrameWork;
            const char* playerName = framework->GetPlayerName();
            if (playerName)
            {
                std::strcpy(output, playerName);
                output += std::strlen(playerName);
            }
            return;
        }

        if (currentObject->GetGameObj() != TYPE_HERO && std::strncmp(text, "Cost", 4) == 0)
        {
            zString cost;
            cost.Format("%d$",
                _pObj->GetBuyCostFull());
            if (hero && !hero->HasMoneyToBuy(_pObj))
            {
                std::strcpy(output, "<Font=07/>");
                output += std::strlen("<Font=07/>");
            }
            std::strcpy(output, static_cast<const char*>(cost));
            output += std::strlen(static_cast<const char*>(cost));
        }
        return;
    }

    if (keeper)
    {
        int markedIds[10];
        const int markedCount = keeper->NoSkillsToHold(_pObj, markedIds);
        if (markedCount>0)
        {
            for (int i = 0;i<markedCount;++i)
            {
                if (parameterIds[0] == markedIds[i])
                {
                    std::strcpy(output, "<Font=07/>");
                    output += std::strlen("<Font=07/>");
                }
            }
        }
    }

    bool first = true;
    for (int i = 0;i<valueCount;++i)
    {
        zString formatted;
        const int parameter = parameterIds[i];
        const int value = _pObj->GetSkillValue(parameter);
        formatted.Format("%d", value);

        if (currentObject->GetGameObj() == TYPE_ARMOR && parameter == RPGARMOR_MAXHP)
        {
            const int current = _pObj->GetSkillValue(RPGARMOR_HPCALCED);
            if (current<value)
            {
                if (current == 0)
                    formatted.Format("<Font=07/>%d / %d", current, value);
                else
                    formatted.Format("%d / %d", current, value);
            }
        }

        if (!first)
        {
            *output = '-';
            ++output;
        }
        std::strcpy(output, static_cast<const char*>(formatted));
        output += std::strlen(static_cast<const char*>(formatted));
        first = false;
    }
}

