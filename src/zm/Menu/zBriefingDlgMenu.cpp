#include "mylib.h"
#include "zm/Menu/zBriefingDlgMenu.h"
#include "vid.h"
#include "irpg.h"
#include "sprite.h"
#include <new>

#include "basetype.h"


void zBriefingDlgMenu::ReloadAll()
{
    m_captionSprite = 0;
    m_bodySprite = 0;
}


void  zBriefingDlgMenu::SetRPGFrameWork(iRPGFrameWork* frameWork)
{
    m_framework = frameWork;
}


void*  zBriefingDlgMenu::QueryInterface(int id) const
{
    void* self = const_cast<zBriefingDlgMenu*>(this);
    if (id == 15u) return self;
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zBriefingDlgMenu.cpp", 22u);
    return 0;
}

int  zBriefingDlgMenu::iRelease()
{
    delete static_cast<zBaseDlgMenu*>(this);
    return 0;
}

void  zBriefingDlgMenu::DeletePointerToSprite(SPRITE* spriteToClear)
{
    if (spriteToClear == m_captionSprite || spriteToClear == m_bodySprite) ReloadAll();
}
#include "map.h"


zBriefingDlgMenu::~zBriefingDlgMenu()
{
}

zBriefingDlgMenu::zBriefingDlgMenu()
    : zBaseDlgMenu(), m_captionSprite(0), m_bodySprite(0), m_framework(0)
{
}


void  zBriefingDlgMenu::Control(INPUT* input)
{
    (void) input;
    if (m_captionSprite && m_bodySprite) return;
    TryToFindButtons();

    iRPGObj* levelEntry = m_framework->GetObjByNamee("Lvl_01", true);
    STRING briefingText;
    if (m_captionSprite)(void) m_captionSprite->Action(ACT_SET_TEXT, PtrToU32(&briefingText), 0, 0);
    if (levelEntry)
    {
        const char* description = m_framework->AsTextDB()->GetObjInfo(levelEntry, true);
        if (description) briefingText = description;
    }
    if (m_bodySprite)(void) m_bodySprite->Action(ACT_SET_TEXT, PtrToU32(&briefingText), 0, 0);
}


void __cdecl GetNew_zBriefingDlgMenu(iBriefingDlgMenu** outObject)
{
    zBriefingDlgMenu* object = static_cast<zBriefingDlgMenu*>(CrtNew(sizeof(zBriefingDlgMenu)));
    if (object)
    {
        try { new (object) zBriefingDlgMenu(); }
        catch (...) { CrtFreePointer(object); throw; }
    }
    *outObject = object;
}


void  zBriefingDlgMenu::TryToFindButtons()
{
    MAP* const map = Map;
    const int menuSpriteCount = map->MenuState().m_count;
    SPRITE** const menuSprites = map->MenuState().m_data;
    for (int menuSpriteIndex = 0; menuSpriteIndex<menuSpriteCount; ++menuSpriteIndex)
    {
        SPRITE* const candidateSprite = menuSprites[menuSpriteIndex];
        VID* const candidateVid = candidateSprite->Vid();
        const unsigned int vidId = static_cast<unsigned int>(candidateVid->nVid);
        const unsigned char direction = candidateSprite->Direction().value;
        if (!m_captionSprite && vidId == 4u && candidateVid->RealDirection(direction) == 1)
        {
            m_captionSprite = candidateSprite;
            continue;
        }
        if (!m_bodySprite && vidId == 3u && candidateVid->RealDirection(direction) == 1)
            m_bodySprite = candidateSprite;
    }
}



