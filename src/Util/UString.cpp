//========= Copyright (c) 2009, 2D Boy & PG & 2025, WH, All rights reserved. =========//
//
// Purpose:		unicode string class (modified)
//
// $NoKeywords: $ustring $string
//====================================================================================//

#include "UString.h"
#include <cstdarg>
#include <cstring>
#include <cwchar>
#include <cwctype>
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

UString::UString() : m_length(0), m_lengthUtf8(0), m_isAsciiOnly(true) {}

UString::UString(const wchar_t *str) : m_length(0), m_lengthUtf8(0), m_isAsciiOnly(true)
{
	if (str && *str)
	{
		m_unicode = str;
		m_length = static_cast<int>(m_unicode.length());
		updateUtf8();
	}
}

UString::UString(const char *utf8) : m_length(0), m_lengthUtf8(0), m_isAsciiOnly(true)
{
	if (utf8 && *utf8)
	{
		fromUtf8(utf8);
	}
}

UString::UString(const char *utf8, int length) : m_length(0), m_lengthUtf8(length), m_isAsciiOnly(true)
{
	if (utf8 && length > 0)
	{
		fromUtf8(utf8, length);
	}
}

UString::UString(const UString &ustr) = default;

UString::UString(UString &&ustr) noexcept
    : m_unicode(std::move(ustr.m_unicode)), m_utf8(std::move(ustr.m_utf8)), m_length(ustr.m_length), m_lengthUtf8(ustr.m_lengthUtf8), m_isAsciiOnly(ustr.m_isAsciiOnly)
{
	// reset moved-from object
	ustr.m_length = 0;
	ustr.m_lengthUtf8 = 0;
	ustr.m_isAsciiOnly = true;
}

UString::~UString() = default;

void UString::clear()
{
	m_unicode.clear();
	m_utf8.clear();
	m_length = 0;
	m_lengthUtf8 = 0;
	m_isAsciiOnly = true;
}

UString UString::format(const char *utf8format, ...)
{
	if (!utf8format || *utf8format == '\0')
		return {};

	// create a UString from the format string
	UString formatted(utf8format);
	if (formatted.m_length == 0)
		return formatted;

	int bufSize = std::max(128, formatted.m_length * 2);
	std::wstring buffer;
	int written = -1;

	while (true)
	{
		if (unlikely(bufSize >= 1024 * 1024))
		{
			printf("WARNING: UString::format buffer size limit reached for format: %s\n", utf8format);
			return formatted;
		}

		buffer.resize(bufSize);

		va_list ap;
		va_start(ap, utf8format);
		written = vswprintf(buffer.data(), bufSize, formatted.wc_str(), ap);
		va_end(ap);

		// if we didn't use the entire buffer
		if (likely(written >= 0))
		{
			UString result;
			buffer.resize(written);
			result.m_unicode = std::move(buffer);
			result.m_length = written;
			result.updateUtf8();
			return result;
		}
		// we need a larger buffer
		bufSize *= 2;
	}

	std::unreachable();
}

bool UString::isWhitespaceOnly() const
{
	return std::ranges::all_of(m_unicode, [](wchar_t c) { return std::iswspace(c); });
}

