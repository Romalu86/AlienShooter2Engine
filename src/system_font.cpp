

#include "system_font.h"
#include "texture.h"
#include <windows.h>
#include <d3d9.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <cstring>
#include <new>
#include <cmath>
#include <cfloat>
#include <malloc.h>
#include "legacy_d3dx_font.h"


const IID IID_ID3DXFont =
    {0x4AAE6B4D, 0xD15F, 0x4909, {0xB0, 0x9F, 0x8D, 0x6A, 0xA3, 0x4A, 0xC0, 0x6B}};
const IID IID_ID3DXSprite =
    {0xB07EC84A, 0x8D35, 0x4E86, {0xA9, 0xA0, 0x8D, 0xFF, 0x21, 0xD7, 0x10, 0x75}};

static void __stdcall D3DXDebugMute(BOOL mute)
{
    typedef void (__cdecl *DebugSetMute)(BOOL);
    static DebugSetMute releaseMute = 0;
    static DebugSetMute debugMute = 0;
    static DWORD doNotMute = ~0u;
    if (!releaseMute && !debugMute)
    {
        if (GetModuleHandleA("d3d9.dll"))
        {
            HMODULE module = LoadLibraryA("d3d9.dll");
            if (module) releaseMute = reinterpret_cast<DebugSetMute>(GetProcAddress(module, "DebugSetMute"));
        }
        if (GetModuleHandleA("d3d9d.dll"))
        {
            HMODULE module = LoadLibraryA("d3d9d.dll");
            if (module) debugMute = reinterpret_cast<DebugSetMute>(GetProcAddress(module, "DebugSetMute"));
        }
    }
    if (doNotMute == ~0u)
    {
        HKEY key = 0;
        bool found = false;
        if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Direct3D", &key) == ERROR_SUCCESS)
        {
            DWORD type = REG_DWORD;
            DWORD bytes = sizeof(doNotMute);
            const LONG result = RegQueryValueExA(key, "D3DXDoNotMute", 0, &type,
                reinterpret_cast<BYTE*>(&doNotMute), &bytes);
            RegCloseKey(key);
            found = result == ERROR_SUCCESS && type == REG_DWORD;
        }
        if (!found) doNotMute = 0;
        if (doNotMute) doNotMute = 1;
    }
    if (!doNotMute)
    {
        if (releaseMute) releaseMute(mute);
        if (debugMute) debugMute(mute);
    }
}

