//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		resource manager
//
// $NoKeywords: $rm
//===============================================================================//

#include "ResourceManager.h"

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "Thread.h"
#include "Timing.h"

#include <algorithm>
#include <mutex>

static constexpr int default_numthreads = 3;

static void *_resourceLoaderThread(void *data);

class ResourceManagerLoaderThread
{
public:
	// self
	McThread *thread{};

	// synchronization
	std::atomic<bool> running{true};

	// parent reference
	ResourceManager *resourceManager{};
	size_t threadIndex{};

	ResourceManagerLoaderThread() = default;
};

ConVar rm_numthreads("rm_numthreads", default_numthreads, FCVAR_NONE,
                     "how many parallel resource loader threads are spawned once on startup (!), and subsequently used during runtime");
ConVar rm_warnings("rm_warnings", false, FCVAR_NONE);
ConVar rm_debug_async_delay("rm_debug_async_delay", 0.0f, FCVAR_CHEAT);
ConVar rm_interrupt_on_destroy("rm_interrupt_on_destroy", true, FCVAR_CHEAT);
ConVar debug_rm_("debug_rm", false, FCVAR_NONE);

ConVar *ResourceManager::debug_rm = &debug_rm_;

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

	int threads = std::clamp(env->getLogicalCPUCount(), default_numthreads, 32); // sanity
	if (threads > default_numthreads)
		rm_numthreads.setValue(threads);

	m_iNumResourceInitPerFrameLimit = threads * 2l;

	// create loader threads
	for (int i = 0; i < threads; i++)
	{
		auto *loaderThread = new ResourceManagerLoaderThread();

		loaderThread->running = true;
		loaderThread->threadIndex = i;
		loaderThread->resourceManager = this;

		loaderThread->thread = new McThread(_resourceLoaderThread, (void *)loaderThread);
		if (!loaderThread->thread->isReady())
		{
			engine->showMessageError("ResourceManager Error", "Couldn't create thread!");
			SAFE_DELETE(loaderThread->thread);
			SAFE_DELETE(loaderThread);
		}
		else
			m_threads.push_back(loaderThread);
	}
}

ResourceManager::~ResourceManager()
{
	// release all not-currently-being-loaded resources (1)
	destroyResources();

	// let all loader threads exit
	for (auto &thread : m_threads)
	{
		thread->running = false;
	}

	// wake up all threads so they can exit
	m_workAvailable.notify_all();

	// wait for threads to stop
	for (auto &thread : m_threads)
	{
		delete thread->thread;
		delete thread;
	}

	m_threads.clear();

	// cleanup all work items
	{
		std::lock_guard<std::mutex> lock(m_allWorkMutex);
		for (auto work : m_allWork)
		{
			delete work;
		}
		m_allWork.clear();
	}

	// cleanup leftovers (can only do that after loader threads have exited) (2)
	for (auto &rs : m_loadingWorkAsyncDestroy)
	{
		delete rs;
	}
	m_loadingWorkAsyncDestroy.clear();
}

