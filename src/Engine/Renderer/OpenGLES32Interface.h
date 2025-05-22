//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		raw opengl es 3.2 graphics interface
//
// $NoKeywords: $gles32i
//===============================================================================//

#pragma once
#ifndef OPENGLES32INTERFACE_H
#define OPENGLES32INTERFACE_H

#include "EngineFeatures.h"

#ifdef MCENGINE_FEATURE_GLES32

#include "NullGraphicsInterface.h"
#include "OpenGLSync.h"

class OpenGLES32Shader;

class OpenGLES32Interface : public Graphics
{
public:
	friend class OpenGLES32Shader;
	OpenGLES32Interface();
	~OpenGLES32Interface() override;

	// scene
	void beginScene() final;
	void endScene() override;

	// depth buffer
	void clearDepthBuffer() final;

	// color
	void setColor(Color color) final;
	void setAlpha(float alpha) final;

	// 2d primitive drawing
	void drawLine(int x1, int y1, int x2, int y2) final;
	void drawLine(Vector2 pos1, Vector2 pos2) final;
	void drawRect(int x, int y, int width, int height) final;
	void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) final;
	// TODO
	void drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type, const void *pixels) final {;}
	void drawPixel(int x, int y) final {;}
	void fillRoundedRect(int x, int y, int width, int height, int radius) final {;}

	void fillRect(int x, int y, int width, int height) final;
	void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor, Color bottomLeftColor, Color bottomRightColor) final;

	void drawQuad(int x, int y, int width, int height) final;
	void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft, Color topLeftColor, Color topRightColor, Color bottomRightColor,
	              Color bottomLeftColor) final;

	// 2d resource drawing
	void drawImage(Image *image) final;
	void drawString(McFont *font, UString text) final;

	// 3d type drawing
	void drawVAO(VertexArrayObject *vao) final;

	// DEPRECATED: 2d clipping
	void setClipRect(McRect clipRect) final;
	void pushClipRect(McRect clipRect) final;
	void popClipRect() final;

	// stencil
	void pushStencil() final;
	void fillStencil(bool inside) final;
	void popStencil() final;

	// renderer settings
#ifndef MCENGINE_PLATFORM_WASM // not compatible with WebGL
	void setAlphaTesting(bool enabled) final;
	void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) final;
	void setAntialiasing(bool aa) final;
#endif
	void setClipping(bool enabled) final;
	void setBlending(bool enabled) final;
	void setBlendMode(BLEND_MODE blendMode) final;
	void setDepthBuffer(bool enabled) final;
	void setCulling(bool culling) final;
	void setWireframe(bool _) final;

	// renderer actions
	void flush() final;
	std::vector<unsigned char> getScreenshot() final;

	// renderer info
	[[nodiscard]] Vector2 getResolution() const final { return m_vResolution; }

	// callbacks
	void onResolutionChange(Vector2 newResolution) final;

	// factory
	Image *createImage(UString filePath, bool mipmapped, bool keepInSystemMemory) final;
	Image *createImage(int width, int height, bool mipmapped, bool keepInSystemMemory) final;
	RenderTarget *createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) final;
	Shader *createShaderFromFile(UString vertexShaderFilePath, UString fragmentShaderFilePath) final; // DEPRECATED
	Shader *createShaderFromSource(UString vertexShader, UString fragmentShader) final;               // DEPRECATED
	Shader *createShaderFromFile(UString shaderFilePath) final;
	Shader *createShaderFromSource(UString shaderSource) final;
	VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory) final;

	// matrices & transforms
	void forceUpdateTransform();

	[[nodiscard]] inline int getShaderGenericAttribPosition() const { return m_iShaderTexturedGenericAttribPosition; }
	[[nodiscard]] inline int getShaderGenericAttribUV() const { return m_iShaderTexturedGenericAttribUV; }
	[[nodiscard]] inline int getShaderGenericAttribCol() const { return m_iShaderTexturedGenericAttribCol; }

	[[nodiscard]] inline int getVBOVertices() const { return m_iVBOVertices; }
	[[nodiscard]] inline int getVBOTexcoords() const { return m_iVBOTexcoords; }
	[[nodiscard]] inline int getVBOTexcolors() const { return m_iVBOTexcolors; }

	[[nodiscard]] inline Matrix4 getMVP() const { return m_MP; }

protected:
	void init() override;
	void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) override;

private:
	void handleGLErrors();

	static int primitiveToOpenGL(Graphics::PRIMITIVE primitive);
	static int compareFuncToOpenGL(Graphics::COMPARE_FUNC compareFunc);

	void registerShader(OpenGLES32Shader *shader);
	void unregisterShader(OpenGLES32Shader *shader);
	void updateAllShaderTransforms();

	// renderer
	bool m_bInScene;
	Vector2 m_vResolution;
	Matrix4 m_projectionMatrix;
	Matrix4 m_worldMatrix;
	Matrix4 m_MP;

	OpenGLES32Shader *m_shaderTexturedGeneric;
	std::vector<OpenGLES32Shader *> m_registeredShaders;
	int m_iShaderTexturedGenericPrevType;
	int m_iShaderTexturedGenericAttribPosition;
	int m_iShaderTexturedGenericAttribUV;
	int m_iShaderTexturedGenericAttribCol;
	unsigned int m_iVBOVertices;
	unsigned int m_iVBOTexcoords;
	unsigned int m_iVBOTexcolors;

	// synchronization
	OpenGLSync *m_syncobj;

	// persistent vars
	Color m_color;
	bool m_bAntiAliasing;

	// clipping
	std::stack<McRect> m_clipRectStack;
};

#else
class OpenGLES32Interface
{};
#endif

#endif
