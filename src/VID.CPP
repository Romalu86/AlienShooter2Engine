#if defined(_WIN32)
#define INPUT WIN32_INPUT
#include <windows.h>
#include "engine_globals.h"
#include <d3d9.h>
#undef INPUT
#endif
#include "myerror.h"
#include "mylib.h"
#include "vid.h"

unsigned int VID_SURFACE::noObj = 0;
unsigned int VID_SURFACE::noVertex = 0;
unsigned int VID_SURFACE::noTri = 0;
unsigned int VID_SURFACE::noDrawReal[16] = {};
unsigned int VID_SURFACE::noDrawCall[16] = {};
unsigned int VID_SURFACE::noDrawShadow = 0;
#include "list.h"
#include "texture.h"
#include "registry.h"
#include "basetype.h"
#include "region.h"
#include "message.h"
#include "sprite.h"
#include "map.h"
#include "resread.h"
#include "sprite_collector.h"
#include "graph.h"
#include "system_font.h"
#include "legacy_d3dx_effect.h"
#include "constant.h"
#include "sound.h"
#include "net.h"
#include <new>
#include <cstdlib>
#include <intrin.h>
#include <cstddef>
#include <cstdio>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


VECTOR2 VID::viewMin;
VECTOR2 VID::viewMax;
int VID::MemoryInUse = 0;
unsigned int VID::useTextureMemory = 0;
unsigned int VID::useVertexMemory = 0;
int VID::optDrawFlagmanBehindWalls = 0;
CADR_DATA* VID::lastUse = 0;


namespace
{
enum VidHardwareRuntimeFlags
{
    kVidRuntimeHidden       = 0x00000800u,
    kVidAnimateScale        = 0x00000002u,
    kVidAnimateOffset       = 0x00000004u
};
enum VidOwnerDrawFlags
{
    kVidApplyPixelCenterOffset = 0x00008000u,
    kVidApplyDirectionOffset   = 0x00010000u
};
}

int VID::GetMaxAmmo() const
{
    if (linkVid && linkVid->nWeapon && linkVid->childVid[8])
        return linkVid->weapon->MaxAmmo;
    return weapon->MaxAmmo;
}





const GAMMA& GAMMA::operator = (const GAMMA& source)
{
    modulate = source.modulate;
    add = source.add;
    return *this;
}

void __cdecl VidStaticRangeAInit()
{
    VID::viewMax.x = 0.0f;
    VID::viewMax.y = 1024.0f;
}

void __cdecl VidStaticRangeBInit()
{
    VID::viewMin.x = 0.0f;
    VID::viewMin.y = 1024.0f;
}


unsigned int __cdecl Vid_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


void VID::SetPropHideWithoutLink(int hide)
{
    runtimeFlags = (runtimeFlags&~0x00000800u)|(hide?0x00000800u:0u);
}


VECTOR VECTOR::operator-(const VECTOR& value) const
{
    return VECTOR(x-value.x, y-value.y, z-value.z);
}


VECTOR VECTOR::operator*(float scale) const
{
    return VECTOR(scale*x, scale*y, scale*z);
}


void VID::SetPropHide(int hide)
{
    const unsigned int hideBit = hide?0x00000800u:0u;
    for (VID* current = this;current;current = current->linkVid)
        current->runtimeFlags = (current->runtimeFlags&~0x00000800u)|hideBit;
}


static int ScaleHpRatioFpu(int currentHp, int newMaxHp, int oldMaxHp)
{

    const double product = static_cast<double>(currentHp)*static_cast<double>(newMaxHp);
    return TruncateDoubleToInt32(product/static_cast<double>(oldMaxHp));
}

static int TruncateFixedPointTowardZero(int value, unsigned int fractionalBits)
{
    if (value >= 0) return value>>fractionalBits;
    return -static_cast<int>((0u-static_cast<unsigned int>(value))>>fractionalBits);
}

static int ScaleHpRatioFixedShift4(int currentHp, int newMaxHp, int oldMaxHp)
{
    const unsigned int productBits = static_cast<unsigned int>(currentHp)*static_cast<unsigned int>(newMaxHp);
    const int quotient = static_cast<int>(productBits)/oldMaxHp;
    return TruncateFixedPointTowardZero(quotient, 4u);
}

static int ScaleHpRatioFixedShift8(int currentHp, int newMaxHp, int oldMaxHp)
{
    unsigned int productBits = static_cast<unsigned int>(currentHp)*static_cast<unsigned int>(newMaxHp);
    productBits <<= 8;
    const int quotient = static_cast<int>(productBits)/oldMaxHp;
    return TruncateFixedPointTowardZero(quotient, 8u);
}

static SPRITE* FirstAllSpriteForVid()
{
    SPRITE_COLLECTOR* collector = ::Sprites;
    return static_cast<SPRITE*>(collector->FirstAllSprite());
}

static SPRITE* NextAllSpriteForVid()
{
    SPRITE_COLLECTOR* collector = ::Sprites;
    return static_cast<SPRITE*>(collector->NextAllSprite());
}

void VID::SetHpCoeff(unsigned int direction, int hpCoeff)
{
    const unsigned int dir = static_cast<unsigned int>(direction)&7u;
    VID* vid = this;
    while (vid)
    {
        int oldMaxHp = vid->maxHpByPlayer[dir];
        if (oldMaxHp <= 1) oldMaxHp = 1;

        if (hpCoeff >= 0)
        {

            const int baseHp = vid->maxHp;
            int newMaxHp = static_cast<int>(static_cast<unsigned int>(baseHp)*static_cast<unsigned int>(hpCoeff));
            newMaxHp /= 100;
            if (newMaxHp <= 1) newMaxHp = 1;
            vid->maxHpByPlayer[dir] = newMaxHp;
        }

        if (vid->maxHp != 0)
        {
            SPRITE* sprite = FirstAllSpriteForVid();
            while (sprite)
            {
                if (sprite->Vid() == vid && ((sprite->RuntimeFlags()>>17)&7u) == dir)
                {
                    const int newMaxHp = vid->maxHpByPlayer[dir];
                    const int currentHp = sprite->m_hp;
                    int scaledHp;
                    if (newMaxHp>5000)
                        scaledHp = ScaleHpRatioFpu(currentHp, newMaxHp, oldMaxHp);
                    else
                        scaledHp = ScaleHpRatioFixedShift4(currentHp, newMaxHp, oldMaxHp);
                    if (scaledHp <= 1) scaledHp = 1;
                    sprite->ChangeHp(scaledHp);
                }
                sprite = NextAllSpriteForVid();
            }
        }

        vid = vid->linkVid;
    }
}

void VID::SetMaxHp(unsigned int direction, int hp)
{
    const unsigned int dir = static_cast<unsigned int>(direction)&7u;
    VID* vid = this;
    while (vid)
    {
        int oldMaxHp = vid->maxHpByPlayer[dir];
        if (oldMaxHp <= 1) oldMaxHp = 1;
        if (hp >= 0) vid->maxHpByPlayer[dir] = hp;

        if (vid->maxHp != 0)
        {
            SPRITE* sprite = FirstAllSpriteForVid();
            while (sprite)
            {
                if (sprite->Vid() == vid && ((sprite->RuntimeFlags()>>17)&7u) == dir)
                {
                    const int newMaxHp = vid->maxHpByPlayer[dir];
                    const int currentHp = sprite->m_hp;
                    const int scaledHp = (newMaxHp>5000)
                        ? ScaleHpRatioFpu(currentHp, newMaxHp, oldMaxHp)
                        : ScaleHpRatioFixedShift8(currentHp, newMaxHp, oldMaxHp);
                    sprite->ChangeHp(scaledHp);
                }
                sprite = NextAllSpriteForVid();
            }
        }

        vid = vid->linkVid;
    }
}

static bool DirectionQuantizationOverrideEnabled()
{

    NET* const state = ::Net;
    return state != 0 && (state->m_flags&0x10u) != 0u;
}

int VID::RealDirectionWithGraphDetail(unsigned char direction)
{
    const unsigned int wrapped = (static_cast<unsigned int>(directionQuantizationOffset)+static_cast<unsigned int>(direction))&0xFFu;
    const int directionCount = noDir;

    if ((runtimeFlags&0x00000200u) != 0u)
    {
        GRAPH* const camera = Graph;
        if (static_cast<int>(camera->graphDetail)<5 || DirectionQuantizationOverrideEnabled())
        {
            if (directionCount == 32)
                return static_cast<int>(((wrapped<<5)>>8)&0xFFFCu);
            return static_cast<int>(((wrapped*static_cast<unsigned int>(directionCount))>>8)&0xFFFEu);
        }
    }

    return static_cast<int>((wrapped*static_cast<unsigned int>(directionCount))>>8);
}

void VID::ResetSprites()
{
    for (int i = 0;i<ScriptFunctionCount;++i)
        scriptFunction[i] = -1;

    for (int player = 0;player<PlayerSlotCount;++player)
    {
        spriteLimitByPlayer[player] = -1;
        spriteCountsByPlayer[player] = 0u;
        killedByPlayer[player] = 0;
        reCapturedByPlayer[player] = 0;
        maxHpByPlayer[player] = maxHp;
    }

    spriteLimit = -1;
    lastSpriteCountChangeTick = 0u;
    runtimeFlags &= ~0x20u;
}


int VID::GetBuildTime() const
{
    const VID* source = (linkVid && linkVid->nWeapon != 0)?linkVid:this;
    return source->weapon->BuildTime/1000;
}

void VID_SURFACE::ReleaseVideoMemory()
{
    if (!frames || (surfaceFlags&1u) != 0u) return;
    for (int i = 0;i<static_cast<int>(noCadr);++i)
        frames[i].ReleaseVideoMemory();
}

void VID_SURFACE::SetFramesToDevice()
{
    if (noCadr>0)
    {
        int frameIndex = 0;
        do
        {
            frames[frameIndex].SetTextureToDevice(this, 0);
            frames[frameIndex].SetVertexToDevice(this);
            ++frameIndex;
        }
        while (frameIndex<noCadr);
    }
}

CADR_DATA::CADR_DATA()
{
    lastUseTick = 0u;
    lruPrevious = 0u;
    lruNext = 0u;
    indexCount = 0u;
    vertexCount = 0u;
    width = 0u;
    height = 0u;
    offsetX = 0u;
    offsetY = 0u;
    maxZ = 10.0f;
    minZ = -999999.0f;
    textureObject = 0u;
    vertexBuffer = 0u;
    indexBuffer = 0u;
}

CADR_DATA::~CADR_DATA() noexcept(false)
{
    if (textureObject)
    {
        TEXTURE* const object = textureObject;
        delete object;
        textureObject = 0u;
    }
    if (indexBuffer)
    {
        IUnknown* const object = indexBuffer;
        (void)object->Release();
        indexBuffer = 0u;
    }
    if (vertexBuffer)
    {
        IUnknown* const object = vertexBuffer;
        (void)object->Release();
        vertexBuffer = 0u;
    }
}


void __cdecl InitializeVidFirstRangeAtStartup()
{
    VidStaticRangeAInit();
}

void __cdecl InitializeVidSecondRangeAtStartup()
{
    VidStaticRangeBInit();
}


void* __cdecl InitializeVidRootAtStartup()
{
    return VidInitializeGlobalRoot();
}


int VID::GetFireDamageMax() const
{
    const int linkedDamage = linkVid?linkVid->GetFireDamageMax():0;
    const int child15Damage = childVid[15]
        ? MultiplySigned32Wrap(childVid[15]->GetFireDamageMax(), noChild[15]):0;
    const int child14Damage = childVid[14]
        ? MultiplySigned32Wrap(childVid[14]->GetFireDamageMax(), noChild[14]):0;
    const int child8Damage = childVid[8]
        ? MultiplySigned32Wrap(childVid[8]->GetFireDamageMax(), noChild[8]):0;
    int result = AddSigned32Wrap(deathDamageMax, child8Damage);
    result = AddSigned32Wrap(result, child14Damage);
    result = AddSigned32Wrap(result, child15Damage);
    return AddSigned32Wrap(result, linkedDamage);
}


void* __cdecl VidInitializeGlobalRoot()
{
    VID* object = static_cast<VID*>(CrtNew(sizeof(VID)));
    if (object) new(object) VID;
    static_cast<void>(::EmptyVid = object);
    return object;
}

unsigned int VID::CalcImportantForSync()
{
    if ((runtimeFlags&0x1000u) != 0u) return 1u;

    if ((spriteType&0x20u) != 0u || spriteClass == 19u)
    {
        runtimeFlags &= ~0x1000u;
        return 0u;
    }

    if (spriteClass == 2u || spriteClass == 7u || spriteClass == 3u ||
       (spriteType&0x18u) != 0u || deathDamageMin != 0 || deathDamageMax != 0 ||
       (property&0x68u) != 0u)
    {
        runtimeFlags |= 0x1000u;
        return 1u;
    }

    if ((runtimeFlags&0x2000u) != 0u) return 0u;
    runtimeFlags |= 0x2000u;

    if (linkVid && linkVid->CalcImportantForSync() != 0u)
    {
        runtimeFlags = (runtimeFlags&~0x2000u)|0x1000u;
        return 1u;
    }

    for (int i = 0;i<AnimationSlotCount;++i)
    {
        if (childVid[i] && childVid[i]->CalcImportantForSync() != 0u)
        {
            runtimeFlags = (runtimeFlags&~0x2000u)|0x1000u;
            return 1u;
        }
    }

    runtimeFlags &= ~0x2000u;
    return (runtimeFlags>>12)&1u;
}

void CADR_DATA::CalcMaxMinZ(const VID_SURFACE* surface)
{
    if (FloatBitsU32(minZ) != 0xC97423F0u) return;
    const unsigned int vertexCount = this->vertexCount;
    if (vertexCount == 0u)
    {
        minZ = 0.0012f;
        return;
    }

    const unsigned int frameBlockOffset = frameDataOffset;
    const unsigned int indexWordCount = indexCount;
    const unsigned char* const data = surface->surfaceData;
    maxZ = 0.0f;
    minZ = 999.0f;
    const unsigned char* const vertexData = data+indexWordCount*2u+frameBlockOffset;
    const float bias = 32.0f;
    for (unsigned int i = 0;i<vertexCount;++i)
    {
        const unsigned int packedVertex = *reinterpret_cast<const unsigned int*>(vertexData+i*4u);
        const unsigned int mantissa = (packedVertex>>20)&0x3FFu;
        const unsigned int divisor = 1u<<(packedVertex>>30);
        const float value = static_cast<float>(mantissa)/static_cast<float>(divisor)-bias;
        if (value>maxZ) maxZ = value;
        if (!(value >= minZ)) minZ = value;
    }
}

static int ClampByteToRange(int value)
{
    if (value<0)return 0;
    if (value>255)return 255;
    return value;
}

COLOR::COLOR(const GAMMA& gamma, const COLOR& baseColor)
{
    if (gamma.modulate == 0u && gamma.add == 0u)
    {
        value = baseColor.value;
        return;
    }

    const unsigned int inverse = ~gamma.modulate;
    const unsigned int add = gamma.add;
    const unsigned int base = baseColor.value;
    int blue = ((static_cast<int>(inverse&0xFFu)+1)*static_cast<int>(base&0xFFu)>>8)+static_cast<int>(add&0xFFu);
    int green = ((static_cast<int>((inverse>>8)&0xFFu)+1)*static_cast<int>((base>>8)&0xFFu)>>8)+static_cast<int>((add>>8)&0xFFu);
    int red = ((static_cast<int>((inverse>>16)&0xFFu)+1)*static_cast<int>((base>>16)&0xFFu)>>8)+static_cast<int>((add>>16)&0xFFu);
    blue = ClampByteToRange(blue);
    green = ClampByteToRange(green);
    red = ClampByteToRange(red);
    value = 0xFF000000u|(static_cast<unsigned int>(red)<<16)|
        (static_cast<unsigned int>(green)<<8)|static_cast<unsigned int>(blue);
}





float VID::CalculateZSpeed(float numerator, float denominator) const
{
    const float scale = maxSpeed;
    float result;

    if (scale == 999999.0f)
        result = 0.0f;
    else if ((property&2u) != 0u || (property&4u) != 0u)
    {
        const float graphScale = (property&2u) != 0u ? Const->Gravitation : Const->Gravitation2;
        result = (denominator*graphScale/scale*0.5f)+(numerator*scale/denominator);
        result *= result>0.0f ? 1.1f : 0.9f;
    }
    else if ((property&0x08000000u) != 0u)
        result = maxZSpeed;
    else if (!FloatEqualOrUnordered(topZ, 0.0f) && spriteClass != 5u)
        result = 0.0f;
    else if (acceleration == 999999.0f)
        result = numerator*scale/denominator;
    else
        result = (numerator*scale)/((scale*scale*0.5f/acceleration)+denominator);

    if (result>maxZSpeed) return maxZSpeed;
    if (!(result >= -maxZSpeed)) result = -maxZSpeed;
    return result;
}

void VID::SetGamma(const GAMMA& gamma, int slot)
{
    GAMMA value;
    value.modulate = gamma.modulate;
    value.add = gamma.add;
    if (slot<0)
    {
        gammaRaw = value;
        for (int i = 0;i<PlayerSlotCount;++i) gammaByPlayer[i] = value;
        return;
    }

    if (slot<PlayerSlotCount)
    {
        gammaByPlayer[slot] = value;
        return;
    }

    const STRING message("n_gamma in VID::SetGamma");
    Error(E_INVALID, message, static_cast<unsigned int>(slot));
}


void CADR_DATA::ReleaseVideoMemory()
{
    if (textureObject)
    {
        TEXTURE* const texture = textureObject;
        const TEXTURE& info = *texture;
        const unsigned int pixelCount = info.m_width*info.m_height;
        unsigned int bytes = 0u;
        if (info.m_format == D3DFMT_DXT1)
            bytes = static_cast<unsigned int>(BitsToSigned32(pixelCount)/2);
        else
        {
            unsigned int bytesPerPixel = 2u;
            if (info.m_format == D3DFMT_P8 || info.m_format == D3DFMT_L8) bytesPerPixel = 1u;
            else if (info.m_format == D3DFMT_A8R8G8B8) bytesPerPixel = 4u;
            else if (info.m_format == D3DFMT_R8G8B8) bytesPerPixel = 3u;
            bytes = pixelCount*bytesPerPixel;
        }
        static_cast<void>(VID::useTextureMemory = VID::useTextureMemory-bytes);
        if (textureObject)
        {
            delete textureObject;
            textureObject = 0u;
        }

        if (lruPrevious)
        {
            if (VID::lastUse == this)
                static_cast<void>(VID::lastUse = lruNext);
            lruPrevious->lruNext = lruNext;
            lruNext->lruPrevious = lruPrevious;
            lruNext = 0u;
            lruPrevious = 0u;
            if (VID::lastUse == this)
                static_cast<void>(VID::lastUse = 0);
        }
    }

    if (indexBuffer)
    {
        IUnknown* const object = indexBuffer;
        (void)object->Release();
        indexBuffer = 0u;
    }

    if (vertexBuffer)
    {
        const unsigned int displayValue = Graph->vertexShaderVersion;
        const unsigned int bytesPerEntry = displayValue<256u ? 20u : 12u;
        static_cast<void>(VID::useVertexMemory = VID::useVertexMemory-static_cast<unsigned int>(vertexCount)*bytesPerEntry);
        if (vertexBuffer)
        {
            IUnknown* const object = vertexBuffer;
            (void)object->Release();
            vertexBuffer = 0u;
        }
    }
}



static int DrawRichTextRun(
    const VID_FONT* font, int x, int y, const char* text, int length, unsigned int color)
{
    ID3DXFont* const renderer = font->fontBackend;
    RECT rect = {x, y, AddSigned32Wrap(x, 1), AddSigned32Wrap(y, 1)};
    const char* drawText = text;
    int drawLength = length;
    char prefixed[4096] = {};

    if (text[0] == 'W')
    {
        const int cellWidth = static_cast<int>(font->vidSizeX);
        rect.left = BitsToSigned32(Signed32ToBits(x)-Signed32ToBits(cellWidth/2));
        prefixed[0] = ' ';
        unsigned int i = 0u;
        do
        {
            prefixed[i+1u] = text[i];
        }
        while (text[i++] != '\0');
        drawText = prefixed;
        drawLength = AddSigned32Wrap(length, 1);
    }

    return renderer->DrawTextA(
        drawText, drawLength, &rect, (DT_NOCLIP|DT_SINGLELINE|DT_NOPREFIX), color);
}

void VID::Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const
{
    const STRING objectFormat("VID [%i-%s]");
    ::Error->Error(objectFormat, type, subject, code, nVid, name.CharPtr());
}

VID::~VID() noexcept(false)
{

    unsigned int totalSprites = 0u;
    for (int i = 0;i<PlayerSlotCount;++i) totalSprites += spriteCountsByPlayer[i];
    if (totalSprites != 0u)
    {
        const STRING message("Not all sprites with this VID deleted");
        Error(E_ERROR, message, totalSprites);
    }

    if (nextMirror != this)
    {
        VID* predecessor = nextMirror;
        while (predecessor->nextMirror != this) predecessor = predecessor->nextMirror;
        predecessor->nextMirror = nextMirror;
    }
    else
    {
        if ((storageFlags&8u) == 0u && cadrLinkXYZ)
            CrtFreePointer(cadrLinkXYZ);
        cadrLinkXYZ = 0;

        if ((storageFlags&8u) == 0u || (property&8u) != 0u)
        {
            if (gridZ)
            {
                CrtFreePointer(gridZ);
                gridZ = 0;
            }
            if (gridCadrShift)
            {
                CrtFreePointer(gridCadrShift);
                gridCadrShift = 0;
            }
        }
    }

}
static void VidReportMissingGridTransform(const VID* vid)
{
    const STRING text("not exData for SetGrid");
    vid->Error(E_ERROR, text, 0u);
}

void VID::ResetGridZ(const SPRITE* sprite)
{
    const SPRITE* const mouse =
        Mouse;
    if (sprite == mouse || noGridZ == 0) return;

    EX_SPRITE_DATA* transform = sprite->ExtraData();
    if (!transform)
    {
        VidReportMissingGridTransform(this);
        transform = sprite->ExtraData();
    }

    const int frame = BitsToSigned32(transform->reserved38);
    if (frame<0) return;

    if (sprite->m_vid->spriteClass == 8u) return;

    int begin = 0;
    int end = noGridZ;
    if (gridCadrShift)
    {
        const int frameCount = static_cast<int>(noCadr);
        begin = (frame<frameCount)?gridCadrShift[frame]:0;
        end = (frame<frameCount-1)?gridCadrShift[frame+1]:noGridZ;
    }
    if (begin >= end) return;

    const VECTOR scale = transform->scale;
    for (int i = begin;i<end;++i)
    {
        const VECTOR& dot = gridZ[i];
        const float scaledY = dot.y*scale.y;
        const float scaledZ = dot.z*scale.z;
        VECTOR point(
            static_cast<float>(static_cast<double>(dot.x)*static_cast<double>(scale.x)+
                static_cast<double>(sprite->m_xyz.x)),
            sprite->m_xyz.y+scaledY,
            sprite->m_xyz.z+scaledZ);
        Map->ClearTempGroundZ(point);
    }
}

