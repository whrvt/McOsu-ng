//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		asynchronous resource loading system
//
// $NoKeywords: $arl
//===============================================================================//

#include "AsyncResourceLoader.h"

#include "App.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "Thread.h"

#include <algorithm>

using namespace std::chrono_literals;

//==================================
// LOADER THREAD
//==================================
class AsyncResourceLoader::LoaderThread final
{
public:
	std::unique_ptr<McThread> thread;
	AsyncResourceLoader *loader{};
	size_t threadIndex{};
	std::chrono::steady_clock::time_point lastWorkTime{};

	LoaderThread(AsyncResourceLoader *loader_ptr, size_t index)
	    : loader(loader_ptr),
	      threadIndex(index)
	{
	}
};

namespace
{
void *asyncResourceLoaderThread(void *data, std::stop_token stopToken)
{
	auto *loaderThread = static_cast<AsyncResourceLoader::LoaderThread *>(data);
	AsyncResourceLoader *loader = loaderThread->loader;
	const size_t threadIndex = loaderThread->threadIndex;

	loaderThread->lastWorkTime = std::chrono::steady_clock::now();
	loader->m_activeThreadCount.fetch_add(1);

	if (cv::debug_rm.getBool())
		debugLog("AsyncResourceLoader: Thread #{} started\n", threadIndex);

	while (!stopToken.stop_requested() && !loader->m_shuttingDown.load())
	{
		// yield in case we're sharing a logical CPU, like on a single-core system
		Timing::sleep(0);

		auto work = loader->getNextPendingWork();

		if (!work)
		{
			std::unique_lock<std::mutex> lock(loader->m_workAvailableMutex);

			std::stop_callback stopCallback(stopToken, [&]() { loader->m_workAvailable.notify_all(); });

			auto waitTime = loader->m_threadIdleTimeout;
			bool workAvailable = loader->m_workAvailable.wait_for(
			    lock, waitTime, [&]() { return stopToken.stop_requested() || loader->m_shuttingDown.load() || loader->m_activeWorkCount.load() > 0; });

			if (!workAvailable && !(app && app->isInCriticalInteractiveSession()))
			{
				if (cv::debug_rm.getBool())
					debugLog("AsyncResourceLoader: Thread #{} terminating due to idle timeout\n", threadIndex);

				loaderThread->thread->requestStop();
				break;
			}

			if (stopToken.stop_requested() || loader->m_shuttingDown.load())
				break;

			continue;
		}

		loaderThread->lastWorkTime = std::chrono::steady_clock::now();

		Resource *resource = work->resource;
		work->state.store(AsyncResourceLoader::WorkState::ASYNC_IN_PROGRESS);

		if (cv::debug_rm.getBool())
			debugLog("AsyncResourceLoader: Thread #{} loading {:s}\n", threadIndex, resource->getName());

		resource->loadAsync();

		work->state.store(AsyncResourceLoader::WorkState::ASYNC_COMPLETE);
		loader->markWorkAsyncComplete(std::move(work));

		if (cv::debug_rm.getBool())
			debugLog("AsyncResourceLoader: Thread #{} finished async loading {:s}\n", threadIndex, resource->getName());
	}

	loader->m_activeThreadCount.fetch_sub(1);

	if (cv::debug_rm.getBool())
		debugLog("AsyncResourceLoader: Thread #{} exiting\n", threadIndex);

	return nullptr;
}
} // namespace

//==================================
// ASYNC RESOURCE LOADER
//==================================

AsyncResourceLoader::AsyncResourceLoader()
{
	m_maxThreads = std::clamp(env->getLogicalCPUCount() - 1, MIN_NUM_THREADS, 32);
	m_threadIdleTimeout = THREAD_IDLE_TIMEOUT;

	// create initial threads
	for (size_t i = 0; i < MIN_NUM_THREADS; i++)
	{
		auto loaderThread = std::make_unique<LoaderThread>(this, m_totalThreadsCreated.fetch_add(1));

		loaderThread->thread = std::make_unique<McThread>(asyncResourceLoaderThread, loaderThread.get());
		if (!loaderThread->thread->isReady())
		{
			engine->showMessageError("AsyncResourceLoader Error", "Couldn't create core thread!");
		}
		else
		{
			std::lock_guard<std::mutex> lock(m_threadsMutex);
			m_threads.push_back(std::move(loaderThread));
		}
	}
}

