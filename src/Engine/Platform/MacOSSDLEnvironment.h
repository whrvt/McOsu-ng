//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		macOS SDL environment
//
// $NoKeywords: $macsdlenv
//===============================================================================//


#pragma once
#ifndef MACOSSDLENVIRONMENT_H
#define MACOSSDLENVIRONMENT_H

#include "SDLEnvironment.h"

#ifdef __APPLE__

#ifdef MCENGINE_FEATURE_SDL

class MacOSEnvironment;

class MacOSSDLEnvironment : public SDLEnvironment
{
public:
	MacOSSDLEnvironment();
	virtual ~MacOSSDLEnvironment();

	// system
	void sleep(unsigned int us);
	void openURLInDefaultBrowser(UString url);

	// user
	UString getUsername();
	UString getUserDataPath();

	// file IO
	bool directoryExists(UString directoryName);
	bool createDirectory(UString directoryName);
	std::vector<UString> getFilesInFolder(UString folder);
	std::vector<UString> getFoldersInFolder(UString folder);
	std::vector<UString> getLogicalDrives();
	UString getFolderFromFilePath(UString filepath);

	// dialogs & message boxes
	UString openFileWindow(const char *filetypefilters, UString title, UString initialpath);
	UString openFolderWindow(UString title, UString initialpath);

	// window
	int getDPI();

private:
	MacOSEnvironment *m_environment;
};

#endif

#else
class MacOSSDLEnvironment : public SDLEnvironment{};
#endif

#endif