namespace FontTexture {
struct FormatInfo {
    uint32_t format, group, bitsPerPixel, channels[5], allowsFallback;
};
static const FormatInfo kFormats[] = {
    {D3DFMT_R8G8B8, 0u, 24u, 0u, 0u, 8u, 8u, 8u, 0u},
    {D3DFMT_A8R8G8B8, 0u, 32u, 0u, 8u, 8u, 8u, 8u, 1u},
    {D3DFMT_X8R8G8B8, 0u, 32u, 0u, 0u, 8u, 8u, 8u, 1u},
    {D3DFMT_R5G6B5, 0u, 16u, 0u, 0u, 5u, 6u, 5u, 1u},
    {D3DFMT_X1R5G5B5, 0u, 16u, 0u, 0u, 5u, 5u, 5u, 1u},
    {D3DFMT_A8P8, 1u, 16u, 0u, 8u, 8u, 8u, 8u, 1u},
    {D3DFMT_P8, 1u, 8u, 0u, 8u, 8u, 8u, 8u, 1u},
    {D3DFMT_A1R5G5B5, 0u, 16u, 0u, 1u, 5u, 5u, 5u, 1u},
    {D3DFMT_A4R4G4B4, 0u, 16u, 0u, 4u, 4u, 4u, 4u, 1u},
    {D3DFMT_R3G3B2, 0u, 8u, 0u, 0u, 3u, 3u, 2u, 1u},
    {D3DFMT_A8, 0u, 8u, 0u, 8u, 0u, 0u, 0u, 1u},
    {D3DFMT_A8R3G3B2, 0u, 16u, 0u, 8u, 3u, 3u, 2u, 1u},
    {D3DFMT_X4R4G4B4, 0u, 16u, 0u, 0u, 4u, 4u, 4u, 1u},
    {D3DFMT_A2B10G10R10, 0u, 32u, 0u, 2u, 10u, 10u, 10u, 1u},
    {D3DFMT_A8B8G8R8, 0u, 32u, 0u, 8u, 8u, 8u, 8u, 1u},
    {D3DFMT_X8B8G8R8, 0u, 32u, 0u, 0u, 8u, 8u, 8u, 1u},
    {D3DFMT_G16R16, 0u, 32u, 0u, 0u, 16u, 16u, 0u, 1u},
    {D3DFMT_A2R10G10B10, 0u, 32u, 0u, 2u, 10u, 10u, 10u, 1u},
    {D3DFMT_A16B16G16R16, 0u, 64u, 0u, 16u, 16u, 16u, 16u, 1u},
    {D3DFMT_L8, 2u, 8u, 0u, 0u, 8u, 8u, 8u, 1u},
    {D3DFMT_A8L8, 2u, 16u, 0u, 8u, 8u, 8u, 8u, 1u},
    {D3DFMT_A4L4, 2u, 8u, 0u, 4u, 4u, 4u, 4u, 1u},
    {D3DFMT_L16, 2u, 16u, 0u, 0u, 16u, 16u, 16u, 1u},
    {D3DFMT_V8U8, 3u, 16u, 0u, 0u, 0u, 8u, 8u, 1u},
    {D3DFMT_L6V5U5, 3u, 16u, 6u, 0u, 0u, 5u, 5u, 1u},
    {D3DFMT_X8L8V8U8, 3u, 32u, 8u, 0u, 0u, 8u, 8u, 1u},
    {D3DFMT_Q8W8V8U8, 3u, 32u, 0u, 8u, 8u, 8u, 8u, 1u},
    {D3DFMT_V16U16, 3u, 32u, 0u, 0u, 0u, 16u, 16u, 1u},
    {D3DFMT_A2W10V10U10, 3u, 32u, 2u, 0u, 10u, 10u, 10u, 1u},
    {D3DFMT_Q16W16V16U16, 3u, 64u, 0u, 16u, 16u, 16u, 16u, 1u},
    {D3DFMT_CxV8U8, 3u, 16u, 8u, 0u, 0u, 8u, 8u, 0u},
    {D3DFMT_UYVY, 0u, 16u, 0u, 0u, 8u, 8u, 8u, 0u},
    {D3DFMT_R8G8_B8G8, 0u, 16u, 0u, 0u, 8u, 8u, 8u, 0u},
    {D3DFMT_YUY2, 0u, 16u, 0u, 0u, 8u, 8u, 8u, 0u},
    {D3DFMT_G8R8_G8B8, 0u, 16u, 0u, 0u, 8u, 8u, 8u, 0u},
    {D3DFMT_DXT1, 0u, 16u, 0u, 1u, 5u, 6u, 5u, 0u},
    {D3DFMT_DXT2, 0u, 16u, 0u, 4u, 5u, 6u, 5u, 0u},
    {D3DFMT_DXT3, 0u, 16u, 0u, 4u, 5u, 6u, 5u, 0u},
    {D3DFMT_DXT4, 0u, 16u, 0u, 8u, 5u, 6u, 5u, 0u},
    {D3DFMT_DXT5, 0u, 16u, 0u, 8u, 5u, 6u, 5u, 0u},
    {D3DFMT_D16_LOCKABLE, 4u, 16u, 1u, 0u, 0u, 0u, 16u, 1u},
    {D3DFMT_D32F_LOCKABLE, 4u, 32u, 1u, 0u, 0u, 0u, 32u, 0u},
    {D3DFMT_D32, 4u, 32u, 0u, 0u, 0u, 0u, 32u, 1u},
    {D3DFMT_D15S1, 4u, 16u, 0u, 0u, 0u, 1u, 15u, 1u},
    {D3DFMT_D24S8, 4u, 32u, 0u, 0u, 0u, 8u, 24u, 1u},
    {D3DFMT_D16, 4u, 16u, 0u, 0u, 0u, 0u, 16u, 1u},
    {D3DFMT_D24X8, 4u, 32u, 0u, 0u, 0u, 0u, 24u, 1u},
    {D3DFMT_D24X4S4, 4u, 32u, 0u, 0u, 0u, 4u, 24u, 1u},
    {D3DFMT_D24FS8, 4u, 32u, 0u, 0u, 0u, 8u, 24u, 0u},
    {D3DFMT_R16F, 0u, 16u, 0u, 0u, 16u, 0u, 0u, 0u},
    {D3DFMT_G16R16F, 0u, 32u, 0u, 0u, 16u, 16u, 0u, 0u},
    {D3DFMT_A16B16G16R16F, 0u, 64u, 0u, 16u, 16u, 16u, 16u, 0u},
    {D3DFMT_R32F, 0u, 32u, 0u, 0u, 32u, 0u, 0u, 0u},
    {D3DFMT_G32R32F, 0u, 64u, 0u, 0u, 32u, 32u, 0u, 0u},
    {D3DFMT_A32B32G32R32F, 0u, 128u, 0u, 32u, 32u, 32u, 32u, 0u},
    {MAKEFOURCC('A', 'L', '1', '6'), 2u, 32u, 0u, 16u, 16u, 16u, 16u, 0u},
    {MAKEFOURCC(' ', 'R', '1', '6'), 0u, 48u, 0u, 0u, 16u, 16u, 16u, 0u},

};
static const FormatInfo* Lookup(uint32_t format)
{
    for (size_t formatIndex = 0; formatIndex<sizeof(kFormats)/sizeof(kFormats[0]); ++formatIndex)
        if (kFormats[formatIndex].format == format) return &kFormats[formatIndex];
    return 0;
}
static const uint32_t kFormatCompatibility[5][5] = {
    {1u, 0u, 0u, 0u, 0u},
    {1u, 1u, 0u, 0u, 0u},
    {1u, 0u, 1u, 0u, 0u},
    {0u, 0u, 0u, 1u, 0u},
    {0u, 0u, 0u, 0u, 1u},
};
static uint32_t Rank(const FormatInfo& requested, const FormatInfo& candidate)
{
    if (!kFormatCompatibility[requested.group][candidate.group]) return UINT32_MAX;
    uint32_t rank = 0, nonzeroRequestedChannels = 0;
    for (unsigned channelIndex = 0; channelIndex<5; ++channelIndex)
    {
        const uint32_t requestedBits = requested.channels[channelIndex], candidateBits = candidate.channels[channelIndex];
        if (requestedBits) ++nonzeroRequestedChannels;
        if (requestedBits<candidateBits) rank += requestedBits ? candidateBits-requestedBits : 256u;
        else if (requestedBits>candidateBits) rank += candidateBits ? (requestedBits-candidateBits)<<16 : 0x1000000u;
    }
    return nonzeroRequestedChannels == (rank>>24) ? UINT32_MAX : rank;
}
struct QueryContext {
    IDirect3D9* d3d;
    D3DCAPS9 caps;
    D3DDISPLAYMODE displayMode;
    DWORD usage;
};


static uint32_t SelectFormat(uint32_t requested, QueryContext& query)
{
    const FormatInfo* source = Lookup(requested);
    if (!source) return 0;
    const FormatInfo* best = 0;
    uint32_t bestRank = UINT32_MAX;
    for (size_t formatIndex = 0; formatIndex<sizeof(kFormats)/sizeof(kFormats[0]); ++formatIndex)
    {
        const FormatInfo& candidate = kFormats[formatIndex];
        if (query.d3d && FAILED(query.d3d->CheckDeviceFormat(query.caps.AdapterOrdinal,
            query.caps.DeviceType, query.displayMode.Format, query.usage, D3DRTYPE_TEXTURE,
            static_cast<D3DFORMAT>(candidate.format)))) continue;
        if (candidate.format == requested) return requested;
        if (!candidate.allowsFallback) continue;
        const uint32_t rank = Rank(*source, candidate);
        if (rank == UINT32_MAX || rank>bestRank) continue;
        if (rank == bestRank && best && candidate.bitsPerPixel >= best->bitsPerPixel) continue;
        best = &candidate;
        bestRank = rank;
    }
    return best ? best->format : 0;
}
struct Caps {
    uint32_t textureCaps, maxWidth, maxHeight, maxAspect;
};
static uint32_t CeilPower2(uint32_t value)
{

    uint32_t result = 1;
    while (result<value) result <<= 1;
    return result;
}
static unsigned BitLength(uint32_t value)
{
    unsigned result = 0;
    while (value)
    {
        value >>= 1;
        ++result;
    }
    return result;
}


static void ApplyCaps(uint32_t& width, uint32_t& height, uint32_t* levels,
    const Caps& caps, D3DFORMAT format, bool useDeviceCaps)
{
    const uint32_t requestedLevels = levels ? *levels : UINT32_MAX;
    if (width == UINT32_MAX) width = height == UINT32_MAX ? 256u : height;
    if (height == UINT32_MAX) height = width;
    if (!width) width = 1;
    if (!height) height = 1;
    if (useDeviceCaps)
    {
        if (width>caps.maxWidth) width = caps.maxWidth;
        if (height>caps.maxHeight) height = caps.maxHeight;
        const uint32_t aspect = (caps.textureCaps&D3DPTEXTURECAPS_SQUAREONLY) ? 1u : caps.maxAspect;
        if (aspect)
        {
            const uint32_t maxWidth = aspect*height;
            if (width>maxWidth) width = maxWidth;
            const uint32_t maxHeight = aspect*width;
            if (height>maxHeight) height = maxHeight;
        }
        if ((caps.textureCaps&D3DPTEXTURECAPS_POW2) &&
            !(requestedLevels == 1u && (caps.textureCaps&D3DPTEXTURECAPS_NONPOW2CONDITIONAL)))
        {
            width = CeilPower2(width);
            height = CeilPower2(height);
        }
    }
    switch (format)
    {
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        width = (width + 3u) & ~3u;
        height = (height + 3u) & ~3u;
        break;
    default:
        break;
    }
    if (levels)
    {
        uint32_t result;
        if (useDeviceCaps && (!(caps.textureCaps&D3DPTEXTURECAPS_MIPMAP) ||
            ((caps.textureCaps&D3DPTEXTURECAPS_NONPOW2CONDITIONAL) &&
            ((width&(width-1u)) || (height&(height-1u)))))) result = 1;
        else
        {
            const uint32_t maxLevels = BitLength(width)>BitLength(height)
            ? BitLength(width) : BitLength(height);
            result = (!requestedLevels || requestedLevels>maxLevels)
            ? maxLevels : requestedLevels;
        }
        *levels = result;
    }
}

static HRESULT Check(IDirect3DDevice9* device, UINT* width, UINT* height, UINT* levels,
    DWORD usage, D3DFORMAT* format, D3DPOOL pool)
{
    if (!device) return D3DERR_INVALIDCALL;
    if (usage == UINT32_MAX) usage = 0;
    if (usage&0xFFE039ECu) return D3DERR_INVALIDCALL;
    if (pool != D3DPOOL_DEFAULT && pool != D3DPOOL_MANAGED &&
        pool != D3DPOOL_SYSTEMMEM && pool != D3DPOOL_SCRATCH) return D3DERR_INVALIDCALL;
    uint32_t selectedFormat = format ? static_cast<uint32_t>(*format) : D3DFMT_UNKNOWN;
    Caps caps = {};
    if (pool != D3DPOOL_SCRATCH)
    {
        D3DXDebugMute(TRUE);
        QueryContext queryContext;
        queryContext.d3d = 0;
        queryContext.usage = usage;
        device->GetDirect3D(&queryContext.d3d);
        device->GetDeviceCaps(&queryContext.caps);
        device->GetDisplayMode(0, &queryContext.displayMode);
        if (usage & D3DUSAGE_QUERY_VERTEXTEXTURE)
        {
            D3DDEVICE_CREATION_PARAMETERS creation;
            device->GetCreationParameters(&creation);
            if (creation.BehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING)
                queryContext.usage |= D3DUSAGE_SOFTWAREPROCESSING;
        }
        selectedFormat = SelectFormat(selectedFormat, queryContext);
        if (queryContext.d3d) queryContext.d3d->Release();
        D3DXDebugMute(FALSE);
        if (!selectedFormat) return D3DERR_NOTAVAILABLE;
        D3DCAPS9 nativeCaps;
        device->GetDeviceCaps(&nativeCaps);
        caps.textureCaps = nativeCaps.TextureCaps;
        caps.maxWidth = nativeCaps.MaxTextureWidth;
        caps.maxHeight = nativeCaps.MaxTextureHeight;
        caps.maxAspect = nativeCaps.MaxTextureAspectRatio;
    }
    else if (selectedFormat == D3DFMT_UNKNOWN) selectedFormat = D3DFMT_A8R8G8B8;
    uint32_t adjustedWidth = width ? *width : UINT32_MAX;
    uint32_t adjustedHeight = height ? *height : UINT32_MAX;
    uint32_t mipLevels = levels ? *levels : UINT32_MAX;
    ApplyCaps(adjustedWidth, adjustedHeight, levels ? &mipLevels : 0, caps,
        static_cast<D3DFORMAT>(selectedFormat), pool != D3DPOOL_SCRATCH);
    if (width) *width = adjustedWidth;
    if (height) *height = adjustedHeight;
    if (levels) *levels = mipLevels;
    if (format) *format = static_cast<D3DFORMAT>(selectedFormat);
    return S_OK;
}
static HRESULT Create(IDirect3DDevice9* device, UINT width, UINT height, UINT levels,
    DWORD usage, D3DFORMAT format, D3DPOOL pool,
    IDirect3DTexture9** result)
{
    if (!device || !result) return D3DERR_INVALIDCALL;
    if (usage == UINT32_MAX) usage = 0;
    const HRESULT hr = Check(device, &width, &height, &levels, usage, &format, pool);
    if (FAILED(hr)) return hr;
    const HRESULT created = device->CreateTexture(width, height, levels, usage&0xFFE07FFFu,
        format, pool, result, 0);
    return FAILED(created) ? created : S_OK;
}


}

