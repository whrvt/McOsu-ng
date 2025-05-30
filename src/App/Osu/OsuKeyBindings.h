//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		key bindings container
//
// $NoKeywords: $osukey
//===============================================================================//

#pragma once
#ifndef OSUKEYBINDINGS_H
#define OSUKEYBINDINGS_H

#include <vector>
class ConVar;

class OsuKeyBindings
{
public:
	OsuKeyBindings();
	~OsuKeyBindings();

	[[nodiscard]] constexpr const std::vector<ConVar *> *get() const { return &ALL; }
	[[nodiscard]] constexpr const std::vector<std::vector<ConVar *>> *getMania() const { return &MANIA; }

private:
	std::vector<ConVar *> ALL;
	std::vector<std::vector<ConVar *>> MANIA;

	static std::vector<ConVar *> createManiaConVarSet(int k);
	static std::vector<std::vector<ConVar *>> createManiaConVarSets();
	static void setDefaultManiaKeys(std::vector<std::vector<ConVar *>> mania);
};

#endif
