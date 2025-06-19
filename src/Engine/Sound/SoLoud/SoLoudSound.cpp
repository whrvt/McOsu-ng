//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudSound.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include <soloud.h>
#include <soloud_file.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include "SoLoudFX.h"
#include "SoLoudSoundEngine.h"

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

namespace cv
{
ConVar snd_soloud_prefer_ffmpeg("snd_soloud_prefer_ffmpeg", 0, FCVAR_NONE,
                                "(0=no, 1=streams, 2=streams+samples) prioritize using ffmpeg as a decoder (if available) over other decoder backends");
}

SoLoudSound::SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan)
    : Sound(filepath, stream, threeD, loop, prescan),
      m_handle(0),
      m_speed(1.0f),
      m_pitch(1.0f),
      m_frequency(44100.0f),
      m_audioSource(nullptr),
      m_fLastRawSoLoudPosition(0.0),
      m_fLastSoLoudPositionTime(0.0),
      m_fSoLoudPositionRate(1000.0)
{
}

void SoLoudSound::init()
{
	if (m_bIgnored || m_sFilePath.length() < 2 || !(m_bAsyncReady.load()))
		return;

	if (!m_audioSource)
		debugLog(0xffdd3333, "Couldn't load sound \"{}\", stream = {}, file = {}\n", m_sFilePath, (int)m_bStream, m_sFilePath);
	else
		m_bReady = true;
}

SoLoudSound::~SoLoudSound()
{
	destroy();
}

void SoLoudSound::initAsync()
{
	Sound::initAsync();
	if (m_bIgnored)
		return;

	// clean up any previous instance
	if (m_audioSource)
	{
		if (m_bStream)
			delete static_cast<SoLoud::SLFXStream *>(m_audioSource);
		else
			delete static_cast<SoLoud::Wav *>(m_audioSource);

		m_audioSource = nullptr;
	}

	// load file into memory first to handle unicode paths properly (windows shenanigans)
	McFile file(Env::cfg(OS::WINDOWS) ? m_sFilePath : "");
	const char *fileData = nullptr;
	size_t fileSize = 0;

	if constexpr (Env::cfg(OS::WINDOWS))
	{
		if (!file.canRead())
		{
			debugLog("Sound Error: Cannot open file {:s}\n", m_sFilePath.toUtf8());
			return;
		}
		fileSize = file.getFileSize();
		if (fileSize == 0)
		{
			debugLog("Sound Error: File is empty {:s}\n", m_sFilePath.toUtf8());
			return;
		}
		fileData = file.readFile();
		if (!fileData)
		{
			debugLog("Sound Error: Failed to read file data {:s}\n", m_sFilePath.toUtf8());
			return;
		}
	}

	// create the appropriate audio source based on streaming flag
	SoLoud::result result = SoLoud::SO_NO_ERROR;
	if (m_bStream)
	{
		// use SLFXStream for streaming audio (music, etc.) includes rate/pitch processing like BASS_FX_TempoCreate
		auto *stream = new SoLoud::SLFXStream(cv::snd_soloud_prefer_ffmpeg.getInt() > 0);

		// use loadToMem for streaming to handle unicode paths on windows
		if constexpr (Env::cfg(OS::WINDOWS))
			result = stream->loadMem(reinterpret_cast<const unsigned char *>(fileData), fileSize, true, false);
		else
			result = stream->load(m_sFilePath.toUtf8());

		if (result == SoLoud::SO_NO_ERROR)
		{
			m_audioSource = stream;
			m_frequency = stream->mBaseSamplerate;

			m_audioSource->setSingleInstance(true);           // only play one music track at a time
			m_audioSource->setInaudibleBehavior(true, false); // keep ticking the sound if it goes to 0 volume, and don't kill it

			if (cv::debug_snd.getBool())
				debugLog("SoLoudSound: Created SLFXStream for {:s} with speed={:f}, pitch={:f}, looping={:s}, decoder={:s}\n", m_sFilePath.toUtf8(), m_speed,
				         m_pitch, m_bIsLooped ? "true" : "false", stream->getDecoder());
		}
		else
		{
			delete stream;
			debugLog("Sound Error: SLFXStream::load() error {} on file {:s}\n", result, m_sFilePath.toUtf8());
			return;
		}
	}
	else
	{
		// use Wav for non-streaming audio (hit sounds, effects, etc.)
		auto *wav = new SoLoud::Wav(cv::snd_soloud_prefer_ffmpeg.getInt() > 1);

		if constexpr (Env::cfg(OS::WINDOWS))
			result = wav->loadMem(reinterpret_cast<const unsigned char *>(fileData), fileSize, true, false);
		else
			result = wav->load(m_sFilePath.toUtf8());

		if (result == SoLoud::SO_NO_ERROR)
		{
			m_audioSource = wav;
			m_frequency = wav->mBaseSamplerate;

			m_audioSource->setSingleInstance(false);         // allow non-music tracks to overlap by default
			m_audioSource->setInaudibleBehavior(true, true); // keep ticking the sound if it goes to 0 volume, but do kill it if necessary
		}
		else
		{
			delete wav;
			debugLog("Sound Error: SoLoud::Wav::load() error {} on file {:s}\n", result, m_sFilePath.toUtf8());
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

SOUNDHANDLE SoLoudSound::getHandle()
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
		soloud->stop(m_handle);
		m_handle = 0;
	}

	// clean up audio source
	if (m_audioSource)
	{
		if (m_bStream)
			delete static_cast<SoLoud::SLFXStream *>(m_audioSource);
		else
			delete static_cast<SoLoud::Wav *>(m_audioSource);

		m_audioSource = nullptr;
	}
}

void SoLoudSound::setPosition(double percent)
{
	if (!m_bReady || !m_audioSource || !m_handle)
		return;

	percent = std::clamp<double>(percent, 0.0, 1.0);

	// calculate position based on the ORIGINAL timeline
	const double streamLengthInSeconds = getSourceLengthInSeconds();
	double positionInSeconds = streamLengthInSeconds * percent;

	// reset position interp vars
	m_fLastRawSoLoudPosition = positionInSeconds * 1000.0;
	m_fLastSoLoudPositionTime = Timing::getTimeReal();
	m_fSoLoudPositionRate = 1000.0 * getSpeed();

	if (cv::debug_snd.getBool())
		debugLog("seeking to {:.2f} percent (position: {}ms, length: {}ms)\n", percent, static_cast<unsigned long>(positionInSeconds * 1000),
		         static_cast<unsigned long>(streamLengthInSeconds * 1000));

	// seek
	soloud->seek(m_handle, positionInSeconds);
}

void SoLoudSound::setPositionMS(unsigned long ms)
{
	if (!m_bReady || !m_audioSource || !m_handle)
		return;

	auto msD = static_cast<double>(ms);

	auto streamLengthMS = static_cast<double>(getLengthMS());
	if (msD > streamLengthMS)
		return;

	double positionInSeconds = msD / 1000.0;

	// reset position interp vars
	m_fLastRawSoLoudPosition = msD;
	m_fLastSoLoudPositionTime = Timing::getTimeReal();
	m_fSoLoudPositionRate = 1000.0 * getSpeed();

	if (cv::debug_snd.getBool())
		debugLog("seeking to {:g}ms (length: {:g}ms)\n", msD, streamLengthMS);

	// seek
	soloud->seek(m_handle, positionInSeconds);
}

void SoLoudSound::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	if (!m_handle)
		return;

	// apply to active voice if not overlayable
	if (!m_bIsOverlayable)
		soloud->setVolume(m_handle, m_fVolume);
}

