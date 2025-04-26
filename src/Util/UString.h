//========= Copyright (c) 2009, 2D Boy & PG & 2025, WH, All rights reserved. =========//
//
// Purpose:		unicode string class (modified)
//
// $NoKeywords: $ustring $string
//====================================================================================//

#pragma once
#ifndef USTRING_H
#define USTRING_H

#include "cbase.h"

class UString
{
public:
	static UString format(const char *utf8format, ...);

public:
	UString();
	UString(const wchar_t *str);
	UString(const char *utf8);
	UString(const char *utf8, int length);
	UString(const UString &ustr);
	UString(UString &&ustr) noexcept;
	~UString();

	void clear();

	// get
	[[nodiscard]] inline int length() const { return m_length; }
	[[nodiscard]] inline int lengthUtf8() const { return m_lengthUtf8; }
	[[nodiscard]] inline const char *toUtf8() const { return m_utf8.c_str(); }
	[[nodiscard]] inline const wchar_t *wc_str() const { return m_unicode.c_str(); }
	// basically just for autodetecting windows' wchar_t
	[[nodiscard]] inline auto plat_str() const {
		if constexpr (Env::cfg(OS::WINDOWS)) return m_unicode.c_str();
		else return m_utf8.c_str();
	}
	[[nodiscard]] inline bool isAsciiOnly() const { return m_isAsciiOnly; }
	[[nodiscard]] bool isWhitespaceOnly() const;
	[[nodiscard]] inline bool isEmpty() const { return length() > 0 && !isWhitespaceOnly(); }

	[[nodiscard]] int findChar(wchar_t ch, int start = 0, bool respectEscapeChars = false) const;
	[[nodiscard]] int findChar(const UString &str, int start = 0, bool respectEscapeChars = false) const;
	[[nodiscard]] int find(const UString &str, int start = 0) const;
	[[nodiscard]] int find(const UString &str, int start, int end) const;
	[[nodiscard]] int findLast(const UString &str, int start = 0) const;
	[[nodiscard]] int findLast(const UString &str, int start, int end) const;

	[[nodiscard]] int findIgnoreCase(const UString &str, int start = 0) const;
	[[nodiscard]] int findIgnoreCase(const UString &str, int start, int end) const;

	// modifiers
	void collapseEscapes();
	void append(const UString &str);
	void append(wchar_t ch);
	void insert(int offset, const UString &str);
	void insert(int offset, wchar_t ch);
	void erase(int offset, int count);

	// actions (non-modifying)
	[[nodiscard]] UString substr(int offset, int charCount = -1) const;
	[[nodiscard]] std::vector<UString> split(UString delim) const;
	[[nodiscard]] UString trim() const;

	// conversions
	[[nodiscard]] float toFloat() const;
	[[nodiscard]] double toDouble() const;
	[[nodiscard]] long double toLongDouble() const;
	[[nodiscard]] int toInt() const;
	[[nodiscard]] long toLong() const;
	[[nodiscard]] long long toLongLong() const;
	[[nodiscard]] unsigned int toUnsignedInt() const;
	[[nodiscard]] unsigned long toUnsignedLong() const;
	[[nodiscard]] unsigned long long toUnsignedLongLong() const;

	void lowerCase();
	void upperCase();

	// operators
	wchar_t operator[](int index) const;
	UString &operator=(const UString &ustr);
	UString &operator=(UString &&ustr) noexcept;
	bool operator==(const UString &ustr) const;
	bool operator!=(const UString &ustr) const;
	bool operator<(const UString &ustr) const;

	[[nodiscard]] bool equalsIgnoreCase(const UString &ustr) const;
	[[nodiscard]] bool lessThanIgnoreCase(const UString &ustr) const;

private:
	static constexpr char ESCAPE_CHAR = '\\';

	int fromUtf8(const char *utf8, int length = -1);
	void updateUtf8();

	std::wstring m_unicode;
	std::string m_utf8;

	int m_length;
	int m_lengthUtf8;
	bool m_isAsciiOnly;
};

#endif
