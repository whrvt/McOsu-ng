//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		fast approximations of expensive math operations using lookup tables
//
// $NoKeywords: $math $mcmath
//===============================================================================//

#pragma once
#ifndef MCMATH_H
#define MCMATH_H

#include "cbase.h"
#include <array>
#include <cmath>
#include <concepts>
#include <memory>
#include <type_traits>

class McMath
{
public:
	McMath(); // constructor must be called before using math helpers (set up static tables)

	// helper template struct for type-specific settings
	template <typename T> struct ExpTableTraits;

	template <typename T>
	    requires std::floating_point<T>
	static forceinline T fastExp(T x)
	{
		using Traits = ExpTableTraits<T>;

		// out-of-range values
		if (x <= Traits::TABLE_MIN)
			return T(0);
		if (x >= Traits::TABLE_MAX)
			return Traits::getTable()[Traits::TABLE_SIZE - 1];

		// convert x to table index
		T tablePos = (x - Traits::TABLE_MIN) / Traits::TABLE_STEP;
		int index = static_cast<int>(tablePos);

		// don't go out of bounds
		if (index >= Traits::TABLE_SIZE - 1)
			return Traits::getTable()[Traits::TABLE_SIZE - 1];

		// lerp between table entries
		T fraction = tablePos - static_cast<T>(index);
		return Traits::getTable()[index] * (T(1) - fraction) + Traits::getTable()[index + 1] * fraction;
	}

private:
	// lookup tables
	static std::unique_ptr<double[]> m_expDoubleTable;
	static std::unique_ptr<float[]> m_expFloatTable;
};

// doubles traits specialization
template <> struct McMath::ExpTableTraits<double>
{
	static constexpr int TABLE_SIZE = 10000;
	static constexpr double TABLE_MIN = -20.0;
	static constexpr double TABLE_MAX = 20.0;
	static constexpr double TABLE_STEP = (TABLE_MAX - TABLE_MIN) / (TABLE_SIZE - 1);

	static forceinline constexpr double *getTable() { return m_expDoubleTable.get(); }
};

// floats traits specialization
template <> struct McMath::ExpTableTraits<float>
{
	static constexpr int TABLE_SIZE = 5000;
	static constexpr float TABLE_MIN = -20.0f;
	static constexpr float TABLE_MAX = 20.0f;
	static constexpr float TABLE_STEP = (TABLE_MAX - TABLE_MIN) / (TABLE_SIZE - 1);

	static forceinline constexpr float *getTable() { return m_expFloatTable.get(); }
};

#endif
