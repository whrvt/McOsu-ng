//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		handles sounds using SoLoud library
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudSoundEngine.h"
#include "SoLoudManager.h"
#include "SoLoudSound.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"

extern ConVar snd_output_device;
extern ConVar snd_restart;
extern ConVar snd_freq;
extern ConVar snd_restrict_play_frame;
extern ConVar snd_change_check_interval;
extern ConVar snd_speed_compensate_pitch;
extern ConVar debug_snd;

// SoLoud-specific ConVars
ConVar snd_soloud_buffer("snd_soloud_buffer", SoLoud::Soloud::AUTO, FCVAR_NONE, "SoLoud audio device buffer size");
ConVar snd_soloud_backend("snd_soloud_backend", SoLoud::Soloud::MINIAUDIO, FCVAR_NONE, "SoLoud backend (0=auto, 1=SDL2, 2=MiniAudio, 3=WASAPI, etc.)");

SoLoudSoundEngine::SoLoudSoundEngine() : SoundEngine()
{
	m_slManager = new SL();
	m_iCurrentOutputDevice = -1;
	m_sCurrentOutputDevice = "Default";

	OUTPUT_DEVICE defaultOutputDevice;
	defaultOutputDevice.id = -1;
	defaultOutputDevice.name = "Default";
	defaultOutputDevice.enabled = true;
	defaultOutputDevice.isDefault = true;

	snd_output_device.setValue(defaultOutputDevice.name);
	m_outputDevices.push_back(defaultOutputDevice);

	// other output devices (TODO: bogus for now)
	updateOutputDevices(false, true);

	initializeOutputDevice(defaultOutputDevice.id);

	// convar callbacks
	snd_freq.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
	snd_restart.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
	snd_output_device.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::setOutputDevice));
	snd_soloud_backend.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
}

SoLoudSoundEngine::~SoLoudSoundEngine()
{
	if (m_bReady)
		SL::deinit();
	SAFE_DELETE(m_slManager);
}

void SoLoudSoundEngine::restart()
{
	setOutputDeviceForce(m_sCurrentOutputDevice);

	// callback (reload sound buffers etc.)
	if (m_outputDeviceChangeCallback != nullptr)
		m_outputDeviceChangeCallback();
}

void SoLoudSoundEngine::update()
{
	// check for device changes if interval is enabled
	float checkInterval = snd_change_check_interval.getFloat();
	if (checkInterval > 0.0f)
	{
		auto currentTime = Timing::getTimeReal<float>();
		if (currentTime - m_fPrevOutputDeviceChangeCheckTime > checkInterval)
		{
			m_fPrevOutputDeviceChangeCheckTime = currentTime;
			updateOutputDevices(true, false);
		}
	}
}

bool SoLoudSoundEngine::play(Sound *snd, float pan, float pitch)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return false;

	SoLoudSound *soloudSound = snd->getSound();
	if (!soloudSound)
		return false;

	if (soloudSound->m_handle != 0 && SL::getPause(soloudSound->m_handle))
	{
		// just unpause if paused
		SL::setPause(soloudSound->m_handle, false);
		return true;
	}

	return playSound(soloudSound, pan, pitch);
}

bool SoLoudSoundEngine::playSound(SoLoudSound *soloudSound, float pan, float pitch, bool is3d, Vector3 *pos)
{
	if (!soloudSound)
		return false;

	pan = std::clamp<float>(pan, -1.0f, 1.0f);
	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

	// check if we should allow playing this frame (for overlayable sounds)
	const bool allowPlayFrame = !soloudSound->isOverlayable() || !snd_restrict_play_frame.getBool() || Timing::getTimeReal() > soloudSound->getLastPlayTime();
	if (!allowPlayFrame)
		return false;

	auto restorePos = 0.0; // position in the track to potentially restore to

	// if the sound is already playing and not overlayable, stop it
	if (soloudSound->m_handle != 0 && !soloudSound->isOverlayable())
	{
		restorePos = SL::getStreamPosition(soloudSound->m_handle);
		SL::stop(soloudSound->m_handle);
		soloudSound->m_handle = 0;
	}

	if (debug_snd.getBool())
	{
		debugLog("SoLoudSoundEngine: Playing %s (stream=%d, 3d=%d) with speed=%f, pitch=%f\n", soloudSound->m_sFilePath.toUtf8(), soloudSound->m_bStream ? 1 : 0, is3d ? 1 : 0,
		         soloudSound->m_speed, pitch);
	}

	// play the sound with appropriate method
	unsigned int handle = 0;

	if (is3d && pos)
	{
		// 3D playback - always use direct audio source for 3D
		handle = play3dSound(soloudSound, *pos, soloudSound->m_fVolume);
	}
	else if (soloudSound->m_bStream)
	{
		// streaming audio (music) - always use SoundTouch filter
		handle = playSoundWithFilter(soloudSound, pan, soloudSound->m_fVolume);
		if (handle)
			SL::setProtectVoice(handle, true); // protect the music channel (don't let it get interrupted when many sounds play back at once)
		                                       // NOTE: this doesn't seem to work properly, not sure why... need to set setMaxActiveVoiceCount as a workaround,
		                                       // otherwise stuff like buzzsliders can cause glitches in music playback
	}
	else
	{
		// non-streaming audio (sound effects) - always use direct playback
		// speed/pitch changes are applied via SoLoud's native mechanisms
		float finalPitch = pitch;

		// combine all pitch modifiers
		if (soloudSound->m_pitch != 1.0f)
			finalPitch *= soloudSound->m_pitch;

		// if speed compensation is disabled, apply speed as pitch
		if (!snd_speed_compensate_pitch.getBool() && soloudSound->m_speed != 1.0f)
			finalPitch *= soloudSound->m_speed;

		handle = playDirectSound(soloudSound, pan, finalPitch, soloudSound->m_fVolume);
	}

	// finalize playback
	if (handle != 0)
	{
		if (restorePos != 0.0)
			SL::seek(handle, restorePos); // restore the position to where we were pre-pause

		// store the handle and mark playback time
		soloudSound->m_handle = handle;
		soloudSound->setLastPlayTime(Timing::getTimeReal());

		// get the actual sample rate for the file from SoLoud
		float actualFreq = SL::getSamplerate(handle);
		if (actualFreq > 0)
			soloudSound->m_frequency = actualFreq;

		return true;
	}

	if (debug_snd.getBool())
		debugLog("SoLoudSoundEngine: Failed to play sound %s\n", soloudSound->m_sFilePath.toUtf8());

	return false;
}

