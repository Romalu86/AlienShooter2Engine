#include "mylib.h"
#include "zBaseDlgMenu.h"
#include "sprite.h"

#include "basetype.h"


zBaseDlgMenu::zBaseDlgMenu()
    : m_enabled(1u), m_dispatchEnabled(1u)
{
}

bool  zBaseDlgMenu::IsVivible()
{
    return m_enabled != 0;
}

zBaseDlgMenu::~zBaseDlgMenu() noexcept(false)
{
}

void zBaseDlgMenu::operator delete(void* pointer)
{
    CrtFreePointer(pointer);
}


void*  zBaseDlgMenu::QueryInterface(int id) const
{
    void* self = const_cast<zBaseDlgMenu*>(this);
    if (id == 11u) return self;
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zBaseDlgMenu.cpp", 13u);
    return 0;
}

int  zBaseDlgMenu::iRelease()
{
    delete this;
    return 0;
}

void  zBaseDlgMenu::ControlInput(INPUT*)
{
}

void  zBaseDlgMenu::DeletePointerToSprite(SPRITE*)
{
}


void  zBaseDlgMenu::Control(INPUT* input)
{
    if (m_dispatchEnabled && input) ControlInput(input);
}

bool zBaseDlgMenu::SpriteInArray(SPRITE* sprite, SPRITE** sprites, int spriteCount)
{
    for (int spriteIndex = 0; spriteIndex<spriteCount; ++spriteIndex) if (sprites[spriteIndex] == sprite) return true;
    return false;
}

bool  zBaseDlgMenu::Show(bool show)
{
    if ((m_enabled != 0) == show) return false;
    m_enabled = show?1u:0u;
    return true;
}


bool  zBaseDlgMenu::SetInputActive(bool inputActive)
{
    if ((m_dispatchEnabled != 0) == inputActive) return false;
    m_dispatchEnabled = inputActive?1u:0u;
    return true;
}

#include "map.h"


static double MenuY(const SPRITE* sprite)
{
    const VECTOR& position = sprite->Coor();
    return static_cast<double>(position.y)-static_cast<double>(position.z)-static_cast<double>(Map->ShiftXY().y);
}
static double MenuX(const SPRITE* sprite)
{
    return static_cast<double>(sprite->Coor().x)-static_cast<double>(Map->ShiftXY().x);
}


void zBaseDlgMenu::Sort(SPRITE** items, int spriteCount)
{
    for (int sortedEndIndex = spriteCount-1; sortedEndIndex >= 1; --sortedEndIndex)
    {
        int selectedIndex = sortedEndIndex;
        for (int candidateIndex = 0; candidateIndex<sortedEndIndex; ++candidateIndex)
        {
            const double candidateY = MenuY(items[candidateIndex]), selectedY = MenuY(items[selectedIndex]);

            if (!(selectedY >= candidateY) || (candidateY == selectedY && !(MenuX(items[selectedIndex]) >= MenuX(items[candidateIndex])))) selectedIndex = candidateIndex;
        }
        SPRITE* savedSprite = items[sortedEndIndex];
        items[sortedEndIndex] = items[selectedIndex];
        items[selectedIndex] = savedSprite;
    }
}


void zBaseDlgMenu::SortByX(SPRITE** items, int spriteCount)
{
    for (int sortedEndIndex = spriteCount-1; sortedEndIndex >= 1; --sortedEndIndex)
    {
        int selectedIndex = sortedEndIndex;
        for (int candidateIndex = 0; candidateIndex<sortedEndIndex; ++candidateIndex)
        {
            const double candidateX = MenuX(items[candidateIndex]), selectedX = MenuX(items[selectedIndex]);

            if (candidateX>selectedX || ((!(candidateX<selectedX) && !(candidateX>selectedX)) && !(MenuY(items[selectedIndex]) >= MenuY(items[candidateIndex])))) selectedIndex = candidateIndex;
        }
        SPRITE* savedSprite = items[sortedEndIndex];
        items[sortedEndIndex] = items[selectedIndex];
        items[selectedIndex] = savedSprite;
    }
}


