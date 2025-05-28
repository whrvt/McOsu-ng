//======= Copyright (c) 2014, PG, 2024, CW, 2025, WH All rights reserved. =======//
//
// Purpose:		handles sounds using BASS library
//
// $NoKeywords: $snd $bass
//===============================================================================//

#include "BassSoundEngine2.h"

#if defined(MCENGINE_FEATURE_BASS) && defined(MCENGINE_NEOSU_BASS_PORT_FINISHED)
#include "BassSound2.h"

#include "BassManager.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"

extern ConVar snd_output_device;
extern ConVar snd_restart;
extern ConVar snd_freq;
extern ConVar snd_restrict_play_frame;
extern ConVar snd_change_check_interval;
extern ConVar win_snd_fallback_dsound;
extern ConVar debug_snd;

#if __has_include("Osu.h") // :^)
extern ConVar osu_universal_offset_hardcoded;
#endif

ConVar snd_updateperiod("snd_updateperiod", 5, FCVAR_NONE, "BASS_CONFIG_UPDATEPERIOD length in milliseconds, minimum is 5");
ConVar snd_buffer("snd_buffer", 100, FCVAR_NONE, "BASS_CONFIG_BUFFER length in milliseconds, minimum is 1 above snd_updateperiod");
ConVar snd_dev_period("snd_dev_period", 5, FCVAR_NONE, "BASS_CONFIG_DEV_PERIOD length in milliseconds, or if negative then in samples");
ConVar snd_dev_buffer("snd_dev_buffer", 10, FCVAR_NONE, "BASS_CONFIG_DEV_BUFFER length in milliseconds");
ConVar snd_ready_delay("snd_ready_delay", 0.0f, FCVAR_NONE,
                          "after a sound engine restart, wait this many seconds before marking it as ready");
#ifdef MCENGINE_PLATFORM_WINDOWS
void _WIN_SND_WASAPI_BUFFER_SIZE_CHANGE(UString oldValue, UString newValue);
void _WIN_SND_WASAPI_PERIOD_SIZE_CHANGE(UString oldValue, UString newValue);
void _WIN_SND_WASAPI_EXCLUSIVE_CHANGE(UString oldValue, UString newValue);

ConVar snd_asio_buffer_size("snd_asio_buffer_size", -1, FCVAR_NONE, "buffer size in samples (usually 44100 samples per second)");
ConVar win_snd_wasapi_buffer_size("win_snd_wasapi_buffer_size", 0.011f, FCVAR_NONE,
                                  "buffer size/length in seconds (e.g. 0.011 = 11 ms), directly responsible for audio delay and crackling",
                                  _WIN_SND_WASAPI_BUFFER_SIZE_CHANGE);
ConVar win_snd_wasapi_period_size("win_snd_wasapi_period_size", 0.0f, FCVAR_NONE,
                                  "interval between OutputWasapiProc calls in seconds (e.g. 0.016 = 16 ms) (0 = use default)", _WIN_SND_WASAPI_PERIOD_SIZE_CHANGE);
ConVar win_snd_wasapi_exclusive("win_snd_wasapi_exclusive", true, FCVAR_NONE, "whether to use exclusive device mode to further reduce latency",
                                _WIN_SND_WASAPI_EXCLUSIVE_CHANGE);
ConVar win_snd_wasapi_shared_volume_affects_device("win_snd_wasapi_shared_volume_affects_device", false, FCVAR_NONE,
                                                   "if in shared mode, whether to affect device volume globally or use separate session volume (default)");

void _WIN_SND_WASAPI_BUFFER_SIZE_CHANGE(UString oldValue, UString newValue)
{
	const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
	const int newValueMS = std::round(newValue.toFloat() * 1000.0f);

	if (oldValueMS != newValueMS)
		soundEngine->setOutputDeviceForce(soundEngine->getOutputDevice()); // force restart
}

void _WIN_SND_WASAPI_PERIOD_SIZE_CHANGE(UString oldValue, UString newValue)
{
	const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
	const int newValueMS = std::round(newValue.toFloat() * 1000.0f);

	if (oldValueMS != newValueMS)
		soundEngine->setOutputDeviceForce(soundEngine->getOutputDevice()); // force restart
}

