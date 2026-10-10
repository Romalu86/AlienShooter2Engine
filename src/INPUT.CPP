#include "input.h"
#include "zm/Input/zInputMap.h"
#include "graph.h"
#include "map.h"
#include "sprite.h"
#include "player.h"
#include "myerror.h"
#include "mylib.h"
#include <math.h>

#if defined(_WIN32)
#  ifndef DIRECTINPUT_VERSION
#    define DIRECTINPUT_VERSION 0x0800
#  endif
#endif
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <windows.h>
#if defined(_WIN32)
#include <dinput.h>
#endif
#pragma pop_macro("INPUT")

IDirectInput8A* g_pDI=0;
IDirectInputDevice8A* g_pJoystick=0;

namespace
{

    unsigned int g_joystickSliderCount=0;
    unsigned int g_joystickPovCount=0;
}

HWND__* INPUT::hWnd=0;
unsigned long INPUT::imEventsRemoveAfterTact=0;
unsigned long INPUT::iaEventsRemoveAfterTact=0;
int INPUT::inputType=1;
unsigned int INPUT::pauseForceFeedBack=0;
unsigned int INPUT::volumeForceFeedBack=100;
unsigned int INPUT::capsForceFeedBack=0;
VECTOR2 INPUT::lClickMouseCoor={0.0f,0.0f};
zInputMap INPUT::inputMap;

int __stdcall EnumJoysticksCallback(const DIDEVICEINSTANCEA* instance,void*            )
{
#if defined(_WIN32)
    const HRESULT createResult=g_pDI->CreateDevice(instance->guidInstance,&g_pJoystick,0);


    return FAILED(createResult)?DIENUM_CONTINUE:DIENUM_STOP;
#else
    (void)instance;
    return 0;
#endif
}

int __stdcall EnumObjectsCallback(const DIDEVICEOBJECTINSTANCEA* instance,void*            )
{
#if defined(_WIN32)
    if((instance->dwType&DIDFT_AXIS)!=0u)
    {
        DIPROPRANGE range;
        range.diph.dwSize=sizeof(range);
        range.diph.dwHeaderSize=sizeof(range.diph);
        range.diph.dwObj=instance->dwType;
        range.diph.dwHow=DIPH_BYID;
        range.lMin=-1000;
        range.lMax=1000;
        if(FAILED(g_pJoystick->SetProperty(DIPROP_RANGE,&range.diph)))
            return DIENUM_STOP;
    }

    if(IsEqualGUID(instance->guidType,GUID_Slider))
        ++g_joystickSliderCount;
    if(IsEqualGUID(instance->guidType,GUID_POV))
        ++g_joystickPovCount;
    return DIENUM_CONTINUE;
#else
    (void)instance;
    return 0;
#endif
}

long InitDirectInput(HWND__* hwnd)
{
#if defined(_WIN32)
    if(!g_pDI)
    {
        const HRESULT createHr=DirectInput8Create(
            GetModuleHandleA(0),DIRECTINPUT_VERSION,IID_IDirectInput8A,
            reinterpret_cast<void**>(&g_pDI),0);
        if(FAILED(createHr))
            return createHr;

        const HRESULT enumHr=g_pDI->EnumDevices(
            DI8DEVCLASS_GAMECTRL,EnumJoysticksCallback,0,DIEDFL_ATTACHEDONLY);
        if(FAILED(enumHr))
            return enumHr;
    }

    if(!g_pJoystick)
        return 0;

    HRESULT hr=g_pJoystick->SetDataFormat(&c_dfDIJoystick);
    if(FAILED(hr))
        return hr;
    hr=g_pJoystick->SetCooperativeLevel(
        hwnd,DISCL_EXCLUSIVE|DISCL_FOREGROUND);
    if(FAILED(hr))
        return hr;
    hr=g_pJoystick->EnumObjects(EnumObjectsCallback,hwnd,DIDFT_ALL);
    return FAILED(hr)?hr:0;
#else
    (void)hwnd;
    return 0;
#endif
}

void FreeDirectInput()
{
#if defined(_WIN32)
    if(g_pJoystick)
    {
        g_pJoystick->Unacquire();

        if(g_pJoystick)
        {
            g_pJoystick->Release();
            g_pJoystick=0;
        }
    }
    if(g_pDI)
    {
        g_pDI->Release();
        g_pDI=0;
    }
#endif
}

