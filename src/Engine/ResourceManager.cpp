//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		resource manager
//
// $NoKeywords: $rm
//===============================================================================//

#include "ResourceManager.h"

#include "ConVar.h"
#include "Environment.h"
#include "Thread.h"
#include "App.h" // for isInCriticalInteractiveSession() (gameplay detection)

#include <algorithm>
#include <mutex>

using namespace std::chrono_literals;

static constexpr int MIN_NUM_THREADS = 1;       // always keep 1 alive
static constexpr auto THREAD_IDLE_TIMEOUT = 5s; // how long to keep a non-core thread alive waiting for work before destroying it

ConVar rm_interrupt_on_destroy("rm_interrupt_on_destroy", true, FCVAR_CHEAT);
ConVar debug_rm_("debug_rm", false, FCVAR_NONE);

ConVar *ResourceManager::debug_rm = &debug_rm_;

//==================================
// LOADER THREAD
//==================================
class ResourceManagerLoaderThread final
{
public:
	// self
	McThread *thread{};

	// parent reference
	ResourceManager *resourceManager{};
	size_t threadIndex{};
	bool isCore{}; // true if this is a core thread that shouldn't terminate
	std::chrono::steady_clock::time_point lastWorkTime{};
	std::chrono::milliseconds idleTimeoutOffset{0}; // randomize destroy timeout to avoid a wave of threads being destroyed at once

	ResourceManagerLoaderThread() = default;
};

static void *_resourceLoaderThread(void *data, std::stop_token stopToken)
{
	auto *self = static_cast<ResourceManagerLoaderThread *>(data);
	ResourceManager *manager = self->resourceManager;
	const size_t threadIndex = self->threadIndex;
	const bool isCore = self->isCore;

	// update last work time to now
	self->lastWorkTime = std::chrono::steady_clock::now();

	// increment active thread count
	manager->m_activeThreadCount.fetch_add(1);

	if (ResourceManager::debug_rm->getBool())
		debugLog("Resource Manager: Thread #{} started (core: {})\n", threadIndex, isCore);

	while (!stopToken.stop_requested() && !manager->m_bShuttingDown.load())
	{
		// try to get work
		ResourceManager::LOADING_WORK *work = manager->getNextWork();

		// if no work available, wait for notification with timeout
		if (!work)
		{
			std::unique_lock<std::mutex> lock(manager->m_workAvailableMutex);

			// use stop_callback to wake up the condition variable when stop is requested
			std::stop_callback stopCallback(stopToken, [&]() { manager->m_workAvailable.notify_all(); });

			// wait with timeout
			auto waitTime = isCore ? 100ms : manager->m_threadIdleTimeout + self->idleTimeoutOffset;

			// if the wait times out, non-core threads will terminate (if the below conditions hold)
			bool workAvailable = manager->m_workAvailable.wait_for(
			    lock, waitTime, [&]() { return stopToken.stop_requested() || manager->m_bShuttingDown.load() || manager->getNumLoadingWork() > 0; });

			// if we timed out, and we're not a core thread, and the app isn't in a critical section, exit and request destruction of the thread
			if (!workAvailable && !isCore && !manager->m_bLowLatency)
			{
				if (ResourceManager::debug_rm->getBool())
					debugLog("Resource Manager: Thread #{} terminating due to idle timeout\n", threadIndex);

				// request stop to signal that this thread should be cleaned up
				self->thread->requestStop();
				break;
			}

			// check if we should exit due to shutdown or stop request
			if (stopToken.stop_requested() || manager->m_bShuttingDown.load())
				break;

			continue;
		}

		// we have work, update last work time
		self->lastWorkTime = std::chrono::steady_clock::now();

		// process work
		Resource *resource = work->resource;

		if (ResourceManager::debug_rm->getBool())
			debugLog("Resource Manager: Thread #{} loading {:s}\n", threadIndex, resource->getName().toUtf8());

		// async load
		resource->loadAsync();

		// mark as async complete
		work->asyncDone = true;
		manager->markWorkAsyncComplete(work);

		if (ResourceManager::debug_rm->getBool())
			debugLog("Resource Manager: Thread #{} finished async loading {:s}\n", threadIndex, resource->getName().toUtf8());
	}

	// decrement active thread count
	manager->m_activeThreadCount.fetch_sub(1);

	if (ResourceManager::debug_rm->getBool())
		debugLog("Resource Manager: Thread #{} exiting\n", threadIndex);

	return nullptr;
}
//==================================
// LOADER THREAD ENDS
//==================================

