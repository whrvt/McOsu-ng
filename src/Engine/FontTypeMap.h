//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		debug font ranges (TEMP)
//
// $NoKeywords: $font
//===============================================================================//

#pragma once

namespace FontTypeMap
{
inline const char *getCharacterRangeName(wchar_t ch)
{
	if (ch >= 0x0000 && ch <= 0x007F)
		return "Basic Latin";
	if (ch >= 0x0080 && ch <= 0x00FF)
		return "Latin-1 Supplement";
	if (ch >= 0x0100 && ch <= 0x017F)
		return "Latin Extended-A";
	if (ch >= 0x0180 && ch <= 0x024F)
		return "Latin Extended-B";
	if (ch >= 0x0370 && ch <= 0x03FF)
		return "Greek and Coptic";
	if (ch >= 0x0400 && ch <= 0x04FF)
		return "Cyrillic";
	if (ch >= 0x0500 && ch <= 0x052F)
		return "Cyrillic Supplement";
	if (ch >= 0x0590 && ch <= 0x05FF)
		return "Hebrew";
	if (ch >= 0x0600 && ch <= 0x06FF)
		return "Arabic";
	if (ch >= 0x0700 && ch <= 0x074F)
		return "Syriac";
	if (ch >= 0x0780 && ch <= 0x07BF)
		return "Thaana";
	if (ch >= 0x0900 && ch <= 0x097F)
		return "Devanagari";
	if (ch >= 0x0980 && ch <= 0x09FF)
		return "Bengali";
	if (ch >= 0x0A00 && ch <= 0x0A7F)
		return "Gurmukhi";
	if (ch >= 0x0A80 && ch <= 0x0AFF)
		return "Gujarati";
	if (ch >= 0x0B00 && ch <= 0x0B7F)
		return "Oriya";
	if (ch >= 0x0B80 && ch <= 0x0BFF)
		return "Tamil";
	if (ch >= 0x0C00 && ch <= 0x0C7F)
		return "Telugu";
	if (ch >= 0x0C80 && ch <= 0x0CFF)
		return "Kannada";
	if (ch >= 0x0D00 && ch <= 0x0D7F)
		return "Malayalam";
	if (ch >= 0x0E00 && ch <= 0x0E7F)
		return "Thai";
	if (ch >= 0x0E80 && ch <= 0x0EFF)
		return "Lao";
	if (ch >= 0x1000 && ch <= 0x109F)
		return "Myanmar";
	if (ch >= 0x10A0 && ch <= 0x10FF)
		return "Georgian";
	if (ch >= 0x1100 && ch <= 0x11FF)
		return "Hangul Jamo";
	if (ch >= 0x1200 && ch <= 0x137F)
		return "Ethiopic";
	if (ch >= 0x1400 && ch <= 0x167F)
		return "Unified Canadian Aboriginal Syllabics";
	if (ch >= 0x1680 && ch <= 0x169F)
		return "Ogham";
	if (ch >= 0x16A0 && ch <= 0x16FF)
		return "Runic";
	if (ch >= 0x1700 && ch <= 0x171F)
		return "Tagalog";
	if (ch >= 0x1720 && ch <= 0x173F)
		return "Hanunoo";
	if (ch >= 0x1740 && ch <= 0x175F)
		return "Buhid";
	if (ch >= 0x1760 && ch <= 0x177F)
		return "Tagbanwa";
	if (ch >= 0x1780 && ch <= 0x17FF)
		return "Khmer";
	if (ch >= 0x1800 && ch <= 0x18AF)
		return "Mongolian";
	if (ch >= 0x1E00 && ch <= 0x1EFF)
		return "Latin Extended Additional";
	if (ch >= 0x1F00 && ch <= 0x1FFF)
		return "Greek Extended";
	if (ch >= 0x2000 && ch <= 0x206F)
		return "General Punctuation";
	if (ch >= 0x2070 && ch <= 0x209F)
		return "Superscripts and Subscripts";
	if (ch >= 0x20A0 && ch <= 0x20CF)
		return "Currency Symbols";
	if (ch >= 0x20D0 && ch <= 0x20FF)
		return "Combining Diacritical Marks for Symbols";
	if (ch >= 0x2100 && ch <= 0x214F)
		return "Letterlike Symbols";
	if (ch >= 0x2150 && ch <= 0x218F)
		return "Number Forms";
	if (ch >= 0x2190 && ch <= 0x21FF)
		return "Arrows";
	if (ch >= 0x2200 && ch <= 0x22FF)
		return "Mathematical Operators";
	if (ch >= 0x2300 && ch <= 0x23FF)
		return "Miscellaneous Technical";
	if (ch >= 0x2400 && ch <= 0x243F)
		return "Control Pictures";
	if (ch >= 0x2440 && ch <= 0x245F)
		return "Optical Character Recognition";
	if (ch >= 0x2460 && ch <= 0x24FF)
		return "Enclosed Alphanumerics";
	if (ch >= 0x2500 && ch <= 0x257F)
		return "Box Drawing";
	if (ch >= 0x2580 && ch <= 0x259F)
		return "Block Elements";
	if (ch >= 0x25A0 && ch <= 0x25FF)
		return "Geometric Shapes";
	if (ch >= 0x2600 && ch <= 0x26FF)
		return "Miscellaneous Symbols";
	if (ch >= 0x2700 && ch <= 0x27BF)
		return "Dingbats";
	if (ch >= 0x27C0 && ch <= 0x27EF)
		return "Miscellaneous Mathematical Symbols-A";
	if (ch >= 0x27F0 && ch <= 0x27FF)
		return "Supplemental Arrows-A";
	if (ch >= 0x2800 && ch <= 0x28FF)
		return "Braille Patterns";
	if (ch >= 0x2900 && ch <= 0x297F)
		return "Supplemental Arrows-B";
	if (ch >= 0x2980 && ch <= 0x29FF)
		return "Miscellaneous Mathematical Symbols-B";
	if (ch >= 0x2A00 && ch <= 0x2AFF)
		return "Supplemental Mathematical Operators";
	if (ch >= 0x2B00 && ch <= 0x2BFF)
		return "Miscellaneous Symbols and Arrows";
	if (ch >= 0x3000 && ch <= 0x303F)
		return "CJK Symbols and Punctuation";
	if (ch >= 0x3040 && ch <= 0x309F)
		return "Hiragana";
	if (ch >= 0x30A0 && ch <= 0x30FF)
		return "Katakana";
	if (ch >= 0x3100 && ch <= 0x312F)
		return "Bopomofo";
	if (ch >= 0x3130 && ch <= 0x318F)
		return "Hangul Compatibility Jamo";
	if (ch >= 0x3190 && ch <= 0x319F)
		return "Kanbun";
	if (ch >= 0x31A0 && ch <= 0x31BF)
		return "Bopomofo Extended";
	if (ch >= 0x31C0 && ch <= 0x31EF)
		return "CJK Strokes";
	if (ch >= 0x31F0 && ch <= 0x31FF)
		return "Katakana Phonetic Extensions";
	if (ch >= 0x3200 && ch <= 0x32FF)
		return "Enclosed CJK Letters and Months";
	if (ch >= 0x3300 && ch <= 0x33FF)
		return "CJK Compatibility";
	if (ch >= 0x3400 && ch <= 0x4DBF)
		return "CJK Unified Ideographs Extension A";
	if (ch >= 0x4DC0 && ch <= 0x4DFF)
		return "Yijing Hexagram Symbols";
	if (ch >= 0x4E00 && ch <= 0x9FFF)
		return "CJK Unified Ideographs";
	if (ch >= 0xA000 && ch <= 0xA48F)
		return "Yi Syllables";
	if (ch >= 0xA490 && ch <= 0xA4CF)
		return "Yi Radicals";
	if (ch >= 0xAC00 && ch <= 0xD7AF)
		return "Hangul Syllables";
	if (ch >= 0xD800 && ch <= 0xDB7F)
		return "High Surrogates";
	if (ch >= 0xDB80 && ch <= 0xDBFF)
		return "High Private Use Surrogates";
	if (ch >= 0xDC00 && ch <= 0xDFFF)
		return "Low Surrogates";
	if (ch >= 0xE000 && ch <= 0xF8FF)
		return "Private Use Area";
	if (ch >= 0xF900 && ch <= 0xFAFF)
		return "CJK Compatibility Ideographs";
	if (ch >= 0xFB00 && ch <= 0xFB4F)
		return "Alphabetic Presentation Forms";
	if (ch >= 0xFB50 && ch <= 0xFDFF)
		return "Arabic Presentation Forms-A";
	if (ch >= 0xFE00 && ch <= 0xFE0F)
		return "Variation Selectors";
	if (ch >= 0xFE10 && ch <= 0xFE1F)
		return "Vertical Forms";
	if (ch >= 0xFE20 && ch <= 0xFE2F)
		return "Combining Half Marks";
	if (ch >= 0xFE30 && ch <= 0xFE4F)
		return "CJK Compatibility Forms";
	if (ch >= 0xFE50 && ch <= 0xFE6F)
		return "Small Form Variants";
	if (ch >= 0xFE70 && ch <= 0xFEFF)
		return "Arabic Presentation Forms-B";
	if (ch >= 0xFF00 && ch <= 0xFFEF)
		return "Halfwidth and Fullwidth Forms";
	if (ch >= 0xFFF0 && ch <= 0xFFFF)
		return "Specials";
#if WCHAR_MAX > 0xFFFF
	// emoji ranges (TODO: requires surrogate pair support)
	if (ch >= 0x1F600 && ch <= 0x1F64F)
		return "Emoticons";
	if (ch >= 0x1F300 && ch <= 0x1F5FF)
		return "Miscellaneous Symbols and Pictographs";
	if (ch >= 0x1F680 && ch <= 0x1F6FF)
		return "Transport and Map Symbols";
	if (ch >= 0x1F700 && ch <= 0x1F77F)
		return "Alchemical Symbols";
	if (ch >= 0x1F780 && ch <= 0x1F7FF)
		return "Geometric Shapes Extended";
	if (ch >= 0x1F800 && ch <= 0x1F8FF)
		return "Supplemental Arrows-C";
	if (ch >= 0x1F900 && ch <= 0x1F9FF)
		return "Supplemental Symbols and Pictographs";
#endif

	return nullptr;
}
} // namespace FontTypeMap
