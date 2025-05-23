//================ Copyright (c) 2016, PG, All rights reserved. =================//
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
#include <string>
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
		WRITE,
		CHECK
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
	[[nodiscard]] inline bool exists() const { return m_bExists; }

private:
	// fallback for case-insensitive file finding (i.e. Skin.ini vs skin.ini)
	bool tryFindCaseInsensitive(UString& filePath);

	UString m_filePath;
	TYPE m_type;
	bool m_ready;
	bool m_bExists;
	size_t m_fileSize;

	// file streams
	std::unique_ptr<std::ifstream> m_ifstream;
	std::unique_ptr<std::ofstream> m_ofstream;

	// buffer for full file reading
	std::vector<char> m_fullBuffer;
};

#endif
