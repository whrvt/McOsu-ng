//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		raw DirectX 11 graphics interface
//
// $NoKeywords: $dx11i
//===============================================================================//

#pragma once
#ifndef DIRECTX11INTERFACE_H
#define DIRECTX11INTERFACE_H

#include "BaseEnvironment.h"
#include "Graphics.h"

class DirectX11Shader;

#ifdef MCENGINE_FEATURE_DIRECTX11

#include "d3d11.h"

class DirectX11Interface : public Graphics
{
public:
	struct SimpleVertex
	{
		Vector3 pos;
		Vector4 col;
		Vector2 tex;
	};

public:
	DirectX11Interface(HWND hwnd, bool minimalistContext = false);
	~DirectX11Interface() override;

	// scene
	void beginScene() override;
	void endScene() override;

	// depth buffer
	void clearDepthBuffer() override;

	// color
	void setColor(Color color) override;
	void setAlpha(float alpha) override;

	// 2d primitive drawing
	void drawPixel(int x, int y) override;
	void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type, const void *pixels) override;
	void drawLine(int x1, int y1, int x2, int y2) override;
	void drawLine(Vector2 pos1, Vector2 pos2) override;
	void drawRect(int x, int y, int width, int height) override;
	void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) override;

	void fillRect(int x, int y, int width, int height) override;
	void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor, Color bottomLeftColor, Color bottomRightColor) override;

	void drawQuad(int x, int y, int width, int height) override;
	void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft, Color topLeftColor, Color topRightColor, Color bottomRightColor,
	              Color bottomLeftColor) override;

	// 2d resource drawing
	void drawImage(Image *image) override;
	void drawString(McFont *font, UString text) override;

	// 3d type drawing
	void drawVAO(VertexArrayObject *vao) override;

	// DEPRECATED: 2d clipping
	void setClipRect(McRect clipRect) override;
	void pushClipRect(McRect clipRect) override;
	void popClipRect() override;

	// TODO:
	void fillRoundedRect(int /*x*/, int /*y*/, int /*width*/, int /*height*/, int /*radius*/) override { ; }

	// TODO (?): unused currently
	void pushStencil() override { ; }
	void fillStencil(bool /*inside*/) override { ; }
	void popStencil() override { ; }

	// renderer settings
	void setClipping(bool enabled) override;
	void setAlphaTesting(bool enabled) override;
	void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) override;
	void setBlending(bool enabled) override;
	void setBlendMode(BLEND_MODE blendMode) override;
	void setDepthBuffer(bool enabled) override;
	void setCulling(bool culling) override;
	void setAntialiasing(bool aa) override;
	void setWireframe(bool enabled) override;

	// renderer actions
	void flush() override;
	std::vector<unsigned char> getScreenshot() override;

	// renderer info
	Vector2 getResolution() const override { return m_vResolution; }
	UString getVendor() override;
	UString getModel() override;
	UString getVersion() override;
	int getVRAMTotal() override;
	int getVRAMRemaining() override;

	// device settings
	void setVSync(bool vsync) override;

	// callbacks
	void onResolutionChange(Vector2 newResolution) override;

	// factory
	Image *createImage(UString filePath, bool mipmapped, bool keepInSystemMemory) override;
	Image *createImage(int width, int height, bool mipmapped, bool keepInSystemMemory) override;
	RenderTarget *createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) override;
	Shader *createShaderFromFile(UString vertexShaderFilePath, UString fragmentShaderFilePath) override; // DEPRECATED
	Shader *createShaderFromSource(UString vertexShader, UString fragmentShader) override;               // DEPRECATED
	Shader *createShaderFromFile(UString shaderFilePath) override;
	Shader *createShaderFromSource(UString shaderSource) override;
	VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory) override;

	// ILLEGAL:
	void resizeTarget(Vector2 newResolution);
	bool enableFullscreen(bool borderlessWindowedFullscreen = false);
	void disableFullscreen();
	void setActiveShader(DirectX11Shader *shader) { m_activeShader = shader; }
	inline bool isReady() const { return m_bReady; }
	inline ID3D11Device *getDevice() const { return m_device; }
	inline ID3D11DeviceContext *getDeviceContext() const { return m_deviceContext; }
	inline IDXGISwapChain *getSwapChain() const { return m_swapChain; }
	inline DirectX11Shader *getShaderGeneric() const { return m_shaderTexturedGeneric; }
	inline DirectX11Shader *getActiveShader() const { return m_activeShader; }

protected:
	void init() override;
	void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) override;

private:
	static int primitiveToDirectX(Graphics::PRIMITIVE primitive);
	static int compareFuncToDirectX(Graphics::COMPARE_FUNC compareFunc);

private:
	bool m_bReady;

	// device context
	HWND m_hwnd;
	bool m_bMinimalistContext;

	// d3d
	ID3D11Device *m_device;
	ID3D11DeviceContext *m_deviceContext;
	DXGI_MODE_DESC m_swapChainModeDesc;
	IDXGISwapChain *m_swapChain;
	ID3D11RenderTargetView *m_frameBuffer;
	ID3D11Texture2D *m_frameBufferDepthStencilTexture;
	ID3D11DepthStencilView *m_frameBufferDepthStencilView;

	// renderer
	bool m_bIsFullscreen;
	bool m_bIsFullscreenBorderlessWindowed;
	Vector2 m_vResolution;

	ID3D11RasterizerState *m_rasterizerState;
	D3D11_RASTERIZER_DESC m_rasterizerDesc;

	ID3D11DepthStencilState *m_depthStencilState;
	D3D11_DEPTH_STENCIL_DESC m_depthStencilDesc;

	ID3D11BlendState *m_blendState;
	D3D11_BLEND_DESC m_blendDesc;

	DirectX11Shader *m_shaderTexturedGeneric;

	std::vector<SimpleVertex> m_vertices;
	size_t m_iVertexBufferMaxNumVertices;
	size_t m_iVertexBufferNumVertexOffsetCounter;
	D3D11_BUFFER_DESC m_vertexBufferDesc;
	ID3D11Buffer *m_vertexBuffer;

	// persistent vars
	bool m_bVSync;
	Color m_color;
	DirectX11Shader *m_activeShader;

	// clipping
	std::stack<McRect> m_clipRectStack;

	// stats
	int m_iStatsNumDrawCalls;
};

#else

class DirectX11Interface : public Graphics
{
public:
	void resizeTarget(Vector2) {}
	bool enableFullscreen(bool) { return false; }
	void disableFullscreen() {}
};

#endif

#endif
