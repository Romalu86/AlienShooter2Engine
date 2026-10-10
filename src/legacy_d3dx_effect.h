#pragma once
#include "basetype.h"
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <d3d9.h>
#pragma pop_macro("INPUT")

// DirectX 9 effect interfaces. All methods retain the SDK COM calling convention.
typedef const char* D3DXHANDLE;
struct D3DXVECTOR4;
struct D3DXMATRIX;
struct _D3DXEFFECT_DESC;
typedef _D3DXEFFECT_DESC D3DXEFFECT_DESC;
struct _D3DXPARAMETER_DESC;
typedef _D3DXPARAMETER_DESC D3DXPARAMETER_DESC;
struct _D3DXTECHNIQUE_DESC;
typedef _D3DXTECHNIQUE_DESC D3DXTECHNIQUE_DESC;
struct _D3DXPASS_DESC;
typedef _D3DXPASS_DESC D3DXPASS_DESC;
struct _D3DXFUNCTION_DESC;
typedef _D3DXFUNCTION_DESC D3DXFUNCTION_DESC;
struct ID3DXEffectPool;
struct ID3DXEffectStateManager;
struct ID3DXInclude;
struct ID3DXBuffer;
struct D3DXMACRO;
struct ID3DXEffect;

extern "C" HRESULT __stdcall D3DXCreateEffectFromFileA(
    IDirect3DDevice9* device, const char* path, const D3DXMACRO* defines,
    ID3DXInclude* include, DWORD flags, ID3DXEffectPool* pool,
    ID3DXEffect** effectOut, ID3DXBuffer** errorsOut);

struct ID3DXBaseEffect : IUnknown
{
    virtual HRESULT __stdcall GetDesc(D3DXEFFECT_DESC*) = 0;
    virtual HRESULT __stdcall GetParameterDesc(D3DXHANDLE, D3DXPARAMETER_DESC*) = 0;
    virtual HRESULT __stdcall GetTechniqueDesc(D3DXHANDLE, D3DXTECHNIQUE_DESC*) = 0;
    virtual HRESULT __stdcall GetPassDesc(D3DXHANDLE, D3DXPASS_DESC*) = 0;
    virtual HRESULT __stdcall GetFunctionDesc(D3DXHANDLE, D3DXFUNCTION_DESC*) = 0;
    virtual D3DXHANDLE __stdcall GetParameter(D3DXHANDLE, UINT) = 0;
    virtual D3DXHANDLE __stdcall GetParameterByName(D3DXHANDLE, LPCSTR) = 0;
    virtual D3DXHANDLE __stdcall GetParameterBySemantic(D3DXHANDLE, LPCSTR) = 0;
    virtual D3DXHANDLE __stdcall GetParameterElement(D3DXHANDLE, UINT) = 0;
    virtual D3DXHANDLE __stdcall GetTechnique(UINT) = 0;
    virtual D3DXHANDLE __stdcall GetTechniqueByName(LPCSTR) = 0;
    virtual D3DXHANDLE __stdcall GetPass(D3DXHANDLE, UINT) = 0;
    virtual D3DXHANDLE __stdcall GetPassByName(D3DXHANDLE, LPCSTR) = 0;
    virtual D3DXHANDLE __stdcall GetFunction(UINT) = 0;
    virtual D3DXHANDLE __stdcall GetFunctionByName(LPCSTR) = 0;
    virtual D3DXHANDLE __stdcall GetAnnotation(D3DXHANDLE, UINT) = 0;
    virtual D3DXHANDLE __stdcall GetAnnotationByName(D3DXHANDLE, LPCSTR) = 0;
    virtual HRESULT __stdcall SetValue(D3DXHANDLE, LPCVOID, UINT) = 0;
    virtual HRESULT __stdcall GetValue(D3DXHANDLE, LPVOID, UINT) = 0;
    virtual HRESULT __stdcall SetBool(D3DXHANDLE, BOOL) = 0;
    virtual HRESULT __stdcall GetBool(D3DXHANDLE, BOOL*) = 0;
    virtual HRESULT __stdcall SetBoolArray(D3DXHANDLE, const BOOL*, UINT) = 0;
    virtual HRESULT __stdcall GetBoolArray(D3DXHANDLE, BOOL*, UINT) = 0;
    virtual HRESULT __stdcall SetInt(D3DXHANDLE, INT) = 0;
    virtual HRESULT __stdcall GetInt(D3DXHANDLE, INT*) = 0;
    virtual HRESULT __stdcall SetIntArray(D3DXHANDLE, const INT*, UINT) = 0;
    virtual HRESULT __stdcall GetIntArray(D3DXHANDLE, INT*, UINT) = 0;
    virtual HRESULT __stdcall SetFloat(D3DXHANDLE, FLOAT) = 0;
    virtual HRESULT __stdcall GetFloat(D3DXHANDLE, FLOAT*) = 0;
    virtual HRESULT __stdcall SetFloatArray(D3DXHANDLE, const FLOAT*, UINT) = 0;
    virtual HRESULT __stdcall GetFloatArray(D3DXHANDLE, FLOAT*, UINT) = 0;
    virtual HRESULT __stdcall SetVector(D3DXHANDLE, const D3DXVECTOR4*) = 0;
    virtual HRESULT __stdcall GetVector(D3DXHANDLE, D3DXVECTOR4*) = 0;
    virtual HRESULT __stdcall SetVectorArray(D3DXHANDLE, const D3DXVECTOR4*, UINT) = 0;
    virtual HRESULT __stdcall GetVectorArray(D3DXHANDLE, D3DXVECTOR4*, UINT) = 0;
    virtual HRESULT __stdcall SetMatrix(D3DXHANDLE, const D3DXMATRIX*) = 0;
    virtual HRESULT __stdcall GetMatrix(D3DXHANDLE, D3DXMATRIX*) = 0;
    virtual HRESULT __stdcall SetMatrixArray(D3DXHANDLE, const D3DXMATRIX*, UINT) = 0;
    virtual HRESULT __stdcall GetMatrixArray(D3DXHANDLE, D3DXMATRIX*, UINT) = 0;
    virtual HRESULT __stdcall SetMatrixPointerArray(D3DXHANDLE, const D3DXMATRIX**, UINT) = 0;
    virtual HRESULT __stdcall GetMatrixPointerArray(D3DXHANDLE, D3DXMATRIX**, UINT) = 0;
    virtual HRESULT __stdcall SetMatrixTranspose(D3DXHANDLE, const D3DXMATRIX*) = 0;
    virtual HRESULT __stdcall GetMatrixTranspose(D3DXHANDLE, D3DXMATRIX*) = 0;
    virtual HRESULT __stdcall SetMatrixTransposeArray(D3DXHANDLE, const D3DXMATRIX*, UINT) = 0;
    virtual HRESULT __stdcall GetMatrixTransposeArray(D3DXHANDLE, D3DXMATRIX*, UINT) = 0;
    virtual HRESULT __stdcall SetMatrixTransposePointerArray(D3DXHANDLE, const D3DXMATRIX**, UINT) = 0;
    virtual HRESULT __stdcall GetMatrixTransposePointerArray(D3DXHANDLE, D3DXMATRIX**, UINT) = 0;
    virtual HRESULT __stdcall SetString(D3DXHANDLE, LPCSTR) = 0;
    virtual HRESULT __stdcall GetString(D3DXHANDLE, LPCSTR*) = 0;
    virtual HRESULT __stdcall SetTexture(D3DXHANDLE, IDirect3DBaseTexture9*) = 0;
    virtual HRESULT __stdcall GetTexture(D3DXHANDLE, IDirect3DBaseTexture9**) = 0;
    virtual HRESULT __stdcall GetPixelShader(D3DXHANDLE, IDirect3DPixelShader9**) = 0;
    virtual HRESULT __stdcall GetVertexShader(D3DXHANDLE, IDirect3DVertexShader9**) = 0;
    virtual HRESULT __stdcall SetArrayRange(D3DXHANDLE, UINT, UINT) = 0;
};

