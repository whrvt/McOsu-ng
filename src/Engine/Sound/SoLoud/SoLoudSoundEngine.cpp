//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		handles sounds using SoLoud library
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudSoundEngine.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include <soloud/soloud.h>

#include "SoLoudSound.h"

#include "ConVar.h"
#include "Engine.h"

#include "Environment.h"
#include <utility>

// SoLoud-specific ConVars
namespace cv
{
ConVar snd_soloud_buffer("snd_soloud_buffer", SoLoud::Soloud::AUTO, FCVAR_NONE, "SoLoud audio device buffer size (recommended to leave this on 0/auto)");
ConVar snd_soloud_backend("snd_soloud_backend", "MiniAudio", FCVAR_NONE, R"(SoLoud backend, "MiniAudio" or "SDL3" (MiniAudio is default))");
ConVar snd_sanity_simultaneous_limit("snd_sanity_simultaneous_limit", 128, FCVAR_NONE,
                                     "The maximum number of overlayable sounds that are allowed to be active at once");
} // namespace cv

std::unique_ptr<SoLoud::Soloud> SoLoudSoundEngine::s_SLInstance = nullptr;
SoLoud::Soloud *soloud = nullptr;

SoLoudSoundEngine::SoLoudSoundEngine()
    : SoundEngine()
{
	if (!s_SLInstance)
	{
		s_SLInstance = std::make_unique<SoLoud::Soloud>();
		soloud = s_SLInstance.get();
	}

	cv::snd_freq.setValue(SoLoud::Soloud::AUTO); // let it be auto-negotiated (the snd_freq callback will adjust if needed, if this is manually set in a config)
	cv::snd_freq.setDefaultFloat(SoLoud::Soloud::AUTO);

	m_iMaxActiveVoices = std::clamp<int>(cv::snd_sanity_simultaneous_limit.getInt(), 64,
	                                     255); // TODO: lower this minimum (it will crash if more than this many sounds play at once...)

	m_iCurrentOutputDevice = -1;
	m_sCurrentOutputDevice = "Default";

	OUTPUT_DEVICE defaultOutputDevice;
	defaultOutputDevice.id = -1;
	defaultOutputDevice.name = "Default";
	defaultOutputDevice.enabled = true;
	defaultOutputDevice.isDefault = true;

	cv::snd_output_device.setValue(defaultOutputDevice.name);
	m_outputDevices.push_back(defaultOutputDevice);

	// other output devices (TODO: bogus for now)
	updateOutputDevices(false, true);

	initializeOutputDevice(defaultOutputDevice.id);

	// convar callbacks
	cv::snd_freq.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
	cv::snd_restart.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
	cv::snd_output_device.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::setOutputDevice));
	cv::snd_soloud_backend.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::restart));
	cv::snd_sanity_simultaneous_limit.setCallback(fastdelegate::MakeDelegate(this, &SoLoudSoundEngine::onMaxActiveChange));
}

SoLoudSoundEngine::~SoLoudSoundEngine()
{
	if (m_bReady)
		soloud->deinit();
	s_SLInstance.reset();
	soloud = nullptr;
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
	// unused
}

