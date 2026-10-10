#pragma once
#include "basetype.h"
#include <stddef.h>
class SPRITE;
class STRING;
class zItem;
class iRPGFrameWork;
class iInventory;
class iRPGHero;
class iRPGObj;
class INPUT;

class iSpriteInventory {
public:
    virtual void* QueryInterface(int id) const = 0;
    virtual int iRelease() = 0;
    virtual void SetInventory(iInventory* object) = 0;
    virtual void SetDrawCellsType(int type) = 0;
    virtual void SetSpriteZ(int value) = 0;
    virtual void ControlPlayerMenu(INPUT* input) = 0;
    virtual void DeletePointerToSprite(SPRITE* target) = 0;
    virtual void ReloadAll() = 0;
    virtual void RefreshItems() = 0;
    virtual bool ShowPlaceToPut(const iRPGObj* selector, float x, float y) = 0;
    virtual void SetGetDirtyInventory(iInventory* object) = 0;
    virtual void ClearDirty() = 0;
    virtual bool HighLightItem(const iRPGObj* selector) = 0;
    virtual void HideHighLight() = 0;
    virtual void SetRPGHero(iRPGHero* hero) = 0;
    virtual void ShowCost(bool value) = 0;
};


struct zSpritePointerVector {
    unsigned char m_allocatorState;
    unsigned char m_allocatorPadding[3];
    SPRITE** m_begin;
    SPRITE** m_end;
    SPRITE** m_capEnd;

    zSpritePointerVector();
    ~zSpritePointerVector() noexcept(false);
};

class zSpriteInventory : public iSpriteInventory {
public:
    explicit zSpriteInventory(iRPGFrameWork* constructorArg);
    virtual ~zSpriteInventory() noexcept(false);
    static void operator delete(void* pointer);

    class zInventory* m_inventory;
    class zInventory2D* m_inventory2D;
    class zInventory3D* m_inventory3D;
    iRPGHero* m_rpgHero;
    iRPGFrameWork* m_framework;
    zSpritePointerVector m_spriteVectors[6];
    int m_drawCellsType;
    int m_spriteZ;
    SPRITE* m_highLightSprite;
    int m_highlightFirstX; int m_highlightFirstY; int m_highlightLastX; int m_highlightLastY;
    class zInventory* m_dirtyInventory;
    unsigned char m_showCost;
    unsigned char m_costPadding[3];

    void* QueryInterface(int id) const override;
    int iRelease() override;
    void SetInventory(iInventory* inventory) override;
    void SetDrawCellsType(int type) override;
    void SetSpriteZ(int spriteZ) override;
    void ControlPlayerMenu(INPUT* input) override;
    void DeletePointerToSprite(SPRITE* target) override;
    void ReloadAll() override;
private:
    void RefreshItems() override;
public:
    bool ShowPlaceToPut(const iRPGObj* selector, float x, float y) override;
    void SetGetDirtyInventory(iInventory* inventory) override;
    void ClearDirty() override;
private:
    bool HighLightItem(const iRPGObj* selector) override;
public:
    void HideHighLight() override;
private:
    void SetRPGHero(iRPGHero* hero) override;
    void ShowCost(bool showCost) override;

private:
    void CreateItemSprite(int _iNum);
    void CreateTextIfNeeded(zItem* _pItem, int index);
    void CreateText(zSpritePointerVector& sprites, int index, const STRING& text, int commandValue, int vidId);
    void MoveCells(bool _bCreate);
    void ClearSprites();
    void ClearCells();
public:
    void RefreshAll();
private:
    bool RectIsVisible(float centerX, float centerY, float halfW, float halfH);
    void RefreshCells();
    void ShowHighLight(int row, int col, int width, int height);
};

void __cdecl GetNew_zSpriteInventory(iSpriteInventory** outObject, iRPGFrameWork* constructorArg);