void ResourceManager::update()
{
	// process completed async work
	std::vector<LOADING_WORK *> completedWork;

	// grab all completed work at once to minimize lock time
	{
		std::lock_guard<std::mutex> lock(m_asyncCompleteWorkMutex);
		while (!m_asyncCompleteWork.empty())
		{
			completedWork.push_back(m_asyncCompleteWork.front());
			m_asyncCompleteWork.pop();
		}
	}

	// process completed work (synchronous init)
	size_t numProcessed = 0;
	for (auto work : completedWork)
	{
		if (!work->syncDone.load() && work->asyncDone.load())
		{
			Resource *rs = work->resource;

			if (debug_rm->getBool())
				debugLog("Resource Manager: Sync init for {:s}\n", rs->getName().toUtf8());

			// synchronous init on main thread
			rs->load();
			work->syncDone = true;

			numProcessed++;

			// only allow set number of work items to finish per frame (avoid stutters)
			if (m_iNumResourceInitPerFrameLimit > 0 && numProcessed >= m_iNumResourceInitPerFrameLimit)
			{
				// put remaining work back in the queue
				if (numProcessed < completedWork.size())
				{
					std::lock_guard<std::mutex> lock(m_asyncCompleteWorkMutex);
					for (size_t i = numProcessed; i < completedWork.size(); i++)
					{
						m_asyncCompleteWork.push(completedWork[i]);
					}
				}
				break;
			}
		}
	}

	// cleanup fully completed work
	{
		std::lock_guard<std::mutex> lock(m_allWorkMutex);
		m_allWork.erase(std::remove_if(m_allWork.begin(), m_allWork.end(), // this formatting is messed up
		                               [](LOADING_WORK *work) {
			                               if (work->syncDone.load())
			                               {
				                               delete work;
				                               return true;
			                               }
			                               return false;
		                               }),
		                m_allWork.end());
	}

	// process async destroy queue
	std::vector<Resource *> resourcesReadyForDestroy;

	{
		std::lock_guard<std::mutex> lock(m_asyncDestroyMutex);
		for (size_t i = 0; i < m_loadingWorkAsyncDestroy.size(); i++)
		{
			bool canBeDestroyed = true;

			// check if resource is still being processed
			{
				std::lock_guard<std::mutex> workLock(m_allWorkMutex);
				for (auto work : m_allWork)
				{
					if (work->resource == m_loadingWorkAsyncDestroy[i])
					{
						canBeDestroyed = false;
						break;
					}
				}
			}

			if (canBeDestroyed)
			{
				resourcesReadyForDestroy.push_back(m_loadingWorkAsyncDestroy[i]);
				m_loadingWorkAsyncDestroy.erase(m_loadingWorkAsyncDestroy.begin() + i);
				i--;
			}
		}
	}

	// don't need to hold the lock to destroy the resource
	for (Resource *rs : resourcesReadyForDestroy)
	{
		if (debug_rm->getBool())
			debugLog("Resource Manager: Async destroy of resource {:s}\n", rs->getName().toUtf8());

		delete rs; // implicitly calls release() through the Resource destructor
	}
}

void ResourceManager::destroyResources()
{
	while (m_vResources.size() > 0)
	{
		destroyResource(m_vResources[0]);
	}
	m_vResources.clear();
	m_nameToResourceMap.clear();
	m_vImages.clear();
	m_vFonts.clear();
	m_vSounds.clear();
	m_vShaders.clear();
	m_vRenderTargets.clear();
	m_vTextureAtlases.clear();
	m_vVertexArrayObjects.clear();
}

void ResourceManager::destroyResource(Resource *rs)
{
	if (rs == NULL)
	{
		if (rm_warnings.getBool())
			debugLog("RESOURCE MANAGER Warning: destroyResource(NULL)!\n");
		return;
	}

	if (debug_rm->getBool())
		debugLog("ResourceManager: Destroying {:s}\n", rs->getName().toUtf8());

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
	bool isBeingLoaded = false;
	{
		std::lock_guard<std::mutex> lock(m_allWorkMutex);
		for (auto work : m_allWork)
		{
			if (work->resource == rs)
			{
				isBeingLoaded = true;
				break;
			}
		}
	}

	if (isBeingLoaded)
	{
		if (debug_rm->getBool())
			debugLog("Resource Manager: Scheduled async destroy of {:s}\n", rs->getName().toUtf8());

		if (rm_interrupt_on_destroy.getBool())
			rs->interruptLoad();

		std::lock_guard<std::mutex> lock(m_asyncDestroyMutex);
		m_loadingWorkAsyncDestroy.push_back(rs);

		if (isManagedResource)
			removeManagedResource(rs, managedResourceIndex);

		return;
	}

	// standard destroy
	if (isManagedResource)
		removeManagedResource(rs, managedResourceIndex);

	SAFE_DELETE(rs);
}

void ResourceManager::requestNextLoadAsync()
{
	m_bNextLoadAsync = true;
}

void ResourceManager::requestNextLoadUnmanaged()
{
	m_nextLoadUnmanagedStack.push(true);
}

