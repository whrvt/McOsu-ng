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
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

class ConVar;

class DirectoryCache;

class McFile
{
public:
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

	[[nodiscard]] constexpr bool canRead() const { return m_ready && m_ifstream && m_ifstream->good() && m_type == TYPE::READ; }
	[[nodiscard]] constexpr bool canWrite() const { return m_ready && m_ofstream && m_ofstream->good() && m_type == TYPE::WRITE; }

	void write(const char *buffer, size_t size);
	bool writeLine(const UString &line, bool insertNewline = true);

	UString readLine();
	UString readString();
	const char *readFile();                           // WARNING: this is NOT a null-terminated string! DO NOT USE THIS with UString/std::string!
	[[nodiscard]] std::vector<char> takeFileBuffer(); // moves the file buffer out, allowing immediate destruction of the file object

	[[nodiscard]] constexpr size_t getFileSize() const { return m_fileSize; }
	[[nodiscard]] inline UString getPath() const { return m_filePath; }

	// public path resolution methods
	[[nodiscard]] static McFile::FILETYPE existsCaseInsensitive(UString &filePath); // modifies the input path with the actual found path
	[[nodiscard]] static McFile::FILETYPE exists(const UString &filePath);

private:
	UString m_filePath;
	TYPE m_type;
	bool m_ready;
	size_t m_fileSize;

	// file streams
	std::unique_ptr<std::ifstream> m_ifstream;
	std::unique_ptr<std::ofstream> m_ofstream;

	// buffer for full file reading
	std::vector<char> m_fullBuffer;

	// private implementation helpers
	bool openForReading();
	bool openForWriting();

	// internal path resolution helpers
	[[nodiscard]] static McFile::FILETYPE existsCaseInsensitive(UString &filePath, std::filesystem::path &path);
	[[nodiscard]] static McFile::FILETYPE exists(const UString &filePath, const std::filesystem::path &path);

	// directory cache
	static std::unique_ptr<DirectoryCache> s_directoryCache;
};

#endif
