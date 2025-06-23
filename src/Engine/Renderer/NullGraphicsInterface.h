//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty renderer, for debugging and new OS implementations
//
// $NoKeywords: $ni
//===============================================================================//

#pragma once
#ifndef NULLGRAPHICSINTERFACE_H
#define NULLGRAPHICSINTERFACE_H

#include "Graphics.h"

class NullGraphicsInterface : public Graphics
{
public:
	NullGraphicsInterface() : Graphics() {;}
	~NullGraphicsInterface() override {;}

	// scene
	void beginScene() override {;}
	void endScene() override {;}

	// depth buffer
	void clearDepthBuffer() override {;}

	// color
	void setColor(Color color) override {;}
	void setAlpha(float alpha) override {;}

	// 2d primitive drawing
	void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type, const void *pixels) override {;}
	void drawPixel(int x, int y) override {;}
	void drawLine(int x1, int y1, int x2, int y2) override {;}
	void drawLine(Vector2 pos1, Vector2 pos2) override {;}
	void drawRect(int x, int y, int width, int height) override {;}
	void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) override {;}

	void fillRect(int x, int y, int width, int height) override {;}
	void fillRoundedRect(int x, int y, int width, int height, int radius) override {;}
	void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor, Color bottomLeftColor, Color bottomRightColor) override {;}

	void drawQuad(int x, int y, int width, int height) override {;}
	void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft, Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor) override {;}

	// 2d resource drawing
	void drawImage(Image *image) override {;}
	void drawString(McFont *font, UString text) override;

	// 3d type drawing
	void drawVAO(VertexArrayObject *vao) override {;}

	// DEPRECATED: 2d clipping
	void setClipRect(McRect clipRect) override {;}
	void pushClipRect(McRect clipRect) override {;}
	void popClipRect() override {;}

	// stencil
	void pushStencil() override {;}
	void fillStencil(bool inside) override {;}
	void popStencil() override {;}

	// renderer settings
	void setClipping(bool enabled) override {;}
	void setAlphaTesting(bool enabled) override {;}
	void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) override {;}
	void setBlending(bool enabled) override {;}
	void setBlendMode(BLEND_MODE blendMode) override {;}
	void setDepthBuffer(bool enabled) override {;}
	void setCulling(bool culling) override {;}
	void setVSync(bool vsync) override {;}
	void setAntialiasing(bool aa) override {;}
	void setWireframe(bool enabled) override {;}

	// renderer actions
	void flush() override {;}
	std::vector<unsigned char> getScreenshot() override {std::vector<unsigned char> temp; temp.resize((size_t)m_vResolution.x * (size_t)m_vResolution.y * 3); return temp;} // RGB

	// renderer info
	Vector2 getResolution() const override {return m_vResolution;}
	UString getVendor() override;
	UString getModel() override;
	UString getVersion() override;
	int getVRAMTotal() override {return -1;}
	int getVRAMRemaining() override {return -1;}

	// callbacks
	void onResolutionChange(Vector2 newResolution) override {m_vResolution = newResolution;}

	// factory
	Image *createImage(UString filePath, bool mipmapped, bool keepInSystemMemory) override;
	Image *createImage(int width, int height, bool mipmapped, bool keepInSystemMemory) override;
	RenderTarget *createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) override;
	Shader *createShaderFromFile(UString vertexShaderFilePath, UString fragmentShaderFilePath) override;	// DEPRECATED
	Shader *createShaderFromSource(UString vertexShader, UString fragmentShader) override;				// DEPRECATED
	Shader *createShaderFromFile(UString shaderFilePath) override;
	Shader *createShaderFromSource(UString shaderSource) override;
	VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory) override;

protected:
	void init() override {;}
	void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) override {;}

private:
	// renderer
	Vector2 m_vResolution;
};

#endif
