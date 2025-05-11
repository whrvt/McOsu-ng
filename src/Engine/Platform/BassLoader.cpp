//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL-based dynamic loading of BASS libraries (workaround linking to broken shared libraries)
//
// $NoKeywords: $snd $bass $loader
//===============================================================================//

#include "BassLoader.h"

#if defined(MCENGINE_FEATURE_BASS)

#include "Engine.h"

namespace BassLoader
{
static SDL_SharedObject *s_bassLib = nullptr;
static SDL_SharedObject *s_bassFxLib = nullptr;
#ifdef MCENGINE_FEATURE_BASS_WASAPI
static SDL_SharedObject *s_bassWasapiLib = nullptr;
static SDL_SharedObject *s_bassMixLib = nullptr;
#endif

#if MCENGINE_PLATFORM_WINDOWS
constexpr auto BASS_LIB_NAME = "bass.dll";
constexpr auto BASS_FX_LIB_NAME = "bass_fx.dll";
#ifdef MCENGINE_FEATURE_BASS_WASAPI
constexpr auto BASS_WASAPI_LIB_NAME = "basswasapi.dll";
constexpr auto BASS_MIX_LIB_NAME = "bassmix.dll";
#endif
#elif MCENGINE_PLATFORM_LINUX
constexpr auto BASS_LIB_NAME = "libbass.so";
constexpr auto BASS_FX_LIB_NAME = "libbass_fx.so";
#elif defined(__APPLE__)
constexpr auto BASS_LIB_NAME = "libbass.dylib";
constexpr auto BASS_FX_LIB_NAME = "libbass_fx.dylib";
#endif

// BASS
BASS_GetVersion_t BASS_GetVersion = nullptr;
BASS_SetConfig_t BASS_SetConfig = nullptr;
BASS_GetConfig_t BASS_GetConfig = nullptr;
BASS_Init_t BASS_Init = nullptr;
BASS_Free_t BASS_Free = nullptr;
BASS_GetDeviceInfo_t BASS_GetDeviceInfo = nullptr;
BASS_ErrorGetCode_t BASS_ErrorGetCode = nullptr;
BASS_StreamCreateFile_t BASS_StreamCreateFile = nullptr;
BASS_SampleLoad_t BASS_SampleLoad = nullptr;
BASS_SampleFree_t BASS_SampleFree = nullptr;
BASS_SampleGetChannel_t BASS_SampleGetChannel = nullptr;
BASS_ChannelPlay_t BASS_ChannelPlay = nullptr;
BASS_ChannelPause_t BASS_ChannelPause = nullptr;
BASS_ChannelStop_t BASS_ChannelStop = nullptr;
BASS_ChannelSetAttribute_t BASS_ChannelSetAttribute = nullptr;
BASS_ChannelGetAttribute_t BASS_ChannelGetAttribute = nullptr;
BASS_ChannelSetPosition_t BASS_ChannelSetPosition = nullptr;
BASS_ChannelGetPosition_t BASS_ChannelGetPosition = nullptr;
BASS_ChannelGetLength_t BASS_ChannelGetLength = nullptr;
BASS_ChannelFlags_t BASS_ChannelFlags = nullptr;
BASS_ChannelIsActive_t BASS_ChannelIsActive = nullptr;
BASS_ChannelBytes2Seconds_t BASS_ChannelBytes2Seconds = nullptr;
BASS_ChannelSeconds2Bytes_t BASS_ChannelSeconds2Bytes = nullptr;
BASS_ChannelSet3DPosition_t BASS_ChannelSet3DPosition = nullptr;
BASS_Set3DPosition_t BASS_Set3DPosition = nullptr;
BASS_Apply3D_t BASS_Apply3D = nullptr;
BASS_StreamFree_t BASS_StreamFree = nullptr;

// BASS_FX
BASS_FX_TempoCreate_t BASS_FX_TempoCreate = nullptr;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
// BASSWASAPI
BASS_WASAPI_Init_t BASS_WASAPI_Init = nullptr;
BASS_WASAPI_Free_t BASS_WASAPI_Free = nullptr;
BASS_WASAPI_Start_t BASS_WASAPI_Start = nullptr;
BASS_WASAPI_Stop_t BASS_WASAPI_Stop = nullptr;
BASS_WASAPI_SetVolume_t BASS_WASAPI_SetVolume = nullptr;
BASS_WASAPI_GetInfo_t BASS_WASAPI_GetInfo = nullptr;
BASS_WASAPI_GetDeviceInfo_t BASS_WASAPI_GetDeviceInfo = nullptr;

// BASSMIX
BASS_Mixer_StreamCreate_t BASS_Mixer_StreamCreate = nullptr;
BASS_Mixer_StreamAddChannel_t BASS_Mixer_StreamAddChannel = nullptr;
BASS_Mixer_ChannelGetMixer_t BASS_Mixer_ChannelGetMixer = nullptr;
BASS_Mixer_ChannelRemove_t BASS_Mixer_ChannelRemove = nullptr;
#endif

template <typename T> T loadFunction(SDL_SharedObject *lib, const char *funcName)
{
	T func = reinterpret_cast<T>(SDL_LoadFunction(lib, funcName));
	if (!func)
		debugLog("BassLoader: Failed to load function %s: %s\n", funcName, SDL_GetError());
	return func;
}

bool init()
{
	// just make sure we don't load more than once
	cleanup();

	// BASS
	if (!(s_bassLib = SDL_LoadObject(BASS_LIB_NAME)) && !(s_bassLib = SDL_LoadObject(std::format("lib/{}", BASS_LIB_NAME).c_str())))
	{
		debugLog("BassLoader: Failed to load BASS library: %s\n", SDL_GetError());
		return false;
	}

	BASS_GetVersion = loadFunction<BASS_GetVersion_t>(s_bassLib, "BASS_GetVersion");
	BASS_SetConfig = loadFunction<BASS_SetConfig_t>(s_bassLib, "BASS_SetConfig");
	BASS_GetConfig = loadFunction<BASS_GetConfig_t>(s_bassLib, "BASS_GetConfig");
	BASS_Init = loadFunction<BASS_Init_t>(s_bassLib, "BASS_Init");
	BASS_Free = loadFunction<BASS_Free_t>(s_bassLib, "BASS_Free");
	BASS_GetDeviceInfo = loadFunction<BASS_GetDeviceInfo_t>(s_bassLib, "BASS_GetDeviceInfo");
	BASS_ErrorGetCode = loadFunction<BASS_ErrorGetCode_t>(s_bassLib, "BASS_ErrorGetCode");
	BASS_StreamCreateFile = loadFunction<BASS_StreamCreateFile_t>(s_bassLib, "BASS_StreamCreateFile");
	BASS_SampleLoad = loadFunction<BASS_SampleLoad_t>(s_bassLib, "BASS_SampleLoad");
	BASS_SampleFree = loadFunction<BASS_SampleFree_t>(s_bassLib, "BASS_SampleFree");
	BASS_SampleGetChannel = loadFunction<BASS_SampleGetChannel_t>(s_bassLib, "BASS_SampleGetChannel");
	BASS_ChannelPlay = loadFunction<BASS_ChannelPlay_t>(s_bassLib, "BASS_ChannelPlay");
	BASS_ChannelPause = loadFunction<BASS_ChannelPause_t>(s_bassLib, "BASS_ChannelPause");
	BASS_ChannelStop = loadFunction<BASS_ChannelStop_t>(s_bassLib, "BASS_ChannelStop");
	BASS_ChannelSetAttribute = loadFunction<BASS_ChannelSetAttribute_t>(s_bassLib, "BASS_ChannelSetAttribute");
	BASS_ChannelGetAttribute = loadFunction<BASS_ChannelGetAttribute_t>(s_bassLib, "BASS_ChannelGetAttribute");
	BASS_ChannelSetPosition = loadFunction<BASS_ChannelSetPosition_t>(s_bassLib, "BASS_ChannelSetPosition");
	BASS_ChannelGetPosition = loadFunction<BASS_ChannelGetPosition_t>(s_bassLib, "BASS_ChannelGetPosition");
	BASS_ChannelGetLength = loadFunction<BASS_ChannelGetLength_t>(s_bassLib, "BASS_ChannelGetLength");
	BASS_ChannelFlags = loadFunction<BASS_ChannelFlags_t>(s_bassLib, "BASS_ChannelFlags");
	BASS_ChannelIsActive = loadFunction<BASS_ChannelIsActive_t>(s_bassLib, "BASS_ChannelIsActive");
	BASS_ChannelBytes2Seconds = loadFunction<BASS_ChannelBytes2Seconds_t>(s_bassLib, "BASS_ChannelBytes2Seconds");
	BASS_ChannelSeconds2Bytes = loadFunction<BASS_ChannelSeconds2Bytes_t>(s_bassLib, "BASS_ChannelSeconds2Bytes");
	BASS_ChannelSet3DPosition = loadFunction<BASS_ChannelSet3DPosition_t>(s_bassLib, "BASS_ChannelSet3DPosition");
	BASS_Set3DPosition = loadFunction<BASS_Set3DPosition_t>(s_bassLib, "BASS_Set3DPosition");
	BASS_Apply3D = loadFunction<BASS_Apply3D_t>(s_bassLib, "BASS_Apply3D");
	BASS_StreamFree = loadFunction<BASS_StreamFree_t>(s_bassLib, "BASS_StreamFree");

	// quick sanity check
	if (!BASS_GetVersion || !BASS_Init || !BASS_Free || !BASS_ErrorGetCode)
	{
		debugLog("BassLoader: Failed to load essential BASS functions\n");
		cleanup();
		return false;
	}

	// BASS_FX
	if (!(s_bassFxLib = SDL_LoadObject(BASS_FX_LIB_NAME)) && !(s_bassFxLib = SDL_LoadObject(std::format("lib/{}", BASS_FX_LIB_NAME).c_str())))
	{
		debugLog("BassLoader: Failed to load BASS_FX library: %s\n", SDL_GetError());
		cleanup();
		return false;
	}

	BASS_FX_TempoCreate = loadFunction<BASS_FX_TempoCreate_t>(s_bassFxLib, "BASS_FX_TempoCreate");

	// quick sanity check
	if (!BASS_FX_TempoCreate)
	{
		debugLog("BassLoader: Failed to load essential BASS_FX functions\n");
		cleanup();
		return false;
	}

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	// BASSWASAPI
	if (!(s_bassWasapiLib = SDL_LoadObject(BASS_WASAPI_LIB_NAME)) && !(s_bassWasapiLib = SDL_LoadObject(std::format("lib/{}", BASS_WASAPI_LIB_NAME).c_str())))
	{
		debugLog("BassLoader: Failed to load BASSWASAPI library: %s\n", SDL_GetError());
		// TODO: graceful failure here?
		cleanup();
		return false;
	}

	BASS_WASAPI_Init = loadFunction<BASS_WASAPI_Init_t>(s_bassWasapiLib, "BASS_WASAPI_Init");
	BASS_WASAPI_Free = loadFunction<BASS_WASAPI_Free_t>(s_bassWasapiLib, "BASS_WASAPI_Free");
	BASS_WASAPI_Start = loadFunction<BASS_WASAPI_Start_t>(s_bassWasapiLib, "BASS_WASAPI_Start");
	BASS_WASAPI_Stop = loadFunction<BASS_WASAPI_Stop_t>(s_bassWasapiLib, "BASS_WASAPI_Stop");
	BASS_WASAPI_SetVolume = loadFunction<BASS_WASAPI_SetVolume_t>(s_bassWasapiLib, "BASS_WASAPI_SetVolume");
	BASS_WASAPI_GetInfo = loadFunction<BASS_WASAPI_GetInfo_t>(s_bassWasapiLib, "BASS_WASAPI_GetInfo");
	BASS_WASAPI_GetDeviceInfo = loadFunction<BASS_WASAPI_GetDeviceInfo_t>(s_bassWasapiLib, "BASS_WASAPI_GetDeviceInfo");

	// BASSMIX
	if (!(s_bassMixLib = SDL_LoadObject(BASS_MIX_LIB_NAME)) && !(s_bassMixLib = SDL_LoadObject(std::format("lib/{}", BASS_MIX_LIB_NAME).c_str())))
	{
		debugLog("BassLoader: Failed to load BASSMIX library: %s\n", SDL_GetError());
		// TODO: graceful failure here?
		cleanup();
		return false;
	}

	BASS_Mixer_StreamCreate = loadFunction<BASS_Mixer_StreamCreate_t>(s_bassMixLib, "BASS_Mixer_StreamCreate");
	BASS_Mixer_StreamAddChannel = loadFunction<BASS_Mixer_StreamAddChannel_t>(s_bassMixLib, "BASS_Mixer_StreamAddChannel");
	BASS_Mixer_ChannelGetMixer = loadFunction<BASS_Mixer_ChannelGetMixer_t>(s_bassMixLib, "BASS_Mixer_ChannelGetMixer");
	BASS_Mixer_ChannelRemove = loadFunction<BASS_Mixer_ChannelRemove_t>(s_bassMixLib, "BASS_Mixer_ChannelRemove");
#endif

	// debugLog("BassLoader: Successfully loaded all libraries\n");
	return true;
}

void cleanup()
{
	// unload in reverse order
#ifdef MCENGINE_FEATURE_BASS_WASAPI
	if (s_bassMixLib)
	{
		SDL_UnloadObject(s_bassMixLib);
		s_bassMixLib = nullptr;
	}

	if (s_bassWasapiLib)
	{
		SDL_UnloadObject(s_bassWasapiLib);
		s_bassWasapiLib = nullptr;
	}
#endif

	if (s_bassFxLib)
	{
		SDL_UnloadObject(s_bassFxLib);
		s_bassFxLib = nullptr;
	}

	if (s_bassLib)
	{
		SDL_UnloadObject(s_bassLib);
		s_bassLib = nullptr;
	}

	// reset to null
	BASS_GetVersion = nullptr;
	BASS_SetConfig = nullptr;
	BASS_GetConfig = nullptr;
	BASS_Init = nullptr;
	BASS_Free = nullptr;
	BASS_GetDeviceInfo = nullptr;
	BASS_ErrorGetCode = nullptr;
	BASS_StreamCreateFile = nullptr;
	BASS_SampleLoad = nullptr;
	BASS_SampleFree = nullptr;
	BASS_SampleGetChannel = nullptr;
	BASS_ChannelPlay = nullptr;
	BASS_ChannelPause = nullptr;
	BASS_ChannelStop = nullptr;
	BASS_ChannelSetAttribute = nullptr;
	BASS_ChannelGetAttribute = nullptr;
	BASS_ChannelSetPosition = nullptr;
	BASS_ChannelGetPosition = nullptr;
	BASS_ChannelGetLength = nullptr;
	BASS_ChannelFlags = nullptr;
	BASS_ChannelIsActive = nullptr;
	BASS_ChannelBytes2Seconds = nullptr;
	BASS_ChannelSeconds2Bytes = nullptr;
	BASS_ChannelSet3DPosition = nullptr;
	BASS_Set3DPosition = nullptr;
	BASS_Apply3D = nullptr;
	BASS_StreamFree = nullptr;
	BASS_FX_TempoCreate = nullptr;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	BASS_WASAPI_Init = nullptr;
	BASS_WASAPI_Free = nullptr;
	BASS_WASAPI_Start = nullptr;
	BASS_WASAPI_Stop = nullptr;
	BASS_WASAPI_SetVolume = nullptr;
	BASS_WASAPI_GetInfo = nullptr;
	BASS_WASAPI_GetDeviceInfo = nullptr;
	BASS_Mixer_StreamCreate = nullptr;
	BASS_Mixer_StreamAddChannel = nullptr;
	BASS_Mixer_ChannelGetMixer = nullptr;
	BASS_Mixer_ChannelRemove = nullptr;
#endif
}
} // namespace BassLoader

#endif