void SoLoudSound::setSpeed(float speed)
{
	if (!m_bReady || !m_audioSource)
		return;

	// sample speed could be supported, but there is nothing using it right now so i will only bother when the time comes
	if (!m_bStream)
	{
		debugLog("Programmer Error: tried to setSpeed on a sample!\n");
		return;
	}

	speed = std::clamp<float>(speed, 0.05f, 50.0f);

	if (m_speed != speed)
	{
		float previousSpeed = m_speed;
		m_speed = speed;

		// simply update the SLFXStream parameters
		auto *stream = static_cast<SoLoud::SLFXStream *>(m_audioSource);
		stream->setSpeedFactor(m_speed);

		if (cv::debug_snd.getBool())
			debugLog("SoLoudSound: Speed change {:s}: {:f}->{:f} (stream, updated live)\n", m_sFilePath.toUtf8(), previousSpeed, m_speed);
	}
}

void SoLoudSound::setPitch(float pitch)
{
	if (!m_bReady || !m_audioSource)
		return;

	// sample pitch could be supported, but there is nothing using it right now so i will only bother when the time comes
	if (!m_bStream)
	{
		debugLog("Programmer Error: tried to setPitch on a sample!\n");
		return;
	}

	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

	if (m_pitch != pitch)
	{
		float previousPitch = m_pitch;
		m_pitch = pitch;

		// simply update the SLFXStream parameters
		auto *stream = static_cast<SoLoud::SLFXStream *>(m_audioSource);
		stream->setPitchFactor(m_pitch);

		if (cv::debug_snd.getBool())
			debugLog("SoLoudSound: Pitch change {:s}: {:f}->{:f} (stream, updated live)\n", m_sFilePath.toUtf8(), previousPitch, m_pitch);
	}
}