Image *ResourceManager::loadImage(UString filepath, UString resourceName, bool mipmapped, bool keepInSystemMemory)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asImage();
	}

	// create instance and load it
	filepath.insert(0, PATH_DEFAULT_IMAGES);
	Image *img = graphics->createImage(filepath, mipmapped, keepInSystemMemory);
	img->setName(resourceName);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageUnnamed(UString filepath, bool mipmapped, bool keepInSystemMemory)
{
	filepath.insert(0, PATH_DEFAULT_IMAGES);
	Image *img = graphics->createImage(filepath, mipmapped, keepInSystemMemory);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageAbs(UString absoluteFilepath, UString resourceName, bool mipmapped, bool keepInSystemMemory)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asImage();
	}

	// create instance and load it
	Image *img = graphics->createImage(absoluteFilepath, mipmapped, keepInSystemMemory);
	img->setName(resourceName);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageAbsUnnamed(UString absoluteFilepath, bool mipmapped, bool keepInSystemMemory)
{
	Image *img = graphics->createImage(absoluteFilepath, mipmapped, keepInSystemMemory);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::createImage(unsigned int width, unsigned int height, bool mipmapped, bool keepInSystemMemory)
{
	if (width > 8192 || height > 8192)
	{
		engine->showMessageError("Resource Manager Error", UString::format("Invalid parameters in createImage(%i, %i, %i)!\n", width, height, (int)mipmapped));
		return NULL;
	}

	Image *img = graphics->createImage(width, height, mipmapped, keepInSystemMemory);

	loadResource(img, false);

	return img;
}

McFont *ResourceManager::loadFont(UString filepath, UString resourceName, int fontSize, bool antialiasing, int fontDPI)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asFont();
	}

	// create instance and load it
	filepath.insert(0, PATH_DEFAULT_FONTS);
	auto *fnt = new McFont(filepath, fontSize, antialiasing, fontDPI);
	fnt->setName(resourceName);

	loadResource(fnt, true);

	return fnt;
}

McFont *ResourceManager::loadFont(UString filepath, UString resourceName, std::vector<wchar_t> characters, int fontSize, bool antialiasing, int fontDPI)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asFont();
	}

	// create instance and load it
	filepath.insert(0, PATH_DEFAULT_FONTS);
	auto *fnt = new McFont(filepath, characters, fontSize, antialiasing, fontDPI);
	fnt->setName(resourceName);

	loadResource(fnt, true);

	return fnt;
}

Sound *ResourceManager::loadSound(UString filepath, UString resourceName, bool stream, bool threeD, bool loop, bool prescan)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asSound();
	}

	// create instance and load it
	filepath.insert(0, PATH_DEFAULT_SOUNDS);
	Sound *snd = Sound::createSound(filepath, stream, threeD, loop, prescan);
	snd->setName(resourceName);

	loadResource(snd, true);

	return snd;
}

Sound *ResourceManager::loadSoundAbs(UString filepath, UString resourceName, bool stream, bool threeD, bool loop, bool prescan)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asSound();
	}

	// create instance and load it
	Sound *snd = Sound::createSound(filepath, stream, threeD, loop, prescan);
	snd->setName(resourceName);

	loadResource(snd, true);

	return snd;
}

Shader *ResourceManager::loadShader(UString vertexShaderFilePath, UString fragmentShaderFilePath, UString resourceName)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asShader();
	}

	// create instance and load it
	vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = graphics->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::loadShader(UString vertexShaderFilePath, UString fragmentShaderFilePath)
{
	vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = graphics->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader(UString vertexShader, UString fragmentShader, UString resourceName)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asShader();
	}

	// create instance and load it
	Shader *shader = graphics->createShaderFromSource(vertexShader, fragmentShader);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader(UString vertexShader, UString fragmentShader)
{
	Shader *shader = graphics->createShaderFromSource(vertexShader, fragmentShader);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::loadShader2(UString shaderFilePath, UString resourceName)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asShader();
	}

	// create instance and load it
	shaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = graphics->createShaderFromFile(shaderFilePath);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::loadShader2(UString shaderFilePath)
{
	shaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = graphics->createShaderFromFile(shaderFilePath);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader2(UString shaderSource, UString resourceName)
{
	// check if it already exists
	if (resourceName.length() > 0)
	{
		Resource *temp = checkIfExistsAndHandle(resourceName);
		if (temp != NULL)
			return temp->asShader();
	}

	// create instance and load it
	Shader *shader = graphics->createShaderFromSource(shaderSource);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader2(UString shaderSource)
{
	Shader *shader = graphics->createShaderFromSource(shaderSource);

	loadResource(shader, true);

	return shader;
}

RenderTarget *ResourceManager::createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	RenderTarget *rt = graphics->createRenderTarget(x, y, width, height, multiSampleType);
	rt->setName(UString::format("_RT_%ix%i", width, height));

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
	ta->setName(UString::format("_TA_%ix%i", width, height));

	loadResource(ta, false);

	return ta;
}

VertexArrayObject *ResourceManager::createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory)
{
	VertexArrayObject *vao = graphics->createVertexArrayObject(primitive, usage, keepInSystemMemory);

	loadResource(vao, false);

	return vao;
}

