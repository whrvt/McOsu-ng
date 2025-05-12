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

#ifdef MCENGINE_FEATURE_MULTITHREADING

#include <mutex>

static constexpr int default_numthreads = 3;

static std::mutex g_resourceManagerMutex;            // internal lock for nested async loads
static std::mutex g_resourceManagerLoadingWorkMutex; // work vector lock across all threads

static void *_resourceLoaderThread(void *data);

#else
static constexpr int default_numthreads = 0;
#endif

class ResourceManagerLoaderThread
{
public:
#ifdef MCENGINE_FEATURE_MULTITHREADING

	// self
	McThread *thread{};

	// synchronization
	std::mutex workMutex;
	std::condition_variable workCondition;
	bool hasWork{};

	// args
	std::atomic<size_t> threadIndex;
	std::atomic<bool> running;
	std::vector<ResourceManager::LOADING_WORK> *loadingWork{};

	ResourceManagerLoaderThread() : threadIndex(0), running(false) {}

#endif
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
	m_iNumResourceInitPerFrameLimit = 1;

	m_loadingWork.reserve(32);
	
	int threads = std::clamp(env->getLogicalCPUCount(), default_numthreads, 32); // sanity
	if (threads > default_numthreads)
		rm_numthreads.setValue(threads);

	// create loader threads
#ifdef MCENGINE_FEATURE_MULTITHREADING

