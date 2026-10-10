#include "mylib.h"
#include <new>
#if defined(_WIN32)
#define INPUT WIN32_INPUT
#include <windows.h>
#include <d3d9.h>
#undef INPUT
#endif

#include "myerror.h"
#include "texture.h"
#include "basetype.h"
#include "graph.h"
#include "map.h"
#include "vid.h"
#include "sprite_list.h"


namespace {
IDirect3DSurface9* stagingSurface = 0;
IDirect3DTexture9* stagingTexture = 0;
RECT stagingRect = {};
}

int TEXTURE::memoryInUse = 0;
int TEXTURE::capsSquare = 0;
int TEXTURE::capsDynamic = 0;
int TEXTURE::capsPalette = 0;
int TEXTURE::capsDXT = 0;
int TEXTURE::capsAlphaPalette = 0;
int TEXTURE::capsCondSizePow2 = 0;
int TEXTURE::capsSizePower2 = 1;
int TEXTURE::capsMaxSizeX = 256;
int TEXTURE::capsMaxSizeY = 256;

void TEXTURE::SetDeviceCaps(IDirect3DDevice9* device)
{
    D3DCAPS9 caps;
    IDirect3DDevice9* const d3dDevice = device;
    const HRESULT capsHr = d3dDevice->GetDeviceCaps(&caps);
    if (capsHr<0)
    {
        STRING subject("Caps");
        if (::Error)
        {
            STRING objectFormat("TEXTURE");
            ::Error->Error(objectFormat, E_GET, subject, 0UL);
        }
    }

    const unsigned int caps2 = caps.Caps2;
    const unsigned int textureCaps = caps.TextureCaps;
    capsMaxSizeX = caps.MaxTextureWidth;
    capsMaxSizeY = caps.MaxTextureHeight;
    capsDynamic = caps2&D3DCAPS2_DYNAMICTEXTURES;
    capsSquare = textureCaps&D3DPTEXTURECAPS_SQUAREONLY;
    capsSizePower2 = textureCaps&D3DPTEXTURECAPS_POW2;
    capsAlphaPalette = textureCaps&D3DPTEXTURECAPS_ALPHAPALETTE;
    capsCondSizePow2 = textureCaps&D3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    if (capsSizePower2 == 0u)
        capsCondSizePow2 = 1u;

    IDirect3DSurface9* backBuffer;
    if (d3dDevice->GetBackBuffer(0u, 0u, D3DBACKBUFFER_TYPE_MONO, &backBuffer) == 0u)
    {
        D3DSURFACE_DESC desc;
        (void) backBuffer->GetDesc(&desc);

        IDirect3D9* d3dObject;
        (void) d3dDevice->GetDirect3D(&d3dObject);
        if (d3dObject->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format,
            0u, D3DRTYPE_TEXTURE, D3DFMT_P8) == 0u)
        capsPalette = 1u;
        else
        capsAlphaPalette = 0u;
        if (d3dObject->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format,
            0u, D3DRTYPE_TEXTURE, D3DFMT_DXT1) == 0u)
        capsDXT = capsDXT | 1u;
        if (d3dObject->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format,
            0u, D3DRTYPE_TEXTURE, D3DFMT_DXT3) == 0u)
        capsDXT = capsDXT | 4u;
        if (d3dObject->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format,
            0u, D3DRTYPE_TEXTURE, D3DFMT_DXT5) == 0u)
        capsDXT = capsDXT | 0x10u;
        (void) backBuffer->Release();
        (void) d3dObject->Release();
    }

    capsPalette = 0u;
    capsAlphaPalette = 0u;

    STRING capability;
    if (capsSquare != 0u) capability += "SQUARE ";
    capability += (capsSizePower2 != 0u)
    ? "POWER2 "
    : "NOTPOWER2 ";
    if (capsCondSizePow2 != 0u) capability += "COND_NON_POW2 ";
    if (capsDynamic != 0u) capability += "DYNAMIC ";
    if (capsPalette != 0u) capability += "PALETTE ";
    if (capsAlphaPalette != 0u) capability += "ALPHA_PALETTE ";
    if ((capsDXT&1u) != 0u) capability += "DXT1 ";
    if ((capsDXT&4u) != 0u) capability += "DXT3 ";
    if ((capsDXT&0x10u) != 0u) capability += "DXT5 ";

    const unsigned int textureMemory = Graph->device->GetAvailableTextureMem();
    {
        STRING limits = Printf("MAXSIZE=%i,%i MAXTEXTUREMEMORY = %i", capsMaxSizeX, capsMaxSizeY, textureMemory);
        capability += limits;
    }
    ::Error->Log("TextureCaps=%s", capability.m_value);
    capsSizePower2 = 1u;
}


