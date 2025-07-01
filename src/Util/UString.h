//========= Copyright (c) 2009, 2D Boy & PG & 2025, WH, All rights reserved. =========//
//
// Purpose:		unicode string class (modified)
//
// $NoKeywords: $ustring $string
//====================================================================================//

#pragma once
#ifndef USTRING_H
#define USTRING_H

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "BaseEnvironment.h" // for Env::cfg (consteval)
#include "fmt/compile.h"
#include "fmt/format.h"
#include "fmt/printf.h"

using namespace fmt::literals;

class McFont;

class UString
{
public:
	template <typename... Args>
	[[nodiscard]] static UString format(std::string_view fmt, Args &&...args) noexcept;

	template <typename... Args>
	[[nodiscard]] static UString fmt(fmt::format_string<Args...> fmt, Args &&...args) noexcept;

	template <typename Range>
	    requires std::ranges::range<Range> && std::convertible_to<std::ranges::range_value_t<Range>, UString>
	[[nodiscard]] static UString join(const Range &range, std::string_view delim = " ") noexcept;

	[[nodiscard]] static UString join(std::span<const UString> strings, std::string_view delim = " ") noexcept;

public:
	// constructors
	constexpr UString() noexcept = default;

	// explicit constructors for sized arrays to prevent accidental conversions
	explicit UString(const char *utf8, int length);
	explicit UString(const wchar_t *str, int length);

	// implicit constructors for string literals
	template <size_t N>
	UString(const char (&str)[N])
	{
		if (N > 1)
			fromUtf8(str, static_cast<int>(N - 1)); // N-1 to exclude null terminator
	}

	template <size_t N>
	UString(const wchar_t (&str)[N])
	{
		if (N > 1)
		{
			this->sUnicode.assign(str, N - 1);
			setLength(static_cast<int>(this->sUnicode.length()));
			updateUtf8();
		}
	}

	// implicit constructors for const char*/const wchar_t*
	UString(const char *utf8);
	UString(const wchar_t *str);

	// implicit constructors for standard string types
	UString(const std::string &utf8);
	UString(const std::wstring &str);
	UString(std::string_view utf8);
	UString(std::wstring_view str);

	// member functions
	UString(const UString &ustr) = default;
	UString(UString &&ustr) noexcept = default;
	UString &operator=(const UString &ustr) = default;
	UString &operator=(UString &&ustr) noexcept = default;
	~UString() = default;

	// basic operations
	void clear() noexcept;

	// getters
	[[nodiscard]] constexpr int length() const noexcept { return this->iLengthUnicode & LENGTH_MASK; }
	[[nodiscard]] constexpr int lengthUtf8() const noexcept { return this->iLengthUtf8; }
	[[nodiscard]] constexpr std::string_view utf8View() const noexcept { return this->sUtf8; }
	[[nodiscard]] constexpr const char *toUtf8() const noexcept { return this->sUtf8.c_str(); }
	[[nodiscard]] constexpr std::wstring_view unicodeView() const noexcept { return this->sUnicode; }
	[[nodiscard]] constexpr const wchar_t *wc_str() const noexcept { return this->sUnicode.c_str(); }

	// basically just for autodetecting windows' wchar_t preference (paths)
	[[nodiscard]] constexpr auto plat_str() const noexcept
	{
		if constexpr (Env::cfg(OS::WINDOWS))
			return this->sUnicode.c_str();
		else
			return this->sUtf8.c_str();
	}
	[[nodiscard]] constexpr auto plat_view() const noexcept
	{
		if constexpr (Env::cfg(OS::WINDOWS))
			return this->sUnicode;
		else
			return this->sUtf8;
	}
	// state queries
	[[nodiscard]] constexpr bool isAsciiOnly() const noexcept { return (this->iLengthUnicode & ASCII_FLAG) != 0; }
	[[nodiscard]] bool isWhitespaceOnly() const noexcept;
	[[nodiscard]] constexpr bool isEmpty() const noexcept { return this->sUnicode.empty(); }

	// string tests
	[[nodiscard]] constexpr bool endsWith(char ch) const noexcept { return !this->sUtf8.empty() && this->sUtf8.back() == ch; }
	[[nodiscard]] constexpr bool endsWith(wchar_t ch) const noexcept { return !this->sUnicode.empty() && this->sUnicode.back() == ch; }
	[[nodiscard]] constexpr bool endsWith(const UString &suffix) const noexcept
	{
		int suffixLen = suffix.length();
		int thisLen = length();
		return suffixLen <= thisLen && std::equal(suffix.sUnicode.begin(), suffix.sUnicode.end(), this->sUnicode.end() - suffixLen);
	}
	[[nodiscard]] constexpr bool startsWith(char ch) const noexcept { return !this->sUtf8.empty() && this->sUtf8.front() == ch; }
	[[nodiscard]] constexpr bool startsWith(wchar_t ch) const noexcept { return !this->sUnicode.empty() && this->sUnicode.front() == ch; }
	[[nodiscard]] constexpr bool startsWith(const UString &suffix) const noexcept
	{
		int suffixLen = suffix.length();
		int thisLen = length();
		return suffixLen <= thisLen && std::equal(suffix.sUnicode.begin(), suffix.sUnicode.end(), this->sUnicode.begin());
	}

