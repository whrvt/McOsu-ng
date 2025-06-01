//========== Copyright (c) 2012, PG & 2025, WH, All rights reserved. ============//
//
// purpose:		file wrapper, for cross-platform unicode path support
//
// $NoKeywords: $file
//===============================================================================//

#include "File.h"
#include "ConVar.h"
#include "Engine.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace cv {
ConVar debug_file("debug_file", false, FCVAR_NONE);
ConVar file_size_max("file_size_max", 1024, FCVAR_NONE, "maximum filesize sanity limit in MB, all files bigger than this are not allowed to load");
}

//------------------------------------------------------------------------------
// encapsulation of directory caching logic
//------------------------------------------------------------------------------
class DirectoryCache final
{
public:
	DirectoryCache() = default;

	// case-insensitive string utilities
	struct CaseInsensitiveHash
	{
		size_t operator()(const UString &str) const
		{
			size_t hash = 0;
			for (auto &c : str.plat_view())
			{
				if constexpr (Env::cfg(OS::WINDOWS))
					hash = hash * 31 + std::towlower(c);
				else
					hash = hash * 31 + std::tolower(c);
			}
			return hash;
		}
	};

	struct CaseInsensitiveEqual
	{
		bool operator()(const UString &lhs, const UString &rhs) const { return lhs.equalsIgnoreCase(rhs); }
	};

	// directory entry type
	struct DirectoryEntry
	{
		std::unordered_map<UString, std::pair<UString, McFile::FILETYPE>, CaseInsensitiveHash, CaseInsensitiveEqual> files;
		std::chrono::steady_clock::time_point lastAccess;
		fs::file_time_type lastModified;
	};

	// look up a file with case-insensitive matching
	std::pair<UString, McFile::FILETYPE> lookup(const fs::path &dirPath, const UString &filename)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		UString dirKey(dirPath.string());
		auto it = m_cache.find(dirKey);

		DirectoryEntry *entry = nullptr;

		// check if cache exists and is still valid
		if (it != m_cache.end())
		{
			// check if directory has been modified
			std::error_code ec;
			auto currentModTime = fs::last_write_time(dirPath, ec);

			if (!ec && currentModTime != it->second.lastModified)
				m_cache.erase(it); // cache is stale, remove it
			else
				entry = &it->second;
		}

		// create new entry if needed
		if (!entry)
		{
			// evict old entries if we're at capacity
			if (m_cache.size() >= DIR_CACHE_MAX_ENTRIES)
				evictOldEntries();

			// build new cache entry
			DirectoryEntry newEntry;
			newEntry.lastAccess = std::chrono::steady_clock::now();

			std::error_code ec;
			newEntry.lastModified = fs::last_write_time(dirPath, ec);

			// scan directory and populate cache
			for (const auto &dirEntry : fs::directory_iterator(dirPath, ec))
			{
				if (ec)
					break;

				UString filename(dirEntry.path().filename().string());
				McFile::FILETYPE type = McFile::FILETYPE::OTHER;

				if (dirEntry.is_regular_file())
					type = McFile::FILETYPE::FILE;
				else if (dirEntry.is_directory())
					type = McFile::FILETYPE::FOLDER;

				// store both the actual filename and its type
				newEntry.files[filename] = {filename, type};
			}

			// insert into cache
			auto [insertIt, inserted] = m_cache.emplace(dirKey, std::move(newEntry));
			entry = inserted ? &insertIt->second : nullptr;
		}

		if (!entry)
			return {{}, McFile::FILETYPE::NONE};

		// update last access time
		entry->lastAccess = std::chrono::steady_clock::now();

		// find the case-insensitive match
		auto fileIt = entry->files.find(filename);
		if (fileIt != entry->files.end())
			return fileIt->second;

		return {{}, McFile::FILETYPE::NONE};
	}

private:
	static constexpr size_t DIR_CACHE_MAX_ENTRIES = 100;
	static constexpr size_t DIR_CACHE_EVICT_COUNT = DIR_CACHE_MAX_ENTRIES / 4;

	// evict least recently used entries when cache is full
	void evictOldEntries()
	{
		const size_t entriesToRemove = std::min(DIR_CACHE_EVICT_COUNT, m_cache.size());

		if (entriesToRemove == m_cache.size())
		{
			m_cache.clear();
			return;
		}

		// collect entries with their access times
		std::vector<std::pair<std::chrono::steady_clock::time_point, decltype(m_cache)::iterator>> entries;
		entries.reserve(m_cache.size());

		for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
			entries.emplace_back(it->second.lastAccess, it);

		// sort by access time (oldest first)
		std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.first < b.first; });

		// remove the oldest entries
		for (size_t i = 0; i < entriesToRemove; ++i)
			m_cache.erase(entries[i].second);
	}

	// cache storage
	std::unordered_map<UString, DirectoryEntry> m_cache;

	// thread safety
	std::mutex m_mutex;
};

// init static directory cache
std::unique_ptr<DirectoryCache> McFile::s_directoryCache = std::make_unique<DirectoryCache>();

//------------------------------------------------------------------------------
// path resolution methods
//------------------------------------------------------------------------------
// public
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

