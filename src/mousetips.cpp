
#include "mousetips.h"
#include "engine_globals.h"
#include "input.h"
#include "map.h"
#include "graph.h"
#include "sprite.h"
#include "vid.h"
#include "mylib.h"
#include "basetype.h"
#include "constant.h"
#include "zm/Utils/zStaticStorage.h"
#include <new>
#include <string.h>

namespace {

unsigned int lastMouseMoveTime = 0;
float lastMouseX = 0.0f;
float lastMouseY = 0.0f;
unsigned char mouseTipsInitialized = 0;
StaticObjectStorage<STRING> cachedMouseTipText;

void __cdecl MouseTipsStringCleanup()
{
    cachedMouseTipText.Get().~STRING();
}

void __cdecl MouseTipsStaticNoOp()
{
}

void EnsureMouseTipsStatics()
{
    unsigned char guard = mouseTipsInitialized;
    if ((guard&0x01u) == 0u)
    {
        guard = static_cast<unsigned char>(guard|0x01u);
        mouseTipsInitialized = guard;
        (void)RegisterCrtAtExit(&MouseTipsStaticNoOp);
    }

    guard = mouseTipsInitialized;
    if ((guard&0x02u) == 0u)
    {
        ::new (cachedMouseTipText.Address()) STRING;
        guard = static_cast<unsigned char>(guard|0x02u);
        mouseTipsInitialized = guard;
        (void)RegisterCrtAtExit(&MouseTipsStringCleanup);
    }
}

}

void MOUSETIPS::Tact(const INPUT* input)
{
    EnsureMouseTipsStatics();

    const unsigned int now = ::RealCurrentTime;
    const float mouseX = input->ClientMouseXY().x;
    const float mouseY = input->ClientMouseXY().y;

    if ((lastMouseX<mouseX || lastMouseX>mouseX) ||
       (lastMouseY<mouseY || lastMouseY>mouseY))
    {
        lastMouseMoveTime = now;
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    const unsigned int lastTick = lastMouseMoveTime;
    const unsigned int delay = static_cast<unsigned int>(Const->MouseTipsTime);
    const unsigned int elapsed = now-lastTick;
    if (elapsed <= delay)
    {
        Clear();
        return;
    }

    if (input->GetAction(IM_LBUTTONCLICK) != 0 || input->GetAction(IM_KEYDOWN) != 0)
    {
        Clear();
        return;
    }

    MAP* const map = ::Map;
    if ((map->MenuState().ControlFlags()&0x01u) != 0u || (map->Flags()&0x00200000u) == 0u)
    {
        Clear();
        return;
    }

    SPRITE* sprite = tip;
    if (!sprite)
    {
        VID* tooltipVid = 0;
        if (map->VidCount()>15)
            tooltipVid = map->StoredVid(15);
        if (!tooltipVid)
            tooltipVid = ::EmptyVid;

        STRING* const cached = cachedMouseTipText.Address();
        {
            const STRING queried = map->GetMouseTipsString();
            *cached = queried;
        }

        if (tooltipVid == ::EmptyVid)
            return;
        if (strcmp(cached->m_value, StringGlobals::EmptyText()) == 0)
            return;

        VECTOR position(input->ClientMouseXY().x+20.0f, input->ClientMouseXY().y+4008.0f, 4000.0f);
        sprite = Map->CreateSprite(tooltipVid, position, ANGLE(static_cast<unsigned char>(0u)), 0);
        tip = sprite;
        if (!sprite)
            return;

        const STRING ownedText(cached->value);
        (void)sprite->Action(ACT_SET_TEXT, static_cast<int>(PtrToU32(&ownedText)), 0, 0);

        sprite = tip;
        const double width = static_cast<double>(ReadS32(sprite, 116u));
        float x = position.x;
        double y = position.y;
        if (!(static_cast<double>(Graph->ViewXMax())>static_cast<double>(x)+width))
        {
            x = static_cast<float>(static_cast<double>(Graph->ViewXMax())-width);
            y+=10.0;
        }

        const float halfHeight = tooltipVid->sizeXYZ.y*0.5f;
        if (static_cast<double>(Graph->ViewYMin()) >= static_cast<double>(halfHeight)+y-4000.0)
            y = static_cast<double>(halfHeight)+input->ClientMouseXY().y+4010.0;

        MAP* const currentMap = Map;
        VECTOR shifted(static_cast<float>(static_cast<double>(x)+currentMap->ShiftXY().x),
                       static_cast<float>(y+currentMap->ShiftXY().y), 4000.0f);
        sprite->ChangeCoor(shifted);
        return;
    }


    const unsigned int currentDelay = static_cast<unsigned int>(Const->MouseTipsTime);
    const unsigned int currentLastTick = lastMouseMoveTime;
    if (::RealCurrentTime-currentLastTick <= currentDelay+500u)
        return;

    lastMouseMoveTime = currentLastTick+500u;
    bool changed;
    {
        const STRING current = map->GetMouseTipsString();
        const char* const cachedText = cachedMouseTipText.Get().value;
        changed = strcmp(current.value, cachedText) != 0;
    }
    if (changed)
        Clear();
}

void MOUSETIPS::Clear()
{
    delete tip;
    tip = 0;
}

void MOUSETIPS::DeletePointerToSprite(SPRITE* sprite)
{
    if (tip == sprite)
        tip = 0;
}

