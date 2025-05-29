//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		handles sounds using BASS library
//
// $NoKeywords: $snd $bass
//===============================================================================//

#include "BassSoundEngine.h"
#include "BassSound.h"

#ifdef MCENGINE_FEATURE_BASS

#include "BassManager.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"

#include <utility>

extern ConVar snd_output_device;
extern ConVar snd_restart;
extern ConVar snd_freq;
extern ConVar snd_restrict_play_frame;
extern ConVar snd_change_check_interval;
extern ConVar win_snd_fallback_dsound;
extern ConVar debug_snd;

// BASS-specific ConVars
ConVar snd_updateperiod("snd_updateperiod", 5, FCVAR_NONE, "BASS_CONFIG_UPDATEPERIOD length in milliseconds, minimum is 5");
ConVar snd_buffer("snd_buffer", 100, FCVAR_NONE, "BASS_CONFIG_BUFFER length in milliseconds, minimum is 1 above snd_updateperiod");
ConVar snd_dev_period("snd_dev_period", 5, FCVAR_NONE, "BASS_CONFIG_DEV_PERIOD length in milliseconds, or if negative then in samples");
ConVar snd_dev_buffer("snd_dev_buffer", 10, FCVAR_NONE, "BASS_CONFIG_DEV_BUFFER length in milliseconds");

#ifdef MCENGINE_FEATURE_BASS_WASAPI
// WASAPI-specific ConVars
ConVar win_snd_wasapi_buffer_size("win_snd_wasapi_buffer_size", 0.011f, FCVAR_NONE,
                                  "buffer size/length in seconds (e.g. 0.011 = 11 ms), directly responsible for audio delay and crackling");
ConVar win_snd_wasapi_period_size("win_snd_wasapi_period_size", 0.0f, FCVAR_NONE,
                                  "interval between OutputWasapiProc calls in seconds (e.g. 0.016 = 16 ms) (0 = use default)");
ConVar win_snd_wasapi_exclusive("win_snd_wasapi_exclusive", true, FCVAR_NONE, "whether to use exclusive device mode to further reduce latency");
ConVar win_snd_wasapi_shared_volume_affects_device("win_snd_wasapi_shared_volume_affects_device", false, FCVAR_NONE,
                                                   "if in shared mode, whether to affect device volume globally or use separate session volume (default)");

SOUNDHANDLE g_wasapiOutputMixer = 0;

DWORD CALLBACK OutputWasapiProc(void *buffer, DWORD length, void *user)
{
	if (g_wasapiOutputMixer != 0)
	{
		const DWORD c = BASS_ChannelGetData(g_wasapiOutputMixer, buffer, length);
		return (c < 0) ? 0 : c;
	}
	return 0;
}

// ConVar change callbacks for WASAPI settings that require restart
void forceDeviceRestart()
{
	soundEngine->setOutputDeviceForce(soundEngine->getOutputDevice());
}

void _WIN_SND_WASAPI_BUFFER_SIZE_CHANGE(UString oldValue, UString newValue)
{
	const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
	const int newValueMS = std::round(newValue.toFloat() * 1000.0f);
	if (oldValueMS != newValueMS)
		forceDeviceRestart();
}

void _WIN_SND_WASAPI_PERIOD_SIZE_CHANGE(UString oldValue, UString newValue)
{
	const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
	const int newValueMS = std::round(newValue.toFloat() * 1000.0f);
	if (oldValueMS != newValueMS)
		forceDeviceRestart();
}

void _WIN_SND_WASAPI_EXCLUSIVE_CHANGE(UString oldValue, UString newValue)
{
	const bool oldValueBool = oldValue.toInt();
	const bool newValueBool = newValue.toInt();
	if (oldValueBool != newValueBool)
		forceDeviceRestart();
}
#endif