TEXTURE::TEXTURE(int width, int height, D3DFORMAT format, unsigned long flags)
{
    Create(width, height, format, flags);
}

static unsigned int TextureByteCount(D3DFORMAT format, unsigned int width, unsigned int height)
{
    const unsigned int pixels = width*height;
    if (format == D3DFMT_DXT1) return static_cast<unsigned int>(BitsToSigned32(pixels)/2);
    if (format == D3DFMT_P8 || format == D3DFMT_L8) return pixels;
    if (format == D3DFMT_A8R8G8B8) return pixels*4u;
    if (format == D3DFMT_R8G8B8) return pixels*3u;
    return pixels*2u;
}

static bool TextureReleaseVideoMemoryForRetry()
{
    CADR_DATA* root = VID::lastUse;
    if (root == 0) return false;

    const unsigned int initialBytes = VID::useTextureMemory;
    while (root != 0)
    {
        const unsigned int currentBytes = VID::useTextureMemory;
        if (initialBytes-currentBytes >= 1000000u) break;
        CADR_DATA* const oldest = root->lruPrevious;
        oldest->ReleaseVideoMemory();
        root = VID::lastUse;
    }
    return true;
}

static unsigned int TextureNextPow2(unsigned int requestedSize)
{
    unsigned int powerOfTwo = 1u;
    while (static_cast<int>(requestedSize)>static_cast<int>(powerOfTwo)) powerOfTwo += powerOfTwo;
    return powerOfTwo;
}

static unsigned int TextureNextPow2AtLeast4(unsigned int value);


