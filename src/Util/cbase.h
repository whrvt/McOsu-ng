//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		CBASE - all global stuff goes in here
//
// $NoKeywords: $base
//===============================================================================//

#pragma once
#ifndef CBASE_H
#define CBASE_H

// STD INCLUDES

#include <cmath>
#include <limits>
#include <vector>
#include <stack>
#include <string>
#include <random>
#include <memory>
#include <atomic>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <numbers>

#include <fstream>
#include <iostream>

#if __has_include(<sstream>)
#include <sstream>
#endif

#include <cstdarg>
#include <cstdint>
#include <cstring>

#ifdef __GNUC__
#define likely(x) __builtin_expect(bool(x),1)
#define unlikely(x) __builtin_expect(bool(x),0)
#define forceinline __attribute__((always_inline)) inline
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define forceinline
#endif

// ENGINE INCLUDES

#include "BaseEnvironment.h"

#include "EngineFeatures.h"

#include "FastDelegate.h"

#include "Graphics.h"
#include "Environment.h"
#include "KeyboardEvent.h"

#include "Vectors.h"
#include "Matrices.h"
#include "Rect.h"
#include "UString.h"
#include "McMath.h"



// DEFS

#ifdef NULL 
#undef NULL
#endif
#define NULL nullptr

using COLORPART = unsigned char;

/*
#ifndef DWORD
typedef unsigned long 	DWORD;
#endif
#ifndef WORD
typedef unsigned short	WORD;
#endif
#ifndef BYTE
typedef unsigned char	BYTE;
#endif
#ifndef UINT8
typedef unsigned char 	UINT8;
#endif
*/

#define SAFE_DELETE(p) { if(p) { delete (p); (p) = NULL; } }

#define COLOR(a,r,g,b) \
    ((Color)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))

#define COLORf(a,r,g,b) \
    ((Color)(((((int)( clamp<float>(a,0.0f,1.0f)*255.0f ))&0xff)<<24)|((((int)( clamp<float>(r,0.0f,1.0f)*255.0f ))&0xff)<<16)|((((int)( clamp<float>(g,0.0f,1.0f)*255.0f ))&0xff)<<8)|(((int)( clamp<float>(b,0.0f,1.0f)*255.0f ))&0xff)))

#define COLOR_GET_Ri(color) \
	(((COLORPART)((color) >> 16)))

#define COLOR_GET_Gi(color) \
	(((COLORPART)((color) >> 8)))

#define COLOR_GET_Bi(color) \
	(((COLORPART)((color) >> 0)))

#define COLOR_GET_Ai(color) \
	(((COLORPART)((color) >> 24)))

#define COLOR_GET_Rf(color) \
	(((COLORPART)((color) >> 16))  / 255.0f)

#define COLOR_GET_Gf(color) \
	(((COLORPART)((color) >> 8)) / 255.0f)

#define COLOR_GET_Bf(color) \
	(((COLORPART)((color) >> 0)) / 255.0f)

#define COLOR_GET_Af(color) \
	(((COLORPART)((color) >> 24)) / 255.0f)

#define COLOR_INVERT(color) \
	(COLOR(255, 255-COLOR_GET_Ri(color), 255-COLOR_GET_Gi(color), 255-COLOR_GET_Bi(color)))

#define COLOR_MULTIPLY(color1, color2) \
	(COLORf(1.0f, COLOR_GET_Rf(color1)*COLOR_GET_Rf(color2), COLOR_GET_Gf(color1)*COLOR_GET_Gf(color2), COLOR_GET_Bf(color1)*COLOR_GET_Bf(color2)))

#define COLOR_ADD(color1, color2) \
	(COLORf(1.0f, clamp<float>(COLOR_GET_Rf(color1)+COLOR_GET_Rf(color2),0.0f,1.0f), clamp<float>(COLOR_GET_Gf(color1)+COLOR_GET_Gf(color2),0.0f,1.0f), clamp<float>(COLOR_GET_Bf(color1)+COLOR_GET_Bf(color2),0.0f,1.0f)))

#define COLOR_SUBTRACT(color1, color2) \
	(COLORf(1.0f, clamp<float>(COLOR_GET_Rf(color1)-COLOR_GET_Rf(color2),0.0f,1.0f), clamp<float>(COLOR_GET_Gf(color1)-COLOR_GET_Gf(color2),0.0f,1.0f), clamp<float>(COLOR_GET_Bf(color1)-COLOR_GET_Bf(color2),0.0f,1.0f)))

constexpr const auto PI = std::numbers::pi;
constexpr const auto PIOVER180 = (PI/180.0f);
constexpr const auto ONE80OVERPI = (180.0f/PI);

// UTIL

using std::clamp;
using std::lerp;
using std::isfinite;
using std::signbit;

template <class T>
constexpr forceinline float deg2rad(T deg)
{
	return deg * PIOVER180;
}

template <class T>
constexpr forceinline float rad2deg(T rad)
{
	return rad * ONE80OVERPI;
}

constexpr forceinline bool isInt(float f)
{
    return (f == static_cast<float>(static_cast<int>(f)));
}

template <typename T>
    requires std::floating_point<T>
[[nodiscard]] constexpr inline bool almostEqual(T x, T y, T relativeTolerance = T{100} * std::numeric_limits<T>::epsilon(),
                                                T absoluteTolerance = T{100} * std::numeric_limits<T>::epsilon())
{
	if (x == y) return true;
	// absolute difference for values near zero
	T diff = std::abs(x - y);
	if (x == T{0} || y == T{0} || diff < std::numeric_limits<T>::min())
		return diff < absoluteTolerance;
	// relative difference for other values
	T absX = std::abs(x);
	T absY = std::abs(y);
	// use the smaller value to calculate relative error
	return diff < relativeTolerance * std::min(absX, absY);
}

#endif