namespace D3DXCore {

class CSprite final : public ID3DXSprite
{
    ULONG references_;
    IDirect3DDevice9* device_;
    BOOL active_;
    IDirect3DStateBlock9* originalState_;
    IDirect3DStateBlock9* drawState_;
    BOOL alphaBlend_;
    BOOL transformed_;

    struct Vertex { float x, y, z; DWORD color; float u, v; };
    struct TLVertex { float x, y, z, rhw; DWORD color; float u, v; };

    static void TransformCoordinate(float& x, float& y, float& z, const D3DXMATRIX& matrix)
    {
        const double sourceX = x, sourceY = y, sourceZ = z;
        x = static_cast<float>(((sourceX * matrix._11 + sourceY * matrix._21)
            + sourceZ * matrix._31) + matrix._41);
        y = static_cast<float>(((sourceX * matrix._12 + sourceY * matrix._22)
            + sourceZ * matrix._32) + matrix._42);
        z = static_cast<float>(((sourceX * matrix._13 + sourceY * matrix._23)
            + sourceZ * matrix._33) + matrix._43);
        const float homogeneous = static_cast<float>(((sourceX * matrix._14
            + sourceY * matrix._24) + sourceZ * matrix._34) + matrix._44);
        const double difference = static_cast<double>(homogeneous) - 1.0;
        if (!(difference > -FLT_EPSILON && difference < FLT_EPSILON))
        {
            const double inverse = 1.0 / homogeneous;
            x = static_cast<float>(inverse * x);
            y = static_cast<float>(inverse * y);
            z = static_cast<float>(inverse * z);
        }
    }

