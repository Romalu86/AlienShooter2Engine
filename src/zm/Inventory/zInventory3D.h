#pragma once

#include "zm/Inventory/zInventory2D.h"

class zItem;


class zInventory3D : public zInventory2D, public iInventory3D
{

    friend class zInvServer;
public:
    zItem*** m_itemsGrid;
    zItem*** m_savedGrid;
    float m_originX;
    float m_originY;
    zItem* m_displacedItem;
    unsigned char m_allowReplacement;
    unsigned char m_allowPointTake;
    unsigned char m_gridFlagsPadding[2];

    explicit zInventory3D(int inventoryId);
    virtual ~zInventory3D();

    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    void SetCoord(float x, float y);
    void GetCoord(float& outX, float& outY);
    void SetLimits(float left, float top, float right, float bottom);
    void GetLimits(float& left, float& top, float& right, float& bottom);
    void CheckScroll();
    void Scroll(float dx, float dy);
    void ScrollPer(int multiplier);
    zItem* GetItemByPoint(float x, float y);
    bool SetSizeInCells(int width, int height) override;
    void RemoveAllItems() override;
protected:
    zItem* DeleteItem(int itemIndex) override;
private:
    bool DeleteItemFromCells(zItem* item);
public:
    bool CellIsFree(int _iCellX, int _iCellY);
    bool GetItemCellPos(const zItem* item, int* outX, int* outY);
private:
    bool CanPutItemToCell(const zItem* item, int x, int y);
protected:
    bool CanPutItemToCell(int rectWidth, int rectHeight, int startX, int startY) override;
private:
    int IfObjectCanBePut(const zItem* item, bool testGrid) override;
    int PutItemToCell(zItem* item, int x, int y);
public:
    int PutItemToCellEx(zItem* _pItem, int x, int y);
    int PutItem(zItem* pItem, zItem** outResolved) override;
private:
    bool TryPutItemToGrid(zItem* item);
    bool CanPutItemToGrid(const zItem* item, int& outX, int& outY, bool unused);
    bool ReAllignItems(bool removeUnplaceable) override;
    void PushGrid();
    void PopGrid();
public:
    bool CanPutItemToPoint(const zItem* item, float x, float y);
    int PutItemToPoint(zItem* item, float x, float y);
    bool GetItemPos(const zItem* item, float* outX, float* outY, bool centerItem);
private:
    void CreateItemsArray();
    void FreeItemsArray();
    void CheckForBugs();
};