void TEXTURE::Create(int width, int height, D3DFORMAT format, unsigned long flags)
{
    TEXTURE* const owner = this;
    owner->m_texture = 0;
    owner->m_surface = 0;
    owner->m_cpuData = 0;

    int effectiveWidth = width;
    int effectiveHeight = height;
    D3DFORMAT effectiveFormat = format;
    unsigned int effectiveFlags = static_cast<unsigned int>(flags);

    if (capsDynamic == 0u && (effectiveFlags&7u) == 1u)
        effectiveFlags &= ~1u;

    if (capsSquare != 0u)
    {
        if (effectiveWidth>effectiveHeight) effectiveHeight = effectiveWidth;
        else if (effectiveWidth<effectiveHeight) effectiveWidth = effectiveHeight;
    }

    if (capsCondSizePow2 == 0u)
    {
        effectiveWidth = BitsToSigned32(TextureNextPow2(static_cast<unsigned int>(effectiveWidth)));
        effectiveHeight = BitsToSigned32(TextureNextPow2(static_cast<unsigned int>(effectiveHeight)));
    }
    if (effectiveWidth <= 4) effectiveWidth = 4;
    if (effectiveHeight <= 4) effectiveHeight = 4;

    if (effectiveWidth>static_cast<int>(capsMaxSizeX) || effectiveWidth <= 0)
    {
        STRING subject("initial sizeX");
        if (Error)
        {
            STRING object("TEXTURE");
            Error->Error(object, E_INVALID, subject, static_cast<unsigned int>(effectiveWidth));
        }
        return;
    }
    if (effectiveHeight>static_cast<int>(capsMaxSizeY) || effectiveHeight <= 0)
    {
        STRING subject("initial sizeY");
        if (Error)
        {
            STRING object("TEXTURE");
            Error->Error(object, E_INVALID, subject, static_cast<unsigned int>(effectiveHeight));
        }
        return;
    }

    owner->m_flags = effectiveFlags;
    if (effectiveFormat == D3DFMT_A8R8G8B8 || effectiveFormat == D3DFMT_A4R4G4B4 || effectiveFormat == D3DFMT_A1R5G5B5 || effectiveFormat == D3DFMT_DXT3 || effectiveFormat == D3DFMT_DXT5)
    {
        owner->m_flags = effectiveFlags | 8u;
    }

    if (effectiveFormat == D3DFMT_P8 && (owner->m_flags&8u) != 0u && capsAlphaPalette == 0u)
        effectiveFormat = D3DFMT_A8R8G8B8;

    if ((effectiveFlags&0x10u) != 0u)
    {
        if (effectiveFormat == D3DFMT_P8)
        {
            if ((owner->m_flags&8u) == 0u) effectiveFormat = D3DFMT_A1R5G5B5;
            else if (capsPalette == 0u) effectiveFormat = D3DFMT_R5G6B5;
        }
        else if (effectiveFormat == D3DFMT_R5G6B5) effectiveFormat = D3DFMT_A1R5G5B5;
        else if (effectiveFormat == D3DFMT_R8G8B8) effectiveFormat = D3DFMT_A8R8G8B8;
    }
    else if (effectiveFormat == D3DFMT_P8 && capsPalette == 0u)
    {
        effectiveFormat = D3DFMT_R5G6B5;
    }

    HRESULT hr = 0;
    if ((effectiveFlags&4u) != 0u && effectiveFormat == D3DFMT_D16)
    {
        const unsigned int bytes = static_cast<unsigned int>(effectiveWidth)*static_cast<unsigned int>(effectiveHeight)*2u;
        owner->m_cpuData = static_cast<unsigned char*>(CrtNew(bytes));
    }
    else
    {
        for (; ;)
        {
            IDirect3DDevice9* const device = Graph->device;
            const unsigned int mode = effectiveFlags&7u;
            unsigned int usage = 0u;
            if (mode == 2u) usage = D3DUSAGE_RENDERTARGET;
            if (mode == 1u) usage |= D3DUSAGE_DYNAMIC;
            const D3DPOOL pool = (mode == 0u)?D3DPOOL_MANAGED:
            ((mode == 4u)?D3DPOOL_SYSTEMMEM:D3DPOOL_DEFAULT);
            hr = device->CreateTexture(static_cast<unsigned int>(effectiveWidth), static_cast<unsigned int>(effectiveHeight),
                1u, usage, effectiveFormat, pool, &owner->m_texture, 0);
            if (hr == 0u) break;

            if (hr == D3DERR_OUTOFVIDEOMEMORY)
            {
                if (!TextureReleaseVideoMemoryForRetry()) break;
                continue;
            }

            if (effectiveFormat == D3DFMT_P8)
            {
                effectiveFormat = ((owner->m_flags&8u) != 0u)?D3DFMT_A8R8G8B8:D3DFMT_R5G6B5;
                continue;
            }
            if (effectiveFormat == D3DFMT_L8)
            {
                effectiveFormat = D3DFMT_A8R8G8B8;
                continue;
            }
            if (effectiveFormat == D3DFMT_DXT1)
            {
                effectiveFormat = D3DFMT_R5G6B5;
                continue;
            }
            if (effectiveFormat == D3DFMT_DXT3)
            {
                effectiveFormat = D3DFMT_A4R4G4B4;
                continue;
            }
            if (effectiveFormat == D3DFMT_R5G6B5)
            {
                effectiveFormat = D3DFMT_X1R5G5B5;
                continue;
            }
            if (effectiveFormat == D3DFMT_X1R5G5B5)
            {
                effectiveFormat = D3DFMT_A1R5G5B5;
                continue;
            }
            if (effectiveFormat == D3DFMT_R8G8B8)
            {
                effectiveFormat = D3DFMT_X8R8G8B8;
                continue;
            }
            if (effectiveFormat == D3DFMT_X8R8G8B8)
            {
                effectiveFormat = D3DFMT_A8R8G8B8;
                continue;
            }

            if (capsCondSizePow2 != 0u)
            {
                const int powerOfTwoWidth = static_cast<int>(TextureNextPow2(static_cast<unsigned int>(effectiveWidth)));
                const int powerOfTwoHeight = static_cast<int>(TextureNextPow2(static_cast<unsigned int>(effectiveHeight)));
                if (powerOfTwoWidth != effectiveWidth || powerOfTwoHeight != effectiveHeight)
                {
                    effectiveWidth = powerOfTwoWidth;
                    effectiveHeight = powerOfTwoHeight;
                    continue;
                }
            }
            break;
        }
    }

    owner->m_width = static_cast<unsigned int>(effectiveWidth);
    owner->m_height = static_cast<unsigned int>(effectiveHeight);
    owner->m_format = effectiveFormat;

    if (hr != 0u || owner->m_texture == 0) return;
    const unsigned int bytes = TextureByteCount(effectiveFormat, static_cast<unsigned int>(effectiveWidth), static_cast<unsigned int>(effectiveHeight));
    memoryInUse = memoryInUse+bytes;
}

