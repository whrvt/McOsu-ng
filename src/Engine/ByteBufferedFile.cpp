#include "ByteBufferedFile.h"

#include "Engine.h"
#include <system_error>

ByteBufferedFile::Reader::Reader(const std::filesystem::path &path)
    : m_ringBuf(READ_BUFFER_SIZE)
{
	m_inFile.open(path, std::ios::binary);
	if (!m_inFile.is_open())
	{
		setError("Failed to open file for reading: " + std::generic_category().message(errno));
		debugLog("Failed to open '{:s}': {:s}\n", path.string().c_str(), std::generic_category().message(errno).c_str());
		return;
	}

	m_inFile.seekg(0, std::ios::end);
	if (m_inFile.fail())
	{
		goto seek_error;
	}

	m_iTotalSize = m_inFile.tellg();

	m_inFile.seekg(0, std::ios::beg);
	if (m_inFile.fail())
	{
		goto seek_error;
	}

	return; // success

seek_error:
	setError("Failed to initialize file reader: " + std::generic_category().message(errno));
	debugLog("Failed to initialize file reader '{:s}': {:s}\n", path.string().c_str(), std::generic_category().message(errno).c_str());
	m_inFile.close();
	return;
}

void ByteBufferedFile::Reader::setError(const std::string &error_msg)
{
	if (!m_bErrorFlag)
	{ // only set first error
		m_bErrorFlag = true;
		m_sLastError = error_msg;
	}
}

// MD5Hash ByteBufferedFile::Reader::read_hash() {
//     MD5Hash hash;

//     if(m_bErrorFlag) {
//         return hash;
//     }

//     uint8_t empty_check = read<uint8_t>();
//     if(empty_check == 0) return hash;

//     uint32_t len = read_uleb128();
//     uint32_t extra = 0;
//     if(len > 32) {
//         // just continue, don't set error flag
//         debugLog("WARNING: Expected 32 bytes for hash, got {}!\n", len);
//         extra = len - 32;
//         len = 32;
//     }

//     if(read_bytes(reinterpret_cast<uint8_t*>(hash.hash.data()), len) != len) {
//         // just continue, don't set error flag
//         debugLog("WARNING: failed to read {} bytes to obtain hash.\n", len);
//         extra = len;
//     }
//     skip_bytes(extra);
//     hash.hash[len] = '\0';
//     return hash;
// }

std::string ByteBufferedFile::Reader::readString()
{
	if (m_bErrorFlag)
	{
		return {};
	}

	uint8_t empty_check = read<uint8_t>();
	if (empty_check == 0)
		return {};

	uint32_t len = readULEB128();
	auto str = std::make_unique<uint8_t[]>(len + 1);
	if (readBytes(str.get(), len) != len)
	{
		setError("Failed to read " + std::to_string(len) + " bytes for string");
		return {};
	}

	std::string str_out(reinterpret_cast<const char *>(str.get()), len);

	return str_out;
}

uint32_t ByteBufferedFile::Reader::readULEB128()
{
	if (m_bErrorFlag)
	{
		return 0;
	}

	uint32_t result = 0;
	uint32_t shift = 0;
	uint8_t byte = 0;

	do
	{
		byte = read<uint8_t>();
		result |= (byte & 0x7f) << shift;
		shift += 7;
	} while (byte & 0x80);

	return result;
}

void ByteBufferedFile::Reader::skipString()
{
	if (m_bErrorFlag)
	{
		return;
	}

	uint8_t empty_check = read<uint8_t>();
	if (empty_check == 0)
		return;

	uint32_t len = readULEB128();
	skipBytes(len);
}

ByteBufferedFile::Writer::Writer(const std::filesystem::path &path)
    : m_ringBuf(WRITE_BUFFER_SIZE)
{
	m_outFilePath = path;
	m_tmpFilePath = m_outFilePath;
	m_tmpFilePath += ".tmp";

	m_outFile.open(m_tmpFilePath, std::ios::binary);
	if (!m_outFile.is_open())
	{
		setError("Failed to open file for writing: " + std::generic_category().message(errno));
		debugLog("Failed to open '{:s}': {:s}\n", path.string().c_str(), std::generic_category().message(errno).c_str());
		return;
	}
}

