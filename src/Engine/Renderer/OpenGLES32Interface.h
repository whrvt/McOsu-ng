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

#include "NullGraphicsInterface.h"
#ifdef MCENGINE_FEATURE_GLES32

#include "OpenGLSync.h"

class OpenGLES32Shader;

class OpenGLES32Interface : public NullGraphicsInterface
{
public:
	friend class OpenGLES32Shader;
	OpenGLES32Interface();
	~OpenGLES32Interface() override;

	// scene
	void beginScene() override;
	void endScene() override;

	// depth buffer
	void clearDepthBuffer() override;

	// color
	void setColor(Color color) override;
	void setAlpha(float alpha) override;

	// 2d primitive drawing
	void drawLine(int x1, int y1, int x2, int y2) override;
	void drawLine(Vector2 pos1, Vector2 pos2) override;
	void drawRect(int x, int y, int width, int height) override;
	void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) override;

	// TODO

	void fillRect(int x, int y, int width, int height) override;
	void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor, Color bottomLeftColor, Color bottomRightColor) override;

	void drawQuad(int x, int y, int width, int height) override;
	void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft, Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor) override;

	// 2d resource drawing
	void drawImage(Image *image) override;
	void drawString(McFont *font, UString text) override;

	// 3d type drawing
	void drawVAO(VertexArrayObject *vao) override;

	// DEPRECATED: 2d clipping
	void setClipRect(McRect clipRect) override;
	void pushClipRect(McRect clipRect) override;
	void popClipRect() override;

	// stencil
	void pushStencil() override;
	void fillStencil(bool inside) override;
	void popStencil() override;

	// renderer settings
	void setClipping(bool enabled) override;
	void setAlphaTesting(bool enabled) override;
	void setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) override;
	void setBlending(bool enabled) override;
	void setBlendMode(BLEND_MODE blendMode) override;
	void setDepthBuffer(bool enabled) override;
	void setCulling(bool culling) override;
	void setAntialiasing(bool aa) override;
	void setWireframe(bool _) override;

	// renderer actions
	void flush() override;
	std::vector<unsigned char> getScreenshot() override;

	// renderer info
	[[nodiscard]] Vector2 getResolution() const override {return m_vResolution;}

	// callbacks
	void onResolutionChange(Vector2 newResolution) override;

	// factory
	Image *createImage(UString filePath, bool mipmapped, bool keepInSystemMemory) override;
	Image *createImage(int width, int height, bool mipmapped, bool keepInSystemMemory) override;
	RenderTarget *createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) override;
	Shader *createShaderFromFile(UString vertexShaderFilePath, UString fragmentShaderFilePath) override;	// DEPRECATED
	Shader *createShaderFromSource(UString vertexShader, UString fragmentShader) override;				// DEPRECATED
	Shader *createShaderFromFile(UString shaderFilePath) override;
	Shader *createShaderFromSource(UString shaderSource) override;
	VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory) override;

	// matrices & transforms
	void forceUpdateTransform();

	[[nodiscard]] inline int getShaderGenericAttribPosition() const {return m_iShaderTexturedGenericAttribPosition;}
	[[nodiscard]] inline int getShaderGenericAttribUV() const {return m_iShaderTexturedGenericAttribUV;}
	[[nodiscard]] inline int getShaderGenericAttribCol() const {return m_iShaderTexturedGenericAttribCol;}

	[[nodiscard]] inline int getVBOVertices() const {return m_iVBOVertices;}
	[[nodiscard]] inline int getVBOTexcoords() const {return m_iVBOTexcoords;}
	[[nodiscard]] inline int getVBOTexcolors() const {return m_iVBOTexcolors;}

	[[nodiscard]] inline Matrix4 getMVP() const {return m_MP;}

protected:
	void init() override;
	void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) override;

private:
	void handleGLErrors();

	static int primitiveToOpenGL(Graphics::PRIMITIVE primitive);
	static int compareFuncToOpenGL(Graphics::COMPARE_FUNC compareFunc);

    void registerShader(OpenGLES32Shader* shader);
    void unregisterShader(OpenGLES32Shader* shader);
    void updateAllShaderTransforms();

	// renderer
	bool m_bInScene;
	Vector2 m_vResolution;
	Matrix4 m_projectionMatrix;
	Matrix4 m_worldMatrix;
	Matrix4 m_MP;

	OpenGLES32Shader *m_shaderTexturedGeneric;
	std::vector<OpenGLES32Shader*> m_registeredShaders;
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
class OpenGLES32Interface : public NullGraphicsInterface{};
#endif

#endif
