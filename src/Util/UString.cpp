//============== Copyright (c) 2009, 2D Boy & PG, All rights reserved. ===============//
//
// Purpose:		unicode string class (modified)
//
// $NoKeywords: $ustring $string
//====================================================================================//

#include "UString.h"

#include <wchar.h>
#include <string.h>

#define USTRING_MASK_1BYTE  0x80 /* 1000 0000 */
#define USTRING_VALUE_1BYTE 0x00 /* 0000 0000 */
#define USTRING_MASK_2BYTE  0xE0 /* 1110 0000 */
#define USTRING_VALUE_2BYTE 0xC0 /* 1100 0000 */
#define USTRING_MASK_3BYTE  0xF0 /* 1111 0000 */
#define USTRING_VALUE_3BYTE 0xE0 /* 1110 0000 */
#define USTRING_MASK_4BYTE  0xF8 /* 1111 1000 */
#define USTRING_VALUE_4BYTE 0xF0 /* 1111 0000 */
#define USTRING_MASK_5BYTE  0xFC /* 1111 1100 */
#define USTRING_VALUE_5BYTE 0xF8 /* 1111 1000 */
#define USTRING_MASK_6BYTE  0xFE /* 1111 1110 */
#define USTRING_VALUE_6BYTE 0xFC /* 1111 1100 */

#define USTRING_MASK_MULTIBYTE 0x3F /* 0011 1111 */

#define USTRING_ESCAPE_CHAR '\\'

constexpr char UString::nullString[];
constexpr wchar_t UString::nullWString[];

UString::UString()
{
	mLength = 0;
	mLengthUtf8 = 0;
	mIsAsciiOnly = false;
	mUnicode = (wchar_t*)nullWString;
	mUtf8 = (char*)nullString;
}

UString::UString(const char *utf8)
{
	mLength = 0;
	mLengthUtf8 = 0;
	mUnicode = (wchar_t*)nullWString;
	mUtf8 = (char*)nullString;

	fromUtf8(utf8);
}

UString::UString(const char *utf8, int length)
{
	mLength = 0;
	mLengthUtf8 = length;
	mUnicode = (wchar_t*)nullWString;
	mUtf8 = (char*)nullString;

	fromUtf8(utf8, length);
}

UString::UString(const UString &ustr)
{
	mLength = 0;
	mLengthUtf8 = 0;
	mUnicode = (wchar_t*)nullWString;
	mUtf8 = (char*)nullString;

	(*this) = ustr;
}

UString::UString(UString &&ustr)
{
	// move
	mLength = ustr.mLength;
	mLengthUtf8 = ustr.mLengthUtf8;
	mIsAsciiOnly = ustr.mIsAsciiOnly;
	mUnicode = ustr.mUnicode;
	mUtf8 = ustr.mUtf8;

	// reset source
	ustr.mLength = 0;
	ustr.mIsAsciiOnly = false;
	ustr.mUnicode = NULL;
	ustr.mUtf8 = NULL;
}

UString::UString(const wchar_t *str)
{
	// get length
	mLength = (str != NULL ? (int)std::wcslen(str) : 0);

	// allocate new mem for unicode data
	mUnicode = new wchar_t[mLength + 1]; // +1 for null termination later

	// copy contents and null terminate
	if (mLength > 0)
		memcpy(mUnicode, str, (mLength)*sizeof(wchar_t));

	mUnicode[mLength] = '\0'; // null terminate

	// null out and rebuild the utf version
	{
		mUtf8 = NULL;
		mLengthUtf8 = 0;
		mIsAsciiOnly = false;

		updateUtf8();
	}
}

UString::~UString()
{
	mLength = 0;
	mLengthUtf8 = 0;

	deleteUnicode();
	deleteUtf8();
}

void UString::clear()
{
	mLength = 0;
	mLengthUtf8 = 0;
	mIsAsciiOnly = false;

	deleteUnicode();
	deleteUtf8();
}