bool SoLoudSoundEngine::play(Sound *snd, float pan, float pitch)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return false;

	auto *soloudSound = snd->as<SoLoudSound>();

	auto handle = soloudSound->getHandle();
	if (handle != 0 && soloud->getPause(handle))
	{
		// just unpause if paused
		soloudSound->setPitch(pitch);
		soloudSound->setPan(pan);
		soloud->setPause(handle, false);
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
	const bool allowPlayFrame = !soloudSound->isOverlayable() || !cv::snd_restrict_play_frame.getBool() || engine->getTime() > soloudSound->getLastPlayTime();
	if (!allowPlayFrame)
		return false;

	auto restorePos = 0.0; // position in the track to potentially restore to

	// if the sound is already playing and not overlayable, stop it
	if (soloudSound->m_handle != 0 && !soloudSound->isOverlayable())
	{
		restorePos = soloudSound->getStreamPositionInSeconds();
		soloud->stop(soloudSound->m_handle);
		soloudSound->m_handle = 0;
	}

	if (cv::debug_snd.getBool())
	{
		debugLog("SoLoudSoundEngine: Playing {:s} (stream={:d}, 3d={:d}) with speed={:f}, pitch={:f}, volume={:f}\n", soloudSound->m_sFilePath.toUtf8(),
		         soloudSound->m_bStream ? 1 : 0, is3d ? 1 : 0, soloudSound->m_speed, pitch, soloudSound->m_fVolume);
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
		// reset these, because they're "sticky" properties
		soloudSound->setPitch(pitch);
		soloudSound->setPan(pan);

		// streaming audio (music) - play SLFXStream directly (it handles SoundTouch internally)
		// start it at 0 volume and fade it in when we play it (to avoid clicks/pops)
		handle = soloud->play(*soloudSound->m_audioSource, 0, pan, true /* paused */);
		if (handle)
			soloud->setProtectVoice(handle,
			                        true); // protect the music channel (don't let it get interrupted when many sounds play back at once)
			                               // NOTE: this doesn't seem to work properly, not sure why... need to setMaxActiveVoiceCount higher than the default 16
			                               // as a workaround, otherwise rapidly overlapping samples like from buzzsliders can cause glitches in music playback
			                               // TODO: a better workaround would be to manually prevent samples from playing if
			                               // it would lead to getMaxActiveVoiceCount() <= getActiveVoiceCount()

		if (cv::debug_snd.getBool() && handle)
			debugLog("SoLoudSoundEngine: Playing streaming audio through SLFXStream with speed={:f}, pitch={:f}\n", soloudSound->m_speed, soloudSound->m_pitch);
	}
	else
	{
		// non-streaming audio (sound effects) - use direct playback with SoLoud's native speed/pitch control
		handle = playDirectSound(soloudSound, pan, pitch, soloudSound->m_fVolume);
	}

	// finalize playback
	if (handle != 0)
	{
		if (restorePos != 0.0)
			soloud->seek(handle, restorePos); // restore the position to where we were pre-pause

		// store the handle and mark playback time
		soloudSound->m_handle = handle;
		soloudSound->setLastPlayTime(engine->getTime());

		soloud->setPause(handle, false); // now, unpause

		if (soloudSound->m_bStream) // fade it in if it's a stream (since we started it with 0 volume)
			setVolumeGradual(handle, soloudSound->m_fVolume);

		return true;
	}

	if (cv::debug_snd.getBool())
		debugLog("SoLoudSoundEngine: Failed to play sound {:s}\n", soloudSound->m_sFilePath.toUtf8());

	return false;
}

unsigned int SoLoudSoundEngine::playDirectSound(SoLoudSound *soloudSound, float pan, float pitch, float volume)
{
	if (!soloudSound || !soloudSound->m_audioSource)
		return 0;

	// calculate final pitch by combining all pitch modifiers
	float finalPitch = pitch;

	// combine with sound's pitch setting
	if (soloudSound->m_pitch != 1.0f)
		finalPitch *= soloudSound->m_pitch;

	// if speed compensation is disabled, apply speed as pitch
	if (!cv::snd_speed_compensate_pitch.getBool() && soloudSound->m_speed != 1.0f)
		finalPitch *= soloudSound->m_speed;

	// play directly
	unsigned int handle = soloud->play(*soloudSound->m_audioSource, volume, pan, true /* paused */);

	if (handle != 0)
	{
		// set relative play speed (affects both pitch and speed)
		if (finalPitch != 1.0f)
			soloud->setRelativePlaySpeed(handle, finalPitch);

		if (cv::debug_snd.getBool())
			debugLog("SoLoudSoundEngine: Playing non-streaming audio with finalPitch={:f} (pitch={:f} * soundPitch={:f} * speedAsPitch={:f})\n", finalPitch, pitch,
			         soloudSound->m_pitch, (!cv::snd_speed_compensate_pitch.getBool() && soloudSound->m_speed != 1.0f) ? soloudSound->m_speed : 1.0f);
	}

	return handle;
}