INPUT::INPUT()
{
    Reset();
}

INPUT::~INPUT()
{
    FreeDirectInput();
}

namespace
{
    int g_windowPositionX=0;
    int g_windowPositionY=0;

    static float ClampViewportX(float value, float minimum, float maximum)
    {
        if (value<minimum) value = minimum;
        if (!(value<maximum)) value = maximum-1.0f;
        return value;
    }

    static double ClampViewportY(double value, float minimum, float maximum)
    {
        if (!(value >= minimum)) value = minimum;
        if (value >= maximum) value = static_cast<double>(maximum)-1.0;
        return value;
    }

    static void InputReportError(TYPE_ERROR type, const char* text, unsigned int errorCode)
    {
        STRING message(text);
        if (!Error) return;
        STRING category("INPUT");
        Error->Error(category, type, message, errorCode);
    }

    static int AbsInputAxis(short value)
    {
        const int axisValue = static_cast<int>(value);
        return axisValue<0?-axisValue:axisValue;
    }
}

void INPUT::Reset()
{

    m_networkDeltaTicks = 0;
    m_randomSyncValue = 0;
    m_networkControlFlags = 0;
    m_mouseXY = VECTOR2(0.0f, 0.0f);
    m_clientMouseXY = VECTOR2(0.0f, 0.0f);
    m_joystickX = 0;
    m_joystickY = 0;
    m_joystickZ = 0;
    m_joystickR = 0;
    m_wheelDelta = 0;
    m_keyCode = 0;
    m_actionEvents = 0;
    m_inputEvents = 0;
    inputMap.Reset();
}

void INPUT::Init(HWND__* hwnd)
{
    hWnd=hwnd;
    if(inputType!=1) return;

    const long initResult=InitDirectInput(hwnd);
    if(initResult<0)
        InputReportError(E_INIT,"DirectInput",static_cast<unsigned int>(initResult));

#if defined(_WIN32)
    if(!g_pJoystick) return;

    DIDEVCAPS caps;
    caps.dwSize=sizeof(caps);
    (void)g_pJoystick->GetCapabilities(&caps);
    capsForceFeedBack=caps.dwFlags&DIDC_FORCEFEEDBACK;
#endif
}

