#pragma once

#include "zInventory.h"


class zInventory2D : public zInventory, public iInventory2D
{

    friend class zSpriteInventory;
    friend class zInvServer;
public:
    int m_cellWidth;
    int m_cellHeight;
    int m_gridWidth;
    int m_gridHeight;
    int m_activeCellPercent;
    unsigned char m_alwaysInCenter;
    unsigned char m_pointClipped;
    unsigned char m_cellFlagsPadding[2];
    int m_cellX;
    int m_cellY;
    float m_scrollX;
    float m_scrollY;
    float m_viewWidth;
    float m_viewHeight;

    explicit zInventory2D(int inventoryId);
    virtual ~zInventory2D();

    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    int GetCellSizeX() override;
    int GetCellSizeY() override;
    void SetCellSize(int width, int height);
    bool SetSizeInCells(int width, int height) override;
    bool SetActiveCellsCnt(int _iActiveCellsCnt);
    bool IsCellActive(int row, int col);
    void GetSizeInCells(int& outWidth, int& outHeight) override;
    void SetItemAlwaysInCenter(bool enabled);
    bool IfItemAlwaysInCenter();
protected:
    bool SmoothToGrid(bool smooth, int x, int y, int spanX, int spanY);
    void SetUnlimited(bool enabled) override;

protected:
    virtual bool ReAllignItems(bool removeUnplaceable) = 0;
    virtual bool CanPutItemToCell(int rectWidth, int rectHeight, int startX, int startY) = 0;
};

