//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL-based dynamic loading of BASS libraries (workaround linking to broken shared libraries)
//
// $NoKeywords: $snd $bass $loader
//===============================================================================//

#include "BassLoader.h"

#if defined(MCENGINE_FEATURE_BASS) && !defined(MCENGINE_PLATFORM_WINDOWS) // FIXME: broken on 32bit windows

#include "Engine.h"

namespace BassLoader
{
using namespace std::string_view_literals;
static SDL_SharedObject *s_bassLib = nullptr;
static SDL_SharedObject *s_bassFxLib = nullptr;
#ifdef MCENGINE_FEATURE_BASS_WASAPI
static SDL_SharedObject *s_bassWasapiLib = nullptr;
static SDL_SharedObject *s_bassMixLib = nullptr;
#endif

// Library file names for different platforms
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
DWORD (*BASS_GetVersion)() = nullptr;
BOOL (*BASS_SetConfig)(DWORD option, DWORD value) = nullptr;
DWORD (*BASS_GetConfig)(DWORD option) = nullptr;
BOOL (*BASS_Init)(int device, DWORD freq, DWORD flags, void *win, void *dsguid) = nullptr;
BOOL (*BASS_Free)() = nullptr;
BOOL (*BASS_GetDeviceInfo)(DWORD device, BASS_DEVICEINFO *info) = nullptr;
DWORD (*BASS_ErrorGetCode)() = nullptr;
HSTREAM (*BASS_StreamCreateFile)(BOOL mem, const void *file, DWORD offset, DWORD length, DWORD flags) = nullptr;
HSAMPLE (*BASS_SampleLoad)(BOOL mem, const void *file, DWORD offset, DWORD length, DWORD max, DWORD flags) = nullptr;
BOOL (*BASS_SampleFree)(HSAMPLE handle) = nullptr;
HCHANNEL (*BASS_SampleGetChannel)(HSAMPLE handle, BOOL onlynew) = nullptr;
BOOL (*BASS_ChannelPlay)(DWORD handle, BOOL restart) = nullptr;
BOOL (*BASS_ChannelPause)(DWORD handle) = nullptr;
BOOL (*BASS_ChannelStop)(DWORD handle) = nullptr;
BOOL (*BASS_ChannelSetAttribute)(DWORD handle, DWORD attrib, float value) = nullptr;
BOOL (*BASS_ChannelGetAttribute)(DWORD handle, DWORD attrib, float *value) = nullptr;
BOOL (*BASS_ChannelSetPosition)(DWORD handle, DWORD pos, DWORD mode) = nullptr;
DWORD (*BASS_ChannelGetPosition)(DWORD handle, DWORD mode) = nullptr;
DWORD (*BASS_ChannelGetLength)(DWORD handle, DWORD mode) = nullptr;
DWORD (*BASS_ChannelFlags)(DWORD handle, DWORD flags, DWORD mask) = nullptr;
DWORD (*BASS_ChannelIsActive)(DWORD handle) = nullptr;
double (*BASS_ChannelBytes2Seconds)(DWORD handle, DWORD pos) = nullptr;
DWORD (*BASS_ChannelSeconds2Bytes)(DWORD handle, double pos) = nullptr;
BOOL (*BASS_ChannelSet3DPosition)(DWORD handle, const BASS_3DVECTOR *pos, const BASS_3DVECTOR *orient, const BASS_3DVECTOR *vel) = nullptr;
BOOL (*BASS_Set3DPosition)(const BASS_3DVECTOR *pos, const BASS_3DVECTOR *vel, const BASS_3DVECTOR *front, const BASS_3DVECTOR *top) = nullptr;
BOOL (*BASS_Apply3D)() = nullptr;
BOOL (*BASS_StreamFree)(HSTREAM handle) = nullptr;

// BASS_FX
HSTREAM (*BASS_FX_TempoCreate)(DWORD chan, DWORD flags) = nullptr;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
// BASSWASAPI
BOOL (*BASS_WASAPI_Init)(int device, DWORD freq, DWORD chans, DWORD flags, float buffer, float period, void *proc, void *user) = nullptr;
BOOL (*BASS_WASAPI_Free)() = nullptr;
BOOL (*BASS_WASAPI_Start)() = nullptr;
BOOL (*BASS_WASAPI_Stop)(BOOL reset) = nullptr;
BOOL (*BASS_WASAPI_SetVolume)(DWORD mode, float volume) = nullptr;
BOOL (*BASS_WASAPI_GetInfo)(BASS_WASAPI_INFO *info) = nullptr;
BOOL (*BASS_WASAPI_GetDeviceInfo)(DWORD device, BASS_WASAPI_DEVICEINFO *info) = nullptr;

// BASSMIX
HSTREAM (*BASS_Mixer_StreamCreate)(DWORD freq, DWORD chans, DWORD flags) = nullptr;
BOOL (*BASS_Mixer_StreamAddChannel)(HSTREAM handle, DWORD channel, DWORD flags) = nullptr;
DWORD (*BASS_Mixer_ChannelGetMixer)(DWORD handle) = nullptr;
BOOL (*BASS_Mixer_ChannelRemove)(DWORD handle) = nullptr;
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

