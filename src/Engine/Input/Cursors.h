//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		cursor defs, these need to be supported by Environment
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef CURSORS_H
#define CURSORS_H
#include <cstdint>

enum class CURSORTYPE : uint8_t
{
	CURSOR_NORMAL,
	CURSOR_WAIT,
	CURSOR_SIZE_H,
	CURSOR_SIZE_V,
	CURSOR_SIZE_HV,
	CURSOR_SIZE_VH,
	CURSOR_TEXT
};

#endif
