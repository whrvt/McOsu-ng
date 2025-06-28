//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		asynchronous resource loading system
//
// $NoKeywords: $arl
//===============================================================================//

#pragma once
#ifndef ASYNCRESOURCELOADER_H
#define ASYNCRESOURCELOADER_H

#include "Resource.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <vector>

class ConVar;
class McThread;

// everything is public because the class data should only be accessed by ResourceManager and the resource threads themselves
class AsyncResourceLoader final
{
public:
	AsyncResourceLoader();
	~AsyncResourceLoader();

	AsyncResourceLoader &operator=(const AsyncResourceLoader &) = delete;
	AsyncResourceLoader &operator=(AsyncResourceLoader &&) = delete;

	AsyncResourceLoader(const AsyncResourceLoader &) = delete;
	AsyncResourceLoader(AsyncResourceLoader &&) = delete;

	// main interface for ResourceManager
	void requestAsyncLoad(Resource *resource);
	void update(bool lowLatency);
	void shutdown();

	// resource lifecycle management
	void scheduleAsyncDestroy(Resource *resource);
	void reloadResources(const std::vector<Resource *> &resources);

	// status queries
	[[nodiscard]] inline bool isLoading() const { return m_activeWorkCount.load() > 0; }
	[[nodiscard]] bool isLoadingResource(Resource *resource) const;
	[[nodiscard]] size_t getNumLoadingWork() const { return m_activeWorkCount.load(); }
	[[nodiscard]] size_t getNumActiveThreads() const { return m_activeThreadCount.load(); }
	[[nodiscard]] inline size_t getNumLoadingWorkAsyncDestroy() const { return m_asyncDestroyQueue.size(); }

	enum class WorkState : uint8_t
	{
		PENDING = 0,
		ASYNC_IN_PROGRESS = 1,
		ASYNC_COMPLETE = 2,
		SYNC_COMPLETE = 3
	};

	struct LoadingWork
	{
		Resource *resource;
		size_t workId;
		std::atomic<WorkState> state{WorkState::PENDING};

		LoadingWork(Resource *res, size_t id) : resource(res), workId(id) {}
	};

	class LoaderThread;
	friend class LoaderThread;

	// threading configuration
	static constexpr int MIN_NUM_THREADS = 1;
	static constexpr auto THREAD_IDLE_TIMEOUT = std::chrono::seconds{5};

	// thread management
	void ensureThreadAvailable();
	void cleanupIdleThreads();

	// work queue management
	std::unique_ptr<LoadingWork> getNextPendingWork();
	void markWorkAsyncComplete(std::unique_ptr<LoadingWork> work);
	std::unique_ptr<LoadingWork> getNextAsyncCompleteWork();

	size_t m_maxThreads;
	std::chrono::seconds m_threadIdleTimeout{5};

	// thread pool
	std::vector<std::unique_ptr<LoaderThread>> m_threads;
	mutable std::mutex m_threadsMutex;

	// thread lifecycle tracking
	std::atomic<size_t> m_activeThreadCount{0};
	std::atomic<size_t> m_totalThreadsCreated{0};

	// separate queues for different work states (avoids O(n) scanning)
	std::queue<std::unique_ptr<LoadingWork>> m_pendingWork;
	std::queue<std::unique_ptr<LoadingWork>> m_asyncCompleteWork;

	// single mutex for both work queues (they're accessed in sequence, not concurrently)
	mutable std::mutex m_workQueueMutex;

	// fast lookup for checking if a resource is being loaded
	std::unordered_set<Resource *> m_loadingResources;
	mutable std::mutex m_loadingResourcesMutex;

	// atomic counters for efficient status queries
	std::atomic<size_t> m_activeWorkCount{0};
	std::atomic<size_t> m_workIdCounter{0};

	// work notification
	std::condition_variable m_workAvailable;
	std::mutex m_workAvailableMutex;

	// async destroy queue
	std::vector<Resource *> m_asyncDestroyQueue;
	std::mutex m_asyncDestroyMutex;

	// lifecycle flags
	std::atomic<bool> m_shuttingDown{false};
};

#endif