BassSoundEngine::BassSoundEngine() : SoundEngine()
{
	if (!BassManager::init()) // this checks the library versions as well
	{
		engine->showMessageErrorFatal("Fatal Sound Error", UString::fmt("Failed to load BASS library: {:s} !", BassManager::getFailedLibrary()));
		engine->shutdown();
		return;
	}

	// apply default global settings
	BASS_SetConfig(BASS_CONFIG_BUFFER, static_cast<DWORD>(snd_buffer.getDefaultFloat()));
	BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, static_cast<DWORD>(snd_dev_buffer.getDefaultFloat())); // NOTE: only used by new osu atm
	BASS_SetConfig(BASS_CONFIG_MP3_OLDGAPS,
	               1); // NOTE: only used by osu atm (all beatmaps timed to non-iTunesSMPB + 529 sample deletion offsets on old dlls pre 2015)
	BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP,
	               1); // NOTE: only used by osu atm (avoids lag/jitter in BASS_ChannelGetPosition() shortly after a BASS_ChannelPlay() after loading/silence)

	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, (Env::cfg(AUD::WASAPI) ? 0 : static_cast<DWORD>(snd_updateperiod.getDefaultFloat()))); // NOTE: only used by osu atm
	BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, (Env::cfg(AUD::WASAPI) ? 0 : 1));
	BASS_SetConfig(BASS_CONFIG_VISTA_TRUEPOS, 0); // NOTE: if set to 1, increases sample playback latency +10 ms
	BASS_SetConfig(BASS_CONFIG_DEV_TIMEOUT, 0);   // prevents playback from ever stopping due to device issues

	// add default output device
	m_iCurrentOutputDevice = -1;
	m_sCurrentOutputDevice = "NULL";

	OUTPUT_DEVICE defaultOutputDevice;
	defaultOutputDevice.id = -1;
	defaultOutputDevice.name = "Default";
	defaultOutputDevice.enabled = true;
	defaultOutputDevice.isDefault = false; // custom -1 can never have default

	snd_output_device.setValue(defaultOutputDevice.name);
	m_outputDevices.push_back(defaultOutputDevice);

	// add all other output devices
	updateOutputDevices(false, true);
	initializeOutputDevice(defaultOutputDevice.id);

	// convar callbacks
	snd_freq.setCallback(fastdelegate::MakeDelegate(this, &BassSoundEngine::onFreqChanged));
	snd_restart.setCallback(fastdelegate::MakeDelegate(this, &BassSoundEngine::restart));
	snd_output_device.setCallback(fastdelegate::MakeDelegate(this, &BassSoundEngine::setOutputDevice));

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	win_snd_wasapi_buffer_size.setCallback(_WIN_SND_WASAPI_BUFFER_SIZE_CHANGE);
	win_snd_wasapi_period_size.setCallback(_WIN_SND_WASAPI_PERIOD_SIZE_CHANGE);
	win_snd_wasapi_exclusive.setCallback(_WIN_SND_WASAPI_EXCLUSIVE_CHANGE);
#endif
}

BassSoundEngine::~BassSoundEngine()
{
	if (m_bReady)
	{
		BASS_Free();
#ifdef MCENGINE_FEATURE_BASS_WASAPI
		BASS_WASAPI_Free();
#endif
	}
	BassManager::cleanup();
}

void BassSoundEngine::restart()
{
	setOutputDeviceForce(m_sCurrentOutputDevice);

	// callback (reload sound buffers etc.)
	if (m_outputDeviceChangeCallback != nullptr)
		m_outputDeviceChangeCallback();
}

void BassSoundEngine::update()
{
	// unused currently
}

bool BassSoundEngine::play(Sound *snd, float pan, float pitch)
{
	auto [bassSound, handle] = GETHANDLE();

	const bool allowPlayFrame = bassSound && (!bassSound->isOverlayable() || !snd_restrict_play_frame.getBool() || engine->getTime() > bassSound->getLastPlayTime());

	if (!allowPlayFrame || !handle)
	{
		if (!handle && debug_snd.getBool())
			debugLog("invalid handle to play! last BASS error: {}\n", BASS_ErrorGetCode());
		return false;
	}

	return playChannelWithAttributes(bassSound, handle, pan, pitch);
}

bool BassSoundEngine::play3d(Sound *snd, Vector3 pos)
{
	auto [bassSound, handle] = GETHANDLE(snd->is3d());
	if (!handle)
	{
		if (debug_snd.getBool())
			debugLog("invalid handle to play3d! last BASS error: {}\n", BASS_ErrorGetCode());
		return false;
	}

	if (!snd_restrict_play_frame.getBool() || engine->getTime() > bassSound->getLastPlayTime())
	{
		if (BASS_ChannelIsActive(handle) != BASS_ACTIVE_PLAYING)
		{
			BASS_3DVECTOR bassPos = BASS_3DVECTOR(pos.x, pos.y, pos.z);
			if (!BASS_ChannelSet3DPosition(handle, &bassPos, nullptr, nullptr))
				BassManager::printBassError(fmt::format("BASS_ChannelSet3DPosition({}, bassPos, NULL, NULL)", handle), BASS_ErrorGetCode());
			else
				BASS_Apply3D();

			bool ret = BASS_ChannelPlay(handle, FALSE);
			if (!ret)
				BassManager::printBassError(fmt::format("BASS_ChannelPlay({}, FALSE)", handle), BASS_ErrorGetCode());
			else
				bassSound->setLastPlayTime(engine->getTime());

			return ret;
		}
	}

	return false;
}