    void RecordState(unsigned int pass)
    {
        device_->BeginStateBlock();
        device_->SetFVF(transformed_ ? (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1) : (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1));
        device_->SetVertexShader(0);
        if (!transformed_)
        {
            D3DMATRIX identity = {};
            identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
            device_->SetTransform(D3DTS_VIEW, &identity);
            device_->SetTransform(D3DTS_PROJECTION, &identity);
            if (pass == 0) device_->SetTransform(D3DTS_WORLD, &identity);
        }
        device_->SetRenderState(D3DRS_ZENABLE, FALSE);
        device_->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device_->SetRenderState(D3DRS_WRAP0, 0);
        device_->SetRenderState(D3DRS_CLIPPING, TRUE);
        device_->SetRenderState(D3DRS_VERTEXBLEND, 0);
        device_->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
        device_->SetNPatchMode(0.0f);
        device_->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
        device_->SetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION, FALSE);
        device_->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
        device_->SetRenderState(D3DRS_COLORWRITEENABLE, 0xfu);
        if (!transformed_)
        {
            device_->SetRenderState(D3DRS_FOGENABLE, FALSE);
            device_->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
            device_->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            device_->SetRenderState(D3DRS_LIGHTING, FALSE);
            device_->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
        }
        device_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        device_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        device_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        device_->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        device_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        device_->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        device_->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        device_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device_->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        device_->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        device_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        device_->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
        if (alphaBlend_)
        {
            device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            device_->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        }
        else
        {
            device_->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            device_->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
            device_->SetRenderState(D3DRS_ALPHAREF, 127u);
        }
        if (pass == 0)
        {
            device_->SetTexture(0, 0);
            device_->SetStreamSource(0, 0, 0, 0);
            device_->EndStateBlock(&originalState_);
        }
        else device_->EndStateBlock(&drawState_);
    }

public:
CSprite() : references_(1), device_(0), active_(FALSE), originalState_(0), drawState_(0)
{
}

    ~CSprite()
    {
        if (device_)
        {
            if (drawState_)
            {
                drawState_->Release();
                drawState_ = 0;
            }
            if (originalState_)
            {
                originalState_->Release();
                originalState_ = 0;
            }
            device_->Release();
        }
    }

    HRESULT __stdcall QueryInterface(REFIID iid, void** output) override
    {
        if (!IsEqualIID(iid, IID_ID3DXSprite) && !IsEqualIID(iid, __uuidof(IUnknown)))
            return E_NOINTERFACE;
        *output = this;
        AddRef();
        return S_OK;
    }

    ULONG __stdcall AddRef() override
    {
        return ++references_;
    }

    ULONG __stdcall Release() override
    {
        const ULONG remaining = --references_;
        if (!remaining)
        {
            this->~CSprite();
            CrtFreePointer(this);
        }
        return remaining;
    }

    HRESULT __stdcall GetDevice(IDirect3DDevice9** output) override
    {
        if (!output) return D3DERR_INVALIDCALL;
        *output = device_;
        device_->AddRef();
        return S_OK;
    }

    HRESULT Initialize(IDirect3DDevice9* device)
    {
        if (!device) return D3DERR_INVALIDCALL;
        device_ = device;
        device_->AddRef();
        D3DCAPS9 caps;
        D3DDEVICE_CREATION_PARAMETERS creation;
        device_->GetDeviceCaps(&caps);
        device_->GetCreationParameters(&creation);
        transformed_ = ((caps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS) != 0 ||
            (creation.BehaviorFlags & D3DCREATE_PUREDEVICE) != 0);
        alphaBlend_ = ((caps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) != 0 &&
            (caps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) != 0);
        return S_OK;
    }

    HRESULT __stdcall Begin() override
    {
        if (active_) return D3DERR_INVALIDCALL;
        if (!originalState_)
        {
            RecordState(0);
            RecordState(1);
        }
        originalState_->Capture();
        drawState_->Apply();
        if (!transformed_)
        {
            D3DVIEWPORT9 viewport;
            device_->GetViewport(&viewport);

            const double inverseWidth = 1.0/static_cast<double>(viewport.Width);
            const float inverseHeight = static_cast<float>(1.0/static_cast<double>(viewport.Height));
            D3DMATRIX world = {};
            world._11 = static_cast<float>(inverseWidth+inverseWidth);
            world._22 = -2.0f*inverseHeight;
            world._41 = static_cast<float>(-2.0*static_cast<double>(viewport.X)*inverseWidth-inverseWidth-1.0);
            world._42 = static_cast<float>((2.0*static_cast<double>(viewport.Y)+1.0)*inverseHeight+1.0);
            world._44 = 1.0f;
            device_->SetTransform(D3DTS_WORLD, &world);
        }
        active_ = TRUE;
        return S_OK;
    }

    HRESULT __stdcall End() override
    {
        if (!active_) return D3DERR_INVALIDCALL;
        originalState_->Apply();
        active_ = FALSE;
        return S_OK;
    }

    HRESULT __stdcall OnLostDevice() override
    {
        if (device_)
        {
            if (drawState_)
            {
                drawState_->Release();
                drawState_ = 0;
            }
            if (originalState_)
            {
                originalState_->Release();
                originalState_ = 0;
            }
        }
        return S_OK;
    }


    HRESULT __stdcall OnResetDevice() override
    {
        return S_OK;
    }

