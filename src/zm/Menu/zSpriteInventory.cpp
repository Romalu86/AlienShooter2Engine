
#include "zm/Utils/zLogConsole.h"
#include "../../engine_globals.h"
#include "basetype.h"
#include "zSpriteInventory.h"
#include <new>
#include "sprite.h"
#include "vid.h"
#include "zm/Inventory/zInventory.h"
#include "zm/Inventory/zInventory2D.h"
#include "zm/Inventory/zInventory3D.h"
#include "base_graph.h"
#include "mylib.h"
#include "zm/Inventory/zInventoryItem.h"
#include "zm/RPGHero/zRPGWeapon.h"
#include "zm/RPGHero/zRPGAmmo.h"
#include "zm/Survive/zSurvive.h"

void zSpriteInventory_SpriteVectorInsertFill(zSpritePointerVector* self, SPRITE** position, unsigned int count, SPRITE* const* value);


void zSpriteInventory::SetSpriteZ(int spriteZ)
{
    m_spriteZ = spriteZ;
}

void zSpriteInventory::ShowCost(bool showCost)
{
    m_showCost = showCost?1u:0u;
}

void zSpriteInventory::operator delete(void* pointer)
{
    CrtFreePointer(pointer);
}


int  zSpriteInventory::iRelease()
{
    if (this)
    {
        this->~zSpriteInventory();
        zSpriteInventory::operator delete(this);
    }
    return 0;
}


void  zSpriteInventory::SetDrawCellsType(int type)
{
    m_drawCellsType = type;
    ReloadAll();
}


void  zSpriteInventory::SetRPGHero(iRPGHero* hero)
{
    m_rpgHero = hero;
    ReloadAll();
}


void  zSpriteInventory::SetGetDirtyInventory(iInventory* inventory)
{
    m_dirtyInventory = static_cast<zInventory*>(inventory->QueryInterface(46));
}
#include "map.h"


void*  zSpriteInventory::QueryInterface(int id) const
{
    if (id == 26u) return const_cast<zSpriteInventory*>(this);
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 45u);
    return 0;
}


void  zSpriteInventory::HideHighLight()
{
    if (m_highLightSprite)
        (void) m_highLightSprite->Action(ACT_SET_INVISIBLE, 1, 0, 0);
    m_highlightFirstX = m_highlightFirstY = m_highlightLastX = m_highlightLastY = -1;
}

void zSpriteInventory::ClearDirty()
{
    if (m_inventory)
    {
        zInventory* inventory = m_inventory;
        (void) inventory->GetDirty();
        (void) inventory->GetAmmoDirty();
    }
}

void  zSpriteInventory::ReloadAll()
{
    ClearSprites();
    ClearCells();
    delete m_highLightSprite;
    m_highLightSprite = 0;
}

zSpriteInventory::zSpriteInventory(iRPGFrameWork* constructorArg)
{
    const unsigned char allocatorState = static_cast<unsigned char>(PtrToU32(constructorArg));
    m_inventory = 0;
    m_inventory2D = 0;
    m_inventory3D = 0;
    m_rpgHero = 0;
    m_framework = constructorArg;
    for (unsigned int vectorIndex = 0; vectorIndex<6u; ++vectorIndex)
    {
        m_spriteVectors[vectorIndex].m_allocatorState = allocatorState;
        m_spriteVectors[vectorIndex].m_begin = 0;
        m_spriteVectors[vectorIndex].m_end = 0;
        m_spriteVectors[vectorIndex].m_capEnd = 0;
    }
    m_drawCellsType = 0;
    m_spriteZ = 0;
    m_highLightSprite = 0;
    m_highlightFirstX = m_highlightFirstY = m_highlightLastX = m_highlightLastY = -1;
    m_dirtyInventory = 0;
    m_showCost = 0u;
}




void  zSpriteInventory::SetInventory(iInventory* inventory)
{
    if (m_inventory == inventory)
        return;

    m_inventory = static_cast<zInventory*>(inventory->QueryInterface(46));
    m_inventory2D = 0;
    m_inventory3D = 0;
    if (m_inventory)
    {
        m_inventory2D = static_cast<zInventory2D*>(m_inventory->QueryInterface(47));

        m_inventory3D = static_cast<zInventory3D*>(m_inventory->QueryInterface(48));
    }
    ReloadAll();
}


void  zSpriteInventory::ControlPlayerMenu(INPUT* input)
{
    if (!m_inventory)
    {
        (void) input;
        return;
    }

    if (m_inventory->GetDirtyNotErase())
    {
        RefreshAll();
        (void) input;
        return;
    }

    if (m_inventory->GetAmmoDirtyNotErase())
    {
        RefreshAll();
        (void) input;
        return;
    }

    if (m_dirtyInventory && m_dirtyInventory->GetDirtyNotErase())
    {
        RefreshAll();
        (void) input;
        return;
    }

    if (m_dirtyInventory && m_dirtyInventory->GetAmmoDirtyNotErase())
        RefreshAll();
    (void) input;
}