Image *ResourceManager::getImage(UString resourceName) const
{
	auto it = m_nameToResourceMap.find(resourceName);
	if (it != m_nameToResourceMap.end())
		return it->second->asImage();

	doesntExistWarning(resourceName);
	return NULL;
}

McFont *ResourceManager::getFont(UString resourceName) const
{
	auto it = m_nameToResourceMap.find(resourceName);
	if (it != m_nameToResourceMap.end())
		return it->second->asFont();

	doesntExistWarning(resourceName);
	return NULL;
}

Sound *ResourceManager::getSound(UString resourceName) const
{
	auto it = m_nameToResourceMap.find(resourceName);
	if (it != m_nameToResourceMap.end())
		return it->second->asSound();

	doesntExistWarning(resourceName);
	return NULL;
}

Shader *ResourceManager::getShader(UString resourceName) const
{
	auto it = m_nameToResourceMap.find(resourceName);
	if (it != m_nameToResourceMap.end())
		return it->second->asShader();

	doesntExistWarning(resourceName);
	return NULL;
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
		if (rm_numthreads.getInt() > 0)
		{
			// create a work item for the resource
			auto *work = new LOADING_WORK();
			work->resource = res;
			work->workId = m_workIdCounter++;
			work->asyncDone = false;
			work->syncDone = false;

			// add it to the work queues
			{
				std::lock_guard<std::mutex> lock(m_allWorkMutex);
				m_allWork.push_back(work);
			}

			{
				std::lock_guard<std::mutex> lock(m_pendingWorkMutex);
				m_pendingWork.push(work);
			}

			// notify worker threads of available work
			m_workAvailable.notify_one();
		}
		else
		{
			// load normally (threading disabled)
			res->loadAsync();
			res->load();
		}
	}
}

void ResourceManager::reloadResource(Resource *rs, bool async)
{
	if (rs == NULL)
	{
		if (rm_warnings.getBool())
			debugLog("RESOURCE MANAGER Warning: reloadResource(NULL)!\n");
		return;
	}

	const std::vector<Resource *> resourceToReload{rs};
	reloadResources(resourceToReload, async);
}

