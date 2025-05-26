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

#define LNAME(x) (LPREFIX #x LSUFFIX)

static SDL_SharedObject *s_bassLib = nullptr;
static SDL_SharedObject *s_bassFxLib = nullptr;
[[maybe_unused]] static SDL_SharedObject *s_bassMixLib = nullptr;
[[maybe_unused]] static SDL_SharedObject *s_bassAsioLib = nullptr;
[[maybe_unused]] static SDL_SharedObject *s_bassWasapiLib = nullptr;

static constexpr auto BASS_LIB_NAME = LNAME(bass);
static constexpr auto BASS_FX_LIB_NAME = LNAME(bass_fx);
[[maybe_unused]] static constexpr auto BASS_MIX_LIB_NAME = LNAME(bassmix);
[[maybe_unused]] static constexpr auto BASS_ASIO_LIB_NAME = LNAME(bassasio);
[[maybe_unused]] static constexpr auto BASS_WASAPI_LIB_NAME = LNAME(basswasapi);

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
#define LOAD_BASS_FUNCTION(name) name = loadFunction<name##_t>(s_bassLib, #name);
#define LOAD_BASS_FX_FUNCTION(name) name = loadFunction<name##_t>(s_bassFxLib, #name);
#define LOAD_BASS_MIX_FUNCTION(name) name = loadFunction<name##_t>(s_bassMixLib, #name);
#define LOAD_BASS_ASIO_FUNCTION(name) name = loadFunction<name##_t>(s_bassAsioLib, #name);
#define LOAD_BASS_WASAPI_FUNCTION(name) name = loadFunction<name##_t>(s_bassWasapiLib, #name);

#define CLEANUP_BASS_FUNCTION(name) name = nullptr;

