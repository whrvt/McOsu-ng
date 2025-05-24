//========== Copyright (c) 2012, PG & 2025, WH, All rights reserved. ============//
//
// purpose:		file wrapper, for cross-platform unicode path support
//
// $NoKeywords: $file
//===============================================================================//

#include "File.h"
#include "ConVar.h"
#include "Engine.h"
#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

ConVar debug_file("debug_file", false, FCVAR_NONE);
ConVar file_size_max("file_size_max", 1024, FCVAR_NONE, "maximum filesize sanity limit in MB, all files bigger than this are not allowed to load");
ConVar file_directory_cache_max_size("file_directory_cache_max_size", 100, FCVAR_NONE);

ConVar *McFile::debug = &debug_file;
ConVar *McFile::size_max = &file_size_max;

// static member initialization
// TODO: move this into ResourceManager, it makes more sense that way
std::unordered_map<UString, McFile::DirectoryCacheEntry> McFile::s_directoryCache;
std::mutex McFile::s_directoryCacheMutex;

// cache management methods
void McFile::clearDirectoryCache()
{
	std::lock_guard<std::mutex> lock(s_directoryCacheMutex);
	s_directoryCache.clear();
}

void McFile::clearDirectoryCache(const UString &directoryPath)
{
	std::lock_guard<std::mutex> lock(s_directoryCacheMutex);
	s_directoryCache.erase(directoryPath);
}

// create a fast lookup cache for directory entries so we don't re-scan the same directory over and over again to
// to find a case-insensitively-matched file from a given directory
McFile::DirectoryCacheEntry *McFile::getOrCreateDirectoryCache(const fs::path &dirPath)
{
	std::lock_guard<std::mutex> lock(s_directoryCacheMutex);

	// evict old entries if we're at capacity
	if (s_directoryCache.size() >= file_directory_cache_max_size.getInt())
		evictOldCacheEntries();

	UString dirKey(dirPath.string());
	auto it = s_directoryCache.find(dirKey);

	// check if cache exists and is still valid
	if (it != s_directoryCache.end())
	{
		auto now = std::chrono::steady_clock::now();
		auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastAccess);

		// check if cache is too old or directory has been modified
		std::error_code ec;
		auto currentModTime = fs::last_write_time(dirPath, ec);

		if (age > static_cast<std::chrono::seconds>(300) || (!ec && currentModTime != it->second.lastModified))
		{
			// cache is stale, remove it
			s_directoryCache.erase(it);
		}
		else
		{
			// update last access time
			it->second.lastAccess = now;
			return &it->second;
		}
	}

	// build new cache entry
	DirectoryCacheEntry newEntry;
	newEntry.lastAccess = std::chrono::steady_clock::now();

	std::error_code ec;
	newEntry.lastModified = fs::last_write_time(dirPath, ec);

	// scan directory and populate cache
	for (const auto &entry : fs::directory_iterator(dirPath, ec))
	{
		if (ec)
			break;

		UString filename(entry.path().filename().string());
		FILETYPE type = FILETYPE::OTHER;

		if (entry.is_regular_file())
			type = FILETYPE::FILE;
		else if (entry.is_directory())
			type = FILETYPE::FOLDER;

		// store both the actual filename and its type
		newEntry.files[filename] = {filename, type};
	}

	// insert into cache and return pointer
	auto [insertIt, inserted] = s_directoryCache.emplace(dirKey, std::move(newEntry));
	return inserted ? &insertIt->second : nullptr;
}

void McFile::evictOldCacheEntries()
{
	// LRU evict
	if (s_directoryCache.empty())
		return;

	auto oldest = s_directoryCache.begin();
	auto oldestTime = oldest->second.lastAccess;

	for (auto it = s_directoryCache.begin(); it != s_directoryCache.end(); ++it)
	{
		if (it->second.lastAccess < oldestTime)
		{
			oldest = it;
			oldestTime = it->second.lastAccess;
		}
	}

	s_directoryCache.erase(oldest);
}

// internal use versions of the above, to prevent re-creating new fs::paths constantly
McFile::FILETYPE McFile::existsCaseInsensitive(UString &filePath, fs::path &path)
{
	auto retType = McFile::exists(filePath, path);

	if (retType == McFile::FILETYPE::NONE)
		return McFile::FILETYPE::NONE;
	else if (!(retType == McFile::FILETYPE::MAYBE_INSENSITIVE))
		return retType; // direct match

	if constexpr (Env::cfg(OS::WINDOWS)) // no point in continuing, windows is already case insensitive
		return McFile::FILETYPE::NONE;

	auto parentPath = path.parent_path();
	auto filename = path.filename().string();

	// don't try scanning all the way back to the root directory lol, that would be horrendous
	std::error_code ec;
	auto parentStatus = fs::status(parentPath, ec);
	if (ec || parentStatus.type() != fs::file_type::directory)
		return McFile::FILETYPE::NONE;

	UString targetFilename(filename);

	// try to use cached directory listing first
	auto cacheEntry = getOrCreateDirectoryCache(parentPath);
	if (cacheEntry)
	{
		// fast case-insensitive lookup in the hash map
		auto it = cacheEntry->files.find(targetFilename);
		if (it != cacheEntry->files.end())
		{
			retType = it->second.second;

			filePath = UString(parentPath.string());
			if (!filePath.endsWith('/') && !filePath.endsWith('\\'))
				filePath.append('/');
			filePath.append(it->second.first); // use the actual filename from cache

			if (McFile::debug->getBool())
				debugLog("File: Case-insensitive match found (cached) for {:s} -> {:s}\n", path.string().c_str(), filePath.toUtf8());

			path = fs::path(filePath.plat_str());
			return retType;
		}
	}

	// not found in cache means it doesn't exist
	return McFile::FILETYPE::NONE;
}