zSpritePointerVector::zSpritePointerVector()
    : m_allocatorState(0), m_begin(0), m_end(0), m_capEnd(0)
{
}

zSpritePointerVector::~zSpritePointerVector() noexcept(false)
{
    CrtFreePointer(m_begin);
    m_begin = 0;
    m_end = 0;
    m_capEnd = 0;
}

zSpriteInventory::~zSpriteInventory() noexcept(false)
{
    ReloadAll();
}

bool  zSpriteInventory::HighLightItem(const iRPGObj* selector)
{
    if (selector == 0)
    {
        HideHighLight();
        return false;
    }

    zItem* item = static_cast<zItem*>(selector->QueryInterface(49));
    if (m_inventory->GetItemNum(item) == -1)
    {
        HideHighLight();
        return false;
    }

    int itemWidth = 0;
    int itemHeight = 0;
    int cellX = 0;
    int cellY = 0;
    (void) m_inventory3D->GetItemCellPos(item, &cellX, &cellY);
    selector->GetSize2DCalced(itemWidth, itemHeight, m_inventory);

    ShowHighLight(cellX, cellY, itemWidth, itemHeight);
    return true;
}

void  zSpriteInventory::DeletePointerToSprite(SPRITE* target)
{
    zSpritePointerVector& main = m_spriteVectors[0];
    for (unsigned int index = 0; ; ++index)
    {
        if (!main.m_begin)
            break;
        const unsigned int count = static_cast<unsigned int>(main.m_end-main.m_begin);
        if (index >= count)
            break;
        for (unsigned int lane = 0; lane<5u; ++lane)
            if (m_spriteVectors[lane].m_begin[index] == target)
                m_spriteVectors[lane].m_begin[index] = 0u;
    }

    zSpritePointerVector& cells = m_spriteVectors[5];
    for (unsigned int index = 0; ; ++index)
    {
        if (!cells.m_begin)
            break;
        const unsigned int count = static_cast<unsigned int>(cells.m_end-cells.m_begin);
        if (index >= count)
            break;
        if (cells.m_begin[index] == target)
            cells.m_begin[index] = 0u;
    }

    unsigned int index = 0;
    for (; ; ++index)
    {
        if (!cells.m_begin)
            break;
        const unsigned int count = static_cast<unsigned int>(cells.m_end-cells.m_begin);
        if (index >= count)
            break;
        if (cells.m_begin[index] != 0u)
            break;
    }
    if (!cells.m_begin || index >= static_cast<unsigned int>(cells.m_end-cells.m_begin))
        cells.m_end = cells.m_begin;

    if (m_highLightSprite == target)
        m_highLightSprite = 0;
}

bool  zSpriteInventory::ShowPlaceToPut(const iRPGObj* selector, float x, float y)
{
    zItem* item = static_cast<zItem*>(selector->QueryInterface(49));
    if (selector == 0 || !m_inventory3D->CanPutItemToPoint(item, x, y))
    {
        HideHighLight();
        return false;
    }

    const int cellX = m_inventory2D->m_cellX;
    const int cellY = m_inventory2D->m_cellY;
    int itemWidth = 0;
    int itemHeight = 0;
    selector->GetSize2DCalced(itemWidth, itemHeight, m_inventory);
    ShowHighLight(cellX, cellY, itemWidth, itemHeight);
    return true;
}


void  zSpriteInventory::RefreshAll()
{
    RefreshItems();
    RefreshCells();
}


void __cdecl GetNew_zSpriteInventory(iSpriteInventory** outObject, iRPGFrameWork* constructorArg)
{
    void* const storage = static_cast<unsigned char*>(CrtNew(sizeof(zSpriteInventory)));
    zSpriteInventory* object = 0;
    try
    {
        if (storage)
            object = ::new (storage) zSpriteInventory(constructorArg);
    }
    catch (...)
    {
        CrtFreePointer(storage);
        throw;
    }
    *outObject = object;
}

void  zSpriteInventory::ClearCells()
{
    zSpritePointerVector& cells = m_spriteVectors[5];
    const int cellSpriteCount = cells.m_begin
        ? static_cast<int>(cells.m_end-cells.m_begin) : 0;
    for (int cellIndex = 0; cellIndex<cellSpriteCount; ++cellIndex)
    {
        SPRITE* sprite = cells.m_begin[cellIndex];
        delete sprite;
    }
    cells.m_end = cells.m_begin;
}