	BASS_GetVersion = loadFunction<decltype(BASS_GetVersion)>(s_bassLib, "BASS_GetVersion");
	BASS_SetConfig = loadFunction<decltype(BASS_SetConfig)>(s_bassLib, "BASS_SetConfig");
	BASS_GetConfig = loadFunction<decltype(BASS_GetConfig)>(s_bassLib, "BASS_GetConfig");
	BASS_Init = loadFunction<decltype(BASS_Init)>(s_bassLib, "BASS_Init");
	BASS_Free = loadFunction<decltype(BASS_Free)>(s_bassLib, "BASS_Free");
	BASS_GetDeviceInfo = loadFunction<decltype(BASS_GetDeviceInfo)>(s_bassLib, "BASS_GetDeviceInfo");
	BASS_ErrorGetCode = loadFunction<decltype(BASS_ErrorGetCode)>(s_bassLib, "BASS_ErrorGetCode");
	BASS_StreamCreateFile = loadFunction<decltype(BASS_StreamCreateFile)>(s_bassLib, "BASS_StreamCreateFile");
	BASS_SampleLoad = loadFunction<decltype(BASS_SampleLoad)>(s_bassLib, "BASS_SampleLoad");
	BASS_SampleFree = loadFunction<decltype(BASS_SampleFree)>(s_bassLib, "BASS_SampleFree");
	BASS_SampleGetChannel = loadFunction<decltype(BASS_SampleGetChannel)>(s_bassLib, "BASS_SampleGetChannel");
	BASS_ChannelPlay = loadFunction<decltype(BASS_ChannelPlay)>(s_bassLib, "BASS_ChannelPlay");
	BASS_ChannelPause = loadFunction<decltype(BASS_ChannelPause)>(s_bassLib, "BASS_ChannelPause");
	BASS_ChannelStop = loadFunction<decltype(BASS_ChannelStop)>(s_bassLib, "BASS_ChannelStop");
	BASS_ChannelSetAttribute = loadFunction<decltype(BASS_ChannelSetAttribute)>(s_bassLib, "BASS_ChannelSetAttribute");
	BASS_ChannelGetAttribute = loadFunction<decltype(BASS_ChannelGetAttribute)>(s_bassLib, "BASS_ChannelGetAttribute");
	BASS_ChannelSetPosition = loadFunction<decltype(BASS_ChannelSetPosition)>(s_bassLib, "BASS_ChannelSetPosition");
	BASS_ChannelGetPosition = loadFunction<decltype(BASS_ChannelGetPosition)>(s_bassLib, "BASS_ChannelGetPosition");
	BASS_ChannelGetLength = loadFunction<decltype(BASS_ChannelGetLength)>(s_bassLib, "BASS_ChannelGetLength");
	BASS_ChannelFlags = loadFunction<decltype(BASS_ChannelFlags)>(s_bassLib, "BASS_ChannelFlags");
	BASS_ChannelIsActive = loadFunction<decltype(BASS_ChannelIsActive)>(s_bassLib, "BASS_ChannelIsActive");
	BASS_ChannelBytes2Seconds = loadFunction<decltype(BASS_ChannelBytes2Seconds)>(s_bassLib, "BASS_ChannelBytes2Seconds");
	BASS_ChannelSeconds2Bytes = loadFunction<decltype(BASS_ChannelSeconds2Bytes)>(s_bassLib, "BASS_ChannelSeconds2Bytes");
	BASS_ChannelSet3DPosition = loadFunction<decltype(BASS_ChannelSet3DPosition)>(s_bassLib, "BASS_ChannelSet3DPosition");
	BASS_Set3DPosition = loadFunction<decltype(BASS_Set3DPosition)>(s_bassLib, "BASS_Set3DPosition");
	BASS_Apply3D = loadFunction<decltype(BASS_Apply3D)>(s_bassLib, "BASS_Apply3D");
	BASS_StreamFree = loadFunction<decltype(BASS_StreamFree)>(s_bassLib, "BASS_StreamFree");

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