void VID::SetGridZ(const SPRITE* sprite)
{
    const SPRITE* const mouse =
        Mouse;
    if (sprite == mouse || noGridZ == 0) return;

    EX_SPRITE_DATA* transform = sprite->ExtraData();
    if (!transform)
    {
        VidReportMissingGridTransform(this);
        transform = sprite->ExtraData();
    }

    transform->reserved38 = sprite->m_currentFrame;
    const int frame = sprite->m_currentFrame;

    int begin = 0;
    int end = noGridZ;
    if (gridCadrShift)
    {
        const int frameCount = static_cast<int>(noCadr);
        begin = (frame<frameCount)?gridCadrShift[frame]:0;
        end = (frame<frameCount-1)?gridCadrShift[frame+1]:noGridZ;
    }
    if (begin >= end) return;

    const VECTOR scale = transform->scale;
    const bool permanent = sprite->m_vid->spriteClass == 8u;
    for (int i = begin;i<end;++i)
    {
        const VECTOR& dot = gridZ[i];
        const float scaledY = dot.y*scale.y;
        const float scaledZ = dot.z*scale.z;
        VECTOR point(
            static_cast<float>(static_cast<double>(dot.x)*static_cast<double>(scale.x)+
                static_cast<double>(sprite->m_xyz.x)),
            sprite->m_xyz.y+scaledY,
            sprite->m_xyz.z+scaledZ);
        if (permanent) Map->SetGroundZ(point);
        else Map->SetTempGroundZ(point);
    }
}

VID_FONT::VID_FONT(VID* source, const STRING& fontName) : VID()
{
    frameSpeedDefault = 71u;
    noCadr = 256;
    vidSizeX = static_cast<short>(TruncateFloatToInt32(source->sizeXYZ.x));
    vidSizeY = static_cast<short>(TruncateFloatToInt32(source->sizeXYZ.y));
    buildSizeX = source->sizeXYZ.x;
    buildSizeY = source->sizeXYZ.y;
    type = 0x4000u;
    vidName = fontName;

    LOGFONTA descriptor;

    HDC const dc = ::GetDC(0);
    const int dpiY = ::GetDeviceCaps(dc, LOGPIXELSY);
    const int scaledDpiY = TruncateFloatToInt32(static_cast<float>(dpiY)*source->scaleXYZ.y);
    descriptor.lfHeight = BitsToSigned32(0u-Signed32ToBits(::MulDiv(static_cast<int>(vidSizeY), scaledDpiY, 72)));
    const int dpiX = ::GetDeviceCaps(dc, LOGPIXELSX);
    const int scaledDpiX = TruncateFloatToInt32(static_cast<float>(dpiX)*source->scaleXYZ.x);
    descriptor.lfWidth = BitsToSigned32(0u-Signed32ToBits(::MulDiv(static_cast<int>(vidSizeX), scaledDpiX, 72)));

    descriptor.lfEscapement = 0;
    descriptor.lfOrientation = 0;
    descriptor.lfWeight = (property&0x00200000u)?700:400;
    descriptor.lfItalic = (property&0x00400000u)?1u:0u;
    descriptor.lfUnderline = 0u;
    descriptor.lfStrikeOut = 0u;
    descriptor.lfCharSet = 1u;
    descriptor.lfOutPrecision = 0u;
    descriptor.lfClipPrecision = 0u;
    descriptor.lfQuality = 4u;
    descriptor.lfPitchAndFamily = 1u;
    const char* const face = fontName.value;
    const unsigned int faceBytes = static_cast<unsigned int>(strlen(face))+1u;
    __movsd(reinterpret_cast<unsigned long*>(descriptor.lfFaceName),
        reinterpret_cast<const unsigned long*>(face), faceBytes>>2u);
    __movsb(reinterpret_cast<unsigned char*>(descriptor.lfFaceName)+(faceBytes&~3u),
        reinterpret_cast<const unsigned char*>(face)+(faceBytes&~3u), faceBytes&3u);
    (void)::ReleaseDC(0, dc);

    GRAPH* const graph = Graph;
    const HRESULT hr = D3DXCreateFontIndirect(graph->device, &descriptor, &fontBackend);
    if (hr<0)
    {
        STRING subject = "font"+fontName;
        Error(E_CREATE, subject, static_cast<unsigned long>(hr));
        return;
    }

    RECT rect = {0, 0, 1, 1};
    static const char measureText[] = "X";
    (void)fontBackend->DrawTextA(measureText, 1, &rect, 0x0D20u, 0xFFFFFFFFu);
    vidSizeX = static_cast<short>(rect.right);
    vidSizeY = static_cast<short>(rect.bottom);
}

VID_SURFACE::VID_SURFACE(VID* source, RESOURCE* resource, const STRING& name)
{
    unsigned int flags = surfaceFlags;
    flags = (flags&0xFFFFFF00u)|(((flags&0xFFu)&0xFEu)|2u);
    this->surfaceBackend = 0;
    this->frames = 0;
    this->surfaceData = 0;
    this->palette = 0;
    this->surfaceFlags = flags;
    this->recolorPalette = 0;
    (void)(this->vidName = name);
    this->type = 0x41u;
    this->noCadr = (source && source->spriteClass == 19u)?256:1;

    if (this->noCadr == 1)
    {

        const STRING stem = name.BeforeLast(".");
        const int stemLength = stem.Length();
        const int stemTail = (stemLength != 0)?static_cast<signed char>(stem[stemLength-1]):0;
        if (isdigit(stemTail) != 0)
        {
            const STRING suffix = name.AfterLast("_");
            if (isdigit(static_cast<signed char>(suffix[0])) != 0)
            {
                for (;;)
                {
                    bool found;
                    {
                        const STRING candidate = name.Add(this->noCadr);
                        std::FILE* file = 0;
                        if (!candidate.IsEmpty())
                            file = std::fopen(candidate.CharPtr(), "rb");
                        if (file) (void)std::fclose(file);
                        found = file != 0;
                    }
                    if (!found) break;
                    ++this->noCadr;
                }
            }
        }
    }

    if (resource) LoadParameters(resource);

    this->surfaceMode = (!(this->sizeXYZ.z>this->sizeXYZ.y))?1:0;
    this->renderMode = 0u;

    D3DXImageInfo imageInfo;
    (void)D3DXGetImageInfoFromFileA(name.CharPtr(), &imageInfo);
    this->pixelFormat = imageInfo.Format;

    unsigned int imageWidth = imageInfo.Width;
    unsigned int imageHeight = imageInfo.Height;
    if (this->spriteClass == 19u)
    {
        imageWidth = (imageWidth>>4u)-1u;
        imageHeight = (imageHeight>>4u)-1u;
    }
    this->vidSizeX = static_cast<short>(imageWidth);
    this->vidSizeY = static_cast<short>(imageHeight);
    this->buildSizeX = static_cast<float>(this->vidSizeX);
    this->buildSizeY = static_cast<float>(this->vidSizeY);

    if (this->pixelFormat == D3DFMT_A8R8G8B8 || this->pixelFormat == D3DFMT_A4R4G4B4 ||
       this->pixelFormat == D3DFMT_A1R5G5B5 || this->pixelFormat == D3DFMT_DXT3 ||
       this->pixelFormat == D3DFMT_DXT5)
        this->type = static_cast<unsigned short>(this->type|2u);

    const int frameCount = static_cast<int>(this->noCadr);
    this->frames = new(std::nothrow) CADR_DATA[frameCount];
    for (int frameIndex = 0;frameIndex<frameCount;++frameIndex)
        this->frames[frameIndex].height = 1u;

}

static bool ResolveVidLinkTarget(int index, VID*& target)
{
    MAP* const application = Map;
    VID* const source = application->StoredVid(index);
    if (!source) return false;
    target = source->exchangedVid;
    return true;
}

static void ReportVidReferenceError(const VID* owner, const char* subjectText, int value)
{
    if (g_iZmDebug != 0u) return;
    STRING subject;
    subject = subjectText;
    owner->Error(E_INVALID, subject, static_cast<unsigned long>(value));
}


void VID::SetChildAndLink()
{
    if (nLinkVid != 0)
    {
        VID* resolved = 0;
        if (ResolveVidLinkTarget(nLinkVid, resolved)) linkVid = resolved;
        else ReportVidReferenceError(this, "LinkVid", nLinkVid);
    }

    const WEAPON_RUNTIME* const weaponState = weapon;
    if (weaponState->maxColumn >= 0)
    {
        for (int lane = 0; lane <= weaponState->maxColumn; ++lane)
        {
            if (weaponState->GammaR[lane] != 0 ||
               weaponState->GammaG[lane] != 0 ||
               weaponState->GammaB[lane] != 0 ||
               weaponState->GammaA[lane] != 0)
                runtimeFlags |= 0x01u;

            const VECTOR& lane0 = weaponState->Scale[lane];
            if (lane0.x != 1.0f || lane0.y != 1.0f || lane0.z != 1.0f)
                runtimeFlags |= 0x02u;

            const VECTOR& lane1 = weaponState->Shift[lane];
            if (!FloatEqualOrUnordered(lane1.x, 0.0f) ||
               !FloatEqualOrUnordered(lane1.y, 0.0f) ||
               !FloatEqualOrUnordered(lane1.z, 0.0f))
                runtimeFlags |= 0x04u;

            if (weaponState->DirectionX[lane] != 0 ||
               weaponState->DirectionY[lane] != 0 ||
               weaponState->DirectionZ[lane] != 0)
                runtimeFlags |= 0x08u;

            if (!FloatEqualOrUnordered(weaponState->Speed[lane], 0.0f))
                runtimeFlags |= 0x10u;
        }
    }

    if (lifeTime != 999999 ||
       (property&0x00200000u) != 0u ||
       !FloatEqualOrUnordered(maxSpeed, randomSpeed) ||
       noGridZ != 0 ||
       (property&0x00182000u) != 0u ||
       (runtimeFlags&0x1Fu) != 0u ||
       (nWeapon != 0 && childVid[8] != 0) ||
       (property&0x28u) != 0u)
        actionAuxStateRequired = 1;

    if ((spriteClass == 2u || spriteClass == 7u) && (noDir == 16 || noDir == 32))
    {
        runtimeFlags |= 0x200u;
        if (linkVid && (linkVid->noDir == 16 || linkVid->noDir == 32))
            linkVid->runtimeFlags |= 0x200u;

        for (int slot = 15; slot <= 16; ++slot)
            if (childVid[slot] && (childVid[slot]->noDir == 16 || childVid[slot]->noDir == 32))
                childVid[slot]->runtimeFlags |= 0x200u;
    }

    if (!FloatEqualOrUnordered(weaponState->Weight, 999999.0f))
        movementTactEnabled |= 1;

    for (int slot = 0; slot<AnimationSlotCount; ++slot)
    {
        const int sourceChild = nChildVid[slot];
        if (sourceChild == 0) continue;

        const unsigned int childIndexBits = static_cast<unsigned int>(sourceChild);
        const unsigned int signMask = 0u-(childIndexBits>>31u);
        const int childIndex = static_cast<int>((childIndexBits^signMask)-signMask);
        VID* resolved = 0;
        if (ResolveVidLinkTarget(childIndex, resolved))
        {
            childVid[slot] = resolved;
            if ((resolved->property&0x80u) != 0u || (resolved->property&0x00080000u) != 0u)
                actionAuxStateRequired = 1;
        }
        else
            ReportVidReferenceError(this, "child", sourceChild);
    }

    runtimeFlags = (runtimeFlags&~0x100u)|((nWeapon != 0 && childVid[8] != 0)?0x100u:0u);
}

static void VidReportResourceError(const VID* owner,TYPE_ERROR type,const char* text,int value);
static VID_FONT* SelectRichTextFontVid(VID_FONT* currentFont,int index,bool requireValid);
static bool RichTextTagStartsWith(const char* p,const char* literal);

static void MeasureRichTextRun(const VID_FONT* selectedFont, const char* begin, int length, unsigned int color, RECT* rect)
{
    (void)selectedFont->fontBackend->DrawTextA(begin, length, rect, 0x0D20u, color);
}
static void AppendRichTextLineOffset(LIST<int>* lines, int width)
{
    if (lines->m_count >= lines->m_capacity)
    {
        const int newCapacity = lines->m_capacity*2+4;
        lines->Expand(newCapacity);
    }
    lines->m_data[lines->m_count++] = width;
}
void VID_FONT::CalcTextSize(const STRING& textValue, int* outMaxWidth, int* outHeight,
    int* outLineCount, LIST<int>* lineOffsets, unsigned long alignFlags) const
{
    VID_FONT* const owner = const_cast<VID_FONT*>(this);
    if (!owner->fontBackend)
    {
        VidReportResourceError(owner, E_ERROR, "not font for CalcText", 0);
        return;
    }

    *outMaxWidth = 0;
    *outHeight = 0;
    *outLineCount = 0;
    lineOffsets->m_count = 0;
    lineOffsets->m_capacity = 0;
    if (lineOffsets->m_data) ::operator delete(lineOffsets->m_data);
    lineOffsets->m_data = 0;

    const STRING* const textObject = &textValue;
    const char* const text = textObject->m_value;
    VID_FONT* selected = owner;
    int lineWidth = 0;
    int runLength = 0;
    RECT measureRect = {0, 0, 1, 1};

    for (int index = 0;;++index)
    {
        const char ch = text[index];
        if (ch == '\0')
        {
            if (runLength>0)
            {
                MeasureRichTextRun(selected, text+index-runLength, runLength, 0xFFFFFFFFu, &measureRect);
                lineWidth += measureRect.right;
                *outHeight += measureRect.bottom;
            }
            ++*outLineCount;
            AppendRichTextLineOffset(lineOffsets, lineWidth);
            if (lineWidth>*outMaxWidth) *outMaxWidth = lineWidth;
            break;
        }

        const bool lineBreak = (ch == '\n' || ch == '\r');
        const bool alignTag = ch == '<' && RichTextTagStartsWith(text+index, "<Align=");
        const bool fontTag = ch == '<' && RichTextTagStartsWith(text+index, "<Font=");
        const bool colorTag = ch == '<' && RichTextTagStartsWith(text+index, "<Color=");

        if (alignTag || fontTag || colorTag || lineBreak)
        {
            if (runLength>0)
            {
                MeasureRichTextRun(selected, text+index-runLength, runLength, 0xFFFFFFFFu, &measureRect);
                if (!alignTag) lineWidth += measureRect.right;
            }

            if (alignTag)
            {
                lineWidth = static_cast<int>(selected->vidSizeX)*BitsToSigned32(CrtAtoiText(text+index+7));
            }
            else if (fontTag)
            {
                VID_FONT* const candidate = SelectRichTextFontVid(selected, BitsToSigned32(CrtAtoiText(text+index+6)), false);
                if (candidate) selected = candidate;
            }
            else if (lineBreak)
            {
                *outHeight += static_cast<int>(selected->vidSizeY);
                ++*outLineCount;
                AppendRichTextLineOffset(lineOffsets, lineWidth);
                if (lineWidth>*outMaxWidth) *outMaxWidth = lineWidth;
                lineWidth = 0;
                ++index;
            }

            runLength = -1;
            if (alignTag || fontTag || colorTag)
            {
                while (text[index] && text[index] != '>') ++index;
            }
        }
        ++runLength;
    }

    for (int i = 0;i<lineOffsets->m_count;++i)
    {
        if ((alignFlags&1u) != 0u) lineOffsets->m_data[i] = -(lineOffsets->m_data[i]/2);
        else if ((alignFlags&2u) != 0u) lineOffsets->m_data[i] = -lineOffsets->m_data[i];
        else lineOffsets->m_data[i] = 0;
    }
}

namespace {
static VECTOR EvaluateVidHardwareCurve(
    const WEAPON_RUNTIME* owner,const VECTOR* keys,float phase);
static int VidHardwareHalfTowardZero(int value);
}

namespace {
struct VidShadowVertex
{
    float x;
    float y;
    float z;
    float rhw;
    float u;
    float v;
};

static bool VidShadowOutsideRuntimeBounds(const VID* owner, float x, float y, const VECTOR& scale)
{
    const float extentX = owner->buildSizeX*(0.5f)*scale.x;
    const float extentY = owner->buildSizeY*(0.5f)*scale.y;
    if (x-extentX>VID::viewMax.x &&
       y-extentY>VID::viewMax.y)
        return true;
    if (!(x+extentX >= VID::viewMin.x) &&
       !(y+extentY >= VID::viewMin.y))
        return true;
    return false;
}



static bool VidShadowUsesDirectionalQuad(const CADR_DATA* frame)
{
    const int width = static_cast<int>(static_cast<short>(frame->width));
    const int height = static_cast<int>(static_cast<short>(frame->height));
    const int maximum = width>height?width:height;
    const float threshold = frame->maxZ*(1.79999995f);
    return threshold<static_cast<float>(maximum);
}
}

void VID_SURFACE::DrawShadow(const SPRITE* spriteValue, const VECTOR& offset)
{
    VID_SURFACE* const self = this;

    CADR_DATA* const frameBase = self->frames;
    if (!frameBase || (self->runtimeFlags&kVidRuntimeHidden) != 0u) return;

    const SPRITE* const sprite = spriteValue;
    const VECTOR* const shadowOrigin = &offset;

    const float directionX = shadowOrigin->x-sprite->Coor().x;
    const float directionY = shadowOrigin->y-sprite->Coor().y;
    const unsigned char shadowDirection = static_cast<unsigned char>(
        Decart2Polar(directionX, directionY).value+128u);
    unsigned char drawDirection = shadowDirection;

    CADR_DATA* frame = 0;
    bool directionalQuad = false;
    const unsigned int ownerProperty = self->property;
    if ((ownerProperty&0x20000000u) != 0u)
    {
        const int relativeFrame = static_cast<int>(sprite->CurrentFrame())-sprite->m_currentFrameBegin;
        const unsigned char relativeDirection = static_cast<unsigned char>(
            sprite->Direction().value-shadowDirection);
        const int realDirection = self->RealDirectionWithGraphDetail(relativeDirection);
        const int animation = sprite->Animation();
        const int frameIndex = relativeFrame+self->animationBaseFrame[animation]+
            realDirection*self->animationFrameCount[animation];
        frame = frameBase+frameIndex;

        directionalQuad = VidShadowUsesDirectionalQuad(frameBase);
    }
    else
    {
        frame = frameBase+static_cast<int>(sprite->CurrentFrame());
        directionalQuad = VidShadowUsesDirectionalQuad(frame);
    }

    if (frame->height == 0u) return;

    VECTOR scale;
    const EX_SPRITE_DATA* const transformOwner =
        sprite->ExtraData();
    if (transformOwner) scale = transformOwner->scale;
    else
    {
        scale.x = 1.0f;
        scale.y = 1.0f;
        scale.z = 1.0f;
    }
    scale.x *= self->scaleXYZ.x;
    scale.y *= self->scaleXYZ.y;
    scale.z *= self->scaleXYZ.z;

    float positionX = sprite->Coor().x;
    float positionY = sprite->Coor().y-sprite->Coor().z;
    if ((ownerProperty&0x00010000u) != 0u)
    {
        const unsigned int waveIndex = (::CurrentTime>>3u)&0xFFu;
        positionY -= FSin[waveIndex]*self->topZ;
    }

    const unsigned int runtimeFlags = self->runtimeFlags;
    if ((runtimeFlags&kVidAnimateScale) != 0u)
    {
        const WEAPON_RUNTIME* const curveOwner =
            self->weapon;
        const float phase = transformOwner->effectCurvePosition;
        const VECTOR curveScale = EvaluateVidHardwareCurve(curveOwner, curveOwner->Scale, phase);
        scale.x *= curveScale.x;
        scale.y *= curveScale.y;
        scale.z *= curveScale.z;
    }
    if ((runtimeFlags&kVidAnimateOffset) != 0u)
    {
        const WEAPON_RUNTIME* const curveOwner =
            self->weapon;
        const float phase = transformOwner->effectCurvePosition;
        const VECTOR curveOffset = EvaluateVidHardwareCurve(curveOwner, curveOwner->Shift, phase);
        positionX += curveOffset.x;
        positionY += curveOffset.y;
    }

    if (VidShadowOutsideRuntimeBounds(self, positionX, positionY, scale)) return;

    if (directionalQuad && (ownerProperty&0x20000000u) != 0u)
    {
        if ((ownerProperty&0x00002000u) != 0u) drawDirection = sprite->Direction().value;
        else
        {
            const unsigned char relativeDirection = static_cast<unsigned char>(
                sprite->Direction().value-shadowDirection);
            const int realDirection = self->RealDirectionWithGraphDetail(relativeDirection);
            const int correction = (realDirection<<8)/self->noDir;
            drawDirection = static_cast<unsigned char>(sprite->Direction().value-static_cast<unsigned char>(correction));
        }
    }

    self->lastRenderUseTick = ::RealCurrentTime;
    frame->SetTextureToDevice(self, 0);
    if (FloatBitsU32(frame->minZ) == 0xC97423F0u)
        frame->CalcMaxMinZ(self);
    if (!(frame->maxZ>(0.0f))) return;

    const VECTOR2 groundPoint(sprite->Coor().x, sprite->Coor().y);
    float groundDelta = sprite->Coor().z-Map->GetGroundZ(groundPoint);
    if (groundDelta<0.0f) groundDelta = 0.0f;

    const int width = static_cast<int>(static_cast<short>(frame->width));
    const int height = static_cast<int>(static_cast<short>(frame->height));
    const int halfOwnerX = VidHardwareHalfTowardZero(self->vidSizeX);
    const int halfOwnerY = VidHardwareHalfTowardZero(self->vidSizeY);
    const int frameX = static_cast<int>(static_cast<short>(frame->offsetX));
    const int frameY = static_cast<int>(static_cast<short>(frame->offsetY));
    const int xSource[4] = {0, width, 0, width};
    const int ySource[4] = {0, 0, height, height};
    const TEXTURE* const texture = frame->textureObject;
    const float textureWidth = static_cast<float>(texture->m_width);
    const float textureHeight = static_cast<float>(texture->m_height);
    VidShadowVertex vertices[4];
    for (int i = 0;i<4;++i)
    {
        vertices[i].x = static_cast<float>(xSource[i]-halfOwnerX+frameX);
        vertices[i].y = static_cast<float>(ySource[i]-halfOwnerY+frameY)-groundDelta;
        vertices[i].z = (i<2?frame->maxZ*(0.300000012f):frame->minZ);
        vertices[i].rhw = 1.0f;
        vertices[i].u = static_cast<float>(xSource[i])/textureWidth;
        vertices[i].v = static_cast<float>(ySource[i])/textureHeight;
    }

    const ANGLE rotation(drawDirection);
    const float rotateXX = rotation.Cos();
    const float rotateXY = rotation.Sin();
    MAP* const world = Map;
    for (int i = 0;i<4;++i)
    {
        VidShadowVertex& vertex = vertices[i];
        vertex.x *= scale.x;
        vertex.y *= scale.y;
        vertex.z *= scale.z;
        vertex.z = (sprite->Coor().z+vertex.z+(128.0f))*
            (0.000122070312f);

        if (!directionalQuad || (ownerProperty&0x20000000u) != 0u)
        {
            const float x = vertex.x*rotateXX-vertex.y*rotateXY;
            const float y = vertex.y*rotateXX+vertex.x*rotateXY;
            vertex.x = x;
            vertex.y = y;
        }
        if (directionalQuad)
        {
            const float radius = frame->maxZ*(-0.600000024f);
            const ANGLE shadowAngle(shadowDirection);
            vertex.x -= radius*shadowAngle.Sin();
            vertex.y += radius*shadowAngle.CosY();
        }

        const float worldX = positionX+vertex.x;
        const float worldY = positionY+vertex.y;
        if (self->layer != 9)
        {
            const VECTOR2 projected = world->ToScreenScaled(VECTOR2(worldX, worldY));
            vertex.x = projected.x;
            vertex.y = projected.y;
        }
        else
        {
            vertex.x = worldX-world->ShiftXY().x;
            vertex.y = worldY-world->ShiftXY().y;
        }
        vertex.y += groundDelta;
    }

    GRAPH* const graph = Graph;
    IDirect3DDevice9* const device = graph->device;
    (void)device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vertices, sizeof(VidShadowVertex));
    static_cast<void>(VID_SURFACE::noDrawShadow = VID_SURFACE::noDrawShadow+1u);
}

