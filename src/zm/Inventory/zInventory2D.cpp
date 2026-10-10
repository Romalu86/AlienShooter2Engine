#include "mylib.h"
#include "../../engine_globals.h"
#include "zInventory2D.h"
#include "zm/Utils/zLogConsole.h"

#include "basetype.h"
#include <new>


zInventory2D::zInventory2D(int inventoryId)
    : zInventory(inventoryId),
      m_cellWidth(0), m_cellHeight(0), m_gridWidth(0), m_gridHeight(0), m_activeCellPercent(0),
      m_alwaysInCenter(0), m_pointClipped(0), m_cellX(0), m_cellY(0),
      m_scrollX(0.0f), m_scrollY(0.0f), m_viewWidth(0.0f), m_viewHeight(0.0f)
{
}

zInventory2D::~zInventory2D()
{
}

int zInventory2D::iRelease()
{
    delete this;
    return 0;
}

int zInventory2D::GetCellSizeX()
{
    return m_cellWidth;
}


void zInventory2D::GetSizeInCells(int& outWidth, int& outHeight)
{
    outWidth = m_gridWidth;
    outHeight = m_gridHeight;
}


void zInventory2D::SetCellSize(int width, int height)
{
    if (m_cellWidth == width && m_cellHeight == height)
        return;
    m_cellWidth = width;
    m_cellHeight = height;
    if (m_apItems.begin &&
        m_apItems.begin != m_apItems.end)
        (void) ReAllignItems(true);
}


int zInventory2D::GetCellSizeY()
{
    return m_cellHeight;
}


bool zInventory2D::SetSizeInCells(int width, int height)
{
    if (m_gridWidth == width && m_gridHeight == height)
        return false;
    m_gridWidth = width;
    m_gridHeight = height;
    return true;
}


void* zInventory2D::QueryInterface(int interfaceId) const
{
    if (interfaceId == 17)
        return static_cast<iInventory2D*>(const_cast<zInventory2D*>(this));
    if (interfaceId == 47)
        return static_cast<iInventory*>(const_cast<zInventory2D*>(this));
    return zInventory::QueryInterface(interfaceId);
}


void zInventory2D::SetUnlimited(bool enabled)
{
    const unsigned char value = enabled ? 1u : 0u;
    if (m_unlimited != value)
    {
        m_unlimited = value;
        if (value == 0u)
            (void) ReAllignItems(true);
        else
            (void) SetActiveCellsCnt(100);
    }

    iLogConsole* const logger =
        ::gpLogCons;
    logger->LogStr(LT_INFO, "zInventory2D::SetUnlimited %d",
        value != 0u ? 1 : 0);
}