void SoLoudSound::setFrequency(float frequency)
{
	if (!m_bReady || !m_audioSource)
		return;

	frequency = (frequency > 99.0f ? std::clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

	if (m_frequency != frequency)
	{
		if (frequency > 0)
		{
			if (m_bStream)
			{
				float pitchRatio = frequency / m_frequency;

				// apply the frequency change through pitch
				// this isn't the only or even a good way, but it does the trick
				setPitch(m_pitch * pitchRatio);
			}
			else if (m_handle)
			{
				soloud->setSamplerate(m_handle, frequency);
			}
			m_frequency = frequency;
		}
		else // 0 means reset to default
		{
			m_frequency = m_audioSource->mBaseSamplerate;
			if (m_bStream)
				setPitch(1.0f);
			else if (m_handle)
				soloud->setSamplerate(m_handle, m_frequency);
		}
	}
}

void SoLoudSound::setPan(float pan)
{
	if (!m_bReady || !m_handle)
		return;

	pan = std::clamp<float>(pan, -1.0f, 1.0f);

	// apply to the active voice
	soloud->setPan(m_handle, pan);
}

void SoLoudSound::setLoop(bool loop)
{
	if (!m_bReady || !m_audioSource)
		return;

	if (cv::debug_snd.getBool())
		debugLog("setLoop {}\n", loop);

	m_bIsLooped = loop;

	// apply to the source
	m_audioSource->setLooping(loop);

	// apply to the active voice
	if (m_handle != 0)
		soloud->setLooping(m_handle, loop);
}

void SoLoudSound::setOverlayable(bool overlayable)
{
	m_bIsOverlayable = overlayable;
	if (m_audioSource)
		m_audioSource->setSingleInstance(!overlayable);
}

float SoLoudSound::getPosition()
{
	if (!m_bReady || !m_audioSource || !m_handle)
		return 0.0f;

	double streamLengthInSeconds = getSourceLengthInSeconds();
	if (streamLengthInSeconds <= 0.0)
		return 0.0f;

	double streamPositionInSeconds = getStreamPositionInSeconds();

	return std::clamp<float>(streamPositionInSeconds / streamLengthInSeconds, 0.0f, 1.0f);
}

// slightly tweaked interp algo from the SDL_mixer version, to smooth out position updates
unsigned long SoLoudSound::getPositionMS()
{
	if (!m_bReady || !m_audioSource || !m_handle)
		return 0;

	double streamPositionInSeconds = getStreamPositionInSeconds();

	const double currentTime = Timing::getTimeReal();
	const double streamPositionMS = streamPositionInSeconds * 1000.0;

	if (m_fLastSoLoudPositionTime <= 0.0 || !isPlaying())
	{
		m_fLastRawSoLoudPosition = streamPositionMS;
		m_fLastSoLoudPositionTime = currentTime;
		m_fSoLoudPositionRate = 1000.0 * getSpeed(); // Gameplay rate
		return (unsigned long)streamPositionMS;
	}

	// if the position changed, update our rate estimate
	if (m_fLastRawSoLoudPosition != streamPositionMS)
	{
		const double timeDelta = currentTime - m_fLastSoLoudPositionTime;

		// only update rate if enough time has passed
		if (timeDelta > 0.005)
		{
			double newRate;

			if (streamPositionMS >= m_fLastRawSoLoudPosition)
			{
				newRate = (streamPositionMS - m_fLastRawSoLoudPosition) / timeDelta;
			}
			else if (m_bIsLooped)
			{
				// handle loop wraparound
				auto length = static_cast<double>(getLengthMS());
				if (length > 0)
				{
					double wrappedChange = (length - m_fLastRawSoLoudPosition) + streamPositionMS;
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

		m_fLastRawSoLoudPosition = streamPositionMS;
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
		auto length = static_cast<double>(getLengthMS());
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

	const double lengthInMilliSeconds = getSourceLengthInSeconds() * 1000.0;
	// if (cv::debug_snd.getBool())
	// 	debugLog("lengthMS for {:s}: {:g}\n", m_sFilePath, lengthInMilliSeconds);
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
	if (!m_bReady || !m_handle)
		return 44100.0f;

	// get sample rate from active voice, unless we changed the frequency through pitch for streams, then just return our own frequency
	if (!m_bStream)
	{
		float currentFreq = soloud->getSamplerate(m_handle);
		if (currentFreq > 0)
			m_frequency = currentFreq;
	}

	return m_frequency;
}

bool SoLoudSound::isPlaying()
{
	if (!m_bReady || !m_handle)
		return false;

	// a sound is playing if the handle is valid and the sound isn't paused
	return soloud->isValidVoiceHandle(m_handle) && !soloud->getPause(m_handle);
}

bool SoLoudSound::isFinished()
{
	if (!m_bReady)
		return false;

	if (m_handle == 0)
		return true;

	// a sound is finished if the handle is no longer valid
	return !soloud->isValidVoiceHandle(m_handle);
}

void SoLoudSound::rebuild(UString newFilePath)
{
	m_sFilePath = newFilePath;
	resourceManager->reloadResource(this);
}

// soloud-specific accessors

double SoLoudSound::getStreamPositionInSeconds() const
{
	if (!m_audioSource)
		return m_fLastSoLoudPositionTime;
	if (m_bStream)
		return static_cast<SoLoud::SLFXStream *>(m_audioSource)->getRealStreamPosition();
	else
		return m_handle ? soloud->getStreamPosition(m_handle) : m_fLastSoLoudPositionTime;
}

double SoLoudSound::getSourceLengthInSeconds() const
{
	if (!m_audioSource)
		return 0.0;
	if (m_bStream)
		return static_cast<SoLoud::SLFXStream *>(m_audioSource)->getLength();
	else
		return static_cast<SoLoud::Wav *>(m_audioSource)->getLength();
}

#endif // MCENGINE_FEATURE_SOLOUD
