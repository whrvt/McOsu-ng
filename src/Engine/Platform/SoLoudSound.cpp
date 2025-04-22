//========== Copyright (c) 2025, WH, All rights reserved. ============//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//===============================================================================//

#include "SoLoudSound.h"
#include "SoLoudSoundEngine.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

extern ConVar debug_snd;
extern ConVar snd_speed_compensate_pitch;
extern ConVar snd_play_interp_duration;
extern ConVar snd_play_interp_ratio;
extern ConVar snd_wav_file_min_size;

SoLoudSound::SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan) : Sound(filepath, stream, threeD, loop, prescan)
{
	m_handle = 0;
	m_speed = 1.0f;
	m_pitch = 1.0f;
	m_frequency = 44100.0f;
	m_prevPosition = 0;
	m_audioSource = nullptr;
	m_engine = nullptr;
}

SoLoudSoundEngine *SoLoudSound::getSoLoudEngine()
{
	if (m_engine == nullptr)
	{
		m_engine = dynamic_cast<SoLoudSoundEngine *>(engine->getSound());
		if (m_engine == nullptr && debug_snd.getBool())
		{
			debugLog("SoLoudSound: Failed to get SoLoudSoundEngine instance");
		}
	}
	return m_engine;
}

void SoLoudSound::init()
{
	if (m_sFilePath.length() < 2 || !(m_bAsyncReady.load()))
		return;

	getSoLoudEngine();

	if (!m_audioSource)
	{
		UString msg = "Couldn't load sound \"";
		msg.append(m_sFilePath);
		msg.append(UString::format("\", stream = %i", (int)m_bStream));
		msg.append(", file = ");
		msg.append(m_sFilePath);
		msg.append("\n");
		debugLog(0xffdd3333, "%s", msg.toUtf8());
	}
	else
		m_bReady = true;
}

SoLoudSound::~SoLoudSound()
{
	destroy();
}

void SoLoudSound::initAsync()
{
	if (ResourceManager::debug_rm->getBool())
		debugLog("Resource Manager: Loading %s\n", m_sFilePath.toUtf8());

	// quick n dirty check for corrupt WAV files (same as BASS impl.)
	const int minWavFileSize = snd_wav_file_min_size.getInt();
	if (minWavFileSize > 0)
	{
		UString fileExtensionLowerCase = env->getFileExtensionFromFilePath(m_sFilePath);
		fileExtensionLowerCase.lowerCase();
		if (fileExtensionLowerCase == "wav")
		{
			McFile wavFile(m_sFilePath);
			if (wavFile.getFileSize() < (size_t)minWavFileSize)
			{
				debugLog("Sound: Ignoring malformed/corrupt WAV file (%i) %s\n", (int)wavFile.getFileSize(), m_sFilePath.toUtf8());
				return;
			}
		}
	}

	// clean up any previous instance
	if (m_audioSource)
	{
		if (m_bStream)
			delete static_cast<SoLoud::WavStream *>(m_audioSource);
		else
			delete static_cast<SoLoud::Wav *>(m_audioSource);
		m_audioSource = nullptr;
	}

	// create the appropriate audio source based on streaming flag
	SoLoud::result result = SoLoud::SO_NO_ERROR;
	if (m_bStream)
	{
		// use WavStream for streaming audio (music, etc.)
		SoLoud::WavStream *wavStream = new SoLoud::WavStream();
		result = wavStream->load(m_sFilePath.toUtf8());

		if (result == SoLoud::SO_NO_ERROR)
		{
			m_audioSource = wavStream;
			// we'll get the actual frequency after we have a handle
			m_frequency = 44100.0f;
		}
		else
		{
			delete wavStream;
			debugLog("Sound Error: SoLoud::WavStream::load() error %i on file %s\n", result, m_sFilePath.toUtf8());
			return;
		}
	}
	else
	{
		// use Wav for non-streaming audio (hit sounds, effects, etc.)
		SoLoud::Wav *wav = new SoLoud::Wav();
		result = wav->load(m_sFilePath.toUtf8());

		if (result == SoLoud::SO_NO_ERROR)
		{
			m_audioSource = wav;
			m_frequency = 44100.0f;
		}
		else
		{
			delete wav;
			debugLog("Sound Error: SoLoud::Wav::load() error %i on file %s\n", result, m_sFilePath.toUtf8());
			return;
		}
	}

	// configure common properties
	m_audioSource->setLooping(m_bIsLooped);

	// configure 3D audio if needed
	if (m_bIs3d)
	{
		m_audioSource->set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, 2.1f);
		m_audioSource->set3dMinMaxDistance(1.0f, 1000.0f);
	}

	m_bAsyncReady = true;
}

