#include "pch.h"
#include "FastFunctionHook.hpp"

#undef DIRECT3D_VERSION
#include "d3d9.h"

// Lantern Engine only implements the bare minimum of the fixed pipeline
// We edit the shader to support multi material (up to 4 stages and limited features)

enum Uniforms
{
    TexStageCount = 100,
    TexStageColorOp,
    TexStageEnvMap,
};

DataPointer(void**, d3ddev8, 0x03D128B0);

FastUserpurgeHookPtr<void(*)(int, int), noret, rEDX, stack4> CreateDirect3DDevice_t(0x794000);

static HRESULT WINAPI SetTextureStageState_r(IDirect3DDevice9* _this, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
FastFunctionHookPtr<decltype(&SetTextureStageState_r)>* SetTextureStageState_h;

static float texstagecount;
static float colorops[4] = {};
static float envmaps[4] = {};

static HRESULT WINAPI SetTextureStageState_r(IDirect3DDevice9* _this, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    if (Stage < 4)
    {
        if (Type == D3DTSS_COLOROP)
        {
            float f = (float)Value;
            if (colorops[Stage] != f)
            {
                colorops[Stage] = f;

                float old = texstagecount;

                texstagecount = 0.0f;
                for (int i = 0; i < 4; ++i)
                {
                    if (colorops[i] == D3DTOP_DISABLE)
                    {
                        break;
                    }
                    texstagecount += 1.0f;
                }

                if (old != texstagecount)
                {
                    _this->SetVertexShaderConstantF(TexStageCount, &texstagecount, 1);
                    _this->SetPixelShaderConstantF(TexStageCount, &texstagecount, 1);
                }

                _this->SetPixelShaderConstantF(TexStageColorOp, colorops, 1);
            }
            
        }
        else if (Type == D3DTSS_TEXCOORDINDEX)
        {
            float f = Value == D3DTSS_TCI_CAMERASPACENORMAL ? 1.0f : 0.0f;
            if (envmaps[Stage] != f)
            {
                envmaps[Stage] = f;
                _this->SetVertexShaderConstantF(TexStageEnvMap, envmaps, 1);
            }
        }
    }

    return SetTextureStageState_h->Original(_this, Stage, Type, Value);
}

#define HOOK_VTBL(name, id) name##_h = new FastFunctionHookPtr<decltype(&##name##_r)>((decltype(&##name##_r))vtbl[id], ##name##_r);

void CreateDirect3DDevice_r(int behavior, int type)
{
    CreateDirect3DDevice_t.Original(behavior, type);

    if (d3ddev8 && !SetTextureStageState_h)
    {
        IDirect3DDevice9* d3ddev9 = (IDirect3DDevice9*)d3ddev8[3];

        if (d3ddev9)
        {
            auto vtbl = (void**)(*(void**)d3ddev9);
            HOOK_VTBL(SetTextureStageState, 67);
        }
    }
}

void LanternSupport()
{
    CreateDirect3DDevice_t.Hook(CreateDirect3DDevice_r);
}