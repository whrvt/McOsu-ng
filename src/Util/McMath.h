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
	template <typename T>
	struct ExpTableTraits;

	template <typename T>
	    requires std::floating_point<T>
	static forceinline T fastExp(T x)
	{
		using Traits = ExpTableTraits<T>;

		// out-of-range values
		if (x <= Traits::TABLE_MIN) return T(0);
		if (x >= Traits::TABLE_MAX) return Traits::getTable()[Traits::TABLE_SIZE - 1];

		// convert x to table index
		T tablePos = (x - Traits::TABLE_MIN) * Traits::TABLE_INV_STEP;
		int index = static_cast<int>(tablePos);

		// don't go out of bounds
		if (index >= Traits::TABLE_SIZE - 1) return Traits::getTable()[Traits::TABLE_SIZE - 1];

		// lerp between table entries
		T fraction = tablePos - static_cast<T>(index);
		return Traits::getTable()[index] * (T(1) - fraction) + Traits::getTable()[index + 1] * fraction;
	}

	// specialized sigmoid for the hot loop: 1.1 / (1.0 + exp(-10.0 * x))
	template <typename T>
	    requires std::floating_point<T>
	static forceinline T fastSigmoid(T x)
	{
		using Traits = SigmoidTableTraits<T>;

		// clamp to table range
		if (x <= Traits::TABLE_MIN) return Traits::getTable()[0];
		if (x >= Traits::TABLE_MAX) return Traits::getTable()[Traits::TABLE_SIZE - 1];

		// convert x to table index
		T tablePos = (x - Traits::TABLE_MIN) * Traits::TABLE_INV_STEP;
		int index = static_cast<int>(tablePos);

		// don't go out of bounds
		if (index >= Traits::TABLE_SIZE - 1) return Traits::getTable()[Traits::TABLE_SIZE - 1];

		// lerp between table entries
		T fraction = tablePos - static_cast<T>(index);
		return Traits::getTable()[index] * (T(1) - fraction) + Traits::getTable()[index + 1] * fraction;
	}

	// helper template struct for sigmoid-specific settings
	template <typename T>
	struct SigmoidTableTraits;

private:
	// lookup tables
	static std::unique_ptr<double[]> m_expDoubleTable;
	static std::unique_ptr<float[]> m_expFloatTable;
	static std::unique_ptr<double[]> m_sigmoidDoubleTable;
	static std::unique_ptr<float[]> m_sigmoidFloatTable;
};

// exp tables, should fit in L1 cache
template <>
struct McMath::ExpTableTraits<double>
{
	static constexpr int TABLE_SIZE = 2048;
	static constexpr double TABLE_MIN = -20.0;
	static constexpr double TABLE_MAX = 20.0;
	static constexpr double TABLE_STEP = (TABLE_MAX - TABLE_MIN) / (TABLE_SIZE - 1);
	static constexpr double TABLE_INV_STEP = 1.0 / TABLE_STEP;

	static forceinline constexpr double *getTable() { return m_expDoubleTable.get(); }
};

template <>
struct McMath::ExpTableTraits<float>
{
	static constexpr int TABLE_SIZE = 1024;
	static constexpr float TABLE_MIN = -20.0f;
	static constexpr float TABLE_MAX = 20.0f;
	static constexpr float TABLE_STEP = (TABLE_MAX - TABLE_MIN) / (TABLE_SIZE - 1);
	static constexpr float TABLE_INV_STEP = 1.0f / TABLE_STEP;

	static forceinline constexpr float *getTable() { return m_expFloatTable.get(); }
};

// specialized sigmoid tables: 1.1 / (1.0 + exp(-10.0 * x))
// for diffcalc hotloop
template <>
struct McMath::SigmoidTableTraits<double>
{
	static constexpr int TABLE_SIZE = 1024;
	static constexpr double TABLE_MIN = -1.5; // covers useful range for (strain/consistentTopStrain - 0.88)
	static constexpr double TABLE_MAX = 1.5;
	static constexpr double TABLE_STEP = (TABLE_MAX - TABLE_MIN) / (TABLE_SIZE - 1);
	static constexpr double TABLE_INV_STEP = 1.0 / TABLE_STEP;

	static forceinline constexpr double *getTable() { return m_sigmoidDoubleTable.get(); }
};

template <>
struct McMath::SigmoidTableTraits<float>
{
	static constexpr int TABLE_SIZE = 512;
	static constexpr float TABLE_MIN = -1.5f;
	static constexpr float TABLE_MAX = 1.5f;
	static constexpr float TABLE_STEP = (TABLE_MAX - TABLE_MIN) / (TABLE_SIZE - 1);
	static constexpr float TABLE_INV_STEP = 1.0f / TABLE_STEP;

	static forceinline constexpr float *getTable() { return m_sigmoidFloatTable.get(); }
};

#endif