UString UString::format(const char *utf8format, ...)
{
	// decode the utf8 string
	UString formatted;
	int bufSize = formatted.fromUtf8(utf8format) + 1; // +1 for default heuristic (no args, but null char). arguments will need multiple iterations and allocations anyway

	if (formatted.mLength == 0) return formatted;

	// print the args to the format
	wchar_t *buf = NULL;
	int written = -1;
	while (true)
	{
		if (bufSize >= 1024*1024)
		{
			printf("WARNING: Potential vswprintf(%s, ...) infinite loop, stopping ...\n", utf8format);
			return formatted;
		}

		buf = new wchar_t[bufSize];

		va_list ap;
		va_start(ap, utf8format);
		written = vswprintf(buf, bufSize, formatted.mUnicode, ap);
		va_end(ap);

		// if we didn't use the entire buffer
		if (written > 0 && written < bufSize)
		{
			// cool, keep the formatted string and we're done
			if (!formatted.isUnicodeNull())
				delete[] formatted.mUnicode;

			formatted.mUnicode = buf;
			formatted.mLength = written;
			formatted.updateUtf8();

			break;
		}
		else
		{
			// we need a larger buffer
			delete[] buf;
			bufSize *= 2;
		}
	}

	return formatted;
}

bool UString::isWhitespaceOnly() const
{
	int startPos = 0;
	while (startPos < mLength)
	{
		if (!std::iswspace(mUnicode[startPos]))
			return false;

		startPos++;
	}

	return true;
}

int UString::findChar(wchar_t ch, int start, bool respectEscapeChars) const
{
	bool escaped = false;
	for (int i=start; i<mLength; i++)
	{
		// if we're respecting escape chars AND we are not in an escape
		// sequence AND we've found an escape character
		if (respectEscapeChars && !escaped && mUnicode[i] == USTRING_ESCAPE_CHAR)
		{
			// now we're in an escape sequence
			escaped = true;
		}
		else
		{
			if (!escaped && mUnicode[i] == ch)
				return i;

			escaped = false;
		}
	}

	return -1;
}

int UString::findChar(const UString &str, int start, bool respectEscapeChars) const
{
	bool escaped = false;
	for (int i=start; i<mLength; i++)
	{
		// if we're respecting escape chars AND we are not in an escape
		// sequence AND we've found an escape character
		if (respectEscapeChars && !escaped && mUnicode[i] == USTRING_ESCAPE_CHAR)
		{
			// now we're in an escape sequence
			escaped = true;
		}
		else
		{
			if (!escaped && str.findChar(mUnicode[i]) >= 0)
				return i;

			escaped = false;
		}
	}

	return -1;
}

int UString::find(const UString &str, int start) const
{
	const int lastPossibleMatch = mLength - str.mLength;
	for (int i=start; i<=lastPossibleMatch; i++)
	{
		if (memcmp(&(mUnicode[i]), str.mUnicode, str.mLength * sizeof(*mUnicode)) == 0)
			return i;
	}

	return -1;
}

int UString::find(const UString &str, int start, int end) const
{
	const int lastPossibleMatch = mLength - str.mLength;
	for (int i=start; i<=lastPossibleMatch && i<end; i++)
	{
		if (memcmp(&(mUnicode[i]), str.mUnicode, str.mLength * sizeof(*mUnicode)) == 0)
			return i;
	}

	return -1;
}

int UString::findLast(const UString &str, int start) const
{
	int lastI = -1;
	for (int i=start; i<mLength; i++)
	{
		if (memcmp(&(mUnicode[i]), str.mUnicode, str.mLength * sizeof(*mUnicode)) == 0)
			lastI = i;
	}

	return lastI;
}

int UString::findLast(const UString &str, int start, int end) const
{
	int lastI = -1;
	for (int i=start; i<mLength && i<end; i++)
	{
		if (memcmp(&(mUnicode[i]), str.mUnicode, str.mLength * sizeof(*mUnicode)) == 0)
			lastI = i;
	}

	return lastI;
}

int UString::findIgnoreCase(const UString &str, int start) const
{
	const int lastPossibleMatch = mLength - str.mLength;
	for (int i=start; i<=lastPossibleMatch; i++)
	{
		bool equal = true;
		for (int c=0; c<str.mLength; c++)
		{
			if ((std::towlower(mUnicode[i + c]) - std::towlower(str.mUnicode[c])) != 0)
			{
				equal = false;
				break;
			}
		}
		if (equal)
			return i;
	}

	return -1;
}