void  zSpriteInventory::CreateText(
    zSpritePointerVector& sprites, int index, const STRING& text,
    int commandValue, int vidId)
{
    const char* chars = text.m_value;
    if (chars[0] == '\0')
    {
        SPRITE* sprite = sprites.m_begin[index];
        if (sprite != 0)
        {
            delete sprite;
            sprites.m_begin[index] = 0u;
        }
        return;
    }

    SPRITE* sprite = sprites.m_begin[index];
    bool create = sprite == 0;
    if (sprite != 0 && sprite->Vid()->nVid != vidId)
    {
        delete sprite;
        create = true;
    }
    if (create)
    {
        const ANGLE direction(0u);
        VECTOR position(0.0f, 0.0f, 0.0f);
        VID* vid = Map->Vid(vidId);
        sprite = Map->CreateSprite(vid, position, direction, 0);
        sprites.m_begin[index] = sprite;
    }

    sprite = sprites.m_begin[index];
    (void) sprite->Action(ACT_SET_TEXT, static_cast<int>(PtrToU32(&text)), 0, 0);
    sprite = sprites.m_begin[index];
    (void) sprite->Action(ACT_SET_BEHAVE, commandValue, 0, 0);
}

bool  zSpriteInventory::RectIsVisible(float centerX, float centerY, float halfW, float halfH)
{
    const float x0 = static_cast<float>(static_cast<double>(centerX)-static_cast<double>(halfW)*0.5);
    const float x1 = static_cast<float>(static_cast<double>(centerX)+static_cast<double>(halfW)*0.5);
    const float y0 = static_cast<float>(static_cast<double>(centerY)-static_cast<double>(halfH)*0.5);
    const float y1 = static_cast<float>(static_cast<double>(centerY)+static_cast<double>(halfH)*0.5);
    if (FloatEqualOrUnordered(m_inventory3D->m_viewWidth, 0.0f) &&
        FloatEqualOrUnordered(m_inventory3D->m_viewHeight, 0.0f)) return true;

    const float xs[4] = {x0, x0, x1, x1};
    const float ys[4] = {y0, y1, y1, y0};
    for (unsigned int corner = 0u; corner<4u; ++corner)
    {
        zInventory3D* const inventory = m_inventory3D;
        const int py = TruncateDoubleToInt32(static_cast<double>(ys[corner])-inventory->m_originY+inventory->m_scrollY);
        const int px = TruncateDoubleToInt32(static_cast<double>(xs[corner])-inventory->m_originX+inventory->m_scrollX);
        if (inventory->SmoothToGrid(false, px, py, 0, 0)) return true;
    }
    return false;
}

void  zSpriteInventory::ClearSprites()
{
    zSpritePointerVector& main = m_spriteVectors[0];
    const int itemSpriteCount = main.m_begin
        ? static_cast<int>(main.m_end-main.m_begin) : 0;
    for (int itemIndex = itemSpriteCount-1; itemIndex >= 0; --itemIndex)
    {
        for (unsigned int lane = 0; lane<5u; ++lane)
        {
            zSpritePointerVector& vector = m_spriteVectors[lane];
            SPRITE* itemSprite = vector.m_begin[itemIndex];
            delete itemSprite;
        }
    }

    for (unsigned int lane = 0; lane<3u; ++lane)
        m_spriteVectors[lane].m_end = m_spriteVectors[lane].m_begin;

    for (unsigned int lane = 3u; lane<5u; ++lane)
    {
        zSpritePointerVector& vector = m_spriteVectors[lane];
        vector.m_end = vector.m_begin;
    }
}

void  zSpriteInventory::RefreshCells()
{
    zSpritePointerVector& cells = m_spriteVectors[5];
    if ((!cells.m_begin || cells.m_end == cells.m_begin) && m_drawCellsType != 0)
        MoveCells(true);
    if (cells.m_begin && cells.m_end != cells.m_begin)
        MoveCells(false);

    int rows = 0;
    int columns = 0;
    m_inventory2D->GetSizeInCells(rows, columns);
    for (unsigned int index = 0u; ; ++index)
    {
        if (!cells.m_begin) break;
        const unsigned int count = static_cast<unsigned int>(cells.m_end-cells.m_begin);
        if (index >= count) break;
        if (cells.m_begin[index] == 0) continue;

        const int signedIndex = static_cast<int>(index);
        const int row = signedIndex/columns;
        const int column = signedIndex%columns;
        unsigned char visible = 0u;
        const int mode = m_drawCellsType;
        bool project = false;
        if (mode == 1)
        {
            visible = m_inventory2D->IsCellActive(row, column);
            project = visible != 0u;
        }
        else if (mode == 2)
        {
            visible = m_inventory3D->CellIsFree(row, column)?0u:1u;
            project = (row >= m_highlightFirstX && row <= m_highlightLastX && column >= m_highlightFirstY && column <= m_highlightLastY) || visible != 0u;
        }
        if (project)
        {
            SPRITE* const item = cells.m_begin[index];
            MAP* const map = Map;
            zInventory3D* const inventory = m_inventory3D;
            const VECTOR& itemPos = item->Coor();
            const float shiftedX = itemPos.x-map->m_shiftXY.x;
            const int screenY = TruncateDoubleToInt32(static_cast<double>(itemPos.y)-itemPos.z-map->m_shiftXY.y
                -inventory->m_originY+inventory->m_scrollY);
            const int screenX = TruncateDoubleToInt32(static_cast<double>(shiftedX)-inventory->m_originX+inventory->m_scrollX);
            visible = inventory->SmoothToGrid(true, screenX, screenY, 0, 0);
        }
        SPRITE* const item = cells.m_begin[index];
        (void) item->Action(ACT_SET_INVISIBLE, visible?0:1, 0, 0);
    }
}


