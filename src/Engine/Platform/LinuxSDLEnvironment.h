//================ Copyright (c) 2022, PG, All rights reserved. =================//
//
// Purpose:		linux sdl environment
//
// $NoKeywords: $sdllinuxenv
//===============================================================================//

#pragma once
#ifndef LINUXSDLENVIRONMENT_H
#define LINUXSDLENVIRONMENT_H

#include "SDLEnvironment.h"

#ifdef __linux__

#ifdef MCENGINE_FEATURE_SDL

class LinuxSDLEnvironment : public SDLEnvironment
{
public:
	LinuxSDLEnvironment();
	virtual ~LinuxSDLEnvironment() {;}

	// system
	virtual void sleep(unsigned int us);
	virtual void openURLInDefaultBrowser(UString url);

	// user
	virtual UString getUsername();
	virtual UString getUserDataPath();

	// file IO
	virtual bool directoryExists(UString directoryName);
	virtual bool createDirectory(UString directoryName);
	virtual std::vector<UString> getFilesInFolder(UString folder);
	virtual std::vector<UString> getFoldersInFolder(UString folder);
	virtual std::vector<UString> getLogicalDrives();
	virtual UString getFolderFromFilePath(UString filepath);

private:
	static int getFilesInFolderFilter(const struct dirent *entry);
	static int getFoldersInFolderFilter(const struct dirent *entry);
};

#endif

#else
class LinuxSDLEnvironment : public SDLEnvironment{};
#endif

#endif
