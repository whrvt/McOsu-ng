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

McMath::McMath()
{
	if (m_expDoubleTable || m_expFloatTable)
		return; // already init

	// double precision table
	m_expDoubleTable = std::make_unique<double[]>(ExpTableTraits<double>::TABLE_SIZE);
	for (int i = 0; i < ExpTableTraits<double>::TABLE_SIZE; ++i)
	{
		double x = ExpTableTraits<double>::TABLE_MIN + static_cast<double>(i) * ExpTableTraits<double>::TABLE_STEP;
		m_expDoubleTable[i] = std::exp(x);
	}

	// single precision table
	m_expFloatTable = std::make_unique<float[]>(ExpTableTraits<float>::TABLE_SIZE);
	for (int i = 0; i < ExpTableTraits<float>::TABLE_SIZE; ++i)
	{
		float x = ExpTableTraits<float>::TABLE_MIN + static_cast<float>(i) * ExpTableTraits<float>::TABLE_STEP;
		m_expFloatTable[i] = std::exp(x);
	}
}
