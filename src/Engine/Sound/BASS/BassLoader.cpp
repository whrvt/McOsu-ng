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
#if MCENGINE_PLATFORM_WINDOWS
#define LPREFIX ""
#define LSUFFIX ".dll"
#else
#define LPREFIX "lib"
#define LSUFFIX ".so"
#endif

#define LNAME(x) LPREFIX #x LSUFFIX

#define DEFINE_LIB(name) [[maybe_unused]] static SDL_SharedObject *s_lib##name = nullptr; \
	[[maybe_unused]] static constexpr auto name##_libpaths = {LNAME(name), "lib/" LNAME(name)};

DEFINE_LIB(bass)
DEFINE_LIB(bass_fx)
DEFINE_LIB(bassmix)
DEFINE_LIB(bassasio)
DEFINE_LIB(basswasapi)

// generate function pointer definitions
#define DEFINE_BASS_FUNCTION(name) name##_t name = nullptr;

BASS_CORE_FUNCTIONS(DEFINE_BASS_FUNCTION)
BASS_FX_FUNCTIONS(DEFINE_BASS_FUNCTION)
BASS_MIX_FUNCTIONS(DEFINE_BASS_FUNCTION)

#ifdef MCENGINE_PLATFORM_WINDOWS
BASS_WASAPI_FUNCTIONS(DEFINE_BASS_FUNCTION)
BASS_ASIO_FUNCTIONS(DEFINE_BASS_FUNCTION)
#endif

template <typename T>
T loadFunction(SDL_SharedObject *lib, const char *funcName)
{
	T func = reinterpret_cast<T>(SDL_LoadFunction(lib, funcName));
	if (!func)
		debugLog("BassLoader: Failed to load function {:s}: {:s}\n", funcName, SDL_GetError());
	return func;
}

// macros for loading/cleaning up functions from specific libraries
#define LOAD_BASS_FUNCTION(name) name = loadFunction<name##_t>(s_libbass, #name);
#define LOAD_BASS_FX_FUNCTION(name) name = loadFunction<name##_t>(s_libbass_fx, #name);
#define LOAD_BASS_MIX_FUNCTION(name) name = loadFunction<name##_t>(s_libbassmix, #name);
#define LOAD_BASS_ASIO_FUNCTION(name) name = loadFunction<name##_t>(s_libbassasio, #name);
#define LOAD_BASS_WASAPI_FUNCTION(name) name = loadFunction<name##_t>(s_libbasswasapi, #name);

