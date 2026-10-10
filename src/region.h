#pragma once
#include "mylib.h"
#include "basetype.h"
#include "sprite.h"

class VID;
class COLOR
{
public:
    COLOR()
{
}
    COLOR(const COLOR& source);
    COLOR(const GAMMA& gamma, const COLOR& baseColor);

    unsigned int value;
};

class REGION : public SPRITE
{
    friend class SPRITE;
public:
    enum { VidConversionCount = 6 };

    REGION(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent);
    virtual ~REGION();
    virtual int Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter);
    virtual void Draw();
    virtual void DrawSecondaryInfo();

    VECTOR2 ScreenLeftTop() const;
    VECTOR2 ScreenRightBottom() const;
    static VID* ConvertVid(VID* sourceVid, const VECTOR& position);
    void SetFogParameters(int fogStart, int fogEnd, COLOR fogColor);
    float HalfWidth() const
    {
        return m_halfWidth;
    }
    float HalfHeight() const
    {
        return m_halfHeight;
    }

private:
    int m_fogPhase;
    unsigned int m_lastFogPhase;
    unsigned short* m_fogRamp;
    unsigned int m_fogFlags;
    int m_fogEnd;
    int m_fogStart;
    COLOR m_fogColor;
    unsigned int m_084;
    float m_halfWidth;
    float m_halfHeight;
    VID* m_regionVid;
    unsigned int m_094;
    VID* m_sourceVid[VidConversionCount];
    VID* m_targetVid[VidConversionCount];
};

