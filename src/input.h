#pragma once

#include "basetype.h"
#include "stream.h"
#include "zm/Input/zInputMap.h"

struct HWND__;
struct DIDEVICEINSTANCEA;
struct DIDEVICEOBJECTINSTANCEA;
struct IDirectInput8A;
struct IDirectInputDevice8A;

extern IDirectInput8A* g_pDI;
extern IDirectInputDevice8A* g_pJoystick;

int __stdcall EnumJoysticksCallback(const DIDEVICEINSTANCEA* instance, void* context);
int __stdcall EnumObjectsCallback(const DIDEVICEOBJECTINSTANCEA* instance, void* context);
long InitDirectInput(HWND__* hwnd);
void FreeDirectInput();

#pragma pack(push,1)
class INPUT
{
public:
    INPUT();
    ~INPUT();
    void Reset();
    void Init(HWND__* hwnd);
    int WorkWndMessage(HWND__* hwnd,unsigned long message,unsigned long wParam,unsigned long lParam);
    int GetAction(unsigned int action) const;
    const VECTOR2& MouseXY() const;
    const VECTOR2& ClientMouseXY() const
    {
        return m_clientMouseXY;
    }
    VECTOR2 ChangeCoor(VECTOR2 coor);
    void Save(STREAM* stream) const;
    void Load(STREAM* stream);
    void Tact();
    void PlayFFB(int effect);
    void StopFFB(int effect);
    void SetInputType(int type);

    static HWND__* hWnd;
    static unsigned long imEventsRemoveAfterTact;
    static unsigned long iaEventsRemoveAfterTact;
    static int inputType;
    static unsigned int pauseForceFeedBack;
    static unsigned int volumeForceFeedBack;
    static unsigned int capsForceFeedBack;
    static VECTOR2 lClickMouseCoor;
    static zInputMap inputMap;

private:
    friend class MAP;
    friend class MENU;

    void AddEvent(unsigned long event,unsigned long value);
    void RemoveEvent(unsigned long event,unsigned long value);
    void RemoveEventAfterTact(unsigned long event,unsigned long value);

    unsigned char m_networkDeltaTicks;
    unsigned char m_randomSyncValue;
    unsigned char m_networkControlFlags;
    VECTOR2 m_mouseXY;
    VECTOR2 m_clientMouseXY;
    short m_joystickX;
    short m_joystickY;
    short m_joystickZ;
    short m_joystickR;
    short m_wheelDelta;
    unsigned short m_keyCode;
    unsigned long m_actionEvents;
    unsigned long m_inputEvents;
};
#pragma pack(pop)
