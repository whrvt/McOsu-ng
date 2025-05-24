//========== Copyright (c) 2016, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		file wrapper, for cross-platform unicode path support
//
// $NoKeywords: $file
//===============================================================================//

#pragma once
#ifndef FILE_H
#define FILE_H

#include "cbase.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class ConVar;

class McFile
{
public:
	static ConVar *debug;
	static ConVar *size_max;

	enum class TYPE : uint8_t
	{
		READ,
		WRITE
	};

	enum class FILETYPE : uint8_t
	{
		NONE,
		FILE,
		FOLDER,
		MAYBE_INSENSITIVE,
		OTHER
	};

public:
	McFile(UString filePath, TYPE type = TYPE::READ);
	~McFile() = default;

	bool canRead() const;
	bool canWrite() const;

	void write(const char *buffer, size_t size);

	UString readLine();
	UString readString();
	const char *readFile(); // WARNING: this is NOT a null-terminated string! DO NOT USE THIS with UString/std::string!

	size_t getFileSize() const;
	[[nodiscard]] inline UString getPath() const { return m_filePath; }
	[[nodiscard]] static McFile::FILETYPE existsCaseInsensitive(
	    UString &filePath); // modifies the input string with the actual found (case-insensitive-past-last-slash) path!
	[[nodiscard]] static McFile::FILETYPE exists(const UString &filePath);

	// Cache management
	static void clearDirectoryCache();
	static void clearDirectoryCache(const UString &directoryPath);

private:
	// for non-windows file/folder finding directory caching
	struct CaseInsensitiveHash
	{
		size_t operator()(const UString &str) const
		{
			size_t hash = 0;
			for (wchar_t c : str.unicodeView())
			{
				hash = hash * 31 + std::towlower(c);
			}
			return hash;
		}
	};

	// Case-insensitive string comparator
	struct CaseInsensitiveEqual
	{
		bool operator()(const UString &lhs, const UString &rhs) const { return lhs.equalsIgnoreCase(rhs); }
	};

	// Directory cache entry
	struct DirectoryCacheEntry
	{
		std::unordered_map<UString, std::pair<UString, FILETYPE>, CaseInsensitiveHash, CaseInsensitiveEqual> files;
		std::chrono::steady_clock::time_point lastAccess;
		std::filesystem::file_time_type lastModified;
	};

	[[nodiscard]] static McFile::FILETYPE existsCaseInsensitive(
	    UString &filePath, std::filesystem::path &path); // modifies the input string with the actual found (case-insensitive-past-last-slash) path!
	[[nodiscard]] static McFile::FILETYPE exists(const UString &filePath, const std::filesystem::path &path);

	// Directory cache methods
	static DirectoryCacheEntry *getOrCreateDirectoryCache(const std::filesystem::path &dirPath);
	static void evictOldCacheEntries();

	UString m_filePath;
	TYPE m_type;
	bool m_ready;
	size_t m_fileSize;

	// file streams
	std::unique_ptr<std::ifstream> m_ifstream;
	std::unique_ptr<std::ofstream> m_ofstream;

	// buffer for full file reading
	std::vector<char> m_fullBuffer;

	// Directory cache for case-insensitive lookups
	static std::unordered_map<UString, DirectoryCacheEntry> s_directoryCache;
	static std::mutex s_directoryCacheMutex;
};

#endif
