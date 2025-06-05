//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		resource manager
//
// $NoKeywords: $rm
//===============================================================================//

#include "ResourceManager.h"
#include "AsyncResourceLoader.h"
#include "Resource.h"

#include "App.h"
#include "ConVar.h"
#include "Environment.h"

#include <algorithm>
namespace cv {
ConVar rm_interrupt_on_destroy("rm_interrupt_on_destroy", true, FCVAR_CHEAT);
ConVar debug_rm("debug_rm", false, FCVAR_NONE);
}

ResourceManager::ResourceManager()
{
	m_bNextLoadAsync = false;

	// reserve space for typed vectors
	m_vImages.reserve(64);
	m_vFonts.reserve(16);
	m_vSounds.reserve(64);
	m_vShaders.reserve(32);
	m_vRenderTargets.reserve(8);
	m_vTextureAtlases.reserve(8);
	m_vVertexArrayObjects.reserve(32);

	// create async loader
	m_asyncLoader = new AsyncResourceLoader();
}

ResourceManager::~ResourceManager()
{
	// release all not-currently-being-loaded resources
	destroyResources();

	// shutdown async loader (handles thread cleanup)
	delete m_asyncLoader;
}

void ResourceManager::update()
{
	// delegate to async loader
	bool lowLatency = app && app->isInCriticalInteractiveSession();
	m_asyncLoader->update(lowLatency);
}

void ResourceManager::destroyResources()
{
	while (m_vResources.size() > 0)
	{
		destroyResource(m_vResources[0]);
	}
	m_vResources.clear();
	m_vImages.clear();
	m_vFonts.clear();
	m_vSounds.clear();
	m_vShaders.clear();
	m_vRenderTargets.clear();
	m_vTextureAtlases.clear();
	m_vVertexArrayObjects.clear();
	m_nameToResourceMap.clear();
}

void ResourceManager::destroyResource(Resource *rs)
{
	if (rs == nullptr)
	{
		if (cv::debug_rm.getBool())
			debugLog("ResourceManager Warning: destroyResource(NULL)!\n");
		return;
	}

	if (cv::debug_rm.getBool())
		debugLog("ResourceManager: Destroying {:s}\n", rs->getName());

	bool isManagedResource = false;
	int managedResourceIndex = -1;
	for (size_t i = 0; i < m_vResources.size(); i++)
	{
		if (m_vResources[i] == rs)
		{
			isManagedResource = true;
			managedResourceIndex = i;
			break;
		}
	}

	// check if it's being loaded and schedule async destroy if so
	if (m_asyncLoader->isLoadingResource(rs))
	{
		if (cv::debug_rm.getBool())
			debugLog("Resource Manager: Scheduled async destroy of {:s}\n", rs->getName());

		if (cv::rm_interrupt_on_destroy.getBool())
			rs->interruptLoad();

		m_asyncLoader->scheduleAsyncDestroy(rs);

		if (isManagedResource)
			removeManagedResource(rs, managedResourceIndex);

		return;
	}

	// standard destroy
	if (isManagedResource)
		removeManagedResource(rs, managedResourceIndex);

	SAFE_DELETE(rs);
}

void ResourceManager::loadResource(Resource *res, bool load)
{
	// handle flags
	const bool isManaged = (m_nextLoadUnmanagedStack.size() < 1 || !m_nextLoadUnmanagedStack.top());
	if (isManaged)
		addManagedResource(res);

	const bool isNextLoadAsync = m_bNextLoadAsync;

	// flags must be reset on every load, to not carry over
	resetFlags();

	if (!load)
		return;

	if (!isNextLoadAsync)
	{
		// load normally
		res->loadAsync();
		res->load();
	}
	else
	{
		// delegate to async loader
		m_asyncLoader->requestAsyncLoad(res);
	}
}

bool ResourceManager::isLoading() const
{
	return m_asyncLoader->isLoading();
}

bool ResourceManager::isLoadingResource(Resource *rs) const
{
	return m_asyncLoader->isLoadingResource(rs);
}

size_t ResourceManager::getNumLoadingWork() const
{
	return m_asyncLoader->getNumLoadingWork();
}

size_t ResourceManager::getNumActiveThreads() const
{
	return m_asyncLoader->getNumActiveThreads();
}

size_t ResourceManager::getNumLoadingWorkAsyncDestroy() const
{
	return m_asyncLoader->getNumLoadingWorkAsyncDestroy();
}

void ResourceManager::resetFlags()
{
	if (m_nextLoadUnmanagedStack.size() > 0)
		m_nextLoadUnmanagedStack.pop();

	m_bNextLoadAsync = false;
}

void ResourceManager::requestNextLoadAsync()
{
	m_bNextLoadAsync = true;
}

void ResourceManager::requestNextLoadUnmanaged()
{
	m_nextLoadUnmanagedStack.push(true);
}

void ResourceManager::reloadResource(Resource *rs, bool async)
{
	if (rs == nullptr)
	{
		if (cv::debug_rm.getBool())
			debugLog("ResourceManager Warning: reloadResource(NULL)!\n");
		return;
	}

	const std::vector<Resource *> resourceToReload{rs};
	reloadResources(resourceToReload, async);
}

