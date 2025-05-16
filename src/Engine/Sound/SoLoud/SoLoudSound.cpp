//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudSound.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include "SoLoudManager.h"
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
    : Sound(filepath, stream, threeD, loop, prescan), m_handle(0), m_speed(1.0f), m_pitch(1.0f), m_frequency(44100.0f), m_audioSource(nullptr), m_filter(nullptr),
      m_usingFilter(false), m_fActualSpeedForDisabledPitchCompensation(1.0f), m_fLastRawSoLoudPosition(0.0), m_fLastSoLoudPositionTime(0.0), m_fSoLoudPositionRate(1000.0)
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
	if (m_handle != 0)
	{
		SL::stop(m_handle);
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

// TODO: move this somewhere move appropriate
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
			m_filter->setSource(m_audioSource);

		// set initial parameters
		m_filter->setSpeedFactor(m_speed);
		m_filter->setPitchFactor(m_pitch);

		// FIXME: ok, something is messed up, why do i have to manually set looping on the filter instance? why is it not inherited?
		m_filter->setLooping(m_bIsLooped);

		if (debug_snd.getBool())
			debugLog("SoLoudSound: Created SoundTouch filter for %s with speed=%f, pitch=%f\n", m_sFilePath.toUtf8(), m_speed, m_pitch);
	}

	return m_filter;
}

bool SoLoudSound::updateFilterParameters()
{
	if (!m_filter)
		return false;

	m_filter->setSpeedFactor(m_speed);
	m_filter->setPitchFactor(m_pitch);

	// FIXME: same as above
	m_filter->setLooping(m_bIsLooped);

	return true;
}

void SoLoudSound::setPosition(double percent)
{
	if (!m_bReady || !m_audioSource)
		return;

	percent = std::clamp<double>(percent, 0.0, 1.0);

	// calculate position based on the ORIGINAL timeline
	const double originalLengthInSeconds = m_bStream ? asWavStream()->getLength() : asWav()->getLength();
	double originalPositionInSeconds = originalLengthInSeconds * percent;

	// convert to the actual seek position for the engine
	double seekPositionInSeconds = convertFromOriginalTimeline(originalPositionInSeconds);

	// reset position interp vars (use original timeline values)
	m_fLastRawSoLoudPosition = originalPositionInSeconds * 1000.0;
	m_fLastSoLoudPositionTime = Timing::getTimeReal();
	m_fSoLoudPositionRate = 1000.0 * getSpeed();

	// seek in the engine
	if (m_handle != 0)
		SL::seek(m_handle, seekPositionInSeconds);
}

void SoLoudSound::setPositionMS(unsigned long ms, bool internal)
{
	if (!m_bReady || !m_audioSource)
		return;

	// check against the ORIGINAL length
	unsigned long originalLengthMS = getLengthMS();
	if (ms > originalLengthMS)
		return;

	double originalPositionInSeconds = ms / 1000.0;

	// convert to the actual seek position for the engine
	double seekPositionInSeconds = convertFromOriginalTimeline(originalPositionInSeconds);

	// reset position interp vars (use original timeline values)
	m_fLastRawSoLoudPosition = ms;
	m_fLastSoLoudPositionTime = Timing::getTimeReal();
	m_fSoLoudPositionRate = 1000.0 * getSpeed();

	// seek in the engine
	if (m_handle != 0)
		SL::seek(m_handle, seekPositionInSeconds);
}

void SoLoudSound::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	// apply to active voice if not overlayable
	if (!m_bIsOverlayable && m_handle != 0)
		SL::setVolume(m_handle, m_fVolume);
}

void SoLoudSound::setSpeed(float speed)
{
	if (!m_bReady)
		return;

	speed = std::clamp<float>(speed, 0.05f, 50.0f);

	if (m_speed != speed)
	{
		float previousSpeed = m_speed;
		m_speed = speed;

		// for streaming audio, simply update the filter parameters (no restart needed)
		if (m_bStream && m_filter)
		{
			updateFilterParameters();

			if (debug_snd.getBool())
				debugLog("SoLoudSound: Speed change %s: %f->%f (stream, filter updated live)\n", m_sFilePath.toUtf8(), previousSpeed, m_speed);
		}
		// for non-streaming audio, we still need the restart logic
		// TODO: remove this and all related logic in SoLoudSoundEngine, we never want to play through the filter for non-streamed audio
		else if (!m_bStream)
		{
			bool wasUsingFilter = isUsingRateChange();

			// real source playback position before stopping
			double actualAudioPosition = 0.0;
			if (isPlaying())
				actualAudioPosition = getActualAudioPositionInSeconds();

			updateFilterParameters();

			if (isPlaying())
			{
				// stop and restart with new parameters
				SL::stop(m_handle);
				m_handle = 0;

				// this will apply the new parameters
				engine->getSound()->play(this, 0.0f, m_pitch);

				// restore position to source position
				if (m_handle != 0)
					SL::seek(m_handle, actualAudioPosition);

				// interp tracks GAMEPLAY position
				double gameplayPosition = actualAudioPosition * speed;
				m_fLastRawSoLoudPosition = gameplayPosition * 1000.0;
				m_fLastSoLoudPositionTime = Timing::getTimeReal();
				m_fSoLoudPositionRate = 1000.0 * speed;

				if (debug_snd.getBool())
					debugLog("SoLoudSound: Speed change %s: %f->%f, actualAudioPos=%.3fs, seekPos=%.3fs (filter: %d->%d)\n", m_sFilePath.toUtf8(), previousSpeed, m_speed,
					         actualAudioPosition, actualAudioPosition, wasUsingFilter ? 1 : 0, isUsingRateChange() ? 1 : 0);
			}
		}
	}

	m_fActualSpeedForDisabledPitchCompensation = speed;
}