void BassSoundEngine::pause(Sound *snd)
{
	auto [bassSound, handle] = GETHANDLE();
	if (!handle)
	{
		if (debug_snd.getBool())
			debugLog("no handle to pause! last BASS error: {}\n", BASS_ErrorGetCode());
		return;
	}

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	if (bassSound->isStream())
	{
		if (bassSound->isPlaying())
		{
			bassSound->setPrevPosition(bassSound->getPositionMS());
			BASS_Mixer_ChannelRemove(handle);
		}
		else
		{
			play(bassSound);
			bassSound->setPositionMS(bassSound->getPrevPosition());
		}
	}
	else if (!BASS_ChannelPause(handle))
	{
		BassManager::printBassError(fmt::format("BASS_ChannelPause({})", handle), BASS_ErrorGetCode());
	}
#else
	BASS_ChannelPause(handle);
	bassSound->setLastPlayTime(0.0);
#endif
}

void BassSoundEngine::stop(Sound *snd)
{
	auto [bassSound, handle] = GETHANDLE();
	if (!handle)
	{
		if (debug_snd.getBool())
			debugLog("invalid handle to stop! last BASS error: {}\n", BASS_ErrorGetCode());
		return;
	}

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	if (BASS_Mixer_ChannelGetMixer(handle) != 0)
		BASS_Mixer_ChannelRemove(handle);
#endif

	BASS_ChannelStop(handle);

	bassSound->setPosition(0.0);
	bassSound->setLastPlayTime(0.0);

	// alow next play()/getHandle() to reallocate (BASS_ChannelStop() frees the channel)
	bassSound->m_HCHANNEL = 0;
	bassSound->m_HCHANNELBACKUP = 0;
}

void BassSoundEngine::setOutputDevice(UString outputDeviceName)
{
	for (size_t i = 0; i < m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].name == outputDeviceName)
		{
			if (m_outputDevices[i].id != m_iCurrentOutputDevice)
			{
				int previousOutputDevice = m_iCurrentOutputDevice;
				if (!initializeOutputDevice(m_outputDevices[i].id))
					initializeOutputDevice(previousOutputDevice);
			}
			else
			{
				debugLog("\"{:s}\" already is the current device.\n", outputDeviceName.toUtf8());
			}
			return;
		}
	}

	debugLog("couldn't find output device \"{:s}\"!\n", outputDeviceName.toUtf8());
}

void BassSoundEngine::setOutputDeviceForce(UString outputDeviceName)
{
	for (auto &m_outputDevice : m_outputDevices)
	{
		if (m_outputDevice.name == outputDeviceName)
		{
			int previousOutputDevice = m_iCurrentOutputDevice;
			if (!initializeOutputDevice(m_outputDevice.id, true))
				initializeOutputDevice(previousOutputDevice, true);
			return;
		}
	}

	debugLog("couldn't find output device \"{:s}\"!\n", outputDeviceName.toUtf8());
}

void BassSoundEngine::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	// set volume for all BASS channel types (0 = silent, 10000 = full)
	const auto volumeValue = static_cast<DWORD>(m_fVolume * 10000);
	BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, volumeValue);
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, volumeValue);
	BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, volumeValue);

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS |
	                          (!win_snd_wasapi_exclusive.getBool() && !win_snd_wasapi_shared_volume_affects_device.getBool() ? BASS_WASAPI_VOL_SESSION : 0),
	                      m_fVolume);
#endif
}

void BassSoundEngine::set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp)
{
	if (!m_bReady)
		return;

	BASS_3DVECTOR bassHeadPos = BASS_3DVECTOR(headPos.x, headPos.y, headPos.z);
	BASS_3DVECTOR bassViewDir = BASS_3DVECTOR(viewDir.x, viewDir.y, viewDir.z);
	BASS_3DVECTOR bassViewUp = BASS_3DVECTOR(viewUp.x, viewUp.y, viewUp.z);

	if (!BASS_Set3DPosition(&bassHeadPos, nullptr, &bassViewDir, &bassViewUp))
		BassManager::printBassError("BASS_Set3DPosition(...)", BASS_ErrorGetCode());
	else
		BASS_Apply3D();
}

