

#include "mylib.h"
#include "engine_globals.h"
#include "basetype.h"
#include "mouse.h"
#include "sprite.h"
#include "vid.h"
#include "sprite_collector.h"
#include "map.h"
#include "graph.h"
#include "myerror.h"
#include <new>
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <windows.h>
#pragma pop_macro("INPUT")

namespace {
const char* const cursorFileNames[36] = {
    "arrow", "noammo", "move", "clash",
    "repair", "attack", "farattack", "select",
    "nomove", "cycle", "link", "unlink",
    "cantmove", "patrol", "delete", "capture",
    "mine", "unmine", "small - arrow", "small - noammo",
    "small - move", "small - taran", "small - repair", "small - attack",
    "small - farattack", "small - select", "small - nomove", "cycle",
    "small - link", "small - unlink", "small - cantmove", "small - patrol",
    "delete", "small - capture", "small - mine", "small - unmine"
};
}

MOUSE* Mouse=0;

MOUSE::MOUSE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : SPRITE(vid, position, direction, parent)
{
    for (unsigned int i = 0u;i<36u;++i) m_cursorHandles[i] = 0;
    m_enabled = 0u;
    m_hardwareCursor = 1u;

    if (m_vid != ::EmptyVid)
    {
        SPRITE* node = this;
        while (node)
        {
            SPRITE_COLLECTOR* const collector = ::Sprites;
            (void)collector->Delete(node);
            node = node->Link();
        }
    }
    ++m_referenceCount;
}

void MOUSE::Draw()
{
    if (m_hardwareCursor == 0u && m_enabled != 0u)
        m_vid->Draw(this);
}

MOUSE::~MOUSE()
{
    ::Error->Log("Mouse  release");
}

void MOUSE::Enable()
{
    if (m_enabled != 0u) return;
    m_enabled = 1u;
    (void)::SetCursor(0);
    if (m_hardwareCursor == 0u) return;

    for (unsigned int i = 0u;i<36u;++i)
    {
        HCURSOR* const slot = &m_cursorHandles[i];
        if (*slot)
        {
            (void)::DestroyCursor(*slot);
            *slot = 0;
        }
        const char* const source = cursorFileNames[i];
        if (source && source[0])
        {
            STRING path("cursores\\");
            path+=cursorFileNames[i];
            path+=".ani";
            *slot = ::LoadCursorFromFileA(path.value);
        }
        if (*slot == 0)
            *slot = ::LoadCursorA(0, MAKEINTRESOURCEA(32512));
    }
    const unsigned int selected = static_cast<unsigned int>(m_currentAnimation);
    (void)::SetCursor(m_cursorHandles[selected]);
}

void MOUSE::Disable()
{
    if (m_enabled == 0u) return;
    const unsigned int hardware = m_hardwareCursor;
    m_enabled = 0u;
    if (hardware == 0u) return;

    (void)::SetCursor(0);
    BOOL (WINAPI* const destroyCursor)(HCURSOR) = &::DestroyCursor;
    for (unsigned int i = 0;i<36u;++i)
    {
        const HCURSOR handle = m_cursorHandles[i];
        if (handle)
        {
            (void)destroyCursor(handle);
            m_cursorHandles[i] = 0;
        }
    }
}

void MOUSE::HardwareOn()
{
    if (m_hardwareCursor != 0u) return;
    Disable();
    m_vid = ::EmptyVid;
    m_hardwareCursor = 1u;
    Enable();
}

void MOUSE::HardwareOff()
{
    if (m_hardwareCursor == 0u) return;
    Disable();

    MAP* const world = Map;
    VID* selected = world->m_vidCount>1 ? world->m_vids[1] : 0;
    if (selected == 0)
        selected = ::EmptyVid;
    m_vid = selected;
    const unsigned int mode = static_cast<unsigned int>(m_currentAnimation);
    SPRITE::ChangeAnimation(mode == 0u?1:0);
    SPRITE::ChangeAnimation(static_cast<int>(mode));
    m_hardwareCursor = 0u;
    Enable();
}

void MOUSE::ChangeAnimation(int animation)
{
    const unsigned int mode = static_cast<unsigned int>(animation);
    if (m_hardwareCursor != 0u)
    {
        if (static_cast<unsigned int>(m_currentAnimation) != mode && m_enabled != 0u)
            (void)::SetCursor(m_cursorHandles[mode]);
        if (BitsToSigned32(mode)<17)
            SPRITE::ChangeAnimation(static_cast<int>(mode));
        else
            m_currentAnimation = static_cast<int>(mode);
        return;
    }
    SPRITE::ChangeAnimation(static_cast<int>(mode));
}

int MOUSE::Action(int action, int firstArgument, int secondArgument, int thirdArgument)
{
    if (action == ACT_CHANGE_ANIMATION)
    {
        ChangeAnimation(firstArgument);
        return 0;
    }
    if (action == ACT_CHANGE_COOR)
    {
        RECT windowRect;
        (void)::GetWindowRect(reinterpret_cast<HWND>(Graph->window), &windowRect);
        (void)::SetCursorPos(AddSigned32Wrap(windowRect.left, firstArgument),
            AddSigned32Wrap(windowRect.top, secondArgument));
        return 0;
    }
    if (action == ACT_CHANGE_VID)
    {
        const int index = firstArgument;
        if (index<0 || index >= Map->m_vidCount || !Map->m_vids[index])
            return 0;
        if (m_vid && m_vid->nVid == index) return 0;

        for (SPRITE* node = this;node;node = node->Link())
        {
            SPRITE_COLLECTOR* const collector = ::Sprites;
            collector->Insert(node);
        }
        (void)SPRITE::Action(ACT_CHANGE_VID, firstArgument, secondArgument, thirdArgument);
        for (SPRITE* node = this;node;node = node->Link())
        {
            SPRITE_COLLECTOR* const collector = ::Sprites;
            (void)collector->Delete(node);
        }
        return 0;
    }
    return SPRITE::Action(action, firstArgument, secondArgument, thirdArgument);
}


