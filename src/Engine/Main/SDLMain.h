//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		main application class for SDL3 callbacks
//
// $NoKeywords: $sdlmain
//===============================================================================//

#pragma once

#include "SDLEnvironment.h"
#include "Timer.h"
#include <SDL3/SDL.h>

class SDLMain : public SDLEnvironment
{
public:
	SDLMain(int argc, char *argv[]);
	~SDLMain();

	SDL_AppResult initialize(int argc, char *argv[]);
	SDL_AppResult iterate();
	SDL_AppResult handleEvent(SDL_Event *event);
	void shutdown() override { SDLEnvironment::shutdown(); }
	void shutdown(SDL_AppResult result);

private:
	// window and context
	SDL_GLContext m_context;

	// engine update timer
	Timer *m_deltaTimer;

	// init methods
	void setupLogging();
	bool createWindow(int width, int height);
	void setupOpenGL();
	void configureEvents();

	// callback handlers
	void fps_max_callback(UString oldVal, UString newVal);
	void fps_max_background_callback(UString oldVal, UString newVal);
	void fps_unlimited_callback(UString oldVal, UString newVal);

	void parseArgs();
};

extern ConVar fps_max;
extern ConVar fps_max_background;
extern ConVar fps_unlimited;
