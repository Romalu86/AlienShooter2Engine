#pragma once
#include <stddef.h>
#include "../Inventory/zInventoryItem.h"

class SPRITE;
enum OBJ_TYPE : int;

class zGroundItem : public zItem {
public:
    int m_vidNum[4];
    int m_dirNum[4];
    SPRITE* m_groundSprite;
    int m_throwTime;

    explicit zGroundItem(OBJ_TYPE objectType);
    ~zGroundItem() override;
    void* QueryInterface(int interfaceId) const override;
    int iRelease() override;
    bool ParseUnknownCommand(const char* fieldName,int valueCount,const int* values) override;
    int GetVidNum(int _iVidType) const override;
    int GetDirNum(int _iVidType) const override;
    void SetGrndSpr(SPRITE* sprite) override;
    SPRITE* GetGrndSpr() const override;
    void SetThrowTime(int throwTime) override;
    int GetThrowTime() const override;

protected:
    void CopyParametersFrom(zGroundItem* source);
private:
    void SetVidNum(int _iVidType,int vidNumber);
    void SetDirNum(int _iVidType,int direction);
};