static VID_FONT* SelectRichTextFontVid(VID_FONT* currentFont, int index, bool requireValid)
{
    VID* const sentinel = ::EmptyVid;
    VID* const stored = index >= 0 ? Map->StoredVid(index) : 0;
    if (!stored || stored == sentinel || (stored->type&0x4000u) == 0u)
        return requireValid ? 0 : currentFont;

    VID_FONT* const font = static_cast<VID_FONT*>(stored);
    return font->fontBackend ? font : (requireValid ? 0 : currentFont);
}
static bool RichTextTagStartsWith(const char* textCursor, const char* literal)
{
    while (*literal)
    {
        if (*textCursor++ != *literal++) return false;
    }
    return true;
}
static unsigned int NormalizeRichTextColor(unsigned int color)
{
    return (color&0x00FFFFFFu) != 0u ? (color|0xFF000000u) : 0xFFFFFFFFu;
}

static int DrawRichTextRunClipped(
    VID_FONT* owner, VID_FONT* selected, int x, int y, const char* text, int length, unsigned int color, bool measureRun, RECT* measureRect)
{
    int start = x;
    const char* begin = text;
    int count = length;
    while (count>0 && start<0)
    {
        MeasureRichTextRun(selected, begin, 1, color, measureRect);
        start += measureRect->right;
        ++begin;
        --count;
    }
    if (measureRun) MeasureRichTextRun(selected, begin, count, color, measureRect);
    (void)DrawRichTextRun(selected, start, y, begin, count, color);
    return start+measureRect->right;
}

void VID_FONT::DrawTextA(int startX, int startY, const STRING& textValue, int textLength,
    int firstLine, int lastLine, COLOR colorValue, const LIST<int>& lineOffsetValues) const
{
    const LIST<int>* const lineOffsets = &lineOffsetValues;
    VID_FONT* const owner = const_cast<VID_FONT*>(this);
    if (!owner->fontBackend || textLength == 0) return;
    const STRING* const textObject = &textValue;
    const char* const text = textObject->m_value;
    if (!*text) return;

    const unsigned int originalColor = NormalizeRichTextColor(colorValue.value);
    unsigned int currentColor = originalColor;
    VID_FONT* selected = owner;
    int line = 0;
    int y = startY;
    int x = startX+lineOffsets->m_data[0];
    int runLength = 0;
    RECT measureRect = {0, 0, 1, 1};
    const int endIndex = textLength;
    const int lastVisibleLine = lastLine+1;

    int index = 0;
    for (;index<endIndex && line <= lastVisibleLine;++index)
    {
        const char ch = text[index];
        const bool lineBreak = (ch == '\n' || ch == '\r');
        const bool alignTag = ch == '<' && RichTextTagStartsWith(text+index, "<Align=");
        const bool fontTag = ch == '<' && RichTextTagStartsWith(text+index, "<Font=");
        const bool colorTag = ch == '<' && RichTextTagStartsWith(text+index, "<Color=");

        if (alignTag || fontTag || colorTag || lineBreak)
        {
            if (runLength>0 && line >= firstLine)
            {
                const char* const begin = text+index-runLength;
                const int advanced = DrawRichTextRunClipped(owner, selected, x, y, begin, runLength, currentColor, !lineBreak, &measureRect);
                if (!alignTag && !lineBreak) x = advanced;
            }

            if (alignTag)
            {
                x = startX+static_cast<int>(owner->vidSizeX)*BitsToSigned32(CrtAtoiText(text+index+7));
            }
            else if (fontTag)
            {
                const int fontIndex = BitsToSigned32(CrtAtoiText(text+index+6));
                VID_FONT* const candidate = SelectRichTextFontVid(selected, fontIndex, true);
                if (candidate)
                {
                    selected = candidate;
                    currentColor = NormalizeRichTextColor(selected->gammaRaw.add);
                }
                else
                {
                    VidReportResourceError(owner, E_INVALID, "font in DrawText()", fontIndex);
                }
            }
            else if (colorTag)
            {
                int parsed = static_cast<int>(currentColor);
                (void)sscanf(text+index+7, "%i", &parsed);
                currentColor = (parsed == 0) ? originalColor : (static_cast<unsigned int>(parsed)|0xFF000000u);
            }
            else if (lineBreak)
            {
                if (text[index+1] == '\n' || text[index+1] == '\r') ++index;
                ++line;
                y += static_cast<int>(selected->vidSizeY);
                x = startX+lineOffsets->m_data[line];
            }

            runLength = -1;
            if (alignTag || fontTag || colorTag)
            {
                while (index<endIndex && text[index] != '>') ++index;
            }
        }
        ++runLength;
    }

    if (runLength>0 && line >= firstLine && line <= lastVisibleLine)
    {
        const char* begin = text+index-runLength;
        (void)DrawRichTextRunClipped(owner, selected, x, y, begin, runLength, currentColor, false, &measureRect);
    }
}


static void VidReportResourceError(const VID* owner, TYPE_ERROR type, const char* text, int value)
{
    STRING subject;
    subject = text;
    owner->Error(type, subject, static_cast<unsigned long>(value));
}

static int VidClampGammaDelta(int value)
{
    if (value>255) return 255;
    if (value<-255) return -255;
    return value;
}

static void VidSetGammaDelta(GAMMA& gamma, int component, int value)
{
    unsigned char* const bytes = gamma.value;
    value = VidClampGammaDelta(value);
    bytes[component] = 0;
    bytes[component+4] = 0;
    if (value<0) bytes[component] = static_cast<unsigned char>(-value);
    else bytes[component+4] = static_cast<unsigned char>(value);
}



static long VidResourceTell(RESOURCE* resource)
{
    return static_cast<long>(BitsToSigned32(static_cast<unsigned int>(std::ftell(resource->FileHandle()))));
}

static unsigned int VidResourceSectionSize(const RESOURCE* resource)
{
    return resource->RecordSize();
}

static int VidResourceBytesToEndOfSubrecord(RESOURCE* resource)
{
    const long position = VidResourceTell(resource);
    const int currentPosition = resource->DataPosition();
    const int currentSize = resource->SubRecordSize();
    return currentPosition-static_cast<int>(position)+currentSize+4;
}

static void VidSkipResourceBytes(RESOURCE* resource, long bytes)
{
    resource->SetIoState(2u);
    (void)std::fseek(resource->FileHandle(), BitsToSigned32(Signed32ToBits(static_cast<int>(bytes))), SEEK_CUR);
}

void VID::LoadParameters(RESOURCE* resource)
{
    (void)resource->Read(&spriteType, 4);
    (void)resource->Read(&spriteClass, 4);
    (void)resource->Read(&property, 4);
    (void)resource->Read(&moveMask, 4);
    if (nVid == 0) property |= 0x01000000u;

    (void)resource->Read(&sizeXYZ.x, 4);
    (void)resource->Read(&sizeXYZ.y, 4);
    (void)resource->Read(&sizeXYZ.z, 4);
    (void)resource->Read(&maxHp, 4);
    (void)resource->Read(&maxSpeed, 4);
    (void)resource->Read(&randomSpeed, 4);
    (void)resource->Read(&maxZSpeed, 4);
    (void)resource->Read(&randomZSpeed, 4);
    (void)resource->Read(&acceleration, 4);
    (void)resource->Read(&slow, 4);
    (void)resource->Read(&rotationSpeed, 4);
    (void)resource->Read(&nWeapon, 4);
    (void)resource->Read(&deathRange, 4);
    (void)resource->Read(&deathDamageMin, 4);
    (void)resource->Read(&deathDamageMax, 4);
    if (deathDamageMax<deathDamageMin)
    {
        deathDamageMax = deathDamageMin;
        VidReportResourceError(this, E_INVALID, "deathDamageMax < deathDamageMin", 0);
    }
    (void)resource->Read(&deathPush, 4);
    deathPush *= 0.02f;

    (void)resource->Read(&linkXYZ.x, 4);
    (void)resource->Read(&linkXYZ.y, 4);
    (void)resource->Read(&linkXYZ.z, 4);
    linkXYZ.y = -linkXYZ.y;
    (void)resource->Read(&nLinkVid, 4);
    (void)resource->Read(&topZ, 4);
    (void)resource->Read(&forMoveUpZ, 4);
    (void)resource->Read(&forMoveDownZ, 4);
    (void)resource->Read(&lifeTime, 4);

    VidSkipResourceBytes(resource, 16);
    (void)resource->Read(&noDir, 4);
    (void)resource->Read(noAnimCadr, sizeof(noAnimCadr));
    (void)resource->Read(sfx, sizeof(sfx));
    (void)resource->Read(frameSpeed, sizeof(frameSpeed));
    (void)resource->Read(childX, sizeof(childX));
    (void)resource->Read(childY, sizeof(childY));
    (void)resource->Read(childZ, sizeof(childZ));
    (void)resource->Read(nChildVid, sizeof(nChildVid));
    (void)resource->Read(noChild, sizeof(noChild));

    int gammaRed;
    int gammaGreen;
    int gammaBlue;
    int gammaAlpha;
    (void)resource->Read(&gammaRed, 4);
    (void)resource->Read(&gammaGreen, 4);
    (void)resource->Read(&gammaBlue, 4);
    (void)resource->Read(&gammaAlpha, 4);

    gammaRaw.modulate = 0;
    gammaRaw.add = 0;
    VidSetGammaDelta(gammaRaw, 3, gammaAlpha);
    VidSetGammaDelta(gammaRaw, 2, gammaRed);
    VidSetGammaDelta(gammaRaw, 1, gammaGreen);
    VidSetGammaDelta(gammaRaw, 0, gammaBlue);
    for (int player = 0;player<PlayerSlotCount;++player) gammaByPlayer[player] = gammaRaw;

    (void)resource->Read(&scaleXYZ.x, 4);
    (void)resource->Read(&scaleXYZ.y, 4);
    (void)resource->Read(&scaleXYZ.z, 4);

    const float unlimited = 999999.0f;
    if (rotationSpeed == unlimited) rotationSpeed = 0.0f;
    else if (FloatEqualOrUnordered(rotationSpeed, 0.0f)) rotationSpeed = unlimited;
    else rotationSpeed = 256.0f/rotationSpeed;

    if (maxSpeed != unlimited) maxSpeed *= 0.001f;
    if (randomSpeed != unlimited) randomSpeed *= 0.001f;
    if (maxZSpeed != unlimited) maxZSpeed *= 0.001f;
    if (randomZSpeed != unlimited) randomZSpeed *= 0.001f;
    if (acceleration != unlimited) acceleration *= 0.000001f;
    if (slow != unlimited) slow *= 0.000001f;

    movementTactEnabled = (!FloatEqualOrUnordered(maxSpeed, 0.0f) ||
                         !FloatEqualOrUnordered(randomSpeed, 0.0f) ||
                         !FloatEqualOrUnordered(maxZSpeed, 0.0f) ||
                         !FloatEqualOrUnordered(randomZSpeed, 0.0f) ||
                         (property&0x06u) != 0u) ? 1 : 0;

    if (noDir == 0)
    {
        VidReportResourceError(this, E_INVALID, "NoDir == 0", 0);
        exit(1);
    }

    for (int slot = 0;slot<AnimationSlotCount;++slot)
        if (frameSpeed[slot] == 0) frameSpeed[slot] = static_cast<int>(frameSpeedDefault);

    directionQuantizationOffset = 128/noDir;
    realSizeXYZ = sizeXYZ;

    if ((property&0x100u) != 0u)
    {
        const float yAsX = sizeXYZ.y*1.414306640625f;
        if (!(yAsX >= sizeXYZ.x)) sizeXYZ.y = sizeXYZ.x*0.7070602178573608f;
        else sizeXYZ.x = yAsX;
    }
    halfSizeXY.x = sizeXYZ.x*0.5f;
    halfSizeXY.y = sizeXYZ.y*0.5f;

    if (noCadr == 0)
        VidReportResourceError(this, E_INVALID, "noCadr == 0", 0);
    else if (static_cast<int>(noCadr)<noDir && spriteClass != 19u)
    {
        VidReportResourceError(this, E_INVALID, "noCadr < noDir", 0);
        noDir = static_cast<int>(noCadr);
    }

    int requestedFrames = 0;
    for (int slot = 0;slot<AnimationSlotCount;++slot)
        requestedFrames = BitsToSigned32(static_cast<unsigned int>(requestedFrames)+
            static_cast<unsigned int>(noAnimCadr[slot])*static_cast<unsigned int>(noDir));

    if (requestedFrames>static_cast<int>(noCadr))
    {
        VidReportResourceError(this, E_MISSING, "noCadr for noAnimCadr and noDir", 0);
        for (int slot = AnimationSlotCount-1;slot >= 0;--slot)
        {
            const int withoutCurrent = BitsToSigned32(static_cast<unsigned int>(requestedFrames)-
                static_cast<unsigned int>(noAnimCadr[slot])*static_cast<unsigned int>(noDir));
            if (withoutCurrent>static_cast<int>(noCadr))
            {
                noAnimCadr[slot] = 0;
                requestedFrames = withoutCurrent;
                continue;
            }
            const int excess = BitsToSigned32(static_cast<unsigned int>(requestedFrames)-
                static_cast<unsigned int>(static_cast<int>(noCadr)));
            noAnimCadr[slot] = BitsToSigned32(static_cast<unsigned int>(noAnimCadr[slot])-
                static_cast<unsigned int>(excess/noDir));
            break;
        }
    }

    if (spriteType == 32u && moveMask == 0u && nWeapon == 0)
        runtimeFlags &= ~0x80u;

    int firstRealAnimation = -1;
    int totalFrames = 0;
    for (int slot = 0;slot<AnimationSlotCount;++slot)
    {
        const SOUND* const soundOwner =
            ::Sound;
        const int soundIndex = sfx[slot];
        const SFX* const soundTable = soundOwner->m_sfx;
        if (soundTable)
        {
            bool soundLoaded = false;
            if (soundIndex >= 0 && soundIndex <= soundOwner->m_sfxCount)
            {
                soundLoaded = soundTable[soundIndex].m_bufferCount != 0u;
            }
            if (!soundLoaded && nVid != -1)
            {
                {
                    STRING subject("sfx");
                    Error(E_INVALID, subject, static_cast<unsigned long>(sfx[slot]));
                }
                sfx[slot] = 0;
            }
        }

        const int animationFrames = noAnimCadr[slot];
        if (animationFrames != 0)
        {
            if (animationFrames != 1) runtimeFlags |= 0x80u;
            animationBaseFrame[slot] = totalFrames;
            animationFrameCount[slot] = animationFrames;
            if (firstRealAnimation<0)
            {
                firstRealAnimation = slot;
                for (int previous = 0;previous<slot;++previous)
                    if (animationFrameCount[previous] == 0)
                        animationFrameCount[previous] = animationFrames;
            }
        }
        else
        {
            bool copiedClass10Fallback = false;
            if (spriteClass == 10u && (slot&1) != 0 && slot <= 7 &&
               noAnimCadr[firstRealAnimation+1] != 0)
            {
                animationBaseFrame[slot] = animationBaseFrame[firstRealAnimation+1];
                animationFrameCount[slot] = animationFrameCount[firstRealAnimation+1];
                copiedClass10Fallback = true;
            }
            if (!copiedClass10Fallback)
            {
                animationBaseFrame[slot] = 0;
                animationFrameCount[slot] = *reinterpret_cast<const int*>(
                    reinterpret_cast<const unsigned char*>(this)+static_cast<int>(offsetof(VID, animationFrameCount))+firstRealAnimation*4);
            }
        }

        totalFrames = BitsToSigned32(static_cast<unsigned int>(totalFrames)+
            static_cast<unsigned int>(noAnimCadr[slot])*static_cast<unsigned int>(noDir));
        if (totalFrames>static_cast<int>(noCadr))
        {
            VidReportResourceError(this, E_ERROR, "noCadr and noAnimCadr and noDir", slot);
            animationBaseFrame[slot] = 0;
            animationFrameCount[slot] = *reinterpret_cast<const int*>(
                reinterpret_cast<const unsigned char*>(this)+static_cast<int>(offsetof(VID, animationFrameCount))+firstRealAnimation*4);
        }
    }

    if ((property&0x08u) != 0u && noGridZ == 0)
    {
        minXGridZ = maxXGridZ = minYGridZ = maxYGridZ = 0.0f;

        const int widthExtent = TruncateFloatToInt32(sizeXYZ.x)+17;
        const int rowStride = widthExtent/8;
        const int heightExtent = TruncateFloatToInt32(sizeXYZ.y)+17;
        const int scratchCount = (heightExtent*rowStride)/8+1;
        VECTOR* const scratch = static_cast<VECTOR*>(::operator new(static_cast<size_t>(scratchCount)*sizeof(VECTOR)));

        noGridZ = 0;
        for (int y = 0;static_cast<float>(y)<sizeXYZ.y;y += 8)
        {
            for (int x = 0;static_cast<float>(x)<sizeXYZ.x;x += 8)
            {
                VECTOR& point = scratch[noGridZ++];
                point.x = static_cast<float>(x)-halfSizeXY.x;
                point.y = static_cast<float>(y)-halfSizeXY.y;
                point.z = sizeXYZ.z;
            }
            VECTOR& right = scratch[noGridZ++];
            right.x = halfSizeXY.x;
            right.y = static_cast<float>(y)-halfSizeXY.y;
            right.z = sizeXYZ.z;
        }
        for (int x = 0;static_cast<float>(x)<sizeXYZ.x;x += 8)
        {
            VECTOR& bottom = scratch[noGridZ++];
            bottom.x = static_cast<float>(x)-halfSizeXY.x;
            bottom.y = halfSizeXY.y;
            bottom.z = sizeXYZ.z;
        }
        VECTOR& corner = scratch[noGridZ++];
        corner.x = halfSizeXY.x;
        corner.y = halfSizeXY.y;
        corner.z = sizeXYZ.z;

        if (gridZ) ::operator delete(gridZ);
        gridZ = static_cast<VECTOR*>(::operator new(static_cast<size_t>(noGridZ)*sizeof(VECTOR)));
        for (int index = 0;index<noGridZ;++index)
        {
            gridZ[index] = scratch[index];
            if (gridZ[index].x<minXGridZ) minXGridZ = gridZ[index].x;
            if (gridZ[index].x>maxXGridZ) maxXGridZ = gridZ[index].x;
            if (gridZ[index].y<minYGridZ) minYGridZ = gridZ[index].y;
            if (gridZ[index].y>maxYGridZ) maxYGridZ = gridZ[index].y;
        }
        ::operator delete(scratch);
    }
}

namespace {
struct VidMeshVertex12 { float x,y,z; };
struct VidMeshVertex20 { float x,y,z,u,v; };
static void VidReleaseFrameArray(VID_SURFACE& surface)
{
    if (surface.nextMirror != &surface) return;
    if (surface.frames)
    {
        delete[] surface.frames;
        surface.frames = 0;
    }
    surface.surfaceData = 0;
}
}

