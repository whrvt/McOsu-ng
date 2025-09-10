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

#include <utility>

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
	std::vector<char> fileBuffer;
	const char *fileData{nullptr};
	size_t fileSize{0};

	if constexpr (Env::cfg(OS::WINDOWS))
	{
		McFile file(m_sFilePath);

		if (!file.canRead())
		{
			debugLog("Sound Error: Cannot open file {:s}\n", m_sFilePath);
			return;
		}

		fileSize = file.getFileSize();
		if (fileSize == 0)
		{
			debugLog("Sound Error: File is empty {:s}\n", m_sFilePath);
			return;
		}

		fileBuffer = file.takeFileBuffer();
		fileData = fileBuffer.data();
		if (!fileData)
		{
			debugLog("Sound Error: Failed to read file data {:s}\n", m_sFilePath);
			return;
		}
		// file is closed here
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
			m_fFrequency = stream->mBaseSamplerate;

			m_audioSource->setInaudibleBehavior(true, false); // keep ticking the sound if it goes to 0 volume, and don't kill it

			if (cv::debug_snd.getBool())
				debugLog("SoLoudSound: Created SLFXStream for {:s} with speed={:f}, pitch={:f}, looping={:s}, decoder={:s}\n", m_sFilePath.toUtf8(), m_fSpeed,
				         m_fPitch, m_bIsLooped ? "true" : "false", stream->getDecoder());
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
			m_fFrequency = wav->mBaseSamplerate;

			m_audioSource->setInaudibleBehavior(true, true); // keep ticking the sound if it goes to 0 volume, but do kill it if necessary
		}
		else
		{
			delete wav;
			debugLog("Sound Error: SoLoud::Wav::load() error {} on file {:s}\n", result, m_sFilePath.toUtf8());
			return;
		}
	}

	// only play one music track at a time, allow non-music sounds to have multiple instances playing at a time by default
	m_audioSource->setSingleInstance(m_bStream);

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

	m_fFrequency = 44100.0f;
	m_fPitch = 1.0f;
	m_fSpeed = 1.0f;
	m_fVolume = 1.0f;
	m_fLastPlayTime = 0.0f;
	m_bIgnored = false;
}

void SoLoudSound::setPosition(double percent)
{
	if (!m_bReady || !m_audioSource || !m_handle)
		return;

	percent = std::clamp<double>(percent, 0.0, 1.0);

	// calculate position based on the ORIGINAL timeline
	const double streamLengthInSeconds = getSourceLengthInSeconds();
	double positionInSeconds = streamLengthInSeconds * percent;

	if (cv::debug_snd.getBool())
		debugLog("seeking to {:.2f} percent (position: {}ms, length: {}ms)\n", percent, static_cast<unsigned long>(positionInSeconds * 1000),
		         static_cast<unsigned long>(streamLengthInSeconds * 1000));

	// seek
	soloud->seek(m_handle, positionInSeconds);

	// reset position interp vars
	m_interpolator.reset(getStreamPositionInSeconds() / 1000.0, Timing::getTimeReal(), getSpeed());
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

	if (cv::debug_snd.getBool())
		debugLog("seeking to {:g}ms (length: {:g}ms)\n", msD, streamLengthMS);

	// seek
	soloud->seek(m_handle, positionInSeconds);

	// reset position interp vars
	m_interpolator.reset(getStreamPositionInSeconds() / 1000.0, Timing::getTimeReal(), getSpeed());
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

	if (m_fSpeed != speed)
	{
		float previousSpeed = m_fSpeed;
		m_fSpeed = speed;

		// simply update the SLFXStream parameters
		auto *stream = static_cast<SoLoud::SLFXStream *>(m_audioSource);
		stream->setSpeedFactor(m_fSpeed);

		if (cv::debug_snd.getBool())
			debugLog("SoLoudSound: Speed change {:s}: {:f}->{:f} (stream, updated live)\n", m_sFilePath.toUtf8(), previousSpeed, m_fSpeed);
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

	if (m_fPitch != pitch)
	{
		float previousPitch = m_fPitch;
		m_fPitch = pitch;

		// simply update the SLFXStream parameters
		auto *stream = static_cast<SoLoud::SLFXStream *>(m_audioSource);
		stream->setPitchFactor(m_fPitch);

		if (cv::debug_snd.getBool())
			debugLog("SoLoudSound: Pitch change {:s}: {:f}->{:f} (stream, updated live)\n", m_sFilePath.toUtf8(), previousPitch, m_fPitch);
	}
}

void SoLoudSound::setFrequency(float frequency)
{
	if (!m_bReady || !m_audioSource)
		return;

	frequency = (frequency > 99.0f ? std::clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

	if (m_fFrequency != frequency)
	{
		if (frequency > 0)
		{
			if (m_bStream)
			{
				float pitchRatio = frequency / m_fFrequency;

				// apply the frequency change through pitch
				// this isn't the only or even a good way, but it does the trick
				setPitch(m_fPitch * pitchRatio);
			}
			else if (m_handle)
			{
				soloud->setSamplerate(m_handle, frequency);
			}
			m_fFrequency = frequency;
		}
		else // 0 means reset to default
		{
			if (m_bStream)
				setPitch(1.0f);
			else if (m_handle)
				soloud->setSamplerate(m_handle, frequency);
			m_fFrequency = m_audioSource->mBaseSamplerate;
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

	return m_interpolator.update(getStreamPositionInSeconds() * 1000.0, Timing::getTimeReal(), getSpeed(), isLooped(), getLengthMS(), isPlaying());
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

	return m_fSpeed;
}

float SoLoudSound::getPitch()
{
	if (!m_bReady)
		return 1.0f;

	return m_fPitch;
}

float SoLoudSound::getBPM()
{
	if (!m_bReady || !m_audioSource)
		return -1.0f;

	if (!m_bStream) // currently unsupported because it's unneeded for the current use case
	{
		debugLog("Programmer Error: tried to get BPM for a sample!\n");
		return -1.0f;
	}

	if (!soundEngine->shouldDetectBPM())
	{
		debugLog("tried to get BPM, but BPM detection wasn't enabled\n");
		return -1.0f;
	}

	// delegate to SLFXStream for bpm detection
	auto *stream = static_cast<SoLoud::SLFXStream *>(m_audioSource);
	float currentBPM = stream->getCurrentBPM();

	if (currentBPM > 0.0f)
		m_fCurrentBPM = currentBPM;

	return m_fCurrentBPM;
}

bool SoLoudSound::isPlaying()
{
	if (!m_bReady || !m_handle)
		return false;

	// a sound is playing if the handle is valid and the sound isn't paused
	if (!soloud->isValidVoiceHandle(m_handle))
		m_handle = 0;

	return !!m_handle && !soloud->getPause(m_handle);
}

bool SoLoudSound::isFinished()
{
	if (!m_bReady)
		return false;

	if (!m_handle)
		return true;

	// a sound is finished if the handle is no longer valid
	if (!soloud->isValidVoiceHandle(m_handle))
		m_handle = 0;

	return !m_handle;
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
		return m_interpolator.getLastInterpolatedPositionMS() / 1000.0;
	if (m_bStream)
		return static_cast<SoLoud::SLFXStream *>(m_audioSource)->getRealStreamPosition();
	else
		return m_handle ? soloud->getStreamPosition(m_handle) : m_interpolator.getLastInterpolatedPositionMS() / 1000.0;
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
