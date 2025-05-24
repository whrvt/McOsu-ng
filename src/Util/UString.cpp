//========= Copyright (c) 2009, 2D Boy & PG & 2025, WH, All rights reserved. =========//
//
// Purpose:		unicode string class (modified)
//
// $NoKeywords: $ustring $string
//====================================================================================//

#include "UString.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <ranges>
#include <utility>

#define USTRING_MASK_1BYTE 0x80  /* 1000 0000 */
#define USTRING_VALUE_1BYTE 0x00 /* 0000 0000 */
#define USTRING_MASK_2BYTE 0xE0  /* 1110 0000 */
#define USTRING_VALUE_2BYTE 0xC0 /* 1100 0000 */
#define USTRING_MASK_3BYTE 0xF0  /* 1111 0000 */
#define USTRING_VALUE_3BYTE 0xE0 /* 1110 0000 */
#define USTRING_MASK_4BYTE 0xF8  /* 1111 1000 */
#define USTRING_VALUE_4BYTE 0xF0 /* 1111 0000 */
#define USTRING_MASK_5BYTE 0xFC  /* 1111 1100 */
#define USTRING_VALUE_5BYTE 0xF8 /* 1111 1000 */
#define USTRING_MASK_6BYTE 0xFE  /* 1111 1110 */
#define USTRING_VALUE_6BYTE 0xFC /* 1111 1100 */

#define USTRING_MASK_MULTIBYTE 0x3F /* 0011 1111 */

static constexpr char ESCAPE_CHAR = '\\';

UString::UString(const wchar_t *str)
{
	if (str && *str)
	{
		m_unicode = str;
		m_length = static_cast<int>(m_unicode.length());
		updateUtf8();
	}
}

UString::UString(const char *utf8)
{
	if (utf8 && *utf8)
		fromUtf8(utf8);
}

UString::UString(const char *utf8, int length) : m_lengthUtf8(length)
{
	if (utf8 && length > 0)
		fromUtf8(utf8, length);
}

UString::UString(std::string_view utf8) : m_lengthUtf8(static_cast<int>(utf8.length()))
{
	if (!utf8.empty())
		fromUtf8(utf8.data(), static_cast<int>(utf8.length()));
}

void UString::clear() noexcept
{
	m_unicode.clear();
	m_utf8.clear();
	m_length = 0;
	m_lengthUtf8 = 0;
	m_isAsciiOnly = true;
}

UString UString::join(std::span<const UString> strings, std::string_view delim) noexcept
{
	if (strings.empty())
		return {};

	UString delimStr(delim);
	UString result = strings[0];

	for (size_t i = 1; i < strings.size(); ++i)
	{
		result += delimStr;
		result += strings[i];
	}

	return result;
}

bool UString::isWhitespaceOnly() const noexcept
{
	return std::ranges::all_of(m_unicode, [](wchar_t c) { return std::iswspace(c) != 0; });
}

int UString::findChar(wchar_t ch, int start, bool respectEscapeChars) const
{
	if (start < 0 || start >= m_length)
		return -1;

	bool escaped = false;
	for (int i = start; i < m_length; i++)
	{
		if (respectEscapeChars && !escaped && m_unicode[i] == ESCAPE_CHAR)
		{
			escaped = true;
		}
		else
		{
			if (!escaped && m_unicode[i] == ch)
				return i;

			escaped = false;
		}
	}

	return -1;
}

int UString::findChar(const UString &str, int start, bool respectEscapeChars) const
{
	if (start < 0 || start >= m_length || str.m_length == 0)
		return -1;

	std::vector<bool> charMap(0x10000, false); // lookup table, assumes 16-bit wide chars
	for (int i = 0; i < str.m_length; i++)
		charMap[str.m_unicode[i]] = true;

	bool escaped = false;
	for (int i = start; i < m_length; i++)
	{
		if (respectEscapeChars && !escaped && m_unicode[i] == ESCAPE_CHAR)
		{
			escaped = true;
		}
		else
		{
			if (!escaped && charMap[m_unicode[i]])
				return i;

			escaped = false;
		}
	}

	return -1;
}