#define UNLOAD_LIB(name) if (s_lib##name) \
	{ \
		SDL_UnloadObject(s_lib##name); \
		s_lib##name = nullptr; \
	}

#define CLEANUP_BASS_FUNCTION(name) name = nullptr;

bool init()
{
	// just make sure we don't load more than once
	cleanup();

	// BASS core library loading
	for (auto path : bass_libpaths)
	{
		if (!(s_libbass = SDL_LoadObject(path)) || !(BASS_GetVersion = loadFunction<BASS_GetVersion_t>(s_libbass, "BASS_GetVersion")))
		{
			UNLOAD_LIB(bass)
			continue;
		}
		if (static_cast<uint64_t>(BASS_GetVersion()) >= static_cast<uint64_t>(BASSVERSION_REAL))
			break;
		debugLog("BassLoader: version too old for {:s} (expected {:x}, got {:x})\n", path, BASSVERSION_REAL, BASS_GetVersion());
		UNLOAD_LIB(bass) // try again in the lib/ folder
		BASS_GetVersion = nullptr;
	}
	if (!s_libbass)
	{
		debugLog("BassLoader: Failed to load BASS library: {:s}\n", SDL_GetError());
		return false;
	}

	// load all BASS core functions
	BASS_CORE_FUNCTIONS(LOAD_BASS_FUNCTION)

	// quick sanity check
	if (!BASS_Init || !BASS_Free || !BASS_ErrorGetCode)
	{
		debugLog("BassLoader: Failed to load essential BASS functions\n");
		cleanup();
		return false;
	}

	// BASS_FX library loading
	for (auto path : bass_fx_libpaths)
	{
		if (!(s_libbass_fx = SDL_LoadObject(path)) || !(BASS_FX_GetVersion = loadFunction<BASS_FX_GetVersion_t>(s_libbass_fx, "BASS_FX_GetVersion")))
		{
			UNLOAD_LIB(bass_fx)
			continue;
		}
		if (static_cast<uint64_t>(BASS_FX_GetVersion()) >= static_cast<uint64_t>(BASSFXVERSION_REAL))
			break;
		debugLog("BassLoader: version too old for {:s} (expected {:x}, got {:x})\n", path, BASSFXVERSION_REAL, BASS_FX_GetVersion());
		UNLOAD_LIB(bass_fx) // try again in the lib/ folder
		BASS_FX_GetVersion = nullptr;
	}
	if (!s_libbass_fx)
	{
		debugLog("BassLoader: Failed to load BASS_FX library: {:s}\n", SDL_GetError());
		return false;
	}

	// load all BASS_FX functions
	BASS_FX_FUNCTIONS(LOAD_BASS_FX_FUNCTION)

	// quick sanity check
	if (!BASS_FX_TempoCreate)
	{
		debugLog("BassLoader: Failed to load essential BASS_FX functions\n");
		cleanup();
		return false;
	}
#if defined(MCENGINE_NEOSU_BASS_PORT_FINISHED) || (defined(MCENGINE_PLATFORM_WINDOWS) && defined(MCENGINE_FEATURE_BASS_WASAPI))
	// BASSMIX library loading
	for (auto path : bassmix_libpaths)
	{
		if (!(s_libbassmix = SDL_LoadObject(path)) || !(BASS_Mixer_GetVersion = loadFunction<BASS_Mixer_GetVersion_t>(s_libbassmix, "BASS_Mixer_GetVersion")))
		{
			UNLOAD_LIB(bassmix)
			continue;
		}
		if (static_cast<uint64_t>(BASS_Mixer_GetVersion()) >= static_cast<uint64_t>(BASSMIXVERSION_REAL))
			break;
		debugLog("BassLoader: version too old for {:s} (expected {:x}, got {:x})\n", path, BASSMIXVERSION_REAL, BASS_Mixer_GetVersion());
		UNLOAD_LIB(bassmix) // try again in the lib/ folder
		BASS_Mixer_GetVersion = nullptr;
	}
	if (!s_libbassmix)
	{
		debugLog("BassLoader: Failed to load BASSMIX library: {:s}\n", SDL_GetError());
		return false;
	}

	// load all BASSMIX functions
	BASS_MIX_FUNCTIONS(LOAD_BASS_MIX_FUNCTION)

	// quick sanity check
	if (!BASS_Mixer_ChannelGetMixer)
	{
		debugLog("BassLoader: Failed to load essential BASSMIX functions\n");
		cleanup();
		return false;
	}
#endif
#if defined(MCENGINE_PLATFORM_WINDOWS)
#ifdef MCENGINE_NEOSU_BASS_PORT_FINISHED
	for (auto path : bassasio_libpaths)
	{
		if (!(s_libbassasio = SDL_LoadObject(path)) || !(BASS_ASIO_GetVersion = loadFunction<BASS_ASIO_GetVersion_t>(s_libbassasio, "BASS_ASIO_GetVersion")))
		{
			UNLOAD_LIB(bassasio)
			continue;
		}
		if (static_cast<uint64_t>(BASS_ASIO_GetVersion()) >= static_cast<uint64_t>(BASSASIOVERSION_REAL))
			break;
		debugLog("BassLoader: version too old for {:s} (expected {:x}, got {:x})\n", path, BASSASIOVERSION_REAL, BASS_ASIO_GetVersion());
		UNLOAD_LIB(bassasio) // try again in the lib/ folder
		BASS_ASIO_GetVersion = nullptr;
	}
	if (!s_libbassasio)
	{
		debugLog("BassLoader: Failed to load BASSASIO library: {:s}\n", SDL_GetError());
		return false;
	}

	// load all BASSASIO functions
	BASS_ASIO_FUNCTIONS(LOAD_BASS_ASIO_FUNCTION)

	// quick sanity check
	if (!BASS_ASIO_GetInfo)
	{
		debugLog("BassLoader: Failed to load essential BASSASIO functions\n");
		cleanup();
		return false;
	}
#endif
#if (defined(MCENGINE_NEOSU_BASS_PORT_FINISHED) || defined(MCENGINE_FEATURE_BASS_WASAPI))
	for (auto path : basswasapi_libpaths)
	{
		if (!(s_libbasswasapi = SDL_LoadObject(path)) || !(BASS_WASAPI_GetVersion = loadFunction<BASS_WASAPI_GetVersion_t>(s_libbasswasapi, "BASS_WASAPI_GetVersion")))
		{
			UNLOAD_LIB(basswasapi)
			continue;
		}
		if (static_cast<uint64_t>(BASS_WASAPI_GetVersion()) >= static_cast<uint64_t>(BASSWASAPIVERSION_REAL))
			break;
		debugLog("BassLoader: version too old for {:s} (expected {:x}, got {:x})\n", path, BASSWASAPIVERSION_REAL, BASS_WASAPI_GetVersion());
		UNLOAD_LIB(basswasapi) // try again in the lib/ folder
		BASS_WASAPI_GetVersion = nullptr;
	}
	if (!s_libbasswasapi)
	{
		debugLog("BassLoader: Failed to load BASSWASAPI library: {:s}\n", SDL_GetError());
		return false;
	}

	// load all BASSWASAPI functions
	BASS_WASAPI_FUNCTIONS(LOAD_BASS_WASAPI_FUNCTION)

	// quick sanity check
	if (!BASS_WASAPI_GetDeviceInfo)
	{
		debugLog("BassLoader: Failed to load essential BASSWASAPI functions\n");
		cleanup();
		return false;
	}
#endif
#endif

	// debugLog("BassLoader: Successfully loaded all libraries\n");
	return true;
}

void cleanup()
{
	// unload in reverse order (ifdef hell to be removed once the neosu port is done)
#ifdef MCENGINE_PLATFORM_WINDOWS
#if (defined(MCENGINE_NEOSU_BASS_PORT_FINISHED) || defined(MCENGINE_FEATURE_BASS_WASAPI))
	UNLOAD_LIB(basswasapi)
#endif
#ifdef MCENGINE_NEOSU_BASS_PORT_FINISHED
	UNLOAD_LIB(bassasio)
#endif
#endif
#if defined(MCENGINE_NEOSU_BASS_PORT_FINISHED) || (defined(MCENGINE_PLATFORM_WINDOWS) && defined(MCENGINE_FEATURE_BASS_WASAPI))
	UNLOAD_LIB(bassmix)
#endif
	UNLOAD_LIB(bass_fx)
	UNLOAD_LIB(bass)

	// reset all function pointers to null (ifdef hell to be removed once the neosu port is done)
	BASS_CORE_FUNCTIONS(CLEANUP_BASS_FUNCTION)
	BASS_FX_FUNCTIONS(CLEANUP_BASS_FUNCTION)
#if defined(MCENGINE_NEOSU_BASS_PORT_FINISHED) || (defined(MCENGINE_PLATFORM_WINDOWS) && defined(MCENGINE_FEATURE_BASS_WASAPI))
	BASS_MIX_FUNCTIONS(CLEANUP_BASS_FUNCTION)
#endif
#ifdef MCENGINE_PLATFORM_WINDOWS
#ifdef MCENGINE_NEOSU_BASS_PORT_FINISHED
	BASS_ASIO_FUNCTIONS(CLEANUP_BASS_FUNCTION)
#endif
#if (defined(MCENGINE_NEOSU_BASS_PORT_FINISHED) || defined(MCENGINE_FEATURE_BASS_WASAPI))
	BASS_WASAPI_FUNCTIONS(CLEANUP_BASS_FUNCTION)
#endif
#endif
}
} // namespace BassLoader

#endif