// private (cache the fs::path)
McFile::FILETYPE McFile::existsCaseInsensitive(UString &filePath, fs::path &path)
{
	auto retType = McFile::exists(filePath, path);

	if (retType == McFile::FILETYPE::NONE)
		return McFile::FILETYPE::NONE;
	else if (!(retType == McFile::FILETYPE::MAYBE_INSENSITIVE))
		return retType; // direct match

	// no point in continuing, windows is already case insensitive
	if constexpr (Env::cfg(OS::WINDOWS))
		return McFile::FILETYPE::NONE;

	auto parentPath = path.parent_path();

	// verify parent directory exists
	std::error_code ec;
	auto parentStatus = fs::status(parentPath, ec);
	if (ec || parentStatus.type() != fs::file_type::directory)
		return McFile::FILETYPE::NONE;

	// try case-insensitive lookup using cache
	auto [resolvedName, fileType] = s_directoryCache->lookup(parentPath, {path.filename().string()}); // takes the bare filename

	if (fileType == McFile::FILETYPE::NONE)
		return McFile::FILETYPE::NONE; // no match, even case-insensitively

	UString resolvedPath(parentPath.string());
	if (!resolvedPath.endsWith('/') && !resolvedPath.endsWith('\\'))
		resolvedPath.append('/');
	resolvedPath.append(resolvedName);

	if (cv::debug_file.getBool())
		debugLog("File: Case-insensitive match found for {:s} -> {:s}\n", path.string(), resolvedPath);

	// now update the given paths with the actual found path
	filePath = resolvedPath;
	path = fs::path(filePath.plat_str());
	return fileType;
}

McFile::FILETYPE McFile::exists(const UString &filePath, const fs::path &path)
{
	if (filePath.isEmpty())
		return McFile::FILETYPE::NONE;

	std::error_code ec;
	auto status = fs::status(path, ec);

	if (ec || status.type() == fs::file_type::not_found)
		return McFile::FILETYPE::MAYBE_INSENSITIVE; // path not found, try case-insensitive lookup

	if (status.type() == fs::file_type::regular)
		return McFile::FILETYPE::FILE;
	else if (status.type() == fs::file_type::directory)
		return McFile::FILETYPE::FOLDER;
	else
		return McFile::FILETYPE::OTHER;
}

//------------------------------------------------------------------------------
// McFile implementation
//------------------------------------------------------------------------------
McFile::McFile(UString filePath, TYPE type) : m_filePath(filePath), m_type(type), m_ready(false), m_fileSize(0)
{
	if (type == TYPE::READ)
	{
		if (!openForReading())
			return;
	}
	else if (type == TYPE::WRITE)
	{
		if (!openForWriting())
			return;
	}

	if (cv::debug_file.getBool())
		debugLog("File: Opening {:s}\n", filePath);

	m_ready = true;
}

bool McFile::openForReading()
{
	// resolve the file path (handles case-insensitive matching)
	fs::path path(m_filePath.plat_str());
	auto fileType = existsCaseInsensitive(m_filePath, path);

	if (fileType != McFile::FILETYPE::FILE)
	{
		if (cv::debug_file.getBool())
			debugLog("File Error: Path {:s} {:s}\n", m_filePath, fileType == McFile::FILETYPE::NONE ? "doesn't exist" : "is not a file");
		return false;
	}

	// create and open input file stream
	m_ifstream = std::make_unique<std::ifstream>();
	m_ifstream->open(path, std::ios::in | std::ios::binary);

	// check if file opened successfully
	if (!m_ifstream || !m_ifstream->good())
	{
		debugLog("File Error: Couldn't open file {:s}\n", m_filePath);
		return false;
	}

	// get file size
	std::error_code ec;
	m_fileSize = fs::file_size(path, ec);

	if (ec)
	{
		debugLog("File Error: Couldn't get file size for {:s}\n", m_filePath);
		return false;
	}

	// validate file size
	if (m_fileSize == 0) // empty file is valid
		return true;
	else if (m_fileSize < 0)
	{
		debugLog("File Error: FileSize is < 0\n");
		return false;
	}
	else if (std::cmp_greater(m_fileSize, 1024 * 1024 * cv::file_size_max.getInt())) // size sanity check
	{
		debugLog("File Error: FileSize of {:s} is > {} MB!!!\n", m_filePath, cv::file_size_max.getInt());
		return false;
	}

	return true;
}

bool McFile::openForWriting()
{
	// get filesystem path
	fs::path path(m_filePath.plat_str());

	// create parent directories if needed
	if (!path.parent_path().empty())
	{
		std::error_code ec;
		fs::create_directories(path.parent_path(), ec);
		if (ec)
		{
			debugLog("File Error: Couldn't create parent directories for {:s} (error: {:s})\n", m_filePath, ec.message());
			// continue anyway, the file open might still succeed if the directory exists
		}
	}

	// create and open output file stream
	m_ofstream = std::make_unique<std::ofstream>();
	m_ofstream->open(path, std::ios::out | std::ios::trunc | std::ios::binary);

	// check if file opened successfully
	if (!m_ofstream->good())
	{
		debugLog("File Error: Couldn't open file {:s} for writing\n", m_filePath);
		return false;
	}

	return true;
}

void McFile::write(const char *buffer, size_t size)
{
	if (!canWrite())
		return;

	m_ofstream->write(buffer, static_cast<std::streamsize>(size));
}

bool McFile::writeLine(const UString &line, bool insertNewline) {
	if (!canWrite())
		return false;

	std::string writeLine = line.toUtf8();
	if (insertNewline)
		writeLine = writeLine + "\n";
	m_ofstream->write(writeLine.c_str(), static_cast<std::streamsize>(writeLine.length()));
	return !m_ofstream->bad();
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

		return {line};
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
	if (cv::debug_file.getBool())
		debugLog("File::readFile() on {:s}\n", m_filePath);

	// return cached buffer if already read
	if (!m_fullBuffer.empty())
		return m_fullBuffer.data();

	if (!m_ready || !canRead())
		return nullptr;

	// allocate buffer for file contents
	m_fullBuffer.resize(m_fileSize);

	// read entire file
	m_ifstream->seekg(0, std::ios::beg);
	if (m_ifstream->read(m_fullBuffer.data(), static_cast<std::streamsize>(m_fileSize)))
		return m_fullBuffer.data();

	return nullptr;
}