McFile::FILETYPE McFile::exists(const UString &filePath, const fs::path &path)
{
	if (filePath.isEmpty())
		return McFile::FILETYPE::NONE;

	auto status = fs::status(path);

	if (status.type() != fs::file_type::not_found)
	{
		if (status.type() == fs::file_type::regular)
			return McFile::FILETYPE::FILE;
		else if (status.type() == fs::file_type::directory)
			return McFile::FILETYPE::FOLDER;
		else
			return McFile::FILETYPE::OTHER;
	}

	return McFile::FILETYPE::MAYBE_INSENSITIVE; // keep searching
}

// modifies the input string with the actual found (case-insensitive-past-last-slash) path!
McFile::FILETYPE McFile::existsCaseInsensitive(UString &filePath)
{
	auto fsPath = fs::path(filePath.plat_str());
	return existsCaseInsensitive(filePath, fsPath);
}

McFile::FILETYPE McFile::exists(const UString &filePath)
{
	const auto fsPath = fs::path(filePath.plat_str());
	return exists(filePath, fsPath);
}

McFile::McFile(UString filePath, TYPE type) : m_filePath(filePath), m_type(type), m_ready(false), m_fileSize(0)
{
	if (type == TYPE::READ)
	{
		auto path = fs::path(m_filePath.plat_str());
		auto fileType = McFile::FILETYPE::NONE;

		if ((fileType = existsCaseInsensitive(m_filePath, path)) != McFile::FILETYPE::FILE)
		{
			if (McFile::debug->getBool()) // let the caller print that if it wants to
				debugLog("File Error: Path {:s} {:s}\n", filePath.toUtf8(), fileType == McFile::FILETYPE::NONE ? "doesn't exist" : "is not a file");
			return;
		}

		// create and open input file stream
		m_ifstream = std::make_unique<std::ifstream>();
		m_ifstream->open(path, std::ios::in | std::ios::binary);

		// check if file opened successfully
		if (!m_ifstream->good())
		{
			debugLog("File Error: Couldn't open file {:s}\n", filePath.toUtf8());
			return;
		}

		// get file size
		m_fileSize = fs::file_size(path);

		// validate file size
		if (!m_fileSize) // empty
			return;
		else if (m_fileSize < 0)
		{
			debugLog("File Error: FileSize is < 0\n");
			return;
		}
		else if (std::cmp_greater(m_fileSize, 1024 * 1024 * McFile::size_max->getInt())) // size sanity check
		{
			debugLog("File Error: FileSize of {:s} is > {} MB!!!\n", filePath.toUtf8(), McFile::size_max->getInt());
			return;
		}

		// check if it's a directory with an additional sanity check
		std::string tempLine;
		std::getline(*m_ifstream, tempLine);
		if (!m_ifstream->good() && m_fileSize < 1)
		{
			debugLog("File Error: File {:s} is a directory.\n", filePath.toUtf8());
			return;
		}
		m_ifstream->clear();
		m_ifstream->seekg(0, std::ios::beg);
	}
	else // WRITE
	{
		auto path = fs::path(m_filePath.plat_str());
		// only try to create parent directories if there is a parent path
		if (!path.parent_path().empty())
		{
			std::error_code ec;
			fs::create_directories(path.parent_path(), ec);
			if (ec)
			{
				debugLog("File Error: Couldn't create parent directories for {:s} (error: {:s})\n", filePath.toUtf8(), ec.message().c_str());
				// continue anyway - the file open might still succeed if the directory exists
			}
		}

		// create and open output file stream
		m_ofstream = std::make_unique<std::ofstream>();
		m_ofstream->open(path, std::ios::out | std::ios::trunc | std::ios::binary);

		// check if file opened successfully
		if (!m_ofstream->good())
		{
			debugLog("File Error: Couldn't open file {:s} for writing\n", filePath.toUtf8());
			return;
		}
	}

	if (McFile::debug->getBool())
		debugLog("File: Opening {:s}\n", filePath.toUtf8());

	m_ready = true;
}

bool McFile::canRead() const
{
	return m_ready && m_ifstream && m_ifstream->good() && m_type == TYPE::READ;
}

bool McFile::canWrite() const
{
	return m_ready && m_ofstream && m_ofstream->good() && m_type == TYPE::WRITE;
}

void McFile::write(const char *buffer, size_t size)
{
	if (!canWrite())
		return;

	m_ofstream->write(buffer, size);
}

UString McFile::readLine()
{
	if (!canRead())
		return "";

	std::string line;
	if (std::getline(*m_ifstream, line))
	{
		// handle CRLF line endings
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		UString result(line.c_str());

		return result;
	}

	return "";
}

UString McFile::readString()
{
	const int size = static_cast<int>(getFileSize());
	if (size < 1)
		return "";

	return {readFile(), size};
}

const char *McFile::readFile()
{
	if (McFile::debug->getBool())
		debugLog("File::readFile() on {:s}\n", m_filePath.toUtf8());

	// return cached buffer if already read
	if (!m_fullBuffer.empty())
		return m_fullBuffer.data();

	if (!m_ready || !canRead())
		return nullptr;

	// allocate buffer for file contents
	m_fullBuffer.resize(m_fileSize);

	// read entire file
	m_ifstream->seekg(0, std::ios::beg);
	if (m_ifstream->read(m_fullBuffer.data(), m_fileSize))
		return m_fullBuffer.data();

	return nullptr;
}

size_t McFile::getFileSize() const
{
	return m_fileSize;
}