	// search functions
	[[nodiscard]] int findChar(wchar_t ch, int start = 0, bool respectEscapeChars = false) const;
	[[nodiscard]] int findChar(const UString &str, int start = 0, bool respectEscapeChars = false) const;
	[[nodiscard]] int find(const UString &str, int start = 0) const;
	[[nodiscard]] int find(const UString &str, int start, int end) const;
	[[nodiscard]] int findLast(const UString &str, int start = 0) const;
	[[nodiscard]] int findLast(const UString &str, int start, int end) const;
	[[nodiscard]] int findIgnoreCase(const UString &str, int start = 0) const;
	[[nodiscard]] int findIgnoreCase(const UString &str, int start, int end) const;

	// iterators for range-based for loops
	[[nodiscard]] constexpr auto begin() noexcept { return this->sUnicode.begin(); }
	[[nodiscard]] constexpr auto end() noexcept { return this->sUnicode.end(); }
	[[nodiscard]] constexpr auto begin() const noexcept { return this->sUnicode.begin(); }
	[[nodiscard]] constexpr auto end() const noexcept { return this->sUnicode.end(); }
	[[nodiscard]] constexpr auto cbegin() const noexcept { return this->sUnicode.cbegin(); }
	[[nodiscard]] constexpr auto cend() const noexcept { return this->sUnicode.cend(); }

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
		int len = length();
		offset = std::clamp<int>(offset, 0, len);

		if (charCount < 0)
			charCount = len - offset;
		charCount = std::clamp<int>(charCount, 0, len - offset);

		UString result;
		result.sUnicode = this->sUnicode.substr(offset, charCount);
		result.setLength(static_cast<int>(result.sUnicode.length()));
		result.updateUtf8();

		if constexpr (std::is_same_v<T, UString>)
			return result;
		else
			return result.to<T>();
	}

	template <typename T = UString>
	[[nodiscard]] std::vector<T> split(const UString &delim) const
	{
		std::vector<T> results;
		int delimLen = delim.length();
		int thisLen = length();
		if (delimLen < 1 || thisLen < 1)
			return results;

		int start = 0;
		int end = 0;

		while ((end = find(delim, start)) != -1)
		{
			results.push_back(substr<T>(start, end - start));
			start = end + delimLen;
		}
		results.push_back(substr<T>(start));

		return results;
	}

	[[nodiscard]] UString trim() const;

	// type conversions
	template <typename T>
	[[nodiscard]] constexpr T to() const noexcept
	{
		if (this->sUtf8.empty())
			return T{};

		if constexpr (std::is_same_v<T, UString>)
			return *this;
		else if constexpr (std::is_same_v<T, std::string>)
			return std::string{this->sUtf8};
		else if constexpr (std::is_same_v<T, std::string_view>)
			return std::string_view{this->sUtf8};
		else if constexpr (std::is_same_v<T, std::wstring>)
			return std::wstring{this->sUnicode};
		else if constexpr (std::is_same_v<T, std::wstring_view>)
			return std::wstring_view{this->sUnicode};
		else if constexpr (std::is_same_v<T, float>)
			return std::strtof(this->sUtf8.c_str(), nullptr);
		else if constexpr (std::is_same_v<T, double>)
			return std::strtod(this->sUtf8.c_str(), nullptr);
		else if constexpr (std::is_same_v<T, long double>)
			return std::strtold(this->sUtf8.c_str(), nullptr);
		else if constexpr (std::is_same_v<T, int>)
			return static_cast<int>(std::strtol(this->sUtf8.c_str(), nullptr, 0));
		else if constexpr (std::is_same_v<T, bool>)
			return !!static_cast<int>(std::strtol(this->sUtf8.c_str(), nullptr, 0));
		else if constexpr (std::is_same_v<T, long>)
			return std::strtol(this->sUtf8.c_str(), nullptr, 0);
		else if constexpr (std::is_same_v<T, long long>)
			return std::strtoll(this->sUtf8.c_str(), nullptr, 0);
		else if constexpr (std::is_same_v<T, unsigned int>)
			return static_cast<unsigned int>(std::strtoul(this->sUtf8.c_str(), nullptr, 0));
		else if constexpr (std::is_same_v<T, unsigned long>)
			return std::strtoul(this->sUtf8.c_str(), nullptr, 0);
		else if constexpr (std::is_same_v<T, unsigned long long>)
			return std::strtoull(this->sUtf8.c_str(), nullptr, 0);
		else
			static_assert(Env::always_false_v<T>, "unsupported type");
	}

	// conversion shortcuts
	[[nodiscard]] constexpr float toFloat() const noexcept { return to<float>(); }
	[[nodiscard]] constexpr double toDouble() const noexcept { return to<double>(); }
	[[nodiscard]] constexpr long double toLongDouble() const noexcept { return to<long double>(); }
	[[nodiscard]] constexpr int toInt() const noexcept { return to<int>(); }
	[[nodiscard]] constexpr bool toBool() const noexcept { return to<bool>(); }
	[[nodiscard]] constexpr long toLong() const noexcept { return to<long>(); }
	[[nodiscard]] constexpr long long toLongLong() const noexcept { return to<long long>(); }
	[[nodiscard]] constexpr unsigned int toUnsignedInt() const noexcept { return to<unsigned int>(); }
	[[nodiscard]] constexpr unsigned long toUnsignedLong() const noexcept { return to<unsigned long>(); }
	[[nodiscard]] constexpr unsigned long long toUnsignedLongLong() const noexcept { return to<unsigned long long>(); }

	// case conversion
	void lowerCase();
	void upperCase();

	// operators
	[[nodiscard]] constexpr const wchar_t &operator[](int index) const
	{
		int len = length();
		return this->sUnicode[std::clamp(index, 0, len - 1)];
	}

	// operators
	[[nodiscard]] constexpr wchar_t &operator[](int index)
	{
		int len = length();
		return this->sUnicode[std::clamp(index, 0, len - 1)];
	}

	bool operator==(const UString &ustr) const = default;
	auto operator<=>(const UString &ustr) const = default;

	UString &operator+=(const UString &ustr);
	[[nodiscard]] UString operator+(const UString &ustr) const;
	UString &operator+=(wchar_t ch);
	[[nodiscard]] UString operator+(wchar_t ch) const;
	UString &operator+=(char ch);
	[[nodiscard]] UString operator+(char ch) const;

	[[nodiscard]] bool equalsIgnoreCase(const UString &ustr) const;
	[[nodiscard]] bool lessThanIgnoreCase(const UString &ustr) const;

	// for strict-weak-ordering
	[[nodiscard]] bool lessThanIgnoreCaseStrict(const UString &rhs) const noexcept { return ncasecomp{}(*this, rhs); }

	class ncasecomp
	{
	public:
		bool operator()(const UString &lhs, const UString &rhs) const noexcept;

	private:
		// consistent case normalization that avoids locale-dependent behavior (to use with std::sort to satisfy
		// strict-weak-ordering)
		static constexpr wchar_t normalizeCase(wchar_t ch) noexcept
		{
			// ASCII fast path
			if (ch >= L'A' && ch <= L'Z')
				return ch + (L'a' - L'A');
			// clamp to valid wchar_t range
			if (ch <= 0xFFFF)
				return static_cast<wchar_t>(std::towlower(static_cast<std::wint_t>(ch)));
			// outside BMP (basic multi-lingual plane) is returned as-is to stay deterministic because they aren't
			// handled by UString currently
			return ch;
		}
	};

	friend struct std::hash<UString>;

