#pragma once
#include "basetype.h"

#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <d3d9.h>
#pragma pop_macro("INPUT")

struct D3DXVECTOR2
{
    float x, y;
};

struct D3DXMATRIX : D3DMATRIX
{
};

extern const IID IID_ID3DXFont;
extern const IID IID_ID3DXSprite;

// These early D3DX9 interfaces have a different ABI from later SDK versions.
struct ID3DXFont : IUnknown
{
    virtual HRESULT __stdcall GetDevice(IDirect3DDevice9** output) = 0;
    virtual HRESULT __stdcall GetLogFont(LOGFONTA* output) = 0;
    virtual HRESULT __stdcall Begin() = 0;
    virtual int __stdcall DrawTextA(const char* value, int count, RECT* rect, DWORD flags, D3DCOLOR color) = 0;
    virtual int __stdcall DrawTextW(const wchar_t* value, int count, RECT* rect, DWORD flags, D3DCOLOR color) = 0;
    virtual HRESULT __stdcall End() = 0;
    virtual HRESULT __stdcall OnLostDevice() = 0;
    virtual HRESULT __stdcall OnResetDevice() = 0;
};

struct ID3DXSprite : IUnknown
{
    virtual HRESULT __stdcall GetDevice(IDirect3DDevice9** output) = 0;
    virtual HRESULT __stdcall Begin() = 0;
    virtual HRESULT __stdcall Draw(IDirect3DTexture9* texture, const RECT* source,
        const D3DXVECTOR2* scaling, const D3DXVECTOR2* rotationCenter,
        float rotation, const D3DXVECTOR2* translation, D3DCOLOR color) = 0;
    virtual HRESULT __stdcall DrawTransform(IDirect3DTexture9* texture,
        const RECT* source, const D3DXMATRIX* transform, D3DCOLOR color) = 0;
    virtual HRESULT __stdcall End() = 0;
    virtual HRESULT __stdcall OnLostDevice() = 0;
    virtual HRESULT __stdcall OnResetDevice() = 0;
};