    HRESULT __stdcall Draw(IDirect3DTexture9* texture, const RECT* source,
        const D3DXVECTOR2* scaling, const D3DXVECTOR2* rotationCenter,
        float rotation, const D3DXVECTOR2* translation, D3DCOLOR color) override
    {
        D3DXMATRIX transform = {};
        transform._11 = scaling ? scaling->x : 1.0f;
        transform._22 = scaling ? scaling->y : 1.0f;
        transform._33 = transform._44 = 1.0f;
        if (rotation != 0.0f)
        {
            const double halfAngle = -0.5 * static_cast<double>(rotation);
            const float z = static_cast<float>(std::sin(halfAngle));
            const float w = static_cast<float>(std::cos(halfAngle));
            const double twiceZ = 2.0 * z;
            const float xz = static_cast<float>(twiceZ * 0.0);
            const float yz = static_cast<float>(twiceZ * 0.0);
            const double xw = 0.0 * w;
            const double yw = 0.0 * w;
            const double zw = twiceZ * w;
            const double zz = twiceZ * z;
            D3DXMATRIX rotationMatrix = {};
            rotationMatrix._11 = rotationMatrix._22 = static_cast<float>(1.0 - zz);
            rotationMatrix._12 = static_cast<float>(zw);
            rotationMatrix._21 = static_cast<float>(-zw);
            rotationMatrix._13 = static_cast<float>(xz - yw);
            rotationMatrix._31 = static_cast<float>(xz + yw);
            rotationMatrix._23 = static_cast<float>(yz + xw);
            rotationMatrix._32 = static_cast<float>(yz - xw);
            rotationMatrix._33 = rotationMatrix._44 = 1.0f;
            if (rotationCenter)
            {
                transform._41 -= rotationCenter->x;
                transform._42 -= rotationCenter->y;
            }
            const D3DXMATRIX beforeRotation = transform;
            for (unsigned int row = 0; row < 4; ++row)
                for (unsigned int column = 0; column < 4; ++column)
                    transform.m[row][column] = static_cast<float>(
                        ((static_cast<double>(beforeRotation.m[row][3]) * rotationMatrix.m[3][column]
                        + static_cast<double>(beforeRotation.m[row][0]) * rotationMatrix.m[0][column])
                        + static_cast<double>(beforeRotation.m[row][2]) * rotationMatrix.m[2][column])
                        + static_cast<double>(beforeRotation.m[row][1]) * rotationMatrix.m[1][column]);
            if (rotationCenter)
            {
                transform._41 += rotationCenter->x;
                transform._42 += rotationCenter->y;
            }
        }
        if (translation)
        {
            transform._41 += translation->x;
            transform._42 += translation->y;
        }
        return DrawTransform(texture, source, &transform, color);
    }

    HRESULT __stdcall DrawTransform(IDirect3DTexture9* texture, const RECT* source,
        const D3DXMATRIX* transform, D3DCOLOR color) override
    {
        if (!texture) return D3DERR_INVALIDCALL;
        D3DSURFACE_DESC textureDescription;
        texture->GetLevelDesc(0, &textureDescription);
        RECT sampleRect;
        if (source)
        {
            sampleRect = *source;

            if (sampleRect.left<0 || sampleRect.left>sampleRect.right ||
                sampleRect.left>static_cast<LONG>(textureDescription.Width) || sampleRect.top<0 ||
                sampleRect.top>sampleRect.bottom || sampleRect.top>static_cast<LONG>(textureDescription.Height))
            return D3DERR_INVALIDCALL;
        }
        else
        {
            sampleRect.left = sampleRect.top = 0;
            sampleRect.right = static_cast<LONG>(textureDescription.Width);
            sampleRect.bottom = static_cast<LONG>(textureDescription.Height);
        }
        BOOL hasAlpha = ((color & 0xff000000u) != 0xff000000u);
        switch (textureDescription.Format)
        {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_A8P8:
        case D3DFMT_P8:
        case D3DFMT_A8L8:
        case D3DFMT_A4L4:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            hasAlpha = TRUE;
            break;
        default:
            break;
        }
        const BOOL wasActive = active_;
        if (!wasActive)
        {
            const HRESULT begin = Begin();
            if (FAILED(begin)) return begin;
        }
        HRESULT result = device_->SetTexture(0, texture);
        if (SUCCEEDED(result))
            result = device_->SetRenderState(alphaBlend_ ? D3DRS_ALPHABLENDENABLE :
            D3DRS_ALPHATESTENABLE, hasAlpha);
        if (SUCCEEDED(result))
        {
            const float width = static_cast<float>(sampleRect.right-sampleRect.left);
            const float height = static_cast<float>(sampleRect.bottom-sampleRect.top);
            const float inverseWidth = static_cast<float>(1.0/static_cast<double>(textureDescription.Width));
            const float inverseHeight = static_cast<float>(1.0/static_cast<double>(textureDescription.Height));
            const float u0 = static_cast<float>(static_cast<double>(sampleRect.left)*inverseWidth);
            const float u1 = static_cast<float>(static_cast<double>(sampleRect.right)*inverseWidth);
            const float v0 = static_cast<float>(static_cast<double>(sampleRect.top)*inverseHeight);
            const float v1 = static_cast<float>(static_cast<double>(sampleRect.bottom)*inverseHeight);
            if (transformed_)
            {
                TLVertex vertices[4] = {
                    {0.0f, 0.0f, 0.0f, 1.0f, color, u0, v0},
                    {0.0f, height, 0.0f, 1.0f, color, u0, v1},
                    {width, height, 0.0f, 1.0f, color, u1, v1},
                    {width, 0.0f, 0.0f, 1.0f, color, u1, v0}
                };
                for (unsigned int index = 0; index < 4; ++index)
                {
                    if (transform) TransformCoordinate(vertices[index].x,
                        vertices[index].y, vertices[index].z, *transform);
                    vertices[index].x -= 0.5f;
                    vertices[index].y -= 0.5f;
                    vertices[index].z = 0.0f;
                    vertices[index].rhw = 1.0f;
                }
                result = device_->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(TLVertex));
            }
            else
            {
                Vertex vertices[4] = {
                    {0.0f, 0.0f, 0.0f, color, u0, v0},
                    {0.0f, height, 0.0f, color, u0, v1},
                    {width, height, 0.0f, color, u1, v1},
                    {width, 0.0f, 0.0f, color, u1, v0}
                };
                if (transform)
                    for (unsigned int index = 0; index < 4; ++index)
                        TransformCoordinate(vertices[index].x, vertices[index].y,
                            vertices[index].z, *transform);
                result = device_->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(Vertex));
            }
            if (SUCCEEDED(result)) result = S_OK;
        }
        if (!wasActive)
        {
            const HRESULT end = End();
            if (FAILED(end) && SUCCEEDED(result)) result = end;
        }
        return result;
    }
};


