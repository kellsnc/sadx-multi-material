#include "pch.h"
#include <d3d8.h>
#include "SADXModLoader.h"
#include "FastFunctionHook.hpp"

#define NJD_USERFLAG_MULTITEX 0x20

#define NJD_BASIC_ENVIRONMENT_MAP   BIT_0
#define NJD_BASIC_CONSTANT_MATERIAL BIT_1
#define NJD_BASIC_OFFSET_MATERIAL   BIT_2
#define NJD_BASIC_NO_VERTEX_COLOR   BIT_4

DataPointer(IDirect3DDevice8*, _st_d3d_device_, 0x3D128B0);
DataPointer(D3DMATRIX, _gj_env_matrix_, 0x38A5DD0);
DataPointer(D3DMATRIX, _gj_unit_matrix_, 0x38A5E10);
DataPointer(int, _nj_basic_attr_, 0x3D08498);
DataPointer(NJS_TEXLIST*, _nj_current_texlist, 0x3D0FA24);
FunctionPointer(Bool, stApplyPalette, (NJS_TEXMEMLIST* memlist), 0x78CDC0);

FastThiscallHook<void, NJS_MATERIAL*> _njSetMaterial_h(0x784850);
FastFunctionHook<void> _njEndModel_h(0x781DF0);
UsercallFunction<void, int, int> stSetTextureStage(0x78D380, rEDX, rECX);

static bool multitex = false;

void SetMaterialStage(int stage, NJS_MATERIAL* material)
{
	int attrflags = material->attrflags;

	if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR)
	{
		attrflags = _nj_constant_attr_or_ | _nj_constant_attr_and_ & attrflags;
	}

	int texid = material->attr_texId & 0xFFFF;

	if (attrflags & NJD_FLAG_USE_TEXTURE)
	{
		NJS_TEXMEMLIST* memlist = (NJS_TEXMEMLIST*)_nj_current_texlist->textures[texid].texaddr;
		if (memlist->texinfo.texsurface.pSurface)
		{
			stApplyPalette(memlist);
			IDirect3DDevice8_SetTexture(_st_d3d_device_, stage, (IDirect3DBaseTexture8*)memlist->texinfo.texsurface.pSurface);
		}

		const int wraps[4] = {
			D3DTADDRESS_WRAP,
			D3DTADDRESS_CLAMP,
			D3DTADDRESS_MIRROR,
			D3DTADDRESS_CLAMP
		};

		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_ADDRESSU, wraps[(attrflags & NJD_FLAG_CLAMP_U | (attrflags >> 1) & NJD_FLAG_FLIP_V) >> 16]);
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_ADDRESSV, wraps[(attrflags & NJD_FLAG_CLAMP_V | (attrflags >> 1) & NJD_FLAG_CLAMP_U) >> 15]);

		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	}
	else
	{
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
	}

	int opindex = material->attrflags >> 29;
	int opindex_a = (material->attrflags >> 26) & 0x7;

	const int colorops[8] = {
		D3DTOP_SELECTARG1,	      // Use the new texture, disregard previous one
		D3DTOP_SELECTARG2,	      // Use the previous texture, disregard new one
		D3DTOP_ADD,			      // Add the new texture to the previous (additive blending)
		D3DTOP_SUBTRACT,	      // Subtract the new texture from the previous
		D3DTOP_MODULATE,	      // Multiply the new texture with the previous
		D3DTOP_BLENDTEXTUREALPHA, // The new texture appears based on the new texture alpha
		D3DTOP_BLENDCURRENTALPHA, // The new texture appears based on the previous texture alpha
		D3DTOP_BLENDFACTORALPHA   // The new texture appears based on the material diffuse alpha
	};

	if (opindex == D3DTOP_BLENDFACTORALPHA || opindex_a == D3DTOP_BLENDFACTORALPHA)
	{
		IDirect3DDevice8_SetRenderState(_st_d3d_device_, D3DRS_TEXTUREFACTOR, material->diffuse.color);
	}

	IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_COLOROP, colorops[opindex]);
	IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_ALPHAOP, colorops[opindex_a]);

	if (attrflags & NJD_FILTER_POINT)
	{
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_MINFILTER, D3DTEXF_POINT);
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_MIPFILTER, D3DTEXF_POINT);
	}
	else
	{
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
	}

	if (attrflags & NJD_FLAG_USE_ENV)
	{
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
		IDirect3DDevice8_SetTransform(_st_d3d_device_, (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), &_gj_env_matrix_);
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
		_nj_basic_attr_ |= NJD_BASIC_ENVIRONMENT_MAP;
	}
	else
	{
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		IDirect3DDevice8_SetTransform(_st_d3d_device_, (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), &_gj_unit_matrix_);
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_TEXCOORDINDEX, 0);
		_nj_basic_attr_ &= ~NJD_BASIC_ENVIRONMENT_MAP;
	}

	if (!(attrflags & NJD_FLAG_USE_ALPHA))
	{
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	}
}

void _njSetMaterial_r(NJS_MATERIAL* material)
{
	_njSetMaterial_h.Original(material);

	for (int i = 1; i < 8; ++i)
	{
		// If the multitex user flag is set, the next texture is the N+1 stage
		if (material->attrflags & NJD_USERFLAG_MULTITEX)
		{
			multitex = true;
			SetMaterialStage(i, ++material);
		}
		else
		{
			if (multitex)
			{
				IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, i, D3DTSS_COLOROP, D3DTOP_DISABLE);
			}
			break;
		}
	}
}

void _njEndModel_r()
{
	_njEndModel_h.Original();

	if (multitex)
	{
		IDirect3DDevice8_SetTextureStageState(_st_d3d_device_, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		multitex = false;
	}
}

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
		_njSetMaterial_h.Hook(_njSetMaterial_r);
		_njEndModel_h.Hook(_njEndModel_r);
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}