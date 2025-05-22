//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		changelog screen
//
// $NoKeywords: $osulog
//===============================================================================//

#pragma once
#ifndef OSUCHANGELOG_H
#define OSUCHANGELOG_H

#include "OsuScreenBackable.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;
class CBaseUILabel;

class OsuChangelog : public OsuScreenBackable
{
public:
	OsuChangelog();
	~OsuChangelog() override;

	void draw(Graphics *g) override;
	void update() override;

	void setVisible(bool visible) override;

private:
	void updateLayout() override;
	void onBack() override;

	void onChangeClicked(CBaseUIButton *button);

	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_scrollView;

	struct CHANGELOG
	{
		UString title;
		std::vector<UString> changes;
	};

	struct CHANGELOG_UI
	{
		CBaseUILabel *title;
		std::vector<CBaseUIButton*> changes;
	};

	std::vector<CHANGELOG_UI> m_changelogs;
};

#endif
