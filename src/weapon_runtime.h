#pragma once
#include <stddef.h>
#include "basetype.h"

struct WEAPON_SERIALIZED
{
    unsigned char prefix000[256];
    float ScaleX[8];
    float ScaleY[8];
    float ScaleZ[8];
    float ShiftX[8];
    float ShiftY[8];
    float ShiftZ[8];
    unsigned int DirectionX[8];
    unsigned int DirectionY[8];
    unsigned int DirectionZ[8];
    unsigned int FrameSpeed[8];
    float Speed[8];
    float ZSpeed[8];
};


struct WEAPON_RUNTIME
{
    int SpriteType;
    unsigned int Property;
    float Length;
    float Weight;
    float Power;
    unsigned int DetectPeriod;
    float DetectRange;
    float BattleRange;
    float aimRange;
    int clipSize;
    int MaxAmmo;
    unsigned int fullReloadTime;
    unsigned int reloadTime;
    unsigned int BuildTime;
    unsigned int DefaultArmy;
    unsigned int DefaultBehave;
    unsigned int Icon;
    unsigned int state44;
    unsigned int state48;
    unsigned int state4C;
    unsigned int state50;
    int EnemyRating;
    float DeadZone;
    unsigned int Period;
    float Time[8];
    int GammaR[8];
    int GammaG[8];
    int GammaB[8];
    int GammaA[8];
    VECTOR Scale[8];
    VECTOR Shift[8];
    unsigned char DirectionX[8];
    unsigned char DirectionY[8];
    unsigned char DirectionZ[8];
    unsigned int FrameSpeed[8];
    float Speed[8];
    float ZSpeed[8];
    int maxColumn;
};

