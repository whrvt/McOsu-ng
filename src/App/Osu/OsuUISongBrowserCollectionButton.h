//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		collection button (also used for grouping)
//
// $NoKeywords: $osusbcb
//===============================================================================//

#ifndef OSUUISONGBROWSERCOLLECTIONBUTTON_H
#define OSUUISONGBROWSERCOLLECTIONBUTTON_H

#include "OsuUISongBrowserButton.h"

class OsuUISongBrowserCollectionButton : public OsuUISongBrowserButton
{
public:
	OsuUISongBrowserCollectionButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString collectionName, std::vector<OsuUISongBrowserButton*> children);

	virtual void draw(Graphics *g);

private:
	virtual void onSelected(bool wasSelected);

	UString buildTitleString();

	UString m_sCollectionName;

	float m_fTitleScale;
};

#endif