void CADR_DATA::SetTextureToDevice(VID_SURFACE* surface, int finalizeArg)
{
    if (textureObject == 0u && height != 0u)
    {
        D3DFORMAT desiredFormat = surface->pixelFormat;
        if (desiredFormat == D3DFMT_P8)
        {
            desiredFormat = ((surface->type&2u) != 0u || (Graph->capabilityFlags&0x4000u) != 0u)?D3DFMT_A8R8G8B8:D3DFMT_A1R5G5B5;
        }
        CADR_DATA* root = VID::lastUse;
        while (root)
        {
            CADR_DATA* const oldest = root->lruPrevious;
            const unsigned int age = ::RealCurrentTime-oldest->lastUseTick;
            if (age <= 5000u) break;
            if (VID::useTextureMemory <= 64000000u && age <= 20000u) break;
            oldest->ReleaseVideoMemory();
            root = VID::lastUse;
        }

        D3DXImageInfo imageInfo;
        const bool fileTexture = (surface->surfaceFlags&2u) != 0u;
        void* const textureMemory = CrtNew(sizeof(TEXTURE));
        if (fileTexture)
        {
            if (textureMemory)
            {
                bool constructed = false;
                try
                {
                    const int frameIndex = static_cast<int>(this-surface->frames);
                    STRING frameName = surface->vidName.Add(frameIndex);
                    const unsigned long colorKey = (surface->spriteClass == 19u)?0xFF000000u:0u;
                    textureObject = new(textureMemory) TEXTURE(frameName, colorKey, &imageInfo);
                    constructed = true;
                }
                catch (...)
                {
                    if (!constructed) CrtFreePointer(textureMemory);
                    throw;
                }
            }
            else textureObject = 0u;
            TEXTURE* const texture = textureObject;
            if (texture->m_texture != 0 || texture->m_cpuData != 0)
            {
                width = static_cast<unsigned short>(surface->spriteClass == 19u?
                    (imageInfo.Width>>4u)-1u:imageInfo.Width);
                height = static_cast<unsigned short>(surface->spriteClass == 19u?
                    (imageInfo.Height>>4u)-1u:imageInfo.Height);
            }
        }
        else
        {
            if (textureMemory)
            {
                const int textureHeight = (static_cast<int>(static_cast<short>(height))+3)&~3;
                const int textureWidth = (static_cast<int>(static_cast<short>(width))+3)&~3;
                try
                {
                    textureObject = new(textureMemory) TEXTURE(textureWidth, textureHeight,
                        desiredFormat, 3u);
                }
                catch (...)
                {
                    CrtFreePointer(textureMemory);
                    throw;
                }
            }
            else textureObject = 0u;
        }

        TEXTURE* const texture =
            textureObject;
        const D3DFORMAT format = texture->m_format;
        const unsigned int pixels = texture->m_height*texture->m_width;
        unsigned int textureBytes;
        if (format == D3DFMT_DXT1) textureBytes = static_cast<unsigned int>(BitsToSigned32(pixels)/2);
        else if (format == D3DFMT_P8 || format == D3DFMT_L8) textureBytes = pixels;
        else if (format == D3DFMT_A8R8G8B8) textureBytes = pixels*4u;
        else if (format == D3DFMT_R8G8B8) textureBytes = pixels*3u;
        else textureBytes = pixels*2u;
        static_cast<void>(VID::useTextureMemory = VID::useTextureMemory+textureBytes);

        TEXTURE* const liveTexture = textureObject;
        if (liveTexture->m_texture == 0u && liveTexture->m_cpuData == 0u)
        {
            STRING subject("texture");
            const int frameIndex = static_cast<int>(this-surface->frames);
            surface->Error(E_CREATE, subject, static_cast<unsigned long>(frameIndex));
            return;
        }

        if ((surface->surfaceFlags&2u) == 0u)
        {
            RECT rect = {0, 0, static_cast<int>(static_cast<short>(width)), static_cast<int>(static_cast<short>(height))};
            if ((surface->type&0x10u) != 0u) rect.left = 999999;
            int pitch;
            unsigned char* destination = liveTexture->Lock(&pitch, &rect);
            const unsigned int vertexWords = vertexCount;
            const unsigned int indexWords = indexCount;
            const unsigned char* source = surface->surfaceData+(indexWords+vertexWords*2u)*2u+frameDataOffset;
            if (!destination)
            {
                STRING subject("texture");
                const int frameIndex = static_cast<int>(this-surface->frames);
                surface->Error(E_LOCK, subject, static_cast<unsigned long>(frameIndex));
                return;
            }

            const D3DFORMAT sourceFormat = surface->pixelFormat;
            if (sourceFormat == D3DFMT_DXT1 || sourceFormat == D3DFMT_DXT3 || sourceFormat == D3DFMT_DXT5)
            {
                const unsigned int bitsPerPixel = sourceFormat == D3DFMT_DXT1?4u:8u;
                const unsigned int packedBits = static_cast<unsigned int>(static_cast<int>(static_cast<short>(width)))*
                    static_cast<unsigned int>(static_cast<int>(static_cast<short>(height)))*bitsPerPixel;
                const unsigned int copyBytes = static_cast<unsigned int>(BitsToSigned32(packedBits)/8);
                __movsd(reinterpret_cast<unsigned long*>(destination),
                    reinterpret_cast<const unsigned long*>(source), copyBytes/4u);
                __movsb(destination+(copyBytes&~3u), source+(copyBytes&~3u), copyBytes&3u);
            }
            else if ((surface->type&8u) != 0u)
            {
                const unsigned short initialFlags = surface->type;
                const unsigned int* const activePalette =
                    surface->recolorPalette?surface->recolorPalette:surface->palette;
                if (desiredFormat == D3DFMT_A8R8G8B8)
                {
                    while (*reinterpret_cast<const unsigned short*>(source) != 0u)
                    {
                        const unsigned int skip = *source++;
                        if (skip != 0u) __stosd(reinterpret_cast<unsigned long*>(destination), 0u, skip);
                        destination += skip*4u;
                        unsigned int count = *source++;
                        while (count != 0u)
                        {
                            const unsigned int color = activePalette[*source++];
                            *reinterpret_cast<unsigned int*>(destination) = color;
                            const unsigned short flags = surface->type;
                            if ((flags&0x2000u) != 0u) ++source;
                            if ((flags&4u) != 0u) source += 2;
                            destination += 4;
                            --count;
                        }
                    }
                }
                else if ((initialFlags&0x2004u) == 0u)
                {
                    while (*reinterpret_cast<const unsigned short*>(source) != 0u)
                    {
                        const unsigned int skip = *source++;
                        if (skip != 0u)
                        {
                            __stosd(reinterpret_cast<unsigned long*>(destination), 0u, skip/2u);
                            __stosw(reinterpret_cast<unsigned short*>(destination)+(skip&~1u), 0u, skip&1u);
                        }
                        destination += skip*2u;
                        unsigned int count = *source++;
                        while (count != 0u)
                        {
                            const unsigned int color = activePalette[*source++];
                            *reinterpret_cast<unsigned short*>(destination) = static_cast<unsigned short>(
                                ((color>>6u)&0x3E0u)|((color>>3u)&0x1Fu)|((color>>9u)&0xFC00u)|0x8000u);
                            destination += 2;
                            --count;
                        }
                    }
                }
                else
                {
                    while (*reinterpret_cast<const unsigned short*>(source) != 0u)
                    {
                        const unsigned int skip = *source++;
                        if (skip != 0u)
                        {
                            __stosd(reinterpret_cast<unsigned long*>(destination), 0u, skip/2u);
                            __stosw(reinterpret_cast<unsigned short*>(destination)+(skip&~1u), 0u, skip&1u);
                        }
                        destination += skip*2u;
                        unsigned int count = *source++;
                        while (count != 0u)
                        {
                            const unsigned int color = activePalette[*source++];
                            *reinterpret_cast<unsigned short*>(destination) = static_cast<unsigned short>(
                                ((color>>3u)&0x1Fu)|((color>>9u)&0xFC00u)|((color>>6u)&0x3E0u)|0x8000u);
                            const unsigned short flags = surface->type;
                            if ((flags&0x2000u) != 0u) ++source;
                            if ((flags&4u) != 0u) source += 2;
                            destination += 2;
                            --count;
                        }
                    }
                }
            }
            else if (sourceFormat == D3DFMT_A8R8G8B8)
            {
                while (*reinterpret_cast<const unsigned short*>(source) != 0u)
                {
                    const unsigned int skip = *source++;
                    if (skip != 0u) __stosd(reinterpret_cast<unsigned long*>(destination), 0u, skip);
                    destination += skip*4u;
                    unsigned int count = *source++;
                    while (count != 0u)
                    {
                        *reinterpret_cast<unsigned int*>(destination) = *reinterpret_cast<const unsigned int*>(source);
                        const unsigned short flags = surface->type;
                        if ((flags&0x2000u) != 0u) ++source;
                        if ((flags&4u) != 0u) source += 2;
                        destination += 4;
                        source += 4;
                        --count;
                    }
                }
            }
            else if (sourceFormat == D3DFMT_R8G8B8)
            {
                while (*reinterpret_cast<const unsigned short*>(source) != 0u)
                {
                    destination += static_cast<unsigned int>(*source++)*3u;
                    unsigned int count = *source++;
                    while (count != 0u)
                    {
                        const unsigned int color = *reinterpret_cast<const unsigned int*>(source);
                        *reinterpret_cast<unsigned int*>(destination) |= color&0xFFFFFFu;
                        const unsigned short flags = surface->type;
                        if ((flags&0x2000u) != 0u) ++source;
                        if ((flags&4u) != 0u) source += 2;
                        destination += 3;
                        source += 3;
                        --count;
                    }
                }
            }
            else
            {
                while (*reinterpret_cast<const unsigned short*>(source) != 0u)
                {
                    const unsigned int skip = *source++;
                    if (skip != 0u)
                    {
                        __stosd(reinterpret_cast<unsigned long*>(destination), 0u, skip/2u);
                        __stosw(reinterpret_cast<unsigned short*>(destination)+(skip&~1u), 0u, skip&1u);
                    }
                    destination += skip*2u;
                    unsigned int count = *source++;
                    while (count != 0u)
                    {
                        *reinterpret_cast<unsigned short*>(destination) = *reinterpret_cast<const unsigned short*>(source);
                        const unsigned short flags = surface->type;
                        if ((flags&0x2000u) != 0u) ++source;
                        if ((flags&4u) != 0u) source += 2;
                        destination += 2;
                        source += 2;
                        --count;
                    }
                }
            }
            textureObject->UnLock();
        }
    }

    if (textureObject != 0u)
    {
        textureObject->SetTextureToDevice(finalizeArg);
        lastUseTick = ::RealCurrentTime;
        if ((surface->surfaceFlags&1u) == 0u)
        {
            const bool linked = lruPrevious != 0u;
            CADR_DATA* root = VID::lastUse;
            if (linked)
            {
                if (root == this) static_cast<void>(VID::lastUse = lruNext);
                CADR_DATA* const previous = lruPrevious;
                CADR_DATA* const nextAddress = lruNext;
                previous->lruNext = nextAddress;
                CADR_DATA* const next = lruNext;
                CADR_DATA* const previousAddress = lruPrevious;
                next->lruPrevious = previousAddress;
                lruNext = 0u;
                lruPrevious = 0u;
                root = VID::lastUse;
                if (root == this) goto frameLruSelf;
            }
            if (root != 0u)
            {
                lruNext = root;
                lruPrevious = VID::lastUse->lruPrevious;
                lruPrevious->lruNext = this;
                lruNext->lruPrevious = this;
            }
            else
            {
frameLruSelf:
                lruNext = this;
                lruPrevious = this;
            }
            static_cast<void>(VID::lastUse = this);
        }
    }
}

void CADR_DATA::SetVertexToDevice(VID_SURFACE* surface)
{
    void* target;

    if (indexBuffer == 0u && indexCount != 0u)
    {
        const unsigned int indexBytes = static_cast<unsigned int>(indexCount)*2u;
        GRAPH* const graph = Graph;
        const D3DPOOL pool = static_cast<D3DPOOL>(((graph->capabilityFlags>>4u)&1u)*2u);
        HRESULT hr = graph->device->CreateIndexBuffer(indexBytes, D3DUSAGE_WRITEONLY,
            D3DFMT_INDEX16, pool, &indexBuffer, 0);
        if (static_cast<int>(hr)<0)
        {
            CADR_DATA* root = VID::lastUse;
            if (root)
            {
                const unsigned int initialBytes = VID::useTextureMemory;
                while (root)
                {
                    const unsigned int currentBytes = VID::useTextureMemory;
                    if (initialBytes-currentBytes >= 100000u) break;
                    root->lruPrevious->ReleaseVideoMemory();
                    root = VID::lastUse;
                }
                hr = Graph->device->CreateIndexBuffer(static_cast<unsigned int>(indexCount)*2u,
                    D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, pool,
                    &indexBuffer, 0);
            }
        }

        if (hr != 0u)
        {
            STRING subject("index buffer");
            STRING object("CADR_DATA");
            Error->Error(object, E_CREATE, subject, hr);
        }
        else
        {
            const HRESULT lockHr = indexBuffer->Lock(
                0u, static_cast<unsigned int>(indexCount)*2u, &target, 0u);
            if (static_cast<int>(lockHr)<0)
            {
                STRING subject("for AddtoIndexbuffer");
                STRING object("CADR_DATA");
                Error->Error(object, E_LOCK, subject, lockHr);
            }
            else
            {
                const unsigned int offset = frameDataOffset;
                const unsigned int copyBytes = static_cast<unsigned int>(indexCount)*2u;
                const unsigned char* const source = surface->surfaceData+offset;
                __movsd(static_cast<unsigned long*>(target),
                    reinterpret_cast<const unsigned long*>(source), copyBytes/4u);
                __movsb(static_cast<unsigned char*>(target)+(copyBytes&~3u),
                    source+(copyBytes&~3u), copyBytes&3u);
                (void)indexBuffer->Unlock();
            }
        }
    }

    if (indexBuffer != 0u)
    {
        const HRESULT hr = Graph->device->SetIndices(indexBuffer);
        if (static_cast<int>(hr)<0)
        {
            STRING subject("index");
            STRING object("CADR_DATA");
            Error->Error(object, E_SET, subject, hr);
        }
    }

    GRAPH* const vertexGraph = Graph;
    const unsigned int stride = vertexGraph->vertexShaderVersion<0x100u?20u:12u;
    if (vertexBuffer == 0u && vertexCount != 0u && textureObject != 0u)
    {
        const unsigned int fvf = vertexGraph->vertexShaderVersion<0x100u?(D3DFVF_XYZ|D3DFVF_TEX1):D3DFVF_XYZ;
        const D3DPOOL pool = static_cast<D3DPOOL>(((vertexGraph->capabilityFlags>>4u)&1u)*2u);
        const unsigned char* const packed = surface->surfaceData+frameDataOffset+
            static_cast<unsigned int>(indexCount)*2u;

        unsigned int retryCount = 0u;
        HRESULT hr;
        for (;;)
        {
            hr = Graph->device->CreateVertexBuffer(static_cast<unsigned int>(vertexCount)*stride,
                D3DUSAGE_WRITEONLY, fvf, pool, &vertexBuffer, 0);
            if (hr != static_cast<unsigned int>(D3DERR_OUTOFVIDEOMEMORY)) break;

            CADR_DATA* const root = VID::lastUse;
            if (!root) break;
            ++retryCount;
            if (retryCount >= 10u) break;
            root->lruPrevious->ReleaseVideoMemory();
        }

        if (static_cast<int>(hr)<0)
        {

            {
                STRING subject("vertex buffer in default memory");
                STRING object("CADR_DATA");
                Error->Error(object, E_CREATE, subject, hr);
            }
            hr = Graph->device->CreateVertexBuffer(static_cast<unsigned int>(vertexCount)*stride,
                D3DUSAGE_WRITEONLY, fvf, D3DPOOL_MANAGED,
                &vertexBuffer, 0);
            if (static_cast<int>(hr)<0)
            {
                STRING subject("vertex buffer");
                STRING object("CADR_DATA");
                Error->Error(object, E_CREATE, subject, hr);
            }
        }

        if (static_cast<int>(hr) >= 0)
        {
            const HRESULT lockHr = vertexBuffer->Lock(
                0u, static_cast<unsigned int>(vertexCount)*stride, &target, 0u);
            if (static_cast<int>(lockHr)<0)
            {
                STRING subject("vertex buffer for LoadMesh");
                STRING object("CADR_DATA");
                Error->Error(object, E_LOCK, subject, lockHr);
            }
            else
            {
                const float isoScale = 1.414306640625f;
                if (Graph->vertexShaderVersion<0x100u)
                {
                    const TEXTURE* const texture = textureObject;
                    const float textureWidth = static_cast<float>(BitsToSigned32(texture->m_width));
                    const float textureHeight = static_cast<float>(BitsToSigned32(texture->m_height));
                    for (unsigned int index = 0;index<static_cast<unsigned int>(vertexCount);++index)
                    {
                        const unsigned int* const input = reinterpret_cast<const unsigned int*>(packed+index*4u);
                        const unsigned int value = *input;
                        const float z = static_cast<float>((value>>20u)&0x3FFu)/
                            static_cast<float>(1u<<(value>>30u))-32.0f;
                        VidMeshVertex20& vertex = static_cast<VidMeshVertex20*>(target)[index];
                        vertex.x = static_cast<float>(static_cast<int>(value&0x3FFu)-static_cast<int>(surface->vidSizeX)/2);
                        vertex.z = z;
                        vertex.y = (static_cast<float>(static_cast<int>((*input>>10u)&0x3FFu)-
                            static_cast<int>(surface->vidSizeY)/2)+z)*isoScale;
                        vertex.u = (static_cast<float>(static_cast<int>(*input&0x3FFu)-
                            static_cast<int>(static_cast<short>(offsetX)))+0.5f)/textureWidth;
                        vertex.v = (static_cast<float>(static_cast<int>((*input>>10u)&0x3FFu)-
                            static_cast<int>(static_cast<short>(offsetY)))+0.5f)/textureHeight;
                    }
                }
                else
                {
                    for (unsigned int index = 0;index<static_cast<unsigned int>(vertexCount);++index)
                    {
                        const unsigned int* const input = reinterpret_cast<const unsigned int*>(packed+index*4u);
                        const unsigned int value = *input;
                        const float z = static_cast<float>((value>>20u)&0x3FFu)/
                            static_cast<float>(1u<<(value>>30u))-32.0f;
                        VidMeshVertex12& vertex = static_cast<VidMeshVertex12*>(target)[index];
                        vertex.x = static_cast<float>(static_cast<int>(value&0x3FFu)-static_cast<int>(surface->vidSizeX)/2);
                        vertex.z = z;
                        vertex.y = (static_cast<float>(static_cast<int>((*input>>10u)&0x3FFu)-
                            static_cast<int>(surface->vidSizeY)/2)+z)*isoScale;
                    }
                }
                (void)vertexBuffer->Unlock();
            }
            static_cast<void>(VID::useVertexMemory = VID::useVertexMemory+
                static_cast<unsigned int>(vertexCount)*stride);
        }
    }

    if (vertexBuffer != 0u)
    {
        const HRESULT hr = Graph->device->SetStreamSource(0u, vertexBuffer, 0u, stride);
        if (static_cast<int>(hr)<0)
        {
            STRING subject("Vertex");
            STRING object("CADR_DATA");
            Error->Error(object, E_SET, subject, hr);
        }
    }
}

namespace {
struct VidHardwareDrawTransform
{
    VECTOR scale;
    VECTOR position;
};
static VECTOR EvaluateVidHardwareCurve(
    const WEAPON_RUNTIME* owner, const VECTOR* keys, float phase)
{

    const int index = TruncateFloatToInt32(phase);
    if (index >= owner->maxColumn)
        return keys[owner->maxColumn];

    const float fraction = phase-static_cast<float>(index);
    const VECTOR first = keys[index];
    const VECTOR next = keys[index+1];
    const VECTOR delta = next-first;
    const VECTOR scaled = delta*fraction;
    return first+scaled;
}
static bool VidHardwareDrawOutsideRuntimeBounds(
    const VID_SURFACE* owner, const VidHardwareDrawTransform& transform)
{

    const float extentY = owner->buildSizeY*(0.5f)
        *transform.scale.y*(1.29999995f);
    const float extentX = owner->buildSizeX*(0.5f)
        *transform.scale.x*(1.29999995f);
    const float diagonal = transform.position.y-transform.position.z;

    if (transform.position.x-extentX>VID::viewMax.x &&
       diagonal-extentY>VID::viewMax.y)
        return true;

    if (!(transform.position.x+extentX >= VID::viewMin.x) &&
       !(diagonal+extentY >= VID::viewMin.y))
        return true;
    return false;
}
static CADR_DATA* SelectVidHardwareFrame(VID_SURFACE* owner, const SPRITE* request)
{
    const unsigned int frameBase32 = PtrToU32(owner->frames);

    return reinterpret_cast<CADR_DATA*>(
        U32ToPtr(frameBase32+static_cast<unsigned int>(request->CurrentFrame())*sizeof(CADR_DATA)));
}
template<class Left, class Right>
static inline bool VidX87EqualOrUnordered(const Left& left, const Right& right)
{
    return FloatEqualOrUnordered(left, right);
}
static unsigned char SelectVidHardwareDrawDirection(
    VID_SURFACE* owner, const SPRITE* request, const float& speed, const float& zSpeed)
{

    const unsigned int ownerFlags = owner->property;
    unsigned char result = 0u;
    if ((ownerFlags&0x20000000u) == 0u) return result;

    if ((ownerFlags&0x00080000u) != 0u)
    {
        const EX_SPRITE_DATA* source =
            request->ExtraData();
        const float requestIso = request->Coor().y-request->Coor().z;
        const double sourceIso = static_cast<double>(source->cachedPosition.y)-source->cachedPosition.z;
        if (!VidX87EqualOrUnordered(source->cachedPosition.x, request->Coor().x) ||
           !VidX87EqualOrUnordered(requestIso, sourceIso))
        {
            const float deltaX = request->Coor().x-source->cachedPosition.x;
            const float deltaIso = static_cast<float>(((static_cast<double>(request->Coor().y)-request->Coor().z)-sourceIso)*1.41430664f);
            return Decart2Polar(deltaX, deltaIso).value;
        }

        if (VidX87EqualOrUnordered(zSpeed, 0.0f))
            return request->Direction().value;
        const ANGLE directionAngle(request->Direction().value);
        const float first = directionAngle.Sin()*speed;
        const float second = static_cast<float>((static_cast<double>(directionAngle.Cos())*speed
            +zSpeed)*(-1.41430664f));
        return Decart2Polar(first, second).value;
    }

    if ((ownerFlags&0x00002000u) != 0u) return request->Direction().value;

    const unsigned char stored = request->Direction().value;
    VID* directionVid = request->Vid();
    const int realDirection = directionVid->RealDirectionWithGraphDetail(stored);
    const int directionCount = owner->noDir;
    const int correction = (realDirection<<8)/directionCount;
    return static_cast<unsigned char>(stored-static_cast<unsigned char>(correction));
}

static int VidHardwareHalfTowardZero(int value)
{
    return value/2;
}
static void FinishVidHardwareDrawCounters(VID_SURFACE* owner, unsigned int submitResult)
{

    if (static_cast<int>(submitResult)<0)
    {
        const STRING text("Draw");
        owner->Error(E_ERROR, text, submitResult);
    }
    static_cast<void>(VID_SURFACE::noObj = VID_SURFACE::noObj+1u);
    const unsigned int mode = static_cast<unsigned int>(owner->layer);
    unsigned int& completedDrawCount = VID_SURFACE::noDrawReal[mode];
    completedDrawCount = completedDrawCount+1u;
}
static unsigned int SubmitVidHardwareMeshBranch(
    VID_SURFACE* owner, const CADR_DATA* frame, const VidHardwareDrawTransform& transform,
    unsigned char direction, const unsigned int colorState[2])
{

    const ANGLE rotation(direction);
    float matrix[16];
    matrix[0] = rotation.Cos()*transform.scale.x;
    matrix[1] = rotation.SinY()*transform.scale.x;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;
    matrix[4] = -rotation.Sin()*transform.scale.y;
    matrix[5] = rotation.CosY()*transform.scale.y;
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;
    matrix[8] = 0.0f;
    matrix[9] = 0.0f;
    matrix[10] = transform.scale.z;
    matrix[11] = 0.0f;
    matrix[12] = transform.position.x;
    matrix[13] = transform.position.y;
    matrix[14] = transform.position.z;
    matrix[15] = 1.0f;

    Graph->SetTransformMatrix(D3DTS_WORLD, reinterpret_cast<const _D3DMATRIX*>(matrix), reinterpret_cast<const GAMMA*>(colorState));

    if (Graph->vertexShaderVersion<0x100u)
    {
        (void)Graph->pseudo3DEffect->SetTechnique(Graph->drawObjectWithoutShaderTechnique);
    }
    else
    {
        D3DXHANDLE material = Graph->drawObjectTechnique;
            if ((static_cast<unsigned char>(owner->type)&2u) == 0u && owner->renderMode == 0u &&
           static_cast<int>(Graph->graphDetail) >= 8 && (Map->Flags()&0x100u) == 0u)
            material = Graph->drawObjectWithZTechnique;
        (void)Graph->pseudo3DEffect->SetTechnique(material);

        float frameTransform[4];
        frameTransform[0] = static_cast<float>(VidHardwareHalfTowardZero(owner->vidSizeX)
            -static_cast<int>(static_cast<short>(frame->offsetX)))+(0.5f);
        frameTransform[1] = static_cast<float>(VidHardwareHalfTowardZero(owner->vidSizeY)
            -static_cast<int>(static_cast<short>(frame->offsetY)))+(0.5f);
        frameTransform[2] = 1.0f;
        frameTransform[3] = 1.0f;
        (void)Graph->pseudo3DEffect->SetVector(Graph->texShiftParameter, reinterpret_cast<const D3DXVECTOR4*>(frameTransform));

        const TEXTURE* texture = frame->textureObject;
        float textureTransform[4];
        textureTransform[0] = static_cast<float>(texture->m_width);
        textureTransform[1] = static_cast<float>(texture->m_height);
        textureTransform[2] = 1.0f;
        textureTransform[3] = 1.0f;
        (void)Graph->pseudo3DEffect->SetVector(Graph->texSizeParameter, reinterpret_cast<const D3DXVECTOR4*>(textureTransform));
    }

    static_cast<void>(VID_SURFACE::noVertex = VID_SURFACE::noVertex+frame->vertexCount);
    const unsigned int primitiveCount = static_cast<unsigned int>(frame->indexCount)/3u;
    static_cast<void>(VID_SURFACE::noTri = VID_SURFACE::noTri+primitiveCount);

    unsigned int batchCount = 0u;
    (void)Graph->pseudo3DEffect->Begin(&batchCount, 0u);
    unsigned int submitResult = FloatBitsU32(transform.position.x);
    if (batchCount != 0u)
    {
            for (unsigned int batch = 0u;batch<batchCount;++batch)
        {
            (void)Graph->pseudo3DEffect->BeginPass(batch);
            submitResult = static_cast<unsigned int>(Graph->device->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST, 0, 0, static_cast<unsigned int>(frame->vertexCount), 0u, primitiveCount));
            (void)Graph->pseudo3DEffect->EndPass();
        }
    }
    (void)Graph->pseudo3DEffect->End();
    return submitResult;
}