int UString::findIgnoreCase(const UString &str, int start, int end) const
{
	const int lastPossibleMatch = mLength - str.mLength;
	for (int i=start; i<=lastPossibleMatch && i<end; i++)
	{
		bool equal = true;
		for (int c=0; c<str.mLength; c++)
		{
			if ((std::towlower(mUnicode[i + c]) - std::towlower(str.mUnicode[c])) != 0)
			{
				equal = false;
				break;
			}
		}
		if (equal)
			return i;
	}

	return -1;
}

static forceinline void getUtf8(wchar_t ch, char *utf8, int numBytes, int firstByteValue)
{
	if (utf8 == NULL) return;

	for (int i=numBytes-1; i>0; i--)
	{
		// store the lowest bits in a utf8 byte
		utf8[i] = (ch & USTRING_MASK_MULTIBYTE) | 0x80;
		ch >>= 6;
	}

	// store the remaining bits
	*utf8 = (firstByteValue | ch);
}

static forceinline int encode(const wchar_t *unicode, int length, char *utf8)
{
	if (unicode == NULL) return 0;

	int utf8len = 0;

	for (int i=0; i<length; i++)
	{
		const wchar_t ch = unicode[i];

		if (ch < 0x00000080) // 1 byte
		{
			if (utf8 != NULL)
				utf8[utf8len] = (char)ch;

			utf8len += 1;
		}
		else if (ch < 0x00000800) // 2 bytes
		{
			if (utf8 != NULL)
				getUtf8(ch, &(utf8[utf8len]), 2, USTRING_VALUE_2BYTE);

			utf8len += 2;
		}
		else if (ch < 0x00010000) // 3 bytes
		{
			if (utf8 != NULL)
				getUtf8(ch, &(utf8[utf8len]), 3, USTRING_VALUE_3BYTE);

			utf8len += 3;
		}
		else if (ch < 0x00200000) // 4 bytes
		{
			if (utf8 != NULL)
				getUtf8(ch, &(utf8[utf8len]), 4, USTRING_VALUE_4BYTE);

			utf8len += 4;
		}
		else if (ch < 0x04000000) // 5 bytes
		{
			if (utf8 != NULL)
				getUtf8(ch, &(utf8[utf8len]), 5, USTRING_VALUE_5BYTE);

			utf8len += 5;
		}
		else // 6 bytes
		{
			if (utf8 != NULL)
				getUtf8(ch, &(utf8[utf8len]), 6, USTRING_VALUE_6BYTE);

			utf8len += 6;
		}
	}

	return utf8len;
}

static forceinline wchar_t getCodePoint(const char *utf8, int offset, int numBytes, unsigned char firstByteMask)
{
	if (utf8 == NULL) return (wchar_t)0;

	// get the bits out of the first byte
	wchar_t wc = utf8[offset] & firstByteMask;

	// iterate over the rest of the bytes
	for (int i=1; i<numBytes; i++)
	{
		// shift the code point bits to make room for the new bits
		wc = wc << 6;

		// add the new bits
		wc |= utf8[offset+i] & USTRING_MASK_MULTIBYTE;
	}

	// return the code point
	return wc;
}

void UString::collapseEscapes()
{
	if (mLength == 0) return;

	int writeIndex = 0;
	bool escaped = false;
	wchar_t *buf = new wchar_t[mLength];

	// iterate over the unicode string
	for (int readIndex=0; readIndex<mLength; readIndex++)
	{
		// if we're not already escaped and this is an escape char
		if (!escaped && mUnicode[readIndex] == USTRING_ESCAPE_CHAR)
		{
			// we're escaped
			escaped = true;
		}
		else
		{
			// move this char over and increment the write index
			buf[writeIndex] = mUnicode[readIndex];
			writeIndex++;

			// we're no longer escaped
			escaped = false;
		}
	}

	// replace old data with new data
	deleteUnicode();
	mLength = writeIndex;
	mUnicode = new wchar_t[mLength];
	memcpy(mUnicode, buf, mLength*sizeof(wchar_t));

	// free the temporary buffer
	delete[] buf;

	// the utf encoding is out of date, update it
	updateUtf8();
}

