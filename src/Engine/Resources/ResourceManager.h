//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		resource manager
//
// $NoKeywords: $rm
//===============================================================================//

#pragma once
#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "Engine.h"
#include "Font.h"
#include "Image.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "Sound.h"
#include "TextureAtlas.h"
#include "VertexArrayObject.h"


#include <stack>
#include <unordered_map>

class ConVar;
class Sound;
class TextureAtlas;
class AsyncResourceLoader;
class ResourceManager final
{
public:
	

	static constexpr auto PATH_DEFAULT_IMAGES = "materials/";
	static constexpr auto PATH_DEFAULT_FONTS = "fonts/";
	static constexpr auto PATH_DEFAULT_SOUNDS = "sounds/";
	static constexpr auto PATH_DEFAULT_SHADERS = "shaders/";

public:
	ResourceManager();
	~ResourceManager();

	void update();

	void loadResource(Resource *rs)
	{
		requestNextLoadUnmanaged();
		loadResource(rs, true);
	}
	void destroyResource(Resource *rs);
	void destroyResources();

	// async reload
	void reloadResource(Resource *rs, bool async = false);
	void reloadResources(const std::vector<Resource *> &resources, bool async = false);

	void requestNextLoadAsync();
	void requestNextLoadUnmanaged();

	// can't allow directly setting resource names, otherwise the map will go out of sync
	void setResourceName(Resource *res, UString name);

	// images
	Image *loadImage(UString filepath, UString resourceName, bool mipmapped = false, bool keepInSystemMemory = false);
	Image *loadImageUnnamed(UString filepath, bool mipmapped = false, bool keepInSystemMemory = false);
	Image *loadImageAbs(UString absoluteFilepath, UString resourceName, bool mipmapped = false, bool keepInSystemMemory = false);
	Image *loadImageAbsUnnamed(UString absoluteFilepath, bool mipmapped = false, bool keepInSystemMemory = false);
	Image *createImage(unsigned int width, unsigned int height, bool mipmapped = false, bool keepInSystemMemory = false);

	// fonts
	McFont *loadFont(UString filepath, UString resourceName, int fontSize = 16, bool antialiasing = true, int fontDPI = 96);
	McFont *loadFont(UString filepath, UString resourceName, std::vector<wchar_t> characters, int fontSize = 16, bool antialiasing = true, int fontDPI = 96);

	// sounds
	Sound *loadSound(UString filepath, UString resourceName, bool stream = false, bool threeD = false, bool loop = false, bool prescan = false);
	Sound *loadSoundAbs(UString filepath, UString resourceName, bool stream = false, bool threeD = false, bool loop = false, bool prescan = false);

	// shaders
	Shader *loadShader(UString shaderFilePath, UString resourceName);
	Shader *loadShader(UString shaderFilePath);
	Shader *createShader(UString shaderSource, UString resourceName);
	Shader *createShader(UString shaderSource);

	// rendertargets
	RenderTarget *createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
	RenderTarget *createRenderTarget(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);

	// texture atlas
	TextureAtlas *createTextureAtlas(int width, int height);

	// models/meshes
	VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES,
	                                           Graphics::USAGE_TYPE usage = Graphics::USAGE_TYPE::USAGE_STATIC, bool keepInSystemMemory = false);

	// resource access by name
	Image *getImage(UString resourceName) const { return tryGet<Image>(resourceName); }
	McFont *getFont(UString resourceName) const { return tryGet<McFont>(resourceName); }
	Sound *getSound(UString resourceName) const { return tryGet<Sound>(resourceName); }
	Shader *getShader(UString resourceName) const { return tryGet<Shader>(resourceName); }

	// methods for getting all resources of a type
	[[nodiscard]] constexpr const std::vector<Image *> &getImages() const { return m_vImages; }
	[[nodiscard]] constexpr const std::vector<McFont *> &getFonts() const { return m_vFonts; }
	[[nodiscard]] constexpr const std::vector<Sound *> &getSounds() const { return m_vSounds; }
	[[nodiscard]] constexpr const std::vector<Shader *> &getShaders() const { return m_vShaders; }
	[[nodiscard]] constexpr const std::vector<RenderTarget *> &getRenderTargets() const { return m_vRenderTargets; }
	[[nodiscard]] constexpr const std::vector<TextureAtlas *> &getTextureAtlases() const { return m_vTextureAtlases; }
	[[nodiscard]] constexpr const std::vector<VertexArrayObject *> &getVertexArrayObjects() const { return m_vVertexArrayObjects; }

	[[nodiscard]] constexpr const std::vector<Resource *> &getResources() const { return m_vResources; }

	[[nodiscard]] bool isLoading() const;
	bool isLoadingResource(Resource *rs) const;
	[[nodiscard]] size_t getNumLoadingWork() const;
	[[nodiscard]] size_t getNumActiveThreads() const;
	[[nodiscard]] size_t getNumLoadingWorkAsyncDestroy() const;

private:
	template <typename T>
	[[nodiscard]] T *tryGet(UString &resourceName) const
	{
		if (resourceName.isEmpty())
			return nullptr;
		auto it = m_nameToResourceMap.find(resourceName);
		if (it != m_nameToResourceMap.end())
			return it->second->as<T>();
		if (cv::debug_rm.getBool())
			debugLog(R"(ResourceManager WARNING: Resource "{:s}" does not exist!)"
			         "\n",
			         resourceName);
		return nullptr;
	}
	template <typename T>
	[[nodiscard]] T *checkIfExistsAndHandle(UString &resourceName)
	{
		if (resourceName.isEmpty())
			return nullptr;
		auto it = m_nameToResourceMap.find(resourceName);
		if (it == m_nameToResourceMap.end())
			return nullptr;
		if (cv::debug_rm.getBool())
			debugLog(R"(ResourceManager NOTICE: Resource "{:s}" already loaded.)"
			         "\n",
			         resourceName);
		// handle flags (reset them)
		resetFlags();
		return it->second->as<T>();
	}

	void loadResource(Resource *res, bool load);
	void resetFlags();

	void addManagedResource(Resource *res);
	void removeManagedResource(Resource *res, int managedResourceIndex);

	// helper methods for managing typed resource vectors
	void addResourceToTypedVector(Resource *res);
	void removeResourceFromTypedVector(Resource *res);

	// async loading system
	AsyncResourceLoader *m_asyncLoader;

	// flags
	bool m_bNextLoadAsync;
	std::stack<bool> m_nextLoadUnmanagedStack;

	// content
	std::vector<Resource *> m_vResources;

	// fast name lookup
	std::unordered_map<UString, Resource *> m_nameToResourceMap;

	// typed resource vectors for fast type-specific access
	std::vector<Image *> m_vImages;
	std::vector<McFont *> m_vFonts;
	std::vector<Sound *> m_vSounds;
	std::vector<Shader *> m_vShaders;
	std::vector<RenderTarget *> m_vRenderTargets;
	std::vector<TextureAtlas *> m_vTextureAtlases;
	std::vector<VertexArrayObject *> m_vVertexArrayObjects;
};

#endif