void ResourceManager::reloadResources(const std::vector<Resource *> &resources, bool async)
{
	if (resources.empty())
	{
		if (cv::debug_rm.getBool())
			debugLog("ResourceManager Warning: reloadResources with an empty resources vector!\n");
		return;
	}

	if (!async) // synchronous
	{
		for (auto &res : resources)
		{
			res->reload();
		}
		return;
	}

	// delegate to async loader
	m_asyncLoader->reloadResources(resources);
}

void ResourceManager::setResourceName(Resource *res, UString name)
{
	if (!res)
	{
		if (cv::debug_rm.getBool())
			debugLog("ResourceManager: attempted to set name {:s} on NULL resource!\n", name);
		return;
	}

	UString currentName = res->getName();
	if (!currentName.isEmpty() && currentName == name)
		return; // it's already the same name, nothing to do

	if (name.isEmpty()) // add a default name (mostly for debugging, see Resource constructor)
		name = UString::fmt("{:p}:postinit=y:found={}:{:s}", static_cast<const void *>(res), res->m_bFileFound, res->getFilePath());

	res->setName(name);
	// add the new name to the resource map (if it's a managed resource)
	if (m_nextLoadUnmanagedStack.size() < 1 || !m_nextLoadUnmanagedStack.top())
		m_nameToResourceMap.try_emplace(name, res);
	return;
}