unsigned int SoLoudSoundEngine::play3dSound(SoLoudSound *soloudSound, Vector3 pos, float volume)
{
	if (!soloudSound || !soloudSound->m_audioSource)
		return 0;

	unsigned int handle = soloud->play3d(*soloudSound->m_audioSource, pos.x, pos.y, pos.z, 0, 0, 0, volume, true /* paused */);

	return handle;
}

bool SoLoudSoundEngine::play3d(Sound *snd, Vector3 pos)
{
	if (!m_bReady || snd == NULL || !snd->isReady() || !snd->is3d())
		return false;

	auto *soloudSound = snd->as<SoLoudSound>();
	if (!soloudSound)
		return false;

	return playSound(soloudSound, 0.0f, 1.0f, true, &pos);
}

void SoLoudSoundEngine::pause(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	auto *soloudSound = snd->as<SoLoudSound>();
	if (!soloudSound || soloudSound->m_handle == 0)
		return;

	soloud->setPause(soloudSound->m_handle, true);
	soloudSound->setLastPlayTime(0.0);
}

void SoLoudSoundEngine::stop(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	auto *soloudSound = snd->as<SoLoudSound>();
	if (!soloudSound || soloudSound->m_handle == 0)
		return;

	soloudSound->setPosition(0.0);
	soloudSound->setLastPlayTime(0.0);
	soloudSound->setFrequency(0.0);
	soloud->stop(soloudSound->m_handle);
	soloudSound->m_handle = 0;
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
				debugLog("\"{:s}\" already is the current device.\n", outputDeviceName.toUtf8());

			return;
		}
	}

	debugLog("couldn't find output device \"{:s}\"!\n", outputDeviceName.toUtf8());
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

	debugLog("couldn't find output device \"{:s}\"!\n", outputDeviceName.toUtf8());
}

void SoLoudSoundEngine::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	// if (cv::debug_snd.getBool())
	// 	debugLog("setting global volume to {:f}\n", m_fVolume);
	soloud->setGlobalVolume(m_fVolume);
}

void SoLoudSoundEngine::setVolumeGradual(unsigned int handle, float targetVol, float fadeTimeMs)
{
	if (!m_bReady || handle == 0 || !soloud->isValidVoiceHandle(handle))
		return;

	soloud->setVolume(handle, 0.0f);

	if (cv::debug_snd.getBool())
		debugLog("fading in to {:.2f}\n", targetVol);

	soloud->fadeVolume(handle, targetVol, fadeTimeMs / 1000.0f);
}