IDirect3DSurface9* TEXTURE::GetSurface()
{
    TEXTURE* const owner = this;
    if (owner->m_surface == 0)
    {
        IDirect3DTexture9* const texture = owner->m_texture;
        (void) texture->GetSurfaceLevel(0u, &owner->m_surface);
    }
    return owner->m_surface;
}


unsigned char* TEXTURE::Lock(int* pitch, RECT* rect) const
{
    const TEXTURE* const owner = this;
    const unsigned int mode = owner->m_flags&7u;
    RECT* const lockRect = rect;

    if (mode == 4u && owner->m_format == D3DFMT_D16)
    {
        *pitch = BitsToSigned32(owner->m_width*2u);
        if (lockRect)
        {
            const unsigned int top = static_cast<unsigned int>(lockRect->top);
            const unsigned int left = static_cast<unsigned int>(lockRect->left);
            const unsigned int offset = (top*owner->m_width+left)*2u;
            return owner->m_cpuData+offset;
        }
        return owner->m_cpuData;
    }

    D3DLOCKED_RECT locked;
    HRESULT hr;
    if (mode != 2u && mode != 3u)
    {
        IDirect3DTexture9* const texture = owner->m_texture;
        hr = (texture->LockRect(
            0u, &locked, rect, 0u));
    }
    else
    {
        const unsigned int width = owner->m_width;
        const unsigned int height = owner->m_height;
        unsigned int lockWidth = width, lockHeight = height;
        if (lockRect)
        {
            if (lockRect->left == 999999)
            {
                lockWidth = TextureNextPow2AtLeast4(width);
                lockHeight = TextureNextPow2AtLeast4(height);
                lockRect->left = 0;
            }
            else
            {
                lockWidth = static_cast<unsigned int>(lockRect->right);
                lockHeight = static_cast<unsigned int>(lockRect->bottom);
            }
        }

        hr = (Graph->device->CreateOffscreenPlainSurface(
            lockWidth, lockHeight, owner->m_format, D3DPOOL_SYSTEMMEM,
            &stagingSurface, 0));
        if (hr<0)
        {
            hr = (Graph->device->CreateTexture(
                lockWidth, lockHeight, 1u, 0u, owner->m_format, D3DPOOL_SYSTEMMEM,
                &stagingTexture, 0));
            if (hr<0)
            {
                STRING subject("surface in system memory for lock surface in video memory");
                if (Error)
                {
                    STRING object("TEXTURE");
                    Error->Error(object, E_CREATE, subject, hr);
                }
                return 0;
            }
            IDirect3DTexture9* const texture = stagingTexture;
            (void) texture->GetSurfaceLevel(0u,
                &stagingSurface);
        }

        if (lockRect)
        {
            stagingRect.left = lockRect->left;
            stagingRect.top = lockRect->top;
            stagingRect.right = lockRect->right;
            stagingRect.bottom = lockRect->bottom;
        }
        else
        {
            stagingRect.left = 0;
            stagingRect.top = 0;
            stagingRect.right = BitsToSigned32(owner->m_width);
            stagingRect.bottom = BitsToSigned32(owner->m_height);
        }
        IDirect3DSurface9* const surface = stagingSurface;
        hr = (surface->LockRect(
            &locked,
            &stagingRect, D3DLOCK_NO_DIRTY_UPDATE));
    }

    if (hr<0)
    {
        STRING subject(StringGlobals::EmptyText());
        if (Error)
        {
            STRING object("TEXTURE");
            Error->Error(object, E_LOCK, subject, hr);
        }
        return 0;
    }
    *pitch = locked.Pitch;
    return static_cast<unsigned char*>(locked.pBits);
}