void UString::append(const UString &str)
{
	if (str.mLength == 0) return;

	// calculate new size
	const int newSize = mLength + str.mLength;

	// allocate new data buffer
	wchar_t *newUnicode = new wchar_t[newSize + 1]; // +1 for null termination later

	// copy existing data
	if (mLength > 0)
		memcpy(newUnicode, mUnicode, mLength*sizeof(wchar_t));

	// copy appended data
	memcpy(&(newUnicode[mLength]), str.mUnicode, (str.mLength + 1)*sizeof(wchar_t)); // +1 to also copy the null char from the old string

	// replace the old values with the new
	deleteUnicode();
	mUnicode = newUnicode;
	mLength = newSize;

	// reencode to utf8
	updateUtf8();
}

void UString::append(wchar_t ch)
{
	// calculate new size
	const int newSize = mLength + 1;

	// allocate new data buffer
	wchar_t *newUnicode = new wchar_t[newSize + 1]; // +1 for null termination later

	// copy existing data
	if (mLength > 0)
		memcpy(newUnicode, mUnicode, mLength*sizeof(wchar_t));

	// copy appended data and null terminate
	newUnicode[mLength] = ch;
	newUnicode[mLength + 1] = '\0';

	// replace the old values with the new
	deleteUnicode();
	mUnicode = newUnicode;
	mLength = newSize;

	// reencode to utf8
	updateUtf8();
}

void UString::insert(int offset, const UString &str)
{
	if (str.mLength == 0) return;

	offset = clamp<int>(offset, 0, mLength);

	// calculate new size
	const int newSize = mLength + str.mLength;

	// allocate new data buffer
	wchar_t *newUnicode = new wchar_t[newSize + 1]; // +1 for null termination later

	// if we're not inserting at the beginning of the string
	if (offset > 0)
		memcpy(newUnicode, mUnicode, offset*sizeof(wchar_t)); // copy first part of data

	// copy inserted string
	memcpy(&(newUnicode[offset]), str.mUnicode, str.mLength*sizeof(wchar_t));

	// if we're not inserting at the end of the string
	if (offset < mLength)
	{
		// copy rest of string (including terminating null char)
		const int numRightChars = (mLength - offset + 1);
		if (numRightChars > 0)
			memcpy(&(newUnicode[offset + str.mLength]), &(mUnicode[offset]), (numRightChars)*sizeof(wchar_t));
	}
	else
		newUnicode[newSize] = '\0';  // null terminate

	// replace the old values with the new
	deleteUnicode();
	mUnicode = newUnicode;
	mLength = newSize;

	// reencode to utf8
	updateUtf8();
}

void UString::insert(int offset, wchar_t ch)
{
	offset = clamp<int>(offset, 0, mLength);

	// calculate new size
	const int newSize = mLength + 1; // +1 for the added character

	// allocate new data buffer
	wchar_t *newUnicode = new wchar_t[newSize + 1]; // and again +1 for null termination later

	// copy first part of data
	if (offset > 0)
		memcpy(newUnicode, mUnicode, offset*sizeof(wchar_t));

	// place the inserted char
	newUnicode[offset] = ch;

	// copy rest of string (including terminating null char)
	const int numRightChars = mLength - offset + 1;
	if (numRightChars > 0)
		memcpy(&(newUnicode[offset + 1]), &(mUnicode[offset]), (numRightChars)*sizeof(wchar_t));

	// replace the old values with the new
	deleteUnicode();
	mUnicode = newUnicode;
	mLength = newSize;

	// reencode to utf8
	updateUtf8();
}

void UString::erase(int offset, int count)
{
	if (isUnicodeNull() || mLength == 0 || count == 0 || offset > (mLength - 1)) return;

	offset = clamp<int>(offset, 0, mLength);
	count = clamp<int>(count, 0, mLength - offset);

	// calculate new size
	const int newLength = mLength - count;

	// allocate new data buffer
	wchar_t *newUnicode = new wchar_t[newLength + 1]; // +1 for null termination later

	// copy first part of data
	if (offset > 0)
		memcpy(newUnicode, mUnicode, offset*sizeof(wchar_t));

	// copy rest of string (including terminating null char)
	const int numRightChars = newLength - offset + 1;
	if (numRightChars > 0)
		memcpy(&(newUnicode[offset]), &(mUnicode[offset + count]), (numRightChars)*sizeof(wchar_t));

	// replace the old values with the new
	deleteUnicode();
	mUnicode = newUnicode;
	mLength = newLength;

	// reencode to utf8
	updateUtf8();
}

