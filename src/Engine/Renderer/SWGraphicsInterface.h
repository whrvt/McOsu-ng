//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		software renderer, just for fun
//
// $NoKeywords: $swi
//===============================================================================//

#pragma once
#ifndef SWGRAPHICSINTERFACE_H
#define SWGRAPHICSINTERFACE_H

#include "Graphics.h"

#ifdef MCENGINE_FEATURE_SOFTRENDERER

class SWGraphicsInterface : public Graphics
{
public:
	struct PIXEL
	{
		unsigned char b;
		unsigned char g;
		unsigned char r;
		unsigned char a;
	};

public:
	SWGraphicsInterface();
	virtual ~SWGraphicsInterface();

	// scene
	virtual void beginScene();
	virtual void endScene();

	// depth buffer
	virtual void clearDepthBuffer();

	// color
	virtual void setColor(Color color);
	virtual void setAlpha(float alpha);

	// 2d primitive drawing
	virtual void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type, const void *pixels);
	virtual void drawPixel(int x, int y);
	virtual void drawLine(int x1, int y1, int x2, int y2);
	virtual void drawLine(Vector2 pos1, Vector2 pos2);
	virtual void drawRect(int x, int y, int width, int height);
	virtual void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left);

	virtual void fillRect(int x, int y, int width, int height);
	virtual void fillRoundedRect(int x, int y, int width, int height, int radius);
	virtual void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor, Color bottomLeftColor, Color bottomRightColor);

	virtual void drawQuad(int x, int y, int width, int height);
	virtual void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft, Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor);

	// 2d resource drawing
	virtual void drawImage(Image *image);
	virtual void drawString(McFont *font, UString text);

	// 3d type drawing
	virtual void drawVAO(VertexArrayObject *vao);

	// DEPRECATED: 2d clipping
	virtual void setClipRect(McRect clipRect);
	virtual void pushClipRect(McRect clipRect);
	virtual void popClipRect();

	// stencil
	virtual void pushStencil();
	virtual void fillStencil(bool inside);
	virtual void popStencil();

	// renderer settings
	virtual void setClipping(bool enabled);
	virtual void setAlphaTesting(bool enabled);
	virtual void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref);
	virtual void setBlending(bool enabled);
	virtual void setBlendMode(BLEND_MODE blendMode);
	virtual void setDepthBuffer(bool enabled);
	virtual void setCulling(bool culling);
	virtual void setAntialiasing(bool aa);
	virtual void setWireframe(bool enabled);

	// renderer actions
	virtual void flush();
	virtual std::vector<unsigned char> getScreenshot();

	// renderer info
	virtual Vector2 getResolution() const {return m_vResolution;}
	virtual UString getVendor();
	virtual UString getModel();
	virtual UString getVersion();
	virtual int getVRAMTotal();
	virtual int getVRAMRemaining();

	// callbacks
	virtual void onResolutionChange(Vector2 newResolution);

	// factory
	virtual Image *createImage(UString filePath, bool mipmapped, bool keepInSystemMemory);
	virtual Image *createImage(int width, int height, bool mipmapped, bool keepInSystemMemory);
	virtual RenderTarget *createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType);
	virtual Shader *createShaderFromFile(UString vertexShaderFilePath, UString fragmentShaderFilePath);	// DEPRECATED
	virtual Shader *createShaderFromSource(UString vertexShader, UString fragmentShader);				// DEPRECATED
	virtual Shader *createShaderFromFile(UString shaderFilePath);
	virtual Shader *createShaderFromSource(UString shaderSource);
	virtual VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory);

	// ILLEGAL:
	void bindTexture(Image *image, unsigned int textureUnit);

protected:
	void init();

	virtual void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix);

	inline PIXEL *getBackBuffer() const {return m_backBuffer;}

private:
	PIXEL getColorPixel(const Color &color);

	// renderer
	Vector2 m_vResolution;
	PIXEL *m_backBuffer;

	// persistent vars
	bool m_bAntiAliasing;
	Color m_color;
	float m_fZ;
	float m_fClearZ;

	// clipping
	std::stack<McRect> m_clipRectStack;

	// matrices
	Matrix4 m_worldMatrix;
	Matrix4 m_projectionMatrix;

	// texturing
	Image *m_boundTexture;
};

#else
class SWGraphicsInterface : public Graphics{};
#endif

#endif
