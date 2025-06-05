//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		handles sounds using SDL_mixer library
//
// $NoKeywords: $snd $sdl
//===============================================================================//

#include "SDLSoundEngine.h"
#include "SDLSound.h"

#if defined(MCENGINE_FEATURE_SDL_MIXER)

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"

#include "Thread.h"

// SDL-specific ConVars
namespace cv {
ConVar snd_chunk_size("snd_chunk_size", 256, FCVAR_NONE, "used to set the SDL audio chunk size");
}

SDLSoundEngine::SDLSoundEngine() : SoundEngine()
{
	m_iMixChunkSize = cv::snd_chunk_size.getInt();
	m_fVolumeMixMusic = 1.0f;

	// add default output device
	m_iCurrentOutputDevice = -1;
	m_sCurrentOutputDevice = "NULL";

	OUTPUT_DEVICE defaultOutputDevice;
	defaultOutputDevice.id = -1;
	defaultOutputDevice.name = "Default";
	defaultOutputDevice.enabled = true;
	defaultOutputDevice.isDefault = false; // custom -1 can never have default

	cv::snd_output_device.setValue(defaultOutputDevice.name);
	m_outputDevices.push_back(defaultOutputDevice);

	if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_Init(SDL_INIT_AUDIO))
	{
		engine->showMessageErrorFatal("Fatal Sound Error", UString::format("Couldn't initialize SDL audio subsystem!\nSDL error: %s\n", SDL_GetError()));
		engine->shutdown();
		return;
	}

	initializeOutputDevice(defaultOutputDevice.id);

	// convar callbacks
	cv::snd_restart.setCallback(fastdelegate::MakeDelegate(this, &SDLSoundEngine::restart));
	cv::snd_output_device.setCallback(fastdelegate::MakeDelegate(this, &SDLSoundEngine::setOutputDevice));
}

SDLSoundEngine::~SDLSoundEngine()
{
	if (m_bReady)
		Mix_CloseAudio();

	Mix_Quit();
}

void SDLSoundEngine::restart()
{
	setOutputDeviceForce(m_sCurrentOutputDevice);

	// callback (reload sound buffers etc.)
	if (m_outputDeviceChangeCallback != nullptr)
		m_outputDeviceChangeCallback();
}

void SDLSoundEngine::update()
{
	// currently not implemented for SDL
}

bool SDLSoundEngine::play(Sound *snd, float pan, float pitch)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return false;

	auto *sdlSound = snd->as<SDLSound>();
	if (!sdlSound)
		return false;

	pan = std::clamp<float>(pan, -1.0f, 1.0f);
	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

	const bool allowPlayFrame = !snd->isOverlayable() || !cv::snd_restrict_play_frame.getBool() || engine->getTime() > snd->getLastPlayTime();

	if (!allowPlayFrame)
		return false;

	if (sdlSound->isStream() && Mix_PlayingMusic() && Mix_PausedMusic())
	{
		Mix_ResumeMusic();
		return true;
	}
	else
	{
		// special case: looped sounds are not supported for sdl/mixer, so do not let them kill other channels
		if (sdlSound->isLooped())
			return false;

		int channel = (sdlSound->isStream() ? (Mix_PlayMusic((Mix_Music *)sdlSound->getMixChunkOrMixMusic(), 1))
		                                    : Mix_PlayChannel(-1, (Mix_Chunk *)sdlSound->getMixChunkOrMixMusic(), 0));

		// allow overriding (oldest channel gets reused)
		if (!sdlSound->isStream() && channel < 0)
		{
			const int oldestChannel = Mix_GroupOldest(1);
			if (oldestChannel > -1)
			{
				Mix_HaltChannel(oldestChannel);
				channel = Mix_PlayChannel(-1, (Mix_Chunk *)sdlSound->getMixChunkOrMixMusic(), 0);
			}
		}

		const bool ret = (channel > -1);

		if (!ret)
			debugLog(sdlSound->isStream() ? "couldn't Mix_PlayMusic(), error on {:s}!\n" : "couldn't Mix_PlayChannel(), error on {:s}!\n",
			         sdlSound->getFilePath().toUtf8());
		else
		{
			sdlSound->setHandle(channel);
			sdlSound->setPan(pan);
			sdlSound->setLastPlayTime(engine->getTime());
		}

		return ret;
	}
}

