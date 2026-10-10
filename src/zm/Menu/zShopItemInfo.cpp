#include "mylib.h"

#include "zShopItemInfo.h"
#include "sprite.h"
#include "frame.h"
#include "zm/RPGHero/zRPGAmmo.h"
#include "vid.h"
#include "map.h"
#include "mouse.h"
#include "zm/RPGHero/zRPGWeapon.h"
#include "zm/Eng/zProfLdr/zProfMngr.h"
#include <new>

#include "basetype.h"
#include "irpg.h"

void zShopItemInfo::SetShowAmmo(bool showAmmo)
{
    m_showAmmo = showAmmo;
}

void  zShopItemInfo::SetInventoryKeeper(iInvKeeper* inventoryKeeper)
{
    m_inventoryKeeper = inventoryKeeper;
}

void  zShopItemInfo::SetRPGFrameWork(iRPGFrameWork* frameWork)
{
    m_framework = frameWork;
}

int  zShopItemInfo::GetCurPageNum()
{
    return m_curPage;
}


void  zShopItemInfo::ShowObjInfo(iRPGObj* object, bool notify, iBaseDlgMenu* dialog)
{
    m_curPage = 0;
    ShowObjInfo(object, notify, dialog, false);
}

void  zShopItemInfo::Refresh()
{
    ShowObjInfo(m_currentItem.m_object, false, m_dialog, true);
}

void*  zShopItemInfo::QueryInterface(int id) const
{
    void* self = const_cast<zShopItemInfo*>(this);
    if (id == 27u) return self;
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopItemInfo.cpp", 62u);
    return 0;
}

int  zShopItemInfo::iRelease()
{
    if (this)
    {
        this->~zShopItemInfo();
        zBaseDlgMenu::operator delete(this);
    }
    return 0;
}

iRPGObj*  zShopItemInfo::GetItemForPut()
{
    iRPGObj* const item = m_itemForPut.m_object;
    iRPGObj* const current = m_currentItem.m_object;
    if (item && !item->IsInfinite()) return m_itemForPut.m_object;
    return current;
}

void  zShopItemInfo::SetMaxCoords(int maxX, int maxY)
{
    m_maxX = maxX;
    m_maxY = maxY;
    m_limitCoords = maxX != 0 || maxY != 0;
}

void zShopItemInfo::PageUp()
{
    if (m_curPage>0) --m_curPage;
    Refresh();
}

void zShopItemInfo::DeletePointerToSprite(SPRITE* spriteToClear)
{
    for (unsigned int spriteIndex = 0; spriteIndex<5u; ++spriteIndex)
        if (m_sprites[spriteIndex] == spriteToClear) m_sprites[spriteIndex] = 0;
}

void zShopItemInfo::PageDown()
{
    const int page = m_curPage;
    const int lastPage = SubSigned32Wrap(GetPagesCnt(), 1);
    if (page<lastPage) m_curPage = AddSigned32Wrap(page, 1);
    Refresh();
}

void  zShopItemInfo::Initialize()
{
    m_currentItem.Set(0);
    m_ammoRef.Set(0);
    m_itemForPut.Set(0);
    m_curPage = 0;
}

zShopItemInfo::zShopItemInfo()
    : zBaseDlgMenu(), m_limitCoords(0), m_framework(0), m_inventoryKeeper(0), m_dialog(0),
      m_screenX(0), m_screenY(0), m_maxX(0), m_maxY(0), m_showAmmo(1),
      m_curPage(0), m_pagesCount(0), m_pageTop(0.0f), m_074(0)
{
    for (unsigned int spriteIndex = 0; spriteIndex<5u; ++spriteIndex) m_sprites[spriteIndex] = 0;
    m_currentItem.m_object = 0;
    m_currentItem.m_refObject = 0;
    m_currentItem.m_owner = 0;
    m_ammoRef.m_object = 0;
    m_ammoRef.m_refObject = 0;
    m_ammoRef.m_owner = 0;
    m_itemForPut.m_object = 0;
    m_itemForPut.m_refObject = 0;
    m_itemForPut.m_owner = 0;
}

void  zShopItemInfo::SetScreenCoordinates(float x, float y)
{
    m_screenX = TruncateFloatToInt32(x);
    m_screenY = TruncateFloatToInt32(y);
}

int  zShopItemInfo::GetPagesCnt()
{
    if (m_pagesCount>0) return m_pagesCount;
    int pages = 1;
    SPRITE* textSprite = m_sprites[0];
    if (textSprite)
    {
        const double sum = static_cast<double>(static_cast<STEXT*>(textSprite)->m_textHeight)+m_pageTop;
        if (sum>(483.0f)) pages = 2;
    }
    m_pagesCount = pages;
    return pages;
}