struct VidHardwareLocalVertex
{
    float x;
    float y;
    float z;
    float rhw;
    unsigned int color;
    float u;
    float v;
};


static void BuildVidHardwareQuadSeedVertices(
    VID_SURFACE* owner, const CADR_DATA* frame, VidHardwareLocalVertex* vertices,
    unsigned int* vertexCount)
{
    const TEXTURE* texture = frame->textureObject;
    const float textureWidth = static_cast<float>(texture->m_width);
    const float textureHeight = static_cast<float>(texture->m_height);
    const float halfTexel = (0.5f);
    const float isoScale = (1.41430664f);
    const int width = static_cast<int>(static_cast<short>(frame->width));
    const int height = static_cast<int>(static_cast<short>(frame->height));
    const int frameX = static_cast<int>(static_cast<short>(frame->offsetX));
    const int frameY = static_cast<int>(static_cast<short>(frame->offsetY));
    const int halfOwnerX = VidHardwareHalfTowardZero(owner->vidSizeX);
    const int halfOwnerY = VidHardwareHalfTowardZero(owner->vidSizeY);
    const int mode = owner->surfaceMode;

    if (mode == 2)
    {
        const int halfWidth = VidHardwareHalfTowardZero(width);
        const int halfHeight = VidHardwareHalfTowardZero(height);
        const int sixthHeight = height/6;
        const int xSource[8] = {halfWidth, 0, halfWidth, width, width, halfWidth, 0, 0};
        const int ySource[8] = {sixthHeight, 0, 0, 0, height, height, height, 0};
        const int depthSource[8] = {halfHeight, 0, sixthHeight, 0, 0, sixthHeight, 0, 0};
        *vertexCount = 8u;
        for (unsigned int i = 0u;i<8u;++i)
        {
            vertices[i].x = static_cast<float>(xSource[i]-halfOwnerX+frameX);
            vertices[i].y = static_cast<float>(ySource[i]-halfOwnerY+frameY)*isoScale;
            vertices[i].z = static_cast<float>(depthSource[i]);
            vertices[i].u = (static_cast<float>(xSource[i])+halfTexel)/textureWidth;
            vertices[i].v = (static_cast<float>(ySource[i])+halfTexel)/textureHeight;
        }
        return;
    }

    const bool specialFour = (mode == 0) ||
        (mode == -1 && owner->sizeXYZ.z>owner->sizeXYZ.y);
    *vertexCount = 4u;
    if (specialFour)
    {
        const int xSource[4] = {0, width, 0, width};
        const int ySource[4] = {0, 0, height, height};
        const int depthSource[4] = {height, height, 0, 0};
        for (unsigned int i = 0u;i<4u;++i)
        {
            vertices[i].x = static_cast<float>(xSource[i]-halfOwnerX+frameX);
            vertices[i].y = static_cast<float>(ySource[i]-halfOwnerY+frameY)*isoScale;
            vertices[i].z = static_cast<float>(depthSource[i]);
            vertices[i].u = (static_cast<float>(xSource[i])+halfTexel)/textureWidth;
            vertices[i].v = (static_cast<float>(ySource[i])+halfTexel)/textureHeight;
        }
        return;
    }

    const int xSource[4] = {0, width, 0, width};
    const int ySource[4] = {0, 0, height, height};
    for (unsigned int i = 0u;i<4u;++i)
    {
        vertices[i].x = static_cast<float>(xSource[i]-halfOwnerX+frameX);
        vertices[i].y = static_cast<float>(ySource[i]-halfOwnerY+frameY)*isoScale;
        vertices[i].z = 0.0f;
        vertices[i].u = (static_cast<float>(xSource[i])+halfTexel)/textureWidth;
        vertices[i].v = (static_cast<float>(ySource[i])+halfTexel)/textureHeight;
    }
}

static unsigned int SubmitVidHardwareQuadBranch(
    VID_SURFACE* owner, const CADR_DATA* frame, const VidHardwareDrawTransform& transform,
    unsigned char direction, const unsigned int colorState[2])
{

    VidHardwareLocalVertex vertices[8];
    unsigned int vertexCount = 4u;
    BuildVidHardwareQuadSeedVertices(owner, frame, vertices, &vertexCount);

    const ANGLE rotation(direction);
    const float rotateXX = rotation.Cos();
    const float rotateXY = rotation.Sin();
    const float rotateYX = rotation.SinY();
    const float rotateYY = rotation.CosY();
    const unsigned int baseColor = ~colorState[0];
    const float isoWorldY = transform.position.y-transform.position.z;

    for (unsigned int i = 0u;i<vertexCount;++i)
    {
        VidHardwareLocalVertex& vertex = vertices[i];
        const float scaledX = vertex.x*transform.scale.x;
        const float scaledY = vertex.y*transform.scale.y;
        const float depth = vertex.z*transform.scale.z;
        const float rotatedX = static_cast<float>(static_cast<double>(scaledX)*rotateXX-static_cast<double>(scaledY)*rotateXY);
        const float rotatedY = static_cast<float>(static_cast<double>(scaledY)*rotateYY+static_cast<double>(scaledX)*rotateYX);

        if (static_cast<unsigned int>(owner->layer) != 9u)
        {
            const VECTOR2 point(transform.position.x+rotatedX, isoWorldY+rotatedY);
            const VECTOR2 projected = Map->ToScreenScaled(point);
            vertex.x = projected.x;
            vertex.y = projected.y;
        }
        else
        {
            vertex.x = transform.position.x+rotatedX-Map->ShiftXY().x;
            vertex.y = isoWorldY+rotatedY-Map->ShiftXY().y;
        }

        unsigned int color = baseColor;
        if ((static_cast<unsigned char>(owner->type)&2u) == 0u && owner->renderMode == 0u &&
           static_cast<int>(Graph->graphDetail) >= 8 && (Map->Flags()&0x100u) == 0u)
        {
            int alpha = VidHardwareHalfTowardZero(TruncateDoubleToInt32(static_cast<double>(transform.position.z)+depth));
            if (alpha <= 1) alpha = 1;
            else if (alpha >= 255) alpha = 255;
            color = (baseColor&0x00FFFFFFu)|(static_cast<unsigned int>(alpha)<<24u);
        }
        vertex.color = color;
        vertex.z = static_cast<float>((static_cast<double>(transform.position.z)+depth+128.0)*0.0001220703125);
        vertex.rhw = 1.0f;
    }

    static_cast<void>(VID_SURFACE::noVertex = VID_SURFACE::noVertex+4u);
    static_cast<void>(VID_SURFACE::noTri = VID_SURFACE::noTri+2u);
    (void)Graph->device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    const unsigned int mode = static_cast<unsigned int>(owner->surfaceMode);
    return static_cast<unsigned int>(Graph->device->DrawPrimitiveUP(
        (mode == 2u?D3DPT_TRIANGLEFAN:D3DPT_TRIANGLESTRIP), mode == 2u?6u:2u, vertices, 28u));
}

static VidHardwareDrawTransform BuildVidHardwareDrawTransformPrefix(
    VID_SURFACE* owner, const SPRITE* request)
{
    VidHardwareDrawTransform result;
    result.scale.x = 1.0f;
    result.scale.y = 1.0f;
    result.scale.z = 1.0f;
    if (request->ExtraData())
    {
        const EX_SPRITE_DATA* transformOwner =
            request->ExtraData();
        result.scale = transformOwner->scale;
    }
    result.scale.x *= owner->scaleXYZ.x;
    result.scale.y *= owner->scaleXYZ.y;
    result.scale.z *= owner->scaleXYZ.z;

    result.position.x = request->Coor().x;
    result.position.y = request->Coor().y;
    result.position.z = request->Coor().z;
    const unsigned int ownerFlags = owner->property;
    if ((ownerFlags&kVidApplyPixelCenterOffset) != 0u)
    {
        const float centerOffset = (1000.0f);
        result.position.y += centerOffset;
        result.position.z += centerOffset;
    }
    if ((ownerFlags&kVidApplyDirectionOffset) != 0u)
    {
        const unsigned int directionIndex = (::CurrentTime>>3u)&0xFFu;
        result.position.z += FSin[directionIndex]*owner->topZ;
    }
    return result;
}
}

void VID_SURFACE::DrawOnlyZ(const SPRITE* sprite)
{
    VID_SURFACE* const owner = this;
    const SPRITE* const request = sprite;

    const unsigned int runtimeFlags = owner->runtimeFlags;
    if (!owner->frames || (runtimeFlags&kVidRuntimeHidden) != 0u) return;
    if ((static_cast<unsigned char>(owner->type)&2u) != 0u) return;
    if (static_cast<unsigned int>(owner->layer)>3u || owner->renderMode != 0u) return;

    CADR_DATA* selectedFrame = SelectVidHardwareFrame(owner, request);
    if (selectedFrame->height == 0u) return;

    VidHardwareDrawTransform drawTransform = BuildVidHardwareDrawTransformPrefix(owner, request);
    (void)drawTransform;

    if ((runtimeFlags&(kVidAnimateScale|kVidAnimateOffset)) != 0u)
    {
        const EX_SPRITE_DATA* transformOwner =
            request->ExtraData();
        const WEAPON_RUNTIME* curveOwner =
            owner->weapon;
        const float phase = transformOwner->effectCurvePosition;
        if ((runtimeFlags&kVidAnimateScale) != 0u)
        {
            const VECTOR curveScale = EvaluateVidHardwareCurve(curveOwner, curveOwner->Scale, phase);
            drawTransform.scale.x *= curveScale.x;
            drawTransform.scale.y *= curveScale.y;
            drawTransform.scale.z *= curveScale.z;
        }
        if ((runtimeFlags&kVidAnimateOffset) != 0u)
        {
            const VECTOR curveOffset = EvaluateVidHardwareCurve(curveOwner, curveOwner->Shift, phase);
            drawTransform.position.x += curveOffset.x;
            drawTransform.position.y += curveOffset.y;
            drawTransform.position.z += curveOffset.z;
        }
    }

    if (VidHardwareDrawOutsideRuntimeBounds(owner, drawTransform)) return;

    const unsigned char drawDirection = SelectVidHardwareDrawDirection(owner, request, request->m_speed, request->m_zSpeed);
    (void)drawDirection;

    owner->lastRenderUseTick = ::RealCurrentTime;
    const unsigned int colorState[2] = {owner->gammaRaw.modulate, owner->gammaRaw.add};
    selectedFrame->SetTextureToDevice(owner, 0);
    selectedFrame->SetVertexToDevice(owner);

    if (selectedFrame->indexCount != 0u)
    {
        const unsigned int submitResult = SubmitVidHardwareMeshBranch(
            owner, selectedFrame, drawTransform, drawDirection, colorState);
        FinishVidHardwareDrawCounters(owner, submitResult);
        return;
    }

    const unsigned int submitResult = SubmitVidHardwareQuadBranch(
        owner, selectedFrame, drawTransform, drawDirection, colorState);
    FinishVidHardwareDrawCounters(owner, submitResult);
}

VID::VID()
{
    gammaRaw.modulate = 0u;
    gammaRaw.add = 0u;

    for (int i = 0;i<PlayerSlotCount;++i)
    {
        gammaByPlayer[i].modulate = 0u;
        gammaByPlayer[i].add = 0u;
    }

    sizeXYZ.x = 24.0f;
    sizeXYZ.y = 16.0f;
    sizeXYZ.z = 20.0f;
    runtimeBackend = 0u;
    spriteClass = 6u;
    spriteType = 0u;
    property = 0x01000000u;
    halfSizeXY.x = sizeXYZ.x*0.5f;
    halfSizeXY.y = sizeXYZ.y*0.5f;
    scaleXYZ.x = 1.0f;
    scaleXYZ.y = 1.0f;
    scaleXYZ.z = 1.0f;
    nextMirror = this;
    exchangedVid = this;
    type = 0u;
    layer = 13;
    const unsigned int originalRuntimeFlags = *reinterpret_cast<volatile unsigned int*>(&runtimeFlags);
    const unsigned int originalStorageFlags = *reinterpret_cast<volatile unsigned int*>(&storageFlags);
    runtimeFlags = (originalRuntimeFlags&0xFFFFC080u)|0x80u;
    storageFlags = (originalStorageFlags&0xFFFFFFF4u)|0x14u;
    nVid = -1;
    maxHp = 0;
    noDir = 1;
    directionQuantizationOffset = 0;
    frameSpeedDefault = 71u;
    nLinkVid = 0;
    linkVid = 0;
    nWeapon = 0;
    weapon = 0;
    noGridZ = 0;
    gridZ = 0;
    gridCadrShift = 0;
    cadrLinkXYZ = 0;
    sourceVid = 0;
    lastRenderUseTick = 0u;
    minXGridZ = 0.0f;
    maxXGridZ = 0.0f;
    minYGridZ = 0.0f;
    maxYGridZ = 0.0f;
    actionAuxStateRequired = 0;
    movementTactEnabled = 0;
    notCreateAsChild = 0;

    for (int i = 0;i<AnimationSlotCount;++i)
    {
        sfx[i] = 0;
        nChildVid[i] = 0;
        childVid[i] = 0;
        noAnimCadr[i] = 0;
        animationBaseFrame[i] = 0;
        animationFrameCount[i] = 0;
        frameSpeed[i] = 71;
    }

    ResetSprites();
}

VID_SOFTWARE::VID_SOFTWARE()
{
    frameOffsets = 0;
    allocatedBytes = 0u;
    mappedFileBase = 0;
}

VID_SOFTWARE::VID_SOFTWARE(VID_SOFTWARE& source) : VID()
{
    nextMirror = source.nextMirror;
    source.nextMirror = this;
    layer = source.layer;
    type = source.type;
    noCadr = source.noCadr;
    frameSpeedDefault = source.frameSpeedDefault;
    vidSizeX = source.vidSizeX;
    vidSizeY = source.vidSizeY;
    buildSizeX = source.buildSizeX;
    buildSizeY = source.buildSizeY;
    frameOffsets = source.frameOffsets;
    mappedFileBase = source.mappedFileBase;
    allocatedBytes = source.allocatedBytes;
    noGridZ = source.noGridZ;
    cadrLinkXYZ = source.cadrLinkXYZ;
    vidName = source.vidName;
    CopyBytes(paletteTables, source.paletteTables, sizeof(paletteTables));
    runtimeFlags = (runtimeFlags&~0x800u)|(source.runtimeFlags&kVidRuntimeHidden);
    storageFlags &= ~4u;

    if (noGridZ != 0)
    {
        const unsigned int bytes = static_cast<unsigned int>(noGridZ)*sizeof(VECTOR);
        gridZ = static_cast<VECTOR*>(CrtNew(bytes));
        if (noGridZ>0)
            CopyBytes(gridZ, source.gridZ, bytes);
        if (source.gridCadrShift)
        {
            const int count = static_cast<int>(noCadr);
            gridCadrShift = static_cast<int*>(CrtNew(static_cast<unsigned int>(count)*sizeof(int)));
            if (count>0)
                CopyBytes(gridCadrShift, source.gridCadrShift,
                                 static_cast<unsigned int>(count)*sizeof(int));
        }
        else gridCadrShift = 0;
    }
}

VID_SOFTWARE::~VID_SOFTWARE()
{
    if (nextMirror == this)
    {
        if (mappedFileBase)
            (void)UnmapViewOfFile(mappedFileBase);
        mappedFileBase = 0;
        if (frameOffsets) CrtFreePointer(frameOffsets);
        frameOffsets = 0;
        static_cast<void>(VID::MemoryInUse = VID::MemoryInUse-allocatedBytes);
        allocatedBytes = 0u;
    }
}

VID_LIGHT::VID_LIGHT() : VID()
{
    lightDataBytes = 0u;
    lightData = 0;
}

VID_LIGHT::VID_LIGHT(VID_LIGHT& source) : VID()
{
    nextMirror = source.nextMirror;
    source.nextMirror = this;
    layer = source.layer;
    type = source.type;
    noCadr = source.noCadr;
    frameSpeedDefault = source.frameSpeedDefault;
    vidSizeX = source.vidSizeX;
    vidSizeY = source.vidSizeY;
    buildSizeX = source.buildSizeX;
    buildSizeY = source.buildSizeY;
    lightData = source.lightData;
    lightDataBytes = source.lightDataBytes;
    cadrLinkXYZ = source.cadrLinkXYZ;
    lastRenderUseTick = source.lastRenderUseTick;
    storageFlags &= ~4u;
}

VID_LIGHT::~VID_LIGHT()
{
    if (nextMirror == this)
    {
        if (lightData) (void)(CrtFreePointer(lightData), 0u);
        lightData = 0;
        static_cast<void>(VID::MemoryInUse = VID::MemoryInUse-lightDataBytes);
        lightDataBytes = 0u;
    }
}

VID_FONT::VID_FONT(VID_FONT& source) : VID()
{
    nextMirror = source.nextMirror;
    source.nextMirror = this;
    layer = source.layer;
    type = source.type;
    noCadr = source.noCadr;
    frameSpeedDefault = source.frameSpeedDefault;
    vidSizeX = source.vidSizeX;
    vidSizeY = source.vidSizeY;
    buildSizeX = source.buildSizeX;
    buildSizeY = source.buildSizeY;
    cadrLinkXYZ = source.cadrLinkXYZ;
    fontBackend = source.fontBackend;
    lastRenderUseTick = source.lastRenderUseTick;
    storageFlags &= ~4u;
}

VID_FONT::~VID_FONT()
{
    if (nextMirror == this && fontBackend)
    {
        (void)fontBackend->Release();
        fontBackend = 0;
    }
}

VID_SURFACE::VID_SURFACE()
{
    const unsigned int flags = surfaceFlags&~3u;
    surfaceBackend = 0;
    frames = 0;
    surfaceData = 0;
    palette = 0;
    renderMode = 0u;
    recolorPalette = 0;
    surfaceMode = -1;
    surfaceFlags = flags;
}

VID_SURFACE::~VID_SURFACE()
{
    if (surfaceBackend)
    {
        (void)surfaceBackend->Release();
        surfaceBackend = 0;
    }
    if (recolorPalette)
    {
        CrtFreePointer(recolorPalette);
        recolorPalette = 0;
    }
    if (nextMirror == this)
    {
        VidReleaseFrameArray(*this);
    }
}

namespace
{
    const short kEmptySoftwareGridCell=static_cast<short>(-32000);

    static int SoftwareGridCellIndex(int x, int y)
    {

        return (y>>3)*256+(x>>3);
    }

    static void SoftwareRasterizeDepthFrame(const unsigned char* frame, short* cells)
    {
        const int headerCount = *reinterpret_cast<const short*>(frame);
        const unsigned char* runs = frame+2+headerCount*6;
        int y = *reinterpret_cast<const short*>(runs);
        const int endY = y+*reinterpret_cast<const short*>(runs+2);
        runs += 4;

        while (y<endY)
        {
            int x = 0;
            while (*reinterpret_cast<const unsigned short*>(runs) != 0u)
            {
                const unsigned int skip = runs[0];
                const unsigned int count = runs[1];
                runs += 2;
                x += static_cast<int>(skip);

                const unsigned char* pixels = runs;
                for (unsigned int i = 0;i<count;++i, ++x, pixels += 2)
                {
                    const int z = (static_cast<int>(*reinterpret_cast<const unsigned short*>(pixels))>>3)-128;
                    const int projectedY = y+z;
                    if (x >= 0 && x<2048 && projectedY >= 0 && projectedY<2048)
                    {
                        short& cell = cells[SoftwareGridCellIndex(x, projectedY)];
                        if (z>cell) cell = static_cast<short>(z);
                    }
                }

                runs += count*3u;
            }
            runs += 2;
            ++y;
        }
    }

    static void SoftwareRasterizeMaskFrame(const unsigned char* frame, short* cells)
    {
        const int headerCount = *reinterpret_cast<const short*>(frame);
        const unsigned char* runs = frame+2+headerCount*6;
        int y = *reinterpret_cast<const short*>(runs);
        const int endY = y+*reinterpret_cast<const short*>(runs+2);
        runs += 4;

        while (y<endY)
        {
            int x = 0;
            while (*reinterpret_cast<const unsigned short*>(runs) != 0u)
            {
                const unsigned int skip = *runs++;
                const unsigned int count = *runs++;
                x += static_cast<int>(skip);
                for (unsigned int i = 0;i<count;++i, ++x)
                {
                    if (x >= 0 && x<2048 && y >= 0 && y<2048)
                        cells[SoftwareGridCellIndex(x, y)] = 0;
                }
                runs += count;
            }
            runs += 2;
            ++y;
        }
    }
}