SoLoudSound::SOUNDHANDLE SoLoudSound::getHandle()
{
	return m_handle;
}

void SoLoudSound::destroy()
{
	if (!m_bReady)
		return;

	m_bReady = false;

	// stop the sound if it's playing
	sndEngine(&SoLoudSoundEngine::stopSound);
	m_handle = 0;

	// clean up audio source
	if (m_audioSource)
	{
		if (m_bStream)
			delete static_cast<SoLoud::WavStream *>(m_audioSource);
		else
			delete static_cast<SoLoud::Wav *>(m_audioSource);

		m_audioSource = nullptr;
	}
}

void SoLoudSound::setPosition(double percent)
{
	if (!m_bReady || !m_audioSource)
		return;

	percent = clamp<double>(percent, 0.0, 1.0);

	const double lengthInSeconds = m_bStream ? asWavStream()->getLength() : asWav()->getLength();

	const double positionInSeconds = lengthInSeconds * percent;

	// for play interpolation (similar to BASS implementation)
	if (positionInSeconds < snd_play_interp_duration.getFloat())
		m_fLastPlayTime = engine->getTime() - positionInSeconds;
	else
		m_fLastPlayTime = 0.0;

	sndEngine(&SoLoudSoundEngine::seekSound, positionInSeconds);
}

void SoLoudSound::setPositionMS(unsigned long ms, bool internal)
{
	if (!m_bReady || !m_audioSource || ms > getLengthMS())
		return;

	const double positionInSeconds = ms / 1000.0;

	// for play interpolation
	if (positionInSeconds < snd_play_interp_duration.getFloat())
		m_fLastPlayTime = engine->getTime() - positionInSeconds;
	else
		m_fLastPlayTime = 0.0;

	sndEngine(&SoLoudSoundEngine::seekSound, positionInSeconds);
}

void SoLoudSound::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = clamp<float>(volume, 0.0f, 1.0f);

	if (!m_bIsOverlayable)
	{
		sndEngine(&SoLoudSoundEngine::setVolumeSound, m_fVolume);
	}
}

void SoLoudSound::setSpeed(float speed)
{
	if (!m_bReady)
		return;

	speed = clamp<float>(speed, 0.05f, 50.0f);
	m_speed = speed;

	// this is bogus, need a custom audio filter to implement this properly
	// if (snd_speed_compensate_pitch.getBool())
	// {
		sndEngine(&SoLoudSoundEngine::setRelativePlaySpeedSound, speed);

	// 	sndEngine(&SoLoudSoundEngine::setSampleRateSound, m_frequency / speed * m_pitch);
	// }
	// else
	// {
	// 	sndEngine(&SoLoudSoundEngine::setRelativePlaySpeedSound, speed);
	// }
}

void SoLoudSound::setPitch(float pitch)
{
	if (!m_bReady)
		return;

	pitch = clamp<float>(pitch, 0.0f, 2.0f);
	m_pitch = pitch;

	// bogus for same reason as above, setting sample rate does the same thing as setRelativePlaySpeed but with a twist
	// if (snd_speed_compensate_pitch.getBool())
	// {
	// 	sndEngine(&SoLoudSoundEngine::setSampleRateSound, m_frequency / m_speed * pitch);
	// }
	// else
	// {
		sndEngine(&SoLoudSoundEngine::setSampleRateSound, m_frequency * pitch);
	// }
}

void SoLoudSound::setFrequency(float frequency)
{
	if (!m_bReady)
		return;

	frequency = (frequency > 99.0f ? clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);
	m_frequency = frequency;

	// bogus again
	// if (snd_speed_compensate_pitch.getBool())
	// {
	// 	sndEngine(&SoLoudSoundEngine::setSampleRateSound, frequency / m_speed * m_pitch);
	// }
	// else
	// {
		sndEngine(&SoLoudSoundEngine::setSampleRateSound, frequency * m_pitch);
	//}
}