int UString::find(const UString &str, int start) const
{
	if (start < 0 || str.m_length == 0 || start > m_length - str.m_length)
		return -1;

	size_t pos = m_unicode.find(str.m_unicode, start);
	return (pos != std::wstring::npos) ? static_cast<int>(pos) : -1;
}

int UString::find(const UString &str, int start, int end) const
{
	if (start < 0 || end > m_length || start >= end || str.m_length == 0)
		return -1;

	if (end < m_length)
	{
		auto tempSubstr = m_unicode.substr(start, end - start);
		size_t pos = tempSubstr.find(str.m_unicode);
		return (pos != std::wstring::npos) ? static_cast<int>(pos + start) : -1;
	}

	return find(str, start);
}

int UString::findLast(const UString &str, int start) const
{
	if (start < 0 || str.m_length == 0 || start > m_length - str.m_length)
		return -1;

	size_t pos = m_unicode.rfind(str.m_unicode);
	if (pos != std::wstring::npos && std::cmp_greater_equal(pos, start))
		return static_cast<int>(pos);

	return -1;
}

int UString::findLast(const UString &str, int start, int end) const
{
	if (start < 0 || end > m_length || start >= end || str.m_length == 0)
		return -1;

	int lastPossibleMatch = std::min(end - str.m_length, m_length - str.m_length);
	for (int i = lastPossibleMatch; i >= start; i--)
	{
		if (std::equal(str.m_unicode.begin(), str.m_unicode.end(), m_unicode.begin() + i))
			return i;
	}

	return -1;
}

int UString::findIgnoreCase(const UString &str, int start) const
{
	if (start < 0 || str.m_length == 0 || start > m_length - str.m_length)
		return -1;

	auto toLower = [](auto c) { return std::towlower(c); };

	auto sourceView = m_unicode | std::views::drop(start) | std::views::transform(toLower);
	auto targetView = str.m_unicode | std::views::transform(toLower);

	auto result = std::ranges::search(sourceView, targetView);

	if (!result.empty())
		return static_cast<int>(std::distance(sourceView.begin(), result.begin())) + start;

	return -1;
}

int UString::findIgnoreCase(const UString &str, int start, int end) const
{
	if (start < 0 || end > m_length || start >= end || str.m_length == 0)
		return -1;

	auto toLower = [](auto c) { return std::towlower(c); };

	auto sourceView = m_unicode | std::views::drop(start) | std::views::take(end - start) | std::views::transform(toLower);
	auto targetView = str.m_unicode | std::views::transform(toLower);

	auto result = std::ranges::search(sourceView, targetView);

	if (!result.empty())
		return static_cast<int>(std::distance(sourceView.begin(), result.begin())) + start;

	return -1;
}

void UString::collapseEscapes()
{
	if (m_length == 0)
		return;

	std::wstring result;
	result.reserve(m_length);

	bool escaped = false;
	for (wchar_t ch : m_unicode)
	{
		if (!escaped && ch == ESCAPE_CHAR)
		{
			escaped = true;
		}
		else
		{
			result.push_back(ch);
			escaped = false;
		}
	}

	m_unicode = std::move(result);
	m_length = static_cast<int>(m_unicode.length());
	updateUtf8();
}

void UString::append(const UString &str)
{
	if (str.m_length == 0)
		return;

	m_unicode.append(str.m_unicode);
	m_length = static_cast<int>(m_unicode.length());

	// fast path for ASCII strings
	if (m_isAsciiOnly && str.m_isAsciiOnly)
	{
		m_utf8.append(str.m_utf8);
		m_lengthUtf8 = static_cast<int>(m_utf8.length());
	}
	else
	{
		updateUtf8();
	}
}

void UString::append(wchar_t ch)
{
	m_unicode.push_back(ch);
	m_length = static_cast<int>(m_unicode.length());

	// fast path for ASCII character
	if (ch < 0x80 && m_isAsciiOnly)
	{
		m_utf8.push_back(static_cast<char>(ch));
		m_lengthUtf8 = static_cast<int>(m_utf8.length());
	}
	else
	{
		updateUtf8();
	}
}

void UString::insert(int offset, const UString &str)
{
	if (str.m_length == 0)
		return;

	offset = std::clamp(offset, 0, m_length);
	m_unicode.insert(offset, str.m_unicode);
	m_length = static_cast<int>(m_unicode.length());
	updateUtf8();
}

