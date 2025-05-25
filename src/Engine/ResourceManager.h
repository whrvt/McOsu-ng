//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		resource manager
//
// $NoKeywords: $rm
//===============================================================================//

#pragma once
#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "Font.h"
#include "Image.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "Sound.h"
#include "TextureAtlas.h"
#include "VertexArrayObject.h"

#include <atomic>
#include <condition_variable>
#include <queue>
#include <unordered_map>

class ConVar;

class ResourceManagerLoaderThread;

class ResourceManager
{
public:
	static ConVar *debug_rm;

	static constexpr auto PATH_DEFAULT_IMAGES = "materials/";
	static constexpr auto PATH_DEFAULT_FONTS = "fonts/";
	static constexpr auto PATH_DEFAULT_SOUNDS = "sounds/";
	static constexpr auto PATH_DEFAULT_SHADERS = "shaders/";

// async loading methods
// TODO: move the actual resource loading to a separate file, it's getting messy in here
public:
	template <typename T>
	struct MobileAtomic
	{
		std::atomic<T> atomic;

		MobileAtomic() : atomic(T()) {}

		explicit MobileAtomic(T const &v) : atomic(v) {}
		explicit MobileAtomic(std::atomic<T> const &a) : atomic(a.load()) {}

		MobileAtomic(MobileAtomic const &other) : atomic(other.atomic.load()) {}

		MobileAtomic &operator=(MobileAtomic const &other)
		{
			atomic.store(other.atomic.load());
			return *this;
		}
	};
	typedef MobileAtomic<bool> MobileAtomicBool;
	typedef MobileAtomic<size_t> MobileAtomicSizeT;
	typedef MobileAtomic<Resource *> MobileAtomicResource;

	struct LOADING_WORK
	{
		Resource *resource;
		size_t workId;
		std::atomic<bool> asyncDone{false};
		std::atomic<bool> syncDone{false};
	};

	// work queue methods
	LOADING_WORK *getNextWork();
	void markWorkAsyncComplete(LOADING_WORK *work);

	// work notification
	std::condition_variable m_workAvailable;
	std::mutex m_workAvailableMutex;

	[[nodiscard]] inline size_t getNumThreads() const { return m_threads.size(); }
	[[nodiscard]] size_t getNumLoadingWork() const;
	[[nodiscard]] inline size_t getNumLoadingWorkAsyncDestroy() const { return m_loadingWorkAsyncDestroy.size(); }

	// flags
	bool m_bNextLoadAsync;
	std::stack<bool> m_nextLoadUnmanagedStack;
	size_t m_iNumResourceInitPerFrameLimit;

	// async work queue threads
	std::vector<ResourceManagerLoaderThread *> m_threads;

	// separate queues for different stages of loading
	std::queue<LOADING_WORK *> m_pendingWork;       // work waiting to be picked up
	std::queue<LOADING_WORK *> m_asyncCompleteWork; // work that completed async phase
	std::vector<LOADING_WORK *> m_allWork;          // all work items for cleanup

	// fine-grained locks for the above
	mutable std::mutex m_pendingWorkMutex;
	mutable std::mutex m_asyncCompleteWorkMutex;
	mutable std::mutex m_allWorkMutex;

	std::atomic<size_t> m_workIdCounter{0};

	// async destroy queue
	std::vector<Resource *> m_loadingWorkAsyncDestroy;
	std::mutex m_asyncDestroyMutex;

public:
	ResourceManager();
	~ResourceManager();

	void update();

	void setNumResourceInitPerFrameLimit(size_t numResourceInitPerFrameLimit) { m_iNumResourceInitPerFrameLimit = numResourceInitPerFrameLimit; }

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

	// shaders (DEPRECATED)
	Shader *loadShader(UString vertexShaderFilePath, UString fragmentShaderFilePath, UString resourceName); // DEPRECATED
	Shader *loadShader(UString vertexShaderFilePath, UString fragmentShaderFilePath);                       // DEPRECATED
	Shader *createShader(UString vertexShader, UString fragmentShader, UString resourceName);               // DEPRECATED
	Shader *createShader(UString vertexShader, UString fragmentShader);                                     // DEPRECATED

	// shaders
	Shader *loadShader2(UString shaderFilePath, UString resourceName);
	Shader *loadShader2(UString shaderFilePath);
	Shader *createShader2(UString shaderSource, UString resourceName);
	Shader *createShader2(UString shaderSource);

	// rendertargets
	RenderTarget *createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);
	RenderTarget *createRenderTarget(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);

	// texture atlas
	TextureAtlas *createTextureAtlas(int width, int height);

	// models/meshes
	VertexArrayObject *createVertexArrayObject(Graphics::PRIMITIVE primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES,
	                                           Graphics::USAGE_TYPE usage = Graphics::USAGE_TYPE::USAGE_STATIC, bool keepInSystemMemory = false);

	// resource access by name
	Image *getImage(UString resourceName) const;
	McFont *getFont(UString resourceName) const;
	Sound *getSound(UString resourceName) const;
	Shader *getShader(UString resourceName) const;

	// methods for getting all resources of a type
	[[nodiscard]] constexpr const std::vector<Image *> &getImages() const { return m_vImages; }
	[[nodiscard]] constexpr const std::vector<McFont *> &getFonts() const { return m_vFonts; }
	[[nodiscard]] constexpr const std::vector<Sound *> &getSounds() const { return m_vSounds; }
	[[nodiscard]] constexpr const std::vector<Shader *> &getShaders() const { return m_vShaders; }
	[[nodiscard]] constexpr const std::vector<RenderTarget *> &getRenderTargets() const { return m_vRenderTargets; }
	[[nodiscard]] constexpr const std::vector<TextureAtlas *> &getTextureAtlases() const { return m_vTextureAtlases; }
	[[nodiscard]] constexpr const std::vector<VertexArrayObject *> &getVertexArrayObjects() const { return m_vVertexArrayObjects; }

	[[nodiscard]] constexpr const std::vector<Resource *> &getResources() const { return m_vResources; }

	bool isLoading() const;
	bool isLoadingResource(Resource *rs) const;

private:
	void loadResource(Resource *res, bool load);
	void doesntExistWarning(UString resourceName) const;
	Resource *checkIfExistsAndHandle(UString resourceName);

	void resetFlags();

	void addManagedResource(Resource *res);
	void removeManagedResource(Resource *res, int managedResourceIndex);

	// helper methods for managing typed resource vectors
	void addResourceToTypedVector(Resource *res);
	void removeResourceFromTypedVector(Resource *res);

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
