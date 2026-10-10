#include "zm/Input/zInputMap.h"
#include "mylib.h"
#include "myerror.h"
#include "basetype.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <windows.h>
#pragma pop_macro("INPUT")

namespace
{

struct NameEntry
{
    unsigned int value;
    const char* name;
};

struct ActionEntry
{
    unsigned int action;
    unsigned int heldUntilRelease;
    const char* name;
};

const ActionEntry kActions[zInputMap::ACTION_COUNT] = {
    {IA_NONE, 0u, ""},
    {IA_MOVE_UP, 1u, "IA_MOVE_UP"},
    {IA_MOVE_DOWN, 1u, "IA_MOVE_DOWN"},
    {IA_MOVE_LEFT, 1u, "IA_MOVE_LEFT"},
    {IA_MOVE_RIGHT, 1u, "IA_MOVE_RIGHT"},
    {IA_ATTACK_COORD, 1u, "IA_ATTACK_COORD"},
    {IA_MOVE_TO_COORD, 1u, "IA_MOVE_TO_COORD"},
    {IA_WPN_PREV, 0u, "IA_WPN_PREV"},
    {IA_WPN_NEXT, 0u, "IA_WPN_NEXT"},
    {IA_WPN_0, 0u, "IA_WPN_0"},
    {IA_WPN_1, 0u, "IA_WPN_1"},
    {IA_WPN_2, 0u, "IA_WPN_2"},
    {IA_WPN_3, 0u, "IA_WPN_3"},
    {IA_WPN_4, 0u, "IA_WPN_4"},
    {IA_WPN_5, 0u, "IA_WPN_5"},
    {IA_FLASH, 0u, "IA_FLASH"},
    {IA_CAR_ESCAPE, 0u, "IA_CAR_ESCAPE"},
    {IA_RELOAD_WPN, 0u, "IA_RELOAD_WPN"},
    {IA_SHOW_INVENTORY, 0u, "IA_SHOW_INVENTORY"},
    {IA_SHOW_CHARSCREEN, 0u, "IA_SHOW_CHARSCREEN"},
    {IA_SHOW_STATEBAR, 0u, "IA_SHOW_STATEBAR"},
    {IA_SCREENSHOT, 0u, "IA_SCREENSHOT"},
    {IA_SHOW_DEBUG_CONSOLE, 0u, "IA_SHOW_DEBUG_CONSOLE"},
    {IA_SKIP_DIALOG, 0u, "IA_SKIP_DIALOG"},
    {IA_BOSS, 0u, "IA_BOSS"},
    {IA_ANY_KEY, 0u, ""},
    {IA_SHOW_LOG, 0u, "IA_SHOW_LOG"},
    {IA_XXX_YYY, 0u, ""},
    {0u, 0u, 0},
    {0u, 0u, 0},
    {0u, 0u, 0},
    {0u, 0u, 0}
};

const NameEntry kInputModes[] = {
    {IM_NONE, ""}, {IM_KEYDOWN, "IM_KEYDOWN"}, {IM_ALT, "IM_ALT"},
    {IM_CTRL, "IM_CTRL"}, {IM_SHIFT, "IM_SHIFT"},
    {IM_JOYBUTTON00, "IM_JOYBUTTON00"}, {IM_JOYBUTTON01, "IM_JOYBUTTON01"},
    {IM_JOYBUTTON02, "IM_JOYBUTTON02"}, {IM_JOYBUTTON03, "IM_JOYBUTTON03"},
    {IM_JOYBUTTON04, "IM_JOYBUTTON04"}, {IM_JOYBUTTON05, "IM_JOYBUTTON05"},
    {IM_JOYBUTTON06, "IM_JOYBUTTON06"}, {IM_JOYBUTTON07, "IM_JOYBUTTON07"},
    {IM_JOYBUTTON08, "IM_JOYBUTTON08"}, {IM_JOYBUTTON09, "IM_JOYBUTTON09"},
    {IM_JOYBUTTON10, "IM_JOYBUTTON10"}, {IM_JOYBUTTON11, "IM_JOYBUTTON11"},
    {IM_JOYBUTTON12, "IM_JOYBUTTON12"}, {IM_JOYBUTTON13, "IM_JOYBUTTON13"},
    {IM_JOYBUTTON14, "IM_JOYBUTTON14"}, {IM_JOYBUTTON15, "IM_JOYBUTTON15"},
    {IM_SPACE, "IM_SPACE"}, {IA_MOUSEWHEEL, "IA_MOUSEWHEEL"},
    {IM_MOUSEWHEELUP, "IM_MOUSEWHEELUP"}, {IM_MOUSEWHEELDOWN, "IM_MOUSEWHEELDOWN"},
    {IM_LBUTTONCLICK, "IM_LBUTTONCLICK"}, {IM_RBUTTONCLICK, "IM_RBUTTONCLICK"},
    {IM_MBUTTONCLICK, "IM_MBUTTONCLICK"}, {IM_LBUTTONDOWN, "IM_LBUTTONDOWN"},
    {IM_RBUTTONDOWN, "IM_RBUTTONDOWN"}, {IM_MBUTTONDOWN, "IM_MBUTTONDOWN"},
    {IM_LBUTTONUP, "IM_LBUTTONUP"}, {IM_RBUTTONUP, "IM_RBUTTONUP"},
    {IM_MBUTTONUP, "IM_MBUTTONUP"}
};

const NameEntry kKeyNames[] = {
    {VK_LBUTTON,"LBUTTON"}, {VK_RBUTTON,"RBUTTON"}, {VK_LEFT,"LEFT"},
    {VK_RIGHT,"RIGHT"}, {VK_UP,"UP"}, {VK_DOWN,"DOWN"},
    {VK_INSERT,"INSERT"}, {VK_DELETE,"DELETE"}, {VK_HOME,"HOME"},
    {VK_END,"END"}, {VK_PRIOR,"PGUP"}, {VK_NEXT,"PGDN"},
    {VK_SHIFT,"SHIFT"}, {VK_CONTROL,"CTRL"}, {VK_RETURN,"RETURN"},
    {VK_TAB,"TAB"}, {VK_F1,"F1"}, {VK_F2,"F2"}, {VK_F3,"F3"},
    {VK_F4,"F4"}, {VK_F5,"F5"}, {VK_F6,"F6"}, {VK_F7,"F7"},
    {VK_F8,"F8"}, {VK_F9,"F9"}, {VK_F10,"F10"}, {VK_F11,"F11"},
    {VK_F12,"F12"}, {VK_SPACE,"SPACE"}, {VK_BACK,"BACKSPACE"}, {VK_MENU,"ALT"}
};

bool IsEmptyMapping(const InpAction& action)
{
    return action.inputMode == IM_NONE || (action.inputMode == IM_KEYDOWN && action.inputValue == 0u);
}

bool IsReservedPair(unsigned int firstAction, unsigned int secondAction)
{
    if (firstAction == IA_SHOW_DEBUG_CONSOLE || secondAction == IA_SHOW_DEBUG_CONSOLE)
        return true;
    if (firstAction>secondAction)
    {
        const unsigned int swappedAction = firstAction;
        firstAction = secondAction;
        secondAction = swappedAction;
    }
    return firstAction == IA_CAR_ESCAPE && (secondAction == IA_RELOAD_WPN ||
        secondAction == IA_SHOW_INVENTORY || secondAction == IA_SHOW_CHARSCREEN);
}

unsigned int InputModeFromString(const char* text)
{
    unsigned int ret = IM_NONE;
    for (unsigned int i = 0;i<sizeof(kInputModes)/sizeof(kInputModes[0]);++i)
    {
        if (strcmp(kInputModes[i].name, text) == 0)
        {
            ret = kInputModes[i].value;
            break;
        }
    }
    if (ret == IM_NONE)
        DebugAssert(E_INVALID, "ret != IM_NONE",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Input\\zInputMap.cpp", 566);
    return ret;
}

unsigned int ActionFromString(const char* text)
{
    unsigned int ret = IA_NONE;
    for (unsigned int i = 0;i<27u;++i)
    {
        if (strcmp(kActions[i].name, text) == 0)
        {
            ret = kActions[i].action;
            break;
        }
    }
    if (ret == IA_NONE)
        DebugAssert(E_INVALID, "ret != IA_NONE",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Input\\zInputMap.cpp", 545);
    return ret;
}

char* SkipSpace(char* text)
{
    while (*text == ' ' || *text == '\t')
        ++text;
    return text;
}
}

InpAction zInputMap::m_aiMappings[zInputMap::ACTION_COUNT][zInputMap::MAX_MAPPING_COUNT] = {
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}},
    {{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u},{IM_NONE,0u}}
};
int zInputMap::m_iKeyWaitNum=0;
unsigned int zInputMap::m_bKeyWaitAct=IA_NONE;