void VID_SOFTWARE::Load(RESOURCE* resource)
{

    if ((type&0x0100u) != 0u)
    {
        SetPropHide(1);
        return;
    }

    {
        STRING fileName = resource->GetFileName();
        mappedFileBase = static_cast<unsigned char*>(FFileMapping(fileName));
    }

    if (!mappedFileBase)
    {
        VidReportResourceError(this, E_CREATE, "file mapping", 0);
        SetPropHide(1);
        return;
    }

    if ((type&0x0008u) != 0u)
    {
        if (resource->GoNext(ResourceType::Palette) == 0)
        {
            const long paletteOffset = VidResourceTell(resource);
            mappedPalette = mappedFileBase+paletteOffset;
        }
        else
        {
            VidReportResourceError(this, E_LOAD, "PAL ", 0);
        }
    }

    if (resource->GoNext(ResourceType::Data) != 0)
        VidReportResourceError(this, E_LOAD, "DATA", 0);

    allocatedBytes = VidResourceSectionSize(resource);
    if ((type&0x0008u) != 0u) allocatedBytes += 2048u;

    frameOffsets = static_cast<unsigned int*>(CrtNew(static_cast<unsigned int>(static_cast<int>(noCadr))*sizeof(unsigned int)));
    if (!frameOffsets)
    {
        {
            STRING subject("cadrShift");
            Error(E_MEMORY, subject, static_cast<unsigned long>(static_cast<int>(noCadr)));
        }
        return;
    }

    if ((type&0x0008u) != 0u)
    {
        for (unsigned int offset = 0;offset<1024u;offset += 4u)
            *reinterpret_cast<unsigned int*>(paletteTables+offset) =
                *reinterpret_cast<const unsigned int*>(mappedPalette+offset);
        __movsd(reinterpret_cast<unsigned long*>(paletteTables+8192u),
                reinterpret_cast<const unsigned long*>(paletteTables), 256u);
    }

    unsigned int frameBytes;
    for (int frameIndex = 0;frameIndex<static_cast<int>(noCadr);++frameIndex)
    {
        (void)resource->Read(&frameBytes, sizeof(frameBytes));

        if (frameBytes != 2u)
        {
            frameOffsets[frameIndex] = static_cast<unsigned int>(VidResourceTell(resource));
        }
        else
        {
            const long frameOffset = VidResourceTell(resource);
            const short linkedFrame = *reinterpret_cast<const short*>(mappedFileBase+frameOffset);
            frameOffsets[frameIndex] = frameOffsets[linkedFrame];
        }

        VidSkipResourceBytes(resource, static_cast<long>(frameBytes));

        if (VidResourceBytesToEndOfSubrecord(resource) >= 12)
        {
            if (!cadrLinkXYZ)
                cadrLinkXYZ = static_cast<VECTOR*>(CrtNew(static_cast<unsigned int>(static_cast<int>(noCadr))*sizeof(VECTOR)));
            VECTOR* const frameLink = cadrLinkXYZ+frameIndex;
            (void)resource->Read(&frameLink->x, 4u);
            (void)resource->Read(&frameLink->y, 4u);
            (void)resource->Read(&frameLink->z, 4u);
        }

        (void)resource->GoNextSub(ResourceType::Data);
    }

    static_cast<void>(VID::MemoryInUse = VID::MemoryInUse+allocatedBytes);

    SetLayer();

    if ((property&0x20u) == 0u || (type&0x0008u) == 0u || (type&0x0001u) == 0u)
        return;

    minXGridZ = maxXGridZ = minYGridZ = maxYGridZ = 0.0f;

    VECTOR* const scratch = static_cast<VECTOR*>(CrtNew(50331648u));

    gridCadrShift = static_cast<int*>(CrtNew(static_cast<unsigned int>(static_cast<int>(noCadr))*sizeof(int)));

    short cells[256*256];
    for (int frameIndex = 0;frameIndex<static_cast<int>(noCadr);++frameIndex)
    {
        for (int i = 0;i<256*256;++i) cells[i] = kEmptySoftwareGridCell;
        gridCadrShift[frameIndex] = noGridZ;

        const unsigned char* const frame = mappedFileBase+frameOffsets[frameIndex];
        if ((type&0x0004u) != 0u) SoftwareRasterizeDepthFrame(frame, cells);
        else SoftwareRasterizeMaskFrame(frame, cells);

        const int rows = static_cast<int>(vidSizeY)/8;
        const int columns = static_cast<int>(vidSizeX)/8;
        for (int row = rows-1;row >= 0;--row)
        {
            for (int column = 0;column<columns;++column)
            {
                const short z = cells[row*256+column];
                if (z == kEmptySoftwareGridCell) continue;

                VECTOR& point = scratch[noGridZ++];
                point.x = static_cast<float>(column*8-static_cast<int>(vidSizeX)/2);
                point.y = static_cast<float>(row*8-static_cast<int>(vidSizeY)/2);
                point.z = static_cast<float>(z);
            }
        }
    }

    gridZ = static_cast<VECTOR*>(CrtNew(static_cast<unsigned int>(noGridZ)*sizeof(VECTOR)));
    for (int i = 0;i<noGridZ;++i)
    {
        gridZ[i] = scratch[i];
        if (gridZ[i].x<minXGridZ) minXGridZ = gridZ[i].x;
        if (gridZ[i].x>maxXGridZ) maxXGridZ = gridZ[i].x;
        if (gridZ[i].y<minYGridZ) minYGridZ = gridZ[i].y;
        if (gridZ[i].y>maxYGridZ) maxYGridZ = gridZ[i].y;
    }

    CrtFreePointer(scratch);
}


void VID_LIGHT::Load(RESOURCE* resource)
{
    if (resource->GoNext(ResourceType::Data) != 0)
    {
        const STRING text("DATA");
        Error(E_LOAD, text, 0u);
    }

    vidSizeX = static_cast<short>(TruncateFloatToInt32(sizeXYZ.x));
    vidSizeY = static_cast<short>(TruncateFloatToInt32(sizeXYZ.y));
    buildSizeX = sizeXYZ.x;
    buildSizeY = sizeXYZ.y;

    lightDataBytes = static_cast<unsigned int>(resource->SubLoad(&lightData, 0));
    if (lightDataBytes == 0u)
    {
        const STRING text("cadr");
        Error(E_LOAD, text, 0u);
    }
    static_cast<void>(VID::MemoryInUse = VID::MemoryInUse+lightDataBytes);
}


void VID_LIGHT::SetLayer()
{
    layer = 7;
}

void VID_LIGHT::Draw(SPRITE* sprite)
{

    const GAMMA spriteGamma = sprite->GetGamma();
    GAMMA vidGamma;
    vidGamma.modulate = gammaRaw.modulate;
    vidGamma.add = gammaRaw.add;
    GAMMA finalGamma(vidGamma, spriteGamma);
    if ((property&0x800u) == 0u)
    {
        GAMMA graphGamma;
        graphGamma.modulate = Graph->gamma.modulate;
        graphGamma.add = Graph->gamma.add;
        finalGamma = GAMMA(graphGamma, finalGamma);
    }

    const int frameIndex = sprite->CurrentFrame();
    const unsigned int frameColor = reinterpret_cast<const unsigned int*>(lightData)[frameIndex];
    if ((runtimeFlags&kVidRuntimeHidden) != 0u || (frameColor&0x00FFFFFFu) == 0u)
        return;

    VECTOR drawScale;
    EX_SPRITE_DATA* const transformOwner = sprite->ExtraData();
    if (transformOwner)
        drawScale = transformOwner->scale;
    else
    {
        drawScale.x = 1.0f;
        drawScale.y = 1.0f;
        drawScale.z = 1.0f;
    }

    VECTOR2 halfSize;
    halfSize.x = sizeXYZ.x*0.5f*scaleXYZ.x*drawScale.x;
    halfSize.y = sizeXYZ.y*0.5f*scaleXYZ.y*drawScale.y;

    VECTOR position;
    position = sprite->Coor();

    if ((runtimeFlags&kVidAnimateScale) != 0u)
    {
        const WEAPON_RUNTIME* const curveOwner =
            weapon;
        const float phase = transformOwner->effectCurvePosition;
        const VECTOR curveScale = EvaluateVidHardwareCurve(curveOwner, curveOwner->Scale, phase);
        halfSize.x *= curveScale.x;
        halfSize.y *= curveScale.y;
    }

    if ((runtimeFlags&kVidAnimateOffset) != 0u)
    {
        const WEAPON_RUNTIME* const curveOwner =
            weapon;
        const float phase = transformOwner->effectCurvePosition;
        const VECTOR curveOffset = EvaluateVidHardwareCurve(curveOwner, curveOwner->Shift, phase);
        position.x += curveOffset.x;
        position.y += curveOffset.y;
        position.z += curveOffset.z;
    }

    if ((property&kVidApplyDirectionOffset) != 0u)
    {
        const unsigned int waveIndex = (::CurrentTime>>3u)&0xFFu;
        position.z += FSin[waveIndex]*topZ;
    }

    if ((property&0x00001000u) != 0u)
    {
        VECTOR2 shadowOffset;
        shadowOffset.x = 0.0f;
        shadowOffset.y = 0.0f;
        if ((property&0x20000000u) != 0u)
        {
            if (transformOwner && transformOwner->items.m_count == 2)
            {
                const int* const pixelOffset = transformOwner->items.m_data;
                shadowOffset.x = static_cast<float>(pixelOffset[0]);
                shadowOffset.y = static_cast<float>(pixelOffset[1]);
            }
            else
            {
                const ANGLE directionAngle(sprite->Direction().value);
                const float radius = halfSize.y*(0.300000012f);
                shadowOffset.x = -radius*directionAngle.Sin();
                shadowOffset.y = radius*directionAngle.CosY();
            }
        }
        Graph->DrawShadowToStencil(position, halfSize, shadowOffset);
    }

    COLOR sourceColor;
    sourceColor.value = frameColor;
    const COLOR drawColor(finalGamma, sourceColor);
    Graph->DrawLight(position, halfSize, drawColor);
}


void VID_FONT::InvalidateDeviceObjects()
{
    if (fontBackend && (storageFlags&4u) != 0u)
        (void)fontBackend->OnLostDevice();
}


void VID_FONT::RestoreDeviceObjects()
{
    if (fontBackend && (storageFlags&4u) != 0u)
        (void)fontBackend->OnResetDevice();
}


void VID_FONT::SetLayer()
{
    layer = 9;
}


void VID_FONT::Draw(SPRITE* sprite)
{
    if (!fontBackend) return;

    const GAMMA gamma = sprite->GetGamma();
    const unsigned int gammaAdd = gamma.add;
    unsigned int color = gammaAdd|0xFF000000u;

    VECTOR2 projected(sprite->Coor().x, sprite->Coor().y-sprite->Coor().z);
    VECTOR2 shifted = projected-Map->ShiftXY();
    const int x = TruncateFloatToInt32(shifted.x);
    projected.x = sprite->Coor().x;
    projected.y = sprite->Coor().y-sprite->Coor().z;
    shifted = projected-Map->ShiftXY();
    int y = TruncateFloatToInt32(shifted.y);

    if ((property&kVidApplyDirectionOffset) != 0u)
    {
        const unsigned int index = (::CurrentTime>>3)&0xFFu;
        const float wave = FSin[index];
        y = BitsToSigned32(Signed32ToBits(y)-Signed32ToBits(TruncateFloatToInt32(wave*topZ)));
    }

    if (sprite == Mouse)
        y = BitsToSigned32(Signed32ToBits(y)-Signed32ToBits(static_cast<int>(vidSizeY)/2));
    else
    {
        const unsigned int behavior = sprite->Behave();
        if ((behavior&0x08u) != 0u)
            y = BitsToSigned32(Signed32ToBits(y)-Signed32ToBits(static_cast<int>(vidSizeY)/2));
        else if ((behavior&0x04u) != 0u)
            y = BitsToSigned32(Signed32ToBits(y)-Signed32ToBits(static_cast<int>(vidSizeY)));
    }

    RECT rect = {x, y, BitsToSigned32(Signed32ToBits(x)+1u), BitsToSigned32(Signed32ToBits(y)+1u)};
    GRAPH* const video = Graph;
    if (!(video->screenWidth >= static_cast<float>(x)) || !(video->screenHeight >= static_cast<float>(y))) return;
    if (BitsToSigned32(Signed32ToBits(x)+6u*Signed32ToBits(static_cast<int>(vidSizeX))+10u)<0 ||
       BitsToSigned32(Signed32ToBits(y)+Signed32ToBits(static_cast<int>(vidSizeY))+10u)<0) return;
    if ((color&0x00FFFFFFu) == 0u) color = 0xFFFFFFFFu;

    IDirect3DDevice9* const device = video->device;
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    (void)Graph->device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    (void)fontBackend->DrawTextA(
        "abcABC", 6, &rect, (DT_NOCLIP|DT_SINGLELINE|DT_NOPREFIX), color);
}

void VID_SURFACE::Load(RESOURCE* resource)
{


    if (nextMirror == this)
    {
        VidReleaseFrameArray(*this);
        surfaceData = 0;
    }

    (void)resource->Read(&pixelFormat, sizeof(pixelFormat));
    (void)resource->Read(&surfaceMode, sizeof(surfaceMode));
    if (VidResourceBytesToEndOfSubrecord(resource) != 0)
        (void)resource->Read(&renderMode, sizeof(renderMode));

    if (pixelFormat == D3DFMT_UNKNOWN) return;

    const int frameCount = static_cast<int>(noCadr);
    frames = new(std::nothrow) CADR_DATA[frameCount];

    unsigned int cachedOffset = frames?PtrToU32(frames)-4u:0u;

    struct CachedSurfaceHeader
    {
        unsigned int key0;
        unsigned int key1;
    };
    struct CachedFrameRecord
    {
        unsigned short indexCount;
        unsigned short vertexCount;
        unsigned short width;
        unsigned short height;
        unsigned short offsetX;
        unsigned short offsetY;
        unsigned int frameDataOffset;
        float minZ;
        float maxZ;
    };

    if (VidResourceBytesToEndOfSubrecord(resource) != 0 &&
       Map->m_fastLoadIndex != 0)
    {
        CachedSurfaceHeader requested;
        (void)resource->Read(&cachedOffset, sizeof(cachedOffset));
        (void)resource->Read(&requested, sizeof(requested));
        const CachedSurfaceHeader* const available =
            reinterpret_cast<const CachedSurfaceHeader*>(Map->m_fastLoadIndex);

        if (requested.key0 == available->key0 && requested.key1 == available->key1)
        {
            type = static_cast<unsigned short>(type&0xDFFBu);
            storageFlags|=8u;
            surfaceData = Map->m_fastLoadData;

            const unsigned char* cursor =
                Map->m_fastLoadIndex+cachedOffset;
            for (int i = 0;i<static_cast<int>(noCadr);++i)
            {
                const CachedFrameRecord& packed =
                    *reinterpret_cast<const CachedFrameRecord*>(cursor);
                frames[i].indexCount = packed.indexCount;
                frames[i].vertexCount = packed.vertexCount;
                frames[i].width = packed.width;
                frames[i].height = packed.height;
                frames[i].offsetX = packed.offsetX;
                frames[i].offsetY = packed.offsetY;
                frames[i].frameDataOffset = packed.frameDataOffset;
                std::memcpy(&frames[i].minZ, &packed.minZ, sizeof(float));
                std::memcpy(&frames[i].maxZ, &packed.maxZ, sizeof(float));
                cursor+=sizeof(CachedFrameRecord);
            }

            minXGridZ = *reinterpret_cast<const float*>(cursor);
            cursor+=4;
            CADR_DATA* const firstFrameRecord = frames;
            maxXGridZ = *reinterpret_cast<const float*>(cursor);
            cursor+=4;
            minYGridZ = *reinterpret_cast<const float*>(cursor);
            cursor+=4;
            maxYGridZ = *reinterpret_cast<const float*>(cursor);
            cursor+=4;

            unsigned char* const firstFrame = surfaceData+firstFrameRecord[0].frameDataOffset;
            const unsigned int packedGridCount =
                *reinterpret_cast<const unsigned int*>(firstFrame-sizeof(unsigned int));
            unsigned int prefixBytes = sizeof(unsigned int);
            if (packedGridCount != 0u)
                prefixBytes = (packedGridCount*3u+static_cast<unsigned int>(static_cast<int>(noCadr)))*4u+4u;

            const unsigned short cachedType = type;
            if ((cachedType&0x0008u) != 0u)
            {
                prefixBytes+=1024u;
                palette = reinterpret_cast<unsigned int*>(firstFrame-prefixBytes);
            }

            if ((cachedType&0x0200u) != 0u)
            {
                unsigned char* linkBase = firstFrame-prefixBytes;
                linkBase-=static_cast<unsigned int>(static_cast<int>(noCadr))*sizeof(VECTOR);
                cadrLinkXYZ = reinterpret_cast<VECTOR*>(linkBase);
            }
            return;
        }
    }
    else if (VidResourceBytesToEndOfSubrecord(resource) == 0)
    {
        VidReportResourceError(this, E_ERROR,
            "old format vid. Can't create fastLoad", 0);
    }

    {
        STRING fileName = resource->GetFileName();
        surfaceData = static_cast<unsigned char*>(FFileMapping(fileName));
    }
    if (!surfaceData)
    {
        VidReportResourceError(this, E_CREATE, "file mapping", 0);
        return;
    }

    if ((type&0x0008u) != 0u)
    {
        if (resource->GoNext(ResourceType::Palette) == 0)
        {
            const long paletteOffset = VidResourceTell(resource);
            palette = reinterpret_cast<unsigned int*>(surfaceData+paletteOffset);
        }
        else
        {
            VidReportResourceError(this, E_LOAD, "PAL ", 0);
            return;
        }
    }

    (void)resource->GoNext(ResourceType::Any);

    if ((type&0x0200u) != 0u)
    {
        const int linkCount = static_cast<int>(noCadr);
        cadrLinkXYZ = static_cast<VECTOR*>(CrtNew(static_cast<unsigned int>(linkCount)*sizeof(VECTOR)));
    }

    if (resource->RecordType() != ResourceType::Frame)
        (void)resource->GoNext(ResourceType::Frame);
    if (resource->RecordType() != ResourceType::Frame)
    {
        VidReportResourceError(this, E_LOAD, "CADR", 0);
        goto surfaceFrameBounds;
    }

    for (int frameIndex = 0;frameIndex<static_cast<int>(noCadr);++frameIndex)
    {
        if (VidResourceBytesToEndOfSubrecord(resource)>4)
        {
            if ((type&0x0200u) != 0u)
            {
                VECTOR* const frameLink = cadrLinkXYZ+frameIndex;
                (void)resource->Read(&frameLink->x, 4u);
                (void)resource->Read(&frameLink->y, 4u);
                (void)resource->Read(&frameLink->z, 4u);
            }

            (void)resource->Read(&frames[frameIndex].indexCount, 2u);
            if ((type&0x0800u) != 0u) VidSkipResourceBytes(resource, 2);
            (void)resource->Read(&frames[frameIndex].vertexCount, 2u);
            (void)resource->Read(&frames[frameIndex].width, 2u);
            (void)resource->Read(&frames[frameIndex].height, 2u);
            (void)resource->Read(&frames[frameIndex].offsetX, 2u);
            (void)resource->Read(&frames[frameIndex].offsetY, 2u);
            const unsigned int streamPosition = static_cast<unsigned int>(VidResourceTell(resource));
            frames[frameIndex].frameDataOffset = streamPosition;
        }
        else
        {
            const long linkOffset = VidResourceTell(resource);
            const int linkedIndex = *reinterpret_cast<const int*>(surfaceData+linkOffset);
            CADR_DATA& frame = frames[frameIndex];
            CADR_DATA& linked = frames[linkedIndex];
            if (&linked != &frame)
            {
                frame.lruNext = 0u;
                frame.lruPrevious = 0u;
                frame.lastUseTick = linked.lastUseTick;
                frame.indexCount = linked.indexCount;
                frame.vertexCount = linked.vertexCount;
                frame.width = linked.width;
                frame.height = linked.height;
                frame.offsetX = linked.offsetX;
                frame.offsetY = linked.offsetY;
                frame.frameDataOffset = linked.frameDataOffset;
                std::memcpy(&frame.maxZ, &linked.maxZ, sizeof(float));
                frame.textureObject = linked.textureObject;
                frame.vertexBuffer = linked.vertexBuffer;
                frame.indexBuffer = linked.indexBuffer;
            }
        }

        if (frameIndex<static_cast<int>(noCadr)-1)
            (void)resource->GoNextSub(ResourceType::Frame);
    }

surfaceFrameBounds:
    if ((property&0x1000u) != 0u && frames)
        frames[0].CalcMaxMinZ(this);
}


void VID_SURFACE::SetReColorForArmy(COLOR color)
{
    if ((type&8u) == 0u)
    {
        const STRING text("SetReColorForArmy for non paletted vid");
        Error(E_ERROR, text, 0u);
        return;
    }

    if (!recolorPalette)
        recolorPalette = static_cast<unsigned int*>(CrtNew(1024u));

    for (unsigned int i = 0;i<256u;++i)
    {
        const unsigned int sourceColor = palette[i];
        const int red = static_cast<int>((sourceColor>>16u)&0xFFu);
        const int green = static_cast<int>((sourceColor>>8u)&0xFFu);
        const int blue = static_cast<int>(sourceColor&0xFFu);
        const float ramp = static_cast<float>(red)*(1.0f/148.0f);

        if (red>20 &&
           green <= static_cast<int>(ramp*80.0f) &&
           (red >= blue?red-blue:blue-red) <= static_cast<int>(ramp*30.0f))
        {
            const float recolorScale = static_cast<float>(red)*(1.0f/128.0f);
            const int scaledBlue = static_cast<int>(static_cast<float>(color.value&0xFFu)*recolorScale);
            const int scaledGreen = static_cast<int>(static_cast<float>((color.value>>8u)&0xFFu)*recolorScale);
            const int scaledRed = static_cast<int>(static_cast<float>((color.value>>16u)&0xFFu)*recolorScale);
            const unsigned int tint = 0xFF000000u|
                (static_cast<unsigned int>(ClampByteToRange(scaledRed))<<16u)|
                (static_cast<unsigned int>(ClampByteToRange(scaledGreen))<<8u)|
                static_cast<unsigned int>(ClampByteToRange(scaledBlue));

            const unsigned int gray = static_cast<unsigned int>(green);
            const unsigned int addBlue = (gray+(tint&0xFFu)>0xFFu)?0xFFu:gray+(tint&0xFFu);
            const unsigned int addGreen = ((gray<<8u)+(tint&0xFF00u)>0xFF00u)?0xFF00u:(gray<<8u)+(tint&0xFF00u);
            const unsigned int addRed = ((gray<<16u)+(tint&0xFF0000u)>0xFF0000u)?0xFF0000u:(gray<<16u)+(tint&0xFF0000u);
            recolorPalette[i] = 0xFF000000u|addRed|addGreen|addBlue;
        }
        else
        {
            recolorPalette[i] = sourceColor;
        }
    }

    if (nextMirror != this)
    {
        VID* predecessor = nextMirror;
        while (predecessor->nextMirror != this) predecessor = predecessor->nextMirror;
        predecessor->nextMirror = nextMirror;

        CADR_DATA* const oldFrames = frames;
        const int frameCount = static_cast<int>(noCadr);
        nextMirror = this;
        frames = new(std::nothrow) CADR_DATA[frameCount];
        for (int i = 0;i<frameCount;++i)
        {
            CADR_DATA& dst = frames[i];
            const CADR_DATA& src = oldFrames[i];
            dst.lastUseTick = src.lastUseTick;
            dst.lruPrevious = 0u;
            dst.lruNext = 0u;
            dst.indexCount = src.indexCount;
            dst.vertexCount = src.vertexCount;
            dst.width = src.width;
            dst.height = src.height;
            dst.offsetX = src.offsetX;
            dst.offsetY = src.offsetY;
            dst.frameDataOffset = src.frameDataOffset;
            std::memcpy(&dst.maxZ, &src.maxZ, sizeof(float));
            dst.textureObject = src.textureObject;
            dst.vertexBuffer = src.vertexBuffer;
            dst.indexBuffer = src.indexBuffer;
        }

        if (cadrLinkXYZ && (storageFlags&8u) == 0u)
        {
            VECTOR* const oldLinks = cadrLinkXYZ;
            cadrLinkXYZ = static_cast<VECTOR*>(CrtNew(static_cast<unsigned int>(frameCount)*sizeof(VECTOR)));
            for (int i = 0;i<frameCount;++i) cadrLinkXYZ[i] = oldLinks[i];
        }
        return;
    }

    ReleaseVideoMemory();
}