void _WIN_SND_WASAPI_EXCLUSIVE_CHANGE(UString oldValue, UString newValue)
{
	const bool oldValueBool = oldValue.toInt();
	const bool newValueBool = newValue.toInt();

	if (oldValueBool != newValueBool)
		soundEngine->setOutputDeviceForce(soundEngine->getOutputDevice()); // force restart
}

DWORD BassSoundEngine2::ASIO_clamp(BASS_ASIO_INFO info, DWORD buflen)
{
	if (buflen == -1)
		return info.bufpref;
	if (buflen < info.bufmin)
		return info.bufmin;
	if (buflen > info.bufmax)
		return info.bufmax;
	if (info.bufgran == 0)
		return buflen;

	if (info.bufgran == -1)
	{
		// Buffer lengths are only allowed in powers of 2
		for (int oksize = info.bufmin; oksize <= info.bufmax; oksize *= 2)
		{
			if (oksize == buflen)
			{
				return buflen;
			}
			else if (oksize > buflen)
			{
				oksize /= 2;
				return oksize;
			}
		}

		// Unreachable
		return info.bufpref;
	}
	else
	{
		// Buffer lengths are only allowed in multiples of info.bufgran
		buflen -= info.bufmin;
		buflen = (buflen / info.bufgran) * info.bufgran;
		buflen += info.bufmin;
		return buflen;
	}
}

bool BassSoundEngine2::hasExclusiveOutput()
{
	return isASIO() || (isWASAPI() && win_snd_wasapi_exclusive.getBool());
}

#else

bool BassSoundEngine2::hasExclusiveOutput()
{
	return false;
}

#endif

BassSoundEngine2::BassSoundEngine2() : SoundEngine()
{
	if (!BassManager::init())
	{
		engine->showMessageErrorFatal("Fatal Sound Error", "Failed to load BASS libraries!\nContinuing, but expect a crash.");
		engine->shutdown();
		return;
	}

	auto bass_version = BASS_GetVersion();
	debugLog("SoundEngine: BASS version = {:08x}\n", bass_version);
	if (HIWORD(bass_version) != BASSVERSION)
	{
		engine->showMessageErrorFatal("Fatal Sound Error", "An incorrect version of the BASS library file was loaded!");
		engine->shutdown();
		return;
	}

	auto mixer_version = BASS_Mixer_GetVersion();
	debugLog("SoundEngine: BASSMIX version = {:08x}\n", mixer_version);
	if (HIWORD(mixer_version) != BASSVERSION)
	{
		engine->showMessageErrorFatal("Fatal Sound Error", "An incorrect version of the BASSMIX library file was loaded!");
		engine->shutdown();
		return;
	}

#ifdef MCENGINE_PLATFORM_WINDOWS
	auto asio_version = BASS_ASIO_GetVersion();
	debugLog("SoundEngine: BASSASIO version = {:08x}\n", asio_version);
	if (HIWORD(asio_version) != BASSASIOVERSION)
	{
		engine->showMessageErrorFatal("Fatal Sound Error", "An incorrect version of the BASSASIO library file was loaded!");
		engine->shutdown();
		return;
	}

	auto wasapi_version = BASS_WASAPI_GetVersion();
	debugLog("SoundEngine: BASSWASAPI version = {:08x}\n", wasapi_version);
	if (HIWORD(wasapi_version) != BASSVERSION)
	{
		engine->showMessageErrorFatal("Fatal Sound Error", "An incorrect version of the BASSWASAPI library file was loaded!");
		engine->shutdown();
		return;
	}
#endif

	// apply default global settings
	BASS_SetConfig(BASS_CONFIG_BUFFER, static_cast<DWORD>(snd_buffer.getDefaultFloat()));
	BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, static_cast<DWORD>(snd_dev_buffer.getDefaultFloat())); // NOTE: only used by new osu atm
	BASS_SetConfig(BASS_CONFIG_MP3_OLDGAPS,
	               1); // NOTE: only used by osu atm (all beatmaps timed to non-iTunesSMPB + 529 sample deletion offsets on old dlls pre 2015)
	BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP,
	               1); // NOTE: only used by osu atm (avoids lag/jitter in BASS_ChannelGetPosition() shortly after a BASS_ChannelPlay() after loading/silence)

	BASS_SetConfig(BASS_CONFIG_VISTA_TRUEPOS, 0); // NOTE: if set to 1, increases sample playback latency +10 ms
	BASS_SetConfig(BASS_CONFIG_DEV_TIMEOUT, 0);   // prevents playback from ever stopping due to device issues

	updateOutputDevices(true);

    m_currentOutputDevice = {
        .id = -1,
        .enabled = true,
        .isDefault = true,
        .name = "Default",
        .driver = OutputDriver::BASS,
    };

	m_bReady = initializeOutputDeviceStruct(m_currentOutputDevice);

	snd_freq.setCallback(fastdelegate::MakeDelegate(this, &BassSoundEngine2::onFreqChanged));
	snd_restart.setCallback(fastdelegate::MakeDelegate(this, &BassSoundEngine2::restart));
	//snd_output_device.setCallback(fastdelegate::MakeDelegate(this, &BassSoundEngine2::setOutputDeviceForce));
}