void  zSpriteInventory::CreateItemSprite(int _iNum)
{
    zSpritePointerVector& sprites = m_spriteVectors[0];
    const unsigned int count = sprites.m_begin ? static_cast<unsigned int>(sprites.m_end-sprites.m_begin) : 0u;
    if (!sprites.m_begin || _iNum<0 || static_cast<unsigned int>(_iNum) >= count)
        DebugAssert(5u, "_iNum < m_aItems.size()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 136u);

    zItem* item = m_inventory->GetItem(_iNum);
    if (!item) DebugAssert(5u, "item", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 139u);

    const int vidIndex = item->GetVidNum(2);
    const unsigned int realDirection = static_cast<unsigned int>(item->GetDirNum(2));
    MAP* map = Map;

    bool valid = false;
    if (vidIndex>0 && vidIndex<map->m_vidCount)
        valid = map->m_vids[vidIndex] != 0;
    if (!valid)
    {
        if (g_iZmDebug != 0) return;
        iLogConsole* log = ::gpLogCons;
        (void) log->LogStr(LT_ERR, "Invalid invvid %d for item %s", vidIndex, item->GetName());
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 149u);
        map = Map;
    }

    SPRITE* itemSprite = sprites.m_begin[_iNum];
    if (!itemSprite)
    {
        VID* resource = ::EmptyVid;
        if (vidIndex >= 0 && vidIndex<map->m_vidCount)
        {
            VID* const candidate = map->m_vids[vidIndex];
            if (candidate) resource = candidate;
        }
        const ANGLE initialDirection(0u);
        VECTOR position(0, 0, 0);
        itemSprite = map->CreateSprite(resource, position, initialDirection, 0);
        sprites.m_begin[_iNum] = itemSprite;
    }

    CreateTextIfNeeded(item, _iNum);
    itemSprite = sprites.m_begin[_iNum];
    if (!itemSprite) return;
    (void) itemSprite->Action(ACT_CHANGE_VID, vidIndex, 0, 0);
    itemSprite = sprites.m_begin[_iNum];
    VID* spriteVid = itemSprite->Vid();
    const unsigned int divisor = static_cast<unsigned int>(spriteVid->noDir);
    const unsigned char direction = static_cast<unsigned char>(((realDirection<<8)/divisor)&0xFFu);
    itemSprite->ChangeDirection(ANGLE(direction));
}


void  zSpriteInventory::MoveCells(bool _bCreate)
{
    zSpritePointerVector& cells = m_spriteVectors[5];
    const bool haveCells = cells.m_begin && cells.m_end != cells.m_begin;
    if (haveCells && _bCreate)
        DebugAssert(5u, "m_aCells.size() == 0 || !_bCreate", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 279u);

    int rows = 0;
    int columns = 0;
    m_inventory2D->GetSizeInCells(rows, columns);
    if (rows <= 0) return;

    unsigned int existingIndex = 0u;
    for (int row = 0; row<rows; ++row)
    {
        if (columns <= 0) continue;
        for (int column = 0; column<columns; ++column)
        {
            SPRITE* cellSprite = 0;
            if (_bCreate)
            {
                MAP* map = Map;
                const ANGLE direction(0u);
                VECTOR zeroPosition(0.0f, 0.0f, 0.0f);
                VID* resource = map->Vid(720);
                cellSprite = map->CreateSprite(resource, zeroPosition, direction, 0);
            }
            else if (cells.m_begin && existingIndex<static_cast<unsigned int>(cells.m_end-cells.m_begin))
                cellSprite = cells.m_begin[existingIndex];

            if (!cellSprite) continue;
            if (_bCreate)
            {
                zSpriteInventory_SpriteVectorInsertFill(&cells, cells.m_end, 1u, &cellSprite);
            }
            else
                ++existingIndex;

            float baseX = 0.0f;
            float baseY = 0.0f;
            m_inventory3D->GetCoord(baseX, baseY);
            baseX += Map->m_shiftXY.x;
            baseY += Map->m_shiftXY.y;
            const int stepX = m_inventory3D->GetCellSizeX();
            const float cellX = static_cast<float>(static_cast<double>(baseX)+MultiplySigned32Wrap(stepX, row));
            const int stepY = m_inventory3D->GetCellSizeY();
            const float cellY = static_cast<float>(static_cast<double>(baseY)+MultiplySigned32Wrap(stepY, column));
            VID* const cellVid = cellSprite->Vid();
            const int frame = m_spriteZ == 0?3002:m_spriteZ;
            const int zValue = AddSigned32Wrap(frame, -1);
            const float x = static_cast<float>(static_cast<double>(cellVid->sizeXYZ.x)*0.5+cellX);
            const float y = static_cast<float>(static_cast<double>(cellVid->sizeXYZ.y)*0.5+cellY+zValue);
            const float z = static_cast<float>(zValue);
            const VECTOR position(x, y, z);
            cellSprite->ChangeCoor(position);
            (void) cellSprite->Action(ACT_SET_INVISIBLE, 1, 0, 0);
        }
    }
}

