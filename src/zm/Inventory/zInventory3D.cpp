#include "mylib.h"

#include "zm/Inventory/zInventory3D.h"
#include "zm/Inventory/zInventoryItem.h"
#include "basetype.h"
#include <new>



zInventory3D::zInventory3D(int inventoryId)
    : zInventory2D(inventoryId),
      m_itemsGrid(0), m_savedGrid(0), m_originX(0), m_originY(0), m_displacedItem(0),
      m_allowReplacement(1), m_allowPointTake(1)
{
}

zInventory3D::~zInventory3D()
{
    FreeItemsArray();
}

int zInventory3D::iRelease()
{
    delete this;
    return 0;
}

void zInventory3D::SetCoord(float x, float y)
{
    if (FloatEqualOrUnordered(m_originX, x) &&
        FloatEqualOrUnordered(m_originY, y))
        return;
    m_originX = x;
    m_originY = y;
    SetDirty();
}

void zInventory3D::GetCoord(float& outX, float& outY)
{
    outX = m_originX - m_scrollX;
    outY = m_originY - m_scrollY;
}

void zInventory3D::SetLimits(float left, float top, float right, float bottom)
{
    if (FloatEqualOrUnordered(left, m_scrollX) &&
        FloatEqualOrUnordered(top, m_scrollY) &&
        FloatEqualOrUnordered(right, m_viewWidth) &&
        FloatEqualOrUnordered(bottom, m_viewHeight))
        return;
    m_scrollX = left;
    m_scrollY = top;
    m_viewWidth = right;
    m_viewHeight = bottom;
    CheckScroll();
    SetDirty();
}

void zInventory3D::GetLimits(float& left, float& top, float& right, float& bottom)
{
    left = m_scrollX;
    top = m_scrollY;
    right = m_viewWidth;
    bottom = m_viewHeight;
}

void zInventory3D::CheckScroll()
{
    const int gridWidth = m_gridWidth;
    const int widthInPixels = MultiplySigned32Wrap(GetCellSizeX(), gridWidth);
    const float storedWidth = static_cast<float>(widthInPixels);
    const int gridHeight = m_gridHeight;
    const int heightInPixels = MultiplySigned32Wrap(GetCellSizeY(), gridHeight);
    const double limitX = static_cast<double>(storedWidth) - m_viewWidth;
    const double limitY = static_cast<double>(heightInPixels) - m_viewHeight;
    const float left = m_scrollX;
    const float top = m_scrollY;
    if (limitX < left || FloatIsNaN(static_cast<float>(limitX)) || FloatIsNaN(left))
        m_scrollX = static_cast<float>(limitX);
    if (limitY < top || FloatIsNaN(static_cast<float>(limitY)) || FloatIsNaN(top))
        m_scrollY = static_cast<float>(limitY);

    const float zero = (0.0f);
    const float checkedLeft = m_scrollX;
    const float checkedTop = m_scrollY;
    if (checkedLeft < zero || FloatIsNaN(checkedLeft) ||
        FloatEqualOrUnordered(m_viewWidth, zero))
        m_scrollX = 0.0f;
    if (checkedTop < zero || FloatIsNaN(checkedTop) ||
        FloatEqualOrUnordered(m_viewHeight, zero))
        m_scrollY = 0.0f;
    SetDirty();
}

void zInventory3D::Scroll(float dx, float dy)
{
    m_scrollX += dx;
    m_scrollY += dy;
    CheckScroll();
}

void zInventory3D::ScrollPer(int multiplier)
{
    const int gridHeight = m_gridHeight;
    const int heightInPixels = MultiplySigned32Wrap(GetCellSizeY(), gridHeight);
    const double scrollOffset = (static_cast<double>(heightInPixels) - m_viewHeight) *
    static_cast<double>(multiplier) * (0.00100000005f);
    m_scrollY = static_cast<float>(scrollOffset);
    CheckScroll();
}

