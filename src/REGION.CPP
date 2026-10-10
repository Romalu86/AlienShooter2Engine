#include "mylib.h"
#include "engine_globals.h"

#include "region.h"
#include "basetype.h"
#include "graph.h"
#include "map.h"
#include "sprite.h"
#include "sprite_collector.h"
#include "stream.h"
#include "vid.h"
#include <new>


extern "C" unsigned int __cdecl RegionStaticCleanupRegister()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

REGION::REGION(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : SPRITE(vid, position, direction, parent),
      m_fogPhase(0), m_lastFogPhase(0), m_fogRamp(0), m_fogFlags(0),
      m_fogEnd(0), m_fogStart(0), m_084(0), m_halfWidth(0.0f), m_halfHeight(0.0f),
      m_regionVid(::EmptyVid)
{
    m_fogColor.value = 0xFF000000u;
    for (unsigned int index = 0;index<static_cast<unsigned int>(VidConversionCount);++index)
    {
        m_sourceVid[index] = 0;
        m_targetVid[index] = 0;
    }
}

REGION::~REGION()
{
    Map->DeletePointerToSprite(this);
    if (m_fogRamp)
        CrtFreePointer(m_fogRamp);
}

void REGION::DrawSecondaryInfo()
{
    const VECTOR2 rightBottom = ScreenRightBottom()+VECTOR2(1, 1);
    const VECTOR2 leftTop = ScreenLeftTop()+VECTOR2(-1, -1);
    COLOR color;
    color.value = 0xFFFFFFFFu;
    Graph->Box(leftTop, rightBottom, color);
}

VECTOR2 REGION::ScreenLeftTop() const
{
    const float screenScale = 0.5f;
    const float projectedHalfWidth = m_halfWidth*screenScale;
    const float projectedHalfHeight = m_halfHeight*screenScale;
    const float projectedY = m_xyz.y-m_xyz.z;
    VECTOR2 corner;
    corner.x = m_xyz.x-projectedHalfWidth-Map->m_shiftXY.x;
    corner.y = projectedY-projectedHalfHeight-Map->m_shiftXY.y;
    return corner;
}

VECTOR2 REGION::ScreenRightBottom() const
{
    const float screenScale = 0.5f;
    const float projectedHalfWidth = m_halfWidth*screenScale;
    const float projectedHalfHeight = m_halfHeight*screenScale;
    const float projectedY = m_xyz.y-m_xyz.z;
    VECTOR2 corner;
    corner.x = m_xyz.x+projectedHalfWidth-Map->m_shiftXY.x;
    corner.y = projectedY+projectedHalfHeight-Map->m_shiftXY.y;
    return corner;
}

void REGION::Draw()
{
    const float originalX = m_xyz.x;
    const float originalY = m_xyz.y;
    const int originalFrame = m_currentFrame;
    VID* const regionVid = m_vid;
    if ((regionVid->runtimeFlags&0x800u) != 0u)
        return;
    if (regionVid != ::EmptyVid && (static_cast<unsigned int>(regionVid->type)&0x40u) != 0u && reinterpret_cast<const VID_SURFACE*>(regionVid)->frames != 0)
    {
        const float halfRegionHeight = m_halfHeight*0.5f;
        float tileY = originalY-halfRegionHeight;
        const float tileYEnd = originalY+halfRegionHeight;
        int tileIndex = 0;
        while (tileY<tileYEnd)
        {
            const float halfRegionWidth = m_halfWidth*0.5f;
            float tileX = originalX-halfRegionWidth;
            const float tileXEnd = originalX+halfRegionWidth;
            while (tileX<tileXEnd)
            {
                if ((regionVid->property&0x01000000u) == 0u)
                {
                    const int frame = originalFrame+2*tileIndex++;
                    m_currentFrame = frame%static_cast<int>(regionVid->noCadr);
                }
                const float halfTileWidth = regionVid->buildSizeX*0.5f;
                const float halfTileHeight = regionVid->buildSizeY*0.5f;
                m_xyz.x = tileX+halfTileWidth;
                m_xyz.y = tileY+halfTileHeight;
                regionVid->Draw(this);
                tileX += static_cast<float>(regionVid->vidSizeX);
            }
            tileY += static_cast<float>(regionVid->vidSizeY);
        }
    }
    m_xyz.x = originalX;
    m_xyz.y = originalY;
    m_currentFrame = originalFrame;
    const int fogEnd = m_fogEnd;
    const int fogStart = m_fogStart;
    if (fogStart >= fogEnd)
    {
        m_fogPhase = 0;
        return;
    }
    const unsigned int fogFlags = m_fogFlags;
    if ((fogFlags&1u) != 0u)
    {
        const unsigned int currentTickPhase = ::CurrentTime&7u;
        const unsigned int previousTickPhase = m_lastFogPhase;
        if (currentTickPhase<previousTickPhase)
        {
            const int phaseLimit = fogEnd*8;
            int fogPhase = m_fogPhase;
            if (fogPhase<phaseLimit)
                fogPhase += 2;
            else if (fogPhase>phaseLimit)
                fogPhase = 0;
            m_fogPhase = fogPhase;
        }
        m_lastFogPhase = currentTickPhase;
    }
    else
    {
        m_fogPhase = fogEnd*8;
    }
    GRAPH* const renderer = Graph;
    COLOR color;
    color.value = m_fogColor.value;
    unsigned short* const fogRamp = reinterpret_cast<unsigned short*>(m_fogRamp);
    const int fogPhase = m_fogPhase;
    const int blendMode = static_cast<int>(fogFlags&2u);
    if ((fogFlags&8u) != 0u)
    {
        renderer->DrawFog(renderer->ViewMin(), renderer->ViewMax(), fogStart, fogEnd, color, fogRamp, fogPhase, blendMode);
    }
    else
    {
        const VECTOR2 rightBottom = ScreenRightBottom();
        const VECTOR2 leftTop = ScreenLeftTop();
        renderer->DrawFog(leftTop, rightBottom, fogStart, fogEnd, color, fogRamp, fogPhase, blendMode);
    }
}

VID* REGION::ConvertVid(VID* sourceVid, const VECTOR& position)
{
    int drawingIterator = 0;
    SPRITE_COLLECTOR* const spriteCollector = ::Sprites;
    SPRITE* candidateSprite = spriteCollector->FirstDrawingSprite(&drawingIterator, 5);
    while (candidateSprite)
    {
        VID* const candidateVid = candidateSprite->Vid();
        const float candidateZ = candidateSprite->Z();
        if (candidateVid->spriteClass == 23u && candidateZ>position.z && candidateZ == candidateZ && position.z == position.z)
        {
            REGION* const candidateRegion = static_cast<REGION*>(candidateSprite);
            bool containsPosition = (candidateRegion->m_fogFlags&8u) != 0u;
            if (!containsPosition)
            {
                const double halfRegionWidth = static_cast<double>(candidateRegion->m_halfWidth)*0.5f;
                const float halfRegionHeight = candidateRegion->m_halfHeight*0.5f;
                const double minX = candidateRegion->m_xyz.x-halfRegionWidth;
                const float minY = candidateRegion->m_xyz.y-halfRegionHeight;
                const double maxX = candidateRegion->m_xyz.x+halfRegionWidth;
                const float maxY = candidateRegion->m_xyz.y+halfRegionHeight;
                const bool xAtLeastMin = (position.x >= minX) || position.x != position.x || minX != minX;
                const bool yAtLeastMin = (position.y >= minY) && position.y == position.y && minY == minY;
                const bool xAtMostMax = (position.x <= maxX) && position.x == position.x && maxX == maxX;
                const bool yAtMostMax = (position.y <= maxY) || position.y != position.y || maxY != maxY;
                containsPosition = xAtLeastMin && yAtLeastMin && xAtMostMax && yAtMostMax;
            }
            if (containsPosition)
            {
                for (unsigned int conversionIndex = 0;conversionIndex<static_cast<unsigned int>(VidConversionCount);++conversionIndex)
                {
                    if (candidateRegion->m_sourceVid[conversionIndex] == sourceVid)
                        return candidateRegion->m_targetVid[conversionIndex];
                }
            }
        }
        candidateSprite = ::Sprites->NextDrawingSprite(&drawingIterator, 5);
    }
    return sourceVid;
}

void REGION::SetFogParameters(int fogStart, int fogEnd, COLOR fogColor)
{
    m_fogEnd = fogEnd;
    m_fogStart = fogStart;
    m_fogColor = fogColor;
    if (m_fogRamp)
        CrtFreePointer(m_fogRamp);
    if (fogStart >= fogEnd)
        return;
    const int rampSampleCount = (fogEnd-fogStart)*8;
    m_fogRamp = static_cast<unsigned short*>(CrtNew(
        static_cast<unsigned int>(rampSampleCount)*sizeof(unsigned short)+sizeof(unsigned short)));
    if (!m_fogRamp)
        ::Error->LogExit(STRING("Enough memory for DrawFog"), rampSampleCount+1);
    const int fogRange = fogEnd-fogStart;
    int scaledIntensity = rampSampleCount*255;
    for (int sampleIndex = rampSampleCount;sampleIndex >= 0;--sampleIndex, scaledIntensity -= 255)
    {
        int dividedIntensity = scaledIntensity/fogRange;
        if (dividedIntensity<0)
            dividedIntensity += 7;
        const int rampIntensity = dividedIntensity>>3;
        m_fogRamp[sampleIndex] = static_cast<unsigned short>(rampIntensity);
    }
}

int REGION::Action(int actionCode, int firstParameter, int secondParameter, int thirdParameter)
{
    if (actionCode != ACT_SAVE && actionCode != ACT_RESTORE && actionCode != ACT_LEGACY_RESTORE)
        return SPRITE::Action(actionCode, firstParameter, secondParameter, thirdParameter);
    STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(firstParameter)));
    (void)SPRITE::Action(actionCode, firstParameter, secondParameter, thirdParameter);
    if (actionCode == ACT_SAVE)
    {
        (void)stream->Write(&m_fogFlags, 4u);
        (void)stream->Write(&m_fogEnd, 4u);
        (void)stream->Write(&m_fogStart, 4u);
        (void)stream->Write(&m_fogColor, 4u);
        (void)stream->Write(&m_halfWidth, 4u);
        (void)stream->Write(&m_halfHeight, 4u);
        (void)stream->Write(&m_094, 4u);
        int vidId = -1;
        VID* serializedVid = m_regionVid;
        (void)stream->Write(serializedVid ? &serializedVid->nVid : &vidId, sizeof(vidId));
        for (int conversionIndex = 0;conversionIndex<VidConversionCount;++conversionIndex)
        {
            vidId = -1;
            serializedVid = m_sourceVid[conversionIndex];
            (void)stream->Write(serializedVid ? &serializedVid->nVid : &vidId, sizeof(vidId));
            vidId = -1;
            serializedVid = m_targetVid[conversionIndex];
            (void)stream->Write(serializedVid ? &serializedVid->nVid : &vidId, sizeof(vidId));
        }
        return 0;
    }
    const int saveVersion = secondParameter;
    (void)stream->Read(&m_fogFlags, 4u);
    int fogEnd = secondParameter;
    int fogStart = firstParameter;
    COLOR fogColor;
    fogColor.value = static_cast<unsigned int>(thirdParameter);
    (void)stream->Read(&fogEnd, 4u);
    (void)stream->Read(&fogStart, 4u);
    (void)stream->Read(&fogColor, 4u);
    SetFogParameters(fogStart, fogEnd, fogColor);
    if (saveVersion>9)
    {
        (void)stream->Read(&m_halfWidth, 4u);
        (void)stream->Read(&m_halfHeight, 4u);
    }
    else
    {
        (void)stream->Read(&m_halfWidth, sizeof(m_halfWidth));
        (void)stream->Read(&m_halfHeight, sizeof(m_halfHeight));
        m_halfWidth = static_cast<float>(BitsToSigned32(FloatBitsU32(m_halfWidth)));
        m_halfHeight = static_cast<float>(BitsToSigned32(FloatBitsU32(m_halfHeight)));
    }
    (void)stream->Read(&m_094, 4u);
    MAP* map = Map;
    int vidId = actionCode;
    (void)stream->Read(&vidId, 4u);
    VID* sourceVid = 0;
    if (vidId >= 0 && vidId<map->m_vidCount)
        sourceVid = map->m_vids[vidId];
    if (!sourceVid)
        sourceVid = ::EmptyVid;
    m_regionVid = sourceVid;
    int targetVidId;
    for (int conversionIndex = 0;conversionIndex<VidConversionCount;++conversionIndex)
    {
        map = Map;
        (void)stream->Read(&vidId, sizeof(vidId));
        sourceVid = 0;
        if (vidId >= 0 && vidId<map->m_vidCount)
            sourceVid = map->m_vids[vidId];
        m_sourceVid[conversionIndex] = sourceVid;
        map = Map;
        (void)stream->Read(&targetVidId, sizeof(targetVidId));
        VID* targetVid = 0;
        if (targetVidId >= 0 && targetVidId<map->m_vidCount)
            targetVid = map->m_vids[targetVidId];
        m_targetVid[conversionIndex] = targetVid;
        sourceVid = m_sourceVid[conversionIndex];
        if (sourceVid)
            sourceVid->runtimeFlags |= 0x20u;
    }
    return 0;
}

extern "C" void*  RegionVectorDeletingDestructorAlias(void* self, unsigned int flags)
{
    REGION* completeRegion = static_cast<REGION*>(self);
    completeRegion->REGION::~REGION();
    if (flags&1u)
        CrtFreePointer(self);
    return self;
}