UString UString::substr(int offset, int charCount) const
{
	offset = clamp<int>(offset, 0, mLength);

	if (charCount < 0)
		charCount = mLength - offset;

	charCount = clamp<int>(charCount, 0, mLength - offset);

	// allocate new mem
	UString str;
	str.mLength = charCount;
	str.mUnicode = new wchar_t[charCount + 1]; // +1 for null termination later

	// copy mem contents
	if (charCount > 0)
		memcpy(str.mUnicode, &(mUnicode[offset]), charCount * sizeof(wchar_t));

	str.mUnicode[charCount] = '\0'; // null terminate

	// update the substring's utf encoding
	str.updateUtf8();

	return str;
}

std::vector<UString> UString::split(UString delim) const
{
	std::vector<UString> results;
	if (delim.length() < 1 || mLength < 1) return results;

	int start = 0;
	int end = 0;

	while ((end = find(delim, start)) != -1)
	{
		results.push_back(substr(start, end - start));
		start = end + delim.length();
	}
	results.push_back(substr(start, end - start));

	return results;
}

UString UString::trim() const
{
	int startPos = 0;
	while (startPos < mLength && std::iswspace(mUnicode[startPos]))
	{
		startPos++;
	}

	int endPos = mLength - 1;
	while ((endPos >= 0) && (endPos < mLength) && std::iswspace(mUnicode[endPos]))
	{
		endPos--;
	}

	return substr(startPos, endPos - startPos + 1);
}

float UString::toFloat() const
{
	return !isUtf8Null() ? std::strtof(mUtf8, NULL) : 0.0f;
}

double UString::toDouble() const
{
	return !isUtf8Null() ? std::strtod(mUtf8, NULL) : 0.0;
}

long double UString::toLongDouble() const
{
	return !isUtf8Null() ? std::strtold(mUtf8, NULL) : 0.0l;
}

int UString::toInt() const
{
	return !isUtf8Null() ? (int)std::strtol(mUtf8, NULL, 0) : 0;
}

long UString::toLong() const
{
	return !isUtf8Null() ? std::strtol(mUtf8, NULL, 0) : 0;
}

long long UString::toLongLong() const
{
	return !isUtf8Null() ? std::strtoll(mUtf8, NULL, 0) : 0;
}

unsigned int UString::toUnsignedInt() const
{
	return !isUtf8Null() ? (unsigned int)std::strtoul(mUtf8, NULL, 0) : 0;
}

unsigned long UString::toUnsignedLong() const
{
	return !isUtf8Null() ? std::strtoul(mUtf8, NULL, 0) : 0;
}

unsigned long long UString::toUnsignedLongLong() const
{
	return !isUtf8Null() ? std::strtoull(mUtf8, NULL, 0) : 0;
}

void UString::lowerCase()
{
	if (!isUnicodeNull() && !isUtf8Null() && mLength > 0)
	{
		for (int i=0; i<mLength; i++)
		{
			mUnicode[i] = std::towlower(mUnicode[i]);
			mUtf8[i] = std::tolower(mUtf8[i]);
		}

		if (!mIsAsciiOnly)
			updateUtf8();
	}
}

void UString::upperCase()
{
	if (!isUnicodeNull() && !isUtf8Null() && mLength > 0)
	{
		for (int i=0; i<mLength; i++)
		{
			mUnicode[i] = std::towupper(mUnicode[i]);
			mUtf8[i] = std::toupper(mUtf8[i]);
		}

		if (!mIsAsciiOnly)
			updateUtf8();
	}
}

wchar_t UString::operator [] (int index) const
{
	if (mLength > 0)
		return mUnicode[clamp<int>(index, 0, mLength - 1)];

	return (wchar_t)0;
}

