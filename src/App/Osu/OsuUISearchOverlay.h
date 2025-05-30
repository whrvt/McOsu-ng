//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		search text overlay
//
// $NoKeywords: $osufind
//===============================================================================//

#pragma once
#ifndef OSUUISEARCHOVERLAY_H
#define OSUUISEARCHOVERLAY_H

#include "OsuUIElement.h"

class Osu;

class OsuUISearchOverlay final : public OsuUIElement
{
public:
	OsuUISearchOverlay(float xPos, float yPos, float xSize, float ySize, UString name);

	void draw() override;

	void setDrawNumResults(bool drawNumResults) {m_bDrawNumResults = drawNumResults;}
	void setOffsetRight(int offsetRight) {m_iOffsetRight = offsetRight;}

	void setSearchString(UString searchString, UString hardcodedSearchString = "") {m_sSearchString = searchString; m_sHardcodedSearchString = hardcodedSearchString;}
	void setNumFoundResults(int numFoundResults) {m_iNumFoundResults = numFoundResults;}

	void setSearching(bool searching) {m_bSearching = searching;}

	// inspection
	CBASE_UI_TYPE(OsuUISearchOverlay, SEARCHOVERLAY, OsuUIElement)
private:

	McFont *m_font;

	int m_iOffsetRight;
	bool m_bDrawNumResults;

	UString m_sSearchString;
	UString m_sHardcodedSearchString;
	int m_iNumFoundResults;

	bool m_bSearching;
};

#endif