struct ID3DXEffect : ID3DXBaseEffect
{
    virtual HRESULT __stdcall GetPool(ID3DXEffectPool**) = 0;
    virtual HRESULT __stdcall SetTechnique(D3DXHANDLE) = 0;
    virtual D3DXHANDLE __stdcall GetCurrentTechnique() = 0;
    virtual HRESULT __stdcall ValidateTechnique(D3DXHANDLE) = 0;
    virtual HRESULT __stdcall FindNextValidTechnique(D3DXHANDLE, D3DXHANDLE*) = 0;
    virtual BOOL __stdcall IsParameterUsed(D3DXHANDLE, D3DXHANDLE) = 0;
    virtual HRESULT __stdcall Begin(UINT*, DWORD) = 0;
    virtual HRESULT __stdcall BeginPass(UINT) = 0;
    virtual HRESULT __stdcall CommitChanges() = 0;
    virtual HRESULT __stdcall EndPass() = 0;
    virtual HRESULT __stdcall End() = 0;
    virtual HRESULT __stdcall GetDevice(IDirect3DDevice9**) = 0;
    virtual HRESULT __stdcall OnLostDevice() = 0;
    virtual HRESULT __stdcall OnResetDevice() = 0;
    virtual HRESULT __stdcall SetStateManager(ID3DXEffectStateManager*) = 0;
    virtual HRESULT __stdcall GetStateManager(ID3DXEffectStateManager**) = 0;
    virtual HRESULT __stdcall BeginParameterBlock() = 0;
    virtual D3DXHANDLE __stdcall EndParameterBlock() = 0;
    virtual HRESULT __stdcall ApplyParameterBlock(D3DXHANDLE) = 0;
    virtual HRESULT __stdcall DeleteParameterBlock(D3DXHANDLE) = 0;
    virtual HRESULT __stdcall CloneEffect(IDirect3DDevice9*, ID3DXEffect**) = 0;
    virtual HRESULT __stdcall SetRawValue(D3DXHANDLE, LPCVOID, UINT, UINT) = 0;
};