unsigned int SoLoudSoundEngine::playSoundWithFilter(SoLoudSound *soloudSound, float pan, float volume)
{
	if (!soloudSound || !soloudSound->m_audioSource)
		return 0;

	SoLoud::SoundTouchFilter *filter = soloudSound->getFilterInstance();
	if (!filter)
		return 0;

	// make sure filter parameters are up to date (TODO: refactor, this is probably redundant)
	soloudSound->updateFilterParameters();

	// play through the filter
	unsigned int handle = SL::play(*filter, volume);

	if (handle != 0)
	{
		SL::setPan(handle, pan);
		if (debug_snd.getBool())
			debugLog("SoLoudSoundEngine: Playing through SoundTouch filter with speed=%f, pitch=%f\n", soloudSound->m_speed, soloudSound->m_pitch);
	}

	return handle;
}

unsigned int SoLoudSoundEngine::playDirectSound(SoLoudSound *soloudSound, float pan, float pitch, float volume)
{
	if (!soloudSound || !soloudSound->m_audioSource)
		return 0;

	// play directly
	unsigned int handle = SL::play(*soloudSound->m_audioSource, volume);

	if (handle != 0)
	{
		SL::setPan(handle, pan);

		// set relative play speed (affects both pitch and speed)
		if (pitch != 1.0f)
			SL::setRelativePlaySpeed(handle, pitch);
	}

	return handle;
}

unsigned int SoLoudSoundEngine::play3dSound(SoLoudSound *soloudSound, Vector3 pos, float volume)
{
	if (!soloudSound || !soloudSound->m_audioSource)
		return 0;

	unsigned int handle = SL::play3d(*soloudSound->m_audioSource, pos.x, pos.y, pos.z, 0, 0, 0, volume);

	return handle;
}

bool SoLoudSoundEngine::play3d(Sound *snd, Vector3 pos)
{
	if (!m_bReady || snd == NULL || !snd->isReady() || !snd->is3d())
		return false;

	SoLoudSound *soloudSound = snd->getSound();
	if (!soloudSound)
		return false;

	return playSound(soloudSound, 0.0f, 1.0f, true, &pos);
}

void SoLoudSoundEngine::pause(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	SoLoudSound *soloudSound = snd->getSound();
	if (!soloudSound || soloudSound->m_handle == 0)
		return;

	SL::setPause(soloudSound->m_handle, true);
	soloudSound->setLastPlayTime(0.0);
}

void SoLoudSoundEngine::stop(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	SoLoudSound *soloudSound = snd->getSound();
	if (!soloudSound || soloudSound->m_handle == 0)
		return;

	SL::stop(soloudSound->m_handle);
	soloudSound->m_handle = 0;
	soloudSound->setPosition(0.0);
	soloudSound->setLastPlayTime(0.0);
}

void SoLoudSoundEngine::setOutputDevice(UString outputDeviceName)
{
	for (size_t i = 0; i < m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].name == outputDeviceName)
		{
			if (m_outputDevices[i].id != m_iCurrentOutputDevice)
			{
				// FIXME: bogus, no handling for separate audio devices at least with the miniaudio backend
				int previousOutputDevice = m_iCurrentOutputDevice;
				if (!initializeOutputDevice(m_outputDevices[i].id))
					initializeOutputDevice(previousOutputDevice);
			}
			else
				debugLog("\"%s\" already is the current device.\n", outputDeviceName.toUtf8());

			return;
		}
	}

	debugLog("couldn't find output device \"%s\"!\n", outputDeviceName.toUtf8());
}

