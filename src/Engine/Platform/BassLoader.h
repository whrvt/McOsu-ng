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

#if defined(MCENGINE_FEATURE_BASS)

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
extern "C"
{
#define NOBASSOVERLOADS
#include <bass.h>
#include <bass_fx.h>

#ifndef BASS_CONFIG_MP3_OLDGAPS
#define BASS_CONFIG_MP3_OLDGAPS 68
#define BASS_CONFIG_DEV_TIMEOUT 70 // https://github.com/ppy/osu-framework/blob/eed788fd166540f7e219e1e48a36d0bf64f07cc4/osu.Framework/Audio/AudioManager.cs#L419
#endif

#ifdef MCENGINE_FEATURE_BASS_WASAPI
#include <bassmix.h>
#include <basswasapi.h>
#endif

#ifdef MCENGINE_PLATFORM_WINDOWS
#define BASSVERSION_REAL 0x2041129
#define BASSFXVERSION_REAL 0x2040c0e
#elif defined(MCENGINE_PLATFORM_LINUX)
#define BASSVERSION_REAL 0x2041118 // FIXME: how tf am i supposed to get this ahead of time?
#define BASSFXVERSION_REAL 0x2040c0f
#else
#error "bass unsupported for MacOS currently"
#endif

}
}; // namespace Bass_EXTERN

