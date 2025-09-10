//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		offscreen rendering
//
// $NoKeywords: $rt
//===============================================================================//

#pragma once
#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "Resource.h"

class ConVar;

class RenderTarget : public Resource
{
public:
	RenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
	~RenderTarget() override;

	virtual void draw(int x, int y);
	virtual void draw(int x, int y, int width, int height);
	virtual void drawRect(int x, int y, int width, int height);

	virtual void enable() = 0;
	virtual void disable() = 0;

	virtual void bind(unsigned int textureUnit = 0) = 0;
	virtual void unbind() = 0;

	void rebuild(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType);
	void rebuild(int x, int y, int width, int height);
	void rebuild(int width, int height);
	void rebuild(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType);

	// set
	void setPos(int x, int y)
	{
		m_vPos.x = x;
		m_vPos.y = y;
	}
	void setPos(Vector2 pos) { m_vPos = pos; }
	void setColor(Color color) { m_color = color; }
	void setClearColor(Color clearColor) { m_clearColor = clearColor; }
	void setClearColorOnDraw(bool clearColorOnDraw) { m_bClearColorOnDraw = clearColorOnDraw; }
	void setClearDepthOnDraw(bool clearDepthOnDraw) { m_bClearDepthOnDraw = clearDepthOnDraw; }

	// get
	[[nodiscard]] float getWidth() const { return m_vSize.x; }
	[[nodiscard]] float getHeight() const { return m_vSize.y; }
	[[nodiscard]] inline Vector2 getSize() const { return m_vSize; }
	[[nodiscard]] inline Vector2 getPos() const { return m_vPos; }
	[[nodiscard]] inline Graphics::MULTISAMPLE_TYPE getMultiSampleType() const { return m_multiSampleType; }

	[[nodiscard]] inline bool isMultiSampled() const { return m_multiSampleType != Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X; }

	// type inspection
	[[nodiscard]] Type getResType() const final { return RENDERTARGET; }

	RenderTarget *asRenderTarget() final { return this; }
	[[nodiscard]] const RenderTarget *asRenderTarget() const final { return this; }

protected:
	void init() override = 0;
	void initAsync() override = 0;
	void destroy() override = 0;

	std::unique_ptr<VertexArrayObject> m_vao1;
	std::unique_ptr<VertexArrayObject> m_vao2;
	std::unique_ptr<VertexArrayObject> m_vao3;

	Vector2 m_vPos{0.f};
	Vector2 m_vSize{0.f};

	Color m_color{static_cast<uint32_t>(-1)};
	Color m_clearColor{0};

	Graphics::MULTISAMPLE_TYPE m_multiSampleType;

	bool m_bClearColorOnDraw{true};
	bool m_bClearDepthOnDraw{true};
};

#endif