void ResourceManager::reloadResources(const std::vector<Resource *> &resources, bool async)
{
	if (resources.empty())
	{
		if (rm_warnings.getBool())
			debugLog("RESOURCE MANAGER Warning: reloadResources with an empty resources vector!\n");
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

	if (debug_rm->getBool())
		debugLog("ResourceManager: Async reloading {} resources\n", resources.size());

	// first pass: release all resources that aren't currently being loaded
	std::vector<Resource *> resourcesToReload;
	for (Resource *rs : resources)
	{
		if (rs == NULL)
			continue;

		if (debug_rm->getBool())
			debugLog("ResourceManager: Async reloading {:s}\n", rs->getName().toUtf8());

		// check if resource is currently being loaded
		bool isBeingLoaded = false;
		{
			std::lock_guard<std::mutex> lock(m_allWorkMutex);
			for (auto work : m_allWork)
			{
				if (work->resource == rs)
				{
					isBeingLoaded = true;
					break;
				}
			}
		}

		if (!isBeingLoaded)
		{
			rs->release();
			resourcesToReload.push_back(rs);
		}
		else if (debug_rm->getBool())
		{
			debugLog("Resource Manager: Resource {:s} is currently being loaded, skipping reload\n", rs->getName().toUtf8());
		}
	}

	// second pass: queue all resources for async reload
	for (Resource *rs : resourcesToReload)
	{
		// mark as unmanaged to prevent re-adding to managed resources
		requestNextLoadUnmanaged();
		requestNextLoadAsync();
		loadResource(rs, true);
	}
}

void ResourceManager::doesntExistWarning(UString resourceName) const
{
	if (rm_warnings.getBool())
	{
		UString errormsg = "Resource \"";
		errormsg.append(resourceName);
		errormsg.append("\" does not exist!");
		engine->showMessageWarning("RESOURCE MANAGER: ", errormsg);
	}
}

Resource *ResourceManager::checkIfExistsAndHandle(UString resourceName)
{
	auto it = m_nameToResourceMap.find(resourceName);
	if (it != m_nameToResourceMap.end())
	{
		if (rm_warnings.getBool())
			debugLog("RESOURCE MANAGER: Resource \"{:s}\" already loaded!\n", resourceName.toUtf8());

		// handle flags (reset them)
		resetFlags();

		return it->second;
	}

	return NULL;
}

void ResourceManager::resetFlags()
{
	if (m_nextLoadUnmanagedStack.size() > 0)
		m_nextLoadUnmanagedStack.pop();

	m_bNextLoadAsync = false;
}

// add a managed resource to the main resources vector + the name map and typed vectors
void ResourceManager::addManagedResource(Resource *res)
{
	if (!res)
		return;

	m_vResources.push_back(res);

	if (res->getName().length() > 0)
		m_nameToResourceMap[res->getName()] = res;
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
		// TODO: app-defined types aren't added to specific vectors
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
		// TODO: app-defined types aren't added to specific vectors
		break;
	}
}

// main async stuff below (TO BE MOVED)

bool ResourceManager::isLoading() const
{
	return getNumLoadingWork() > 0;
}

bool ResourceManager::isLoadingResource(Resource *rs) const
{
	std::lock_guard<std::mutex> lock(m_allWorkMutex);
	for (auto work : m_allWork)
	{
		if (work->resource == rs)
			return true;
	}

	return false;
}

ResourceManager::LOADING_WORK *ResourceManager::getNextWork()
{
	std::lock_guard<std::mutex> lock(m_pendingWorkMutex);
	if (m_pendingWork.empty())
		return nullptr;

	LOADING_WORK *work = m_pendingWork.front();
	m_pendingWork.pop();
	return work;
}

void ResourceManager::markWorkAsyncComplete(LOADING_WORK *work)
{
	std::lock_guard<std::mutex> lock(m_asyncCompleteWorkMutex);
	m_asyncCompleteWork.push(work);
}

size_t ResourceManager::getNumLoadingWork() const
{
	std::lock_guard<std::mutex> lock(m_allWorkMutex);
	return m_allWork.size();
}

static void *_resourceLoaderThread(void *data)
{
	auto *self = static_cast<ResourceManagerLoaderThread *>(data);
	ResourceManager *manager = self->resourceManager;
	const size_t threadIndex = self->threadIndex;

	while (self->running.load())
	{
		// try to get work
		ResourceManager::LOADING_WORK *work = manager->getNextWork();

		// if no work available, wait for notification
		if (!work)
		{
			std::unique_lock<std::mutex> lock(manager->m_workAvailableMutex);
			manager->m_workAvailable.wait_for(lock, std::chrono::milliseconds(50), [&]() { return !self->running.load() || manager->getNumLoadingWork() > 0; });

			// check if we should exit
			if (!self->running.load())
				break;

			continue;
		}

		// we have work, process it
		Resource *resource = work->resource;

		if (ResourceManager::debug_rm->getBool())
			debugLog("Resource Manager: Thread #{} loading {:s}\n", threadIndex, resource->getName().toUtf8());

		// debug pause
		if (rm_debug_async_delay.getFloat() > 0.0f)
			Timing::sleep(rm_debug_async_delay.getFloat() * 1000 * 1000);

		// async load
		resource->loadAsync();

		// mark as async complete
		work->asyncDone = true;
		manager->markWorkAsyncComplete(work);

		if (ResourceManager::debug_rm->getBool())
			debugLog("Resource Manager: Thread #{} finished async loading {:s}\n", threadIndex, resource->getName().toUtf8());
	}

	return nullptr;
}