void  zShopItemInfo::ShowObjInfo(iRPGObj* item, bool notify, iBaseDlgMenu* dialog, bool force)
{
    m_dialog = dialog;
    iRPGObj* const previousItem = m_currentItem.m_object;
    if (previousItem == item && !force) return;
    if (previousItem != item) m_pagesCount = 0;
    if (notify)
    {
        if (!m_framework) DebugAssert(5u, "m_pRPGFrameWork", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopItemInfo.cpp", 243u);
        m_framework->PlaySFXCurArmy(47);
    }
    m_currentItem.Set(item);
    if (!m_currentItem.m_object) DeleteSprites();
    else Draw();
}


zShopItemInfo::~zShopItemInfo()
{
}

void  zShopItemInfo::Control(INPUT* input)
{
    const double ammoCenterX = static_cast<double>(m_screenX)+205.0;
    const float ammoCenterY = static_cast<float>(static_cast<double>(m_screenY)+25.0);
    const VECTOR2& point = input->ClientMouseXY();
    const float margin = (20.0f);
    iRef<iRPGObj>& itemForPut = m_itemForPut;
    const bool inside = (point.x >= ammoCenterX-margin && point.x <= ammoCenterX+margin &&
        point.y >= static_cast<double>(ammoCenterY)-margin && point.y <= static_cast<double>(ammoCenterY)+margin);
    if (!inside)
    {
        itemForPut.Set(0);
        return;
    }
    iRPGAmmo* const source = m_ammoRef.m_object;
    iRPGObj* const target = source
        ? static_cast<zRPGObj*>(static_cast<zRPGAmmo*>(source)) : 0;
    itemForPut.Set(target);
}


void  zShopItemInfo::GetCoordToOutput(int& x, int& y)
{
    x = m_screenX;
    y = m_screenY;
    if (m_maxX != 0 && x>m_maxX) x = m_maxX;
    if (m_maxY != 0 && y>m_maxY) y = m_maxY;
}


void __cdecl GetNew_zShopItemInfo(iShopItemInfo** outObject)
{
    void* const storage = CrtNew(sizeof(zShopItemInfo));
    zShopItemInfo* object = 0;
    if (storage)
    {
        try
        {
            object = ::new (storage) zShopItemInfo();
        }
        catch (...)
        {
            CrtFreePointer(storage);
            throw;
        }
    }
    *outObject = object;
}

void  zShopItemInfo::DeleteSprites()
{
    if (m_sprites[2])
    {
        delete m_sprites[2];
        m_sprites[2] = 0;
    }
    if (m_sprites[3])
    {
        delete m_sprites[3];
        m_sprites[3] = 0;
    }
    if (m_sprites[0])
    {
        delete m_sprites[0];
        m_sprites[0] = 0;
    }
    if (m_sprites[1])
    {
        delete m_sprites[1];
        m_sprites[1] = 0;
    }

    SPRITE* const special = m_sprites[4];
    if (!special) return;
    delete special;
    m_sprites[4] = 0;

    MOUSE* const mouse = Mouse;
    if (mouse->m_enabled == 0u) mouse->Enable();
    Mouse->ChangeAnimation(0);
}

void  zShopItemInfo::ShowAmmo(int x, int y, bool releaseOnly)
{
    if (!releaseOnly)
    {
        iRPGObj* const object = m_currentItem.m_object;
        if (object->GetGameObj() == TYPE_WEAPON && m_showAmmo != 0u)
        {
            void* const weaponInterface = m_currentItem.m_object->QueryInterface(50);
            zRPGAmmo* const ammo = static_cast<zRPGWeapon*>(weaponInterface)->GetAmmo();
            m_ammoRef.Set(ammo);

            iRPGAmmo* const referencedAmmo = m_ammoRef.m_object;
            if (referencedAmmo)
            {
                iRPGObj* ammoObject = static_cast<zRPGObj*>(static_cast<zRPGAmmo*>(referencedAmmo));
                if (!ammoObject->IsInfinite())
                {
                    ammoObject = static_cast<zRPGObj*>(static_cast<zRPGAmmo*>(m_ammoRef.m_object));
                    const int vidIndex = ammoObject->GetVidNum(2);
                    ammoObject = static_cast<zRPGObj*>(static_cast<zRPGAmmo*>(m_ammoRef.m_object));
                    const int realDirection = ammoObject->GetDirNum(2);

                    SPRITE* sprite = m_sprites[3];
                    if (!sprite)
                    {
                        const ANGLE direction(0u);
                        const VECTOR origin(0, 0, 0);
                        sprite = Map->CreateSprite(Map->Vid(vidIndex), origin, direction, 0);
                        m_sprites[3] = sprite;
                    }

                    sprite = m_sprites[3];
                    if (sprite)
                    {
                        (void) sprite->Action(ACT_CHANGE_VID, vidIndex, -1, 0);
                        sprite = m_sprites[3];
                        VID* const spriteVid = sprite->Vid();
                        sprite->ChangeDirection(ANGLE(static_cast<unsigned char>((static_cast<unsigned int>(realDirection)<<8)/static_cast<unsigned int>(spriteVid->noDir))));
                        sprite = m_sprites[3];

                        const float baseY = static_cast<float>(static_cast<double>(y)+4029.0);
                        const VECTOR position(
                            static_cast<float>(static_cast<double>(x)+205.0+Map->m_shiftXY.x),
                            static_cast<float>(static_cast<double>(baseY)+Map->m_shiftXY.y), 4004.0f);
                        sprite->ChangeCoor(position);
                        return;
                    }
                }
            }
        }
    }

    SPRITE* const sprite = m_sprites[3];
    if (sprite)
    {
        delete sprite;
        m_sprites[3] = 0;
    }
}

void  zShopItemInfo::ShowImage(int x, int y, bool releaseOnly)
{
    SPRITE* sprite = m_sprites[2];
    if (releaseOnly)
    {
        if (sprite)
        {
            delete sprite;
            m_sprites[2] = 0;
        }
        return;
    }

    const int vidIndex = m_currentItem.m_object->GetVidNum(2)-1;
    const int realDirection = m_currentItem.m_object->GetDirNum(2);
    sprite = m_sprites[2];
    if (!sprite)
    {
        const ANGLE direction(0u);
        const VECTOR origin(0, 0, 0);
        sprite = Map->CreateSprite(Map->Vid(vidIndex), origin, direction, 0);
        m_sprites[2] = sprite;
    }
    sprite = m_sprites[2];
    if (!sprite) return;

    (void) sprite->Action(ACT_CHANGE_VID, vidIndex, -1, 0);
    sprite = m_sprites[2];
    VID* spriteVid = sprite->Vid();
    sprite->ChangeDirection(ANGLE(static_cast<unsigned char>((static_cast<unsigned int>(realDirection)<<8)/static_cast<unsigned int>(spriteVid->noDir))));

    sprite = m_sprites[2];
    spriteVid = sprite->Vid();
    const int halfWidth = static_cast<int>(spriteVid->vidSizeX)/2;
    const int halfHeight = static_cast<int>(spriteVid->vidSizeY)/2;

    const float baseY = static_cast<float>(static_cast<double>(y)+3980.0);
    const float shiftedY = static_cast<float>(static_cast<double>(baseY)+Map->m_shiftXY.y);
    const VECTOR position(
        static_cast<float>(static_cast<double>(x)-3.0+Map->m_shiftXY.x+halfWidth),
        static_cast<float>(static_cast<double>(shiftedY)+halfHeight), 4004.0f);
    sprite->ChangeCoor(position);

    GAMMA gamma;
    gamma.modulate = 0u;
    gamma.add = 0u;
    iInvKeeper* const keeper = m_inventoryKeeper;
    if (keeper && !keeper->CanHoldObj(m_currentItem.m_object))
    {
        int red;
        int green;
        int blue;
        m_framework->GetGammaItemDisabled(&red, &green, &blue);

        int* const values[3] = {&red, &green, &blue};
        const unsigned int shifts[3] = {16u, 8u, 0u};
        for (unsigned int gammaComponent = 0; gammaComponent<3u; ++gammaComponent)
        {
            int value = *values[gammaComponent];
            if (value<-255) value = -255;
            if (value>255) value = 255;
            if (value<0) gamma.modulate |= static_cast<unsigned int>(-value)<<shifts[gammaComponent];
            else gamma.add |= static_cast<unsigned int>(value)<<shifts[gammaComponent];
        }
    }
    m_sprites[2]->SetGamma(gamma);
}

void zShopItemInfo::Draw()
{
    iRPGObj* const item = m_currentItem.m_object;
    if (!item)
        DebugAssert(5u, "m_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zShopItemInfo.cpp", 456u);

    int x = 0;
    int y = 0;
    GetCoordToOutput(x, y);

    m_ammoRef.Set(0);

    const unsigned char alternatePage = (m_curPage != 0 && m_limitCoords == 0u)?1u:0u;
    ShowAmmo(x, y, alternatePage != 0u);
    const bool imageAlternatePage = m_curPage != 0 && m_limitCoords == 0u;
    ShowImage(x, y, imageAlternatePage);

    STRING text;
    iRPGFrameWork* const framework = m_framework;
    iRPGObj* const descriptionItem = m_currentItem.m_object;
    const char* const description = framework->AsTextDB()->GetObjInfo(descriptionItem, true);
    if (description)
        text = description;

    SPRITE* textSprite = m_sprites[0];
    if (!textSprite)
    {
        const ANGLE direction(0u);
        const VECTOR origin(0, 0, 0);
        textSprite = Map->CreateSprite(Map->Vid(3), origin, direction, 0);
        m_sprites[0] = textSprite;
    }

    textSprite = m_sprites[0];
    (void) textSprite->Action(ACT_SET_TEXT, static_cast<int>(PtrToU32(&text)), 0, 0);
    textSprite = m_sprites[0];
    (void) textSprite->Action(ACT_SET_BEHAVE, 0, 0, 0);

    const float screenX = static_cast<float>(static_cast<double>(x)+1.0);
    double objectY = static_cast<float>(y);
    const unsigned int type0 = static_cast<unsigned int>(m_currentItem.m_object->GetGameObj());
    if (type0 == 3u)
        objectY -= (9.0f);
    else
    {
        const unsigned int type1 = static_cast<unsigned int>(m_currentItem.m_object->GetGameObj());
        if (type1 == 6u)
            objectY -= (16.0f);
        else
        {
            const unsigned int type2 = static_cast<unsigned int>(m_currentItem.m_object->GetGameObj());
            objectY -= (type2 == 5u)?(16.0f):(32.0f);
        }
    }
    m_pageTop = static_cast<float>(objectY);

    const float mapX = Map->m_shiftXY.x;
    const float mapY = Map->m_shiftXY.y;
    float textY = static_cast<float>(objectY+4005.0+mapY);
    const float textX = static_cast<float>(static_cast<double>(screenX)+mapX);

    SPRITE* const imageSprite = m_sprites[2];
    if (imageSprite)
    {
        VID* const imageInfo = imageSprite->Vid();
        const float imageHeight = imageInfo->sizeXYZ.y;
        textY = static_cast<float>(static_cast<double>(textY)+imageHeight);
        m_pageTop = static_cast<float>(objectY+imageHeight);
    }

    const VECTOR textPosition(textX, textY, 4005.0f);
    textSprite = m_sprites[0];
    textSprite->ChangeCoor(textPosition);
    textSprite = m_sprites[0];
    (void) textSprite->Action(ACT_SET_TEXT_ROW, 0, 9999, 0);

    if (m_curPage == 0 && GetPagesCnt()>1 && m_limitCoords == 0u)
    {
        const int visibleHeight = TruncateDoubleToInt32(483.0-static_cast<double>(m_pageTop));
        const int rowCount = visibleHeight/14;
        (void) m_sprites[0]->Action(ACT_SET_TEXT_ROW, 0, rowCount, 0);
    }
    else
        (void) m_sprites[0]->Action(ACT_SET_TEXT_ROW, 0, 9999, 0);

    if (m_limitCoords != 0u)
    {
        SPRITE* pageSprite = m_sprites[4];
        if (!pageSprite)
        {
            const ANGLE direction(0u);
            const VECTOR origin(0, 0, 0);
            pageSprite = Map->CreateSprite(Map->Vid(2202), origin, direction, 0);
            m_sprites[4] = pageSprite;
        }

        pageSprite = m_sprites[4];
        if (pageSprite)
        {
            VID* const spriteInfo = pageSprite->Vid();
            const float baseX = static_cast<float>(static_cast<double>(x)+Map->m_shiftXY.x);
            const float baseY = static_cast<float>(static_cast<float>(y)+static_cast<double>(Map->m_shiftXY.y));
            const float pageX = static_cast<float>(static_cast<double>(spriteInfo->sizeXYZ.x)*0.5+baseX+10.0);
            const float pageY = static_cast<float>(static_cast<double>(spriteInfo->sizeXYZ.y)*0.5+baseY+4003.0);
            const VECTOR pagePosition(pageX, pageY, 4003.0f);
            pageSprite->ChangeCoor(pagePosition);
            Mouse->Disable();
        }
    }
    else
    {
        SPRITE* const pageSprite = m_sprites[4];
        if (pageSprite)
        {
            delete pageSprite;
            m_sprites[4] = 0;
        }
    }
}