	BASS_FX_TempoCreate = loadFunction<decltype(BASS_FX_TempoCreate)>(s_bassFxLib, "BASS_FX_TempoCreate");

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

	BASS_WASAPI_Init = loadFunction<decltype(BASS_WASAPI_Init)>(s_bassWasapiLib, "BASS_WASAPI_Init");
	BASS_WASAPI_Free = loadFunction<decltype(BASS_WASAPI_Free)>(s_bassWasapiLib, "BASS_WASAPI_Free");
	BASS_WASAPI_Start = loadFunction<decltype(BASS_WASAPI_Start)>(s_bassWasapiLib, "BASS_WASAPI_Start");
	BASS_WASAPI_Stop = loadFunction<decltype(BASS_WASAPI_Stop)>(s_bassWasapiLib, "BASS_WASAPI_Stop");
	BASS_WASAPI_SetVolume = loadFunction<decltype(BASS_WASAPI_SetVolume)>(s_bassWasapiLib, "BASS_WASAPI_SetVolume");
	BASS_WASAPI_GetInfo = loadFunction<decltype(BASS_WASAPI_GetInfo)>(s_bassWasapiLib, "BASS_WASAPI_GetInfo");
	BASS_WASAPI_GetDeviceInfo = loadFunction<decltype(BASS_WASAPI_GetDeviceInfo)>(s_bassWasapiLib, "BASS_WASAPI_GetDeviceInfo");

	// BASSMIX
	if (!(s_bassMixLib = SDL_LoadObject(BASS_MIX_LIB_NAME)) && !(s_bassMixLib = SDL_LoadObject(std::format("lib/{}", BASS_MIX_LIB_NAME).c_str())))
	{
		debugLog("BassLoader: Failed to load BASSMIX library: %s\n", SDL_GetError());
		// TODO: graceful failure here?
		cleanup();
		return false;
	}

	BASS_Mixer_StreamCreate = loadFunction<decltype(BASS_Mixer_StreamCreate)>(s_bassMixLib, "BASS_Mixer_StreamCreate");
	BASS_Mixer_StreamAddChannel = loadFunction<decltype(BASS_Mixer_StreamAddChannel)>(s_bassMixLib, "BASS_Mixer_StreamAddChannel");
	BASS_Mixer_ChannelGetMixer = loadFunction<decltype(BASS_Mixer_ChannelGetMixer)>(s_bassMixLib, "BASS_Mixer_ChannelGetMixer");
	BASS_Mixer_ChannelRemove = loadFunction<decltype(BASS_Mixer_ChannelRemove)>(s_bassMixLib, "BASS_Mixer_ChannelRemove");
#endif

	//debugLog("BassLoader: Successfully loaded all libraries\n");
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

#else
namespace BassLoader {
	bool init() {return true;}
	void cleanup() {return;}
};

#endif