int INPUT::WorkWndMessage(HWND__* hwnd,unsigned long message,unsigned long wParam,unsigned long lParam)
{
    const unsigned int msg=static_cast<unsigned int>(message);
    const unsigned int key=static_cast<unsigned int>(wParam);

    switch(msg)
    {
    case WM_MOVE:
#if defined(_WIN32)
        {
            RECT rect;
            (void)::GetWindowRect(hwnd,&rect);
            g_windowPositionX=rect.left;
            g_windowPositionY=rect.top;
        }
#else
        (void)hwnd;
#endif
        return 0;

    case WM_ACTIVATE:
#if defined(_WIN32)
        if(wParam!=0u && g_pJoystick)
            (void)g_pJoystick->Acquire();
#endif
        return 0;

    case WM_ACTIVATEAPP:
        if(wParam!=0u) return 0;
        m_keyCode=0;
        RemoveEvent(IM_SHIFT,0u);
        RemoveEvent(IM_CTRL,0u);
        RemoveEvent(IM_ALT,0u);
        return 0;

    case WM_NCHITTEST:
        {
#if defined(_WIN32)
            RECT rect;
            ::GetWindowRect(hwnd,&rect);
            g_windowPositionX=rect.left;
            g_windowPositionY=rect.top;
#else
            const int left=g_windowPositionX;
            const int top=g_windowPositionY;
#endif
            const int screenX=static_cast<int>(static_cast<unsigned short>(lParam&0xFFFFu));
            const int screenY=static_cast<int>(static_cast<unsigned short>((lParam>>16)&0xFFFFu));
#if defined(_WIN32)
            m_clientMouseXY.x=static_cast<float>(SubSigned32Wrap(screenX,rect.left));
            const double clientY=static_cast<double>(SubSigned32Wrap(screenY,rect.top));
            m_clientMouseXY.y=static_cast<float>(clientY);
#else
            m_clientMouseXY.x=static_cast<float>(SubSigned32Wrap(screenX,left));
            const double clientY=static_cast<double>(SubSigned32Wrap(screenY,top));
            m_clientMouseXY.y=static_cast<float>(clientY);
#endif
            const VECTOR2 viewMin=VECTOR2(Graph->viewLeft,Graph->viewTop);
            const VECTOR2 viewMax=VECTOR2(Graph->viewRight,Graph->viewBottom);
            const float clampedX=ClampViewportX(m_clientMouseXY.x,viewMin.x,viewMax.x);
            const double clampedY=ClampViewportY(clientY,viewMin.y,viewMax.y);

            if(Map)
            {
                const VECTOR2 mapOrigin=Map->ShiftXY();
                m_mouseXY.x=static_cast<float>(static_cast<double>(clampedX)/Graph->screenScaleX+
                    Graph->viewportOffsetX+mapOrigin.x);
                const float dividedY=static_cast<float>(clampedY/Graph->screenScaleY);
                const float offsetY=dividedY+Graph->viewportOffsetY;
                m_mouseXY.y=offsetY+mapOrigin.y;
            }

            if(Mouse)
            {
                const VECTOR oldPosition=Mouse->Coor();
                Mouse->ChangeCoor(VECTOR(m_mouseXY,oldPosition.z));
            }

            return m_clientMouseXY.x>=Graph->viewLeft && m_clientMouseXY.y>=Graph->viewTop &&
                   m_clientMouseXY.x<Graph->viewRight && m_clientMouseXY.y<Graph->viewBottom;
        }

    case WM_INITMENU:
        m_keyCode=0;
        RemoveEvent(IM_SHIFT,0u);
        RemoveEvent(IM_CTRL,0u);
        RemoveEvent(IM_ALT,0u);
        return 0;

    case WM_KEYDOWN:
        m_keyCode=static_cast<unsigned short>((key&0xFFu)<<8);
        AddEvent(IM_KEYDOWN,key);
        if(key==VK_SHIFT) { AddEvent(IM_SHIFT,0u); return 0; }
        if(key==VK_CONTROL) { AddEvent(IM_CTRL,0u); return 0; }
        if(key==VK_SPACE) { AddEvent(IM_SPACE,0u); AddEvent(IA_ANY_KEY,0u); return 0; }
        AddEvent(IA_ANY_KEY,0u);
        return 0;

    case WM_SYSKEYDOWN:
        AddEvent(IM_KEYDOWN,key);
        if(key==VK_MENU) AddEvent(IM_ALT,0u);
        return 0;

    case WM_CHAR:
        m_keyCode=static_cast<unsigned short>(key&0xFFu);
        return 0;

    case WM_KEYUP:
        RemoveEventAfterTact(IM_KEYDOWN,key);
        if(key==VK_SHIFT) { RemoveEvent(IM_SHIFT,0u); return 0; }
        if(key==VK_CONTROL) { RemoveEvent(IM_CTRL,0u); return 0; }
        if(key==VK_SPACE) { RemoveEvent(IM_SPACE,0u); return 0; }
        if(key==VK_SNAPSHOT) AddEvent(IA_SCREENSHOT,0u);
        return 0;

    case WM_SYSKEYUP:
        RemoveEventAfterTact(IM_KEYDOWN,key);
        if(key==VK_MENU) RemoveEvent(IM_ALT,0u);
        return 0;

    case WM_MOUSEWHEEL:
        m_wheelDelta=static_cast<short>(static_cast<short>((wParam>>16)&0xFFFFu)/WHEEL_DELTA);
        if(m_wheelDelta>0) AddEvent(IM_MOUSEWHEELUP,0u);
        if(m_wheelDelta<0) AddEvent(IM_MOUSEWHEELDOWN,0u);
        return 0;

    case WM_LBUTTONDOWN:
        lClickMouseCoor=m_clientMouseXY;
        AddEvent(IM_LBUTTONCLICK,0u);
        AddEvent(IM_LBUTTONDOWN,0u);
        AddEvent(IA_ANY_KEY,0u);
        return 0;

    case WM_LBUTTONDBLCLK:
        lClickMouseCoor=m_clientMouseXY;
        AddEvent(IM_LBUTTONDBLCLK,0u);
        AddEvent(IM_LBUTTONCLICK,0u);
        AddEvent(IM_LBUTTONDOWN,0u);
        AddEvent(IA_ANY_KEY,0u);
        return 0;

    case WM_LBUTTONUP:
        RemoveEventAfterTact(IM_LBUTTONDOWN,0u);
        AddEvent(IM_LBUTTONUP,0u);
        return 0;

    case WM_RBUTTONDOWN:
        AddEvent(IM_RBUTTONCLICK,0u);
        AddEvent(IM_RBUTTONDOWN,0u);
        AddEvent(IA_ANY_KEY,0u);
        return 0;

    case WM_RBUTTONUP:
        RemoveEventAfterTact(IM_RBUTTONDOWN,0u);
        AddEvent(IM_RBUTTONUP,0u);
        return 0;

    case WM_MBUTTONDOWN:
        AddEvent(IM_MBUTTONCLICK,0u);
        AddEvent(IM_MBUTTONDOWN,0u);
        return 0;

    case WM_MBUTTONUP:
        RemoveEventAfterTact(IM_MBUTTONDOWN,0u);
        AddEvent(IM_MBUTTONUP,0u);
        return 0;

    default:
        return 0;
    }
}