int UString::findChar(wchar_t ch, int start, bool respectEscapeChars) const
{
	if (start < 0 || start >= m_length)
		return -1;

	bool escaped = false;
	for (int i = start; i < m_length; i++)
	{
		// if we're respecting escape chars AND we are not in an escape
		// sequence AND we've found an escape character
		if (respectEscapeChars && !escaped && m_unicode[i] == ESCAPE_CHAR)
		{
			// now we're in an escape sequence
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

	bool charMap[0x10000] = {}; // lookup table, assumes 16-bit wide chars
	for (int i = 0; i < str.m_length; i++)
		charMap[str.m_unicode[i]] = true;

	bool escaped = false;
	for (int i = start; i < m_length; i++)
	{
		// if we're respecting escape chars AND we are not in an escape
		// sequence AND we've found an escape character
		if (respectEscapeChars && !escaped && m_unicode[i] == ESCAPE_CHAR)
		{
			// now we're in an escape sequence
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

	size_t pos = m_unicode.rfind(str.m_unicode, m_length - str.m_length);
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
		if (std::memcmp(&m_unicode[i], str.m_unicode.c_str(), str.m_length * sizeof(wchar_t)) == 0)
			return i;
	}

	return -1;
}

int UString::findIgnoreCase(const UString &str, int start) const
{
	if (start < 0 || str.m_length == 0 || start > m_length - str.m_length)
		return -1;

	std::wstring lowerThis = m_unicode;
	std::wstring lowerStr = str.m_unicode;
	std::ranges::transform(lowerThis, lowerThis.begin(), [](wchar_t c) { return std::towlower(c); });
	std::ranges::transform(lowerStr, lowerStr.begin(), [](wchar_t c) { return std::towlower(c); });

	size_t pos = lowerThis.find(lowerStr, start);
	return (pos != std::wstring::npos) ? static_cast<int>(pos) : -1;
}

int UString::findIgnoreCase(const UString &str, int start, int end) const
{
	if (start < 0 || end > m_length || start >= end || str.m_length == 0)
		return -1;

	std::wstring lowerThis = m_unicode.substr(start, end - start);
	std::wstring lowerStr = str.m_unicode;
	std::ranges::transform(lowerThis, lowerThis.begin(), [](wchar_t c) { return std::towlower(c); });
	std::ranges::transform(lowerStr, lowerStr.begin(), [](wchar_t c) { return std::towlower(c); });

	size_t pos = lowerThis.find(lowerStr);
	return (pos != std::wstring::npos) ? static_cast<int>(pos + start) : -1;
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
		// if we're not already escaped and this is an escape char
		if (!escaped && ch == ESCAPE_CHAR)
		{
			// we're escaped
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
	updateUtf8();
}

void UString::append(wchar_t ch)
{
	m_unicode.push_back(ch);
	m_length = static_cast<int>(m_unicode.length());
	updateUtf8();
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

	int startPos = 0;
	while (startPos < m_length && std::iswspace(m_unicode[startPos]))
	{
		startPos++;
	}

	int endPos = m_length - 1;
	while (endPos >= 0 && std::iswspace(m_unicode[endPos]))
	{
		endPos--;
	}

	if (startPos > endPos)
		return {};

	return substr(startPos, endPos - startPos + 1);
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

wchar_t UString::operator[](int index) const
{
	if (m_length > 0)
		return m_unicode[std::clamp(index, 0, m_length - 1)];

	return static_cast<wchar_t>(0);
}

UString &UString::operator=(const UString &ustr)
{
	if (this != &ustr)
	{
		m_unicode = ustr.m_unicode;
		m_utf8 = ustr.m_utf8;
		m_length = ustr.m_length;
		m_lengthUtf8 = ustr.m_lengthUtf8;
		m_isAsciiOnly = ustr.m_isAsciiOnly;
	}
	return *this;
}

UString &UString::operator=(UString &&ustr) noexcept
{
	if (this != &ustr)
	{
		m_unicode = std::move(ustr.m_unicode);
		m_utf8 = std::move(ustr.m_utf8);
		m_length = ustr.m_length;
		m_lengthUtf8 = ustr.m_lengthUtf8;
		m_isAsciiOnly = ustr.m_isAsciiOnly;

		// reset moved-from object
		ustr.m_length = 0;
		ustr.m_lengthUtf8 = 0;
		ustr.m_isAsciiOnly = true;
	}
	return *this;
}

bool UString::operator==(const UString &ustr) const
{
	if (m_length != ustr.m_length)
		return false;
	if (m_length == 0 && ustr.m_length == 0)
		return true;

	return m_unicode == ustr.m_unicode;
}

bool UString::operator!=(const UString &ustr) const
{
	return !(*this == ustr);
}

bool UString::operator<(const UString &ustr) const
{
	return m_unicode < ustr.m_unicode;
}

// appends to an existing string
UString &UString::operator+=(const UString &ustr)
{
	append(ustr);
	return *this;
}

// creates a new string
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
	append(ch);
	return *this;
}

UString UString::operator+(char ch) const
{
	UString result(*this);
	result.append(ch);
	return result;
}

bool UString::equalsIgnoreCase(const UString &ustr) const
{
	if (m_length != ustr.m_length)
		return false;
	if (m_length == 0 && ustr.m_length == 0)
		return true;

	for (int i = 0; i < m_length; i++)
	{
		if (std::towlower(m_unicode[i]) != std::towlower(ustr.m_unicode[i]))
			return false;
	}

	return true;
}

bool UString::lessThanIgnoreCase(const UString &ustr) const
{
	for (int i = 0; i < m_length && i < ustr.m_length; i++)
	{
		wchar_t c1 = std::towlower(m_unicode[i]);
		wchar_t c2 = std::towlower(ustr.m_unicode[i]);
		if (c1 != c2)
			return c1 < c2;
	}

	if (m_length == ustr.m_length)
		return false;
	return m_length < ustr.m_length;
}

// helper function for getUtf8
static forceinline void getUtf8(wchar_t ch, char *utf8, int numBytes, int firstByteValue)
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
static inline int encode(const std::wstring &unicode, int length, char *utf8)
{
	int utf8len = 0;

	for (int i = 0; i < length; i++)
	{
		const wchar_t ch = unicode[i];

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

	const int supposedFullStringSize = (length > -1 ? length : strlen(utf8) + 1);

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

	// for non-ASCII, we still need two passes
	// pass 1: count characters
	int charCount = 0;
	for (int i = 0; i < remainingSize && src[i] != 0;)
	{
		const auto b = static_cast<unsigned char>(src[i]);

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

	// pass 2: decode into the buffer
	m_unicode.resize(charCount);
	m_length = charCount;

	int destIndex = 0;
	for (int i = 0; i < remainingSize && src[i] != 0 && destIndex < m_length;)
	{
		const auto b = static_cast<unsigned char>(src[i]);

		if (b < 0x80)
		{
			m_unicode[destIndex++] = b;
			i += 1;
		}
		else if ((b & USTRING_MASK_2BYTE) == USTRING_VALUE_2BYTE && i + 1 < remainingSize)
		{
			m_unicode[destIndex++] = getCodePoint(src, i, 2, static_cast<unsigned char>(~USTRING_MASK_2BYTE));
			i += 2;
		}
		else if ((b & USTRING_MASK_3BYTE) == USTRING_VALUE_3BYTE && i + 2 < remainingSize)
		{
			m_unicode[destIndex++] = getCodePoint(src, i, 3, static_cast<unsigned char>(~USTRING_MASK_3BYTE));
			i += 3;
		}
		else if ((b & USTRING_MASK_4BYTE) == USTRING_VALUE_4BYTE && i + 3 < remainingSize)
		{
			m_unicode[destIndex++] = getCodePoint(src, i, 4, static_cast<unsigned char>(~USTRING_MASK_4BYTE));
			i += 4;
		}
		else if ((b & USTRING_MASK_5BYTE) == USTRING_VALUE_5BYTE && i + 4 < remainingSize)
		{
			m_unicode[destIndex++] = getCodePoint(src, i, 5, static_cast<unsigned char>(~USTRING_MASK_5BYTE));
			i += 5;
		}
		else if ((b & USTRING_MASK_6BYTE) == USTRING_VALUE_6BYTE && i + 5 < remainingSize)
		{
			m_unicode[destIndex++] = getCodePoint(src, i, 6, static_cast<unsigned char>(~USTRING_MASK_6BYTE));
			i += 6;
		}
		else
		{
			m_unicode[destIndex++] = '?';
			i += 1;
		}
	}

	m_isAsciiOnly = false;
	updateUtf8();

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

	// for non-ASCII strings, calculate needed length
	int newLength = encode(m_unicode, m_length, nullptr);
	m_utf8.resize(newLength);
	m_lengthUtf8 = newLength;
	encode(m_unicode, m_length, m_utf8.data());
}
