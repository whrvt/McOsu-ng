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

// DEFS

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

// ENVIRONMENT

#include "BaseEnvironment.h"

// UTIL

#define SAFE_DELETE(p) { if(p) { delete (p); (p) = NULL; } }

constexpr const auto PI = std::numbers::pi;
constexpr const auto PIOVER180 = (PI/180.0f);

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

// ENGINE INCLUDES

#include "EngineFeatures.h"

#include "FastDelegate.h"
#include "UString.h"
#include "Color.h"
#include "Graphics.h"
#include "Environment.h"
#include "KeyboardEvent.h"

#include "Vectors.h"
#include "Matrices.h"
#include "Rect.h"

// EXTERNAL INCLUDES

#include <glm/trigonometric.hpp> // for glm::radians/glm::degrees

#endif