void SoLoudSoundEngine::set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp)
{
	if (!m_bReady)
		return;

	// set listener position
	soloud->set3dListenerPosition(headPos.x, headPos.y, headPos.z);

	// set listener orientation (at and up vectors)
	Vector3 at = headPos + viewDir; // "at" point = position + direction
	soloud->set3dListenerAt(at.x, at.y, at.z);
	soloud->set3dListenerUp(viewUp.x, viewUp.y, viewUp.z);

	soloud->update3dAudio();
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

void SoLoudSoundEngine::updateOutputDevices(bool, bool printInfo)
{
	// SoLoud doesn't provide direct device enumeration
	if (printInfo)
	{
		debugLog("SoundEngine: Device 0 = \"Default\", enabled = 1, default = 1\n");
		debugLog("SoundEngine: Using SoLoud backend: {:s}\n", cv::snd_soloud_backend.getString().toUtf8());
	}
}

bool SoLoudSoundEngine::initializeOutputDevice(int id, bool)
{
	debugLog("SoundEngine: initializeOutputDevice({}) ...\n", id);

	m_iCurrentOutputDevice = id;

	// cleanup potential previous device
	if (m_bReady)
	{
		soloud->deinit();
		m_bReady = false;
	}

	// basic flags
	// roundoff clipping alters/"damages" the waveform, but it sounds weird without it
	auto flags = SoLoud::Soloud::CLIP_ROUNDOFF; /* | SoLoud::Soloud::NO_FPU_REGISTER_CHANGE; */

	auto backend = SoLoud::Soloud::MINIAUDIO;
	const auto &userBackend = cv::snd_soloud_backend.getString();
	if ((userBackend != cv::snd_soloud_backend.getDefaultString()))
	{
		if (userBackend.findIgnoreCase("sdl") != -1)
			backend = SoLoud::Soloud::SDL3;
		else
			backend = SoLoud::Soloud::MINIAUDIO;
	}

	unsigned int sampleRate =
	    (cv::snd_freq.getVal<unsigned int>() == cv::snd_freq.getDefaultVal<unsigned int>() ? SoLoud::Soloud::AUTO : cv::snd_freq.getVal<unsigned int>());
	if (sampleRate <= 0)
		sampleRate = SoLoud::Soloud::AUTO;

	unsigned int bufferSize =
	    (cv::snd_soloud_buffer.getVal<unsigned int>() == cv::snd_soloud_buffer.getDefaultVal<unsigned int>() ? SoLoud::Soloud::AUTO
	                                                                                                         : cv::snd_soloud_buffer.getVal<unsigned int>());
	if (bufferSize < 0)
		bufferSize = SoLoud::Soloud::AUTO;

	// use stereo output
	const unsigned int channels = 2;

	// setup some SDL hints in case the SDL backend is used
	if (bufferSize != SoLoud::Soloud::AUTO)
		SDL_SetHintWithPriority(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, fmt::format("{}", bufferSize).c_str(), SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, PACKAGE_NAME, SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_AUDIO_DEVICE_STREAM_ROLE, "game", SDL_HINT_OVERRIDE);

	// initialize SoLoud through the manager
	SoLoud::result result = soloud->init(flags, backend, sampleRate, bufferSize, channels);

	if (result != SoLoud::SO_NO_ERROR)
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("SoLoud::Soloud::init() failed (%i)!", result));
		return false;
	}

	// it's 0.95 by default, for some reason
	soloud->setPostClipScaler(1.0f);

	cv::snd_freq.setValue(soloud->getBackendSamplerate(), false);       // set the cvar to match the actual output sample rate (without running callbacks)
	cv::snd_soloud_backend.setValue(soloud->getBackendString(), false); // ditto
	if (cv::snd_soloud_buffer.getVal() != cv::snd_soloud_buffer.getDefaultVal())
		cv::snd_soloud_buffer.setValue(soloud->getBackendBufferSize(), false); // ditto (but only if explicitly non-default was requested already)

	onMaxActiveChange(cv::snd_sanity_simultaneous_limit.getFloat());

	// set current device name (bogus)
	for (auto &m_outputDevice : m_outputDevices)
	{
		if (m_outputDevice.id == id)
		{
			m_sCurrentOutputDevice = m_outputDevice.name;
			break;
		}
	}

	debugLog("SoundEngine: Initialized SoLoud with output device = \"{:s}\" flags: 0x{:x}, backend: {:s}, sampleRate: {}, bufferSize: {}, channels: {}, "
	         "maxActiveVoiceCount: {}\n",
	         m_sCurrentOutputDevice.toUtf8(), static_cast<unsigned int>(flags), soloud->getBackendString(), soloud->getBackendSamplerate(),
	         soloud->getBackendBufferSize(), soloud->getBackendChannels(), m_iMaxActiveVoices);

	m_bReady = true;

	// init global volume
	setVolume(m_fVolume);

	return true;
}

void SoLoudSoundEngine::onMaxActiveChange(float newMax)
{
	const auto desired = std::clamp<unsigned int>(static_cast<unsigned int>(newMax), 64, 255);
	if (std::cmp_not_equal(soloud->getMaxActiveVoiceCount(), desired))
	{
		SoLoud::result res = soloud->setMaxActiveVoiceCount(desired);
		if (res != SoLoud::SO_NO_ERROR)
			debugLog("SoundEngine WARNING: failed to setMaxActiveVoiceCount ({})\n", res);
	}
	m_iMaxActiveVoices = static_cast<int>(soloud->getMaxActiveVoiceCount());
	cv::snd_sanity_simultaneous_limit.setValue(m_iMaxActiveVoices, false); // no infinite callback loop
}

#endif // MCENGINE_FEATURE_SOLOUD
