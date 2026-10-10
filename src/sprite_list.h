#pragma once
#include "list.h"

class SPRITE;

class SPRITE_LIST : public LIST<SPRITE*>
{
public:
    SPRITE_LIST();
    ~SPRITE_LIST() override;

    void Insert(SPRITE* sprite);
    void InsertSorted(SPRITE* sprite);
    void InsertSorted(SPRITE* sprite, float key);
    int InsertUnique(SPRITE* sprite);
    void InsertFirst(SPRITE* sprite);
    void InsertBefore(int index, SPRITE* sprite);
    int Delete(SPRITE* sprite);
    int DeleteSorted(SPRITE* sprite);
    int DeleteSpriteNumber(int index);
    void DeleteAll();
    void Release();
    int IsEqual(const SPRITE_LIST* rhs) const;
};