zInputMap::zInputMap()
    : m_iFoundMapping(0)
{

}

zInputMap::~zInputMap()
{
}

void zInputMap::AddMapping(unsigned int actionIndex, unsigned int inputMode, unsigned long inputValue)
{
    if (inputMode == IM_NONE || (inputMode == IM_KEYDOWN && inputValue == 0u))
        return;

    int freeSlot = -1;
    for (int mappingIndex = 0;mappingIndex<MAX_MAPPING_COUNT;++mappingIndex)
    {
        const InpAction& stored = m_aiMappings[actionIndex][mappingIndex];
        if (IsEmptyMapping(stored))
        {
            freeSlot = mappingIndex;
            break;
        }
        if (stored.inputMode == inputMode && stored.inputValue == inputValue)
            return;
    }

    if (freeSlot<0)
    {
        DebugAssert(E_INVALID, "0",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Input\\zInputMap.cpp", 74);
        return;
    }

    m_aiMappings[actionIndex][freeSlot].inputValue = inputValue;
    m_aiMappings[actionIndex][freeSlot].inputMode = inputMode;
}

InpAction zInputMap::GetHotKey(unsigned int actionIndex, int _iNum)
{
    if (_iNum >= MAX_MAPPING_COUNT)
        DebugAssert(E_LOAD, "_iNum < MAX_MAPPNG_CNT",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Input\\zInputMap.cpp", 89);
    return m_aiMappings[actionIndex][_iNum];
}

unsigned int zInputMap::FirstMapping(unsigned int inputMode, unsigned long inputValue)
{
    unsigned int found = 0;
    for (unsigned int action = 0;action<ACTION_COUNT;++action)
    {
        for (int mapping = 0;mapping<MAX_MAPPING_COUNT;++mapping)
        {
            const InpAction& current = m_aiMappings[action][mapping];
            if (current.inputMode == inputMode && current.inputValue == inputValue)
                m_aiFoundMappings[found++] = action;
        }
    }
    m_aiFoundMappings[found] = IA_NONE;
    m_iFoundMapping = 0;
    return NextMapping();
}

unsigned int zInputMap::NextMapping()
{
    return m_aiFoundMappings[m_iFoundMapping++];
}

void zInputMap::RemMapping(const InpAction& input, unsigned int protectedActionIndex)
{
    for (unsigned int actionIndex = 0;actionIndex<ACTION_COUNT;++actionIndex)
    {
        for (int mappingIndex = 0;mappingIndex<MAX_MAPPING_COUNT;++mappingIndex)
        {
            InpAction& current = m_aiMappings[actionIndex][mappingIndex];
            if (current.inputMode == input.inputMode && current.inputValue == input.inputValue &&
               !IsReservedPair(actionIndex, protectedActionIndex))
                current.inputMode = IM_NONE;
        }
    }
}

void zInputMap::RemapKey(unsigned int inputMode, unsigned long inputValue)
{
    if (!(inputMode == IM_KEYDOWN && inputValue == VK_ESCAPE))
    {
        const InpAction action = {inputMode, inputValue};
        RemMapping(action, m_bKeyWaitAct);
        if (m_iKeyWaitNum >= MAX_MAPPING_COUNT)
            DebugAssert(E_LOAD, "m_iKeyWaitNum < MAX_MAPPNG_CNT",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Input\\zInputMap.cpp", 168);
        m_aiMappings[m_bKeyWaitAct][m_iKeyWaitNum] = action;
    }
    m_bKeyWaitAct = IA_NONE;
}

void zInputMap::FlushEvents()
{
}

void zInputMap::Reset()
{
}

unsigned int zInputMap::StringToKey(STRING text)
{
    text = text.ToUpper();
    text.RemoveEndChars(" \n\r\t");
    const char* chars = text.CharPtr();

    for (unsigned int i = 0;i<sizeof(kKeyNames)/sizeof(kKeyNames[0]);++i)
    {
        if (strcmp(chars, kKeyNames[i].name) == 0 && kKeyNames[i].value != 0u)
            return kKeyNames[i].value;
    }

    const unsigned int length = static_cast<unsigned int>(strlen(chars));
    if (length >= 3u && chars[0] == '\'' && chars[2] == '\'')
        return static_cast<unsigned int>(static_cast<signed char>(chars[1]));
    if (strcmp(chars, "0") == 0)
        return 0u;
    if (length >= 3u && chars[0] == '[' && chars[length-1u] == ']')
    {
        text.value[0] = ' ';
        text.value[length-1u] = ' ';
        return CrtAtoiText(text.value);
    }
    const int ret = static_cast<signed char>(chars[0]);
    if (ret == 0)
        DebugAssert(E_INVALID, "ret",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Input\\zInputMap.cpp", 463);
    return static_cast<unsigned int>(ret);
}

const char* zInputMap::InpMsgToString(unsigned int inputMode)
{
    for (unsigned int i = 0;i<sizeof(kInputModes)/sizeof(kInputModes[0]);++i)
        if (kInputModes[i].value == inputMode)
            return kInputModes[i].name;
    return "";
}

const char* zInputMap::KeyToString(unsigned int key,bool forcePrintable)
{
    for(unsigned int i=0;i<sizeof(kKeyNames)/sizeof(kKeyNames[0]);++i)
        if(kKeyNames[i].value==key && kKeyNames[i].name!=0)
            return kKeyNames[i].name;
    if(key==0u)
        return "0";

    static char text[24];
    if(forcePrintable) {
        sprintf(text,"'%c'",static_cast<signed char>(key));
        if(static_cast<int>(static_cast<signed char>(text[1]))!=static_cast<int>(key))
            sprintf(text,"[%d]\t;%c",static_cast<int>(key),static_cast<int>(key));
        return text;
    }

#if defined(_WIN32)
    unsigned char keyboard[256]={0};
    unsigned short translated[256];
    ToAscii(key,key,keyboard,translated,0);
    sprintf(text,"'%c'",static_cast<signed char>(translated[0]&0xFFu));
    (void)CrtUppercase(PtrToU32(text));
#else
    sprintf(text,"'%c'",static_cast<signed char>(key));
    (void)CrtUppercase(PtrToU32(text));
#endif
    return text;
}

bool zInputMap::Load(const char* fileName)
{
    FILE* file;
    {
        STRING mode("rt");
        STRING name(fileName);
        file = *name.CharPtr()?fopen(name.CharPtr(), mode.CharPtr()):0;
    }
    if (!file)
        return false;

    for (unsigned int action = 0;action<ACTION_COUNT;++action)
        for (int mapping = 0;mapping<MAX_MAPPING_COUNT;++mapping)
    {
            m_aiMappings[action][mapping].inputMode = IM_NONE;
            m_aiMappings[action][mapping].inputValue = 0u;
        }

    char line[200];
    while (!feof(file))
    {
        if (fgets(line, sizeof(line), file) == 0)
            continue;
        size_t length = strlen(line);
        while (length && (line[length-1] == '\n' || line[length-1] == '\r'))
            line[--length] = 0;
        char* comment = strchr(line, ';');
        if (comment) *comment = 0;
        if (line[0] == 0) continue;

        char* firstComma = strchr(line, ',');
        if (!firstComma) continue;
        char* modeText = firstComma+1;
        char* secondComma = strchr(modeText, ',');
        if (!secondComma) continue;

        *firstComma = 0;
        modeText = SkipSpace(modeText);
        *secondComma = 0;
        char* keyText = SkipSpace(secondComma+1);

        const unsigned int key = StringToKey(STRING(keyText));
        const unsigned int mode = InputModeFromString(modeText);
        const unsigned int action = ActionFromString(line);
        AddMapping(action, mode, key);
    }

    fclose(file);
    return true;
}

bool zInputMap::Save(const char* fileName)
{
    FILE* file;
    {
        STRING mode("w");
        STRING name(fileName);
        file = *name.CharPtr()?fopen(name.CharPtr(), mode.CharPtr()):0;
    }
    if (!file)
        return false;

    fprintf(file, ";input configuration file\n");
    for (unsigned int action = 0;action<ACTION_COUNT;++action)
    {
        for (int mapping = 0;mapping<MAX_MAPPING_COUNT;++mapping)
        {
            const InpAction& current = m_aiMappings[action][mapping];
            if (IsEmptyMapping(current))
                continue;
            const char* mode = InpMsgToString(current.inputMode);
            const char* key = KeyToString(static_cast<unsigned int>(current.inputValue), true);
            fprintf(file, "%s,\t\t\t%s,\t\t\t%s\n", kActions[action].name, mode, key);
        }
    }

    fclose(file);
    return true;
}

STRING STRING::ToUpper() const
{
    STRING converted(value);
    (void)CrtUppercase(PtrToU32(converted.value));
    return STRING(converted);
}