ResourceManager::ResourceManager()
{
	m_bNextLoadAsync = false;
	m_bShuttingDown = false;

	// reserve space for typed vectors
	m_vImages.reserve(64);
	m_vFonts.reserve(16);
	m_vSounds.reserve(64);
	m_vShaders.reserve(32);
	m_vRenderTargets.reserve(8);
	m_vTextureAtlases.reserve(8);
	m_vVertexArrayObjects.reserve(32);

	// configure thread pool
	m_maxThreads = std::clamp(env->getLogicalCPUCount(), MIN_NUM_THREADS, 32);

	m_threadIdleTimeout = THREAD_IDLE_TIMEOUT;

	// create initial core threads
	for (size_t i = 0; i < MIN_NUM_THREADS; i++)
	{
		auto *loaderThread = new ResourceManagerLoaderThread();

		loaderThread->threadIndex = m_totalThreadsCreated.fetch_add(1);
		loaderThread->resourceManager = this;
		loaderThread->isCore = true;
		loaderThread->idleTimeoutOffset = 0ms;

		loaderThread->thread = new McThread(_resourceLoaderThread, (void *)loaderThread);
		if (!loaderThread->thread->isReady())
		{
			engine->showMessageError("ResourceManager Error", "Couldn't create core thread!");
			SAFE_DELETE(loaderThread->thread);
			SAFE_DELETE(loaderThread);
		}
		else
		{
			std::lock_guard<std::mutex> lock(m_threadsMutex);
			m_threads.push_back(loaderThread);
		}
	}
}