void UString::insert(int offset, wchar_t ch)
{
	offset = std::clamp(offset, 0, m_length);
	m_unicode.insert(offset, 1, ch);
	m_length = static_cast<int>(m_unicode.length());
	updateUtf8();
}

void UString::erase(int offset, int count)
{
	if (m_length == 0 || count == 0 || offset >= m_length)
		return;

	offset = std::clamp(offset, 0, m_length);
	count = std::clamp(count, 0, m_length - offset);

	m_unicode.erase(offset, count);
	m_length = static_cast<int>(m_unicode.length());
	updateUtf8();
}

UString UString::trim() const
{
	if (m_length == 0)
		return {};

	auto isWhitespace = [](wchar_t c) { return std::iswspace(c) != 0; };

	// find first non-whitespace character
	auto start = std::ranges::find_if_not(m_unicode, isWhitespace);
	if (start == m_unicode.end())
		return {};

	// find last non-whitespace character
	auto rstart = std::ranges::find_if_not(std::ranges::reverse_view(m_unicode), isWhitespace);
	auto end = rstart.base();

	int startPos = static_cast<int>(std::distance(m_unicode.begin(), start));
	int length = static_cast<int>(std::distance(start, end));

	return substr(startPos, length);
}

void UString::lowerCase()
{
	if (m_length == 0)
		return;

	std::ranges::transform(m_unicode, m_unicode.begin(), [](wchar_t c) { return std::towlower(c); });

	if (m_isAsciiOnly)
		std::ranges::transform(m_utf8, m_utf8.begin(), [](char c) { return std::tolower(c); });
	else
		updateUtf8();
}

void UString::upperCase()
{
	if (m_length == 0)
		return;

	std::ranges::transform(m_unicode, m_unicode.begin(), [](wchar_t c) { return std::towupper(c); });

	if (m_isAsciiOnly)
		std::ranges::transform(m_utf8, m_utf8.begin(), [](char c) { return std::toupper(c); });
	else
		updateUtf8();
}

UString &UString::operator+=(const UString &ustr)
{
	append(ustr);
	return *this;
}

UString UString::operator+(const UString &ustr) const
{
	UString result(*this);
	result.append(ustr);
	return result;
}

UString &UString::operator+=(wchar_t ch)
{
	append(ch);
	return *this;
}

UString UString::operator+(wchar_t ch) const
{
	UString result(*this);
	result.append(ch);
	return result;
}

UString &UString::operator+=(char ch)
{
	append(static_cast<wchar_t>(ch));
	return *this;
}

UString UString::operator+(char ch) const
{
	UString result(*this);
	result.append(static_cast<wchar_t>(ch));
	return result;
}

bool UString::equalsIgnoreCase(const UString &ustr) const
{
	if (m_length != ustr.m_length)
		return false;
	if (m_length == 0 && ustr.m_length == 0)
		return true;

	return std::ranges::equal(m_unicode, ustr.m_unicode, [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); });
}

bool UString::lessThanIgnoreCase(const UString &ustr) const
{
	auto it1 = m_unicode.begin();
	auto it2 = ustr.m_unicode.begin();

	while (it1 != m_unicode.end() && it2 != ustr.m_unicode.end())
	{
		wchar_t c1 = std::towlower(*it1); // narrowing conversion from 'wint_t' (aka 'unsigned int') to signed type 'wchar_t' is implementation-defined
		wchar_t c2 = std::towlower(*it2);
		if (c1 != c2)
			return c1 < c2;
		++it1;
		++it2;
	}

	// if we've reached the end of one string but not the other,
	// the shorter string is lexicographically less
	return it1 == m_unicode.end() && it2 != ustr.m_unicode.end();
}

// helper function for getUtf8
static inline void getUtf8(wchar_t ch, char *utf8, int numBytes, int firstByteValue)
{
	for (int i = numBytes - 1; i > 0; i--)
	{
		// store the lowest bits in a utf8 byte
		utf8[i] = static_cast<char>((ch & USTRING_MASK_MULTIBYTE) | 0x80);
		ch >>= 6;
	}

	// store the remaining bits
	*utf8 = static_cast<char>((firstByteValue | ch));
}

