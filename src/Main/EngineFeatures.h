//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		individual engine components to compile with
//
// $NoKeywords: $feat
//===============================================================================//

#pragma once
#ifndef ENGINEFEATURES_H
#define ENGINEFEATURES_H

#include "config.h"

/*
 * OpenGL graphics (Desktop, legacy + modern) (defined in config.h)
 */
//#define MCENGINE_FEATURE_OPENGL

/*
 * OpenGLES 3.2 graphics (Desktop, WebGL) (defined in config.h)
 */
//#define MCENGINE_FEATURE_GLES32

/*
 * OpenGL 3.0 graphics (WIP, not usable)
 */
//#define MCENGINE_FEATURE_GL3

/*
 * DirectX 11 graphics (defined in config.h)
 */
//#define MCENGINE_FEATURE_DIRECTX11

/*
 * ENet & CURL networking
 */
//#define MCENGINE_FEATURE_NETWORKING

/*
 * BASS sound (defined in config.h)
 */
//#define MCENGINE_FEATURE_BASS

/*
 * BASS WASAPI sound (Windows only)
 */
// #if (defined(_WIN32) || defined(_WIN64)) && defined(MCENGINE_FEATURE_BASS)
//  #define MCENGINE_FEATURE_BASS_WASAPI
// #endif

/*
 * SDL3 mixer (audio) (defined in config.h)
 */
//#define MCENGINE_FEATURE_SDL_MIXER

/*
 * Discord RPC (rich presence)
 */
//#define MCENGINE_FEATURE_DISCORD

/*
 * Steam
 */
//#define MCENGINE_FEATURE_STEAMWORKS

/*
 * Performance profiling (ctrl+F11)
 */
#define MCENGINE_FEATURE_PROFILING

/*
 * If, for some reason, you want SDL main callbacks on desktop (it's forced on WASM)
 */
//#define MCENGINE_FEATURE_MAINCALLBACKS

#endif
