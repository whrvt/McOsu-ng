//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		raw opengl es 3.2 graphics interface
//
// $NoKeywords: $gles32i
//===============================================================================//

#pragma once
#ifndef OPENGLES32INTERFACE_H
#define OPENGLES32INTERFACE_H

#include "NullGraphicsInterface.h"
#include "cbase.h"

#ifdef MCENGINE_FEATURE_GLES32

#include "OpenGLSync.h"

class OpenGLES32Shader;

// Interleaved vertex format for improved performance
struct InterleavedVertex
{
	Vector3 position; // 12 bytes
	Vector2 texCoord; // 8 bytes
	Color color;      // 4 bytes
	Vector3 normal;   // 12 bytes (optional, included for future use)
};

class OpenGLES32Interface : public NullGraphicsInterface
{
public:
	friend class OpenGLES32Shader;
	OpenGLES32Interface();
	virtual ~OpenGLES32Interface();

	// scene
	virtual void beginScene();
	virtual void endScene();

	// depth buffer
	virtual void clearDepthBuffer();

	// color
	virtual void setColor(Color color);
	virtual void setAlpha(float alpha);

	// 2d primitive drawing
	virtual void drawLine(int x1, int y1, int x2, int y2);
	virtual void drawLine(Vector2 pos1, Vector2 pos2);
	virtual void drawRect(int x, int y, int width, int height);
	virtual void drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left);

	// TODO

	virtual void fillRect(int x, int y, int width, int height);
	virtual void fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor, Color bottomLeftColor, Color bottomRightColor);

	virtual void drawQuad(int x, int y, int width, int height);
	virtual void drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft, Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor);

	// 2d resource drawing
	virtual void drawImage(Image *image);
	virtual void drawString(McFont *font, UString text);

	// 3d type drawing
	void drawVAO(VertexArrayObject *vao);

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
	virtual void setWireframe(bool enabled);

	// renderer actions
	virtual void flush();
	virtual std::vector<unsigned char> getScreenshot();

	// renderer info
	virtual Vector2 getResolution() const { return m_vResolution; }
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
	virtual Shader *createShaderFromFile(UString vertexShaderFilePath, UString fragmentShaderFilePath); // DEPRECATED
	virtual Shader *createShaderFromSource(UString vertexShader, UString fragmentShader);               // DEPRECATED
	virtual Shader *createShaderFromFile(UString shaderFilePath);
	virtual Shader *createShaderFromSource(UString shaderSource);
	virtual VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory);

	// matrices & transforms
	void forceUpdateTransform();

	// friend class accessors
	inline const int getShaderGenericAttribPosition() const { return m_iShaderTexturedGenericAttribPosition; }
	inline const int getShaderGenericAttribUV() const { return m_iShaderTexturedGenericAttribUV; }
	inline const int getShaderGenericAttribCol() const { return m_iShaderTexturedGenericAttribCol; }
	inline const int getShaderGenericAttribNormal() const { return m_iShaderTexturedGenericAttribNormal; }

	inline const unsigned int getInterleavedVBO() const { return m_iInterleavedVBO; }
	inline const size_t getMaxVertices() const { return m_iMaxVertices; }

	inline Matrix4 getMVP() const { return m_MP; }

protected:
	virtual void init();
	virtual void onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix);

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
	int m_iShaderTexturedGenericAttribNormal;

	// interleaved vertex buffer
	unsigned int m_iInterleavedVBO;
	size_t m_iMaxVertices;

	// synchronization
	OpenGLSync *m_syncobj;

	// persistent vars
	Color m_color;

	// clipping
	std::stack<McRect> m_clipRectStack;
};

#else
class OpenGLES32Interface : public NullGraphicsInterface
{};
#endif

#endif
