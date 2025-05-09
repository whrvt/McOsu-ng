//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL-based dynamic loading of BASS libraries (workaround linking to broken shared libraries)
//
// $NoKeywords: $snd $bass $loader
//===============================================================================//

#pragma once
#ifndef BASS_LOADER_H
#define BASS_LOADER_H

#include "EngineFeatures.h"

#if defined(MCENGINE_FEATURE_BASS) && !defined(MCENGINE_PLATFORM_WINDOWS)

#include <SDL3/SDL_loadso.h>

// can't be namespaced
#ifdef MCENGINE_PLATFORM_WINDOWS
#ifdef WINAPI_FAMILY
#include <winapifamily.h>
#endif
#include <wtypes.h>
typedef unsigned __int64 QWORD;
#endif

// shove BASS declarations into their own namespace
namespace Bass_EXTERN
{
#include <bass.h>
#include <bass_fx.h>

#ifdef MCENGINE_FEATURE_BASS_WASAPI
#include <bassmix.h>
#include <basswasapi.h>
#endif
}; // namespace Bass_EXTERN

namespace BassLoader
{
using QWORD = Bass_EXTERN::QWORD;
#ifdef MCENGINE_PLATFORM_WINDOWS
using DWORD = DWORD;
using WORD = WORD;
using BYTE = BYTE;
using BOOL = BOOL;
#else
using DWORD = Bass_EXTERN::DWORD;
using WORD = Bass_EXTERN::WORD;
using BYTE = Bass_EXTERN::BYTE;
using BOOL = Bass_EXTERN::BOOL;
#endif

using HSTREAM = Bass_EXTERN::HSTREAM;
using HCHANNEL = Bass_EXTERN::HCHANNEL;
using HSAMPLE = Bass_EXTERN::HSAMPLE;
using BASS_DEVICEINFO = Bass_EXTERN::BASS_DEVICEINFO;
using BASS_INFO = Bass_EXTERN::BASS_INFO;
using BASS_3DVECTOR = Bass_EXTERN::BASS_3DVECTOR;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
using BASS_WASAPI_INFO = Bass_EXTERN::BASS_WASAPI_INFO;
using BASS_WASAPI_DEVICEINFO = Bass_EXTERN::BASS_WASAPI_DEVICEINFO;
#endif

// bassfx enums
using Bass_EXTERN::BASS_ATTRIB_TEMPO_OPTION_AA_FILTER_LENGTH;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_OPTION_OVERLAP_MS;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_OPTION_PREVENT_CLICK;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_OPTION_SEEKWINDOW_MS;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_OPTION_SEQUENCE_MS;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_OPTION_USE_AA_FILTER;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_OPTION_USE_QUICKALGO;

using Bass_EXTERN::BASS_ATTRIB_TEMPO;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_FREQ;
using Bass_EXTERN::BASS_ATTRIB_TEMPO_PITCH;

// open the libraries and populate the function pointers
bool init();
// close the libraries (BassSoundEngine destructor)
void cleanup();

// BASS
extern DWORD (*BASS_GetVersion)();
extern BOOL (*BASS_SetConfig)(DWORD option, DWORD value);
extern DWORD (*BASS_GetConfig)(DWORD option);
extern BOOL (*BASS_Init)(int device, DWORD freq, DWORD flags, void *win, void *dsguid);
extern BOOL (*BASS_Free)();
extern BOOL (*BASS_GetDeviceInfo)(DWORD device, BASS_DEVICEINFO *info);
extern DWORD (*BASS_ErrorGetCode)();
extern HSTREAM (*BASS_StreamCreateFile)(BOOL mem, const void *file, DWORD offset, DWORD length, DWORD flags);
extern HSAMPLE (*BASS_SampleLoad)(BOOL mem, const void *file, DWORD offset, DWORD length, DWORD max, DWORD flags);
extern BOOL (*BASS_SampleFree)(HSAMPLE handle);
extern HCHANNEL (*BASS_SampleGetChannel)(HSAMPLE handle, BOOL onlynew);
extern BOOL (*BASS_ChannelPlay)(DWORD handle, BOOL restart);
extern BOOL (*BASS_ChannelPause)(DWORD handle);
extern BOOL (*BASS_ChannelStop)(DWORD handle);
extern BOOL (*BASS_ChannelSetAttribute)(DWORD handle, DWORD attrib, float value);
extern BOOL (*BASS_ChannelGetAttribute)(DWORD handle, DWORD attrib, float *value);
extern BOOL (*BASS_ChannelSetPosition)(DWORD handle, DWORD pos, DWORD mode);
extern DWORD (*BASS_ChannelGetPosition)(DWORD handle, DWORD mode);
extern DWORD (*BASS_ChannelGetLength)(DWORD handle, DWORD mode);
extern DWORD (*BASS_ChannelFlags)(DWORD handle, DWORD flags, DWORD mask);
extern DWORD (*BASS_ChannelIsActive)(DWORD handle);
extern double (*BASS_ChannelBytes2Seconds)(DWORD handle, DWORD pos);
extern DWORD (*BASS_ChannelSeconds2Bytes)(DWORD handle, double pos);
extern BOOL (*BASS_ChannelSet3DPosition)(DWORD handle, const BASS_3DVECTOR *pos, const BASS_3DVECTOR *orient, const BASS_3DVECTOR *vel);
extern BOOL (*BASS_Set3DPosition)(const BASS_3DVECTOR *pos, const BASS_3DVECTOR *vel, const BASS_3DVECTOR *front, const BASS_3DVECTOR *top);
extern BOOL (*BASS_Apply3D)();
extern BOOL (*BASS_StreamFree)(HSTREAM handle);

// BASS_FX
extern HSTREAM (*BASS_FX_TempoCreate)(DWORD chan, DWORD flags);

#ifdef MCENGINE_FEATURE_BASS_WASAPI
// BASSWASAPI
extern BOOL (*BASS_WASAPI_Init)(int device, DWORD freq, DWORD chans, DWORD flags, float buffer, float period, void *proc, void *user);
extern BOOL (*BASS_WASAPI_Free)();
extern BOOL (*BASS_WASAPI_Start)();
extern BOOL (*BASS_WASAPI_Stop)(BOOL reset);
extern BOOL (*BASS_WASAPI_SetVolume)(DWORD mode, float volume);
extern BOOL (*BASS_WASAPI_GetInfo)(BASS_WASAPI_INFO *info);
extern BOOL (*BASS_WASAPI_GetDeviceInfo)(DWORD device, BASS_WASAPI_DEVICEINFO *info);

// BASSMIX
extern HSTREAM (*BASS_Mixer_StreamCreate)(DWORD freq, DWORD chans, DWORD flags);
extern BOOL (*BASS_Mixer_StreamAddChannel)(HSTREAM handle, DWORD channel, DWORD flags);
extern DWORD (*BASS_Mixer_ChannelGetMixer)(DWORD handle);
extern BOOL (*BASS_Mixer_ChannelRemove)(DWORD handle);
#endif
}; // namespace BassLoader

using namespace BassLoader;

#else

#include <bass.h>
#include <bass_fx.h>

#ifdef MCENGINE_FEATURE_BASS_WASAPI
#include <bassmix.h>
#include <basswasapi.h>
#endif

namespace BassLoader { // windows 32 broken
	bool init();
	void cleanup();
};
#endif

#endif // BASS_LOADER_H