private:
	// pack ascii flag into the high bit of this->iLengthUnicode so we don't need an extra field just to store that
	// information, which adds 8 bytes due to padding
	static constexpr int ASCII_FLAG = 0x40000000;
	static constexpr int LENGTH_MASK = 0x3FFFFFFF;

	void setLength(int len) noexcept { this->iLengthUnicode = (this->iLengthUnicode & ASCII_FLAG) | (len & LENGTH_MASK); }
	void setAsciiFlag(bool isAscii) noexcept { this->iLengthUnicode = isAscii ? (this->iLengthUnicode | ASCII_FLAG) : (this->iLengthUnicode & LENGTH_MASK); }

	int fromUtf8(const char *utf8, int length = -1);
	void updateUtf8();

	std::wstring sUnicode;
	std::string sUtf8;

	int iLengthUnicode = ASCII_FLAG; // start with ascii flag set
	int iLengthUtf8 = 0;
};

namespace std
{
template <>
struct hash<UString>
{
	size_t operator()(const UString &str) const noexcept { return hash<std::wstring>()(str.sUnicode); }
};
} // namespace std

// for printf-style formatting (legacy McEngine style, should convert over to UString::fmt because it's nicer)
template <typename... Args>
UString UString::format(std::string_view fmt, Args &&...args) noexcept
{
	return UString(fmt::sprintf(fmt, std::forward<Args>(args)...));
}

// need a specialization for fmt, so that UStrings can be passed directly without needing .toUtf8() (for UString::fmt)
namespace fmt
{
template <>
struct formatter<UString> : formatter<string_view>
{
	template <typename FormatContext>
	auto format(const UString &str, FormatContext &ctx) const
	{
		return formatter<string_view>::format(str.utf8View(), ctx);
	}
};
} // namespace fmt

template <typename... Args>
UString UString::fmt(fmt::format_string<Args...> fmt, Args &&...args) noexcept
{
	return UString(fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename Range>
    requires std::ranges::range<Range> && std::convertible_to<std::ranges::range_value_t<Range>, UString>
UString UString::join(const Range &range, std::string_view delim) noexcept
{
	if (std::ranges::empty(range))
		return {};

	UString delimStr(delim);
	auto it = std::ranges::begin(range);
	UString result = *it;
	++it;

	for (; it != std::ranges::end(range); ++it)
	{
		result += delimStr;
		result += *it;
	}

	return result;
}

#endif // USTRING_H
