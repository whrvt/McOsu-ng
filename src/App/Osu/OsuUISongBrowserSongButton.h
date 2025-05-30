//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap + diff button
//
// $NoKeywords: $osusbsb
//===============================================================================//

#pragma once
#ifndef OSUUISONGBROWSERSONGBUTTON_H
#define OSUUISONGBROWSERSONGBUTTON_H

#include "OsuUISongBrowserButton.h"
#include "OsuScore.h"

class OsuSongBrowser2;
class OsuDatabaseBeatmap;

class OsuUISongBrowserSongButton : public OsuUISongBrowserButton
{
public:
	OsuUISongBrowserSongButton(OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, OsuDatabaseBeatmap *databaseBeatmap);
	~OsuUISongBrowserSongButton() override;

	void draw() override;
	void update() override;

	void triggerContextMenu(Vector2 pos);

	void sortChildren();

	void updateLayoutEx() override;
	virtual void updateGrade() {;}

	[[nodiscard]] OsuDatabaseBeatmap *getDatabaseBeatmap() const override {return m_databaseBeatmap;}

	// inspection
	CBASE_UI_TYPE(OsuUISongBrowserSongButton, OsuUIElement::UISONGBROWSERSONGBUTTON, OsuUISongBrowserButton)

protected:
	void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) override;
	void onRightMouseUpInside() override;

	void onContextMenu(UString text, int id = -1);
	void onAddToCollectionConfirmed(UString text, int id = -1);
	void onCreateNewCollectionConfirmed(UString text, int id = -1);

	void drawBeatmapBackgroundThumbnail(Image *image);
	void drawGrade();
	void drawTitle(float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);
	void drawSubTitle(float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);

	float calculateGradeScale();
	float calculateGradeWidth();

	UString buildTitleString()
	{
		return m_sTitle;
	}

	UString buildSubTitleString()
	{
		UString subTitleString = m_sArtist;
		subTitleString.append(" // ");
		subTitleString.append(m_sMapper);

		return subTitleString;
	}

	OsuDatabaseBeatmap *m_databaseBeatmap;

	UString m_sTitle;
	UString m_sArtist;
	UString m_sMapper;
	OsuScore::GRADE m_grade;
	bool m_bHasGrade;

	float m_fTextOffset;
	float m_fGradeOffset;
	float m_fTextSpacingScale;
	float m_fTextMarginScale;
	float m_fTitleScale;
	float m_fSubTitleScale;
	float m_fGradeScale;

private:
	static float thumbnailYRatio;

	float m_fThumbnailFadeInTime;

	void updateRepresentativeDatabaseBeatmap();

	OsuDatabaseBeatmap *m_representativeDatabaseBeatmap;
};

#endif