int INPUT::GetAction(unsigned int action) const
{
    if (action <= 31u) return static_cast<int>((1u<<action)&m_actionEvents);
    if (action <= 63u) return static_cast<int>((1u<<(action-32u))&m_inputEvents);
    switch (action)
    {
    case IA_MOUSE_X:
        return TruncateFloatToInt32(m_mouseXY.x);
    case IA_MOUSE_Y:
        return TruncateFloatToInt32(m_mouseXY.y);
    case IA_SCREEN_MOUSE_X:
        return TruncateFloatToInt32(m_clientMouseXY.x);
    case IA_SCREEN_MOUSE_Y:
        return TruncateFloatToInt32(m_clientMouseXY.y);
    case IA_AXIS_X:
        return static_cast<int>(m_joystickX);
    case IA_AXIS_Y:
        return static_cast<int>(m_joystickY);
    case IA_AXIS2_X:
        return static_cast<int>(m_joystickZ);
    case IA_AXIS2_Y:
        return static_cast<int>(m_joystickR);
    case IM_KEYDOWN:
        return static_cast<int>(m_keyCode);
    case IA_MOUSEWHEEL:
        return static_cast<int>(m_wheelDelta);
    default:
        return 0;
    }
}

const VECTOR2& INPUT::MouseXY() const
{
    return m_mouseXY;
}

void INPUT::AddEvent(unsigned long event, unsigned long value)
{
    const unsigned int code = static_cast<unsigned int>(event);
    const unsigned int data = static_cast<unsigned int>(value);

    if (zInputMap::m_bKeyWaitAct != IA_NONE)
    {
        if (code<IM_SPACE || code == IM_KEYDOWN || code == IM_MOUSEWHEELUP || code == IM_MOUSEWHEELDOWN)
            inputMap.RemapKey(code, data);
        return;
    }

    if (code <= 31u)
    {
        m_actionEvents|=(1u<<(code&31u));
        return;
    }

    if (code <= 63u || code == IM_KEYDOWN)
    {
        unsigned int mapped = inputMap.FirstMapping(code, data);
        while (mapped != IA_NONE)
        {
            if (mapped <= 31u) m_actionEvents|=(1u<<(mapped&31u));
            mapped = inputMap.NextMapping();
        }
        m_inputEvents|=(1u<<((code-32u)&31u));
    }
}

void INPUT::RemoveEvent(unsigned long event, unsigned long value)
{
    const unsigned int code = static_cast<unsigned int>(event);
    const unsigned int data = static_cast<unsigned int>(value);

    if (code <= 31u)
    {
        m_actionEvents&=~(1u<<(code&31u));
        return;
    }

    if (code <= 63u || code == IM_KEYDOWN)
    {
        unsigned int mapped = inputMap.FirstMapping(code, data);
        while (mapped != IA_NONE)
        {
            if (mapped <= 31u) m_actionEvents&=~(1u<<(mapped&31u));
            mapped = inputMap.NextMapping();
        }
        m_inputEvents&=~(1u<<((code-32u)&31u));
    }
}