class CText
{
protected:
    CText();
    ~CText();
    HRESULT Initialize(IDirect3DDevice9* device);
public:
    static HRESULT __stdcall Create(IDirect3DDevice9* device, CText** output);
    void AddRef()
    {
        ++references;
    }
    void Release()
    {
        if (--references == 0)
        {
            this->~CText();
            CrtFreePointer(this);
        }
    }
    HRESULT Begin();
    HRESULT End();
    HRESULT OnLostDevice();
    HRESULT OnResetDevice();
    HRESULT OnDeleteFont();
    int DrawTextAW(HFONT font, const void* text, int count, RECT* rect, DWORD flags, D3DCOLOR color, BOOL unicode);

private:
    static CText* s_pList;
    ULONG references;
    IDirect3DDevice9* device;
    UINT textureIndex;
    ID3DXSprite* sprite;
    IDirect3DTexture9* textures[8];
    IDirect3DSurface9* surfaces[8];
    DWORD dynamicTextures;
    D3DFORMAT textureFormat;
    UINT textureWidth;
    UINT textureHeight;
    BOOL active;
    IDirect3DStateBlock9* savedDepth;
    IDirect3DStateBlock9* textDepth;
    HDC dc;
    BITMAPINFOHEADER bitmapInfo;
    HBITMAP bitmap;
    HGDIOBJ originalBitmap;
    DWORD* pixels;
    D3DFORMAT bitmapFormat;
    HGDIOBJ originalFont;
    CText* next;
};

CText* CText::s_pList=0;


CText::CText()
: references(1), device(0), textureIndex(0), sprite(0), active(FALSE),
savedDepth(0), textDepth(0), dc(0), bitmap(0), originalBitmap(0), originalFont(0), next(0)
{
    for (UINT textureSlot = 0; textureSlot<8; ++textureSlot)
    {
        textures[textureSlot] = 0;
        surfaces[textureSlot] = 0;
    }
}


CText::~CText()
{
    if (savedDepth)
    {
        savedDepth->Release();
        savedDepth = 0;
    }
    if (textDepth)
    {
        textDepth->Release();
        textDepth = 0;
    }
    for (UINT textureSlot = 0; textureSlot<8; ++textureSlot)
    {
        if (surfaces[textureSlot])
        {
            surfaces[textureSlot]->Release();
            surfaces[textureSlot] = 0;
        }
        if (textures[textureSlot])
        {
            textures[textureSlot]->Release();
            textures[textureSlot] = 0;
        }
    }
    if (sprite)
    {
        sprite->Release();
        sprite = 0;
    }
    if (device)
    {
        device->Release();
        device = 0;
    }
    if (dc && originalFont) SelectObject(dc, originalFont);
    if (dc && originalBitmap) SelectObject(dc, originalBitmap);
    if (bitmap) DeleteObject(bitmap);
    if (dc) DeleteDC(dc);
    CText** link = &s_pList;
    while (*link && *link != this) link = &(*link)->next;
    if (*link)*link = next;
}


HRESULT __stdcall CText::Create(IDirect3DDevice9* device, CText** output)
{
    for (CText* text = s_pList; text; text = text->next)
    {
        if (text->device == device)
        {
            text->AddRef();
            *output = text;
            return S_OK;
        }
    }
    void* const storage = CrtNew(sizeof(CText));
    if (!storage) return E_OUTOFMEMORY;
    CText* text = new(storage) CText;
    const HRESULT result = text->Initialize(device);
    if (FAILED(result))
    {
        text->~CText();
        CrtFreePointer(text);
        return result;
    }
    *output = text;
    return S_OK;
}


HRESULT CText::Initialize(IDirect3DDevice9* newDevice)
{
    next = s_pList;
    s_pList = this;
    device = newDevice;
    device->AddRef();
    D3DCAPS9 caps;
    D3DDEVICE_CREATION_PARAMETERS creation;
    device->GetDeviceCaps(&caps);
    device->GetCreationParameters(&creation);
    dynamicTextures = caps.Caps2&D3DCAPS2_DYNAMICTEXTURES;
    textureWidth = 64;
    textureHeight = 16;
    textureFormat = D3DFMT_A8R8G8B8;
    HRESULT result = OnResetDevice();
    if (FAILED(result)) return result;
    result = D3DXCreateSprite(device, &sprite);
    if (FAILED(result)) return result;
    std::memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.biSize = sizeof(bitmapInfo);
    bitmapInfo.biWidth = 256;
    bitmapInfo.biHeight = 256;
    bitmapInfo.biPlanes = 1;
    bitmapInfo.biBitCount = 32;
    bitmapInfo.biCompression = BI_RGB;
    bitmapFormat = D3DFMT_A8R8G8B8;
    dc = CreateCompatibleDC(0);
    if (!dc) return E_OUTOFMEMORY;
    bitmapInfo.biHeight = -bitmapInfo.biHeight;
    bitmap = CreateDIBSection(dc, reinterpret_cast<const BITMAPINFO*>(&bitmapInfo),
        DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels), 0, 0);
    if (!bitmap) return E_OUTOFMEMORY;
    bitmapInfo.biHeight = -bitmapInfo.biHeight;
    SetBkMode(dc, OPAQUE);
    SetBkColor(dc, 0);
    SetTextColor(dc, 0x00FFFFFF);
    originalBitmap = SelectObject(dc, bitmap);
    return S_OK;
}


HRESULT CText::Begin()
{
    if (active) return D3DERR_INVALIDCALL;
    if (!savedDepth || !textDepth)
    {
        device->BeginStateBlock();
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->EndStateBlock(&savedDepth);
        device->BeginStateBlock();
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->EndStateBlock(&textDepth);
    }
    sprite->Begin();
    savedDepth->Capture();
    textDepth->Apply();
    active = TRUE;
    return S_OK;
}

HRESULT CText::End()
{
    if (!active) return D3DERR_INVALIDCALL;
    savedDepth->Apply();
    sprite->End();
    active = FALSE;
    return S_OK;
}


HRESULT CText::OnLostDevice()
{
    if (device)
    {
        if (savedDepth)
        {
            savedDepth->Release();
            savedDepth = 0;
        }
        if (textDepth)
        {
            textDepth->Release();
            textDepth = 0;
        }
    }
    if (sprite) sprite->OnLostDevice();
    if (dynamicTextures)
    {
        for (UINT textureSlot = 0; textureSlot<8; ++textureSlot)
        {
            if (surfaces[textureSlot])
            {
                surfaces[textureSlot]->Release();
                surfaces[textureSlot] = 0;
            }
            if (textures[textureSlot])
            {
                textures[textureSlot]->Release();
                textures[textureSlot] = 0;
            }
        }
    }
    return S_OK;
}


