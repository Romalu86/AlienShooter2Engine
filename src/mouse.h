#pragma once
#include "mylib.h"
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <windows.h>
#pragma pop_macro("INPUT")

#include "basetype.h"
#include "sprite.h"

class VID;
struct VECTOR;
class ANGLE;

class MOUSE : public SPRITE
{
public:
    MOUSE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent);
    virtual ~MOUSE();
    virtual void Draw();
    virtual int Action(int action, int firstArgument, int secondArgument, int thirdArgument);

    void Enable();
    void HardwareOff();
    void HardwareOn();
    void Disable();
    void ChangeAnimation(int animation);
    void ChangeDirection(ANGLE direction);
    unsigned int HardwareCursorEnabled() const
    {
        return m_hardwareCursor;
    }
    unsigned int Enabled() const
    {
        return m_enabled;
    }

private:
    friend class MAP;
    friend class zShopItemInfo;
    unsigned int m_hardwareCursor;
    HCURSOR m_cursorHandles[36];
    unsigned int m_enabled;
};