std::vector<UString> BassSoundEngine::getOutputDevices()
{
	std::vector<UString> outputDevices;
	for (const auto &device : m_outputDevices)
	{
		if (device.enabled)
			outputDevices.push_back(device.name);
	}
	return outputDevices;
}

void BassSoundEngine::updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo)
{
	const bool allowNoSoundDevice = true;
	const int sanityLimit = 42;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	BASS_WASAPI_DEVICEINFO deviceInfo;
#else
	BASS_DEVICEINFO deviceInfo;
#endif

	int numDevices = 0;
	for (int d = 0;
#ifdef MCENGINE_FEATURE_BASS_WASAPI
	     BASS_WASAPI_GetDeviceInfo(d, &deviceInfo);
#else
	     BASS_GetDeviceInfo(d, &deviceInfo);
#endif
	     d++)
	{
		const bool isEnabled = (deviceInfo.flags & BASS_DEVICE_ENABLED);
		const bool isDefault = (deviceInfo.flags & BASS_DEVICE_DEFAULT);

#ifdef MCENGINE_FEATURE_BASS_WASAPI
		const bool isInput = (deviceInfo.flags & BASS_DEVICE_INPUT);
		if (isInput)
			continue;
#endif

		if (printInfo)
		{
			debugLog("SoundEngine: Device {} = \"{:s}\", enabled = {}, default = {}\n", numDevices, deviceInfo.name, (int)isEnabled, (int)isDefault);
			numDevices++;
		}

		if (d > 0 || allowNoSoundDevice)
		{
			UString originalDeviceName = deviceInfo.name;
			OUTPUT_DEVICE soundDevice;
			soundDevice.id = d;
			soundDevice.name = originalDeviceName;
			soundDevice.enabled = isEnabled;
			soundDevice.isDefault = isDefault;

			// avoid duplicate names
			int duplicateNameCounter = 2;
			while (true)
			{
				bool foundDuplicateName = false;
				for (const auto &existingDevice : m_outputDevices)
				{
					if (existingDevice.name == soundDevice.name)
					{
						foundDuplicateName = true;
						soundDevice.name = originalDeviceName;
						soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));
						duplicateNameCounter++;
						break;
					}
				}

				if (!foundDuplicateName)
					break;
			}

			if ((d + 1 + (allowNoSoundDevice ? 1 : 0)) > m_outputDevices.size()) // only add new devices
				m_outputDevices.push_back(soundDevice);
		}

		// sanity
		if (d > sanityLimit)
		{
			debugLog("WARNING: found too many devices ...\n");
			break;
		}
	}
}

bool BassSoundEngine::initializeOutputDevice(int id, bool force)
{
	bool needsReinit = (!m_bReady || id != m_iCurrentOutputDevice || force);

	applyRuntimeConfigs(needsReinit);

	if (!needsReinit)
	{
		debugLog("SoundEngine: initializeOutputDevice( {} ) already init.\n", id);
		return true;
	}

	debugLog("SoundEngine: initializeOutputDevice( {}, fallback = {} ) ...\n", id, (int)win_snd_fallback_dsound.getBool());

	const bool canReinitInsteadOfFreeInit = (m_iCurrentOutputDevice == id) && m_iCurrentOutputDevice != -1 && id != -1;
	if (!canReinitInsteadOfFreeInit)
		BASS_Free();

	m_iCurrentOutputDevice = id;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	BASS_WASAPI_Free();
#endif

	// initialize BASS
	if (!initializeBass(id, canReinitInsteadOfFreeInit))
		return false;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	// initialize WASAPI
	if (!initializeWasapi(id))
		return false;
#endif

	m_bReady = true;

	// update current device name
	for (const auto &device : m_outputDevices)
	{
		if (device.id == id)
		{
			m_sCurrentOutputDevice = device.name;
			break;
		}
	}
	debugLog("SoundEngine: Output Device = \"{:s}\"\n", m_sCurrentOutputDevice.toUtf8());

	return true;
}

void BassSoundEngine::onFreqChanged(UString oldValue, UString newValue)
{
	if (oldValue == newValue)
	{
		debugLog("SoundEngine: frequency unchanged ({:s}).\n", newValue.toUtf8());
		return;
	}
	debugLog("SoundEngine: frequency changed ({:s})->({:s}).\n", oldValue.toUtf8(), newValue.toUtf8());
	restart();
}

