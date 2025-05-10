//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		cross-platform main() entrypoint
//
// $NoKeywords: $sdlcallbacks $main
//===============================================================================//

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "SDLMain.h"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");
	if (!SDL_Init(SDL_INIT_VIDEO)) // other subsystems can be init later
	{
		fprintf(stderr, "Couldn't SDL_Init(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto *fmain = new SDLMain(argc, argv);
	*appstate = fmain;
	return fmain->initialize(argc, argv);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	return static_cast<SDLMain *>(appstate)->iterate();
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	return static_cast<SDLMain *>(appstate)->handleEvent(event);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	if (result == SDL_APP_FAILURE)
	{
		fprintf(stderr, "Force exiting now, a fatal error occurred. (SDL error: %s)\n", SDL_GetError());
		std::abort();
	}

	auto *fmain = static_cast<SDLMain *>(appstate);
	fmain->shutdown(result);
	SAFE_DELETE(fmain);
}
