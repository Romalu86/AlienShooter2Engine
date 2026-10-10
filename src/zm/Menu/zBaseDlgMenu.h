#pragma once
#include <stddef.h>
class SPRITE;
class INPUT;

class iBaseDlgMenu
{
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void Control(INPUT* input) = 0;
    virtual void ControlInput(INPUT* input) = 0;
    virtual void DeletePointerToSprite(SPRITE* sprite) = 0;
    virtual bool Show(bool show) = 0;
    virtual bool SetInputActive(bool inputActive) = 0;
    virtual bool IsVivible() = 0;
};

class zBaseDlgMenu : public iBaseDlgMenu {
public:
    zBaseDlgMenu();
    virtual void* QueryInterface(int id) const;
    virtual int iRelease();
    virtual void Control(INPUT* input);
    virtual void ControlInput(INPUT* input);
    virtual void DeletePointerToSprite(SPRITE* sprite);
    virtual bool Show(bool show);
    virtual bool SetInputActive(bool inputActive);
    virtual bool IsVivible();
    virtual ~zBaseDlgMenu() noexcept(false);
    static void operator delete(void* pointer);

    bool SpriteInArray(SPRITE* sprite, SPRITE** sprites, int spriteCount);
    void Sort(SPRITE** items, int spriteCount);
    void SortByX(SPRITE** items, int spriteCount);

protected:
    unsigned char m_enabled;
    unsigned char m_dispatchEnabled;
    unsigned char m_006[2];
};