AsyncResourceLoader::~AsyncResourceLoader()
{
	shutdown();
}

void AsyncResourceLoader::shutdown()
{
	m_shuttingDown = true;

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

	// wait for threads to stop
	{
		std::lock_guard<std::mutex> lock(m_threadsMutex);
		m_threads.clear();
	}

	// cleanup remaining work items
	{
		std::lock_guard<std::mutex> lock(m_workQueueMutex);
		while (!m_pendingWork.empty())
		{
			m_pendingWork.pop();
		}
		while (!m_asyncCompleteWork.empty())
		{
			m_asyncCompleteWork.pop();
		}
	}

	// cleanup loading resources tracking
	{
		std::lock_guard<std::mutex> lock(m_loadingResourcesMutex);
		m_loadingResources.clear();
	}

	// cleanup async destroy queue
	for (auto &rs : m_asyncDestroyQueue)
	{
		SAFE_DELETE(rs);
	}
	m_asyncDestroyQueue.clear();
}

void AsyncResourceLoader::requestAsyncLoad(Resource *resource)
{
	auto work = std::make_unique<LoadingWork>(resource, m_workIdCounter.fetch_add(1));

	// add to tracking set
	{
		std::lock_guard<std::mutex> lock(m_loadingResourcesMutex);
		m_loadingResources.insert(resource);
	}

	// add to work queue
	{
		std::lock_guard<std::mutex> lock(m_workQueueMutex);
		m_pendingWork.push(std::move(work));
	}

	m_activeWorkCount.fetch_add(1);
	ensureThreadAvailable();
	m_workAvailable.notify_one();
}

void AsyncResourceLoader::update(bool lowLatency)
{
	if (!lowLatency)
		cleanupIdleThreads();

	const size_t amountToProcess = lowLatency ? 1 : m_maxThreads;

	// process completed async work
	size_t numProcessed = 0;

	while (numProcessed < amountToProcess)
	{
		auto work = getNextAsyncCompleteWork();
		if (!work)
			break;

		Resource *rs = work->resource;

		if (cv::debug_rm.getBool())
			debugLog("AsyncResourceLoader: Sync init for {:s}\n", rs->getName());

		rs->load();
		work->state.store(WorkState::SYNC_COMPLETE);

		// remove from tracking set
		{
			std::lock_guard<std::mutex> lock(m_loadingResourcesMutex);
			m_loadingResources.erase(rs);
		}

		m_activeWorkCount.fetch_sub(1);
		numProcessed++;

		// work will be automatically destroyed when unique_ptr goes out of scope
	}

	// process async destroy queue
	std::vector<Resource *> resourcesReadyForDestroy;

	{
		std::lock_guard<std::mutex> lock(m_asyncDestroyMutex);
		for (size_t i = 0; i < m_asyncDestroyQueue.size(); i++)
		{
			bool canBeDestroyed = true;

			{
				std::lock_guard<std::mutex> loadingLock(m_loadingResourcesMutex);
				if (m_loadingResources.find(m_asyncDestroyQueue[i]) != m_loadingResources.end())
				{
					canBeDestroyed = false;
				}
			}

			if (canBeDestroyed)
			{
				resourcesReadyForDestroy.push_back(m_asyncDestroyQueue[i]);
				m_asyncDestroyQueue.erase(m_asyncDestroyQueue.begin() + i);
				i--;
			}
		}
	}

	for (Resource *rs : resourcesReadyForDestroy)
	{
		if (cv::debug_rm.getBool())
			debugLog("AsyncResourceLoader: Async destroy of resource {:s}\n", rs->getName());

		SAFE_DELETE(rs);
	}
}

void AsyncResourceLoader::scheduleAsyncDestroy(Resource *resource)
{
	if (cv::debug_rm.getBool())
		debugLog("AsyncResourceLoader: Scheduled async destroy of {:s}\n", resource->getName());

	std::lock_guard<std::mutex> lock(m_asyncDestroyMutex);
	m_asyncDestroyQueue.push_back(resource);
}