BassSoundEngine2::~BassSoundEngine2()
{
	bassfree();

	BassManager::cleanup();
}

void BassSoundEngine2::bassfree()
{
	if (m_currentOutputDevice.driver == OutputDriver::BASS)
	{
		BASS_SetDevice(m_currentOutputDevice.id);
		BASS_Free();
		BASS_SetDevice(0);
	}

#ifdef MCENGINE_PLATFORM_WINDOWS
	if (m_currentOutputDevice.driver == OutputDriver::BASS_ASIO)
	{
		BASS_ASIO_Free();
	}
	else if (m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI)
	{
		BASS_WASAPI_Free();
	}
#endif

	g_bassOutputMixer = 0;
	BASS_Free(); // free "No sound" device
}

BassSoundEngine2::OUTPUT_DEVICE_BASS BassSoundEngine2::getWantedDevice()
{
	auto wanted_name = snd_output_device.getString();
	for (auto device : m_vOutputDevices)
	{
		if (device.enabled && device.name == wanted_name)
		{
			return device;
		}
	}

	debugLog("Could not find sound device '{}', initializing default one instead.\n", wanted_name.toUtf8());
	return getDefaultDevice();
}

BassSoundEngine2::OUTPUT_DEVICE_BASS BassSoundEngine2::getDefaultDevice()
{
	for (auto device : m_vOutputDevices)
	{
		if (device.enabled && device.isDefault)
		{
			return device;
		}
	}

	debugLog("Could not find a working sound device!\n");
	return {
	    .id = 0,
	    .enabled = true,
	    .isDefault = true,
	    .name = "No sound",
	    .driver = OutputDriver::NONE,
	};
}

void BassSoundEngine2::updateOutputDevices(bool printInfo)
{
	m_vOutputDevices.clear();

	BASS_DEVICEINFO deviceInfo;
	for (int d = 0; (BASS_GetDeviceInfo(d, &deviceInfo) == true); d++)
	{
		const bool isEnabled = (deviceInfo.flags & BASS_DEVICE_ENABLED);
		const bool isDefault = (deviceInfo.flags & BASS_DEVICE_DEFAULT);

		OUTPUT_DEVICE_BASS soundDevice;
		soundDevice.id = d;
		soundDevice.name = deviceInfo.name;
		soundDevice.enabled = isEnabled;
		soundDevice.isDefault = isDefault;

		// avoid duplicate names
		int duplicateNameCounter = 2;
		while (true)
		{
			bool foundDuplicateName = false;
			for (size_t i = 0; i < m_vOutputDevices.size(); i++)
			{
				if (m_vOutputDevices[i].name == soundDevice.name)
				{
					foundDuplicateName = true;

					soundDevice.name = deviceInfo.name;
					soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));

					duplicateNameCounter++;

					break;
				}
			}

			if (!foundDuplicateName)
				break;
		}

		soundDevice.driver = OutputDriver::BASS;
		m_vOutputDevices.push_back(soundDevice);

		debugLog("DSOUND: Device {} = \"{:s}\", enabled = {}, default = {}\n", d, deviceInfo.name, (int)isEnabled, (int)isDefault);
	}

