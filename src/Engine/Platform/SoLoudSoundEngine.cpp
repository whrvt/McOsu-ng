//========== Copyright (c) 2025, WH, All rights reserved. ============//
//
// Purpose:		handles sounds using SoLoud library
//
// $NoKeywords: $snd $soloud
//===============================================================================//

#include "SoLoudSoundEngine.h"
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

ConVar snd_soloud_buffer("snd_soloud_buffer", SoLoud::Soloud::AUTO, FCVAR_NONE, "SoLoud audio device buffer size");
ConVar snd_soloud_backend("snd_soloud_backend", SoLoud::Soloud::MINIAUDIO, FCVAR_NONE, "SoLoud backend (highly likely you don't need to touch this)");

SoLoudSoundEngine::SoLoudSoundEngine() : SoundEngine()
{
	// add default output device
	m_iCurrentOutputDevice = -1;
	m_sCurrentOutputDevice = "Default";

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
	snd_freq.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
	snd_restart.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
	snd_output_device.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::setOutputDevice));
	snd_soloud_backend.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
}

SoLoudSoundEngine::~SoLoudSoundEngine()
{
	if (m_bReady)
	{
		m_engine.deinit();
	}
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
	// SoLoud handles updates internally, nothing to do here
}

bool SoLoudSoundEngine::play(Sound *snd, float pan, float pitch)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return false;

	// try casting to SoLoudSound
	SoLoudSound *soloudSound = dynamic_cast<SoLoudSound *>(snd);
	if (!soloudSound)
		return false;

	pan = clamp<float>(pan, -1.0f, 1.0f);
	pitch = clamp<float>(pitch, 0.0f, 2.0f);

	const bool allowPlayFrame = !snd->isOverlayable() || !snd_restrict_play_frame.getBool() || engine->getTime() > snd->getLastPlayTime();

	if (!allowPlayFrame)
		return false;

	// if the sound is already playing and not overlayable, stop it
	if (soloudSound->m_handle != 0 && !soloudSound->isOverlayable())
	{
		stopSound(soloudSound->m_handle);
	}

	// play the sound
	unsigned int handle = m_engine.play(*soloudSound->m_audioSource, soloudSound->m_fVolume);

	if (handle != 0)
	{
		// apply pan
		setPanSound(handle, pan);

		// get the actual sample rate from SoLoud
		float actualFreq = m_engine.getSamplerate(handle);
		if (actualFreq > 0)
		{
			soloudSound->m_frequency = actualFreq;
		}

		// apply pitch (only if different from default)
		if (pitch != 1.0f)
		{
			const float semitonesShift = lerp<float>(-60.0f, 60.0f, pitch / 2.0f);
			float freq = soloudSound->m_frequency;
			setSampleRateSound(handle, freq * std::pow(2.0f, (semitonesShift / 12.0f)));
		}

		// store the handle
		soloudSound->m_handle = handle;
		soloudSound->setLastPlayTime(engine->getTime());

		return true;
	}

	return false;
}

bool SoLoudSoundEngine::play3d(Sound *snd, Vector3 pos)
{
	if (!m_bReady || snd == NULL || !snd->isReady() || !snd->is3d())
		return false;

	// try casting to SoLoudSound
	SoLoudSound *soloudSound = dynamic_cast<SoLoudSound *>(snd);
	if (!soloudSound)
		return false;

	if (!snd_restrict_play_frame.getBool() || engine->getTime() > snd->getLastPlayTime())
	{
		// if the sound is already playing and not overlayable, stop it
		if (soloudSound->m_handle != 0 && !soloudSound->isOverlayable())
		{
			stopSound(soloudSound->m_handle);
		}

		// play the sound in 3D
		unsigned int handle = m_engine.play3d(*soloudSound->m_audioSource, pos.x, pos.y, pos.z, 0, 0, 0, soloudSound->m_fVolume);

		if (handle != 0)
		{
			soloudSound->m_handle = handle;
			soloudSound->setLastPlayTime(engine->getTime());

			return true;
		}
	}

	return false;
}

void SoLoudSoundEngine::pause(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	// try casting to SoLoudSound
	SoLoudSound *soloudSound = dynamic_cast<SoLoudSound *>(snd);
	if (!soloudSound)
		return;

	if (soloudSound->m_handle != 0)
	{
		setPauseSound(soloudSound->m_handle, true);
		soloudSound->setLastPlayTime(0.0);
	}
}

void SoLoudSoundEngine::stop(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	// try casting to SoLoudSound
	SoLoudSound *soloudSound = dynamic_cast<SoLoudSound *>(snd);
	if (!soloudSound)
		return;

	if (soloudSound->m_handle != 0)
	{
		stopSound(soloudSound->m_handle);
		soloudSound->m_handle = 0;
		soloudSound->setPosition(0.0);
		soloudSound->setLastPlayTime(0.0);
	}
}

void SoLoudSoundEngine::setOutputDevice(UString outputDeviceName)
{
	for (size_t i = 0; i < m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].name == outputDeviceName)
		{
			if (m_outputDevices[i].id != m_iCurrentOutputDevice)
			{
				int previousOutputDevice = m_iCurrentOutputDevice;
				if (!initializeOutputDevice(m_outputDevices[i].id))
					initializeOutputDevice(previousOutputDevice); // if something went wrong, automatically switch back to the previous device
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
				initializeOutputDevice(previousOutputDevice); // if something went wrong, automatically switch back to the previous device
			return;
		}
	}

	debugLog("couldn't find output device \"%s\"!\n", outputDeviceName.toUtf8());
}