	for (int i = 0; i < threads; i++)
	{
		ResourceManagerLoaderThread *loaderThread = new ResourceManagerLoaderThread();

		loaderThread->hasWork = false;
		loaderThread->threadIndex = i;
		loaderThread->running = true;
		loaderThread->loadingWork = &m_loadingWork;

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

#endif
}

ResourceManager::~ResourceManager()
{
	// release all not-currently-being-loaded resources (1)
	destroyResources();

	// let all loader threads exit
#ifdef MCENGINE_FEATURE_MULTITHREADING

	for (auto & m_thread : m_threads)
	{
		m_thread->running = false;
	}

	// notify all threads to check their running state
	notifyWorkerThreads();

	// wait for threads to stop
	for (auto & m_thread : m_threads)
	{
		delete m_thread->thread;
		delete m_thread;
	}

	m_threads.clear();

#endif

	// cleanup leftovers (can only do that after loader threads have exited) (2)
	for (auto & i : m_loadingWorkAsyncDestroy)
	{
		delete i;
	}
	m_loadingWorkAsyncDestroy.clear();
}

void ResourceManager::update()
{
#ifdef MCENGINE_FEATURE_MULTITHREADING
	std::lock_guard<std::mutex> lock(g_resourceManagerMutex);
#endif

	// handle load finish (and synchronous init())
	size_t numResourceInitCounter = 0;
	for (size_t i = 0; i < m_loadingWork.size(); i++)
	{
		if (m_loadingWork[i].done.atomic.load())
		{
			if (debug_rm->getBool())
				debugLog("Resource Manager: Worker thread #%i finished.\n", i);

			// get resource and thread index before modifying the vector
			Resource *rs = m_loadingWork[i].resource.atomic.load();
			const size_t threadIndex = m_loadingWork[i].threadIndex.atomic.load();

			// remove the work item from the queue
#ifdef MCENGINE_FEATURE_MULTITHREADING
			{
				std::lock_guard<std::mutex> workLock(g_resourceManagerLoadingWorkMutex);
				m_loadingWork.erase(m_loadingWork.begin() + i);
			}
#else
			m_loadingWork.erase(m_loadingWork.begin() + i);
#endif
			i--; // adjust work index after deletion

#ifdef MCENGINE_FEATURE_MULTITHREADING
			// check if the thread has any remaining work
			int numLoadingWorkForThreadIndex = 0;
			for (size_t w = 0; w < m_loadingWork.size(); w++)
			{
				if (m_loadingWork[w].threadIndex.atomic.load() == threadIndex)
					numLoadingWorkForThreadIndex++;
			}

			// update the thread's work status if it has no more work
			if (numLoadingWorkForThreadIndex < 1 && m_threads.size() > 0)
			{
				std::lock_guard<std::mutex> threadLock(m_threads[threadIndex]->workMutex);
				m_threads[threadIndex]->hasWork = false;
			}
#endif

			// finish (synchronous init())
			rs->load();
			numResourceInitCounter++;

			if (m_iNumResourceInitPerFrameLimit > 0 && numResourceInitCounter >= m_iNumResourceInitPerFrameLimit)
				break; // only allow set number of work items to finish per frame (avoid stutters)
		}
	}

	// async destroy, similar approach (collect candidates with minimal lock time)
	std::vector<Resource *> resourcesReadyForDestroy;

	for (size_t i = 0; i < m_loadingWorkAsyncDestroy.size(); i++)
	{
		bool canBeDestroyed = true;

		{
#ifdef MCENGINE_FEATURE_MULTITHREADING
			std::lock_guard<std::mutex> workLock(g_resourceManagerLoadingWorkMutex);
#endif
			for (auto & w : m_loadingWork)
			{
				if (w.resource.atomic.load() == m_loadingWorkAsyncDestroy[i])
				{
					if (debug_rm->getBool())
						debugLog("Resource Manager: Waiting for async destroy of #%zu ...\n", i);

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

	// don't need to hold the lock to destroy the resource
	for (Resource *rs : resourcesReadyForDestroy)
	{
		if (debug_rm->getBool())
			debugLog("Resource Manager: Async destroy of resource %s\n", rs->getName().toUtf8());

		delete rs; // implicitly calls release() through the Resource destructor
	}
}

void ResourceManager::notifyWorkerThreads()
{
#ifdef MCENGINE_FEATURE_MULTITHREADING
	for (auto & m_thread : m_threads)
	{
		{
			std::lock_guard<std::mutex> lock(m_thread->workMutex);
			// no need to set hasWork here, that's handled when adding work
		}
		m_thread->workCondition.notify_one();
	}
#endif
}

void ResourceManager::destroyResources()
{
	while (m_vResources.size() > 0)
	{
		destroyResource(m_vResources[0]);
	}
	m_vResources.clear();
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
		debugLog("ResourceManager: Destroying %s\n", rs->getName().toUtf8());

#ifdef MCENGINE_FEATURE_MULTITHREADING
	std::lock_guard<std::mutex> lock(g_resourceManagerMutex);
#endif

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

	// handle async destroy
	for (auto & w : m_loadingWork)
	{
		if (w.resource.atomic.load() == rs)
		{
			if (debug_rm->getBool())
				debugLog("Resource Manager: Scheduled async destroy of %s\n", rs->getName().toUtf8());

			if (rm_interrupt_on_destroy.getBool())
				rs->interruptLoad();

			m_loadingWorkAsyncDestroy.push_back(rs);
			if (isManagedResource)
				m_vResources.erase(m_vResources.begin() + managedResourceIndex);

			return; // we're done here
		}
	}

	// standard destroy
	SAFE_DELETE(rs);

	if (isManagedResource)
		m_vResources.erase(m_vResources.begin() + managedResourceIndex);
}

void ResourceManager::reloadResources()
{
	for (auto & m_vResource : m_vResources)
	{
		m_vResource->reload();
	}
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
			return dynamic_cast<Image *>(temp);
	}

	// create instance and load it
	filepath.insert(0, PATH_DEFAULT_IMAGES);
	Image *img = engine->getGraphics()->createImage(filepath, mipmapped, keepInSystemMemory);
	img->setName(resourceName);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageUnnamed(UString filepath, bool mipmapped, bool keepInSystemMemory)
{
	filepath.insert(0, PATH_DEFAULT_IMAGES);
	Image *img = engine->getGraphics()->createImage(filepath, mipmapped, keepInSystemMemory);

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
			return dynamic_cast<Image *>(temp);
	}

	// create instance and load it
	Image *img = engine->getGraphics()->createImage(absoluteFilepath, mipmapped, keepInSystemMemory);
	img->setName(resourceName);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageAbsUnnamed(UString absoluteFilepath, bool mipmapped, bool keepInSystemMemory)
{
	Image *img = engine->getGraphics()->createImage(absoluteFilepath, mipmapped, keepInSystemMemory);

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

	Image *img = engine->getGraphics()->createImage(width, height, mipmapped, keepInSystemMemory);

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
			return dynamic_cast<McFont *>(temp);
	}

	// create instance and load it
	filepath.insert(0, PATH_DEFAULT_FONTS);
	McFont *fnt = new McFont(filepath, fontSize, antialiasing, fontDPI);
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
			return dynamic_cast<McFont *>(temp);
	}

	// create instance and load it
	filepath.insert(0, PATH_DEFAULT_FONTS);
	McFont *fnt = new McFont(filepath, characters, fontSize, antialiasing, fontDPI);
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
			return dynamic_cast<Sound *>(temp);
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
			return dynamic_cast<Sound *>(temp);
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
			return dynamic_cast<Shader *>(temp);
	}

	// create instance and load it
	vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = engine->getGraphics()->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::loadShader(UString vertexShaderFilePath, UString fragmentShaderFilePath)
{
	vertexShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	fragmentShaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = engine->getGraphics()->createShaderFromFile(vertexShaderFilePath, fragmentShaderFilePath);

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
			return dynamic_cast<Shader *>(temp);
	}

	// create instance and load it
	Shader *shader = engine->getGraphics()->createShaderFromSource(vertexShader, fragmentShader);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader(UString vertexShader, UString fragmentShader)
{
	Shader *shader = engine->getGraphics()->createShaderFromSource(vertexShader, fragmentShader);

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
			return dynamic_cast<Shader *>(temp);
	}

	// create instance and load it
	shaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = engine->getGraphics()->createShaderFromFile(shaderFilePath);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::loadShader2(UString shaderFilePath)
{
	shaderFilePath.insert(0, PATH_DEFAULT_SHADERS);
	Shader *shader = engine->getGraphics()->createShaderFromFile(shaderFilePath);

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
			return dynamic_cast<Shader *>(temp);
	}

	// create instance and load it
	Shader *shader = engine->getGraphics()->createShaderFromSource(shaderSource);
	shader->setName(resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader2(UString shaderSource)
{
	Shader *shader = engine->getGraphics()->createShaderFromSource(shaderSource);

	loadResource(shader, true);

	return shader;
}

RenderTarget *ResourceManager::createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	RenderTarget *rt = engine->getGraphics()->createRenderTarget(x, y, width, height, multiSampleType);
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
	TextureAtlas *ta = new TextureAtlas(width, height);
	ta->setName(UString::format("_TA_%ix%i", width, height));

	loadResource(ta, false);

	return ta;
}

VertexArrayObject *ResourceManager::createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory)
{
	VertexArrayObject *vao = engine->getGraphics()->createVertexArrayObject(primitive, usage, keepInSystemMemory);

	loadResource(vao, false);

	return vao;
}

Image *ResourceManager::getImage(UString resourceName) const
{
	for (auto m_vResource : m_vResources)
	{
		if (m_vResource->getName() == resourceName)
			return dynamic_cast<Image *>(m_vResource);
	}

	doesntExistWarning(resourceName);
	return NULL;
}

McFont *ResourceManager::getFont(UString resourceName) const
{
	for (auto m_vResource : m_vResources)
	{
		if (m_vResource->getName() == resourceName)
			return dynamic_cast<McFont *>(m_vResource);
	}

	doesntExistWarning(resourceName);
	return NULL;
}

Sound *ResourceManager::getSound(UString resourceName) const
{
	for (auto m_vResource : m_vResources)
	{
		if (m_vResource->getName() == resourceName)
			return dynamic_cast<Sound *>(m_vResource);
	}

	doesntExistWarning(resourceName);
	return NULL;
}

Shader *ResourceManager::getShader(UString resourceName) const
{
	for (auto m_vResource : m_vResources)
	{
		if (m_vResource->getName() == resourceName)
			return dynamic_cast<Shader *>(m_vResource);
	}

	doesntExistWarning(resourceName);
	return NULL;
}

bool ResourceManager::isLoading() const
{
	return (m_loadingWork.size() > 0);
}

bool ResourceManager::isLoadingResource(Resource *rs) const
{
	for (const auto & i : m_loadingWork)
	{
		if (i.resource.atomic.load() == rs)
			return true;
	}

	return false;
}

void ResourceManager::loadResource(Resource *res, bool load)
{
	// handle flags
	if (m_nextLoadUnmanagedStack.size() < 1 || !m_nextLoadUnmanagedStack.top())
		m_vResources.push_back(res); // add managed resource

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
#if defined(MCENGINE_FEATURE_MULTITHREADING)

		if (rm_numthreads.getInt() > 0)
		{
			std::lock_guard<std::mutex> lock(g_resourceManagerMutex);

			// split work evenly/linearly across all threads (RR)
			static size_t threadIndexCounter = 0;
			const size_t threadIndex = threadIndexCounter;

			// add work to loading thread
			LOADING_WORK work;
			work.resource = MobileAtomicResource(res);
			work.threadIndex = MobileAtomicSizeT(threadIndex);
			work.done = MobileAtomicBool(false);

			threadIndexCounter = (threadIndexCounter + 1) % (std::min(m_threads.size(), (size_t)std::max(rm_numthreads.getInt(), 1)));

			{
				std::lock_guard<std::mutex> workLock(g_resourceManagerLoadingWorkMutex);
				m_loadingWork.push_back(work);
			}

			// count work for this thread
			int numLoadingWorkForThreadIndex = 0;
			for (auto & i : m_loadingWork)
			{
				if (i.threadIndex.atomic.load() == threadIndex)
					numLoadingWorkForThreadIndex++;
			}

			// update the thread's work status and notify it
			if (numLoadingWorkForThreadIndex > 0 && m_threads.size() > 0)
			{
				{
					std::lock_guard<std::mutex> threadLock(m_threads[threadIndex]->workMutex);
					m_threads[threadIndex]->hasWork = true;
				}
				m_threads[threadIndex]->workCondition.notify_one();
			}
		}
		else
		{
			// load normally (threading disabled)
			res->loadAsync();
			res->load();
		}

#else

		// load normally (on platforms which don't support multithreading)
		res->loadAsync();
		res->load();

#endif
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
	for (auto & m_vResource : m_vResources)
	{
		if (m_vResource->getName() == resourceName)
		{
			if (rm_warnings.getBool())
				debugLog("RESOURCE MANAGER: Resource \"%s\" already loaded!\n", resourceName.toUtf8());

			// handle flags (reset them)
			resetFlags();

			return m_vResource;
		}
	}

	return NULL;
}

void ResourceManager::resetFlags()
{
	if (m_nextLoadUnmanagedStack.size() > 0)
		m_nextLoadUnmanagedStack.pop();

	m_bNextLoadAsync = false;
}

#ifdef MCENGINE_FEATURE_MULTITHREADING

static void *_resourceLoaderThread(void *data)
{
	ResourceManagerLoaderThread *self = (ResourceManagerLoaderThread *)data;
	const size_t threadIndex = self->threadIndex.load();

	while (self->running.load())
	{
		// wait for work
		{
			std::unique_lock<std::mutex> lock(self->workMutex);
			self->workCondition.wait(lock, [self]() { return self->hasWork || !self->running.load(); });

			// if we were woken up but the thread isn't running anymore, exit
			if (!self->running.load())
				break;
		}

		Resource *resourceToLoad = nullptr;

		// quickly check if there is work to do (this can potentially cause engine lag!)
		{
			std::lock_guard<std::mutex> lock(g_resourceManagerLoadingWorkMutex);

			for (auto & i : *self->loadingWork)
			{
				if (i.threadIndex.atomic.load() == threadIndex && !i.done.atomic.load())
				{
					resourceToLoad = i.resource.atomic.load();
					break;
				}
			}
		}

		// if we have work, process it without holding the lock
		if (resourceToLoad != nullptr)
		{
			// debug pause
			if (rm_debug_async_delay.getFloat() > 0.0f)
				Timing::sleep(rm_debug_async_delay.getFloat() * 1000 * 1000);

			// asynchronous initAsync() (do the actual work)
			resourceToLoad->loadAsync();

			// signal that we're done with this resource
			{
				std::lock_guard<std::mutex> lock(g_resourceManagerLoadingWorkMutex);

				for (auto & i : *self->loadingWork)
				{
					if (i.threadIndex.atomic.load() == threadIndex && i.resource.atomic.load() == resourceToLoad)
					{
						i.done = ResourceManager::MobileAtomicBool(true);
						break;
					}
				}
			}
		}
		else
		{
			// no work found, update thread status
			std::lock_guard<std::mutex> lock(self->workMutex);
			self->hasWork = false;
		}
	}

	return nullptr;
}

#endif
