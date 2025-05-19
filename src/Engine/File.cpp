//================ Copyright (c) 2016, PG, All rights reserved. =================//
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

ConVar *McFile::debug = &debug_file;
ConVar *McFile::size_max = &file_size_max;

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

	UString targetFilename(filename.c_str());

	// now compare all of the directory entries case-insensitively to try finding it
	for (const auto &entry : fs::directory_iterator(parentPath))
	{
		if (!entry.is_regular_file() && !entry.is_directory())
			continue;

		UString entryFilename(entry.path().filename().string().c_str());

		if (entryFilename.equalsIgnoreCase(targetFilename))
		{
			if (entry.is_regular_file())
				retType = McFile::FILETYPE::FILE;
			else if (entry.is_directory())
				retType = McFile::FILETYPE::FOLDER;
			else
				retType = McFile::FILETYPE::OTHER;

			filePath = UString(parentPath.string().c_str());
			if (!filePath.endsWith('/') && !filePath.endsWith('\\'))
				filePath.append('/');
			filePath.append(entryFilename);

			if (McFile::debug->getBool())
				debugLog("File: Case-insensitive match found for %s -> %s\n", path.string().c_str(), filePath.toUtf8());
		}
	}

	if (retType == McFile::FILETYPE::MAYBE_INSENSITIVE)
		return McFile::FILETYPE::NONE; // wasn't found event insensitively

	path = fs::path(filePath.plat_str()); // update filesystem path representation with found path

	return retType;
}

McFile::FILETYPE McFile::exists(const UString &filePath, const fs::path &path)
{
	if (filePath.isEmpty())
		return McFile::FILETYPE::NONE;

	std::error_code ec;
	auto status = fs::status(path, ec);

	if (ec)
	{
		if (McFile::debug->getBool())
			debugLog("File Error: Status check failed for %s with error: %s\n", filePath.toUtf8(), ec.message().c_str());
		return McFile::FILETYPE::NONE;
	}

	if (status.type() != fs::file_type::not_found)
	{
		if (status.type() == fs::file_type::regular)
			return McFile::FILETYPE::FILE;
		else if (status.type() == fs::file_type::directory)
			return McFile::FILETYPE::FOLDER;
		else
			return McFile::FILETYPE::OTHER;
	}

	return McFile::FILETYPE::MAYBE_INSENSITIVE;
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
				debugLog("File Error: Path %s %s\n", filePath.toUtf8(), fileType == McFile::FILETYPE::NONE ? "doesn't exist" : "is not a file");
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
		auto path = fs::path(m_filePath.plat_str());
		// only try to create parent directories if there is a parent path
		if (!path.parent_path().empty())
		{
			std::error_code ec;
			fs::create_directories(path.parent_path(), ec);
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