#ifdef MCENGINE_PLATFORM_WINDOWS
	BASS_ASIO_DEVICEINFO asioDeviceInfo;
	for (int d = 0; (BASS_ASIO_GetDeviceInfo(d, &asioDeviceInfo) == true); d++)
	{
		OUTPUT_DEVICE_BASS soundDevice;
		soundDevice.id = d;
		soundDevice.name = asioDeviceInfo.name;
		soundDevice.enabled = true;
		soundDevice.isDefault = false;

		// avoid duplicate names
		int duplicateNameCounter = 2;
		while (true)
		{
			bool foundDuplicateName = false;
			for (size_t i = 0; i < m_vOutputDevices.size(); i++)
			{
				if (m_vOutputDevices[i].name == soundDevice.name)
				{
					foundDuplicateName = true;

					soundDevice.name = deviceInfo.name;
					soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));

					duplicateNameCounter++;

					break;
				}
			}

			if (!foundDuplicateName)
				break;
		}

		soundDevice.driver = OutputDriver::BASS_ASIO;
		soundDevice.name.append(" [ASIO]");
		m_vOutputDevices.push_back(soundDevice);

		debugLog("ASIO: Device {} = \"{:s}\"\n", d, asioDeviceInfo.name);
	}

	BASS_WASAPI_DEVICEINFO wasapiDeviceInfo;
	for (int d = 0; (BASS_WASAPI_GetDeviceInfo(d, &wasapiDeviceInfo) == true); d++)
	{
		const bool isEnabled = (wasapiDeviceInfo.flags & BASS_DEVICE_ENABLED);
		const bool isDefault = (wasapiDeviceInfo.flags & BASS_DEVICE_DEFAULT);
		const bool isInput = (wasapiDeviceInfo.flags & BASS_DEVICE_INPUT);
		if (isInput)
			continue;

		OUTPUT_DEVICE_BASS soundDevice;
		soundDevice.id = d;
		soundDevice.name = wasapiDeviceInfo.name;
		soundDevice.enabled = isEnabled;
		soundDevice.isDefault = isDefault;

		// avoid duplicate names
		int duplicateNameCounter = 2;
		while (true)
		{
			bool foundDuplicateName = false;
			for (size_t i = 0; i < m_vOutputDevices.size(); i++)
			{
				if (m_vOutputDevices[i].name == soundDevice.name)
				{
					foundDuplicateName = true;

					soundDevice.name = wasapiDeviceInfo.name;
					soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));

					duplicateNameCounter++;

					break;
				}
			}

			if (!foundDuplicateName)
				break;
		}

		soundDevice.driver = OutputDriver::BASS_WASAPI;
		soundDevice.name.append(" [WASAPI]");
		m_vOutputDevices.push_back(soundDevice);

		debugLog("WASAPI: Device {} = \"{}\", enabled = {}, default = {}\n", d, wasapiDeviceInfo.name, (int)isEnabled, (int)isDefault);
	}
#endif
}

// The BASS mixer is used for every sound driver, but it's useful to be able to
// initialize it later on some drivers where we know the best available frequency.
bool BassSoundEngine2::init_bass_mixer(OUTPUT_DEVICE_BASS device)
{
	auto bass_flags = BASS_DEVICE_STEREO | BASS_DEVICE_FREQ | BASS_DEVICE_NOSPEAKER;
	auto freq = snd_freq.getInt();

	// We initialize a "No sound" device for measuring loudness and mixing sounds,
	// regardless of the device we'll use for actual output.
	if (!BASS_Init(0, freq, bass_flags | BASS_DEVICE_SOFTWARE, NULL, NULL))
	{
		auto code = BASS_ErrorGetCode();
		if (code != BASS_ERROR_ALREADY)
		{
			debugLog("BASS_Init(0) failed.\n");
			display_bass_error();
			return false;
		}
	}

	if (device.driver == OutputDriver::BASS)
	{
		if (!BASS_Init(device.id, freq, bass_flags | BASS_DEVICE_SOFTWARE, NULL, NULL))
		{
			debugLog("BASS_Init({}) errored out.\n", device.id);
			display_bass_error();
			return false;
		}
	}

	auto mixer_flags = BASS_SAMPLE_FLOAT | BASS_MIXER_NONSTOP | BASS_MIXER_RESUME;
	if (device.driver != OutputDriver::BASS)
		mixer_flags |= BASS_STREAM_DECODE | BASS_MIXER_POSEX;
	g_bassOutputMixer = BASS_Mixer_StreamCreate(freq, 2, mixer_flags);
	if (g_bassOutputMixer == 0)
	{
		debugLog("BASS_Mixer_StreamCreate() failed.\n");
		display_bass_error();
		return false;
	}

	// Disable buffering to lower latency on regular BASS output
	// This has no effect on ASIO/WASAPI since for those the mixer is a decode stream
	BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_BUFFER, 0.f);

	// Switch to "No sound" device for all future sound processing
	// Only g_bassOutputMixer will be output to the actual device!
	BASS_SetDevice(0);
	return true;
}

