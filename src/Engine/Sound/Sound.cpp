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
#include "SoundEngine.h"
namespace cv
{
ConVar debug_snd("debug_snd", false, FCVAR_NONE);
ConVar snd_speed_compensate_pitch("snd_speed_compensate_pitch", true, FCVAR_NONE, "automatically keep pitch constant if speed changes");
ConVar snd_play_interp_duration("snd_play_interp_duration", 0.75f, FCVAR_NONE,
                                "smooth over freshly started channel position jitter with engine time over this duration in seconds");
ConVar snd_play_interp_ratio("snd_play_interp_ratio", 0.50f, FCVAR_NONE,
                             "percentage of snd_play_interp_duration to use 100% engine time over audio time (some devices report 0 for very long)");
ConVar snd_file_min_size(
    "snd_file_min_size", 64, FCVAR_NONE,
    "minimum file size in bytes for WAV files to be considered valid (everything below will fail to load), this is a workaround for BASS crashes");

ConVar snd_force_load_unknown("snd_force_load_unknown", false, FCVAR_NONE, "force loading of assumed invalid audio files");
} // namespace cv

Sound::Sound(UString filepath, bool stream, bool threeD, bool loop, bool prescan)
    : Resource(filepath)
{
	m_bStream = stream;
	m_bIs3d = threeD;
	m_bIsLooped = loop;
	m_bPrescan = prescan;
	m_bIsOverlayable = false;

	m_bIgnored = false; // check for audio file validity

	m_fVolume = 1.0f;
	m_fLastPlayTime = -1.0f;
}

void Sound::initAsync()
{
	if (cv::debug_rm.getBool())
		debugLog("Resource Manager: Loading {:s}\n", m_sFilePath);

	// sanity check for malformed audio files
	UString fileExtensionLowerCase = env->getFileExtensionFromFilePath(m_sFilePath);
	fileExtensionLowerCase.lowerCase();

	if (m_sFilePath.isEmpty() || fileExtensionLowerCase.isEmpty())
	{
		m_bIgnored = true;
	}
	else if (!isValidAudioFile(m_sFilePath, fileExtensionLowerCase))
	{
		if (!cv::snd_force_load_unknown.getBool())
		{
			debugLog("Sound: Ignoring malformed/corrupt .{:s} file {:s}\n", fileExtensionLowerCase, m_sFilePath);
			m_bIgnored = true;
		}
		else
		{
			debugLog("Sound: snd_force_load_unknown=true, loading what seems to be a malformed/corrupt .{:s} file {:s}\n", fileExtensionLowerCase, m_sFilePath);
			m_bIgnored = false;
		}
	}
}

Sound *Sound::createSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan)
{
#if !defined(MCENGINE_FEATURE_BASS) && !defined(MCENGINE_FEATURE_SOLOUD) && !defined(MCENGINE_FEATURE_SDL_MIXER)
#error No sound backend available!
#endif
#ifdef MCENGINE_FEATURE_BASS
	if (soundEngine->getTypeId() == BASS)
		return new BassSound(filepath, stream, threeD, loop, prescan);
#endif
#ifdef MCENGINE_FEATURE_SDL_MIXER
	if (soundEngine->getTypeId() == SDL)
		return new SDLSound(filepath, stream, threeD, loop, prescan);
#endif
#ifdef MCENGINE_FEATURE_SOLOUD
	if (soundEngine->getTypeId() == SOLOUD)
		return new SoLoudSound(filepath, stream, threeD, loop, prescan);
#endif
	return nullptr;
}

// quick heuristic to check if it's going to be worth loading the audio,
// tolerate some junk data at the start of the files but check for valid headers
bool Sound::isValidAudioFile(const UString &filePath, const UString &fileExt)
{
	McFile audFile(filePath, McFile::TYPE::READ);

	if (!audFile.canRead())
		return false;

	size_t fileSize = audFile.getFileSize();

	if (fileExt == "wav")
	{
		if (fileSize < cv::snd_file_min_size.getVal<size_t>())
			return false;

		const char *data = audFile.readFile();
		if (data == nullptr)
			return false;

		// check immediate RIFF header
		if (memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0)
			return true;

		// search first 512 bytes for RIFF header (minimal tolerance)
		size_t searchLimit = (fileSize < 512) ? fileSize - 12 : 512;
		for (size_t i = 1; i <= searchLimit; i++)
		{
			if (i + 12 < fileSize && memcmp(data + i, "RIFF", 4) == 0 && memcmp(data + i + 8, "WAVE", 4) == 0)
				return true;
		}
		return false;
	}
	else if (fileExt == "mp3") // mostly taken from ffmpeg/libavformat/mp3dec.c mp3_read_header()
	{
		if (fileSize < cv::snd_file_min_size.getVal<size_t>())
			return false;

		const char *data = audFile.readFile();
		if (data == nullptr)
			return false;

		// quick check for ID3v2 tag at start
		if (memcmp(data, "ID3", 3) == 0)
			return true;

		// // search through first 2KB for MP3 sync with basic validation
		// size_t searchLimit = (fileSize < 2048) ? fileSize - 4 : 2048;
		// for (size_t i = 0; i < searchLimit; i++)
		// {
		// 	if (i + 3 < fileSize && (unsigned char)data[i] == 0xFF && ((unsigned char)data[i + 1] & 0xF8) == 0xF8)
		// 	{
		// 		// basic frame header validation
		// 		auto byte2 = (unsigned char)data[i + 1];
		// 		auto byte3 = (unsigned char)data[i + 2];

		// 		// check for reserved/invalid values
		// 		if ((byte2 & 0x18) != 0x08 && // MPEG version not reserved
		// 		    (byte2 & 0x06) != 0x00 && // layer not reserved
		// 		    (byte3 & 0xF0) != 0xF0 && // bitrate not invalid
		// 		    (byte3 & 0x0C) != 0x0C)   // sample rate not reserved
		// 			return true;
		// 	}
		// }
		// this is stupid actually, there's a lot of cases where a file has a .mp3 extension but contains AAC data, so just return valid if it's big enough
		// since SoLoud (+ffmpeg) and BASS (windows) can decode them
		return true;
	}
	else if (fileExt == "ogg")
	{
		if (fileSize < cv::snd_file_min_size.getVal<size_t>())
			return false;

		const char *data = audFile.readFile();
		if (data == nullptr)
			return false;

		// check immediate OGG header
		if (memcmp(data, "OggS", 4) == 0)
			return true;

		// search first 1KB for OGG page header
		size_t searchLimit = (fileSize < 1024) ? fileSize - 4 : 1024;
		for (size_t i = 1; i < searchLimit; i++)
		{
			if (i + 4 < fileSize && memcmp(data + i, "OggS", 4) == 0)
				return true;
		}
		return false;
	}
	else if (fileExt == "flac")
	{
		if (fileSize < std::max<size_t>(cv::snd_file_min_size.getVal<size_t>(), 96)) // account for larger header
			return false;

		const char *data = audFile.readFile();
		if (data == nullptr)
			return false;

		// check immediate FLAC header
		if (memcmp(data, "fLaC", 4) == 0)
			return true;

		// search first 1KB for FLAC page header
		size_t searchLimit = (fileSize < 1024) ? fileSize - 4 : 1024;
		for (size_t i = 1; i < searchLimit; i++)
		{
			if (i + 4 < fileSize && memcmp(data + i, "fLaC", 4) == 0)
				return true;
		}
		return false;
	}

	return false; // don't let unsupported formats be read
}