// helper function to encode a wide character string to UTF-8
static inline int encode(std::wstring_view unicode, char *utf8)
{
	int utf8len = 0;

	for (wchar_t ch : unicode)
	{
		if (ch < 0x00000080) // 1 byte
		{
			if (utf8 != nullptr)
				utf8[utf8len] = static_cast<char>(ch);
			utf8len += 1;
		}
		else if (ch < 0x00000800) // 2 bytes
		{
			if (utf8 != nullptr)
				getUtf8(ch, &(utf8[utf8len]), 2, USTRING_VALUE_2BYTE);
			utf8len += 2;
		}
		else if (ch <= 0xFFFF) // 3 bytes
		{
			if (utf8 != nullptr)
				getUtf8(ch, &(utf8[utf8len]), 3, USTRING_VALUE_3BYTE);
			utf8len += 3;
		}
#if WCHAR_MAX > 0xFFFF
		else if (ch <= 0x1FFFFF) // 4 bytes
		{
			if (utf8 != nullptr)
				getUtf8(ch, &(utf8[utf8len]), 4, USTRING_VALUE_4BYTE);
			utf8len += 4;
		}
		else if (ch <= 0x3FFFFFF) // 5 bytes
		{
			if (utf8 != nullptr)
				getUtf8(ch, &(utf8[utf8len]), 5, USTRING_VALUE_5BYTE);
			utf8len += 5;
		}
		else // 6 bytes
		{
			if (utf8 != nullptr)
				getUtf8(ch, &(utf8[utf8len]), 6, USTRING_VALUE_6BYTE);
			utf8len += 6;
		}
#endif
	}

	return utf8len;
}

// helper function to get a code point from UTF-8
static inline wchar_t getCodePoint(const char *utf8, int offset, int numBytes, unsigned char firstByteMask)
{
	// get the bits out of the first byte
	wchar_t wc = static_cast<unsigned char>(utf8[offset]) & firstByteMask;

	// iterate over the rest of the bytes
	for (int i = 1; i < numBytes; i++)
	{
		// shift the code point bits to make room for the new bits
		wc = wc << 6;
		// add the new bits
		wc |= static_cast<unsigned char>(utf8[offset + i]) & USTRING_MASK_MULTIBYTE;
	}

	return wc;
}