bool init()
{
	// just make sure we don't load more than once
	cleanup();

	// BASS core library loading
	for (auto path : {std::string(BASS_LIB_NAME), fmt::format("lib{}{}", Env::cfg(OS::WINDOWS) ? "\\" : "/", BASS_LIB_NAME)})
	{
		if (!(s_bassLib = SDL_LoadObject(path.c_str())) || !(BASS_GetVersion = loadFunction<BASS_GetVersion_t>(s_bassLib, "BASS_GetVersion")))
		{
			s_bassLib = nullptr;
			continue;
		}
		if (static_cast<uint64_t>(BASS_GetVersion()) >= static_cast<uint64_t>(BASSVERSION_REAL))
			break;
		debugLog("BassLoader: version mismatch for {:s} (expected {:x}, got {:x})\n", path.c_str(), BASSVERSION_REAL, BASS_GetVersion());
		s_bassLib = nullptr;
		BASS_GetVersion = nullptr;
	}
	if (!s_bassLib)
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
	for (auto path : {std::string(BASS_FX_LIB_NAME), fmt::format("lib{}{}", Env::cfg(OS::WINDOWS) ? "\\" : "/", BASS_FX_LIB_NAME)})
	{
		if (!(s_bassFxLib = SDL_LoadObject(path.c_str())) || !(BASS_FX_GetVersion = loadFunction<BASS_FX_GetVersion_t>(s_bassFxLib, "BASS_FX_GetVersion")))
		{
			s_bassFxLib = nullptr;
			continue;
		}
		if (static_cast<uint64_t>(BASS_FX_GetVersion()) >= static_cast<uint64_t>(BASSFXVERSION_REAL))
			break;
		debugLog("BassLoader: version mismatch for {:s} (expected {:x}, got {:x})\n", path.c_str(), BASSFXVERSION_REAL, BASS_FX_GetVersion());
		s_bassFxLib = nullptr;
		BASS_FX_GetVersion = nullptr;
	}
	if (!s_bassFxLib)
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
#ifdef MCENGINE_NEOSU_BASS_PORT_FINISHED
	// BASSMIX library loading
	for (auto path : {std::string(BASS_MIX_LIB_NAME), fmt::format("lib{}{}", Env::cfg(OS::WINDOWS) ? "\\" : "/", BASS_MIX_LIB_NAME)})
	{
		if (!(s_bassMixLib = SDL_LoadObject(path.c_str())) || !(BASS_Mixer_GetVersion = loadFunction<BASS_Mixer_GetVersion_t>(s_bassMixLib, "BASS_Mixer_GetVersion")))
		{
			s_bassMixLib = nullptr;
			continue;
		}
		if (static_cast<uint64_t>(BASS_Mixer_GetVersion()) >= static_cast<uint64_t>(BASSMIXVERSION_REAL))
			break;
		debugLog("BassLoader: version mismatch for {:s} (expected {:x}, got {:x})\n", path.c_str(), BASSMIXVERSION_REAL, BASS_Mixer_GetVersion());
		s_bassMixLib = nullptr;
		BASS_Mixer_GetVersion = nullptr;
	}
	if (!s_bassMixLib)
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

#ifdef MCENGINE_PLATFORM_WINDOWS
	for (auto path : {std::string(BASS_ASIO_LIB_NAME), fmt::format("lib{}{}", "\\", BASS_ASIO_LIB_NAME)})
	{
		if (!(s_bassAsioLib = SDL_LoadObject(path.c_str())) || !(BASS_ASIO_GetVersion = loadFunction<BASS_ASIO_GetVersion_t>(s_bassAsioLib, "BASS_ASIO_GetVersion")))
		{
			s_bassAsioLib = nullptr;
			continue;
		}
		if (static_cast<uint64_t>(BASS_ASIO_GetVersion()) >= static_cast<uint64_t>(BASSASIOVERSION_REAL))
			break;
		debugLog("BassLoader: version mismatch for {:s} (expected {:x}, got {:x})\n", path.c_str(), BASSASIOVERSION_REAL, BASS_ASIO_GetVersion());
		s_bassAsioLib = nullptr;
		BASS_ASIO_GetVersion = nullptr;
	}
	if (!s_bassAsioLib)
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

	for (auto path : {std::string(BASS_WASAPI_LIB_NAME), fmt::format("lib{}{}", "\\", BASS_WASAPI_LIB_NAME)})
	{
		if (!(s_bassWasapiLib = SDL_LoadObject(path.c_str())) || !(BASS_WASAPI_GetVersion = loadFunction<BASS_WASAPI_GetVersion_t>(s_bassWasapiLib, "BASS_WASAPI_GetVersion")))
		{
			s_bassWasapiLib = nullptr;
			continue;
		}
		if (static_cast<uint64_t>(BASS_WASAPI_GetVersion()) >= static_cast<uint64_t>(BASSWASAPIVERSION_REAL))
			break;
		debugLog("BassLoader: version mismatch for {:s} (expected {:x}, got {:x})\n", path.c_str(), BASSWASAPIVERSION_REAL, BASS_WASAPI_GetVersion());
		s_bassWasapiLib = nullptr;
		BASS_WASAPI_GetVersion = nullptr;
	}
	if (!s_bassWasapiLib)
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

	// unload in reverse order
#ifdef MCENGINE_NEOSU_BASS_PORT_FINISHED
#ifdef MCENGINE_PLATFORM_WINDOWS
	if (s_bassWasapiLib)
	{
		SDL_UnloadObject(s_bassWasapiLib);
		s_bassWasapiLib = nullptr;
	}
	if (s_bassAsioLib)
	{
		SDL_UnloadObject(s_bassAsioLib);
		s_bassAsioLib = nullptr;
	}
#endif
	if (s_bassMixLib)
	{
		SDL_UnloadObject(s_bassMixLib);
		s_bassMixLib = nullptr;
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

	// reset all function pointers to null
	BASS_CORE_FUNCTIONS(CLEANUP_BASS_FUNCTION)
	BASS_FX_FUNCTIONS(CLEANUP_BASS_FUNCTION)
#ifdef MCENGINE_NEOSU_BASS_PORT_FINISHED
	BASS_MIX_FUNCTIONS(CLEANUP_BASS_FUNCTION)

#ifdef MCENGINE_PLATFORM_WINDOWS
	BASS_ASIO_FUNCTIONS(CLEANUP_BASS_FUNCTION)
	BASS_WASAPI_FUNCTIONS(CLEANUP_BASS_FUNCTION)
#endif
#endif
}
} // namespace BassLoader

#endif