bool BassSoundEngine2::initializeOutputDeviceStruct(OUTPUT_DEVICE_BASS device)
{
	debugLog("SoundEngine: initializeOutputDevice( {} ) ...\n", device.name);

	bassfree();
#if __has_include("Osu.h") // :^)
	// We compensate for latency via BASS_ATTRIB_MIXER_LATENCY
	osu_universal_offset_hardcoded.setValue(0.f);
#endif

	// if (device.driver == OutputDriver::NONE || (device.driver == OutputDriver::BASS && device.id == 0))
	// {
	// 	m_currentOutputDevice = device;
	// 	snd_output_device.setValue(m_currentOutputDevice.name);
	// 	debugLog("SoundEngine: Output Device = \"{}\"\n", m_currentOutputDevice.name);

	// 	// if (osu && osu->optionsMenu)
	// 	// {
	// 	// 	osu->optionsMenu->updateLayout();
	// 	// }

	// 	return true;
	// }

	if (device.driver == OutputDriver::BASS)
		BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 1);
	else
		BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

	// allow users to override some defaults (but which may cause beatmap desyncs)
	// we only want to set these if their values have been explicitly modified (to avoid sideeffects in the default
	// case, and for my sanity)
	{
		if (snd_dev_buffer.getFloat() != snd_dev_buffer.getDefaultFloat())
		{
			BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, snd_dev_buffer.getInt());
		}
		if (snd_dev_period.getFloat() != snd_dev_period.getDefaultFloat())
		{
			BASS_SetConfig(BASS_CONFIG_DEV_PERIOD, snd_dev_period.getInt());
		}
		if (!(device.driver == OutputDriver::BASS_ASIO || device.driver == OutputDriver::BASS_WASAPI) &&
		    (snd_updateperiod.getFloat() != snd_updateperiod.getDefaultFloat()))
		{
			// ASIO/WASAPI: let driver decide when to render playback buffer
			const auto clamped = static_cast<DWORD>(std::clamp(snd_updateperiod.getFloat(), 5.0f, 100.0f));
			BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, clamped);
			snd_updateperiod.setValue(clamped);
		}
		else
		{
			BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, static_cast<DWORD>(snd_updateperiod.getDefaultFloat()));
		}
		if (snd_buffer.getFloat() != snd_buffer.getDefaultFloat())
		{
			const auto clamped = static_cast<DWORD>(std::clamp(snd_buffer.getFloat(), snd_updateperiod.getFloat() + 1.0f, 5000.0f));
			BASS_SetConfig(BASS_CONFIG_BUFFER, clamped);
			snd_buffer.setValue(clamped);
		}
	}

	// When the driver is BASS, we can init the mixer immediately
	// On other drivers, we'd rather get the sound card's frequency first
	if (device.driver == OutputDriver::BASS)
	{
		if (!init_bass_mixer(device))
		{
			return false;
		}
	}

