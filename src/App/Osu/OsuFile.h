//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu type file io helper
//
// $NoKeywords: $osudb
//===============================================================================//

#pragma once
#ifndef OSUFILE_H
#define OSUFILE_H

#include "cbase.h"

class McFile;

class OsuFile
{
public:
	static constexpr const uint64_t MAX_STRING_LENGTH = 4096ULL;

	struct TIMINGPOINT
	{
		double msPerBeat;
		double offset;
		bool timingChange;
	};

public:
	static std::string md5(const unsigned char *data, size_t numBytes);

public:
	OsuFile(UString filepath, bool write = false, bool writeBufferOnly = false);
	virtual ~OsuFile();

	[[nodiscard]] inline size_t getFileSize() const {return m_iFileSize;}

	// ILLEGAL:
	[[nodiscard]] inline unsigned char *getBuffer() const {return (unsigned char*)m_buffer;}
	[[nodiscard]] inline unsigned char *getReadPointer() const {return (unsigned char*)m_readPointer;}
	[[nodiscard]] inline const std::vector<char> &getWriteBuffer() const {return m_writeBuffer;}

	[[nodiscard]] inline bool isReady() const {return m_bReady;}

	void write();

	// write
	void writeByte(unsigned char val);
	void writeShort(int16_t val);
	void writeInt(int32_t val);
	void writeLongLong(int64_t val);
	void writeULEB128(uint64_t val);
	void writeFloat(float val);
	void writeDouble(double val);
	void writeBool(bool val);
	void writeString(UString &str);
	void writeStdString(std::string str);

	// read
	[[nodiscard]] unsigned char readByte();
	[[nodiscard]] int16_t readShort();
	[[nodiscard]] int32_t readInt();
	[[nodiscard]] int64_t readLongLong();
	[[nodiscard]] uint64_t readULEB128();
	[[nodiscard]] float readFloat();
	[[nodiscard]] double readDouble();
	[[nodiscard]] bool readBool();
	[[nodiscard]] UString readString();
	[[nodiscard]] std::string readStdString();
	void readDateTime();
	[[nodiscard]] TIMINGPOINT readTimingPoint();
	void readByteArray();

	// advance read pointer by amount
	// clang-format off
	inline void skipByte()		  {if (!(!m_bReady || (m_readPointer + sizeof(unsigned char)) >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(unsigned char);}
	inline void skipShort()		  {if (!(!m_bReady || (m_readPointer + sizeof(int16_t))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(int16_t);}
	inline void skipInt()		  {if (!(!m_bReady || (m_readPointer + sizeof(int32_t))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(int32_t);}
	inline void skipLongLong()	  {if (!(!m_bReady || (m_readPointer + sizeof(int64_t))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(int64_t);}
	inline void skipULEB128()	  {if (!(!m_bReady || (m_readPointer + sizeof(uint64_t))	  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(uint64_t);}
	inline void skipFloat()		  {if (!(!m_bReady || (m_readPointer + sizeof(float))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(float);}
	inline void skipDouble()	  {if (!(!m_bReady || (m_readPointer + sizeof(double))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(double);}
	inline void skipBool()		  {if (!(!m_bReady || (m_readPointer + sizeof(bool))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(bool);}
	inline void skipString()	  {if (!(!m_bReady || (m_readPointer + sizeof(UString))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(UString);}
	inline void skipStdString()	  {if (!(!m_bReady || (m_readPointer + sizeof(std::string))	  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(std::string);}
	//inline void skipDateTime()  {if (!(!m_bReady || (m_readPointer + sizeof(void))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(void);}
	inline void skipTimingPoint() {if (!(!m_bReady || (m_readPointer + sizeof(TIMINGPOINT))	  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(TIMINGPOINT);}
	//inline void skipByteArray() {if (!(!m_bReady || (m_readPointer + sizeof(void))		  >= (m_buffer + m_iFileSize))) m_readPointer += sizeof(void);}
	// clang-format on
private:
	uint64_t decodeULEB128(const uint8_t *p, unsigned int *n = NULL);

	McFile *m_file;
	size_t m_iFileSize;
	const char *m_buffer;
	const char *m_readPointer;
	bool m_bReady;
	bool m_bWrite;

	std::vector<char> m_writeBuffer;
};

#endif