zItem* zInventory3D::GetItemByPoint(float x, float y)
{
    const int gridY = TruncateDoubleToInt32(static_cast<double>(y) - m_originY + m_scrollY);
    const int gridX = TruncateDoubleToInt32(static_cast<double>(x) - m_originX + m_scrollX);
    if (!SmoothToGrid(false, gridX, gridY, 0, 0))
        return 0;
    const int cellX = m_cellX;
    const int cellY = m_cellY;
    if (cellX < 0 || cellY < 0 || cellX >= m_gridWidth || cellY >= m_gridHeight)
        return 0;
    return m_itemsGrid[cellX][cellY];
}


zItem* zInventory3D::DeleteItem(int itemIndex)
{
    zItem* item = zInventory::TakeItem(itemIndex);
    if (item)
        (void) DeleteItemFromCells(item);
    return item;
}

void* zInventory3D::QueryInterface(int interfaceId) const
{
    if (interfaceId == 48)
        return static_cast<iInventory*>(const_cast<zInventory3D*>(this));
    if (interfaceId == 18)
        return static_cast<iInventory3D*>(const_cast<zInventory3D*>(this));
    return zInventory2D::QueryInterface(interfaceId);
}


bool zInventory3D::CellIsFree(int _iCellX, int _iCellY)
{
    if (_iCellX < 0 || _iCellX >= m_gridWidth || _iCellY < 0 || _iCellY >= m_gridHeight)
        DebugAssert(5u, "_iCellX >=0 && _iCellX < width && _iCellY >= 0 && _iCellY < height", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 763u);
    zItem*** grid = m_itemsGrid;
    return grid[_iCellX][_iCellY] == 0;
}

bool zInventory3D::SetSizeInCells(int width, int height)
{
    if (m_gridWidth == width && m_gridHeight == height)
        return false;

    FreeItemsArray();
    (void) zInventory2D::SetSizeInCells(width, height);
    CreateItemsArray();
    if (m_apItems.begin &&
        m_apItems.begin != m_apItems.end)
        (void) ReAllignItems(true);
    return true;
}


void zInventory3D::RemoveAllItems()
{
    zInventory::RemoveAllItems();
    if (!m_itemsGrid)
        return;
    const int width = m_gridWidth;
    const int height = m_gridHeight;
    for (int i = 0; i < width; ++i)
        for (int j = 0; j < height; ++j)
        {
            if (m_itemsGrid[i][j])
                DebugAssert(4u, "m_Items[ i ][ j ] == NULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 400u);
            m_itemsGrid[i][j] = 0;
        }
}

bool zInventory3D::GetItemCellPos(const zItem* item, int* outX, int* outY)
{
    zItem*** grid = m_itemsGrid;
    const zItem* const target = item;
    for (int x = 0; x < m_gridWidth; ++x)
        for (int y = 0; y < m_gridHeight; ++y)
            if (grid[x][y] == target)
            {
                *outX = x;
                *outY = y;
                return true;
            }
    if (!m_unlimited)
        DebugAssert(4u, "IsUnlimited() || HaveNeededItem_for_debug", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 709u);
    *outX = 0;
    *outY = 0;
    return false;
}

bool zInventory3D::CanPutItemToCell(const zItem* item, int x, int y)
{
    if (!m_itemsGrid || !item)
        DebugAssert(5u, "m_Items && item", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 532u);
    if (!IfObjectCanBePut(item, false))
        return false;
    int width = 0;
    int height = 0;
    item->GetSize2DCalced(width, height, this);
    return CanPutItemToCell(width, height, x, y);
}




int zInventory3D::IfObjectCanBePut(const zItem* item, bool testGrid)
{
    int result = zInventory::IfObjectCanBePut(item, testGrid);
    if ((result & 1) && testGrid && !m_unlimited)
    {
        int x = 0, y = 0;
        if (!CanPutItemToGrid(item, x, y, true))
            result = 0;
        if ((result & 1) == 0)
            return zInventory::CanAddToItems(item);
    }
    return result;
}


void zInventory3D::CheckForBugs()
{
    if (m_unlimited)
        return;
    for (int x = 0; x < m_gridWidth; ++x)
        for (int y = 0; y < m_gridHeight; ++y)
        {
            zItem* item = m_itemsGrid[x][y];
            if (item && zInventory::GetItemNum(item) == -1)
            {
                DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 835u);
                (void) DeleteItemFromCells(m_itemsGrid[x][y]);
            }
        }
    for (int i = 0; i < GetItemCount();)
    {
        zItem* item = zInventory::GetItem(i);
        int x = 0, y = 0;
        if (!GetItemCellPos(item, &x, &y))
        {
            DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 849u);
            (void) zInventory::TakeItem(item);
        }
        else
            ++i;
    }
}

