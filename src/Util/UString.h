//========= Copyright (c) 2009, 2D Boy & PG & 2025, WH, All rights reserved. =========//
//
// Purpose:		unicode string class (modified)
//
// $NoKeywords: $ustring $string
//====================================================================================//

#pragma once
#ifndef USTRING_H
#define USTRING_H

#include "BaseEnvironment.h" // for Env::cfg (consteval)
#include <algorithm>
#include <vector>
#include <string>

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
	template <typename T = UString>
	[[nodiscard]] constexpr T substr(int offset, int charCount = -1) const
	{
		offset = std::clamp<int>(offset, 0, m_length);

		if (charCount < 0)
			charCount = m_length - offset;

		charCount = std::clamp<int>(charCount, 0, m_length - offset);

		UString result;
		result.m_unicode = m_unicode.substr(offset, charCount);
		result.m_length = static_cast<int>(result.m_unicode.length());
		result.updateUtf8();

		return result.to<T>();
	}

	template <typename T = UString>
	[[nodiscard]] constexpr std::vector<T> split(UString delim) const
	{
		std::vector<T> results;
		if (delim.m_length < 1 || m_length < 1)
			return results;

		int start = 0;
		int end = 0;

		while ((end = find(delim, start)) != -1)
		{
			results.push_back(substr<T>(start, end - start));
			start = end + delim.m_length;
		}
		results.push_back(substr<T>(start));

		return results;
	}

	[[nodiscard]] UString trim() const;

	// conversions
	template <typename T = UString>
	[[nodiscard]] constexpr T to() const noexcept
	{
		if (m_utf8.empty())
			return T{};
		if constexpr (std::is_same_v<T, float>) {
			return std::strtof(m_utf8.c_str(), nullptr);
		} else if constexpr (std::is_same_v<T, double>) {
			return std::strtod(m_utf8.c_str(), nullptr);
		} else if constexpr (std::is_same_v<T, long double>) {
			return std::strtold(m_utf8.c_str(), nullptr);
		} else if constexpr (std::is_same_v<T, int>) {
			return static_cast<int>(std::strtol(m_utf8.c_str(), nullptr, 0));
		} else if constexpr (std::is_same_v<T, long>) {
			return std::strtol(m_utf8.c_str(), nullptr, 0);
		} else if constexpr (std::is_same_v<T, long long>) {
			return std::strtoll(m_utf8.c_str(), nullptr, 0);
		} else if constexpr (std::is_same_v<T, unsigned int>) {
			return static_cast<unsigned int>(std::strtoul(m_utf8.c_str(), nullptr, 0));
		} else if constexpr (std::is_same_v<T, unsigned long>) {
			return std::strtoul(m_utf8.c_str(), nullptr, 0);
		} else if constexpr (std::is_same_v<T, unsigned long long>) {
			return std::strtoull(m_utf8.c_str(), nullptr, 0);
		} else {
			return *this;
		}
	}

	[[nodiscard]] constexpr float toFloat() 						const noexcept {return to<float>();}
	[[nodiscard]] constexpr double toDouble() 						const noexcept {return to<double>();}
	[[nodiscard]] constexpr long double toLongDouble() 				const noexcept {return to<long double>();}
	[[nodiscard]] constexpr int toInt() 							const noexcept {return to<int>();}
	[[nodiscard]] constexpr long toLong() 							const noexcept {return to<long>();}
	[[nodiscard]] constexpr long long toLongLong() 					const noexcept {return to<long long>();}
	[[nodiscard]] constexpr unsigned int toUnsignedInt() 			const noexcept {return to<unsigned int>();}
	[[nodiscard]] constexpr unsigned long toUnsignedLong() 			const noexcept {return to<unsigned long>();}
	[[nodiscard]] constexpr unsigned long long toUnsignedLongLong() const noexcept {return to<unsigned long long>();}

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
	int fromUtf8(const char *utf8, int length = -1);
	void updateUtf8();

	std::wstring m_unicode;
	std::string m_utf8;

	int m_length;
	int m_lengthUtf8;
	bool m_isAsciiOnly;
};

#endif