namespace BassLoader
{
// imported enums/defines
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

// definitions
using BASS_GetVersion_t = decltype(&Bass_EXTERN::BASS_GetVersion);
using BASS_SetConfig_t = decltype(&Bass_EXTERN::BASS_SetConfig);
using BASS_GetConfig_t = decltype(&Bass_EXTERN::BASS_GetConfig);
using BASS_Init_t = decltype(&Bass_EXTERN::BASS_Init);
using BASS_Free_t = decltype(&Bass_EXTERN::BASS_Free);
using BASS_GetDeviceInfo_t = decltype(&Bass_EXTERN::BASS_GetDeviceInfo);
using BASS_ErrorGetCode_t = decltype(&Bass_EXTERN::BASS_ErrorGetCode);
using BASS_StreamCreateFile_t = decltype(&Bass_EXTERN::BASS_StreamCreateFile);
using BASS_SampleLoad_t = decltype(&Bass_EXTERN::BASS_SampleLoad);
using BASS_SampleFree_t = decltype(&Bass_EXTERN::BASS_SampleFree);
using BASS_SampleGetChannel_t = decltype(&Bass_EXTERN::BASS_SampleGetChannel);
using BASS_ChannelPlay_t = decltype(&Bass_EXTERN::BASS_ChannelPlay);
using BASS_ChannelPause_t = decltype(&Bass_EXTERN::BASS_ChannelPause);
using BASS_ChannelStop_t = decltype(&Bass_EXTERN::BASS_ChannelStop);
using BASS_ChannelSetAttribute_t = decltype(&Bass_EXTERN::BASS_ChannelSetAttribute);
using BASS_ChannelGetAttribute_t = decltype(&Bass_EXTERN::BASS_ChannelGetAttribute);
using BASS_ChannelSetPosition_t = decltype(&Bass_EXTERN::BASS_ChannelSetPosition);
using BASS_ChannelGetPosition_t = decltype(&Bass_EXTERN::BASS_ChannelGetPosition);
using BASS_ChannelGetLength_t = decltype(&Bass_EXTERN::BASS_ChannelGetLength);
using BASS_ChannelFlags_t = decltype(&Bass_EXTERN::BASS_ChannelFlags);
using BASS_ChannelIsActive_t = decltype(&Bass_EXTERN::BASS_ChannelIsActive);
using BASS_ChannelBytes2Seconds_t = decltype(&Bass_EXTERN::BASS_ChannelBytes2Seconds);
using BASS_ChannelSeconds2Bytes_t = decltype(&Bass_EXTERN::BASS_ChannelSeconds2Bytes);
using BASS_ChannelSet3DPosition_t = decltype(&Bass_EXTERN::BASS_ChannelSet3DPosition);
using BASS_Set3DPosition_t = decltype(&Bass_EXTERN::BASS_Set3DPosition);
using BASS_Apply3D_t = decltype(&Bass_EXTERN::BASS_Apply3D);
using BASS_StreamFree_t = decltype(&Bass_EXTERN::BASS_StreamFree);

// BASS_FX
using BASS_FX_GetVersion_t = decltype(&Bass_EXTERN::BASS_FX_GetVersion);
using BASS_FX_TempoCreate_t = decltype(&Bass_EXTERN::BASS_FX_TempoCreate);

#ifdef MCENGINE_FEATURE_BASS_WASAPI
// BASSWASAPI
using BASS_WASAPI_Init_t = decltype(&Bass_EXTERN::BASS_WASAPI_Init);
using BASS_WASAPI_Free_t = decltype(&Bass_EXTERN::BASS_WASAPI_Free);
using BASS_WASAPI_Start_t = decltype(&Bass_EXTERN::BASS_WASAPI_Start);
using BASS_WASAPI_Stop_t = decltype(&Bass_EXTERN::BASS_WASAPI_Stop);
using BASS_WASAPI_SetVolume_t = decltype(&Bass_EXTERN::BASS_WASAPI_SetVolume);
using BASS_WASAPI_GetInfo_t = decltype(&Bass_EXTERN::BASS_WASAPI_GetInfo);
using BASS_WASAPI_GetDeviceInfo_t = decltype(&Bass_EXTERN::BASS_WASAPI_GetDeviceInfo);

// BASSMIX
using BASS_Mixer_StreamCreate_t = decltype(&Bass_EXTERN::BASS_Mixer_StreamCreate);
using BASS_Mixer_StreamAddChannel_t = decltype(&Bass_EXTERN::BASS_Mixer_StreamAddChannel);
using BASS_Mixer_ChannelGetMixer_t = decltype(&Bass_EXTERN::BASS_Mixer_ChannelGetMixer);
using BASS_Mixer_ChannelRemove_t = decltype(&Bass_EXTERN::BASS_Mixer_ChannelRemove);
#endif

// declarations
extern BASS_GetVersion_t BASS_GetVersion;
extern BASS_SetConfig_t BASS_SetConfig;
extern BASS_GetConfig_t BASS_GetConfig;
extern BASS_Init_t BASS_Init;
extern BASS_Free_t BASS_Free;
extern BASS_GetDeviceInfo_t BASS_GetDeviceInfo;
extern BASS_ErrorGetCode_t BASS_ErrorGetCode;
extern BASS_StreamCreateFile_t BASS_StreamCreateFile;
extern BASS_SampleLoad_t BASS_SampleLoad;
extern BASS_SampleFree_t BASS_SampleFree;
extern BASS_SampleGetChannel_t BASS_SampleGetChannel;
extern BASS_ChannelPlay_t BASS_ChannelPlay;
extern BASS_ChannelPause_t BASS_ChannelPause;
extern BASS_ChannelStop_t BASS_ChannelStop;
extern BASS_ChannelSetAttribute_t BASS_ChannelSetAttribute;
extern BASS_ChannelGetAttribute_t BASS_ChannelGetAttribute;
extern BASS_ChannelSetPosition_t BASS_ChannelSetPosition;
extern BASS_ChannelGetPosition_t BASS_ChannelGetPosition;
extern BASS_ChannelGetLength_t BASS_ChannelGetLength;
extern BASS_ChannelFlags_t BASS_ChannelFlags;
extern BASS_ChannelIsActive_t BASS_ChannelIsActive;
extern BASS_ChannelBytes2Seconds_t BASS_ChannelBytes2Seconds;
extern BASS_ChannelSeconds2Bytes_t BASS_ChannelSeconds2Bytes;
extern BASS_ChannelSet3DPosition_t BASS_ChannelSet3DPosition;
extern BASS_Set3DPosition_t BASS_Set3DPosition;
extern BASS_Apply3D_t BASS_Apply3D;
extern BASS_StreamFree_t BASS_StreamFree;

// BASS_FX
extern BASS_FX_GetVersion_t BASS_FX_GetVersion;
extern BASS_FX_TempoCreate_t BASS_FX_TempoCreate;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
// BASSWASAPI
extern BASS_WASAPI_Init_t BASS_WASAPI_Init;
extern BASS_WASAPI_Free_t BASS_WASAPI_Free;
extern BASS_WASAPI_Start_t BASS_WASAPI_Start;
extern BASS_WASAPI_Stop_t BASS_WASAPI_Stop;
extern BASS_WASAPI_SetVolume_t BASS_WASAPI_SetVolume;
extern BASS_WASAPI_GetInfo_t BASS_WASAPI_GetInfo;
extern BASS_WASAPI_GetDeviceInfo_t BASS_WASAPI_GetDeviceInfo;

// BASSMIX
extern BASS_Mixer_StreamCreate_t BASS_Mixer_StreamCreate;
extern BASS_Mixer_StreamAddChannel_t BASS_Mixer_StreamAddChannel;
extern BASS_Mixer_ChannelGetMixer_t BASS_Mixer_ChannelGetMixer;
extern BASS_Mixer_ChannelRemove_t BASS_Mixer_ChannelRemove;
#endif

// open the libraries and populate the function pointers
bool init();
// close the libraries (BassSoundEngine destructor)
void cleanup();

}; // namespace BassLoader

using namespace BassLoader;

#endif

#endif // BASS_LOADER_H
