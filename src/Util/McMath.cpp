//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		fast approximations of expensive math operations using lookup tables
//
// $NoKeywords: $math $mcmath
//===============================================================================//

#include "McMath.h"

// static members
std::unique_ptr<double[]> McMath::m_expDoubleTable = nullptr;
std::unique_ptr<float[]> McMath::m_expFloatTable = nullptr;
std::unique_ptr<double[]> McMath::m_sigmoidDoubleTable = nullptr;
std::unique_ptr<float[]> McMath::m_sigmoidFloatTable = nullptr;

McMath::McMath()
{
	if (m_expDoubleTable || m_expFloatTable) return; // already init

	// double precision exp table
	m_expDoubleTable = std::make_unique<double[]>(ExpTableTraits<double>::TABLE_SIZE);
	for (int i = 0; i < ExpTableTraits<double>::TABLE_SIZE; ++i)
	{
		double x = ExpTableTraits<double>::TABLE_MIN + static_cast<double>(i) * ExpTableTraits<double>::TABLE_STEP;
		m_expDoubleTable[i] = std::exp(x);
	}

	// single precision exp table
	m_expFloatTable = std::make_unique<float[]>(ExpTableTraits<float>::TABLE_SIZE);
	for (int i = 0; i < ExpTableTraits<float>::TABLE_SIZE; ++i)
	{
		float x = ExpTableTraits<float>::TABLE_MIN + static_cast<float>(i) * ExpTableTraits<float>::TABLE_STEP;
		m_expFloatTable[i] = std::exp(x);
	}

	// double precision sigmoid table: 1.1 / (1.0 + exp(-10.0 * x))
	m_sigmoidDoubleTable = std::make_unique<double[]>(SigmoidTableTraits<double>::TABLE_SIZE);
	for (int i = 0; i < SigmoidTableTraits<double>::TABLE_SIZE; ++i)
	{
		double x = SigmoidTableTraits<double>::TABLE_MIN + static_cast<double>(i) * SigmoidTableTraits<double>::TABLE_STEP;
		m_sigmoidDoubleTable[i] = 1.1 / (1.0 + std::exp(-10.0 * x));
	}

	// single precision sigmoid table: 1.1 / (1.0 + exp(-10.0 * x))
	m_sigmoidFloatTable = std::make_unique<float[]>(SigmoidTableTraits<float>::TABLE_SIZE);
	for (int i = 0; i < SigmoidTableTraits<float>::TABLE_SIZE; ++i)
	{
		float x = SigmoidTableTraits<float>::TABLE_MIN + static_cast<float>(i) * SigmoidTableTraits<float>::TABLE_STEP;
		m_sigmoidFloatTable[i] = 1.1f / (1.0f + std::exp(-10.0f * x));
	}
}
