//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// purpose:		file wrapper, for cross-platform unicode path support
//
// $NoKeywords: $file
//===============================================================================//

#include "File.h"
#include "ConVar.h"
#include "Engine.h"
#include <algorithm>
#include <filesystem>
#include <utility>

ConVar debug_file("debug_file", false, FCVAR_NONE);
ConVar file_size_max("file_size_max", 1024, FCVAR_NONE, "maximum filesize sanity limit in MB, all files bigger than this are not allowed to load");

ConVar *McFile::debug = &debug_file;
ConVar *McFile::size_max = &file_size_max;

McFile::McFile(UString filePath, TYPE type) : m_filePath(filePath), m_type(type), m_ready(false), m_fileSize(0)
{
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
	auto path = std::filesystem::path(filePath.wc_str());
#else
	auto path = std::filesystem::path(filePath.toUtf8());
#endif

	if (type == TYPE::READ)
	{
		// check if path exists and is a regular file
		if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
		{
			debugLog("File Error: Path %s does not exist or is not a regular file\n", filePath.toUtf8());
			return;
		}

		// create and open input file stream
		m_ifstream = std::make_unique<std::ifstream>();
		m_ifstream->open(path, std::ios::in | std::ios::binary);

		// check if file opened successfully
		if (!m_ifstream->good())
		{
			debugLog("File Error: Couldn't open file %s\n", filePath.toUtf8());
			return;
		}

		// get file size
		m_fileSize = std::filesystem::file_size(path);

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
			debugLog("File Error: FileSize of %s is > %i MB!!!\n", filePath.toUtf8(), McFile::size_max->getInt());
			return;
		}

		// check if it's a directory with an additional sanity check
		std::string tempLine;
		std::getline(*m_ifstream, tempLine);
		if (!m_ifstream->good() && m_fileSize < 1)
		{
			debugLog("File Error: File %s is a directory.\n", filePath.toUtf8());
			return;
		}
		m_ifstream->clear();
		m_ifstream->seekg(0, std::ios::beg);
	}
	else // WRITE
	{
		// only try to create parent directories if there is a parent path
		if (!path.parent_path().empty())
		{
			std::error_code ec;
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec)
			{
				debugLog("File Error: Couldn't create parent directories for %s (error: %s)\n", filePath.toUtf8(), ec.message().c_str());
				// continue anyway - the file open might still succeed if the directory exists
			}
		}

		// create and open output file stream
		m_ofstream = std::make_unique<std::ofstream>();
		m_ofstream->open(path, std::ios::out | std::ios::trunc | std::ios::binary);

		// check if file opened successfully
		if (!m_ofstream->good())
		{
			debugLog("File Error: Couldn't open file %s for writing\n", filePath.toUtf8());
			return;
		}
	}

	if (McFile::debug->getBool())
		debugLog("File: Opening %s\n", filePath.toUtf8());

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
	const int size = getFileSize();
	if (size < 1)
		return "";

	return {readFile(), size};
}

const char *McFile::readFile()
{
	if (McFile::debug->getBool())
		debugLog("File::readFile() on %s\n", m_filePath.toUtf8());

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
