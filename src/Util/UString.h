//============== Copyright (c) 2009, 2D Boy & PG, All rights reserved. ===============//
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
	UString(UString &&ustr);
	~UString();

	void clear();

	// get
	inline int length() const {return mLength;}
	inline int lengthUtf8() const {return mLengthUtf8;}
	inline const char *toUtf8() const {return mUtf8;}
	inline const wchar_t *wc_str() const {return mUnicode;}
	inline bool isAsciiOnly() const {return mIsAsciiOnly;}
	bool isWhitespaceOnly() const;

	int findChar(wchar_t ch, int start = 0, bool respectEscapeChars = false) const;
	int findChar(const UString &str, int start = 0, bool respectEscapeChars = false) const;
	int find(const UString &str, int start = 0) const;
	int find(const UString &str, int start, int end) const;
	int findLast(const UString &str, int start = 0) const;
	int findLast(const UString &str, int start, int end) const;

	int findIgnoreCase(const UString &str, int start = 0) const;
	int findIgnoreCase(const UString &str, int start, int end) const;

	// modifiers
	void collapseEscapes();
	void append(const UString &str);
	void append(wchar_t ch);
	void insert(int offset, const UString &str);
	void insert(int offset, wchar_t ch);
	void erase(int offset, int count);

	// actions (non-modifying)
	UString substr(int offset, int charCount = -1) const;
	std::vector<UString> split(UString delim) const;
	UString trim() const;

	// conversions
	float toFloat() const;
	double toDouble() const;
	long double toLongDouble() const;
	int toInt() const;
	long toLong() const;
	long long toLongLong() const;
	unsigned int toUnsignedInt() const;
	unsigned long toUnsignedLong() const;
	unsigned long long toUnsignedLongLong() const;

	void lowerCase();
	void upperCase();

	// operators
	wchar_t operator [] (int index) const;
	UString &operator = (const UString &ustr);
	UString &operator = (UString &&ustr);
	bool operator == (const UString &ustr) const;
	bool operator != (const UString &ustr) const;
	bool operator < (const UString &ustr) const;

	bool equalsIgnoreCase(const UString &ustr) const;
	bool lessThanIgnoreCase(const UString &ustr) const;

private:
	int fromUtf8(const char *utf8, int length = -1);

	void updateUtf8();

	// inline deletes, guarantee valid empty string
	forceinline void deleteUnicode()
	{
		if (mUnicode != NULL && mUnicode != nullWString)
			delete[] mUnicode;

		mUnicode = (wchar_t*)nullWString;
	}

	forceinline void deleteUtf8()
	{
		if (mUtf8 != NULL && mUtf8 != nullString)
			delete[] mUtf8;

		mUtf8 = (char*)nullString;
	}

	forceinline bool isUnicodeNull() const {return (mUnicode == NULL || mUnicode == nullWString);}
	forceinline bool isUtf8Null() const {return (mUtf8 == NULL || mUtf8 == nullString);}

	static constexpr char nullString[] = "";
	static constexpr wchar_t nullWString[] = L"";

	int mLength;
	int mLengthUtf8;
	bool mIsAsciiOnly;

	wchar_t *mUnicode;
	char *mUtf8;
};

#endif