void TEXTURE::UnLock()
{
    TEXTURE* const owner = this;
    if (owner->m_texture == 0) return;
    const unsigned int mode = owner->m_flags&7u;
    if ((mode == 2u || mode == 3u) && stagingSurface != 0)
    {
        (void) stagingSurface->UnlockRect();
        POINT point = {0, 0};
        IDirect3DDevice9* const device = Graph->device;
        IDirect3DSurface9* const dest = GetSurface();
        const HRESULT hr = device->UpdateSurface(stagingSurface, &stagingRect, dest, &point);
        if (hr<0)
        {
            STRING subject("system memory texture to video for unlock");
            if (Error)
            {
                STRING object("TEXTURE");
                Error->Error(object, E_COPY, subject, hr);
            }
        }
        IDirect3DSurface9* const surface = stagingSurface;
        if (surface)
        {
            (void) surface->Release();
            stagingSurface = 0;
        }
        IDirect3DTexture9* const texture = stagingTexture;
        if (texture)
        {
            (void) texture->Release();
            stagingTexture = 0;
        }
        return;
    }
    IDirect3DTexture9* const texture = owner->m_texture;
    (void) texture->UnlockRect(0u);
}


void TEXTURE::SetTextureToDevice(int stage)
{
    const TEXTURE* const owner = this;
    IDirect3DDevice9* const device = Graph->device;
    HRESULT hr = (device->SetTexture(
        static_cast<unsigned int>(stage), owner->m_texture));
    if (hr != 0u)
    {
        STRING subject(StringGlobals::EmptyText());
        if (Error)
        {
            STRING object("TEXTURE");
            Error->Error(object, E_SET, subject, hr);
        }
    }
    if (owner->m_format == D3DFMT_P8)
    {
        hr = (Graph->device->SetCurrentTexturePalette(owner->m_paletteIndex));
        if (hr != 0u)
        {
            STRING subject("palette");
            if (Error)
            {
                STRING object("TEXTURE");
                Error->Error(object, E_SET, subject, hr);
            }
        }
    }
}

struct TextureVertex {
    float x, y, z, rhw;
    unsigned int diffuse, specular;
    float u, v;
};


long TEXTURE::CopyFromSurface(IDirect3DSurface9* sourceSurface, RECT* sourceRect, POINT* destPoint)
{
    const TEXTURE* const owner = this;
    IDirect3DTexture9* const texture = owner->m_texture;
    IDirect3DSurface9* destSurface;
    HRESULT hr = (texture->GetSurfaceLevel(0u, &destSurface));
    if (hr != 0u)
    {
        STRING subject("surface for copy");
        if (Error)
        {
            STRING object("TEXTURE");
            Error->Error(object, E_GET, subject, hr);
        }
        return static_cast<long>(hr);
    }

    RECT localDest;
    RECT* destRect = 0;
    if (destPoint)
    {
        const int x = destPoint->x;
        const int y = destPoint->y;
        localDest.left = x;
        localDest.top = y;
        localDest.right = BitsToSigned32(static_cast<unsigned int>(sourceRect->right)-
            static_cast<unsigned int>(sourceRect->left)+static_cast<unsigned int>(x));
        localDest.bottom = BitsToSigned32(static_cast<unsigned int>(sourceRect->bottom)-
            static_cast<unsigned int>(sourceRect->top)+static_cast<unsigned int>(y));
        destRect = &localDest;
    }

    hr = D3DXLoadSurfaceFromSurface(destSurface, 0, destRect, sourceSurface, 0, sourceRect, 1u, 0u);
    if (hr != 0u)
    {
        STRING subject("from surface");
        if (Error)
        {
            STRING object("TEXTURE");
            Error->Error(object, E_COPY, subject, hr);
        }
    }
    (void) destSurface->Release();
    return static_cast<long>(hr);
}

static void TextureFillVertex(TextureVertex& vertex, float x, float y, float u, float v, unsigned int diffuse, unsigned int specular)
{
    vertex.x = x;
    vertex.y = y;

    vertex.z = 0.99999988079071044921875f;
    vertex.rhw = 1.0f;
    vertex.diffuse = diffuse;
    vertex.specular = specular;
    vertex.u = u;
    vertex.v = v;
}


