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
 * std::thread/std::mutex support
 */
#define MCENGINE_FEATURE_MULTITHREADING

/*
 * OpenGL graphics (Desktop, legacy + modern)
 */
#define MCENGINE_FEATURE_OPENGL

/*
 * OpenGL graphics (Mobile, ES/EGL, Nintendo Switch)
 */
//#define MCENGINE_FEATURE_GLES2

/*
 * OpenGLES 3.2 graphics (Desktop, WebGL)
 */
//#define MCENGINE_FEATURE_GLES32

/*
 * Software-only graphics renderer (Windows only) (untested)
 */
//#define MCENGINE_FEATURE_SOFTRENDERER

/*
 * DirectX 11 graphics
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
//#define MCENGINE_FEATURE_BASS_WASAPI

/*
 * Vulkan
 */
//#define MCENGINE_FEATURE_VULKAN

/*
 * OpenVR
 */
//#define MCENGINE_FEATURE_OPENVR

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

/* misc, untested */
//#define MCENGINE_SDL_JOYSTICK
//#define MCENGINE_SDL_JOYSTICK_MOUSE
//#define MCENGINE_SDL_TOUCHSUPPORT

#endif
