//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		a simple drive and file selector
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef VSMUSICBROWSER_H
#define VSMUSICBROWSER_H

#include "WindowUIElement.h"

class McFont;

class CBaseUIScrollView;
class CBaseUIButton;

class VSMusicBrowserButton;

class VSMusicBrowser : public WindowUIElement
{
public:
	typedef fastdelegate::FastDelegate2<const UString&, bool> FileClickedCallback;

public:
	VSMusicBrowser(int x, int y, int xSize, int ySize, McFont *font);
	~VSMusicBrowser() override;

	void draw() override;
	void update() override;

	void fireNextSong(bool previous);

	void onInvalidFile();

	void setFileClickedCallback(const FileClickedCallback &callback) {m_fileClickedCallback = callback;}

	// inspection
	CBASE_UI_TYPE(VSMusicBrowser, VSMUSICBROWSER, WindowUIElement)
protected:
	void onMoved() override;
	void onResized() override;
	void onDisabled() override;
	void onEnabled() override;
	void onFocusStolen() override;

private:
	struct COLUMN
	{
		CBaseUIScrollView *view;
		std::vector<VSMusicBrowserButton*> buttons;

		COLUMN()
		{
			view = NULL;
		}
	};

private:
	void updateFolder(const UString& baseFolder, size_t fromDepth);
	void updateDrives();
	void updatePlayingSelection(bool fromInvalidSelection = false);

	void onButtonClicked(CBaseUIButton *button);

	FileClickedCallback m_fileClickedCallback;

	McFont *m_font;

	Color m_defaultTextColor;
	Color m_playingTextBrightColor;
	Color m_playingTextDarkColor;

	CBaseUIScrollView *m_mainContainer;
	std::vector<COLUMN> m_columns;

	UString m_activeSong;
	UString m_previousActiveSong;
	std::vector<UString> m_playlist;
};

#endif