long TEXTURE::Draw(RECT* destRect, RECT* sourceRect, const GAMMA& gamma)
{
    if (!m_texture && !m_cpuData) return E_FAIL;

    RECT* sampleRect = sourceRect;
    if (!sampleRect)
    {
        static RECT fullTextureRect = {0, 0, BitsToSigned32(m_width), BitsToSigned32(m_height)};
        sampleRect = &fullTextureRect;
    }

    const double width = BitsToSigned32(m_width);
    const double height = BitsToSigned32(m_height);
    const float left = static_cast<float>(static_cast<double>(sampleRect->left)+0.5);
    const float top = static_cast<float>(static_cast<double>(sampleRect->top)+0.5);
    const float bottom = static_cast<float>(static_cast<double>(sampleRect->bottom)+0.5);
    const float u0 = static_cast<float>(left/width);
    const float v0 = static_cast<float>(top/height);
    const float u1 = static_cast<float>((static_cast<double>(sampleRect->right)+0.5)/width);
    const float v1 = static_cast<float>(bottom/height);
    const unsigned int diffuse = ~gamma.modulate;
    const unsigned int specular = gamma.add;

    TextureVertex vertices[4];
    TextureFillVertex(vertices[0], static_cast<float>(destRect->left), static_cast<float>(destRect->top), u0, v0, diffuse, specular);
    TextureFillVertex(vertices[1], static_cast<float>(destRect->right), static_cast<float>(destRect->top), u1, v0, diffuse, specular);
    TextureFillVertex(vertices[2], static_cast<float>(destRect->right), static_cast<float>(destRect->bottom), u1, v1, diffuse, specular);
    TextureFillVertex(vertices[3], static_cast<float>(destRect->left), static_cast<float>(destRect->bottom), u0, v1, diffuse, specular);

    SetTextureToDevice(0);
    (void) Graph->device->SetRenderState(D3DRS_SPECULARENABLE, gamma.add?1u:0u);
    const bool sameSize =
    static_cast<unsigned int>(destRect->right)-static_cast<unsigned int>(destRect->left) ==
    static_cast<unsigned int>(sampleRect->right)-static_cast<unsigned int>(sampleRect->left) &&
    static_cast<unsigned int>(destRect->bottom)-static_cast<unsigned int>(destRect->top) ==
    static_cast<unsigned int>(sampleRect->bottom)-static_cast<unsigned int>(sampleRect->top);
    const unsigned int filter = sameSize?D3DTEXF_POINT:D3DTEXF_LINEAR;
    (void) Graph->device->SetSamplerState(0u, D3DSAMP_MINFILTER, filter);
    (void) Graph->device->SetSamplerState(0u, D3DSAMP_MAGFILTER, filter);
    (void) Graph->device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
    const HRESULT hr = (Graph->device->DrawPrimitiveUP(
        D3DPT_TRIANGLEFAN, 2u, vertices, sizeof(TextureVertex)));
    if (hr != 0u)
    {
        STRING subject("DrawPrimitiveUP");
        if (Error)
        {
            STRING object("TEXTURE");
            Error->Error(object, E_ERROR, subject, hr);
        }
    }
    return static_cast<long>(hr);
}

unsigned int __cdecl BaseTexture_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


unsigned int __cdecl BaseTexture_StaticCleanupRegister02()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


unsigned int __cdecl BaseTexture_StaticCleanupRegister03()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


unsigned int __cdecl BaseTexture_StaticCleanupRegister04()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


unsigned int __cdecl BaseTexture_StaticCleanupRegister05()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned int __cdecl BaseTexture_StaticCleanupGuard01()
{
    const unsigned char guard = CrtStaticInitializationGuards[0];
    if ((guard&1u) == 0u) CrtStaticInitializationGuards[0] = static_cast<unsigned char>(guard | 1u);
    return BaseTexture_StaticCleanupRegister01();
}


unsigned int __cdecl BaseTexture_StaticCleanupGuard02()
{
    const unsigned char guard = CrtStaticInitializationGuards[0];
    if ((guard&1u) == 0u) CrtStaticInitializationGuards[0] = static_cast<unsigned char>(guard | 1u);
    return BaseTexture_StaticCleanupRegister02();
}


unsigned int __cdecl BaseTexture_StaticCleanupGuard03()
{
    const unsigned char guard = CrtStaticInitializationGuards[0];
    if ((guard&1u) == 0u) CrtStaticInitializationGuards[0] = static_cast<unsigned char>(guard | 1u);
    return BaseTexture_StaticCleanupRegister03();
}