void AsyncResourceLoader::reloadResources(const std::vector<Resource *> &resources)
{
	if (resources.empty())
	{
		if (cv::debug_rm.getBool())
			debugLog("AsyncResourceLoader Warning: reloadResources with empty resources vector!\n");
		return;
	}

	if (cv::debug_rm.getBool())
		debugLog("AsyncResourceLoader: Async reloading {} resources\n", resources.size());

	std::vector<Resource *> resourcesToReload;
	for (Resource *rs : resources)
	{
		if (rs == nullptr)
			continue;

		if (cv::debug_rm.getBool())
			debugLog("AsyncResourceLoader: Async reloading {:s}\n", rs->getName());

		bool isBeingLoaded = isLoadingResource(rs);

		if (!isBeingLoaded)
		{
			rs->release();
			resourcesToReload.push_back(rs);
		}
		else if (cv::debug_rm.getBool())
		{
			debugLog("AsyncResourceLoader: Resource {:s} is currently being loaded, skipping reload\n", rs->getName());
		}
	}

	for (Resource *rs : resourcesToReload)
	{
		requestAsyncLoad(rs);
	}
}

bool AsyncResourceLoader::isLoadingResource(Resource *resource) const
{
	std::lock_guard<std::mutex> lock(m_loadingResourcesMutex);
	return m_loadingResources.find(resource) != m_loadingResources.end();
}

void AsyncResourceLoader::ensureThreadAvailable()
{
	size_t activeThreads = m_activeThreadCount.load();
	size_t activeWorkCount = m_activeWorkCount.load();

	if (activeWorkCount > activeThreads && activeThreads < m_maxThreads)
	{
		std::lock_guard<std::mutex> lock(m_threadsMutex);

		if (m_threads.size() < m_maxThreads)
		{
			auto loaderThread = std::make_unique<LoaderThread>(this, m_totalThreadsCreated.fetch_add(1));

			loaderThread->thread = std::make_unique<McThread>(asyncResourceLoaderThread, loaderThread.get());
			if (!loaderThread->thread->isReady())
			{
				if (cv::debug_rm.getBool())
					debugLog("AsyncResourceLoader Warning: Couldn't create dynamic thread!\n");
			}
			else
			{
				if (cv::debug_rm.getBool())
					debugLog("AsyncResourceLoader: Created dynamic thread #{} (total: {})\n", loaderThread->threadIndex, m_threads.size() + 1);

				m_threads.push_back(std::move(loaderThread));
			}
		}
	}
}

void AsyncResourceLoader::cleanupIdleThreads()
{
	if (m_threads.size() == 0)
		return;

	std::lock_guard<std::mutex> lock(m_threadsMutex);

	if (m_threads.size() == 0)
		return;

	// always remove threads that have requested stop, regardless of minimum
	// the minimum will be maintained by ensureThreadAvailable() when needed
	std::erase_if(m_threads, [&](const std::unique_ptr<LoaderThread> &thread) {
		if (thread->thread && thread->thread->isStopRequested())
		{
			if (cv::debug_rm.getBool())
				Engine::logRaw("[cleanupIdleThreads] AsyncResourceLoader: Cleaning up thread #{}\n", thread->threadIndex);
			return true;
		}
		return false;
	});
}

std::unique_ptr<AsyncResourceLoader::LoadingWork> AsyncResourceLoader::getNextPendingWork()
{
	std::lock_guard<std::mutex> lock(m_workQueueMutex);

	if (m_pendingWork.empty())
		return nullptr;

	auto work = std::move(m_pendingWork.front());
	m_pendingWork.pop();
	return work;
}

void AsyncResourceLoader::markWorkAsyncComplete(std::unique_ptr<LoadingWork> work)
{
	std::lock_guard<std::mutex> lock(m_workQueueMutex);
	m_asyncCompleteWork.push(std::move(work));
}

std::unique_ptr<AsyncResourceLoader::LoadingWork> AsyncResourceLoader::getNextAsyncCompleteWork()
{
	std::lock_guard<std::mutex> lock(m_workQueueMutex);

	if (m_asyncCompleteWork.empty())
		return nullptr;

	auto work = std::move(m_asyncCompleteWork.front());
	m_asyncCompleteWork.pop();
	return work;
}
