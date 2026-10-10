#pragma once

class STRING;

enum
{
    IA_NONE=65535u,
    IM_NONE=65535u,
    IA_MOVE_UP=1u,
    IA_MOVE_DOWN=2u,
    IA_MOVE_LEFT=3u,
    IA_MOVE_RIGHT=4u,
    IA_ATTACK_COORD=5u,
    IA_MOVE_TO_COORD=6u,
    IA_WPN_PREV=7u,
    IA_WPN_NEXT=8u,
    IA_WPN_0=9u,
    IA_WPN_1=10u,
    IA_WPN_2=11u,
    IA_WPN_3=12u,
    IA_WPN_4=13u,
    IA_WPN_5=14u,
    IA_FLASH=15u,
    IA_CAR_ESCAPE=16u,
    IA_RELOAD_WPN=17u,
    IA_SHOW_INVENTORY=18u,
    IA_SHOW_CHARSCREEN=19u,
    IA_SHOW_STATEBAR=20u,
    IA_SCREENSHOT=21u,
    IA_SHOW_DEBUG_CONSOLE=22u,
    IA_SKIP_DIALOG=23u,
    IA_BOSS=24u,
    IA_ANY_KEY=25u,
    IA_SHOW_LOG=26u,
    IA_XXX_YYY=27u,
    IA_MOUSE_X=64u,
    IA_MOUSE_Y=65u,
    IA_SCREEN_MOUSE_X=66u,
    IA_SCREEN_MOUSE_Y=67u,
    IA_AXIS_X=68u,
    IA_AXIS_Y=69u,
    IA_AXIS2_X=70u,
    IA_AXIS2_Y=71u,
    IM_KEYDOWN=72u,
    IM_ALT=32u,
    IM_CTRL=33u,
    IM_SHIFT=34u,
    IM_JOYBUTTON00=38u,
    IM_JOYBUTTON01=39u,
    IM_JOYBUTTON02=40u,
    IM_JOYBUTTON03=41u,
    IM_JOYBUTTON04=42u,
    IM_JOYBUTTON05=43u,
    IM_JOYBUTTON06=44u,
    IM_JOYBUTTON07=45u,
    IM_JOYBUTTON08=46u,
    IM_JOYBUTTON09=47u,
    IM_JOYBUTTON10=48u,
    IM_JOYBUTTON11=49u,
    IM_JOYBUTTON12=50u,
    IM_JOYBUTTON13=51u,
    IM_JOYBUTTON14=52u,
    IM_JOYBUTTON15=53u,
    IM_SPACE=54u,
    IA_MOUSEWHEEL=73u,
    IM_MOUSEWHEELUP=62u,
    IM_MOUSEWHEELDOWN=63u,
    IM_LBUTTONCLICK=56u,
    IM_LBUTTONDBLCLK=57u,
    IM_RBUTTONCLICK=61u,
    IM_MBUTTONCLICK=59u,
    IM_LBUTTONDOWN=35u,
    IM_RBUTTONDOWN=37u,
    IM_MBUTTONDOWN=36u,
    IM_LBUTTONUP=55u,
    IM_RBUTTONUP=60u,
    IM_MBUTTONUP=58u
};

struct InpAction
{
    unsigned int inputMode;
    unsigned long inputValue;
};

class zInputMap
{
public:
    enum { ACTION_COUNT=32, MAX_MAPPING_COUNT=4 };

    zInputMap();
    virtual ~zInputMap();

    static void AddMapping(unsigned int actionIndex,unsigned int inputMode,unsigned long inputValue);
    static InpAction GetHotKey(unsigned int actionIndex,int _iNum);
    unsigned int FirstMapping(unsigned int inputMode,unsigned long inputValue);
    unsigned int NextMapping();
    void RemapKey(unsigned int inputMode,unsigned long inputValue);

    void FlushEvents();
    void Reset();

    static unsigned int StringToKey(STRING text);
    static const char* InpMsgToString(unsigned int inputMode);
    static const char* KeyToString(unsigned int key,bool forcePrintable);
    static bool Load(const char* fileName);
    static bool Save(const char* fileName);

    static InpAction m_aiMappings[ACTION_COUNT][MAX_MAPPING_COUNT];
    static int m_iKeyWaitNum;
    static unsigned int m_bKeyWaitAct;

private:
    void RemMapping(const InpAction& input,unsigned int protectedActionIndex);

    unsigned int m_aiFoundMappings[ACTION_COUNT];
    int m_iFoundMapping;
};