bool SDLSoundEngine::play3d(Sound *snd, Vector3 pos)
{
	if (!m_bReady || snd == NULL || !snd->isReady() || !snd->is3d())
		return false;

	return play(snd);
}

void SDLSoundEngine::pause(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	auto *sdlSound = snd->as<SDLSound>();
	if (!sdlSound)
		return;

	if (sdlSound->isStream())
		Mix_PauseMusic();
	else if (!sdlSound->isLooped()) // special case: looped sounds are not supported for sdl/mixer, so do not let them kill other channels
		Mix_Pause(sdlSound->getHandle());
}

void SDLSoundEngine::stop(Sound *snd)
{
	if (!m_bReady || snd == NULL || !snd->isReady())
		return;

	auto *sdlSound = snd->as<SDLSound>();
	if (!sdlSound)
		return;

	if (sdlSound->isStream())
		Mix_HaltMusic();
	else if (!sdlSound->isLooped()) // special case: looped sounds are not supported here, so do not let them kill other channels
		Mix_HaltChannel(sdlSound->getHandle());
}

void SDLSoundEngine::setOutputDevice(UString outputDeviceName)
{
	initializeOutputDevice(-1);
}

void SDLSoundEngine::setOutputDeviceForce(UString outputDeviceName)
{
	initializeOutputDevice(-1);
}

void SDLSoundEngine::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	Mix_VolumeMusic((int)(m_fVolume * m_fVolumeMixMusic * MIX_MAX_VOLUME));
	Mix_Volume(-1, (int)(m_fVolume * MIX_MAX_VOLUME));
}

void SDLSoundEngine::set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp)
{
	// currently not implemented for SDL
}

std::vector<UString> SDLSoundEngine::getOutputDevices()
{
	std::vector<UString> outputDevices;

	for (size_t i = 0; i < m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].enabled)
			outputDevices.push_back(m_outputDevices[i].name);
	}

	return outputDevices;
}

void SDLSoundEngine::updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo)
{
	// currently not implemented for SDL
}

bool SDLSoundEngine::initializeOutputDevice(int id, bool force)
{
	// cleanup potential previous device
	if (m_bReady)
		Mix_CloseAudio();

	m_iMixChunkSize = cv::snd_chunk_size.getInt();
	const char *chunkSizeHint = UString::format("%d", m_iMixChunkSize).toUtf8();

	const int freq = cv::snd_freq.getInt();
	const int channels = 16;

	debugLog("setting SDL audio chunk size to {:s}\n", chunkSizeHint);
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, chunkSizeHint);

	const SDL_AudioSpec spec = {.format = MIX_DEFAULT_FORMAT, .channels = MIX_DEFAULT_CHANNELS, .freq = freq};

	if (!Mix_OpenAudio(0, &spec))
	{
		const char *error = SDL_GetError();
		debugLog("SoundEngine: Couldn't Mix_OpenAudio(): {:s}\n", error);
		engine->showMessageError("Sound Error", UString::format("Couldn't Mix_OpenAudio(): %s", error));
		return false;
	}

	const int numAllocatedChannels = Mix_AllocateChannels(channels);
	debugLog("SoundEngine: Allocated {} channels\n", numAllocatedChannels);

	// tag all channels to allow overriding in play()
	Mix_GroupChannels(0, numAllocatedChannels - 1, 1);

	m_bReady = true;

	return true;
}

#endif // defined(MCENGINE_FEATURE_SDL_MIXER)