Image *ResourceManager::loadImage(UString filepath, UString resourceName, bool mipmapped, bool keepInSystemMemory)
{
	auto res = checkIfExistsAndHandle<Image>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	filepath.insert(0, ResourceManager::PATH_DEFAULT_IMAGES);
	Image *img = g->createImage(filepath, mipmapped, keepInSystemMemory);
	setResourceName(img, resourceName);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageUnnamed(UString filepath, bool mipmapped, bool keepInSystemMemory)
{
	filepath.insert(0, ResourceManager::PATH_DEFAULT_IMAGES);
	Image *img = g->createImage(filepath, mipmapped, keepInSystemMemory);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageAbs(UString absoluteFilepath, UString resourceName, bool mipmapped, bool keepInSystemMemory)
{
	auto res = checkIfExistsAndHandle<Image>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	Image *img = g->createImage(absoluteFilepath, mipmapped, keepInSystemMemory);
	setResourceName(img, resourceName);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageAbsUnnamed(UString absoluteFilepath, bool mipmapped, bool keepInSystemMemory)
{
	Image *img = g->createImage(absoluteFilepath, mipmapped, keepInSystemMemory);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::createImage(unsigned int width, unsigned int height, bool mipmapped, bool keepInSystemMemory)
{
	if (width > 8192 || height > 8192)
	{
		engine->showMessageError("Resource Manager Error", UString::format("Invalid parameters in createImage(%i, %i, %i)!\n", width, height, (int)mipmapped));
		return nullptr;
	}

	Image *img = g->createImage(width, height, mipmapped, keepInSystemMemory);

	loadResource(img, false);

	return img;
}

McFont *ResourceManager::loadFont(UString filepath, UString resourceName, int fontSize, bool antialiasing, int fontDPI)
{
	auto res = checkIfExistsAndHandle<McFont>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	filepath.insert(0, ResourceManager::PATH_DEFAULT_FONTS);
	auto *fnt = new McFont(filepath, fontSize, antialiasing, fontDPI);
	setResourceName(fnt, resourceName);

	loadResource(fnt, true);

	return fnt;
}

McFont *ResourceManager::loadFont(UString filepath, UString resourceName, std::vector<wchar_t> characters, int fontSize, bool antialiasing, int fontDPI)
{
	auto res = checkIfExistsAndHandle<McFont>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	filepath.insert(0, ResourceManager::PATH_DEFAULT_FONTS);
	auto *fnt = new McFont(filepath, characters, fontSize, antialiasing, fontDPI);
	setResourceName(fnt, resourceName);

	loadResource(fnt, true);

	return fnt;
}

Sound *ResourceManager::loadSound(UString filepath, UString resourceName, bool stream, bool threeD, bool loop, bool prescan)
{
	auto res = checkIfExistsAndHandle<Sound>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	filepath.insert(0, ResourceManager::PATH_DEFAULT_SOUNDS);
	Sound *snd = Sound::createSound(filepath, stream, threeD, loop, prescan);
	setResourceName(snd, resourceName);

	loadResource(snd, true);

	return snd;
}

Sound *ResourceManager::loadSoundAbs(UString filepath, UString resourceName, bool stream, bool threeD, bool loop, bool prescan)
{
	auto res = checkIfExistsAndHandle<Sound>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	Sound *snd = Sound::createSound(filepath, stream, threeD, loop, prescan);
	setResourceName(snd, resourceName);

	loadResource(snd, true);

	return snd;
}

Shader *ResourceManager::loadShader(UString shaderFilePath, UString resourceName)
{
	auto res = checkIfExistsAndHandle<Shader>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	shaderFilePath.insert(0, ResourceManager::PATH_DEFAULT_SHADERS);
	Shader *shader = g->createShaderFromFile(shaderFilePath);
	setResourceName(shader, resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::loadShader(UString shaderFilePath)
{
	shaderFilePath.insert(0, ResourceManager::PATH_DEFAULT_SHADERS);
	Shader *shader = g->createShaderFromFile(shaderFilePath);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader(UString shaderSource, UString resourceName)
{
	auto res = checkIfExistsAndHandle<Shader>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	Shader *shader = g->createShaderFromSource(shaderSource);
	setResourceName(shader, resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader(UString shaderSource)
{
	Shader *shader = g->createShaderFromSource(shaderSource);

	loadResource(shader, true);

	return shader;
}

RenderTarget *ResourceManager::createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	RenderTarget *rt = g->createRenderTarget(x, y, width, height, multiSampleType);
	setResourceName(rt, UString::format("_RT_%ix%i", width, height));

	loadResource(rt, true);

	return rt;
}

RenderTarget *ResourceManager::createRenderTarget(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	return createRenderTarget(0, 0, width, height, multiSampleType);
}

TextureAtlas *ResourceManager::createTextureAtlas(int width, int height)
{
	auto *ta = new TextureAtlas(width, height);
	setResourceName(ta, UString::format("_TA_%ix%i", width, height));

	loadResource(ta, false);

	return ta;
}

VertexArrayObject *ResourceManager::createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory)
{
	VertexArrayObject *vao = g->createVertexArrayObject(primitive, usage, keepInSystemMemory);

	loadResource(vao, false);

	return vao;
}

// add a managed resource to the main resources vector + the name map and typed vectors
void ResourceManager::addManagedResource(Resource *res)
{
	if (!res)
		return;

	m_vResources.push_back(res);

	if (res->getName().length() > 0)
		m_nameToResourceMap.try_emplace(res->getName(), res);
	addResourceToTypedVector(res);
}

// remove a managed resource from the main resources vector + the name map and typed vectors
void ResourceManager::removeManagedResource(Resource *res, int managedResourceIndex)
{
	if (!res)
		return;

	m_vResources.erase(m_vResources.begin() + managedResourceIndex);

	if (res->getName().length() > 0)
		m_nameToResourceMap.erase(res->getName());
	removeResourceFromTypedVector(res);
}

void ResourceManager::addResourceToTypedVector(Resource *res)
{
	if (!res)
		return;

	switch (res->getResType())
	{
	case Resource::Type::IMAGE:
		m_vImages.push_back(res->asImage());
		break;
	case Resource::Type::FONT:
		m_vFonts.push_back(res->asFont());
		break;
	case Resource::Type::SOUND:
		m_vSounds.push_back(res->asSound());
		break;
	case Resource::Type::SHADER:
		m_vShaders.push_back(res->asShader());
		break;
	case Resource::Type::RENDERTARGET:
		m_vRenderTargets.push_back(res->asRenderTarget());
		break;
	case Resource::Type::TEXTUREATLAS:
		m_vTextureAtlases.push_back(res->asTextureAtlas());
		break;
	case Resource::Type::VAO:
		m_vVertexArrayObjects.push_back(res->asVAO());
		break;
	case Resource::Type::APPDEFINED:
		// app-defined types aren't added to specific vectors
		break;
	}
}

void ResourceManager::removeResourceFromTypedVector(Resource *res)
{
	if (!res)
		return;

	switch (res->getResType())
	{
	case Resource::Type::IMAGE: {
		auto it = std::ranges::find(m_vImages, res);
		if (it != m_vImages.end())
			m_vImages.erase(it);
	}
	break;
	case Resource::Type::FONT: {
		auto it = std::ranges::find(m_vFonts, res);
		if (it != m_vFonts.end())
			m_vFonts.erase(it);
	}
	break;
	case Resource::Type::SOUND: {
		auto it = std::ranges::find(m_vSounds, res);
		if (it != m_vSounds.end())
			m_vSounds.erase(it);
	}
	break;
	case Resource::Type::SHADER: {
		auto it = std::ranges::find(m_vShaders, res);
		if (it != m_vShaders.end())
			m_vShaders.erase(it);
	}
	break;
	case Resource::Type::RENDERTARGET: {
		auto it = std::ranges::find(m_vRenderTargets, res);
		if (it != m_vRenderTargets.end())
			m_vRenderTargets.erase(it);
	}
	break;
	case Resource::Type::TEXTUREATLAS: {
		auto it = std::ranges::find(m_vTextureAtlases, res);
		if (it != m_vTextureAtlases.end())
			m_vTextureAtlases.erase(it);
	}
	break;
	case Resource::Type::VAO: {
		auto it = std::ranges::find(m_vVertexArrayObjects, res);
		if (it != m_vVertexArrayObjects.end())
			m_vVertexArrayObjects.erase(it);
	}
	break;
	case Resource::Type::APPDEFINED:
		// app-defined types aren't added to specific vectors
		break;
	}
}