int UString::fromUtf8(const char *utf8, int length)
{
	if (!utf8)
		return 0;

	const int supposedFullStringSize = (length > -1 ? length : static_cast<int>(strlen(utf8)) + 1);

	int startIndex = 0;
	if (supposedFullStringSize > 2)
	{
		// check for UTF-8 BOM
		if ((unsigned char)utf8[0] == 0xEF && (unsigned char)utf8[1] == 0xBB && (unsigned char)utf8[2] == 0xBF)
			startIndex = 3;
		// check for UTF-16/32 (unsupported)
		else if (((unsigned char)utf8[0] == 0xFE && (unsigned char)utf8[1] == 0xFF) || ((unsigned char)utf8[0] == 0xFF && (unsigned char)utf8[1] == 0xFE))
			return 0;
	}

	const char *src = &(utf8[startIndex]);
	int remainingSize = supposedFullStringSize - startIndex;

	// ASCII-only strings (common case)
	bool isAscii = true;
	for (int i = 0; i < remainingSize && src[i] != 0; i++)
	{
		if ((unsigned char)src[i] >= 0x80)
		{
			isAscii = false;
			break;
		}
	}

	if (isAscii)
	{
		// for ASCII, do a direct 1:1 mapping
		int charCount = 0;
		while (charCount < remainingSize && src[charCount] != 0)
			charCount++;

		// create Unicode buffer
		m_unicode.resize(charCount);
		m_length = charCount;

		for (int i = 0; i < m_length; i++)
			m_unicode[i] = static_cast<wchar_t>(static_cast<unsigned char>(src[i]));

		// update UTF-8 representation
		m_isAsciiOnly = true;
		m_lengthUtf8 = m_length;
		m_utf8.assign(src, m_lengthUtf8);

		return m_length;
	}

	// optimized non-ASCII processing using std::string_view and utf8 decoding
	std::string_view srcView(src, remainingSize);

	// first pass: count characters
	int charCount = 0;
	for (size_t i = 0; i < srcView.size() && srcView[i] != 0;)
	{
		const auto b = static_cast<unsigned char>(srcView[i]);

		if (b < 0x80)
			i += 1;
		else if ((b & USTRING_MASK_2BYTE) == USTRING_VALUE_2BYTE)
			i += 2;
		else if ((b & USTRING_MASK_3BYTE) == USTRING_VALUE_3BYTE)
			i += 3;
		else if ((b & USTRING_MASK_4BYTE) == USTRING_VALUE_4BYTE)
			i += 4;
		else if ((b & USTRING_MASK_5BYTE) == USTRING_VALUE_5BYTE)
			i += 5;
		else if ((b & USTRING_MASK_6BYTE) == USTRING_VALUE_6BYTE)
			i += 6;
		else
			i += 1;

		charCount++;
	}

	// second pass: decode into the buffer
	m_unicode.resize(charCount);
	m_length = charCount;

	int destIndex = 0;
	for (size_t i = 0; i < srcView.size() && srcView[i] != 0 && destIndex < m_length;)
	{
		const auto b = static_cast<unsigned char>(srcView[i]);

		if (b < 0x80)
		{
			m_unicode[destIndex++] = b;
			i += 1;
		}
		else if ((b & USTRING_MASK_2BYTE) == USTRING_VALUE_2BYTE && i + 1 < srcView.size())
		{
			m_unicode[destIndex++] = getCodePoint(srcView.data(), static_cast<int>(i), 2, static_cast<unsigned char>(~USTRING_MASK_2BYTE));
			i += 2;
		}
		else if ((b & USTRING_MASK_3BYTE) == USTRING_VALUE_3BYTE && i + 2 < srcView.size())
		{
			m_unicode[destIndex++] = getCodePoint(srcView.data(), static_cast<int>(i), 3, static_cast<unsigned char>(~USTRING_MASK_3BYTE));
			i += 3;
		}
		else if ((b & USTRING_MASK_4BYTE) == USTRING_VALUE_4BYTE && i + 3 < srcView.size())
		{
			m_unicode[destIndex++] = getCodePoint(srcView.data(), static_cast<int>(i), 4, static_cast<unsigned char>(~USTRING_MASK_4BYTE));
			i += 4;
		}
		else if ((b & USTRING_MASK_5BYTE) == USTRING_VALUE_5BYTE && i + 4 < srcView.size())
		{
			m_unicode[destIndex++] = getCodePoint(srcView.data(), static_cast<int>(i), 5, static_cast<unsigned char>(~USTRING_MASK_5BYTE));
			i += 5;
		}
		else if ((b & USTRING_MASK_6BYTE) == USTRING_VALUE_6BYTE && i + 5 < srcView.size())
		{
			m_unicode[destIndex++] = getCodePoint(srcView.data(), static_cast<int>(i), 6, static_cast<unsigned char>(~USTRING_MASK_6BYTE));
			i += 6;
		}
		else
		{
			m_unicode[destIndex++] = '?';
			i += 1;
		}
	}

	m_isAsciiOnly = false;

	// store the UTF-8 string directly if it was a valid substring
	if (startIndex == 0 && length > 0)
	{
		m_utf8.assign(utf8, length);
		m_lengthUtf8 = length;
	}
	else
	{
		updateUtf8();
	}

	return m_length;
}

void UString::updateUtf8()
{
	// check if the string is empty
	if (m_length == 0)
	{
		m_utf8.clear();
		m_lengthUtf8 = 0;
		m_isAsciiOnly = true;
		return;
	}

	// fast ASCII check (common case)
	m_isAsciiOnly = true;
	for (int i = 0; i < m_length; i++)
	{
		if (m_unicode[i] >= 0x80)
		{
			m_isAsciiOnly = false;
			break;
		}
	}

	// fast path for ASCII-only strings
	if (m_isAsciiOnly)
	{
		m_utf8.resize(m_length);
		for (int i = 0; i < m_length; i++)
		{
			m_utf8[i] = static_cast<char>(m_unicode[i]);
		}
		m_lengthUtf8 = m_length;
		return;
	}

	// for non-ASCII strings, calculate needed length in a single pass
	int newLength = encode(m_unicode, nullptr);
	m_utf8.resize(newLength);
	m_lengthUtf8 = newLength;
	encode(m_unicode, m_utf8.data());
}