bool zInventory2D::SetActiveCellsCnt(int _iActiveCellsCnt)
{
    if (_iActiveCellsCnt < 0 || _iActiveCellsCnt > 100)
        DebugAssert(4u, "_iActiveCellsCnt >= 0 && _iActiveCellsCnt <= 100", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Inventory\\zInventory2D.cpp", 84u);
    if (m_unlimited)
        _iActiveCellsCnt = 100;

    const int oldValue = m_activeCellPercent;
    m_activeCellPercent = _iActiveCellsCnt;
    if (m_apItems.begin &&
        m_apItems.begin != m_apItems.end &&
        _iActiveCellsCnt < oldValue)
        return ReAllignItems(true);
    return true;
}


void zInventory2D::SetItemAlwaysInCenter(bool enabled)
{
    m_alwaysInCenter = enabled ? 1u : 0u;
    SetDirty();
}

bool zInventory2D::IfItemAlwaysInCenter()
{
    return m_alwaysInCenter != 0u;
}


bool zInventory2D::IsCellActive(int row, int col)
{
    const int scale = m_activeCellPercent;
    if (scale == 100)
        return true;

    const int gridHeight = m_gridHeight;
    const int areaMinusSix = SubSigned32Wrap(MultiplySigned32Wrap(m_gridWidth, gridHeight), 6);
    const int scaled = MultiplySigned32Wrap(areaMinusSix, scale);
    const int limit = AddSigned32Wrap(scaled / 100, 6);
    const int cell = AddSigned32Wrap(MultiplySigned32Wrap(gridHeight, row), col);
    const int triple = MultiplySigned32Wrap(gridHeight, 3);

    if (limit >= triple)
        return cell < limit;
    if (row < 2 && col < 3)
        return true;
    if (row == 2 && col < 3 &&
        cell >= MultiplySigned32Wrap(gridHeight, 2) &&
        cell <= AddSigned32Wrap(limit, 1))
        return true;
    if (col != 3)
        return false;
    if (cell == 3 && limit >= 10)
        return true;
    if (cell == 7 && limit >= 11)
        return true;
    return false;
}


bool zInventory2D::SmoothToGrid(bool smooth, int x, int y, int spanX, int spanY)
{
    const int cellWidth = m_cellWidth;
    const int cellHeight = m_cellHeight;
    const int gridWidth = m_gridWidth;
    const int gridHeight = m_gridHeight;

    int minX = 0;
    int minY = 0;
    int maxX = MultiplySigned32Wrap(cellWidth, gridWidth);
    int maxY = MultiplySigned32Wrap(cellHeight, gridHeight);

    m_pointClipped = 0u;
    m_cellX = 0;
    m_cellY = 0;

    const float zero = (0.0f);
    const float viewX = m_scrollX;
    const float viewY = m_scrollY;
    const float viewWidth = m_viewWidth;
    const float viewHeight = m_viewHeight;

    if (!FloatEqualOrUnordered(viewWidth, zero))
    {
        if (FloatLessOrUnordered(zero, viewX))
            minX = TruncateFloatToInt32(viewX);
        const double right = static_cast<double>(viewX) + viewWidth;
        if (static_cast<double>(maxX) > right)
            maxX = TruncateDoubleToInt32(right);
    }

    if (!FloatEqualOrUnordered(viewHeight, zero))
    {
        if (FloatLessOrUnordered(zero, viewY))
            minY = TruncateFloatToInt32(viewY);
        const double bottom = static_cast<double>(minY) + viewHeight;
        if (static_cast<double>(maxY) > bottom)
            maxY = TruncateDoubleToInt32(bottom);
    }

    if (spanX != 0 && spanY != 0)
    {
        const int xOffset = MultiplySigned32Wrap(SubSigned32Wrap(spanX, 1), cellWidth) / 2;
        const int yOffset = MultiplySigned32Wrap(SubSigned32Wrap(spanY, 1), cellHeight) / 2;
        x = SubSigned32Wrap(x, xOffset);
        y = SubSigned32Wrap(y, yOffset);
    }

    if (smooth)
    {
        if (x < minX)
        {
            x = AddSigned32Wrap(x, cellWidth / 2);
            m_pointClipped = 1u;
        }
        if (y < minY)
        {
            y = AddSigned32Wrap(y, cellHeight / 2);
            m_pointClipped = 1u;
        }
        if (x >= maxX)
        {
            x = SubSigned32Wrap(x, cellWidth / 2);
            m_pointClipped = 1u;
        }
        if (y >= maxY)
        {
            y = SubSigned32Wrap(y, cellHeight / 2);
            m_pointClipped = 1u;
        }
    }

    const int pixelMargin = (spanX > 0 && spanY > 0) ? 1 : 0;
    if (AddSigned32Wrap(x, pixelMargin) < minX || AddSigned32Wrap(y, pixelMargin) < minY ||
        SubSigned32Wrap(x, pixelMargin) >= maxX ||
        SubSigned32Wrap(y, pixelMargin) >= maxY)
        return 0u;

    int cellX = x / cellWidth;
    int cellY = y / cellHeight;

    if (spanX > 0 && spanY > 0)
    {
        bool clamped = false;
        if (cellX < 0)
        {
            cellX = AddSigned32Wrap(cellX, 1);
            clamped = true;
        }
        else if (cellY < 0)
        {
            cellY = AddSigned32Wrap(cellY, 1);
            clamped = true;
        }

        if (AddSigned32Wrap(cellX, spanX) > gridWidth)
        {
            cellX = SubSigned32Wrap(cellX, 1);
            clamped = true;
        }
        else if (!clamped && AddSigned32Wrap(cellY, spanY) > gridHeight)
        {
            cellY = SubSigned32Wrap(cellY, 1);
            clamped = true;
        }

        if (!clamped)
        {
            const int originalX = cellX;
            const int originalY = cellY;

            if (!CanPutItemToCell(spanX, spanY, cellX, cellY))
            {
                bool moved = false;
                cellX = SubSigned32Wrap(cellX, 1);
                if (CanPutItemToCell(spanX, spanY, cellX, cellY))
                    moved = true;
                else
                {
                    cellX = originalX;
                    cellY = SubSigned32Wrap(cellY, 1);
                    if (CanPutItemToCell(spanX, spanY, cellX, cellY))
                        moved = true;
                    else
                    {
                        cellY = originalY;
                        cellX = AddSigned32Wrap(originalX, 1);
                        if (CanPutItemToCell(spanX, spanY, cellX, cellY))
                            moved = true;
                        else
                        {
                            cellX = originalX;
                            cellY = AddSigned32Wrap(originalY, 1);
                            if (CanPutItemToCell(spanX, spanY, cellX, cellY))
                                moved = true;
                            else
                                cellY = originalY;
                        }
                    }
                }
                m_pointClipped = moved ? 1u : 0u;
            }
        }
    }

    if (cellX < 0 || cellY < 0 || cellX >= m_gridWidth || cellY >= m_gridHeight)
        return 0u;

    if (m_alwaysInCenter != 0u)
    {
        cellX = 0;
        cellY = 0;
    }

    m_cellX = cellX;
    m_cellY = cellY;
    return 1u;
}