#ifdef MCENGINE_PLATFORM_WINDOWS
	if (device.driver == OutputDriver::BASS_ASIO)
	{
		if (!BASS_ASIO_Init(device.id, 0))
		{
			debugLog("BASS_ASIO_Init() failed.\n");
			display_bass_error();
			return false;
		}

		double sample_rate = BASS_ASIO_GetRate();
		if (sample_rate == 0.0)
		{
			sample_rate = snd_freq.getFloat();
			debugLog("ASIO: BASS_ASIO_GetRate() returned 0, using {} instead!\n", sample_rate);
		}
		else
		{
			snd_freq.setValue(sample_rate);
		}
		if (!init_bass_mixer(device))
		{
			return false;
		}

		BASS_ASIO_INFO info = {{0}};
		BASS_ASIO_GetInfo(&info);
		auto bufsize = snd_asio_buffer_size.getInt();
		bufsize = ASIO_clamp(info, bufsize);

		// if (osu && osu->optionsMenu)
		// {
		// 	auto slider = osu->optionsMenu->asioBufferSizeSlider;
		// 	slider->setBounds(info.bufmin, info.bufmax);
		// 	slider->setKeyDelta(info.bufgran == -1 ? info.bufmin : info.bufgran);
		// }

		if (!BASS_ASIO_ChannelEnableBASS(false, 0, g_bassOutputMixer, true))
		{
			debugLog("BASS_ASIO_ChannelEnableBASS() failed.\n");
			display_bass_error();
			return false;
		}

		if (!BASS_ASIO_Start(bufsize, 0))
		{
			debugLog("BASS_ASIO_Start() failed.\n");
			display_bass_error();
			return false;
		}

		double wanted_latency = 1000.0 * snd_asio_buffer_size.getFloat() / sample_rate;
		double actual_latency = 1000.0 * (double)BASS_ASIO_GetLatency(false) / sample_rate;
		BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_MIXER_LATENCY, actual_latency / 1000.0);
		debugLog("ASIO: wanted {} ms, got {} ms latency. Sample rate: {} Hz\n", wanted_latency, actual_latency, sample_rate);
	}

	if (device.driver == OutputDriver::BASS_WASAPI)
	{
		const float bufferSize = std::round(win_snd_wasapi_buffer_size.getFloat() * 1000.0f) / 1000.0f;   // in seconds
		const float updatePeriod = std::round(win_snd_wasapi_period_size.getFloat() * 1000.0f) / 1000.0f; // in seconds

		BASS_WASAPI_DEVICEINFO info;
		if (!BASS_WASAPI_GetDeviceInfo(device.id, &info))
		{
			debugLog("WASAPI: Failed to get device info\n");
			return false;
		}
		snd_freq.setValue(info.mixfreq);
		if (!init_bass_mixer(device))
		{
			return false;
		}

		// BASS_MIXER_NONSTOP prevents some sound cards from going to sleep when there is no output
		auto flags = BASS_MIXER_NONSTOP;

#ifdef MCENGINE_PLATFORM_WINDOWS
		// BASS_WASAPI_RAW ignores sound "enhancements" that some sound cards offer (adds latency)
		// It is only available on Windows 8.1 or above
		flags |= BASS_WASAPI_RAW;
#endif

		if (win_snd_wasapi_exclusive.getBool())
		{
			// BASS_WASAPI_EXCLUSIVE makes neosu have exclusive output to the sound card
			// BASS_WASAPI_AUTOFORMAT chooses the best matching sample format, BASSWASAPI doesn't resample in exclusive
			// mode
			flags |= BASS_WASAPI_EXCLUSIVE | BASS_WASAPI_AUTOFORMAT;
		}

		if (!BASS_WASAPI_Init(device.id, 0, 0, flags, bufferSize, updatePeriod, WASAPIPROC_BASS, (void *)g_bassOutputMixer))
		{
			debugLog("BASS_WASAPI_Init() failed.\n");
			display_bass_error();
			return false;
		}

		if (!BASS_WASAPI_Start())
		{
			debugLog("BASS_WASAPI_Start() failed.\n");
			display_bass_error();
			return false;
		}

		BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_MIXER_LATENCY, win_snd_wasapi_buffer_size.getFloat());
	}
#endif

	m_currentOutputDevice = device;
	snd_output_device.setValue(m_currentOutputDevice.name);
	debugLog("SoundEngine: Output Device = \"{}\"\n", m_currentOutputDevice.name.toUtf8());

	// if (osu && osu->optionsMenu)
	// {
	// 	osu->optionsMenu->updateLayout();
	// }

	return m_bReady = true;
}