// helper method implementations
void BassSoundEngine::applyRuntimeConfigs(bool &needsReinit)
{
	// allow users to override some defaults (but which may cause beatmap desyncs)
	// we only want to set these if their values have been explicitly modified (to avoid sideeffects in the default case, and for my sanity)
	if (snd_dev_buffer.getFloat() != snd_dev_buffer.getDefaultFloat())
	{
		BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, snd_dev_buffer.getInt());
		needsReinit = true;
	}
	if (snd_dev_period.getFloat() != snd_dev_period.getDefaultFloat())
	{
		BASS_SetConfig(BASS_CONFIG_DEV_PERIOD, snd_dev_period.getInt());
		needsReinit = true;
	}
	if (snd_updateperiod.getFloat() != snd_updateperiod.getDefaultFloat())
	{
		const auto clamped = static_cast<DWORD>(std::clamp(snd_updateperiod.getFloat(), 5.0f, 100.0f));
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, clamped);
		snd_updateperiod.setValue(clamped);
		needsReinit = true;
	}
	if (snd_buffer.getFloat() != snd_buffer.getDefaultFloat())
	{
		const auto clamped = static_cast<DWORD>(std::clamp(snd_buffer.getFloat(), snd_updateperiod.getFloat() + 1.0f, 5000.0f));
		BASS_SetConfig(BASS_CONFIG_BUFFER, clamped);
		snd_buffer.setValue(clamped);
		needsReinit = true;
	}
}

bool BassSoundEngine::initializeBass(int id, bool canReinitInsteadOfFreeInit)
{
	unsigned int runtimeFlags = canReinitInsteadOfFreeInit ? BASS_DEVICE_REINIT : 0;

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	runtimeFlags = BASS_DEVICE_NOSPEAKER;
#else
	runtimeFlags = (win_snd_fallback_dsound.getBool() ? BASS_DEVICE_DSOUND : BASS_DEVICE_NOSPEAKER);
#endif

	const int freq = snd_freq.getInt();
	const unsigned int flags = runtimeFlags;

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
	int idForBassInit = id;
#ifdef MCENGINE_FEATURE_BASS_WASAPI
	idForBassInit = 0;
#endif
	bool ret = BASS_Init(idForBassInit, freq, flags, env->getHwnd(), nullptr);
#else
	bool ret = BASS_Init(id, freq, flags, 0, nullptr);
#endif

	if (!ret && BASS_ErrorGetCode() == BASS_ERROR_ALREADY)
	{
		debugLog("SoundEngine: BASS was already initialized for device {}.\n", id);
	}
	else if (!ret)
	{
		m_bReady = false;

		if constexpr (Env::cfg(AUD::WASAPI))
		{
			// try again with dsound fallback, once
			if (!win_snd_fallback_dsound.getBool())
			{
				BassManager::printBassError(fmt::format("BASS_Init({}, {}, {}, 0, NULL)", id, freq, flags), BASS_ErrorGetCode());
				debugLog("Trying to fall back to DirectSound ...\n");

				win_snd_fallback_dsound.setValue(1.0f);
				const bool didFallbackSucceed = initializeOutputDevice(id);

				if (!didFallbackSucceed)
					win_snd_fallback_dsound.setValue(0.0f);

				return didFallbackSucceed;
			}
		}
		engine->showMessageError("Sound Error", UString::format("BASS_Init() failed (%i)!", BASS_ErrorGetCode()));
		return false;
	}

	return true;
}

bool BassSoundEngine::initializeWasapi(int id)
{
#ifdef MCENGINE_FEATURE_BASS_WASAPI
	const float bufferSize = std::round(win_snd_wasapi_buffer_size.getFloat() * 1000.0f) / 1000.0f;
	const float updatePeriod = std::round(win_snd_wasapi_period_size.getFloat() * 1000.0f) / 1000.0f;

	debugLog("WASAPI Exclusive Mode = {}, bufferSize = {:f}, updatePeriod = {:f}\n", (int)win_snd_wasapi_exclusive.getBool(), bufferSize, updatePeriod);

	bool ret = BASS_WASAPI_Init(id, 0, 0, (win_snd_wasapi_exclusive.getBool() ? BASS_WASAPI_EXCLUSIVE : 0), bufferSize, updatePeriod, OutputWasapiProc, nullptr);
	if (!ret)
	{
		m_bReady = false;
		const int errorCode = BASS_ErrorGetCode();

		if (errorCode == BASS_ERROR_WASAPI_BUFFER)
			BassManager::printBassError(fmt::format("BASS_WASAPI_Init({}, 0, 0, {}, {}, {}, OutputWasapiProc, NULL)", id,
			                                        (win_snd_wasapi_exclusive.getBool() ? BASS_WASAPI_EXCLUSIVE : 0), bufferSize, updatePeriod),
			                            errorCode);
		else
			engine->showMessageError("Sound Error", UString::format("BASS_WASAPI_Init() failed (%i)!", errorCode));

		return false;
	}

	if (!BASS_WASAPI_Start())
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("BASS_WASAPI_Start() failed (%i)!", BASS_ErrorGetCode()));
		return false;
	}

	BASS_WASAPI_INFO wasapiInfo;
	BASS_WASAPI_GetInfo(&wasapiInfo);

	g_wasapiOutputMixer = BASS_Mixer_StreamCreate(wasapiInfo.freq, wasapiInfo.chans, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_NONSTOP);

	if (g_wasapiOutputMixer == 0)
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("BASS_Mixer_StreamCreate() failed (%i)!", BASS_ErrorGetCode()));
		return false;
	}