ResourceManager::~ResourceManager()
{
	// signal shutdown
	m_bShuttingDown = true;

	// release all not-currently-being-loaded resources (1)
	destroyResources();

	// request all threads to stop
	{
		std::lock_guard<std::mutex> lock(m_threadsMutex);
		for (auto &thread : m_threads)
		{
			if (thread->thread)
				thread->thread->requestStop();
		}
	}

	// wake up all threads so they can exit
	m_workAvailable.notify_all();

	// wait for threads to stop and clean them up
	{
		std::lock_guard<std::mutex> lock(m_threadsMutex);
		for (auto &thread : m_threads)
		{
			delete thread->thread;
			delete thread;
		}
		m_threads.clear();
	}

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

//==================================
// async/threading stuff below here
//==================================

void ResourceManager::update()
{
	// cleanup (delete) any non-core threads that have requested destruction due to being idle, and when not in a critical interactive session
	// TODO: duplicated logic between the thread itself and the main thread,
	//		 should consolidate to one or the other
	if (!(m_bLowLatency = app->isInCriticalInteractiveSession()))
		cleanupIdleThreads();

	// only allow set number of work items to finish per frame (avoid stutters)
	const size_t amountToProcess = m_bLowLatency ? 1 : m_maxThreads;

	// process completed async work
	std::vector<LOADING_WORK *> completedWork;

	// grab amountToProcess # completed work at once to minimize lock time
	{
		std::lock_guard<std::mutex> lock(m_asyncCompleteWorkMutex);
		size_t numPopped = 0;
		while (numPopped < amountToProcess && !m_asyncCompleteWork.empty())
		{
			completedWork.push_back(m_asyncCompleteWork.front());
			m_asyncCompleteWork.pop();
			numPopped++;
		}
	}

	// process completed work (synchronous init)
	size_t numProcessed = 0;

	for (auto work : completedWork)
	{
		if (numProcessed >= amountToProcess)
			break;
		if (!work->syncDone.load() && work->asyncDone.load())
		{
			Resource *rs = work->resource;

			if (debug_rm->getBool())
				debugLog("Resource Manager: Sync init for {:s}\n", rs->getName().toUtf8());

			// synchronous init on main thread
			rs->load();
			work->syncDone = true;

			numProcessed++;
		}
	}

	// put remaining work back in the queue for the next update
	if (numProcessed < completedWork.size())
	{
		std::lock_guard<std::mutex> lock(m_asyncCompleteWorkMutex);
		for (size_t i = numProcessed; i < completedWork.size(); i++)
		{
			m_asyncCompleteWork.push(completedWork[i]);
		}
	}

	// cleanup fully completed work
	{
		std::lock_guard<std::mutex> lock(m_allWorkMutex);
		m_allWork.erase(std::remove_if(m_allWork.begin(), m_allWork.end(),
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

void ResourceManager::ensureThreadAvailable()
{
	// quick check without lock first
	size_t activeThreads = m_activeThreadCount.load();
	size_t pendingWorkCount = 0;

	{
		std::lock_guard<std::mutex> lock(m_pendingWorkMutex);
		pendingWorkCount = m_pendingWork.size();
	}

	// if we have more pending work than active threads and haven't hit the max
	if (pendingWorkCount > activeThreads && activeThreads < m_maxThreads)
	{
		std::lock_guard<std::mutex> lock(m_threadsMutex);

		// double-check under lock
		if (m_threads.size() < m_maxThreads)
		{
			auto *loaderThread = new ResourceManagerLoaderThread();

			loaderThread->threadIndex = m_totalThreadsCreated.fetch_add(1);
			loaderThread->resourceManager = this;
			loaderThread->isCore = false; // non-core thread can timeout

			// don't allow a wave of loader threads to exit at once
			const auto randomAdditionalMS = std::chrono::milliseconds((static_cast<long>(1.0f + ((static_cast<float>(std::rand() % 10)) / 10.0f)) * 1000L));
			loaderThread->idleTimeoutOffset = randomAdditionalMS;

			loaderThread->thread = new McThread(_resourceLoaderThread, (void *)loaderThread);
			if (!loaderThread->thread->isReady())
			{
				if (debug_rm->getBool())
					debugLog("ResourceManager Warning: Couldn't create dynamic thread!\n");
				SAFE_DELETE(loaderThread->thread);
				SAFE_DELETE(loaderThread);
			}
			else
			{
				m_threads.push_back(loaderThread);
				if (debug_rm->getBool())
					debugLog("Resource Manager: Created dynamic thread #{} (total: {})\n", loaderThread->threadIndex, m_threads.size());
			}
		}
	}
}

void ResourceManager::cleanupIdleThreads()
{
	// only core threads exist, nothing to clean up
	// don't need to take a lock (since we'd just clean up on the next update if the info was out-of-date)
	if (m_threads.size() <= MIN_NUM_THREADS)
		return;

	std::vector<ResourceManagerLoaderThread *> threadsToDelete;
	{
		std::lock_guard<std::mutex> lock(m_threadsMutex);

		// double-check under lock
		if (m_threads.size() <= MIN_NUM_THREADS)
			return;

		// find threads that have exited
		for (auto it = m_threads.begin(); it != m_threads.end();)
		{
			auto *thread = *it;

			// check if thread has stopped (either by request or self-termination)
			if (thread->thread && thread->thread->isStopRequested())
			{
				// don't remove core threads unless we're shutting down
				if (!thread->isCore || m_bShuttingDown.load())
				{
					threadsToDelete.push_back(thread);
					it = m_threads.erase(it);
					continue;
				}
			}

			++it;
		}
	}

	// delete threads outside of lock
	for (auto *thread : threadsToDelete)
	{
		if (debug_rm->getBool())
			debugLog("Resource Manager: Cleaning up thread #{}\n", thread->threadIndex);

		delete thread->thread;
		delete thread;
	}
}

size_t ResourceManager::getNumActiveThreads() const
{
	return m_activeThreadCount.load();
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
	if (rs == NULL)
	{
		if (debug_rm->getBool())
			debugLog("ResourceManager Warning: destroyResource(NULL)!\n");
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

	// FIXME(?): we never get here in practice, but maybe that's a good thing
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

		// ensure we have a thread available to process the work
		ensureThreadAvailable();

		// notify worker threads of available work
		m_workAvailable.notify_one();
	}
}

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
	if (rs == NULL)
	{
		if (debug_rm->getBool())
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
		if (debug_rm->getBool())
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

//=====================================================
// other non-async-specific loading helpers below here
//=====================================================
void ResourceManager::setResourceName(Resource *res, UString name)
{
	if (!res)
	{
		if (debug_rm->getBool())
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
	// TODO: this is slightly incorrect. this function could be called before the resource is loaded BY ResourceManager,
	//		 OR at any point in time from external callers. if its being called externally, the nextLoadUnmanagedStack is irrelevant.
	//		 since being in the name-to-resource map alone when it "shouldn't be" shouldn't cause any real issues,
	//		 i don't think it's worth putting any more effort (or lazily duplicating code) to prevent that?
	if (m_nextLoadUnmanagedStack.size() < 1 || !m_nextLoadUnmanagedStack.top())
		m_nameToResourceMap.try_emplace(name, res); // its okay if we have multiple named references to the resource
	return;
}

Image *ResourceManager::loadImage(UString filepath, UString resourceName, bool mipmapped, bool keepInSystemMemory)
{
	auto res = checkIfExistsAndHandle<Image>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	filepath.insert(0, ResourceManager::PATH_DEFAULT_IMAGES);
	Image *img = graphics->createImage(filepath, mipmapped, keepInSystemMemory);
	setResourceName(img, resourceName);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageUnnamed(UString filepath, bool mipmapped, bool keepInSystemMemory)
{
	filepath.insert(0, ResourceManager::PATH_DEFAULT_IMAGES);
	Image *img = graphics->createImage(filepath, mipmapped, keepInSystemMemory);

	loadResource(img, true);

	return img;
}

Image *ResourceManager::loadImageAbs(UString absoluteFilepath, UString resourceName, bool mipmapped, bool keepInSystemMemory)
{
	auto res = checkIfExistsAndHandle<Image>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	Image *img = graphics->createImage(absoluteFilepath, mipmapped, keepInSystemMemory);
	setResourceName(img, resourceName);

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
	Shader *shader = graphics->createShaderFromFile(shaderFilePath);
	setResourceName(shader, resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::loadShader(UString shaderFilePath)
{
	shaderFilePath.insert(0, ResourceManager::PATH_DEFAULT_SHADERS);
	Shader *shader = graphics->createShaderFromFile(shaderFilePath);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader(UString shaderSource, UString resourceName)
{
	auto res = checkIfExistsAndHandle<Shader>(resourceName);
	if (res != nullptr)
		return res;

	// create instance and load it
	Shader *shader = graphics->createShaderFromSource(shaderSource);
	setResourceName(shader, resourceName);

	loadResource(shader, true);

	return shader;
}

Shader *ResourceManager::createShader(UString shaderSource)
{
	Shader *shader = graphics->createShaderFromSource(shaderSource);

	loadResource(shader, true);

	return shader;
}

RenderTarget *ResourceManager::createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	RenderTarget *rt = graphics->createRenderTarget(x, y, width, height, multiSampleType);
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
	VertexArrayObject *vao = graphics->createVertexArrayObject(primitive, usage, keepInSystemMemory);

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