void INPUT::RemoveEventAfterTact(unsigned long event, unsigned long value)
{
    const unsigned int code = static_cast<unsigned int>(event);
    const unsigned int data = static_cast<unsigned int>(value);
    if (code <= 31u)
    {
        iaEventsRemoveAfterTact|=(1u<<(code&31u));
        return;
    }
    if (code <= 63u || code == IM_KEYDOWN)
    {
        unsigned int mapped = inputMap.FirstMapping(code, data);
        while (mapped != IA_NONE)
        {
            if (mapped <= 31u) iaEventsRemoveAfterTact|=(1u<<(mapped&31u));
            mapped = inputMap.NextMapping();
        }
        imEventsRemoveAfterTact|=(1u<<((code-32u)&31u));
    }
}

VECTOR2 INPUT::ChangeCoor(VECTOR2 coor)
{
    if(!(coor.x>=0.0f)) coor.x=0.0f;
    if(!(coor.y>=0.0f)) coor.y=0.0f;
    const float maxX=Graph->screenWidth;
    const float maxY=Graph->screenHeight;
    if(!(maxX>coor.x)) coor.x=maxX-1.0f;
    if(!(maxY>coor.y)) coor.y=maxY-1.0f;
#if defined(_WIN32)
    const int cursorY=AddSigned32Wrap(TruncateFloatToInt32(coor.y),g_windowPositionY);
    const int cursorX=AddSigned32Wrap(TruncateFloatToInt32(coor.x),g_windowPositionX);
    ::SetCursorPos(cursorX,cursorY);
#endif
    return coor;
}

void INPUT::Save(STREAM* stream) const
{
    stream->Write(this, sizeof(*this));
}

void INPUT::Load(STREAM* stream)
{
    stream->Read(this, sizeof(*this));
}