UString &UString::operator = (const UString &ustr)
{
	wchar_t *newUnicode = (wchar_t*)nullWString;

	// if this is not a null string
	if (ustr.mLength > 0 && !ustr.isUnicodeNull())
	{
		// allocate new mem for unicode data
		newUnicode = new wchar_t[ustr.mLength + 1];

		// copy unicode mem contents
		memcpy(newUnicode, ustr.mUnicode, (ustr.mLength + 1)*sizeof(wchar_t));
	}

	// deallocate old mem
	if (!isUnicodeNull())
		delete[] mUnicode;

	// init variables
	mLength = ustr.mLength;
	mUnicode = newUnicode;

	// reencode to utf8
	updateUtf8();

	return *this;
}

UString &UString::operator = (UString &&ustr)
{
	if (this != &ustr)
	{
		// free ourself
		if (!isUnicodeNull())
			delete[] mUnicode;
		if (!isUtf8Null())
			delete[] mUtf8;

		// move
		mLength = ustr.mLength;
		mLengthUtf8 = ustr.mLengthUtf8;
		mIsAsciiOnly = ustr.mIsAsciiOnly;
		mUnicode = ustr.mUnicode;
		mUtf8 = ustr.mUtf8;

		// reset source
		ustr.mLength = 0;
		ustr.mLengthUtf8 = 0;
		ustr.mIsAsciiOnly = false;
		ustr.mUnicode = NULL;
		ustr.mUtf8 = NULL;
	}

	return *this;
}

bool UString::operator == (const UString &ustr) const
{
	if (mLength != ustr.mLength) return false;

	if (isUnicodeNull() && ustr.isUnicodeNull())
		return true;
	else if (isUnicodeNull() || ustr.isUnicodeNull())
		return (mLength == 0 && ustr.mLength == 0);

	return memcmp(mUnicode, ustr.mUnicode, mLength*sizeof(wchar_t)) == 0;
}

bool UString::operator != (const UString &ustr) const
{
	bool equal = (*this == ustr);
	return !equal;
}

bool UString::operator < (const UString &ustr) const
{
	for (int i=0; i<mLength && i<ustr.mLength; i++)
	{
		if (mUnicode[i] != ustr.mUnicode[i])
			return mUnicode[i] < ustr.mUnicode[i];
	}

	if (mLength == ustr.mLength) return false;

	return mLength < ustr.mLength;
}

bool UString::equalsIgnoreCase(const UString &ustr) const
{
	if (mLength != ustr.mLength) return false;

	if (isUnicodeNull() && ustr.isUnicodeNull())
		return true;
	else if (isUnicodeNull() || ustr.isUnicodeNull())
		return false;

	for (int i=0; i<mLength; i++)
	{
		if (std::towlower(mUnicode[i]) != std::towlower(ustr.mUnicode[i]))
			return false;
	}

	return true;
}

bool UString::lessThanIgnoreCase(const UString &ustr) const
{
	for (int i=0; i<mLength && i<ustr.mLength; i++)
	{
		if (std::towlower(mUnicode[i]) != std::towlower(ustr.mUnicode[i]))
			return std::towlower(mUnicode[i]) < std::towlower(ustr.mUnicode[i]);
	}

	if (mLength == ustr.mLength) return false;

	return mLength < ustr.mLength;
}