void BassSoundEngine2::restart()
{
	setOutputDevice(m_currentOutputDevice);
}

void BassSoundEngine2::update() {}

bool BassSoundEngine2::play(Sound *snd, float pan, float pitch)
{
	if (!m_bReady)
		debugLog("tried to play, but was not ready!\n");
	if (!m_bReady || snd == NULL || !snd->isReady())
		return false;
	BassSound2 *bassSound = snd->getSound();
	if (!bassSound)
		return false;

	if (!bassSound->isOverlayable() && bassSound->isPlaying())
	{
		return false;
	}

	if (bassSound->isOverlayable() && snd_restrict_play_frame.getBool())
	{
		if (engine->getTime() <= bassSound->getLastPlayTime())
		{
			return false;
		}
	}

	HCHANNEL channel = bassSound->getChannel();
	if (channel == 0)
	{
		debugLog("BassSoundEngine2::play() failed to get channel, errorcode {}\n", BASS_ErrorGetCode());
		return false;
	}

	pan = std::clamp<float>(pan, -1.0f, 1.0f);
	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);
	BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, bassSound->m_fVolume);
	BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, pan);
	BASS_ChannelSetAttribute(channel, BASS_ATTRIB_NORAMP, bassSound->isStream() ? 0 : 1);
	if (pitch != 1.0f)
	{
		float freq = snd_freq.getFloat();
		BASS_ChannelGetAttribute(channel, BASS_ATTRIB_FREQ, &freq);

		const float semitonesShift = std::lerp(-60.0f, 60.0f, pitch / 2.0f);
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_FREQ, std::pow(2.0f, (semitonesShift / 12.0f)) * freq);
	}

	BASS_ChannelFlags(channel, bassSound->isLooped() ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);

	if (BASS_Mixer_ChannelGetMixer(channel) != 0)
		return false;

	auto flags = BASS_MIXER_DOWNMIX | BASS_MIXER_NORAMPIN | BASS_STREAM_AUTOFREE;
	if (!BASS_Mixer_StreamAddChannel(g_bassOutputMixer, channel, flags))
	{
		debugLog("BASS_Mixer_StreamAddChannel() failed ({})!\n", BASS_ErrorGetCode());
		return false;
	}

	// Make sure the mixer is playing! Duh.
	if (m_currentOutputDevice.driver == OutputDriver::BASS)
	{
		if (BASS_ChannelIsActive(g_bassOutputMixer) != BASS_ACTIVE_PLAYING)
		{
			if (!BASS_ChannelPlay(g_bassOutputMixer, true))
			{
				debugLog("BassSoundEngine2::play() couldn't BASS_ChannelPlay(), errorcode {}\n", BASS_ErrorGetCode());
				return false;
			}
		}
	}

	bassSound->m_bStarted = true;
	bassSound->m_dChannelCreationTime = engine->getTime();
	if (bassSound->m_bPaused)
	{
		bassSound->m_bPaused = false;
		bassSound->m_fLastPlayTime = bassSound->m_dChannelCreationTime - ((static_cast<double>(bassSound->m_iPausePositionMS)) / 1000.0);
	}
	else
	{
		bassSound->m_fLastPlayTime = bassSound->m_dChannelCreationTime;
	}

	if (debug_snd.getBool())
	{
		debugLog("Playing {:s}\n", bassSound->getFilePath());
	}

	return true;
}

void BassSoundEngine2::pause(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;
	BassSound2 *bassSound = snd->getSound();
	if (!bassSound)
		return;

	if (!bassSound->isStream())
	{
		engine->showMessageError("Programmer Error", "Called pause on a sample!");
		return;
	}
	if (!bassSound->isPlaying())
		return;

	auto pan = bassSound->getPan();
	auto pos = bassSound->getPositionMS();
	auto loop = bassSound->isLooped();
	auto speed = bassSound->getSpeed();

	// Calling BASS_Mixer_ChannelRemove automatically frees the stream due
	// to BASS_STREAM_AUTOFREE. We need to reinitialize it.
	bassSound->reload();

	bassSound->setPositionMS(pos);
	bassSound->setSpeed(speed);
	bassSound->setPan(pan);
	bassSound->setLoop(loop);
	bassSound->m_bPaused = true;
	bassSound->m_iPausePositionMS = pos;
}

