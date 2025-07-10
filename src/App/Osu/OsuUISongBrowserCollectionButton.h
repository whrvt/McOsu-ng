//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		collection button (also used for grouping)
//
// $NoKeywords: $osusbcb
//===============================================================================//

#pragma once
#ifndef OSUUISONGBROWSERCOLLECTIONBUTTON_H
#define OSUUISONGBROWSERCOLLECTIONBUTTON_H

#include "OsuUISongBrowserButton.h"

class OsuUISongBrowserCollectionButton final : public OsuUISongBrowserButton
{
public:
	OsuUISongBrowserCollectionButton(OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString collectionName, std::vector<OsuUISongBrowserButton*> children);

	void draw() override;

	void triggerContextMenu(Vector2 pos);

	[[nodiscard]] Color getActiveBackgroundColor() const override;
	[[nodiscard]] Color getInactiveBackgroundColor() const override;

	[[nodiscard]] const UString &getCollectionName() const {return m_sCollectionName;}

	// inspection
	CBASE_UI_TYPE(OsuUISongBrowserCollectionButton, OsuUIElement::UISONGBROWSERCOLLECTIONBUTTON, OsuUISongBrowserButton)
private:
	void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) override;
	void onRightMouseUpInside() override;

	void onContextMenu(const UString& text, int id = -1);
	void onRenameCollectionConfirmed(const UString& text, int id = -1);
	void onDeleteCollectionConfirmed(const UString& text, int id = -1);

	UString buildTitleString();

	UString m_sCollectionName;

	float m_fTitleScale;
};

#endif