int zInventory3D::PutItemToCell(zItem* item, int x, int y)
{
    int width = 0;
    int height = 0;
    item->GetSize2DCalced(width, height, this);
    if (!CanPutItemToCell(item, x, y))
        return 0;

    zItem* active = m_displacedItem;
    if (active)
    {
        const int merged = active->TryToAddItem(item, true);
        if (merged)
            return merged;
    }
    active = m_displacedItem;
    if (active)
        (void) zInventory::TakeItem(active);

    const int last_i = AddSigned32Wrap(x, width);
    const int last_j = AddSigned32Wrap(y, height);
    if (last_i > m_gridWidth)
        DebugAssert(5u, "last_i <= this->GetWidthInCells()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 346u);
    if (last_j > m_gridHeight)
        DebugAssert(5u, "last_j <= this->GetHeightInCells()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 347u);

    for (int i = x; i < last_i; ++i)
        for (int j = y; j < last_j; ++j)
        {
            if (m_itemsGrid[i][j])
                DebugAssert(4u, "m_Items[ i ][ j ] == NULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 353u);
            m_itemsGrid[i][j] = item;
        }
    return 1;
}

bool zInventory3D::ReAllignItems(bool removeUnplaceable)
{
    zItem*** grid = m_itemsGrid;
    for (int x = 0; x < m_gridWidth; ++x)
        for (int y = 0; y < m_gridHeight; ++y)
            grid[x][y] = 0;

    bool allPlaced = true;
    for (int index = 0; index < GetItemCount();)
    {
        zItem* item = zInventory::GetItem(index);
        const bool placed = TryPutItemToGrid(item);
        if (!placed)
            allPlaced = false;
        if (!placed && removeUnplaceable)
        {
            (void) zInventory::TakeItem(item);
            iInvKeeper* const keeper = m_keeper;
            iRPGObj* object = static_cast<iRPGObj*>(item->QueryInterface(9));
            (void) keeper->CreateGroundSprite(object);
        }
        else
            ++index;
    }
    SetDirty();
    if (removeUnplaceable)
        CheckForBugs();
    return allPlaced;
}

int zInventory3D::PutItemToCellEx(zItem* _pItem, int x, int y)
{
    if (_pItem->IsBaseObject())
        DebugAssert(5u, "!_pItem->IsBaseObject()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 287u);
    if (_pItem->IsBaseObject())
    {
        iRPGObj* base = _pItem->GetObjCopy();
        _pItem = static_cast<zItem*>(base->QueryInterface(49));
    }
    if (!CanPutItemToCell(_pItem, x, y))
        return 0;

    const int flags = PutItemToCell(_pItem, x, y);
    if ((flags & 1u) && !(flags & 2u))
    {
        zItem* const displacedItem = m_displacedItem;
        const int added = zInventory::PutItem(_pItem, false, false, 0);
        m_displacedItem = displacedItem;
        if (!(added & 1u))
            DebugAssert(4u, "buf & ITEM_ADDEDFULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 305u);
    }
    CheckForBugs();
    return flags;
}

int zInventory3D::PutItem(zItem* pItem, zItem** outResolved)
{
    const bool wasDirty = zInventory::GetDirty();
    zItem* resolved = 0;
    int flags = zInventory::PutItem(pItem, &resolved);
    if (outResolved)
        *outResolved = resolved;
    if (resolved)
        pItem = resolved;
    if (!flags)
        return 0;
    if (!(flags & 1u))
    {
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 179u);
        return flags;
    }
    if (!(flags & 2u) || (flags & 4u))
    {
        if (pItem->IsBaseObject())
            DebugAssert(4u, "!pItem->IsBaseObject()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 186u);
        PushGrid();
        if (!TryPutItemToGrid(pItem) && !ReAllignItems(false))
        {
            if (m_unlimited == 0)
            {
                zItem* const buf = zInventory::TakeItem(pItem);
                if (!buf)
                    DebugAssert(4u, "buf", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory3D.cpp", 196u);
                flags &= ~1u;
            }
            PopGrid();
        }
    }
    if (flags == 0 && !wasDirty && zInventory::GetDirtyNotErase())
        (void) zInventory::GetDirty();
    CheckForBugs();
    return flags;
}

bool zInventory3D::CanPutItemToCell(int rectWidth, int rectHeight, int startX, int startY)
{
    m_displacedItem = 0;
    if (startX < 0 || startY < 0 || startX >= m_gridWidth || startY >= m_gridHeight)
        return false;
    const int gridWidth = m_gridWidth;
    const int gridHeight = m_gridHeight;
    if (IfItemAlwaysInCenter())
    {
        startX = startY = 0;
        m_displacedItem = m_itemsGrid[0][0];
        if (m_allowReplacement)
            return true;
    }
    const int endX = AddSigned32Wrap(startX, rectWidth);
    const int endY = AddSigned32Wrap(startY, rectHeight);
    if (endX > gridWidth || endY > gridHeight)
        return false;

    zItem* selected = 0;
    for (int x = startX; x < endX; ++x)
        for (int y = startY; y < endY; ++y)
        {
            if (!IsCellActive(x, y))
                return false;
            zItem* cell = m_itemsGrid[x][y];
            if (cell)
            {
                m_displacedItem = cell;
                int width = 0;
                int height = 0;
                cell->GetSize2DCalced(width, height, this);
                if (!m_allowReplacement || width != rectWidth || height != rectHeight)
                    return false;
                cell = m_displacedItem;
                if (selected && selected != cell)
                    return false;
                if (!selected && cell && (x != startX || y != startY))
                    return false;
                selected = cell;
            }
            else if (selected)
                return false;
        }
    return true;
}


int zInventory3D::PutItemToPoint(zItem* item, float x, float y)
{
    if (!CanPutItemToPoint(item, x, y))
        return 0;
    return PutItemToCellEx(item, m_cellX, m_cellY);
}


bool zInventory3D::DeleteItemFromCells(zItem* item)
{
    bool found = false;
    zItem*** grid = m_itemsGrid;
    zItem* const target = item;
    for (int x = 0; x < m_gridWidth; ++x)
        for (int y = 0; y < m_gridHeight; ++y)
            if (grid[x][y] == target)
            {
                grid[x][y] = 0;
                found = true;
            }
    return found;
}

bool zInventory3D::TryPutItemToGrid(zItem* item)
{
    int x = 0;
    int y = 0;
    if (!CanPutItemToGrid(item, x, y, false))
        return false;
    return (PutItemToCell(item, x, y) & 1u) != 0;
}

void zInventory3D::PushGrid()
{
    zItem*** grid = m_itemsGrid;
    zItem*** backup = m_savedGrid;
    for (int x = 0; x < m_gridWidth; ++x)
        for (int y = 0; y < m_gridHeight; ++y)
            backup[x][y] = grid[x][y];
}


void zInventory3D::PopGrid()
{
    zItem*** grid = m_itemsGrid;
    zItem*** backup = m_savedGrid;
    for (int x = 0; x < m_gridWidth; ++x)
        for (int y = 0; y < m_gridHeight; ++y)
            grid[x][y] = backup[x][y];
    CheckForBugs();
}


void zInventory3D::FreeItemsArray()
{
    if (!m_itemsGrid)
        return;

    for (int x = 0; x < m_gridWidth; ++x)
    {
        CrtFreePointer(m_itemsGrid[x]);
        CrtFreePointer(m_savedGrid[x]);
    }
    CrtFreePointer(m_itemsGrid);
    CrtFreePointer(m_savedGrid);
    m_itemsGrid = 0;
    (void) zInventory2D::SetSizeInCells(0, 0);
}

bool zInventory3D::CanPutItemToGrid(const zItem* item, int& outX, int& outY, bool unused)
{
    (void) unused;
    const unsigned char allowReplacement = m_allowReplacement;
    for (int y = 0; y < m_gridHeight; ++y)
        for (int x = 0; x < m_gridWidth; ++x)
        {
            m_allowReplacement = 0;
            const bool fits = CanPutItemToCell(item, x, y);
            m_allowReplacement = allowReplacement;
            if (fits)
            {
                outX = x;
                outY = y;
                return true;
            }
        }
    return false;
}


bool zInventory3D::CanPutItemToPoint(const zItem* item, float x, float y)
{
    int spanX = 0;
    int spanY = 0;
    item->GetSize2DCalced(spanX, spanY, this);
    const int gridY = TruncateDoubleToInt32(static_cast<double>(y) - m_originY + m_scrollY);
    const int gridX = TruncateDoubleToInt32(static_cast<double>(x) - m_originX + m_scrollX);
    if (!SmoothToGrid(true, gridX, gridY, spanX, spanY))
        return false;
    return CanPutItemToCell(item, m_cellX, m_cellY);
}

void zInventory3D::CreateItemsArray()
{
    const int width = m_gridWidth;
    const int height = m_gridHeight;
    const unsigned int columnBytes = static_cast<unsigned int>(height) * sizeof(zItem*);
    m_itemsGrid = reinterpret_cast<zItem***>(CrtAllocate(static_cast<unsigned int>(width) * sizeof(zItem**)));
    m_savedGrid = reinterpret_cast<zItem***>(CrtAllocate(static_cast<unsigned int>(width) * sizeof(zItem**)));
    for (int x = 0; x < width; ++x)
    {
        m_itemsGrid[x] = reinterpret_cast<zItem**>(CrtAllocate(columnBytes));
        m_savedGrid[x] = reinterpret_cast<zItem**>(CrtAllocate(columnBytes));
        for (int y = 0; y < height; ++y)
        {
            m_itemsGrid[x][y] = 0;
            m_savedGrid[x][y] = 0;
        }
    }
    SetDirty();
}


bool zInventory3D::GetItemPos(const zItem* item, float* outX, float* outY, bool centerItem)
{
    int cellX = 0;
    int cellY = 0;
    if (!GetItemCellPos(item, &cellX, &cellY))
        return false;

    const float cellCoordX = static_cast<float>(cellX);
    const float cellCoordY = static_cast<float>(cellY);
    int itemWidth = 0;
    int itemHeight = 0;
    item->GetSize2DCalced(itemWidth, itemHeight, this);
    const int cellWidth = GetCellSizeX();
    const float x = static_cast<float>(centerItem
        ? (static_cast<double>(cellCoordX) + cellCoordX + itemWidth) * cellWidth * 0.5f
        : static_cast<double>(cellWidth) * cellCoordX);
    const int cellHeight = GetCellSizeY();
    const double y = centerItem
        ? (static_cast<double>(cellCoordY) + cellCoordY + itemHeight) * cellHeight * 0.5f
        : static_cast<double>(cellHeight) * cellCoordY;
    *outX = static_cast<float>(static_cast<double>(x) + m_originX - m_scrollX);
    *outY = static_cast<float>(y + m_originY - m_scrollY);
    return true;
}