unsigned int __cdecl BaseTexture_StaticCleanupGuard04()
{
    const unsigned char guard = CrtStaticInitializationGuards[0];
    if ((guard&1u) == 0u) CrtStaticInitializationGuards[0] = static_cast<unsigned char>(guard | 1u);
    return BaseTexture_StaticCleanupRegister04();
}


unsigned int __cdecl BaseTexture_StaticCleanupGuard05()
{
    const unsigned char guard = CrtStaticInitializationGuards[0];
    if ((guard&1u) == 0u) CrtStaticInitializationGuards[0] = static_cast<unsigned char>(guard | 1u);
    return BaseTexture_StaticCleanupRegister05();
}

TEXTURE::~TEXTURE() noexcept(false)
{
    TEXTURE* const owner = this;

    IDirect3DSurface9* const surface = owner->m_surface;
    if (surface != 0)
    {
        (void) surface->Release();
        owner->m_surface = 0;
    }

    if (owner->m_texture != 0)
    {
        IDirect3DDevice9* const device = Graph->device;
        (void) device->SetTexture(0u, 0);

        const unsigned int remainingReferences = owner->m_texture->Release();
        if (remainingReferences != 0u)
        {
            STRING subject("Release count !=0");
            if (Error)
            {
                STRING object("TEXTURE");
                Error->Error(object, E_ERROR, subject, remainingReferences);
            }
        }
        else
        {
            const D3DFORMAT format = owner->m_format;
            const unsigned int bytes = TextureByteCount(format, owner->m_width, owner->m_height);

            memoryInUse = memoryInUse-bytes;
        }

    }

    if (owner->m_cpuData != 0)
    {
        CrtFreePointer(owner->m_cpuData);

    }
}

static HRESULT CreateTextureFromFile(
    IDirect3DDevice9* device, const char* sourceFile, unsigned int colorKey,
    D3DXImageInfo* imageInfo, IDirect3DTexture9** outTexture)
{
    const unsigned int defaultSize = static_cast<unsigned int>(-1);
    return D3DXCreateTextureFromFileExA(device, sourceFile, defaultSize, defaultSize,
        1u, 0u, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, 1u, 1u, colorKey, imageInfo, 0, outTexture);
}

TEXTURE::TEXTURE(const STRING& sourceFile, unsigned long colorKey, D3DXImageInfo* imageInfo)
{
    TEXTURE* const owner = this;
    owner->m_flags = 0u;
    owner->m_texture = 0;
    owner->m_surface = 0;
    owner->m_cpuData = 0;
    owner->m_height = 4u;
    owner->m_width = 4u;

    HRESULT hr = CreateTextureFromFile(Graph->device,
        sourceFile.m_value, colorKey, imageInfo, &owner->m_texture);
    while (hr == D3DERR_OUTOFVIDEOMEMORY && TextureReleaseVideoMemoryForRetry())
        hr = CreateTextureFromFile(Graph->device,
        sourceFile.m_value, colorKey, imageInfo, &owner->m_texture);

    if (hr != 0u)
    {
        STRING subject = Printf("texture from file '%s'", sourceFile.m_value);
        if (Error)
        {
            STRING objectFormat("TEXTURE");
            Error->Error(objectFormat, E_CREATE, subject, hr);
        }
        return;
    }

    IDirect3DTexture9* const texture = owner->m_texture;
    D3DSURFACE_DESC desc;
    (void) texture->GetLevelDesc(0u, &desc);
    owner->m_width = desc.Width;
    owner->m_height = desc.Height;
    owner->m_format = desc.Format;
    if (desc.Format == D3DFMT_A8R8G8B8 || desc.Format == D3DFMT_A4R4G4B4 || desc.Format == D3DFMT_A1R5G5B5 ||
        desc.Format == D3DFMT_DXT3 || desc.Format == D3DFMT_DXT5)
    owner->m_flags |= 8u;
    memoryInUse = memoryInUse+
        TextureByteCount(desc.Format, desc.Width, desc.Height);
    return;
}

static unsigned int TextureNextPow2AtLeast4(unsigned int requestedSize)
{
    unsigned int powerOfTwo = 4u;
    while (BitsToSigned32(requestedSize)>BitsToSigned32(powerOfTwo)) powerOfTwo += powerOfTwo;
    return powerOfTwo;
}