void SoLoudSound::setPan(float pan)
{
	if (!m_bReady)
		return;

	pan = clamp<float>(pan, -1.0f, 1.0f);

	sndEngine(&SoLoudSoundEngine::setPanSound, pan);
}

void SoLoudSound::setLoop(bool loop)
{
	if (!m_bReady || !m_audioSource)
		return;

	m_bIsLooped = loop;
	m_audioSource->setLooping(loop);

	sndEngine(&SoLoudSoundEngine::setLoopingSound, loop);
}

float SoLoudSound::getPosition()
{
	if (!m_bReady || !m_audioSource)
		return 0.0f;

	const float positionInSeconds = sndEngine<float>(0.0f, &SoLoudSoundEngine::getStreamPositionSound);
	float lengthInSeconds = 0.0f;

	// get the length from the appropriate source type
	if (m_bStream && asWavStream())
	{
		lengthInSeconds = asWavStream()->getLength();
	}
	else if (!m_bStream && asWav())
	{
		lengthInSeconds = asWav()->getLength();
	}

	if (lengthInSeconds <= 0.0f)
		return 0.0f;

	return positionInSeconds / lengthInSeconds;
}

unsigned long SoLoudSound::getPositionMS()
{
	if (!m_bReady || !m_audioSource)
		return 0;

	const double positionInSeconds = sndEngine<float>(0.0, &SoLoudSoundEngine::getStreamPositionSound);
	const double positionInMilliSeconds = positionInSeconds * 1000.0;

	const unsigned long positionMS = static_cast<unsigned long>(positionInMilliSeconds);

	// special case for freshly started channel position jitter (copied from BASS implementation)
	const double interpDuration = snd_play_interp_duration.getFloat();
	const unsigned long interpDurationMS = interpDuration * 1000;
	if (interpDuration > 0.0 && positionMS < interpDurationMS)
	{
		const float speedMultiplier = getSpeed();
		const double delta = (engine->getTime() - m_fLastPlayTime) * speedMultiplier;
		if (m_fLastPlayTime > 0.0 && delta < interpDuration && isPlaying())
		{
			const double lerpPercent =
			    clamp<double>(((delta / interpDuration) - snd_play_interp_ratio.getFloat()) / (1.0 - snd_play_interp_ratio.getFloat()), 0.0, 1.0);
			return static_cast<unsigned long>(lerp<double>(delta * 1000.0, (double)positionMS, lerpPercent));
		}
	}

	return positionMS;
}

unsigned long SoLoudSound::getLengthMS()
{
	if (!m_bReady || !m_audioSource)
		return 0;

	double lengthInSeconds = 0.0;

	// get the length from the appropriate source type
	if (m_bStream && asWavStream())
	{
		lengthInSeconds = asWavStream()->getLength();
	}
	else if (!m_bStream && asWav())
	{
		lengthInSeconds = asWav()->getLength();
	}

	const double lengthInMilliSeconds = lengthInSeconds * 1000.0;

	return static_cast<unsigned long>(lengthInMilliSeconds);
}

float SoLoudSound::getSpeed()
{
	if (!m_bReady)
		return 1.0f;

	return m_speed;
}

float SoLoudSound::getPitch()
{
	if (!m_bReady)
		return 1.0f;

	return m_pitch;
}

float SoLoudSound::getFrequency()
{
	if (!m_bReady)
		return 44100.0f;

	float currentFreq = sndEngine<float>(44100.0f, &SoLoudSoundEngine::getSampleRateSound);
	if (currentFreq > 0)
	{
		m_frequency = currentFreq;
	}

	return m_frequency;
}

bool SoLoudSound::isPlaying()
{
	if (!m_bReady)
		return false;

	return sndEngine<bool>(false, &SoLoudSoundEngine::isValidVoiceHandleSound) && !sndEngine<bool>(true, &SoLoudSoundEngine::getPauseSound);
}

bool SoLoudSound::isFinished()
{
	if (!m_bReady)
		return false;

	return !sndEngine<bool>(false, &SoLoudSoundEngine::isValidVoiceHandleSound);
}

void SoLoudSound::rebuild(UString newFilePath)
{
	m_sFilePath = newFilePath;
	reload();
}

#endif // MCENGINE_FEATURE_SOLOUD