void BassSoundEngine2::stop(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	// This will stop all samples, then re-init to be ready for a play()
	snd->reload();
}

void BassSoundEngine2::setOutputDevice(BassSoundEngine2::OUTPUT_DEVICE_BASS device)
{
	bool was_playing = false;
	unsigned long prevMusicPositionMS = 0;
	// if (osu->getSelectedBeatmap()->getMusic() != NULL)
	// {
	// 	was_playing = osu->getSelectedBeatmap()->getMusic()->isPlaying();
	// 	prevMusicPositionMS = osu->getSelectedBeatmap()->getMusic()->getPositionMS();
	// }

	// TODO: This is blocking main thread, can freeze for a long time on some sound cards
	auto previous = m_currentOutputDevice;
	if (!initializeOutputDeviceStruct(device))
	{
		if ((device.id == previous.id && device.driver == previous.driver) || !initializeOutputDeviceStruct(previous))
		{
			// We failed to reinitialize the device, don't start an infinite loop, just give up
			m_currentOutputDevice = {
			    .id = 0,
			    .enabled = true,
			    .isDefault = true,
			    .name = "No sound",
			    .driver = OutputDriver::NONE,
			};
		}
	}

	// osu->optionsMenu->outputDeviceLabel->setText(getOutputDeviceName());
	// osu->getSkin()->reloadSounds();
	// osu->optionsMenu->onOutputDeviceResetUpdate();

	// // start playing music again after audio device changed
	// if (osu->getSelectedBeatmap()->getMusic() != NULL)
	// {
	// 	if (osu->isInPlayMode())
	// 	{
	// 		osu->getSelectedBeatmap()->unloadMusic();
	// 		osu->getSelectedBeatmap()->loadMusic(false);
	// 		osu->getSelectedBeatmap()->getMusic()->setLoop(false);
	// 		osu->getSelectedBeatmap()->getMusic()->setPositionMS(prevMusicPositionMS);
	// 	}
	// 	else
	// 	{
	// 		osu->getSelectedBeatmap()->unloadMusic();
	// 		osu->getSelectedBeatmap()->select();
	// 		osu->getSelectedBeatmap()->getMusic()->setPositionMS(prevMusicPositionMS);
	// 	}
	// }

	// if (was_playing)
	// {
	// 	osu->music_unpause_scheduled = true;
	// }
}

void BassSoundEngine2::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);
	if (m_currentOutputDevice.driver == OutputDriver::BASS_ASIO)
	{
#ifdef MCENGINE_PLATFORM_WINDOWS
		BASS_ASIO_ChannelSetVolume(false, 0, m_fVolume);
		BASS_ASIO_ChannelSetVolume(false, 1, m_fVolume);
#endif
	}
	else if (m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI)
	{
#ifdef MCENGINE_PLATFORM_WINDOWS
		if (hasExclusiveOutput())
		{
			// Device volume doesn't seem to work, so we'll use DSP instead
			BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_VOLDSP, m_fVolume);
		}
		else
		{
			BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS | BASS_WASAPI_VOL_SESSION, m_fVolume);
		}
#endif
	}
	else
	{
		// 0 (silent) - 10000 (full).
		BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, (DWORD)(m_fVolume * 10000));
		BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, (DWORD)(m_fVolume * 10000));
		BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, (DWORD)(m_fVolume * 10000));
	}
}

void BassSoundEngine2::onFreqChanged(UString oldValue, UString newValue)
{
	(void)oldValue;
	(void)newValue;
	if (!m_bReady)
		return;
	restart();
}

std::vector<BassSoundEngine2::OUTPUT_DEVICE_BASS> BassSoundEngine2::getOutputDevicesStruct()
{
	std::vector<OUTPUT_DEVICE_BASS> outputDevices;

	for (size_t i = 0; i < m_vOutputDevices.size(); i++)
	{
		if (m_vOutputDevices[i].enabled)
		{
			outputDevices.push_back(m_vOutputDevices[i]);
		}
	}

	return outputDevices;
}

#endif