void  zSpriteInventory::ShowHighLight(int row, int col, int width, int height)
{
    float x = 0.0f;
    float y = 0.0f;
    m_inventory3D->GetCoord(x, y);

    const int cellX = m_inventory2D->GetCellSizeX();
    const int cellY = m_inventory2D->GetCellSizeY();
    const int offsetX = MultiplySigned32Wrap(AddSigned32Wrap(MultiplySigned32Wrap(row, 2), 1), cellX)/2;
    const int offsetY = MultiplySigned32Wrap(AddSigned32Wrap(MultiplySigned32Wrap(col, 2), 1), cellY)/2;
    MAP* const map = Map;
    double drawX = static_cast<double>(x)+map->m_shiftXY.x+offsetX;
    y += map->m_shiftXY.y;
    y = static_cast<float>(static_cast<double>(y)+offsetY);

    if (!FloatEqualOrUnordered(m_inventory3D->m_viewWidth, 0.0f) ||
        !FloatEqualOrUnordered(m_inventory3D->m_viewHeight, 0.0f))
    {
        drawX += MultiplySigned32Wrap(AddSigned32Wrap(cellX, -37), AddSigned32Wrap(width, -1))/2;
        y = static_cast<float>(static_cast<double>(y)+
            MultiplySigned32Wrap(AddSigned32Wrap(cellY, -37), AddSigned32Wrap(height, -1))/2);
    }

    x = static_cast<float>(drawX+40.0);
    int zValue = m_spriteZ;
    if (zValue == 0) zValue = 2988;
    const float z = static_cast<float>(zValue);
    y = static_cast<float>(static_cast<double>(y)+52.0+zValue);

    SPRITE* highlight = m_highLightSprite;
    if (!highlight)
    {
        VECTOR zeroPosition(0.0f, 0.0f, 0.0f);
        const ANGLE direction(0u);
        VID* vid = map->Vid(724);
        highlight = map->CreateSprite(vid, zeroPosition, direction, 0);
        m_highLightSprite = highlight;

        const int inventoryKind = static_cast<int>(m_inventory->GetId());
        if (inventoryKind == 12 || inventoryKind == 13 || inventoryKind == 14)
            (void) m_highLightSprite->Action(ACT_SET_SCALE, 650, 650, 650);
    }

    int RealDir = 0;
    if (width == 1 && height == 1)
    {
        const int inventoryKind = static_cast<int>(m_inventory->GetId());
        (void) m_inventory2D->GetCellSizeX();
        (void) m_inventory2D->GetCellSizeY();
        if (inventoryKind == 11)
        {
            RealDir = 2;
            x -= 20.0f;
            y -= 16.0f;
        }
        else if (inventoryKind == 12 || inventoryKind == 13 || inventoryKind == 14)
        {
            x -= 13.0f;
            y -= 19.0f;
        }
    }
    else if (width == 2)
    {
        if (height == 1) RealDir = 1;
        else if (height == 2) RealDir = 2;
        else if (height == 3) RealDir = 5;
        else DebugAssert(4u, "RealDir >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 750u);
    }
    else if (width == 3)
    {
        if (height == 1) RealDir = 3;
        else if (height == 2) RealDir = 4;
        else DebugAssert(4u, "RealDir >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 750u);
    }
    else
        DebugAssert(4u, "RealDir >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 750u);

    const unsigned int divisor = static_cast<unsigned int>(m_highLightSprite->Vid()->noDir);
    const unsigned char direction = static_cast<unsigned char>(
        (static_cast<unsigned int>(RealDir)<<8)/divisor);
    m_highLightSprite->ChangeDirection(ANGLE(direction));

    const VECTOR position(x, y, z);
    m_highLightSprite->ChangeCoor(position);
    const float screenX = static_cast<float>(static_cast<double>(x)-Map->m_shiftXY.x-40.0);
    const float shiftedY = static_cast<float>(static_cast<double>(y)-z-Map->m_shiftXY.y);
    const float screenY = shiftedY-52.0f;
    const bool visible = RectIsVisible(screenX, screenY, static_cast<float>(MultiplySigned32Wrap(width, 37)), static_cast<float>(MultiplySigned32Wrap(height, 37)));
    (void) m_highLightSprite->Action(ACT_SET_INVISIBLE, visible?0:1, 0, 0);

    if (m_drawCellsType == 2)
    {
        m_highlightFirstX = row;
        m_highlightFirstY = col;
        m_highlightLastX = AddSigned32Wrap(AddSigned32Wrap(row, width), -1);
        m_highlightLastY = AddSigned32Wrap(AddSigned32Wrap(col, height), -1);
        RefreshCells();
    }
}

void  zSpriteInventory::CreateTextIfNeeded(zItem* _pItem, int index)
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 174u);

    STRING primary;
    STRING secondary;
    STRING cost;
    STRING condition;

    const int type = _pItem->GetGameObj();
    if (type == 2)
    {
        const int damageMax = _pItem->GetSkillValue(RPGWEAPON_DAMAGEMAX);
        const int damageMin = _pItem->GetSkillValue(RPGWEAPON_DAMAGEMIN);
        primary = Printf("%d-%d", damageMin, damageMax);

        if (m_inventory2D->IfItemAlwaysInCenter() && m_rpgHero)
        {
            zRPGWeapon* weapon = static_cast<zRPGWeapon*>(_pItem->QueryInterface(50));
            zRPGAmmo* ammo = weapon->GetAmmo();
            iRPGObj* const ammoObject = static_cast<zRPGObj*>(ammo);
            if (!ammoObject->IsInfinite())
            {
                iInvServer* ammoStore = m_rpgHero->GetInvServer();
                const int ammoCount = ammoStore->GetObjectCnt(ammoObject->GetBaseId());
                secondary = ammoCount == 0 ? Printf("<Font=07/>%d", ammoCount) : Printf("%d", ammoCount);
            }
        }
    }
    else if (_pItem->GetGameObj() == TYPE_ARMOR)
    {
        const int hp = _pItem->GetSkillValue(RPGARMOR_HP);
        const int percent = AddSigned32Wrap(MultiplySigned32Wrap(hp, 100), 99999)/100000;
        condition = Printf("%d%c", percent, '%');
        const int defence = _pItem->GetSkillValue(RPGARMOR_DEFENCE);
        const int maxHp = _pItem->GetSkillValue(RPGARMOR_MAXHP);
        primary = Printf("%d / %d", defence, maxHp);
    }
    else if (_pItem->GetGameObj() == TYPE_EQUIP)
    {
        const int maximum = _pItem->GetSkillValue(RPGEQUIP_DroneMaxHp);
        if (maximum>0)
        {
            const int current = _pItem->GetSkillValue(RPGEQUIP_DroneHp);
            int percent = AddSigned32Wrap(MultiplySigned32Wrap(current, 100), SubSigned32Wrap(maximum, 1))/maximum;
            if (percent == 0) percent = 100;
            primary = Printf("%d%c", percent, '%');
        }
    }

    if (_pItem->GetMaxNumInCell()>1)
        primary = Printf("%d", _pItem->GetNumInCell());

    if (m_showCost)
        cost = Printf("%d$", _pItem->GetBuyCostFull());

    CreateText(m_spriteVectors[1], index, primary, 6, 3);
    CreateText(m_spriteVectors[2], index, secondary, 2, 3);
    int priceVid = 3;
    if (m_rpgHero)
    {
        const int money = m_rpgHero->GetMoney();
        const int price = _pItem->GetBuyCostFull();
        if (money<price) priceVid = 7;
    }
    CreateText(m_spriteVectors[3], index, cost, 2, priceVid);
    CreateText(m_spriteVectors[4], index, condition, 2, 3);
}

void  zSpriteInventory::RefreshItems()
{
    zInventory* inventory = m_inventory;
    const int itemCount = inventory->GetItemCount();
    zSpritePointerVector* vectors[5] = {
        &m_spriteVectors[0], &m_spriteVectors[1], &m_spriteVectors[2], &m_spriteVectors[3], &m_spriteVectors[4]
    };

    int spriteCount = vectors[0]->m_begin
        ? static_cast<int>(vectors[0]->m_end-vectors[0]->m_begin) : 0;
    while (spriteCount<itemCount)
    {
        SPRITE* const zero = 0;
        for (int lane = 0; lane<5; ++lane)
            zSpriteInventory_SpriteVectorInsertFill(vectors[lane], vectors[lane]->m_end, 1u, &zero);
        ++spriteCount;
    }

    for (; ;)
    {
        spriteCount = vectors[0]->m_begin?static_cast<int>(vectors[0]->m_end-vectors[0]->m_begin):0;
        if (spriteCount <= itemCount) break;
        const int last = spriteCount-1;
        for (int lane = 0; lane<5; ++lane)
        {
            SPRITE* sprite = vectors[lane]->m_begin[last];
            delete sprite;
        }
        for (int lane = 0; lane<5; ++lane)
        {
            zSpritePointerVector& slots = *vectors[lane];
            for (SPRITE** next = slots.m_begin+last+1; next != slots.m_end; ++next)
                next[-1] = *next;
            --slots.m_end;
        }
    }

    const int lane0Count = vectors[0]->m_begin ? static_cast<int>(vectors[0]->m_end-vectors[0]->m_begin) : 0;
    const int lane1Count = vectors[1]->m_begin ? static_cast<int>(vectors[1]->m_end-vectors[1]->m_begin) : 0;
    const int lane2Count = vectors[2]->m_begin ? static_cast<int>(vectors[2]->m_end-vectors[2]->m_begin) : 0;
    if (lane0Count != itemCount || lane1Count != itemCount || lane2Count != itemCount)
        DebugAssert(4u, "m_aItems.size() == cnt && m_aTexts.size() == cnt && m_aTexts2.size() == cnt", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Menu\\zSpriteInventory.cpp", 413u);

    for (int index = 0; index<itemCount; ++index)
    {
        zItem* item = m_inventory->GetItem(index);
        float itemX = 0.0f;
        float itemY = 0.0f;
        if (!m_inventory3D->GetItemPos(item, &itemX, &itemY, true))
        {
            for (int lane = 0; lane<5; ++lane) vectors[lane]->m_begin[index] = 0;
            continue;
        }

        CreateItemSprite(index);
        const float baseX = itemX+Map->m_shiftXY.x;
        const double baseY = static_cast<double>(itemY)+Map->m_shiftXY.y;
        int zValue = m_spriteZ;
        if (zValue == 0) zValue = 3002;
        const float drawY = static_cast<float>(baseY+zValue);

        SPRITE* mainSprite = vectors[0]->m_begin[index];
        if (mainSprite)
            (void) mainSprite->Action(ACT_CHANGE_COOR, TruncateFloatToInt32(baseX),
                TruncateFloatToInt32(drawY), zValue);

        int itemWidth = 0;
        int itemHeight = 0;
        item->GetSize2DCalced(itemWidth, itemHeight, 0);

        int cellX = 37;
        int cellY = 37;
        float extraY = 0.0f;
        if (m_inventory->GetObjectType() == static_cast<OBJ_TYPE>(4))
        {
            cellX = 73;
            cellY = 73;
            extraY = 4.0f;
        }

        const int halfX = MultiplySigned32Wrap(cellX, itemWidth)/2;
        const int halfY = MultiplySigned32Wrap(cellY, itemHeight)/2;
        const int textZ = AddSigned32Wrap(zValue, 1);

        SPRITE* text0 = vectors[1]->m_begin[index];
        if (text0)
            (void) text0->Action(ACT_CHANGE_COOR, TruncateDoubleToInt32(static_cast<double>(baseX)+halfX),
                TruncateDoubleToInt32(static_cast<double>(AddSigned32Wrap(halfY, 1))+extraY+drawY), textZ);

        SPRITE* text1 = vectors[2]->m_begin[index];
        if (text1)
        {
            int lineOffset = 0;
            if (static_cast<int>(m_inventory->GetObjectExtType()) == 1) lineOffset = -1;
            else if (static_cast<int>(m_inventory->GetObjectExtType()) == 2) lineOffset = -2;
            else if (static_cast<int>(m_inventory->GetObjectExtType()) == 3) lineOffset = -2;
            else if (static_cast<int>(m_inventory->GetObjectExtType()) == 4) lineOffset = -1;
            SPRITE* const currentText1 = vectors[2]->m_begin[index];
            (void) currentText1->Action(ACT_CHANGE_COOR, TruncateDoubleToInt32(static_cast<double>(baseX)+halfX),
                TruncateDoubleToInt32(static_cast<double>(drawY)-AddSigned32Wrap(AddSigned32Wrap(halfY, lineOffset), 1)), textZ);
        }

        SPRITE* text2 = vectors[3]->m_begin[index];
        if (text2)
            (void) text2->Action(ACT_CHANGE_COOR, TruncateDoubleToInt32(static_cast<double>(baseX)+halfX),
                TruncateDoubleToInt32(static_cast<double>(drawY)-AddSigned32Wrap(halfY, -1)), textZ);

        SPRITE* text3 = vectors[4]->m_begin[index];
        if (text3)
            (void) text3->Action(ACT_CHANGE_COOR, TruncateDoubleToInt32(static_cast<double>(baseX)+halfX),
                TruncateDoubleToInt32(static_cast<double>(AddSigned32Wrap(halfY, -24))+extraY+drawY), textZ);

        mainSprite = vectors[0]->m_begin[index];
        if (mainSprite)
        {
            GAMMA gamma;
            gamma.modulate = 0u;
            gamma.add = 0u;
            if (m_rpgHero)
            {
                iInvKeeper* keeper = static_cast<iInvKeeper*>(m_rpgHero->QueryInterface(1));
                if (!keeper->CanHoldItm(item))
                {
                    int red = 0;
                    int green = 0;
                    int blue = 0;
                    m_framework->GetGammaItemDisabled(&red, &green, &blue);
                    int* values[3] = {&red, &green, &blue};
                    const unsigned int shifts[3] = {16u, 8u, 0u};
                    for (unsigned int component = 0; component<3u; ++component)
                    {
                        int value = *values[component];
                        if (value<-255) value = -255;
                        if (value>255) value = 255;
                        if (value<0) gamma.modulate |= static_cast<unsigned int>(-value)<<shifts[component];
                        else gamma.add |= static_cast<unsigned int>(value)<<shifts[component];
                    }
                }
            }
            vectors[0]->m_begin[index]->SetGamma(gamma);
        }

        const bool visible = RectIsVisible(itemX, itemY,
            static_cast<float>(MultiplySigned32Wrap(cellX, itemWidth)), static_cast<float>(MultiplySigned32Wrap(cellY, itemHeight)));
        for (int lane = 0; lane<5; ++lane)
        {
            SPRITE* sprite = vectors[lane]->m_begin[index];
            if (sprite)(void) sprite->Action(ACT_SET_INVISIBLE, visible?0:1, 0, 0);
        }
    }

    RefreshCells();
}

template <class Vector, class Element>
static void SpriteInventoryVectorInsertFill(
    Vector* self, Element* position, unsigned int count, const Element* value)
{
    const unsigned int available =
        static_cast<unsigned int>(self->m_capEnd - self->m_end);

    if (available < count)
    {
        Element* oldBegin = self->m_begin;
        Element* oldEnd = self->m_end;
        const unsigned int oldSize = oldBegin == 0
            ? 0u : static_cast<unsigned int>(oldEnd - oldBegin);
        const unsigned int growBy = count < oldSize ? oldSize : count;
        const unsigned int newCapacity = oldSize + growBy;
        const unsigned int allocationCount = static_cast<int>(newCapacity) < 0
            ? 0u : newCapacity;
        Element* newBegin = static_cast<Element*>(CrtNew(allocationCount * sizeof(Element)));
        Element* destination = newBegin;

        Element* source = oldBegin;
        while (source != position)
        {
            if (destination != 0)
                *destination = *source;
            ++source;
            ++destination;
        }

        unsigned int remainingCopies = count;
        while (remainingCopies != 0)
        {
            if (destination != 0)
                *destination = *value;
            ++destination;
            --remainingCopies;
        }

        source = position;
        while (source != oldEnd)
        {
            if (destination != 0)
                *destination = *source;
            ++source;
            ++destination;
        }

        CrtFreePointer(oldBegin);
        self->m_capEnd = newBegin + newCapacity;
        self->m_begin = newBegin;
        self->m_end = newBegin + oldSize + count;
        return;
    }

    const unsigned int tailCount =
        static_cast<unsigned int>(self->m_end - position);

    if (tailCount < count)
    {
        Element* tailSource = position;
        Element* tailDestination = position + count;
        while (tailSource != self->m_end)
        {
            if (tailDestination != 0)
                *tailDestination = *tailSource;
            ++tailSource;
            ++tailDestination;
        }

        unsigned int missing = count - tailCount;
        Element* append = self->m_end;
        while (missing != 0)
        {
            if (append != 0)
                *append = *value;
            ++append;
            --missing;
        }

        Element* fill = position;
        while (fill != self->m_end)
        {
            *fill = *value;
            ++fill;
        }

        self->m_end += count;
        return;
    }

    if (count != 0)
    {
        Element* oldEnd = self->m_end;
        Element* copySrc = oldEnd - count;
        Element* copyDst = oldEnd;
        while (copySrc != oldEnd)
        {
            *copyDst = *copySrc;
            ++copySrc;
            ++copyDst;
        }

        Element* shiftSrc = oldEnd - count;
        Element* shiftDst = oldEnd;
        while (shiftSrc != position)
        {
            --shiftSrc;
            --shiftDst;
            *shiftDst = *shiftSrc;
        }

        Element* fill = position;
        Element* fillEnd = position + count;
        while (fill != fillEnd)
        {
            *fill = *value;
            ++fill;
        }

        self->m_end += count;
    }
}


void zSpriteInventory_SpriteVectorInsertFill(
    zSpritePointerVector* self, SPRITE** position, unsigned int count,
    SPRITE* const* value)
{
    SpriteInventoryVectorInsertFill(self, position, count, value);
}