HRESULT CText::OnResetDevice()
{
    if (textures[0]) return S_OK;
    const DWORD usage = dynamicTextures?D3DUSAGE_DYNAMIC:0;
    const D3DPOOL pool = dynamicTextures?D3DPOOL_DEFAULT:D3DPOOL_MANAGED;
    const UINT count = dynamicTextures?1:8;
    HRESULT result = FontTexture::Check(device, &textureWidth, &textureHeight, 0, usage, &textureFormat, pool);
    if (FAILED(result)) return result;
    for (UINT textureSlot = 0; textureSlot<count; ++textureSlot)
    {
        result = FontTexture::Create(device, textureWidth, textureHeight, 1, usage, textureFormat, pool, &textures[textureSlot]);
        if (FAILED(result)) return result;
        result = textures[textureSlot]->GetSurfaceLevel(0, &surfaces[textureSlot]);
        if (FAILED(result)) return result;
    }
    return S_OK;
}


HRESULT CText::OnDeleteFont()
{
    if (dc && originalFont)
    {
        SelectObject(dc, originalFont);
        originalFont = 0;
    }
    return S_OK;
}


int CText::DrawTextAW(HFONT font, const void* text, int count, RECT* rect, DWORD flags, D3DCOLOR color, BOOL unicode)
{
    if (!text || !rect) return 0;
    UINT width = static_cast<UINT>(rect->right)-static_cast<UINT>(rect->left);
    UINT height = static_cast<UINT>(rect->bottom)-static_cast<UINT>(rect->top);
    if (unicode)
    {
        OSVERSIONINFOA version;
        version.dwOSVersionInfoSize = sizeof(version);
        GetVersionExA(&version);
        if (version.dwPlatformId != VER_PLATFORM_WIN32_NT)
        {
            const int bytes = WideCharToMultiByte(CP_ACP, 0, static_cast<const wchar_t*>(text), count, 0, 0, 0, 0);
            char* converted = static_cast<char*>(_alloca((bytes+3)&~3));
            WideCharToMultiByte(CP_ACP, 0, static_cast<const wchar_t*>(text), count, converted, bytes, 0, 0);
            text = converted;
            count = bytes;
            unicode = FALSE;
        }
    }
    const HGDIOBJ previous = SelectObject(dc, font);
    if (!originalFont) originalFont = previous;
    RECT measured = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    int result = unicode
    ? ::DrawTextW(dc, static_cast<const wchar_t*>(text), count, &measured, flags | DT_CALCRECT)
    : ::DrawTextA(dc, static_cast<const char*>(text), count, &measured, flags | DT_CALCRECT);
    UINT measuredWidth = static_cast<UINT>(measured.right)-static_cast<UINT>(measured.left);
    UINT measuredHeight = static_cast<UINT>(measured.bottom)-static_cast<UINT>(measured.top);
    if (!(flags&DT_CALCRECT))
    {
        if (!(flags&DT_NOCLIP) && measuredWidth >= width) measuredWidth = width;
        else width = measuredWidth;
        if (!(flags&DT_NOCLIP) && measuredHeight >= height) measuredHeight = height;
        else height = measuredHeight;
    }
    measured.left = 0;
    measured.top = 0;
    measured.right = static_cast<LONG>(measuredWidth);
    measured.bottom = static_cast<LONG>(measuredHeight);
    RECT target;
    if (flags&DT_RIGHT)
    {
        target.right = rect->right;
        target.left = static_cast<LONG>(static_cast<UINT>(target.right)-measuredWidth);
    }
    else
    {
        target.left = (flags&DT_CENTER)?(static_cast<LONG>(static_cast<UINT>(rect->left)+static_cast<UINT>(rect->right)-measuredWidth)>>1):rect->left;
        target.right = static_cast<LONG>(static_cast<UINT>(target.left)+measuredWidth);
    }
    if (flags&DT_BOTTOM)
    {
        target.bottom = rect->bottom;
        target.top = static_cast<LONG>(static_cast<UINT>(target.bottom)-measuredHeight);
    }
    else
    {
        target.top = (flags&DT_VCENTER)?(static_cast<LONG>(static_cast<UINT>(rect->top)+static_cast<UINT>(rect->bottom)-measuredHeight)>>1):rect->top;
        target.bottom = static_cast<LONG>(static_cast<UINT>(target.top)+measuredHeight);
    }
    if (flags&DT_CALCRECT)
    {
        *rect = target;
        return result;
    }
    if (width>static_cast<UINT>(bitmapInfo.biWidth) || height>static_cast<UINT>(bitmapInfo.biHeight))
    {
        BITMAPINFOHEADER larger = bitmapInfo;
        while (static_cast<UINT>(larger.biWidth)<width) larger.biWidth = static_cast<LONG>(static_cast<UINT>(larger.biWidth)+static_cast<UINT>(larger.biWidth>>1));
        while (static_cast<UINT>(larger.biHeight)<height) larger.biHeight = static_cast<LONG>(static_cast<UINT>(larger.biHeight)+static_cast<UINT>(larger.biHeight>>1));
        larger.biHeight = -larger.biHeight;
        DWORD* newPixels;
        HBITMAP newBitmap = CreateDIBSection(dc, reinterpret_cast<const BITMAPINFO*>(&larger),
            DIB_RGB_COLORS, reinterpret_cast<void**>(&newPixels), 0, 0);
        if (!newBitmap) return 0;
        larger.biHeight = -larger.biHeight;
        SelectObject(dc, newBitmap);
        DeleteObject(bitmap);
        pixels = newPixels;
        bitmapInfo.biWidth = larger.biWidth;
        bitmap = newBitmap;
        bitmapInfo.biHeight = larger.biHeight;
    }
    const UINT stride = (static_cast<UINT>(bitmapInfo.biWidth)*(bitmapInfo.biBitCount>>3)+3)&~3u;
    for (UINT y = 0; y<height; ++y)
        std::memset(reinterpret_cast<BYTE*>(pixels)+y*stride, 0, width*(bitmapInfo.biBitCount>>3));
    result = unicode
    ? ::DrawTextW(dc, static_cast<const wchar_t*>(text), count, &measured, flags)
    : ::DrawTextA(dc, static_cast<const char*>(text), count, &measured, flags);
    if (textureFormat != D3DFMT_A8R8G8B8)
    {
        for (UINT y = 0; y<height; ++y)
        {
            DWORD* row = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(pixels)+y*stride);
            for (UINT x = 0; x<width; ++x) row[x] = (row[x]<<24) | 0x00FFFFFF;
        }
    }
    const BOOL wasActive = active;
    if (!wasActive) Begin();
    for (UINT y = 0; y<height; y += textureHeight)
    {
        const UINT bottom = (y+textureHeight>height)?height:y+textureHeight;
        for (UINT x = 0; x<width; x += textureWidth)
        {
            const UINT right = (x+textureWidth>width)?width:x+textureWidth;
            const RECT source = {static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(right), static_cast<LONG>(bottom)};
            const RECT tile = {0, 0, static_cast<LONG>(right-x), static_cast<LONG>(bottom-y)};
            HRESULT upload;
            if (textureFormat == D3DFMT_A8R8G8B8)
            {
                D3DLOCKED_RECT locked;
                upload = surfaces[textureIndex]->LockRect(&locked, dynamicTextures?0:&tile,
                    D3DLOCK_NOSYSLOCK | (dynamicTextures?D3DLOCK_DISCARD:0));
                if (SUCCEEDED(upload))
                {
                    for (UINT row = 0; row<bottom-y; ++row)
                    {
                        const DWORD* input = pixels+(y+row)*bitmapInfo.biWidth+x;
                        DWORD* output = reinterpret_cast<DWORD*>(static_cast<BYTE*>(locked.pBits)+row*locked.Pitch);
                        for (UINT col = 0; col<right-x; ++col) output[col] = (input[col]<<24) | 0x00FFFFFF;
                    }
                    surfaces[textureIndex]->UnlockRect();
                }
            }
            else
            {
                upload = D3DXLoadSurfaceFromMemory(surfaces[textureIndex], 0, &tile,
                    pixels, bitmapFormat, bitmapInfo.biWidth*(bitmapInfo.biBitCount>>3),
                    0, &source, D3DX_FILTER_POINT, 0);
            }
            if (FAILED(upload))
            {
                if (!wasActive) End();
                return 0;
            }
            const float left = static_cast<float>(static_cast<UINT>(target.left)+x);
            const float top = static_cast<float>(static_cast<UINT>(target.top)+y);
            const D3DXVECTOR2 translation = {left, top};
            if (FAILED(sprite->Draw(textures[textureIndex], &tile, 0, 0, 0.0f,
                &translation, color)))
            {
                if (!wasActive) End();
                return 0;
            }
            if (!dynamicTextures) textureIndex = (textureIndex+1)&7;
        }
    }
    if (!wasActive) End();
    return result;
}