#endif
	return true;
}

bool BassSoundEngine::playChannelWithAttributes(BassSound *bassSound, SOUNDHANDLE handle, float pan, float pitch)
{
	pan = std::clamp<float>(pan, -1.0f, 1.0f);
	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	BASS_ChannelSetAttribute(handle, BASS_ATTRIB_PAN, pan);

	if (bassSound->isStream())
	{
		if (bassSound->isLooped())
			BASS_ChannelFlags(handle, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
	}
	else
	{
		BASS_ChannelSetAttribute(handle, BASS_ATTRIB_NORAMP, 1.0f); // see https://github.com/ppy/osu-framework/pull/3146
	}

	// HACKHACK: force add to output mixer
	if (BASS_Mixer_ChannelGetMixer(handle) == 0)
	{
		if (!BASS_Mixer_StreamAddChannel(g_wasapiOutputMixer, handle,
		                                 (!bassSound->isStream() ? BASS_STREAM_AUTOFREE : 0) | BASS_MIXER_DOWNMIX | BASS_MIXER_NORAMPIN))
			BassManager::printBassError(fmt::format("BASS_Mixer_StreamAddChannel({}, {}, {})", g_wasapiOutputMixer, handle,
			                                        (!bassSound->isStream() ? BASS_STREAM_AUTOFREE : 0) | BASS_MIXER_DOWNMIX | BASS_MIXER_NORAMPIN),
			                            BASS_ErrorGetCode());
	}

	if (BASS_ChannelIsActive(handle) != BASS_ACTIVE_PLAYING)
	{
		const bool ret = BASS_ChannelPlay(handle, TRUE);
		if (!ret)
			BassManager::printBassError(fmt::format("BASS_ChannelPlay({}, TRUE)", handle), BASS_ErrorGetCode());
		return ret;
	}

	bassSound->setLastPlayTime(engine->getTime());

	return true;
#else
	if (BASS_ChannelIsActive(handle) != BASS_ACTIVE_PLAYING)
	{
		BASS_ChannelSetAttribute(handle, BASS_ATTRIB_PAN, pan);

		if (pitch != 1.0f)
		{
			float freq = snd_freq.getFloat();
			BASS_ChannelGetAttribute(handle, BASS_ATTRIB_FREQ, &freq);

			const float semitonesShift = std::lerp(-60.0f, 60.0f, pitch / 2.0f);
			BASS_ChannelSetAttribute(handle, BASS_ATTRIB_FREQ, std::pow(2.0f, (semitonesShift / 12.0f)) * freq);
		}

		if (bassSound->isStream())
		{
			if (bassSound->isLooped())
				BASS_ChannelFlags(handle, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
		}
		else
		{
			BASS_ChannelSetAttribute(handle, BASS_ATTRIB_NORAMP, 1.0f); // see https://github.com/ppy/osu-framework/pull/3146
		}

		bool ret = false;
		{
			ret = BASS_ChannelPlay(handle, FALSE);
			auto code = BASS_ErrorGetCode();
			if (!ret)
				BassManager::printBassError(fmt::format("BASS_ChannelPlay({}, TRUE)", handle), code);
			if (code == BASS_ERROR_START)
			{
				debugLog("Attempting to reinitialize the audio device...\n");
				restart();
				ret = m_bReady;
			}
		}

		if (ret)
			bassSound->setLastPlayTime(engine->getTime());

		return ret;
	}
#endif

	return false;
}

#endif // MCENGINE_FEATURE_BASS
