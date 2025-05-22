//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#pragma once
#ifndef RESOURCE_H
#define RESOURCE_H

#include "cbase.h"

class TextureAtlas;
class Sound;
class McFont;

class Resource
{
public:
	enum Type : uint8_t
	{
		IMAGE,
		FONT,
		RENDERTARGET,
		SHADER,
		TEXTUREATLAS,
		VAO,
		SOUND,
		APPDEFINED
	};

public:
	Resource();
	Resource(UString filepath);
	virtual ~Resource() { ; }

	void load();
	void loadAsync();
	void release();
	void reload();

	void interruptLoad();

	void setName(UString name) { m_sName = name; }

	[[nodiscard]] inline UString getName() const { return m_sName; }
	[[nodiscard]] inline UString getFilePath() const { return m_sFilePath; }

	[[nodiscard]] inline bool isReady() const { return m_bReady.load(); }
	[[nodiscard]] inline bool isAsyncReady() const { return m_bAsyncReady.load(); }

	// type inspection
	[[nodiscard]] virtual Type getResType() const = 0;

	virtual Image *asImage() { return nullptr; }
	virtual McFont *asFont() { return nullptr; }
	virtual RenderTarget *asRenderTarget() { return nullptr; }
	virtual Shader *asShader() { return nullptr; }
	virtual TextureAtlas *asTextureAtlas() { return nullptr; }
	virtual VertexArrayObject *asVAO() { return nullptr; }
	virtual Sound *asSound() { return nullptr; }
	[[nodiscard]] const virtual Image *asImage() const { return nullptr; }
	[[nodiscard]] const virtual McFont *asFont() const { return nullptr; }
	[[nodiscard]] const virtual RenderTarget *asRenderTarget() const { return nullptr; }
	[[nodiscard]] const virtual Shader *asShader() const { return nullptr; }
	[[nodiscard]] const virtual TextureAtlas *asTextureAtlas() const { return nullptr; }
	[[nodiscard]] const virtual VertexArrayObject *asVAO() const { return nullptr; }
	[[nodiscard]] const virtual Sound *asSound() const { return nullptr; }

protected:
	virtual void init() = 0;
	virtual void initAsync() = 0;
	virtual void destroy() = 0;

	UString m_sFilePath;
	UString m_sName;

	std::atomic<bool> m_bReady;
	std::atomic<bool> m_bAsyncReady;
	std::atomic<bool> m_bInterrupted;
};

#endif