class CFont final : public ID3DXFont
{
public:
CFont() : references(1), font(0), text(0), device(0)
{
}
    ~CFont();
    HRESULT Initialize(IDirect3DDevice9* newDevice, const LOGFONTA* description);
    HRESULT __stdcall QueryInterface(REFIID iid, void** output) override;
    ULONG __stdcall AddRef() override
    {
        return ++references;
    }
    ULONG __stdcall Release() override;
    HRESULT __stdcall GetDevice(IDirect3DDevice9** output) override;
    HRESULT __stdcall GetLogFont(LOGFONTA* output) override;
    HRESULT __stdcall Begin() override
    {
        return text->Begin();
    }
    int __stdcall DrawTextA(const char* value, int count, RECT* rect, DWORD flags, D3DCOLOR color) override
    {
        return text->DrawTextAW(font, value, count, rect, flags, color, FALSE);
    }
    int __stdcall DrawTextW(const wchar_t* value, int count, RECT* rect, DWORD flags, D3DCOLOR color) override
    {
        return text->DrawTextAW(font, value, count, rect, flags, color, TRUE);
    }
    HRESULT __stdcall End() override
    {
        return text->End();
    }
    HRESULT __stdcall OnLostDevice() override
    {
        return text->OnLostDevice();
    }
    HRESULT __stdcall OnResetDevice() override
    {
        return text->OnResetDevice();
    }

private:
    ULONG references;
    HFONT font;
    LOGFONTA description;
    CText* text;
    IDirect3DDevice9* device;
};


CFont::~CFont()
{
    if (text)
    {
        text->OnDeleteFont();
        text->Release();
    }
    if (font) DeleteObject(font);
    if (device) device->Release();
}

HRESULT CFont::Initialize(IDirect3DDevice9* newDevice, const LOGFONTA* newDescription)
{
    if (!newDevice || !newDescription) return D3DERR_INVALIDCALL;
    const HRESULT result = CText::Create(newDevice, &text);
    if (FAILED(result)) return result;
    font = CreateFontIndirectA(newDescription);
    if (!font) return E_OUTOFMEMORY;
    device = newDevice;
    device->AddRef();
    description = *newDescription;
    return S_OK;
}

HRESULT __stdcall CFont::QueryInterface(REFIID iid, void** output)
{
    if (!IsEqualIID(iid, IID_ID3DXFont) && !IsEqualIID(iid, __uuidof(IUnknown)))
        return E_NOINTERFACE;
    *output = this;
    AddRef();
    return S_OK;
}

ULONG __stdcall CFont::Release()
{
    const ULONG remaining = --references;
    if (!remaining)
    {
        this->~CFont();
        CrtFreePointer(this);
    }
    return remaining;
}

HRESULT __stdcall CFont::GetDevice(IDirect3DDevice9** output)
{
    if (!output) return D3DERR_INVALIDCALL;
    *output = device;
    device->AddRef();
    return S_OK;
}

HRESULT __stdcall CFont::GetLogFont(LOGFONTA* output)
{
    if (!output) return D3DERR_INVALIDCALL;
    *output = description;
    return S_OK;
}


}


HRESULT __stdcall D3DXCreateFontIndirect(IDirect3DDevice9* device, const LOGFONTA* description, ID3DXFont** output)
{
    if (!device || !description || !output) return D3DERR_INVALIDCALL;
    void* const storage = CrtNew(sizeof(D3DXCore::CFont));
    if (!storage) return E_OUTOFMEMORY;
    D3DXCore::CFont* font = new(storage) D3DXCore::CFont;
    const HRESULT result = font->Initialize(device, description);
    if (FAILED(result))
    {
        font->~CFont();
        CrtFreePointer(font);
        return result;
    }
    *output = font;
    return S_OK;
}


HRESULT __stdcall D3DXCreateSprite(IDirect3DDevice9* device, ID3DXSprite** output)
{
    if (!device || !output) return D3DERR_INVALIDCALL;
    void* const storage = CrtNew(sizeof(D3DXCore::CSprite));
    if (!storage) return E_OUTOFMEMORY;
    D3DXCore::CSprite* sprite = new(storage) D3DXCore::CSprite;
    const HRESULT result = sprite->Initialize(device);
    if (FAILED(result))
    {
        sprite->~CSprite();
        CrtFreePointer(sprite);
        return result;
    }
    *output = sprite;
    return S_OK;
}
