//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		sound wrapper base class
//
// $NoKeywords: $snd
//===============================================================================//

#include "Sound.h"
#include "ConVar.h"

#include "BassSound.h"
#include "BassSound2.h"
#include "SDLSound.h"
#include "SoLoudSound.h"

#include "File.h"
#include "ResourceManager.h"

ConVar debug_snd("debug_snd", false, FCVAR_NONE);
ConVar snd_speed_compensate_pitch("snd_speed_compensate_pitch", true, FCVAR_NONE, "automatically keep pitch constant if speed changes");
ConVar snd_play_interp_duration("snd_play_interp_duration", 0.75f, FCVAR_NONE,
                                "smooth over freshly started channel position jitter with engine time over this duration in seconds");
ConVar snd_play_interp_ratio("snd_play_interp_ratio", 0.50f, FCVAR_NONE,
                             "percentage of snd_play_interp_duration to use 100% engine time over audio time (some devices report 0 for very long)");
ConVar snd_file_min_size(
    "snd_file_min_size", 51, FCVAR_NONE,
    "minimum file size in bytes for WAV files to be considered valid (everything below will fail to load), this is a workaround for BASS crashes");

Sound::Sound(UString filepath, bool stream, bool threeD, bool loop, bool prescan) : Resource(filepath)
{
	m_bStream = stream;
	m_bIs3d = threeD;
	m_bIsLooped = loop;
	m_bPrescan = prescan;
	m_bIsOverlayable = false;

	m_bIgnored = true; // check for audio file validity

	m_fVolume = 1.0f;
	m_fLastPlayTime = -1.0f;
}

Sound::~Sound() = default;

void Sound::initAsync()
{
	if (ResourceManager::debug_rm->getBool())
		debugLog("Resource Manager: Loading {:s}\n", m_sFilePath);

	// sanity check for malformed audio files
	UString fileExtensionLowerCase = env->getFileExtensionFromFilePath(m_sFilePath);
	fileExtensionLowerCase.lowerCase();

	if (!m_sFilePath.isEmpty() && !fileExtensionLowerCase.isEmpty())
		m_bIgnored = !isValidAudioFile(m_sFilePath, fileExtensionLowerCase);
	else if (debug_snd.getBool())
		debugLog("Sound: Ignoring malformed/corrupt .{:s} file {:s}\n", fileExtensionLowerCase, m_sFilePath);
}

Sound *Sound::createSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan)
{
#ifdef MCENGINE_FEATURE_BASS
	return new BassSound(filepath, stream, threeD, loop, prescan);
#elif defined(MCENGINE_FEATURE_SDL_MIXER)
	return new SDLSound(filepath, stream, threeD, loop, prescan);
#elif defined(MCENGINE_FEATURE_SOLOUD)
	return new SoLoudSound(filepath, stream, threeD, loop, prescan);
#else
#error No sound backend available!
#endif
}

bool Sound::isValidAudioFile(const UString &filePath, const UString &fileExt)
{
	McFile audFile(filePath, McFile::TYPE::READ);

	if (!audFile.canRead())
		return false;

	size_t fileSize = audFile.getFileSize();

	if (fileExt == "wav")
	{
		// need at least 44 bytes for basic header
		if (fileSize < 44)
			return false;

		// check some headers, "RIFF" + 4 bytes size + "WAVE"
		const char *data = audFile.readFile();
		if (data != nullptr)
			return (memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0);
		return false;
	}
	else if (fileExt == "mp3")
	{
		// need at least 64 bytes for basic header
		if (fileSize < 64)
			return false;

		// check for ID3v2 tag or MP3 sync bytes
		const char *data = audFile.readFile();
		if (data != nullptr)
		{
			// ID3v2 tag
			if (memcmp(data, "ID3", 3) == 0)
				return true;

			// MP3 sync, 0xFF followed by 0xFB/0xFA/0xF9/0xF8
			if ((unsigned char)data[0] == 0xFF && ((unsigned char)data[1] & 0xF8) == 0xF8)
				return true;
		}
		return false;
	}
	else if (fileExt == "ogg")
	{
		// need at least 64 bytes for page header + some content
		if (fileSize < 64)
			return false;

		// find OGG magic number
		const char *data = audFile.readFile();
		if (data != nullptr)
			return memcmp(data, "OggS", 4) == 0;
		return false;
	}

	return false; // don't let unsupported formats be read
}