void SoLoudSoundEngine::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = clamp<float>(volume, 0.0f, 1.0f);
	m_engine.setGlobalVolume(m_fVolume);
}

void SoLoudSoundEngine::set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp)
{
	if (!m_bReady)
		return;

	// set listener position
	m_engine.set3dListenerPosition(headPos.x, headPos.y, headPos.z);

	// set listener orientation (at and up vectors)
	Vector3 at = headPos + viewDir; // "at" point = position + direction
	m_engine.set3dListenerAt(at.x, at.y, at.z);
	m_engine.set3dListenerUp(viewUp.x, viewUp.y, viewUp.z);

	// apply changes
	m_engine.update3dAudio();
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
	// SoLoud doesn't provide direct device enumeration, could try OS-specific shenanigans but nah, just use the default
	if (printInfo)
	{
		debugLog("SoundEngine: Device 0 = \"Default\", enabled = 1, default = 1\n");
		debugLog("SoundEngine: Using SoLoud backend\n");
	}
}

bool SoLoudSoundEngine::initializeOutputDevice(int id)
{
	debugLog("SoundEngine: initializeOutputDevice(%i) ...\n", id);

	m_iCurrentOutputDevice = id;

	// cleanup potential previous device
	if (m_bReady)
	{
		m_engine.deinit();
	}

	// basic flags
	unsigned int flags = SoLoud::Soloud::CLIP_ROUNDOFF;

	unsigned int backend = snd_soloud_backend.getInt();
	if (backend < 0)
		backend = SoLoud::Soloud::MINIAUDIO;

	unsigned int sampleRate = snd_freq.getInt();
	if (!sampleRate || sampleRate < 0)
		sampleRate = SoLoud::Soloud::AUTO;

	unsigned int bufferSize = snd_soloud_buffer.getInt();
	if (!bufferSize || bufferSize < 0)
		bufferSize = SoLoud::Soloud::AUTO;

	const unsigned int channels = 2; // stereo

	SoLoud::result result = m_engine.init(flags, backend, sampleRate, bufferSize, channels);

	if (result != SoLoud::SO_NO_ERROR)
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("SoLoud::Soloud::init() failed (%i)!", result));
		return false;
	}

	m_bReady = true;

	for (size_t i = 0; i < m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].id == id)
		{
			m_sCurrentOutputDevice = m_outputDevices[i].name;
			break;
		}
	}

	debugLog("SoundEngine: Initialized SoLoud with output device = \"%s\" flags: 0x%x, backend: %u, sampleRate: %u, bufferSize: %u, channels: %u\n",
	         m_sCurrentOutputDevice.toUtf8(), flags, backend, sampleRate, bufferSize, channels);

	// set the global volume
	setVolume(m_fVolume);

	return true;
}

// SoLoud-specific accessors implementation
void SoLoudSoundEngine::stopSound(unsigned int handle)
{
	if (m_bReady)
		m_engine.stop(handle);
}

void SoLoudSoundEngine::seekSound(unsigned int handle, double positionSeconds)
{
	if (m_bReady)
		m_engine.seek(handle, positionSeconds);
}

void SoLoudSoundEngine::setPauseSound(unsigned int handle, bool pause)
{
	if (m_bReady)
		m_engine.setPause(handle, pause);
}

void SoLoudSoundEngine::setVolumeSound(unsigned int handle, float volume)
{
	if (m_bReady)
		m_engine.setVolume(handle, volume);
}

void SoLoudSoundEngine::setRelativePlaySpeedSound(unsigned int handle, float speed)
{
	if (m_bReady)
		m_engine.setRelativePlaySpeed(handle, speed);
}

void SoLoudSoundEngine::setSampleRateSound(unsigned int handle, float sampleRate)
{
	if (m_bReady)
		m_engine.setSamplerate(handle, sampleRate);
}

void SoLoudSoundEngine::setPanSound(unsigned int handle, float pan)
{
	if (m_bReady)
		m_engine.setPan(handle, pan);
}

void SoLoudSoundEngine::setLoopingSound(unsigned int handle, bool loop)
{
	if (m_bReady)
		m_engine.setLooping(handle, loop);
}

float SoLoudSoundEngine::getStreamPositionSound(unsigned int handle)
{
	if (m_bReady)
		return m_engine.getStreamPosition(handle);
	return 0.0f;
}

float SoLoudSoundEngine::getSampleRateSound(unsigned int handle)
{
	if (m_bReady)
		return m_engine.getSamplerate(handle);
	return 44100.0f;
}

bool SoLoudSoundEngine::isValidVoiceHandleSound(unsigned int handle)
{
	if (m_bReady)
		return m_engine.isValidVoiceHandle(handle);
	return false;
}

bool SoLoudSoundEngine::getPauseSound(unsigned int handle)
{
	if (m_bReady)
		return m_engine.getPause(handle);
	return false;
}

#endif // MCENGINE_FEATURE_SOLOUD
