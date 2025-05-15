//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		singleton manager for SoLoud instance and operations
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudManager.h"

#ifdef MCENGINE_FEATURE_SOLOUD
std::unique_ptr<SoLoud::Soloud> SL::m_engine = nullptr;

SL::SL()
{
	if (m_engine)
		return; // already init
	m_engine = std::make_unique<SoLoud::Soloud>();
}

#endif // MCENGINE_FEATURE_SOLOUD
