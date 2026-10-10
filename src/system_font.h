#pragma once

#include "legacy_d3dx_font.h"

extern "C" HRESULT __stdcall D3DXCreateFontIndirect(IDirect3DDevice9* device,
    const LOGFONTA* description, ID3DXFont** output);
extern "C" HRESULT __stdcall D3DXCreateSprite(IDirect3DDevice9* device, ID3DXSprite** output);