void INPUT::Tact()
{
    m_keyCode=0;
    m_wheelDelta=0;
    inputMap.FlushEvents();

    m_actionEvents&=0x0000007Fu;
    m_inputEvents&=0x007FFFFFu;
    m_actionEvents&=~iaEventsRemoveAfterTact;
    m_inputEvents&=~imEventsRemoveAfterTact;
    iaEventsRemoveAfterTact=0u;
    imEventsRemoveAfterTact=0u;

#if defined(_WIN32)
    if(g_pJoystick)
    {
        HRESULT hr=g_pJoystick->Poll();
        if(FAILED(hr))
        {
            hr=g_pJoystick->Acquire();
            unsigned int retry=0u;
            while(hr==DIERR_INPUTLOST && retry<5u)
            {
                hr=g_pJoystick->Acquire();
                ++retry;
            }
        }

        else
        {
            DIJOYSTATE state;
            if(SUCCEEDED(g_pJoystick->GetDeviceState(sizeof(state),&state)))
            {
                m_joystickX=static_cast<short>(state.lX);
                m_joystickY=static_cast<short>(state.lY);
                if(g_joystickSliderCount!=0u)
                {

                    m_joystickZ=static_cast<short>(state.lRz);
                    m_joystickR=static_cast<short>(state.rglSlider[0]);
                }

                for(unsigned int action=IM_JOYBUTTON00;action<IM_SPACE;++action)
                {
                    const bool wasDown=GetAction(action)!=0;
                    const bool isDown=(state.rgbButtons[action-IM_JOYBUTTON00]&0x80u)!=0u;
                    if(wasDown && !isDown)
                    {
                        RemoveEvent(action,0u);
                        if((Map->Flags()&0x20u)!=0u && Map->MenuSelectedSprite()!=0)
                            AddEvent(IM_LBUTTONUP,0u);
                    }
                    else if(!wasDown && isDown)
                    {
                        AddEvent(action,0u);
                        if((Map->Flags()&0x20u)!=0u && Map->MenuSelectedSprite()!=0)
                            AddEvent(IM_LBUTTONCLICK,0u);
                        else
                            AddEvent(IA_ANY_KEY,0u);
                    }
                }

                if(AbsInputAxis(m_joystickX)>100 || AbsInputAxis(m_joystickY)>100 ||
                   AbsInputAxis(m_joystickZ)>100 || AbsInputAxis(m_joystickR)>100)
                {
                    VECTOR2 next=m_clientMouseXY;
                    const unsigned int activePlayer=Map->PlayerIndex();
                    const bool tacticalPointerMode=(Map->Flags()&0x20u)==0u &&
                        !(Map->Player(static_cast<int>(activePlayer))->m_controlPauseState!=0u);

                    if(tacticalPointerMode)
                    {

                        if(Map->Flagman(activePlayer)!=0)
                        {
                            SPRITE* const playerSprite=Map->Flagman(Map->PlayerIndex());
                            const VECTOR2 origin=Map->ShiftXY();
                            const VECTOR2 base=playerSprite->Coor().ViewCoor()-origin;
                            if(AbsInputAxis(m_joystickZ)>100 || AbsInputAxis(m_joystickR)>100)
                            {
                                next.x=base.x+static_cast<float>(m_joystickZ)*0.7f;
                                next.y=base.y+static_cast<float>(m_joystickR)*0.5f;
                            }
                            else if(AbsInputAxis(m_joystickX)>100 || AbsInputAxis(m_joystickY)>100)
                            {

                                next.x=base.x+static_cast<float>(static_cast<int>(m_joystickX)/8);
                                next.y=base.y+static_cast<float>(static_cast<int>(m_joystickY)/8);
                            }
                        }
                    }
                    else
                    {
                        const VECTOR2 viewMin=VECTOR2(Graph->viewLeft,Graph->viewTop);
                        const VECTOR2 viewMax=VECTOR2(Graph->viewRight,Graph->viewBottom);
                        const double extentX=static_cast<double>(viewMax.x)-viewMin.x;
                        const float extentY=viewMax.y-viewMin.y;

                        if(m_joystickX>100 || AbsInputAxis(m_joystickY)>100)
                        {
                            next.x=static_cast<float>(viewMin.x+(static_cast<double>(m_joystickX)+1000.0)*0.0005f*extentX);
                            const float normalizedY=static_cast<float>(m_joystickY+1000)*0.0005f;
                            const float scaledY=normalizedY*extentY;
                            next.y=viewMin.y+scaledY;
                        }
                        else if(AbsInputAxis(m_joystickZ)>100 || AbsInputAxis(m_joystickR)>100)
                        {
                            next.x=static_cast<float>(viewMin.x+(static_cast<double>(m_joystickZ)+1000.0)*0.0005f*extentX);
                            const float normalizedY=static_cast<float>(m_joystickR+1000)*0.0005f;
                            const float scaledY=normalizedY*extentY;
                            next.y=viewMin.y+scaledY;
                        }
                    }
                    m_clientMouseXY=ChangeCoor(next);
                }
            }
        }
    }
#endif

    if(Map && Graph)
    {
        const VECTOR2 viewMin=VECTOR2(Graph->viewLeft,Graph->viewTop);
        const VECTOR2 viewMax=VECTOR2(Graph->viewRight,Graph->viewBottom);
        const float x=ClampViewportX(m_clientMouseXY.x,viewMin.x,viewMax.x);
        const double y=ClampViewportY(m_clientMouseXY.y,viewMin.y,viewMax.y);
        const VECTOR2 mapOrigin=Map->ShiftXY();
        const float dividedX=x/Graph->screenScaleX;
        const float offsetX=dividedX+Graph->viewportOffsetX;
        m_mouseXY.x=offsetX+mapOrigin.x;
        m_mouseXY.y=static_cast<float>(static_cast<double>(y)/Graph->screenScaleY+
            Graph->viewportOffsetY+mapOrigin.y);
    }
}

void INPUT::PlayFFB(int           )
{
}

void INPUT::StopFFB(int effect)
{
#if defined(_WIN32)
    if(!g_pJoystick || effect!=-1) return;
    const HRESULT hr=g_pJoystick->SendForceFeedbackCommand(DISFFC_STOPALL);
    if(FAILED(hr))
        InputReportError(E_ERROR,"Stop all feedBack",static_cast<unsigned int>(hr));
#else
    (void)effect;
#endif
}

void INPUT::SetInputType(int type)
{
    inputType = type;
    if (type == 0) FreeDirectInput();
    if (inputType == 1) Init(hWnd);
}