void SoLoudSound::setPitch(float pitch)
{
	if (!m_bReady)
		return;

	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

	if (m_pitch != pitch)
	{
		float previousPitch = m_pitch;
		m_pitch = pitch;

		// for streaming audio, simply update the filter parameters (no restart needed)
		if (m_bStream && m_filter)
		{
			updateFilterParameters();

			if (debug_snd.getBool())
				debugLog("SoLoudSound: Pitch change %s: %f->%f (stream, filter updated live)\n", m_sFilePath.toUtf8(), previousPitch, m_pitch);
		}
		// for non-streaming audio, we still need the restart logic
		// TODO: remove this and all related logic in SoLoudSoundEngine, we never want to play through the filter for non-streamed audio
		else if (!m_bStream)
		{
			bool wasUsingFilter = isUsingRateChange();

			// ACTUAL audio file position
			double actualAudioPosition = 0.0;
			if (isPlaying())
				actualAudioPosition = getActualAudioPositionInSeconds();

			updateFilterParameters();

			if (isPlaying())
			{
				// stop and restart with new parameters
				SL::stop(m_handle);
				m_handle = 0;

				// this will apply the new parameters
				engine->getSound()->play(this, 0.0f, pitch);

				// always seek to the actual audio position
				if (m_handle != 0)
					SL::seek(m_handle, actualAudioPosition);

				// reset interpolation with gameplay timeline values
				double gameplayPosition = actualAudioPosition * getSpeed();
				m_fLastRawSoLoudPosition = gameplayPosition * 1000.0;
				m_fLastSoLoudPositionTime = Timing::getTimeReal();
				m_fSoLoudPositionRate = 1000.0 * getSpeed();

				if (debug_snd.getBool())
					debugLog("SoLoudSound: Pitch change %s: %f->%f, actualAudioPos=%.3fs, seekPos=%.3fs (filter: %d->%d)\n", m_sFilePath.toUtf8(), previousPitch, m_pitch,
					         actualAudioPosition, actualAudioPosition, wasUsingFilter ? 1 : 0, isUsingRateChange() ? 1 : 0);
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
	if (m_handle != 0)
		SL::setPan(m_handle, pan);
}

void SoLoudSound::setLoop(bool loop)
{
	if (!m_bReady || !m_audioSource)
		return;

	m_bIsLooped = loop;

	// apply to the source
	m_audioSource->setLooping(loop);

	// apply to the active voice
	if (m_handle != 0)
		SL::setLooping(m_handle, loop);
}

bool SoLoudSound::isUsingRateChange() const
{
	if (snd_speed_compensate_pitch.getBool())
		return (m_speed != 1.0f || m_pitch != 1.0f);
	else
		return (m_speed != 1.0f && m_pitch == 1.0f);
}

// TODO: factor/clarify/make unnecessary? this is confusing
double SoLoudSound::convertToOriginalTimeline(double enginePosition) const
{
	// for getPositionMS() to return speed-adjusted position for gameplay
	return isUsingRateChange() ? (enginePosition * m_speed) : enginePosition;
}

// TODO: factor/clarify/make unnecessary? this is confusing
double SoLoudSound::convertFromOriginalTimeline(double originalPosition) const
{
	// this is only used for setPosition/setPositionMS from external callers
	// they expect to be able to set positions in "gameplay time"
	return isUsingRateChange() ? (originalPosition / m_speed) : originalPosition;
}

float SoLoudSound::getPosition()
{
	if (!m_bReady || !m_audioSource)
		return 0.0f;

	// get engine position and convert to original timeline
	double enginePositionInSeconds = 0.0;
	if (m_handle != 0)
		enginePositionInSeconds = SL::getStreamPosition(m_handle);

	double originalPositionInSeconds = convertToOriginalTimeline(enginePositionInSeconds);

	// UNADJUSTED source length
	double originalLengthInSeconds = 0.0;
	if (m_bStream && asWavStream())
		originalLengthInSeconds = asWavStream()->getLength();
	else if (!m_bStream && asWav())
		originalLengthInSeconds = asWav()->getLength();

	if (originalLengthInSeconds <= 0.0)
		return 0.0f;

	// relative position to original timeline
	return std::clamp<float>(originalPositionInSeconds / originalLengthInSeconds, 0.0f, 1.0f);
}

// slightly tweaked interp algo from the SDL_mixer version, to smooth out position updates
unsigned long SoLoudSound::getPositionMS()
{
	if (!m_bReady || !m_audioSource)
		return 0;

	// get engine position and convert to GAMEPLAY timeline
	double enginePositionInSeconds = 0.0;
	if (m_handle != 0)
		enginePositionInSeconds = SL::getStreamPosition(m_handle);

	double gameplayPositionInSeconds = convertToOriginalTimeline(enginePositionInSeconds);

	const double currentTime = Timing::getTimeReal();
	const double gameplayPositionMS = gameplayPositionInSeconds * 1000.0;

	if (m_fLastSoLoudPositionTime <= 0.0 || !isPlaying())
	{
		m_fLastRawSoLoudPosition = gameplayPositionMS;
		m_fLastSoLoudPositionTime = currentTime;
		m_fSoLoudPositionRate = 1000.0 * getSpeed(); // Gameplay rate
		return (unsigned long)gameplayPositionMS;
	}

	// if the position changed, update our rate estimate
	if (m_fLastRawSoLoudPosition != gameplayPositionMS)
	{
		const double timeDelta = currentTime - m_fLastSoLoudPositionTime;

		// only update rate if enough time has passed
		if (timeDelta > 0.005)
		{
			double newRate;

			if (gameplayPositionMS >= m_fLastRawSoLoudPosition)
			{
				newRate = (gameplayPositionMS - m_fLastRawSoLoudPosition) / timeDelta;
			}
			else if (m_bIsLooped)
			{
				// handle loop wraparound
				unsigned long length = getLengthMS();
				if (length > 0)
				{
					double wrappedChange = (length - m_fLastRawSoLoudPosition) + gameplayPositionMS;
					newRate = wrappedChange / timeDelta;
				}
				else
				{
					newRate = m_fSoLoudPositionRate;
				}
			}
			else
			{
				newRate = m_fSoLoudPositionRate;
			}

			// sanity check, expected rate is still 1000.0 * speed
			// because we're measuring in the original timeline
			const double expectedRate = 1000.0 * getSpeed();
			const double maxDeviation = 0.2;

			if (newRate < expectedRate * (1.0 - maxDeviation) || newRate > expectedRate * (1.0 + maxDeviation))
			{
				newRate = 0.7 * expectedRate + 0.3 * newRate;
			}

			m_fSoLoudPositionRate = m_fSoLoudPositionRate * 0.6 + newRate * 0.4;
		}

		m_fLastRawSoLoudPosition = gameplayPositionMS;
		m_fLastSoLoudPositionTime = currentTime;
	}
	else
	{
		const double timeSinceLastPositionChange = currentTime - m_fLastSoLoudPositionTime;
		if (timeSinceLastPositionChange > 0.1)
		{
			const double expectedRate = 1000.0 * getSpeed();
			m_fSoLoudPositionRate = m_fSoLoudPositionRate * 0.95 + expectedRate * 0.05;
		}
	}

	// calculate the interpolated position
	const double timeSinceLastReading = currentTime - m_fLastSoLoudPositionTime;
	const double interpolatedPosition = m_fLastRawSoLoudPosition + (timeSinceLastReading * m_fSoLoudPositionRate);

	// check for looping
	if (m_bIsLooped)
	{
		unsigned long length = getLengthMS();
		if (length > 0 && interpolatedPosition >= length)
		{
			return static_cast<unsigned long>(fmod(interpolatedPosition, length));
		}
	}

	return static_cast<unsigned long>(interpolatedPosition);
}

unsigned long SoLoudSound::getLengthMS()
{
	if (!m_bReady || !m_audioSource)
		return 0;

	// UNADJUSTED length
	double originalLengthInSeconds = 0.0;
	if (m_bStream && asWavStream())
		originalLengthInSeconds = asWavStream()->getLength();
	else if (!m_bStream && asWav())
		originalLengthInSeconds = asWav()->getLength();

	const double lengthInMilliSeconds = originalLengthInSeconds * 1000.0;
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
	if (m_handle != 0)
	{
		float currentFreq = SL::getSamplerate(m_handle);
		if (currentFreq > 0)
			m_frequency = currentFreq;
	}

	return m_frequency;
}

bool SoLoudSound::isPlaying()
{
	if (!m_bReady)
		return false;

	if (m_handle == 0)
		return false;

	// a sound is playing if the handle is valid and the sound isn't paused
	return SL::isValidVoiceHandle(m_handle) && !SL::getPause(m_handle);
}

bool SoLoudSound::isFinished()
{
	if (!m_bReady)
		return false;

	if (m_handle == 0)
		return true;

	// a sound is finished if the handle is no longer valid
	return !SL::isValidVoiceHandle(m_handle);
}

void SoLoudSound::rebuild(UString newFilePath)
{
	m_sFilePath = newFilePath;
	reload();
}

#endif // MCENGINE_FEATURE_SOLOUD
