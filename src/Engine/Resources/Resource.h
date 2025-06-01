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
friend class ResourceManager;
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

	[[nodiscard]] inline UString getName() const { return m_sName; }
	[[nodiscard]] inline UString getFilePath() const { return m_sFilePath; }

	[[nodiscard]] inline bool isReady() const { return m_bReady.load(); }
	[[nodiscard]] inline bool isAsyncReady() const { return m_bAsyncReady.load(); }

protected:
	virtual void init() = 0;
	virtual void initAsync() = 0;
	virtual void destroy() = 0;

	UString m_sFilePath;
	UString m_sName;

	std::atomic<bool> m_bReady;
	std::atomic<bool> m_bAsyncReady;
	std::atomic<bool> m_bInterrupted;

public:
	// type inspection
	[[nodiscard]] virtual Type getResType() const = 0;

	template <typename T = Resource>
	T *as()
	{
		if constexpr (std::is_same_v<T, Resource>)
			return this;
		else if constexpr (std::is_same_v<T, Image>)
			return this->asImage();
		else if constexpr (std::is_same_v<T, McFont>)
			return this->asFont();
		else if constexpr (std::is_same_v<T, RenderTarget>)
			return this->asRenderTarget();
		else if constexpr (std::is_same_v<T, TextureAtlas>)
			return this->asTextureAtlas();
		else if constexpr (std::is_same_v<T, Shader>)
			return this->asShader();
		else if constexpr (std::is_same_v<T, VertexArrayObject>)
			return this->asVAO();
		else if constexpr (std::is_same_v<T, Sound>)
			return this->asSound();
		else if constexpr (std::is_same_v<T, const Resource>)
			return static_cast<const Resource*>(this);
		else if constexpr (std::is_same_v<T, const Image>)
			return static_cast<const Image*>(this->asImage());
		else if constexpr (std::is_same_v<T, const McFont>)
			return static_cast<const McFont*>(this->asFont());
		else if constexpr (std::is_same_v<T, const RenderTarget>)
			return static_cast<const RenderTarget*>(this->asRenderTarget());
		else if constexpr (std::is_same_v<T, const TextureAtlas>)
			return static_cast<const TextureAtlas*>(this->asTextureAtlas());
		else if constexpr (std::is_same_v<T, const Shader>)
			return static_cast<const Shader*>(this->asShader());
		else if constexpr (std::is_same_v<T, const VertexArrayObject>)
			return static_cast<const VertexArrayObject*>(this->asVAO());
		else if constexpr (std::is_same_v<T, const Sound>)
			return static_cast<const Sound*>(this->asSound());
		else
			static_assert(Env::always_false_v<T>, "unsupported type for resource");
		return nullptr;
	}
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
private:
	inline void setName(const UString &name) { m_sName = name; }
	bool m_bFileFound;
};

#endif
