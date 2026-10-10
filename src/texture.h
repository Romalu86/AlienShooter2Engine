#pragma once
#include "basetype.h"
#include "mylib.h"
#if defined(_WIN32)
#define INPUT WIN32_INPUT
#include <d3d9.h>
#undef INPUT
#endif
#if !defined(_WIN32)
enum _D3DFORMAT : int;
typedef _D3DFORMAT D3DFORMAT;
enum _D3DPOOL : int;
typedef _D3DPOOL D3DPOOL;
enum _D3DRESOURCETYPE : int;
typedef _D3DRESOURCETYPE D3DRESOURCETYPE;
struct tagRECT;
typedef tagRECT RECT;
struct tagPOINT;
typedef tagPOINT POINT;
#endif
struct IDirect3DDevice9;
struct IDirect3DTexture9;
struct IDirect3DSurface9;

unsigned int BaseTexture_StaticCleanupRegister01();
unsigned int BaseTexture_StaticCleanupRegister02();
unsigned int BaseTexture_StaticCleanupRegister03();
unsigned int BaseTexture_StaticCleanupRegister04();
unsigned int BaseTexture_StaticCleanupRegister05();
enum _D3DXIMAGE_FILEFORMAT : int;
typedef _D3DXIMAGE_FILEFORMAT D3DXIMAGE_FILEFORMAT;
struct _D3DXIMAGE_INFO {
    unsigned int Width;
    unsigned int Height;
    unsigned int Depth;
    unsigned int MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    D3DXIMAGE_FILEFORMAT ImageFileFormat;
};
typedef _D3DXIMAGE_INFO D3DXIMAGE_INFO;
typedef _D3DXIMAGE_INFO D3DXImageInfo;

struct tagPALETTEENTRY;
typedef tagPALETTEENTRY PALETTEENTRY;

extern "C" long __stdcall D3DXGetImageInfoFromFileA(const char* sourceFile, D3DXIMAGE_INFO* sourceInfo);
extern "C" long __stdcall D3DXSaveSurfaceToFileA(
    const char* destFile, D3DXIMAGE_FILEFORMAT destFormat, IDirect3DSurface9* sourceSurface,
    const PALETTEENTRY* sourcePalette, const RECT* sourceRect);
enum { D3DX_FILTER_POINT = 2 };
extern "C" long __stdcall D3DXLoadSurfaceFromMemory(
    IDirect3DSurface9* destSurface, const PALETTEENTRY* destPalette, const RECT* destRect,
    const void* sourceMemory, D3DFORMAT sourceFormat, unsigned int sourcePitch,
    const PALETTEENTRY* sourcePalette, const RECT* sourceRect,
    unsigned long filter, unsigned long colorKey);
extern "C" long __stdcall D3DXLoadSurfaceFromSurface(
    IDirect3DSurface9* destSurface, const PALETTEENTRY* destPalette, const RECT* destRect,
    IDirect3DSurface9* sourceSurface, const PALETTEENTRY* sourcePalette, const RECT* sourceRect,
    unsigned long filter, unsigned long colorKey);
extern "C" long __stdcall D3DXCreateTextureFromFileExA(
    IDirect3DDevice9* device, const char* sourceFile, unsigned int width, unsigned int height,
    unsigned int mipLevels, unsigned long usage, D3DFORMAT format, D3DPOOL pool,
    unsigned long filter, unsigned long mipFilter, unsigned long colorKey,
    D3DXIMAGE_INFO* sourceInfo, PALETTEENTRY* palette, IDirect3DTexture9** textureOut);



class TEXTURE
{
public:
    static int memoryInUse;
    static int capsSquare;
    static int capsDynamic;
    static int capsPalette;
    static int capsDXT;
    static int capsAlphaPalette;
    static int capsCondSizePow2;
    static int capsSizePower2;
    static int capsMaxSizeX;
    static int capsMaxSizeY;

    unsigned char* m_cpuData;
    IDirect3DTexture9* m_texture;
    D3DFORMAT     m_format;
    unsigned int  m_width;
    unsigned int  m_height;
    unsigned int  m_paletteIndex;
    unsigned int  m_flags;
    IDirect3DSurface9* m_surface;

    TEXTURE(const STRING& sourceFile, unsigned long colorKey, D3DXImageInfo* imageInfo);
    TEXTURE(int width, int height, D3DFORMAT format, unsigned long flags);
    virtual ~TEXTURE() noexcept(false);
    static void SetDeviceCaps(IDirect3DDevice9* device);
    void Create(int width, int height, D3DFORMAT format, unsigned long flags);
    IDirect3DSurface9* GetSurface();
    unsigned char* Lock(int* pitch, RECT* rect) const;
    void UnLock();
    void SetTextureToDevice(int stage);
    long Draw(RECT* destRect, RECT* sourceRect, const GAMMA& gamma);
    long CopyFromSurface(IDirect3DSurface9* sourceSurface, RECT* sourceRect, POINT* destPoint);
};