ByteBufferedFile::Writer::~Writer()
{
	if (m_outFile.is_open())
	{
		flush();
		m_outFile.close();

		if (!m_bErrorFlag)
		{
			std::error_code ec;
			std::filesystem::remove(m_outFilePath, ec); // Windows (the Microsoft docs are LYING)
			std::filesystem::rename(m_tmpFilePath, m_outFilePath, ec);
			if (ec)
			{
				// can't set error in destructor, but log it
				debugLog("Failed to rename temporary file: {:s}\n", ec.message().c_str());
			}
		}
	}
}

void ByteBufferedFile::Writer::setError(const std::string &error_msg)
{
	if (!m_bErrorFlag)
	{ // only set first error
		m_bErrorFlag = true;
		m_sLastError = error_msg;
	}
}

// void ByteBufferedFile::Writer::write_hash(MD5Hash hash) {
//     if(m_bErrorFlag) {
//         return;
//     }

//     write<uint8_t>(0x0B);
//     write<uint8_t>(0x20);
//     write_bytes(reinterpret_cast<uint8_t*>(hash.hash.data()), 32);
// }

void ByteBufferedFile::Writer::writeString(std::string str)
{
	if (m_bErrorFlag)
	{
		return;
	}

	if (str[0] == '\0')
	{
		uint8_t zero = 0;
		write<uint8_t>(zero);
		return;
	}

	uint8_t empty_check = 11;
	write<uint8_t>(empty_check);

	uint32_t len = str.length();
	writeULEB128(len);
	writeBytes(reinterpret_cast<uint8_t *>(const_cast<char *>(str.c_str())), len);
}

void ByteBufferedFile::Writer::flush()
{
	if (m_bErrorFlag || !m_outFile.is_open())
	{
		return;
	}

	m_outFile.write(reinterpret_cast<const char *>(m_ringBuf.data()), m_iFilePos);
	if (m_outFile.fail())
	{
		setError("Failed to write to file: " + std::generic_category().message(errno));
		return;
	}
	m_iFilePos = 0;
}

void ByteBufferedFile::Writer::writeBytes(uint8_t *bytes, size_t n)
{
	if (m_bErrorFlag || !m_outFile.is_open())
	{
		return;
	}

	if (m_iFilePos + n > WRITE_BUFFER_SIZE)
	{
		flush();
		if (m_bErrorFlag)
		{
			return;
		}
	}

	if (m_iFilePos + n > WRITE_BUFFER_SIZE)
	{
		setError("Attempted to write " + std::to_string(n) + " bytes (exceeding buffer size " + std::to_string(WRITE_BUFFER_SIZE) + ")");
		return;
	}

	memcpy(m_ringBuf.data() + m_iFilePos, bytes, n);
	m_iFilePos += n;
}

void ByteBufferedFile::Writer::writeULEB128(uint32_t num)
{
	if (m_bErrorFlag)
	{
		return;
	}

	if (num == 0)
	{
		uint8_t zero = 0;
		write<uint8_t>(zero);
		return;
	}

	while (num != 0)
	{
		uint8_t next = num & 0x7F;
		num >>= 7;
		if (num != 0)
		{
			next |= 0x80;
		}
		write<uint8_t>(next);
	}
}

void ByteBufferedFile::copy(const std::filesystem::path &from_path, const std::filesystem::path &to_path)
{
	Reader from(from_path);
	Writer to(to_path);

	if (!from.good())
	{
		debugLog("Failed to open source file for copying: {:s}\n", from.error().data());
		return;
	}

	if (!to.good())
	{
		debugLog("Failed to open destination file for copying: {:s}\n", to.error().data());
		return;
	}

	std::vector<uint8_t> buf(READ_BUFFER_SIZE);

	uint32_t remaining = from.getTotalSize();
	while (remaining > 0 && from.good() && to.good())
	{
		uint32_t len = std::min(remaining, static_cast<uint32_t>(READ_BUFFER_SIZE));
		if (from.readBytes(buf.data(), len) != len)
		{
			debugLog("Copy failed: could not read {} bytes, {} remaining\n", len, remaining);
			break;
		}
		to.writeBytes(buf.data(), len);
		remaining -= len;
	}

	if (!from.good())
	{
		debugLog("Copy failed during read: {:s}\n", from.error().data());
	}
	if (!to.good())
	{
		debugLog("Copy failed during write: {:s}\n", to.error().data());
	}
}