void VID_SURFACE::InvalidateDeviceObjects()
{
    if ((storageFlags&4u) != 0u && surfaceBackend)
    {
        (void)surfaceBackend->Release();
        surfaceBackend = 0;
    }
    ReleaseVideoMemory();
}

namespace
{
struct VidSurfaceDrawVertex
{
    float x,y,z,rhw;
    unsigned int diffuse;
    unsigned int specular;
    float u,v;
};
struct VidSurfaceDrawDualVertex
{
    float x,y,z,rhw;
    unsigned int diffuse;
    unsigned int specular;
    float u0,v0,u1,v1;
};

static void VidSurfaceDrawSeed(VID_SURFACE* owner, const CADR_DATA* frame,
    VidSurfaceDrawVertex* vertices, unsigned int* vertexCount)
{
    const TEXTURE* texture = frame->textureObject;
    const float textureWidth = static_cast<float>(texture->m_width);
    const float textureHeight = static_cast<float>(texture->m_height);
    const float halfTexel = (0.5f);
    const float isoScale = (1.41430664f);
    const int width = static_cast<int>(static_cast<short>(frame->width));
    const int height = static_cast<int>(static_cast<short>(frame->height));
    const int frameX = static_cast<int>(static_cast<short>(frame->offsetX));
    const int frameY = static_cast<int>(static_cast<short>(frame->offsetY));
    const int halfOwnerX = VidHardwareHalfTowardZero(owner->vidSizeX);
    const int halfOwnerY = VidHardwareHalfTowardZero(owner->vidSizeY);
    const int mode = owner->surfaceMode;

    if (mode == 2)
    {
        const float halfWidth = static_cast<float>(width)*halfTexel;
        const float halfHeight = static_cast<float>(height)*halfTexel;
        const float sixthHeight = static_cast<float>(height)*(0.166666672f);
        const float xSource[8] = {halfWidth, 0.0f, halfWidth, static_cast<float>(width), static_cast<float>(width), halfWidth, 0.0f, 0.0f};
        const float ySource[8] = {sixthHeight, 0.0f, 0.0f, 0.0f, static_cast<float>(height), static_cast<float>(height), static_cast<float>(height), 0.0f};
        const float depthSource[8] = {halfHeight, 0.0f, sixthHeight, 0.0f, 0.0f, sixthHeight, 0.0f, 0.0f};
        *vertexCount = 8u;
        for (unsigned int i = 0u;i<8u;++i)
        {
            vertices[i].x = xSource[i]-static_cast<float>(halfOwnerX)+static_cast<float>(frameX);
            vertices[i].y = (ySource[i]-static_cast<float>(halfOwnerY)+static_cast<float>(frameY))*isoScale;
            vertices[i].z = depthSource[i];
            vertices[i].u = (xSource[i]+halfTexel)/textureWidth;
            vertices[i].v = (ySource[i]+halfTexel)/textureHeight;
        }
        return;
    }

    const bool specialFour = (mode == 0) ||
        (mode == -1 && owner->sizeXYZ.z>owner->sizeXYZ.y);
    *vertexCount = 4u;
    const int xSource[4] = {0, width, 0, width};
    const int ySource[4] = {0, 0, height, height};
    if (specialFour && mode != 3)
    {
        const int depthSource[4] = {height, height, 0, 0};
        for (unsigned int i = 0u;i<4u;++i)
        {
            vertices[i].x = static_cast<float>(xSource[i]-halfOwnerX+frameX);
            vertices[i].y = static_cast<float>(ySource[i]-halfOwnerY+frameY)*isoScale;
            vertices[i].z = static_cast<float>(depthSource[i]);
            vertices[i].u = (static_cast<float>(xSource[i])+halfTexel)/textureWidth;
            vertices[i].v = (static_cast<float>(ySource[i])+halfTexel)/textureHeight;
        }
        return;
    }
    for (unsigned int i = 0u;i<4u;++i)
    {
        vertices[i].x = static_cast<float>(xSource[i]-halfOwnerX+frameX);
        vertices[i].y = static_cast<float>(ySource[i]-halfOwnerY+frameY)*isoScale;
        vertices[i].z = 0.0f;
        vertices[i].u = (static_cast<float>(xSource[i])+halfTexel)/textureWidth;
        vertices[i].v = (static_cast<float>(ySource[i])+halfTexel)/textureHeight;
    }
}

static void VidSurfaceApplyAtlasClass13(VID_SURFACE* owner, const SPRITE* request,
    const CADR_DATA* frame, VidSurfaceDrawVertex* vertices)
{
    if (owner->spriteClass != 19u) return;
    const TEXTURE* texture = frame->textureObject;
    const float textureWidth = static_cast<float>(texture->m_width);
    const float textureHeight = static_cast<float>(texture->m_height);
    const int frameIndex = static_cast<int>(request->CurrentFrame());
    const int column = frameIndex%16;
    const int row = frameIndex/16;
    const float uShift = static_cast<float>(column)*(static_cast<float>(static_cast<short>(frame->width))+1.0f)/textureWidth;
    const float vShift = static_cast<float>(row)*(static_cast<float>(static_cast<short>(frame->height))+1.0f)/textureHeight;
    for (unsigned int i = 0u;i<4u;++i)
    {
        vertices[i].u += uShift;
        vertices[i].v += vShift;
    }
}

static void VidSurfaceTransformDrawVertices(VID_SURFACE* owner, const VidHardwareDrawTransform& transform,
    unsigned char direction, unsigned int diffuse, unsigned int specular,
    VidSurfaceDrawVertex* vertices, unsigned int vertexCount)
{
    const ANGLE rotation(direction);
    const float rotateXX = rotation.Cos();
    const float rotateXY = rotation.Sin();
    const float rotateYX = rotation.SinY();
    const float rotateYY = rotation.CosY();
    const float isoWorldY = transform.position.y-transform.position.z;
    for (unsigned int i = 0u;i<vertexCount;++i)
    {
        VidSurfaceDrawVertex& vertex = vertices[i];
        const float scaledX = vertex.x*transform.scale.x;
        const float scaledY = vertex.y*transform.scale.y;
        const float depth = vertex.z*transform.scale.z;
        const float rotatedX = static_cast<float>(static_cast<double>(scaledX)*rotateXX-static_cast<double>(scaledY)*rotateXY);
        const float rotatedY = static_cast<float>(static_cast<double>(scaledY)*rotateYY+static_cast<double>(scaledX)*rotateYX);
        if (static_cast<unsigned int>(owner->layer) != 9u)
        {
            const VECTOR2 projected = Map->ToScreenScaled(
                VECTOR2(transform.position.x+rotatedX, isoWorldY+rotatedY));
            vertex.x = projected.x;
            vertex.y = projected.y;
        }
        else
        {
            vertex.x = transform.position.x+rotatedX-Map->ShiftXY().x;
            vertex.y = static_cast<float>(static_cast<double>(isoWorldY)+rotatedY-Map->ShiftXY().y);
        }
        unsigned int color = diffuse;
        const unsigned int format = owner->renderMode;
        if ((static_cast<unsigned char>(owner->type)&2u) == 0u && (format == 0u || format == 6u) &&
           static_cast<int>(Graph->graphDetail) >= 8 && (Map->Flags()&0x100u) == 0u)
        {
            int alpha = VidHardwareHalfTowardZero(TruncateDoubleToInt32(static_cast<double>(transform.position.z)+depth));
            if (alpha <= 1) alpha = 1;
            else if (alpha >= 255) alpha = 255;
            color = (diffuse&0x00FFFFFFu)|(static_cast<unsigned int>(alpha)<<24u);
        }
        vertex.diffuse = color;
        vertex.specular = specular;
        vertex.z = static_cast<float>((static_cast<double>(transform.position.z)+depth+128.0)*0.0001220703125);
        vertex.rhw = 1.0f;
    }
}

static unsigned int VidSurfaceSubmitDrawQuad(VID_SURFACE* owner, const SPRITE* request,
    const CADR_DATA* frame, const VidHardwareDrawTransform& transform, unsigned char direction,
    unsigned int diffuse, unsigned int specular)
{
    VidSurfaceDrawVertex vertices[8];
    unsigned int vertexCount = 4u;
    VidSurfaceDrawSeed(owner, frame, vertices, &vertexCount);
    VidSurfaceApplyAtlasClass13(owner, request, frame, vertices);
    VidSurfaceTransformDrawVertices(owner, transform, direction, diffuse, specular, vertices, vertexCount);
    if (owner->layer == 7)
    {
            const TEXTURE* const texture = Graph->lightRenderTexture;
        for (unsigned int i = 0u;i<vertexCount;++i)
        {
            vertices[i].x = static_cast<float>(static_cast<double>(texture->m_width)/Graph->screenWidth*vertices[i].x);
            vertices[i].y = static_cast<float>(static_cast<double>(texture->m_height)/Graph->screenHeight*vertices[i].y);
        }
    }
    static_cast<void>(VID_SURFACE::noVertex = VID_SURFACE::noVertex+4u);
    static_cast<void>(VID_SURFACE::noTri = VID_SURFACE::noTri+2u);
    (void)Graph->device->SetRenderState(D3DRS_SPECULARENABLE, specular?1u:0u);
    (void)Graph->device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1);
    const unsigned int mode = static_cast<unsigned int>(owner->surfaceMode);
    return static_cast<unsigned int>(Graph->device->DrawPrimitiveUP(
        (mode == 2u?D3DPT_TRIANGLEFAN:D3DPT_TRIANGLESTRIP), mode == 2u?6u:2u, vertices, 32u));
}

static unsigned int VidSurfaceSubmitDrawDualQuad(VID_SURFACE* owner, const SPRITE* request,
    const CADR_DATA* frame, const VidHardwareDrawTransform& transform, unsigned char direction,
    unsigned int diffuse, unsigned int specular)
{
    VidSurfaceDrawVertex baseVertices[8];
    unsigned int vertexCount = 4u;
    VidSurfaceDrawSeed(owner, frame, baseVertices, &vertexCount);
    VidSurfaceTransformDrawVertices(owner, transform, direction, diffuse, specular, baseVertices, vertexCount);
    VidSurfaceDrawDualVertex dualTextureVertices[8];
    for (unsigned int i = 0u;i<vertexCount;++i)
    {
        dualTextureVertices[i].x = baseVertices[i].x;
        dualTextureVertices[i].y = baseVertices[i].y;
        dualTextureVertices[i].z = baseVertices[i].z;
        dualTextureVertices[i].rhw = baseVertices[i].rhw;
        dualTextureVertices[i].diffuse = baseVertices[i].diffuse;
        dualTextureVertices[i].specular = baseVertices[i].specular;
        dualTextureVertices[i].u0 = baseVertices[i].u;
        dualTextureVertices[i].v0 = baseVertices[i].v;
        dualTextureVertices[i].u1 = (baseVertices[i].x+(0.5f))/Graph->screenWidth;
        dualTextureVertices[i].v1 = (baseVertices[i].y+(0.5f))/Graph->screenHeight;
    }
    static_cast<void>(VID_SURFACE::noVertex = VID_SURFACE::noVertex+4u);
    static_cast<void>(VID_SURFACE::noTri = VID_SURFACE::noTri+2u);
    (void)Graph->device->SetRenderState(D3DRS_SPECULARENABLE, specular?1u:0u);
    (void)Graph->device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2);
    (void)Graph->device->SetTextureStageState(1u, D3DTSS_COLOROP, D3DTOP_MODULATE4X);
    (void)Graph->device->SetTextureStageState(1u, D3DTSS_COLORARG1, 2u);
    (void)Graph->device->SetTextureStageState(1u, D3DTSS_COLORARG2, D3DTA_CURRENT);
    (void)Graph->device->SetTextureStageState(1u, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
    (void)Graph->device->SetTextureStageState(1u, D3DTSS_TEXCOORDINDEX, 1u);
    TEXTURE* const lightTexture = Graph->lightRenderTexture;
    (void)Graph->device->SetTexture(1u, reinterpret_cast<IDirect3DBaseTexture9*>(lightTexture->m_texture));
    const unsigned int mode = static_cast<unsigned int>(owner->surfaceMode);
    const unsigned int result = static_cast<unsigned int>(Graph->device->DrawPrimitiveUP(
        (mode == 2u?D3DPT_TRIANGLEFAN:D3DPT_TRIANGLESTRIP), mode == 2u?6u:2u, dualTextureVertices, 40u));
    (void)Graph->device->SetTexture(0u, 0);
    (void)Graph->device->SetTextureStageState(1u, D3DTSS_COLOROP, D3DTOP_DISABLE);
    (void)Graph->device->SetTextureStageState(1u, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    return result;
}

static unsigned int VidSurfaceSubmitDrawMesh(VID_SURFACE* owner, const CADR_DATA* frame,
    const VidHardwareDrawTransform& transform, unsigned char direction, const GAMMA& finalGamma)
{
    const ANGLE rotation(direction);
    float matrix[16];
    matrix[0] = rotation.Cos()*transform.scale.x;
    matrix[1] = rotation.SinY()*transform.scale.x;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;
    matrix[4] = -rotation.Sin()*transform.scale.y;
    matrix[5] = rotation.CosY()*transform.scale.y;
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;
    matrix[8] = 0.0f;
    matrix[9] = 0.0f;
    matrix[10] = transform.scale.z;
    matrix[11] = 0.0f;
    matrix[12] = transform.position.x;
    matrix[13] = transform.position.y;
    matrix[14] = transform.position.z;
    matrix[15] = 1.0f;
    Graph->SetTransformMatrix(D3DTS_WORLD, reinterpret_cast<const _D3DMATRIX*>(matrix), &finalGamma);
    if (Graph->vertexShaderVersion<0x100u)
    {
        (void)Graph->device->SetRenderState(D3DRS_SPECULARENABLE, 0u);
        (void)Graph->pseudo3DEffect->SetTechnique(Graph->drawObjectWithoutShaderTechnique);
    }
    else
    {
        D3DXHANDLE technique = Graph->drawObjectTechnique;
        const unsigned int format = owner->renderMode;
        if ((static_cast<unsigned char>(owner->type)&2u) == 0u && (format == 0u || format == 6u) && static_cast<int>(Graph->graphDetail) >= 8 &&
           (Map->Flags()&0x100u) == 0u) technique = Graph->drawObjectWithZTechnique;
        (void)Graph->pseudo3DEffect->SetTechnique(technique);
        (void)Graph->device->SetRenderState(D3DRS_SPECULARENABLE, finalGamma.add?1u:0u);
        float frameTransform[4] = {
            static_cast<float>(VidHardwareHalfTowardZero(owner->vidSizeX)-static_cast<int>(static_cast<short>(frame->offsetX)))+(0.5f),
            static_cast<float>(VidHardwareHalfTowardZero(owner->vidSizeY)-static_cast<int>(static_cast<short>(frame->offsetY)))+(0.5f),
            1.0f, 1.0f};
        (void)Graph->pseudo3DEffect->SetVector(Graph->texShiftParameter, reinterpret_cast<const D3DXVECTOR4*>(frameTransform));
        const TEXTURE* texture = frame->textureObject;
        float textureTransform[4] = {static_cast<float>(texture->m_width), static_cast<float>(texture->m_height), 1.0f, 1.0f};
        (void)Graph->pseudo3DEffect->SetVector(Graph->texSizeParameter, reinterpret_cast<const D3DXVECTOR4*>(textureTransform));
    }
    static_cast<void>(VID_SURFACE::noVertex = VID_SURFACE::noVertex+static_cast<unsigned int>(frame->vertexCount));
    const unsigned int primitiveCount = static_cast<unsigned int>(frame->indexCount)/3u;
    static_cast<void>(VID_SURFACE::noTri = VID_SURFACE::noTri+primitiveCount);
    unsigned int passes = 0u;
    (void)Graph->pseudo3DEffect->Begin(&passes, 0u);
    unsigned int result = FloatBitsU32(transform.position.x);
    for (unsigned int pass = 0u;pass<passes;++pass)
    {
        (void)Graph->pseudo3DEffect->BeginPass(pass);
        if (VID::optDrawFlagmanBehindWalls != 0u)
        {
            (void)Graph->device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
            (void)Graph->device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
            (void)Graph->SetAlphaBlend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
            (void)Graph->device->SetRenderState(D3DRS_COLORWRITEENABLE, 7u);
            (void)Graph->device->SetTextureStageState(0u, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            (void)Graph->device->SetTextureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            (void)Graph->device->SetRenderState(D3DRS_TEXTUREFACTOR, ((~finalGamma.modulate)&0x00FFFFFFu)|0x40000000u);
        }
        result = static_cast<unsigned int>(Graph->device->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST, 0, 0, static_cast<unsigned int>(frame->vertexCount), 0u, primitiveCount));
        (void)Graph->pseudo3DEffect->EndPass();
    }
    (void)Graph->pseudo3DEffect->End();
    return result;
}

static void VidSurfaceDrawSetRenderStates(VID_SURFACE* owner, const VidHardwareDrawTransform& transform,
    const GAMMA& finalGamma)
{
    unsigned int sampler = D3DTEXF_LINEAR;
    if (((static_cast<unsigned char>(owner->type)&2u) == 0u && (owner->renderMode == 0u || owner->renderMode == 6u)) ||
       (VidX87EqualOrUnordered(transform.scale.x, 1.0f) && VidX87EqualOrUnordered(transform.scale.y, 1.0f) &&
        VidX87EqualOrUnordered(transform.scale.z, 1.0f))) sampler = D3DTEXF_POINT;
    (void)Graph->device->SetSamplerState(0u, D3DSAMP_MINFILTER, sampler);
    (void)Graph->device->SetSamplerState(0u, D3DSAMP_MAGFILTER, sampler);
    (void)Graph->device->SetRenderState(D3DRS_ALPHATESTENABLE, 1u);
    (void)Graph->device->SetRenderState(D3DRS_ALPHAREF, 0u);
    (void)Graph->device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    (void)Graph->device->SetRenderState(D3DRS_ZFUNC, (owner->renderMode == 7u || owner->renderMode == 4u)?D3DCMP_ALWAYS:D3DCMP_GREATEREQUAL);

    if ((owner->type&2u) == 0u && (owner->renderMode == 0u || owner->renderMode == 6u))
    {
        (void)Graph->device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0u);
        (void)Graph->device->SetRenderState(D3DRS_ZWRITEENABLE, 1u);
        (void)Graph->device->SetRenderState(D3DRS_COLORWRITEENABLE, static_cast<int>(Graph->graphDetail) >= 8?0x0Fu:7u);
    }
    else
    {
        if (owner->renderMode == 1u) (void)Graph->SetAlphaBlend(D3DBLEND_DESTCOLOR, D3DBLEND_SRCCOLOR);
        else if (owner->renderMode == 2u || owner->renderMode == 4u) (void)Graph->SetAlphaBlend(D3DBLEND_ONE, D3DBLEND_ONE);
        else if (owner->renderMode == 5u) (void)Graph->SetAlphaBlend(D3DBLEND_DESTCOLOR, D3DBLEND_ONE);
        else if ((owner->renderMode == 0u || owner->renderMode == 7u || owner->renderMode == 6u) && owner->pixelFormat != D3DFMT_A1R5G5B5 &&
                (owner->type&2u) != 0u)
            (void)Graph->SetAlphaBlend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        else if (owner->renderMode == 0u || owner->renderMode == 7u || owner->renderMode == 6u)
            (void)Graph->device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0u);
        (void)Graph->device->SetRenderState(D3DRS_ZWRITEENABLE, owner->renderMode == 7u?1u:0u);
        (void)Graph->device->SetRenderState(D3DRS_COLORWRITEENABLE, 7u);
    }

    if (Graph->vertexShaderVersion<0x100u && static_cast<unsigned int>(owner->surfaceMode) == 3u)
    {
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_COLORARG2, D3DTA_TFACTOR);
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        (void)Graph->device->SetRenderState(D3DRS_TEXTUREFACTOR, ~finalGamma.modulate);
    }
    else
    {
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }
    if (owner->spriteClass == 8u) static_cast<void>(::NoBuildAlpha = ::NoBuildAlpha+1u);
    if ((Graph->capabilityFlags&0x8000u) != 0u)
    {
        (void)Graph->device->SetRenderState(D3DRS_COLORWRITEENABLE, 7u);
        (void)Graph->device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0u);
        (void)Graph->device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
        (void)Graph->device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_COLORARG1, D3DTA_TFACTOR);
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
        (void)Graph->device->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFFFFFFu);
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        (void)Graph->device->SetTextureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }
}
}