int UString::fromUtf8(const char *utf8, int length)
{
	if (utf8 == NULL) return 0;

	const int supposedFullStringSize = (length > -1 ? length : strlen(utf8) + 1); // NOTE: +1 only for the strlen() case, since if we are called with a specific length the buffer might not have a null byte at the end

	int startIndex = 0;
	if (supposedFullStringSize > 2)
	{
		// check for UTF-8 BOM
		if ((unsigned char)utf8[0] == 0xEF &&
			(unsigned char)utf8[1] == 0xBB &&
			(unsigned char)utf8[2] == 0xBF)
		{
			startIndex = 3;
		}
		// check for UTF-16/32 (unsupported)
		else if (((unsigned char)utf8[0] == 0xFE && (unsigned char)utf8[1] == 0xFF) ||
				 ((unsigned char)utf8[0] == 0xFF && (unsigned char)utf8[1] == 0xFE))
		{
			return 0;
		}
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
		deleteUnicode();
		mLength = charCount;
		mUnicode = new wchar_t[mLength + 1];

		for (int i = 0; i < mLength; i++)
			mUnicode[i] = (wchar_t)(unsigned char)src[i];

		mUnicode[mLength] = '\0';

		// update UTF-8 representation
		mIsAsciiOnly = true;
		mLengthUtf8 = mLength;
		deleteUtf8();
		mUtf8 = new char[mLengthUtf8 + 1];
		memcpy(mUtf8, src, mLengthUtf8);
		mUtf8[mLengthUtf8] = '\0';

		return mLength;
	}

	// for non-ASCII, we still need two passes
	// pass 1: count characters
	int charCount = 0;
	for (int i = 0; i < remainingSize && src[i] != 0; )
	{
		const unsigned char b = (unsigned char)src[i];

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
	deleteUnicode();
	mLength = charCount;
	mUnicode = new wchar_t[mLength + 1];

	int destIndex = 0;
	for (int i = 0; i < remainingSize && src[i] != 0 && destIndex < mLength; )
	{
		const unsigned char b = (unsigned char)src[i];

		if (b < 0x80)
		{
			mUnicode[destIndex++] = b;
			i += 1;
		}
		else if ((b & USTRING_MASK_2BYTE) == USTRING_VALUE_2BYTE && i + 1 < remainingSize)
		{
			mUnicode[destIndex++] = getCodePoint(src, i, 2, (unsigned char)(~USTRING_MASK_2BYTE));
			i += 2;
		}
		else if ((b & USTRING_MASK_3BYTE) == USTRING_VALUE_3BYTE && i + 2 < remainingSize)
		{
			mUnicode[destIndex++] = getCodePoint(src, i, 3, (unsigned char)(~USTRING_MASK_3BYTE));
			i += 3;
		}
		else if ((b & USTRING_MASK_4BYTE) == USTRING_VALUE_4BYTE && i + 3 < remainingSize)
		{
			mUnicode[destIndex++] = getCodePoint(src, i, 4, (unsigned char)(~USTRING_MASK_4BYTE));
			i += 4;
		}
		else if ((b & USTRING_MASK_5BYTE) == USTRING_VALUE_5BYTE && i + 4 < remainingSize)
		{
			mUnicode[destIndex++] = getCodePoint(src, i, 5, (unsigned char)(~USTRING_MASK_5BYTE));
			i += 5;
		}
		else if ((b & USTRING_MASK_6BYTE) == USTRING_VALUE_6BYTE && i + 5 < remainingSize)
		{
			mUnicode[destIndex++] = getCodePoint(src, i, 6, (unsigned char)(~USTRING_MASK_6BYTE));
			i += 6;
		}
		else
		{
			mUnicode[destIndex++] = '?';
			i += 1;
		}
	}

	mUnicode[mLength] = '\0';
	updateUtf8();

	return mLength;
}

void UString::updateUtf8()
{
	// check if the string is empty
	if (mLength == 0)
	{
		deleteUtf8();
		mLengthUtf8 = 0;
		mIsAsciiOnly = true;
		mUtf8 = (char*)nullString;
		return;
	}

	// fast ASCII check (common case)
	bool isAscii = true;
	for (int i = 0; i < mLength; i++) {
		if (mUnicode[i] >= 0x80) {
			isAscii = false;
			break;
		}
	}

	// fast path for ASCII-only strings
	if (isAscii) {
		// only reallocate if necessary
		if (mLengthUtf8 < mLength || isUtf8Null()) {
			deleteUtf8();
			mUtf8 = new char[mLength + 1]; // +1 for null termination
		}

		// direct copy for ASCII
		for (int i = 0; i < mLength; i++) {
			mUtf8[i] = (char)mUnicode[i];
		}

		mUtf8[mLength] = '\0';
		mLengthUtf8 = mLength;
		mIsAsciiOnly = true;
		return;
	}

	mIsAsciiOnly = false;
	// for non-ASCII strings, calculate needed length
	int newLength = encode(mUnicode, mLength, NULL);

	// only reallocate if necessary
	if (newLength > mLengthUtf8 || isUtf8Null()) {
		deleteUtf8();
		mUtf8 = new char[newLength + 1]; // +1 for null termination later
	}

	mLengthUtf8 = newLength;
	encode(mUnicode, mLength, mUtf8);
	mUtf8[mLengthUtf8] = '\0'; // null terminate
}
