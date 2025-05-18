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

using std::filesystem::exists;
using std::filesystem::is_regular_file;
using std::filesystem::path;

ConVar debug_file("debug_file", false, FCVAR_NONE);
ConVar file_size_max("file_size_max", 1024, FCVAR_NONE, "maximum filesize sanity limit in MB, all files bigger than this are not allowed to load");

ConVar *McFile::debug = &debug_file;
ConVar *McFile::size_max = &file_size_max;

McFile::McFile(UString filePath, TYPE type) : m_filePath(filePath), m_type(type), m_ready(false), m_fileSize(0), m_bExists(false)
{
	if (filePath.isEmpty())
		return;

	auto path = ::path(m_filePath.plat_str());
	if (type == TYPE::CHECK || type == TYPE::READ)
	{
		// case-insensitive lookup if exact match fails
		if (!tryFindCaseInsensitive(m_filePath))
		{
			if (debug_file.getBool()) // let the caller print that if it wants to
				debugLog("File Error: Path %s does not exist\n", filePath.toUtf8());
			return;
		}
	}

	m_bExists = true;

	if (type == TYPE::CHECK) // don't do anything else
		return;

	if (type == TYPE::READ)
	{
		auto path = std::filesystem::path(m_filePath.plat_str());
		if (!std::filesystem::is_regular_file(path))
		{
			debugLog("File Error: %s is not a regular file\n", m_filePath.toUtf8());
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
	return m_bExists && m_ready && m_ifstream && m_ifstream->good() && m_type == TYPE::READ;
}

bool McFile::canWrite() const
{
	return m_bExists && m_ready && m_ofstream && m_ofstream->good() && m_type == TYPE::WRITE;
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

bool McFile::tryFindCaseInsensitive(UString &filePath)
{
	auto path = std::filesystem::path(filePath.plat_str()); // direct match
	if (std::filesystem::exists(path))
		return true;

	if constexpr (Env::cfg(OS::WINDOWS)) // no point in continuing, windows is already case insensitive
		return false;

	auto parentPath = path.parent_path();
	auto filename = path.filename().string();

	// don't try scanning all the way back to the root directory lol, that would be horrendous
	if (!std::filesystem::exists(parentPath) || !std::filesystem::is_directory(parentPath))
		return false;

	UString targetFilename(filename.c_str());

	// now compare all of the directory entries case-insensitively to try finding it
	for (const auto &entry : std::filesystem::directory_iterator(parentPath))
	{
		if (!entry.is_regular_file())
			continue;

		UString entryFilename(entry.path().filename().string().c_str());

		if (entryFilename.equalsIgnoreCase(targetFilename))
		{
			filePath = UString(parentPath.string().c_str());
			if (!filePath.endsWith('/') && !filePath.endsWith('\\'))
				filePath.append('/');
			filePath.append(entryFilename);

			if (McFile::debug->getBool())
				debugLog("File: Case-insensitive match found for %s -> %s\n", path.string().c_str(), filePath.toUtf8());

			return true;
		}
	}

	return false;
}