void SoLoudSoundEngine::setOutputDeviceForce(UString outputDeviceName)
{
	for (size_t i = 0; i < m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].name == outputDeviceName)
		{
			int previousOutputDevice = m_iCurrentOutputDevice;
			if (!initializeOutputDevice(m_outputDevices[i].id))
				initializeOutputDevice(previousOutputDevice);
			return;
		}
	}

	debugLog("couldn't find output device \"%s\"!\n", outputDeviceName.toUtf8());
}

void SoLoudSoundEngine::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);
	SL::setGlobalVolume(m_fVolume);
}

void SoLoudSoundEngine::set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp)
{
	if (!m_bReady)
		return;

	// set listener position
	SL::set3dListenerPosition(headPos.x, headPos.y, headPos.z);

	// set listener orientation (at and up vectors)
	Vector3 at = headPos + viewDir; // "at" point = position + direction
	SL::set3dListenerAt(at.x, at.y, at.z);
	SL::set3dListenerUp(viewUp.x, viewUp.y, viewUp.z);

	SL::update3dAudio();
}

std::vector<UString> SoLoudSoundEngine::getOutputDevices()
{
	std::vector<UString> outputDevices;

	for (size_t i = 0; i < m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].enabled)
			outputDevices.push_back(m_outputDevices[i].name);
	}

	return outputDevices;
}

void SoLoudSoundEngine::updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo)
{
	// SoLoud doesn't provide direct device enumeration
	if (printInfo)
	{
		constexpr const char *const backendNames[] = {"Auto", "SDL1",   "SDL2",      "PortAudio", "WinMM",         "XAudio2",   "WASAPI",  "ALSA",       "JACK",
		                                              "OSS",  "OpenAL", "CoreAudio", "OpenSL ES", "Vita Homebrew", "miniaudio", "Nosound", "Nulldriver", "Unknown"};
		debugLog("SoundEngine: Device 0 = \"Default\", enabled = 1, default = 1\n");

		auto backend = snd_soloud_backend.getVal<SoLoud::Soloud::BACKENDS>();
		if (backend < 0 || backend > SoLoud::Soloud::BACKEND_MAX)
			backend = snd_soloud_backend.getDefaultVal<SoLoud::Soloud::BACKENDS>();
		debugLog("SoundEngine: Using SoLoud backend: %s\n", backendNames[backend]);
	}
}

bool SoLoudSoundEngine::initializeOutputDevice(int id, bool force)
{
	debugLog("SoundEngine: initializeOutputDevice(%i) ...\n", id);

	m_iCurrentOutputDevice = id;

	// cleanup potential previous device
	if (m_bReady)
	{
		SL::deinit();
		m_bReady = false;
	}

	// basic flags
	unsigned int flags = SoLoud::Soloud::CLIP_ROUNDOFF;

	unsigned int backend = snd_soloud_backend.getInt();
	if (backend < 0 || backend >= SoLoud::Soloud::BACKEND_MAX)
		backend = SoLoud::Soloud::MINIAUDIO;

	unsigned int sampleRate = snd_freq.getInt();
	if (sampleRate <= 0)
		sampleRate = SoLoud::Soloud::AUTO;

	unsigned int bufferSize = snd_soloud_buffer.getInt();
	if (bufferSize < 0)
		bufferSize = SoLoud::Soloud::AUTO;

	// use stereo output
	const unsigned int channels = 2;

	// initialize SoLoud through the manager
	SoLoud::result result = SL::init(flags, backend, sampleRate, bufferSize, channels);

	if (result != SoLoud::SO_NO_ERROR)
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("SoLoud::Soloud::init() failed (%i)!", result));
		return false;
	}

	result = SL::setMaxActiveVoiceCount(255);
	if (result != SoLoud::SO_NO_ERROR)
		debugLog("SoundEngine WARNING: failed to setMaxActiveVoiceCount (%i)\n", result);

	// set current device name (bogus)
	for (auto & m_outputDevice : m_outputDevices)
	{
		if (m_outputDevice.id == id)
		{
			m_sCurrentOutputDevice = m_outputDevice.name;
			break;
		}
	}

	debugLog("SoundEngine: Initialized SoLoud with output device = \"%s\" flags: 0x%x, backend: %u, sampleRate: %u, bufferSize: %u, channels: %u, maxActiveVoiceCount: %u\n",
	         m_sCurrentOutputDevice.toUtf8(), flags, backend, sampleRate, bufferSize, channels, SL::getMaxActiveVoiceCount());

	m_bReady = true;

	// init global volume
	setVolume(m_fVolume);

	return true;
}

#endif // MCENGINE_FEATURE_SOLOUD