void VID_SURFACE::Draw(SPRITE* sprite)
{
    unsigned int& requestedDrawCount = VID_SURFACE::noDrawCall[static_cast<unsigned int>(layer)];
    requestedDrawCount = requestedDrawCount+1u;
    if (!frames || (runtimeFlags&kVidRuntimeHidden) != 0u) return;

    SPRITE* const request = sprite;
    CADR_DATA* const frame = frames+request->CurrentFrame();
    if (frame->height == 0u) return;

    VidHardwareDrawTransform transform;
    if (request->ExtraData())
        transform.scale = request->ExtraData()->scale;
    else
    {
        transform.scale.x = 1.0f;
        transform.scale.y = 1.0f;
        transform.scale.z = 1.0f;
    }
    transform.scale.x *= scaleXYZ.x;
    transform.scale.y *= scaleXYZ.y;
    transform.scale.z *= scaleXYZ.z;
    transform.position.x = request->Coor().x;
    transform.position.y = request->Coor().y;
    transform.position.z = request->Coor().z;

    if ((property&0x00200000u) != 0u)
    {
        const EX_SPRITE_DATA* source = request->ExtraData();
        const float dx = request->Coor().x-source->cachedPosition.x;
        const float requestIso = request->Coor().y-request->Coor().z;
        const double di = static_cast<double>(requestIso)-(static_cast<double>(source->cachedPosition.y)-source->cachedPosition.z);
        const float ax = dx<0.0f?-dx:dx;
        const double ay = di<0.0?-di:di;
        const double distance = (static_cast<double>(ax)>ay?static_cast<double>(ax)+ay*0.398000002f:ay+static_cast<double>(ax)*0.398000002f);
        const int sourceFrame = static_cast<int>(request->CurrentFrame())-static_cast<int>(request->m_currentFrameBegin);
        const double ratio = distance/static_cast<short>(frames[sourceFrame].height);
        if (ratio>(1.0f))
        {
            transform.scale.y = static_cast<float>(ratio);
            transform.position.x = (request->Coor().x+source->cachedPosition.x)*(0.5f);
            transform.position.y = (request->Coor().y+source->cachedPosition.y)*(0.5f);
            transform.position.z = static_cast<float>((static_cast<double>(request->Coor().z)+source->cachedPosition.z)*0.5f);
        }
    }
    if ((property&kVidApplyPixelCenterOffset) != 0u)
    {
        const float pixelCenterOffset = (1000.0f);
        transform.position.y += pixelCenterOffset;
        transform.position.z += pixelCenterOffset;
    }
    if ((property&kVidApplyDirectionOffset) != 0u)
    {
        const unsigned int i = (::CurrentTime>>3u)&0xFFu;
        transform.position.z += FSin[i]*topZ;
    }
    if ((runtimeFlags&(kVidAnimateScale|kVidAnimateOffset)) != 0u)
    {
        const EX_SPRITE_DATA* source = request->ExtraData();
        const WEAPON_RUNTIME* curves = weapon;
        if ((runtimeFlags&kVidAnimateScale) != 0u)
        {
            const VECTOR v = EvaluateVidHardwareCurve(curves, curves->Scale, source->effectCurvePosition);
            transform.scale.x *= v.x;
            transform.scale.y *= v.y;
            transform.scale.z *= v.z;
        }
        if ((runtimeFlags&kVidAnimateOffset) != 0u)
        {
            const VECTOR v = EvaluateVidHardwareCurve(curves, curves->Shift, source->effectCurvePosition);
            transform.position.x += v.x;
            transform.position.y += v.y;
            transform.position.z += v.z;
        }
    }

    const float extentY = buildSizeY*(0.5f)*transform.scale.y*(1.29999995f);
    const float extentX = buildSizeX*(0.5f)*transform.scale.x*(1.29999995f);
    if (VidHardwareDrawOutsideRuntimeBounds(this, transform)) return;
    const unsigned char direction = SelectVidHardwareDrawDirection(this, request, request->m_speed, request->m_zSpeed);

    if ((property&0x1000u) != 0u && renderMode == 4u)
    {
        const VECTOR2 extent(extentX, extentY), zero(0.0f, 0.0f);
        Graph->DrawShadowToStencil(sprite->Coor(), extent, zero);
    }

    const GAMMA spriteGamma = sprite->GetGamma();
    GAMMA vidGamma;
    vidGamma.modulate = gammaRaw.modulate;
    vidGamma.add = gammaRaw.add;
    GAMMA finalGamma(vidGamma, spriteGamma);
    if (layer >= 7 && (property&0x800u) == 0u)
    {
        GAMMA graphGamma;
        graphGamma.modulate = Graph->gamma.modulate;
        graphGamma.add = Graph->gamma.add;
        finalGamma = GAMMA(graphGamma, finalGamma);
    }

    VidSurfaceDrawSetRenderStates(this, transform, finalGamma);
    lastRenderUseTick = ::RealCurrentTime;
    frame->SetTextureToDevice(this, 0);
    frame->SetVertexToDevice(this);

    unsigned int result = 0u;
    if (frame->indexCount != 0u)
        result = VidSurfaceSubmitDrawMesh(this, frame, transform, direction, finalGamma);
    else if (renderMode == 4u && static_cast<int>(Graph->graphDetail) >= 8)
        result = VidSurfaceSubmitDrawDualQuad(this, request, frame, transform, direction, ~finalGamma.modulate, finalGamma.add);
    else
        result = VidSurfaceSubmitDrawQuad(this, request, frame, transform, direction, ~finalGamma.modulate, finalGamma.add);

    if (static_cast<int>(result)<0)
    {
        const STRING text("Draw");
        Error(E_ERROR, text, result);
    }
    if ((property&0x1000u) != 0u && renderMode == 4u)
        (void)Graph->device->SetRenderState(D3DRS_STENCILENABLE, 0u);
    static_cast<void>(VID_SURFACE::noObj = VID_SURFACE::noObj+1u);
    unsigned int& completedDrawCount = VID_SURFACE::noDrawReal[static_cast<unsigned int>(layer)];
    completedDrawCount = completedDrawCount+1u;
}


namespace
{
    struct VidGridVertex
    {
        float x;
        float y;
        float z;
    };

    static int VidSurfacePixelBytes(unsigned int format)
    {

        switch (format)
        {
        case D3DFMT_R8G8B8:
                return 3;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
                return 4;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
                return 2;
        case D3DFMT_P8:
                return 1;
        case D3DFMT_DXT1:
                return 0;
        case D3DFMT_DXT3:
        case D3DFMT_DXT5:
                return 1;
        default:
                return 0;
        }
    }

    static int VidSurfaceRoundedTextureExtent(int value)
    {
        int rounded = 4;
        while (value>rounded) rounded += rounded;
        return rounded;
    }

    static VidGridVertex VidDecodeGridVertex(unsigned int packed)
    {
        VidGridVertex decodedVertex;
        decodedVertex.x = static_cast<float>(packed&0x3FFu);
        decodedVertex.y = static_cast<float>((packed>>10)&0x3FFu);
        const unsigned int z = (packed>>20)&0x3FFu;
        const unsigned int divisor = 1u<<(packed>>30);
        decodedVertex.z = static_cast<float>(z)/static_cast<float>(divisor)-32.0f;
        return decodedVertex;
    }

    static void VidSwapGridVertex(VidGridVertex& a, VidGridVertex& b)
    {
        const VidGridVertex t = a;
        a = b;
        b = t;
    }

    static void VidGridUpdateCell(short* cells, int x, int projectedY, int z)
    {

        short& cell = cells[(projectedY/8)*256+(x/8)];
        if (z>static_cast<int>(cell)) cell = static_cast<short>(z);
    }

    static void VidRasterizeGridTriangle(const CADR_DATA& frame,
                                         const unsigned char* pixelMask,
                                         int textureWidth,
                                         short* cells,
                                         VidGridVertex a,
                                         VidGridVertex b,
                                         VidGridVertex c)
    {

        if (a.y>b.y) VidSwapGridVertex(a, b);
        if (a.y>c.y) VidSwapGridVertex(a, c);
        if (b.y>c.y) VidSwapGridVertex(b, c);

        const int firstY = static_cast<int>(ceil(static_cast<double>(a.y)));
        const int floorLastY = static_cast<int>(floor(static_cast<double>(c.y)));
        if (floorLastY <= firstY) return;

        const float dyAC = c.y-a.y;
        const double splitRatio = static_cast<double>(b.y-a.y)/static_cast<double>(dyAC);
        const double splitX = static_cast<double>(c.x-a.x)*splitRatio+static_cast<double>(a.x);
        const double splitZ = static_cast<double>(c.z-a.z)*splitRatio+static_cast<double>(a.z);
        const float dzdx = static_cast<float>((splitZ-static_cast<double>(b.z))/(splitX-static_cast<double>(b.x)));
        const float longDx = (c.x-a.x)/dyAC;
        const float longDz = (c.z-a.z)/dyAC;

        const int middleY = static_cast<int>(ceil(static_cast<double>(b.y)));
        float shortDx;
        float shortDz;
        float shortX;
        float shortZ;
        if (middleY>firstY)
        {
            const float dy = b.y-a.y;
            shortDx = (b.x-a.x)/dy;
            shortDz = (b.z-a.z)/dy;
            shortX = a.x;
            shortZ = a.z;
        }
        else
        {
            const float dy = c.y-b.y;
            shortDx = (c.x-b.x)/dy;
            shortDz = (c.z-b.z)/dy;
            shortX = b.x;
            shortZ = b.z;
        }

        float longX = a.x;
        float longZ = a.z;
        int y = firstY;
        if (static_cast<int>(ceil(static_cast<double>(c.y))) <= y) return;

        const int frameX = static_cast<int>(static_cast<short>(frame.offsetX));
        const int frameY = static_cast<int>(static_cast<short>(frame.offsetY));

        while (y<static_cast<int>(ceil(static_cast<double>(c.y))))
        {

            if (y == middleY)
            {
                const float dy = c.y-b.y;
                shortDx = (c.x-b.x)/dy;
                shortDz = (c.z-b.z)/dy;
                shortX = b.x;
                shortZ = b.z;
            }

            int xBegin;
            int xEnd;
            double z;
            if (longX>shortX)
            {
                xBegin = static_cast<int>(ceil(static_cast<double>(shortX)));
                xEnd = static_cast<int>(ceil(static_cast<double>(longX)));
                z = shortZ;
            }
            else
            {
                xBegin = static_cast<int>(ceil(static_cast<double>(longX)));
                xEnd = static_cast<int>(ceil(static_cast<double>(shortX)));
                z = longZ;
            }

            for (int x = xBegin;x<xEnd;++x, z += dzdx)
            {

                const int iz = TruncateDoubleToInt32(z);
                const int projectedY = y+iz;
                if (projectedY<0 || projectedY >= 2048 || x<0 || x >= 2048) continue;

                const int localX = x-frameX;
                if (localX<0 || localX >= 1024) continue;

                const int localY = y-frameY;
                if (localY<0 || localY >= 1024) continue;
                if (pixelMask[localY*textureWidth+localX] == 0u) continue;

                VidGridUpdateCell(cells, x, projectedY, iz);
            }

            longX += longDx;
            longZ += longDz;
            shortX += shortDx;
            shortZ += shortDz;
            ++y;
        }
    }
}


void VID_SURFACE::SetLayer()
{

    if (spriteClass == 8u) runtimeFlags &= ~0x80u;

    if (spriteClass == 8u)
    {
        if ((type&2u) == 0u && renderMode == 0u) layer = 1;
        else
        {
            layer = 2;
            runtimeFlags &= ~0x80u;
        }
    }
    else if (spriteClass == 23u) layer = 5;
    else if (spriteClass == 10u || spriteClass == 19u) layer = 9;
    else if (renderMode == 4u || renderMode == 5u)
    {
        type = static_cast<unsigned short>(type|0x80u);
        layer = 7;
    }
    else if ((type&2u) == 0u && (renderMode == 0u || renderMode == 6u)) layer = 3;
    else if ((property&0x40000000u) == 0u && renderMode != 7u) layer = 8;
    else layer = 4;

    if ((property&0x20u) == 0u || !frames) return;

    if ((storageFlags&8u) != 0u)
    {
        unsigned char* const frameData = surfaceData+frames[0].frameDataOffset;
        noGridZ = *reinterpret_cast<int*>(frameData-sizeof(int));
        if (noGridZ != 0)
        {
            const int frameCount = static_cast<int>(noCadr);
            gridCadrShift = reinterpret_cast<int*>(
                frameData-(static_cast<unsigned int>(frameCount)*sizeof(int)+sizeof(int)));
            const unsigned int vectorDwords = static_cast<unsigned int>(frameCount)+
                3u*static_cast<unsigned int>(noGridZ)+1u;
            gridZ = reinterpret_cast<VECTOR*>(
                frameData-vectorDwords*sizeof(unsigned int));
        }
    }
    if (gridZ) return;

    (void)::Error->Log("not fast for %s %i %i", vidName.m_value, (storageFlags>>3)&1u, static_cast<unsigned int>(noGridZ));

    maxYGridZ = 0.0f;
    minYGridZ = 0.0f;
    maxXGridZ = 0.0f;
    minXGridZ = 0.0f;

    VECTOR* const scratch = static_cast<VECTOR*>(CrtNew(50331648u));
    unsigned char* const pixelMask = static_cast<unsigned char*>(CrtNew(1048576u));
    gridCadrShift = static_cast<int*>(CrtNew(static_cast<unsigned int>(static_cast<int>(noCadr))*sizeof(int)));
    noGridZ = 0;

    const int bytesPerPixel = VidSurfacePixelBytes(pixelFormat);
    short depthGrid[256*256];

    for (int frameIndex = 0;frameIndex<static_cast<int>(noCadr);++frameIndex)
    {
        CADR_DATA& frame = frames[frameIndex];
        int textureWidth = static_cast<int>(static_cast<short>(frame.width));
        if ((type&0x10u) != 0u)
        {
            textureWidth = VidSurfaceRoundedTextureExtent(textureWidth);

            (void)VidSurfaceRoundedTextureExtent(static_cast<int>(static_cast<short>(frame.height)));
        }

        for (int i = 0;i<256*256;++i) depthGrid[i] = kEmptySoftwareGridCell;
        memset(pixelMask, 0, 1048576u);
        gridCadrShift[frameIndex] = noGridZ;

        if (frame.height == 0u) continue;

        const bool compressed = pixelFormat == D3DFMT_DXT1 ||
                              pixelFormat == D3DFMT_DXT3 ||
                              pixelFormat == D3DFMT_DXT5;
        if (!compressed)
        {
            unsigned char* stream = surfaceData+frame.frameDataOffset+
                static_cast<unsigned int>(frame.indexCount)*2u+
                static_cast<unsigned int>(frame.vertexCount)*4u;

            if (*reinterpret_cast<unsigned short*>(stream) != 0u)
            {
                unsigned char* maskCursor = pixelMask;
                while (*reinterpret_cast<unsigned short*>(stream) != 0u)
                {
                    maskCursor += *stream++;
                    int runCount = *stream++;
                    while (runCount-- > 0)
                    {
                        *maskCursor = 1u;
                        ++maskCursor;

                        stream += bytesPerPixel;
                        if ((type&0x2000u) != 0u) ++stream;

                        const int maskOffset = static_cast<int>(maskCursor-pixelMask);
                        int localX = maskOffset%textureWidth;
                        int localY = maskOffset/textureWidth;
                        const int x = localX+static_cast<int>(static_cast<short>(frame.offsetX));
                        const int y = localY+static_cast<int>(static_cast<short>(frame.offsetY));

                        if ((type&4u) != 0u)
                        {
                            const int rawDepth = static_cast<int>(*reinterpret_cast<unsigned short*>(stream));
                            stream += 2;
                            const int z = (rawDepth-1024)/8;
                            if (nVid == 632 && surfaceMode == 3)
                            {
                                const int projectedY = y+z;
                                if (projectedY >= 0 && projectedY<2048 && x >= 0 && x<2048)
                                    VidGridUpdateCell(depthGrid, x, projectedY, z);
                            }
                        }

                        if (frame.vertexCount == 0u)
                            depthGrid[(y/8)*256+(x/8)] = 0;
                    }
                }
            }
        }

        if (frame.vertexCount != 0u &&
           !(nVid == 632 && (type&4u) != 0u && surfaceMode == 3))
        {
            const unsigned short* const indices =
                reinterpret_cast<const unsigned short*>(surfaceData+frame.frameDataOffset);
            const unsigned int* const vertices = reinterpret_cast<const unsigned int*>(
                surfaceData+frame.frameDataOffset+static_cast<unsigned int>(frame.indexCount)*2u);

            for (unsigned int i = 0;i<static_cast<unsigned int>(frame.indexCount);i += 3u)
            {
                VidGridVertex a = VidDecodeGridVertex(vertices[indices[i+0u]]);
                VidGridVertex b = VidDecodeGridVertex(vertices[indices[i+1u]]);
                VidGridVertex c = VidDecodeGridVertex(vertices[indices[i+2u]]);
                VidRasterizeGridTriangle(frame, pixelMask, textureWidth, depthGrid, a, b, c);
            }
        }

        const int rows = static_cast<int>(vidSizeY)/8;
        for (int row = rows-1;row >= 0;--row)
        {
            for (int column = 0;column<static_cast<int>(vidSizeX)/8;++column)
            {
                const short z = depthGrid[row*256+column];
                if (z == kEmptySoftwareGridCell) continue;

                scratch[noGridZ].x = static_cast<float>(column*8-static_cast<int>(vidSizeX)/2);
                scratch[noGridZ].y = static_cast<float>(row*8-static_cast<int>(vidSizeY)/2);
                scratch[noGridZ].z = static_cast<float>(z);
                noGridZ = BitsToSigned32(static_cast<unsigned int>(noGridZ)+1u);
            }
        }
    }

    gridZ = static_cast<VECTOR*>(CrtNew(static_cast<unsigned int>(noGridZ)*sizeof(VECTOR)));
    for (int i = 0;i<noGridZ;++i)
    {
        gridZ[i] = scratch[i];
        if (!(gridZ[i].x >= minXGridZ)) minXGridZ = gridZ[i].x;
        if (gridZ[i].x>maxXGridZ) maxXGridZ = gridZ[i].x;
        if (!(gridZ[i].y >= minYGridZ)) minYGridZ = gridZ[i].y;
        if (gridZ[i].y>maxYGridZ) maxYGridZ = gridZ[i].y;
    }

    CrtFreePointer(scratch);
    CrtFreePointer(pixelMask);
}


void VID_SURFACE::CopyGraphFromVid(VID_SURFACE* source)
{
    ReleaseVideoMemory();

    if (nextMirror == this)
    {
        if ((storageFlags&8u) == 0u && cadrLinkXYZ)
        {
            CrtFreePointer(cadrLinkXYZ);
            cadrLinkXYZ = 0;
        }
        cadrLinkXYZ = 0;

        VidReleaseFrameArray(*this);
        surfaceData = 0;
    }
    else
    {
        VID* predecessor = nextMirror;
        while (predecessor->nextMirror != this) predecessor = predecessor->nextMirror;
        predecessor->nextMirror = nextMirror;
    }

    nextMirror = source->nextMirror;
    source->nextMirror = this;

    layer = source->layer;
    type = source->type;
    noCadr = source->noCadr;
    frameSpeedDefault = source->frameSpeedDefault;
    vidSizeX = source->vidSizeX;
    vidSizeY = source->vidSizeY;
    buildSizeX = source->buildSizeX;
    buildSizeY = source->buildSizeY;
    vidName = source->vidName;

    surfaceData = source->surfaceData;
    cadrLinkXYZ = source->cadrLinkXYZ;
    frames = source->frames;
    palette = source->palette;
    pixelFormat = source->pixelFormat;
    surfaceMode = source->surfaceMode;
    renderMode = source->renderMode;
    lastRenderUseTick = source->lastRenderUseTick;

    surfaceFlags = (surfaceFlags&~1u)|(source->surfaceFlags&1u);
    surfaceFlags = (surfaceFlags&~2u)|(source->surfaceFlags&2u);
    storageFlags = ((storageFlags&~8u)|(source->storageFlags&8u))&~4u;

    if (source->recolorPalette)
    {
        if (!recolorPalette)
            recolorPalette = static_cast<unsigned int*>(CrtNew(1024u));
        for (unsigned int i = 0;i<256u;++i) recolorPalette[i] = source->recolorPalette[i];
    }
    else if (recolorPalette)
    {
        CrtFreePointer(recolorPalette);
        recolorPalette = 0;
    }

    if ((storageFlags&8u) == 0u || (property&8u) != 0u)
    {
        if (gridZ)
        {
            CrtFreePointer(gridZ);
            gridZ = 0;
        }
        if (gridCadrShift)
        {
            CrtFreePointer(gridCadrShift);
            gridCadrShift = 0;
        }
    }

    noGridZ = source->noGridZ;
    gridZ = source->gridZ;
    gridCadrShift = source->gridCadrShift;
    minXGridZ = source->minXGridZ;
    maxXGridZ = source->maxXGridZ;
    minYGridZ = source->minYGridZ;
    maxYGridZ = source->maxYGridZ;
}


void VID_SURFACE::CopyFromVid(VID_SURFACE* source)
{
    sourceVid = source;
    CopyGraphFromVid(source);

    spriteType = source->spriteType;
    spriteClass = source->spriteClass;
    property = source->property;
    moveMask = source->moveMask;
    sizeXYZ = source->sizeXYZ;
    maxHp = source->maxHp;
    maxSpeed = source->maxSpeed;
    randomSpeed = source->randomSpeed;
    maxZSpeed = source->maxZSpeed;
    randomZSpeed = source->randomZSpeed;
    acceleration = source->acceleration;
    slow = source->slow;
    rotationSpeed = source->rotationSpeed;
    deathRange = source->deathRange;
    deathDamageMin = source->deathDamageMin;
    deathDamageMax = source->deathDamageMax;
    deathPush = source->deathPush;
    linkXYZ = source->linkXYZ;
    nLinkVid = source->nLinkVid;
    linkVid = source->linkVid;
    topZ = source->topZ;
    forMoveUpZ = source->forMoveUpZ;
    forMoveDownZ = source->forMoveDownZ;
    lifeTime = source->lifeTime;
    noDir = source->noDir;
    gammaRaw = source->gammaRaw;
    scaleXYZ = source->scaleXYZ;
    halfSizeXY = source->halfSizeXY;
    realSizeXYZ = source->realSizeXYZ;

    for (int i = 0;i<AnimationSlotCount;++i)
    {
        noAnimCadr[i] = source->noAnimCadr[i];
        sfx[i] = source->sfx[i];
        frameSpeed[i] = source->frameSpeed[i];
        childX[i] = source->childX[i];
        childY[i] = source->childY[i];
        childZ[i] = source->childZ[i];
        nChildVid[i] = source->nChildVid[i];
        childVid[i] = source->childVid[i];
        noChild[i] = source->noChild[i];
        animationBaseFrame[i] = source->animationBaseFrame[i];
        animationFrameCount[i] = source->animationFrameCount[i];
    }

    for (int i = 0;i<PlayerSlotCount;++i)
    {
        maxHpByPlayer[i] = source->maxHpByPlayer[i];
        gammaByPlayer[i] = source->gammaByPlayer[i];
    }

    actionAuxStateRequired = source->actionAuxStateRequired;
    movementTactEnabled = source->movementTactEnabled;
    runtimeFlags = (runtimeFlags&~0x80u)|(source->runtimeFlags&0x80u);
    runtimeFlags = (runtimeFlags&~0x100u)|(source->runtimeFlags&0x100u);
    runtimeFlags = (runtimeFlags&~0x1000u)|(source->runtimeFlags&0x1000u);
    runtimeFlags = (runtimeFlags&~0x200u)|(source->runtimeFlags&0x200u);
}

VID* VID::CreateMirror()
{
    void* const memory = CrtNew(sizeof(VID));
    return memory ? new(memory) VID() : 0;
}


void VID::SetLayer()
{
    layer = 0;
}


template<>
void LIST<int>::Expand(int requestedCapacity)
{
    if (requestedCapacity <= m_capacity) return;
    int* const oldData = m_data;
    int* const newData = reinterpret_cast<int*>(
        CrtNew(static_cast<unsigned int>(requestedCapacity)*sizeof(int)));
    m_data = newData;
    if (!newData)
    {
        STRING format("!!!ERROR!!!::LIST: Not enough memory %i");
        ::Error->LogExit(format, requestedCapacity);
    }
    if (oldData)
    {
        for (int index = 0;index<m_capacity;++index) m_data[index] = oldData[index];
        CrtFreePointer(oldData);
    }
    m_capacity = requestedCapacity;
}

VID* VID_SOFTWARE::CreateMirror()
{
    void* const memory = CrtNew(sizeof(VID_SOFTWARE));
    return memory ? new(memory) VID_SOFTWARE(*this) : 0;
}

VID* VID_FONT::CreateMirror()
{
    void* const memory = CrtNew(sizeof(VID_FONT));
    return memory ? new(memory) VID_FONT(*this) : 0;
}


VID* VID_SURFACE::CreateMirror()
{
    void* const memory = CrtNew(sizeof(VID_SURFACE));
    if (!memory) return 0;
    VID_SURFACE* const object = new(memory) VID_SURFACE();
    object->CopyGraphFromVid(this);
    return object;
}

void VID_SURFACE::RestoreDeviceObjects()
{
}



