//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudSound.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include "SoLoudSoundEngine.h"

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

extern ConVar debug_snd;
extern ConVar snd_speed_compensate_pitch;
extern ConVar snd_play_interp_ratio;
extern ConVar snd_wav_file_min_size;

SoLoudSound::SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan)
    : Sound(filepath, stream, threeD, loop, prescan), m_handle(0), m_speed(1.0f), m_pitch(1.0f), m_frequency(44100.0f), m_audioSource(nullptr),
      m_filter(nullptr), m_usingFilter(false), m_fActualSpeedForDisabledPitchCompensation(1.0f), m_fLastRawSoLoudPosition(0.0),
      m_fLastSoLoudPositionTime(0.0), m_fSoLoudPositionRate(1000.0)
{
}

void SoLoudSound::init()
{
	if (m_sFilePath.length() < 2 || !(m_bAsyncReady.load()))
		return;

	// re-set some values to their defaults (only necessary because of the existence of rebuild())
	m_fActualSpeedForDisabledPitchCompensation = 1.0f;

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

	// check for corrupt WAV files (same as BASS impl.)
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
	// similar to the SDL_mixer thingy
	SoLoud::result result = SoLoud::SO_NO_ERROR;
	if (m_bStream)
	{
		// use WavStream for streaming audio (music, etc.)
		auto *wavStream = new SoLoud::WavStream();
		result = wavStream->load(m_sFilePath.toUtf8());

		if (result == SoLoud::SO_NO_ERROR)
		{
			m_audioSource = wavStream;
			m_frequency = 44100.0f; // default, will be updated when played
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
		auto *wav = new SoLoud::Wav();
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

	m_audioSource->setLooping(m_bIsLooped);

	// configure 3D audio (need to test if this works)
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
	SoLoudSoundEngine *engine = getSoLoudEngine();
	if (engine && m_handle != 0)
	{
		engine->stopSound(m_handle);
		m_handle = 0;
	}

	// clean up SoundTouch filter
	if (m_filter)
	{
		delete m_filter;
		m_filter = nullptr;
		m_usingFilter = false;
	}

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

SoLoud::SoundTouchFilter *SoLoudSound::getOrCreateFilter()
{
	if (m_filter)
		return m_filter;

	// create a new SoundTouch filter instance
	m_filter = new SoLoud::SoundTouchFilter();

	if (m_filter)
	{
		// configure initial source
		if (m_audioSource)
		{
			m_filter->setSource(m_audioSource);
		}

		// set initial parameters
		m_filter->setSpeedFactor(m_speed);
		m_filter->setPitchFactor(m_pitch);

		if (debug_snd.getBool())
		{
			debugLog("SoLoudSound: Created SoundTouch filter for %s with speed=%f, pitch=%f\n", m_sFilePath.toUtf8(), m_speed, m_pitch);
		}
	}

	return m_filter;
}

bool SoLoudSound::updateFilterParameters()
{
	if (!m_filter)
		return false;

	m_filter->setSpeedFactor(m_speed);
	m_filter->setPitchFactor(m_pitch);

	return true;
}

void SoLoudSound::setPosition(double percent)
{
	if (!m_bReady || !m_audioSource)
		return;

	percent = std::clamp<double>(percent, 0.0, 1.0);

	const double sourceLengthInSeconds = m_bStream ? asWavStream()->getLength() : asWav()->getLength();

	double positionInSeconds = sourceLengthInSeconds * percent;

	// reset position interp vars
	m_fLastRawSoLoudPosition = positionInSeconds * 1000.0;
	m_fLastSoLoudPositionTime = ::engine->getTime();
	m_fSoLoudPositionRate = 1000.0 * getSpeed();

	// seek it
	SoLoudSoundEngine *soloudEngine = getSoLoudEngine();
	if (soloudEngine && m_handle != 0)
	{
		soloudEngine->seekSound(m_handle, positionInSeconds);
	}
}

void SoLoudSound::setPositionMS(unsigned long ms, bool internal)
{
	if (!m_bReady || !m_audioSource)
		return;

	// don't exceed the actual length
	unsigned long originalLengthMS = getLengthMS();
	if (ms > originalLengthMS)
		return;

	double positionInSeconds = ms / 1000.0;

	// reset position interp vars
	m_fLastRawSoLoudPosition = ms;
	m_fLastSoLoudPositionTime = ::engine->getTime();
	m_fSoLoudPositionRate = 1000.0 * getSpeed();

	// seek it
	SoLoudSoundEngine *soloudEngine = getSoLoudEngine();
	if (soloudEngine && m_handle != 0)
	{
		soloudEngine->seekSound(m_handle, positionInSeconds);
	}
}

void SoLoudSound::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	// apply to active voice if not overlayable
	if (!m_bIsOverlayable && m_handle != 0)
	{
		SoLoudSoundEngine *engine = getSoLoudEngine();
		if (engine)
		{
			engine->setVolumeSound(m_handle, m_fVolume);
		}
	}
}

void SoLoudSound::setSpeed(float speed)
{
	if (!m_bReady)
		return;

	speed = std::clamp<float>(speed, 0.05f, 50.0f);

	if (m_speed != speed)
	{
		// store the new speed value
		m_speed = speed;

		// update position interp rate
		m_fSoLoudPositionRate = 1000.0 * speed;

		updateFilterParameters();

		if (isPlaying())
		{
			// currently can't update filter parameters on the fly, need to restart playback
			// thankfully it's not that heavy
			double pos = getPosition();

			SoLoudSoundEngine *engine = getSoLoudEngine();
			if (engine)
			{
				engine->stop(this);

				// this will apply the new parameters
				engine->play(this, 0.0f, m_pitch);

				// restore position
				setPosition(pos);

				if (debug_snd.getBool())
				{
					debugLog("SoLoudSound: Restarted playback after speed change for %s: speed=%f\n", m_sFilePath.toUtf8(), m_speed);
				}
			}
		}
	}
	// NOTE: currently only used for correctly returning getSpeed() if snd_speed_compensate_pitch is disabled
	m_fActualSpeedForDisabledPitchCompensation = speed;
}

void SoLoudSound::setPitch(float pitch)
{
	if (!m_bReady)
		return;

	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

	if (m_pitch != pitch)
	{
		m_pitch = pitch;
		updateFilterParameters();

		if (isPlaying())
		{
			// as above, need to restart playback
			double pos = getPosition();

			SoLoudSoundEngine *engine = getSoLoudEngine();
			if (engine)
			{
				engine->stop(this);

				engine->play(this, 0.0f, pitch);

				setPosition(pos);

				if (debug_snd.getBool())
				{
					debugLog("SoLoudSound: Restarted playback after pitch change for %s: pitch=%f\n", m_sFilePath.toUtf8(), m_pitch);
				}
			}
		}
	}
}

void SoLoudSound::setFrequency(float frequency)
{
	if (!m_bReady)
		return;

	frequency = (frequency > 99.0f ? std::clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

	if (m_frequency != frequency && frequency > 0)
	{
		float pitchRatio = frequency / m_frequency;
		m_frequency = frequency;

		// apply the frequency change through pitch
		// this isn't the only or even a good way, but it does the trick
		setPitch(m_pitch * pitchRatio);
	}
}

void SoLoudSound::setPan(float pan)
{
	if (!m_bReady)
		return;

	pan = std::clamp<float>(pan, -1.0f, 1.0f);

	// apply to the active voice
	SoLoudSoundEngine *engine = getSoLoudEngine();
	if (engine && m_handle != 0)
	{
		engine->setPanSound(m_handle, pan);
	}
}

void SoLoudSound::setLoop(bool loop)
{
	if (!m_bReady || !m_audioSource)
		return;

	m_bIsLooped = loop;

	// apply to the source
	m_audioSource->setLooping(loop);

	// apply to the active voice
	SoLoudSoundEngine *engine = getSoLoudEngine();
	if (engine && m_handle != 0)
	{
		engine->setLoopingSound(m_handle, loop);
	}
}

float SoLoudSound::getPosition()
{
	if (!m_bReady || !m_audioSource)
		return 0.0f;

	// get position from engine
	float positionInSeconds = 0.0f;
	SoLoudSoundEngine *engine = getSoLoudEngine();
	if (engine && m_handle != 0)
	{
		positionInSeconds = engine->getStreamPositionSound(m_handle);
	}

	// get source length
	float sourceLengthInSeconds = 0.0f;
	if (m_bStream && asWavStream())
	{
		sourceLengthInSeconds = asWavStream()->getLength();
	}
	else if (!m_bStream && asWav())
	{
		sourceLengthInSeconds = asWav()->getLength();
	}

	if (sourceLengthInSeconds <= 0.0f)
		return 0.0f;

	// return relative position
	return std::clamp<float>(positionInSeconds / sourceLengthInSeconds, 0.0f, 1.0f);
}

// slightly tweaked interp algo from the SDL_mixer version, to smooth out position updates
unsigned long SoLoudSound::getPositionMS()
{
	if (!m_bReady || !m_audioSource)
		return 0;

	// get position from engine
	double rawPositionInSeconds = 0.0;
	SoLoudSoundEngine *soloudEngine = getSoLoudEngine();
	if (soloudEngine && m_handle != 0)
	{
		rawPositionInSeconds = soloudEngine->getStreamPositionSound(m_handle);
	}

	const double currentTime = ::engine->getTime();
	const double rawPositionMS = rawPositionInSeconds * 1000.0;

	// if this is our first reading or if we're not playing, just use the raw value
	if (m_fLastSoLoudPositionTime <= 0.0 || !isPlaying())
	{
		m_fLastRawSoLoudPosition = rawPositionMS;
		m_fLastSoLoudPositionTime = currentTime;
		m_fSoLoudPositionRate = 1000.0 * getSpeed(); // initialize rate
		return (unsigned long)rawPositionMS;
	}

	// if the position changed, update our rate estimate
	if (m_fLastRawSoLoudPosition != rawPositionMS)
	{
		const double timeDelta = currentTime - m_fLastSoLoudPositionTime;

		// only update rate if enough time has passed to avoid division by very small numbers
		if (timeDelta > 0.005) // Reduced from 0.01 for more frequent updates
		{
			// calculate new rate (change in position / change in time)
			// account for possibility of wrapping (looped sound) by ensuring positive rate
			double newRate;

			if (rawPositionMS >= m_fLastRawSoLoudPosition)
			{
				newRate = (rawPositionMS - m_fLastRawSoLoudPosition) / timeDelta;
			}
			else if (m_bIsLooped)
			{
				// handle loop wraparound - calculate rate based on length
				unsigned long length = getLengthMS();
				if (length > 0)
				{
					// position wrapped
					double wrappedChange = (length - m_fLastRawSoLoudPosition) + rawPositionMS;
					newRate = wrappedChange / timeDelta;
				}
				else
				{
					// if we can't determine length, use current rate
					newRate = m_fSoLoudPositionRate;
				}
			}
			else
			{
				// not looped but position decreased?? use current rate
				newRate = m_fSoLoudPositionRate;
			}

			// sanity
			const double expectedRate = 1000.0 * getSpeed();
			const double maxDeviation = 0.2; // 20%

			if (newRate < expectedRate * (1.0 - maxDeviation) || newRate > expectedRate * (1.0 + maxDeviation))
			{
				// too far from expected, use a value closer to expected
				newRate = 0.7 * expectedRate + 0.3 * newRate;
			}

			// use weighted average favoring new rate more for better responsiveness
			// while still maintaining smoothness
			m_fSoLoudPositionRate = m_fSoLoudPositionRate * 0.6 + newRate * 0.4;
		}

		// store the new raw position and time
		m_fLastRawSoLoudPosition = rawPositionMS;
		m_fLastSoLoudPositionTime = currentTime;
	}
	else
	{
		// if position hasn't changed for too long, periodically reset rate to expected
		const double timeSinceLastPositionChange = currentTime - m_fLastSoLoudPositionTime;
		if (timeSinceLastPositionChange > 0.1) // 100ms without position change
		{
			// gradually drift toward expected rate to avoid "stuck" positions
			const double expectedRate = 1000.0 * getSpeed();
			m_fSoLoudPositionRate = m_fSoLoudPositionRate * 0.95 + expectedRate * 0.05;
		}
	}

	// calculate the interpolated position based on time elapsed since last raw reading
	const double timeSinceLastReading = currentTime - m_fLastSoLoudPositionTime;
	const double interpolatedPosition = m_fLastRawSoLoudPosition + (timeSinceLastReading * m_fSoLoudPositionRate);

	// check if interpolated position exceeds the sound length (for looped sounds)
	if (m_bIsLooped)
	{
		unsigned long length = getLengthMS();
		if (length > 0 && interpolatedPosition >= length)
		{
			// get modulo for looped position
			return static_cast<unsigned long>(fmod(interpolatedPosition, length));
		}
	}

	return static_cast<unsigned long>(interpolatedPosition);
}

unsigned long SoLoudSound::getLengthMS()
{
	if (!m_bReady || !m_audioSource)
		return 0;

	// get the base length from the source
	double sourceLengthInSeconds = 0.0;
	if (m_bStream && asWavStream())
	{
		sourceLengthInSeconds = asWavStream()->getLength();
	}
	else if (!m_bStream && asWav())
	{
		sourceLengthInSeconds = asWav()->getLength();
	}

	const double lengthInMilliSeconds = sourceLengthInSeconds * 1000.0;
	return static_cast<unsigned long>(lengthInMilliSeconds);
}

float SoLoudSound::getSpeed()
{
	if (!m_bReady)
		return 1.0f;

	if (!snd_speed_compensate_pitch.getBool())
		return m_fActualSpeedForDisabledPitchCompensation;

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

	// get sample rate from active voice
	SoLoudSoundEngine *engine = getSoLoudEngine();
	if (engine && m_handle != 0)
	{
		float currentFreq = engine->getSampleRateSound(m_handle);
		if (currentFreq > 0)
		{
			m_frequency = currentFreq;
		}
	}

	return m_frequency;
}

bool SoLoudSound::isPlaying()
{
	if (!m_bReady)
		return false;

	SoLoudSoundEngine *engine = getSoLoudEngine();
	if (!engine || m_handle == 0)
		return false;

	// a sound is playing if the handle is valid and the sound isn't paused
	return engine->isValidVoiceHandleSound(m_handle) && !engine->getPauseSound(m_handle);
}

bool SoLoudSound::isFinished()
{
	if (!m_bReady)
		return false;

	SoLoudSoundEngine *engine = getSoLoudEngine();
	if (!engine || m_handle == 0)
		return true;

	// a sound is finished if the handle is no longer valid
	return !engine->isValidVoiceHandleSound(m_handle);
}

void SoLoudSound::rebuild(UString newFilePath)
{
	m_sFilePath = newFilePath;
	reload();
}

#endif // MCENGINE_FEATURE_SOLOUD
