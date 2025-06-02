//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		DirectX HLSL implementation of Shader
//
// $NoKeywords: $dxshader
//===============================================================================//

#pragma once
#ifndef DIRECTX11SHADER_H
#define DIRECTX11SHADER_H

#include "Shader.h"

#ifdef MCENGINE_FEATURE_DIRECTX11

#include "d3d11.h"

#ifdef MCENGINE_PLATFORM_LINUX

#include <dlfcn.h>

// calling convention handling for vkd3d compatibility
#ifdef __x86_64__
#define VKD3D_CALL __attribute__((ms_abi))
#else
#define VKD3D_CALL __attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))
#endif

// vkd3d blob interface
typedef struct VKD3DBlob VKD3DBlob;
typedef struct VKD3DBlobVtbl
{
	HRESULT(VKD3D_CALL *QueryInterface)(VKD3DBlob *This, REFIID riid, void **ppvObject);
	ULONG(VKD3D_CALL *AddRef)(VKD3DBlob *This);
	ULONG(VKD3D_CALL *Release)(VKD3DBlob *This);
	void *(VKD3D_CALL *GetBufferPointer)(VKD3DBlob *This);
	SIZE_T(VKD3D_CALL *GetBufferSize)(VKD3DBlob *This);
} VKD3DBlobVtbl;

struct VKD3DBlob
{
	const VKD3DBlobVtbl *lpVtbl;
};

// function pointer type for D3DCompile with proper calling convention
typedef HRESULT(VKD3D_CALL *PFN_D3DCOMPILE_VKD3D)(LPCVOID pSrcData,
                                                  SIZE_T SrcDataSize,
                                                  LPCSTR pSourceName,
                                                  const D3D_SHADER_MACRO *pDefines,
                                                  ID3DInclude *pInclude,
                                                  LPCSTR pEntrypoint,
                                                  LPCSTR pTarget,
                                                  UINT Flags1,
                                                  UINT Flags2,
                                                  ID3DBlob **ppCode,
                                                  ID3DBlob **ppErrorMsgs);
#endif

class DirectX11Shader final : public Shader
{
public:
	DirectX11Shader(const UString &shader, bool source);
	DirectX11Shader(const UString &vertexShader, const UString &fragmentShader, bool source); // DEPRECATED
	~DirectX11Shader() override { destroy(); }

	void enable() override;
	void disable() override;

	void setUniform1f(const UString &name, float value) override;
	void setUniform1fv(const UString &name, int count, float *values) override;
	void setUniform1i(const UString &name, int value) override;
	void setUniform2f(const UString &name, float x, float y) override;
	void setUniform2fv(const UString &name, int count, float *vectors) override;
	void setUniform3f(const UString &name, float x, float y, float z) override;
	void setUniform3fv(const UString &name, int count, float *vectors) override;
	void setUniform4f(const UString &name, float x, float y, float z, float w) override;
	void setUniformMatrix4fv(const UString &name, Matrix4 &matrix) override;
	void setUniformMatrix4fv(const UString &name, float *v) override;

	// ILLEGAL:
	void onJustBeforeDraw();
	inline unsigned long getStatsNumConstantBufferUploadsPerFrame() const { return m_iStatsNumConstantBufferUploadsPerFrameCounter; }
	inline unsigned long getStatsNumConstantBufferUploadsPerFrameEngineFrameCount() const { return m_iStatsNumConstantBufferUploadsPerFrameCounterEngineFrameCount; }

	static bool loadLibs();
	static void cleanupLibs();
private:
	struct INPUT_DESC_LINE
	{
		UString type;           // e.g. "VS_INPUT"
		UString dataType;       // e.g. "POSITION", "COLOR0", "TEXCOORD0", etc.
		DXGI_FORMAT dxgiFormat; // e.g. DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, etc.
		int dxgiFormatBytes;    // e.g. "DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT" -> 12, etc.
		D3D11_INPUT_CLASSIFICATION
		classification; // e.g. D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA
	};

	struct BIND_DESC_LINE
	{
		UString type;             // e.g. "D3D11_BUFFER_DESC"
		D3D11_BIND_FLAG bindFlag; // e.g. D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER
		UString name;             // e.g. "ModelViewProjectionConstantBuffer"
		UString variableName;     // e.g. "mvp", "col", "misc", etc.
		UString variableType;     // e.g. "float4x4", "float4", "float3", "float2", "float", etc.
		int variableBytes;        // e.g. 16 -> "float4x4", 4 -> "float4", 3 -> "float3, 2 -> "float2", 1 -> "float", etc.
	};

	struct INPUT_DESC
	{
		UString type; // INPUT_DESC_LINE::type
		std::vector<INPUT_DESC_LINE> lines;
	};

	struct BIND_DESC
	{
		UString name; // BIND_DESC_LINE::name
		std::vector<BIND_DESC_LINE> lines;
		std::vector<float> floats;
	};

private:
	struct CACHE_ENTRY
	{
		int bindIndex; // into m_bindDescs[bindIndex] and m_constantBuffers[bindIndex]
		int offsetBytes;
	};

protected:
	void init() override;
	void initAsync() override;
	void destroy() override;

	bool compile(const UString &vertexShader, const UString &fragmentShader);

	void setUniform(const UString &name, void *src, size_t numBytes);

	const CACHE_ENTRY getAndCacheUniformLocation(const UString &name);

private:
	static CACHE_ENTRY invalidCacheEntry;

	UString m_sShader;
	UString m_sVsh, m_sFsh;

	bool m_bSource;

	ID3D11VertexShader *m_vs;
	ID3D11PixelShader *m_ps;
	ID3D11InputLayout *m_inputLayout;
	std::vector<ID3D11Buffer *> m_constantBuffers;
	bool m_bConstantBuffersUpToDate;

	DirectX11Shader *m_prevShader;
	ID3D11VertexShader *m_prevVS;
	ID3D11PixelShader *m_prevPS;
	ID3D11InputLayout *m_prevInputLayout;
	std::vector<ID3D11Buffer *> m_prevConstantBuffers;

	std::vector<INPUT_DESC> m_inputDescs;
	std::vector<BIND_DESC> m_bindDescs;

	std::unordered_map<std::string, CACHE_ENTRY> m_uniformLocationCache;
	std::string m_sTempStringBuffer;

	// stats
	unsigned long m_iStatsNumConstantBufferUploadsPerFrameCounter;
	unsigned long m_iStatsNumConstantBufferUploadsPerFrameCounterEngineFrameCount;

#ifdef MCENGINE_PLATFORM_LINUX
	// loading (dxvk-native)
	static void *s_vkd3dHandle;
	static PFN_D3DCOMPILE_VKD3D s_d3dCompileFunc;
#endif
	// wrapper functions for dx blob ops
	static void *getBlobBufferPointer(ID3DBlob *blob);
	static SIZE_T getBlobBufferSize(ID3DBlob *blob);
	static void releaseBlob(ID3DBlob *blob);
};

#else
class DirectX11Shader : public Shader
{};
#endif

#endif
