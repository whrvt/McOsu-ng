//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		BASS-specific sound implementation
//
// $NoKeywords: $snd $bass
//===============================================================================//

#include "EngineFeatures.h"

#ifdef MCENGINE_FEATURE_BASS

#include "BassSound.h"
#include "BassSoundEngine.h"

#include "BassManager.h"

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

extern ConVar debug_snd;
extern ConVar snd_speed_compensate_pitch;
extern ConVar snd_play_interp_duration;
extern ConVar snd_play_interp_ratio;
extern ConVar snd_wav_file_min_size;

BassSound::BassSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan) : Sound(filepath, stream, threeD, loop, prescan)
{
	m_HSTREAM = 0;
	m_HSTREAMBACKUP = 0;
	m_HCHANNEL = 0;
	m_HCHANNELBACKUP = 0;

	m_fActualSpeedForDisabledPitchCompensation = 1.0f;

	m_iPrevPosition = 0;

	m_wasapiSampleBuffer = nullptr;
	m_iWasapiSampleBufferSize = 0;

	if constexpr (Env::cfg(AUD::WASAPI))
		m_danglingWasapiStreams.reserve(32);
}

BassSound::~BassSound()
{
	destroy();
}

void BassSound::init()
{
	if (m_sFilePath.length() < 2 || !(m_bAsyncReady.load()))
		return;

	// reset values to defaults (needed for rebuild())
	m_fActualSpeedForDisabledPitchCompensation = 1.0f;

	// error checking
	if (m_HSTREAM == 0 && m_iWasapiSampleBufferSize < 1)
	{
		debugLog(0xffdd3333, "Couldn't load sound \"{}\", stream = {}, errorcode = {:d}, file = {}\n", m_sFilePath, (int)m_bStream, BASS_ErrorGetCode(),
		         m_sFilePath);
	}
	else
	{
		m_bReady = true;
	}
}

void BassSound::initAsync()
{
	if (ResourceManager::debug_rm->getBool())
		debugLog("Resource Manager: Loading {:s}\n", m_sFilePath);

	// hACKHACK: workaround for BASS crashes on malformed WAV files
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
				if (debug_snd.getBool())
					debugLog("Sound: Ignoring malformed/corrupt WAV file ({}) {:s}\n", (int)wavFile.getFileSize(), m_sFilePath);
				return;
			}
		}
	}

	// create the sound
	constexpr DWORD unicodeFlag = Env::cfg(OS::WINDOWS) ? BASS_UNICODE : 0;

	if (m_bStream)
	{
		DWORD extraStreamCreateFileFlags = 0;
		DWORD extraFXTempoCreateFlags = 0;

		if constexpr (Env::cfg(OS::WINDOWS, AUD::WASAPI))
		{
			extraStreamCreateFileFlags |= BASS_SAMPLE_FLOAT;
			extraFXTempoCreateFlags |= BASS_STREAM_DECODE;
		}

		m_HSTREAM = BASS_StreamCreateFile(BASS_FILE_NAME, m_sFilePath.plat_str(), 0, 0,
		                                  (m_bPrescan ? BASS_STREAM_PRESCAN : 0) | BASS_STREAM_DECODE | extraStreamCreateFileFlags | unicodeFlag);

		m_HSTREAM = BASS_FX_TempoCreate(m_HSTREAM, BASS_FX_TEMPO_ALGO_SHANNON | BASS_FX_FREESOURCE | extraFXTempoCreateFlags);

		BASS_ChannelSetAttribute(m_HSTREAM, BASS_ATTRIB_TEMPO_OPTION_USE_QUICKALGO, false);
		BASS_ChannelSetAttribute(m_HSTREAM, BASS_ATTRIB_TEMPO_OPTION_OVERLAP_MS, 4.0f);
		BASS_ChannelSetAttribute(m_HSTREAM, BASS_ATTRIB_TEMPO_OPTION_SEQUENCE_MS, 30.0f);
		m_HCHANNELBACKUP = m_HSTREAM;
	}
	else // not a stream
	{
		if constexpr (Env::cfg(OS::WINDOWS, AUD::WASAPI))
		{
			McFile file(m_sFilePath);
			if (file.canRead())
			{
				m_iWasapiSampleBufferSize = file.getFileSize();
				if (m_iWasapiSampleBufferSize > 0)
				{
					m_wasapiSampleBuffer = new char[file.getFileSize()];
					memcpy(m_wasapiSampleBuffer, file.readFile(), file.getFileSize());
				}
			}
			else
			{
				debugLog("Sound Error: Couldn't file.canRead() on file {:s}\n", m_sFilePath);
			}
		}
		else
		{
			m_HSTREAM =
			    BASS_SampleLoad(FALSE, m_sFilePath.plat_str(), 0, 0, 5,
			                    (m_bIsLooped ? BASS_SAMPLE_LOOP : 0) | (m_bIs3d ? BASS_SAMPLE_3D | BASS_SAMPLE_MONO : 0) | BASS_SAMPLE_OVER_POS | unicodeFlag);
		}

		m_HSTREAMBACKUP = m_HSTREAM; // needed for proper cleanup for FX HSAMPLES

		if (m_HSTREAM == 0)
		{
			auto code = BASS_ErrorGetCode();
			if (code && (debug_snd.getBool() || (code != BASS_ERROR_NOTAUDIO && code != BASS_ERROR_EMPTY)))
				debugLog("Sound Error: BASS_SampleLoad() error {} on file {:s}\n", code, m_sFilePath);
		}
	}
	m_bAsyncReady = true;
}

void BassSound::destroy()
{
	if (!m_bReady)
		return;

	m_bReady = false;

	if (m_bStream)
	{
#ifdef MCENGINE_FEATURE_BASS_WASAPI
		BASS_Mixer_ChannelRemove(m_HSTREAM);
#endif
		BASS_StreamFree(m_HSTREAM); // FX (but with BASS_FX_FREESOURCE)
	}
	else
	{
		if (m_HCHANNEL)
			BASS_ChannelStop(m_HCHANNEL);
		if (m_HSTREAMBACKUP)
			BASS_SampleFree(m_HSTREAMBACKUP);

		if constexpr (Env::cfg(AUD::WASAPI))
		{
			// nOTE: must guarantee that all channels are stopped before memory is deleted!
			for (const SOUNDHANDLE danglingWasapiStream : m_danglingWasapiStreams)
			{
				BASS_StreamFree(danglingWasapiStream);
			}
			m_danglingWasapiStreams.clear();

			if (m_wasapiSampleBuffer != nullptr)
			{
				delete[] m_wasapiSampleBuffer;
				m_wasapiSampleBuffer = nullptr;
			}
		}
	}

	m_HSTREAM = 0;
	m_HSTREAMBACKUP = 0;
	m_HCHANNEL = 0;
}

SOUNDHANDLE BassSound::getHandle()
{
	// stream files: directly return HSTREAM
	if (m_bStream)
		return m_HSTREAM;

	// non-stream files: create channel if needed, or return existing if not overlayable
	if constexpr (Env::cfg(AUD::WASAPI))
	{
		if (m_HCHANNEL == 0 || m_bIsOverlayable)
		{
			cleanupWasapiStreams();
			m_HCHANNEL = createWasapiChannel();
		}
	}
	else
	{
		if (m_HCHANNEL != 0 && !m_bIsOverlayable)
			return m_HCHANNEL;

		m_HCHANNEL = BASS_SampleGetChannel(m_HSTREAMBACKUP, FALSE);
		m_HCHANNELBACKUP = m_HCHANNEL;

		if (m_HCHANNEL == 0)
		{
			debugLog(0xffdd3333, "Couldn't BASS_SampleGetChannel({}, FALSE) on \"{}\", stream = {}, errorcode = {:d}, file = {}\n", m_HSTREAMBACKUP, m_sFilePath,
			         (int)m_bStream, BASS_ErrorGetCode(), m_sFilePath);
		}
		else
		{
			BASS_ChannelSetAttribute(m_HCHANNEL, BASS_ATTRIB_VOL, m_fVolume);
		}
	}
	return m_HCHANNEL;
}

void BassSound::setPosition(double percent)
{
	if (!m_bReady)
		return;

	percent = std::clamp<double>(percent, 0.0, 1.0);

	const SOUNDHANDLE handle = (m_HCHANNELBACKUP != 0 ? m_HCHANNELBACKUP : getHandle());
	const QWORD length = BASS_ChannelGetLength(handle, BASS_POS_BYTE);
	const double lengthInSeconds = BASS_ChannelBytes2Seconds(handle, length);

	updatePlayInterpolationTime(lengthInSeconds * percent);

	const BOOL res = BASS_ChannelSetPosition(handle, (QWORD)((double)(length)*percent), BASS_POS_BYTE);
	if (!res && debug_snd.getBool())
		debugLog("position {:f} BASS_ChannelSetPosition() error {} on file {:s} (handle: {})\n", percent, BASS_ErrorGetCode(), m_sFilePath, handle);
}

void BassSound::setPositionMS(unsigned long ms)
{
	if (!m_bReady || ms > getLengthMS())
		return;

	const SOUNDHANDLE handle = getHandle();
	const QWORD position = BASS_ChannelSeconds2Bytes(handle, ms / 1000.0);

	updatePlayInterpolationTime((double)ms / 1000.0);

	const BOOL res = BASS_ChannelSetPosition(handle, position, BASS_POS_BYTE);
	if (!res && debug_snd.getBool())
		debugLog("ms {} BASS_ChannelSetPosition() error {} on file {:s}\n", ms, BASS_ErrorGetCode(), m_sFilePath);
}

void BassSound::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	if (!m_bIsOverlayable)
		setBassAttribute(BASS_ATTRIB_VOL, m_fVolume, "volume");
}

void BassSound::setSpeed(float speed)
{
	if (!m_bReady)
		return;

	speed = std::clamp<float>(speed, 0.05f, 50.0f);

	const SOUNDHANDLE handle = getHandle();
	float originalFreq = convar->getConVarByName("snd_freq")->getFloat();
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_FREQ, &originalFreq);

	const DWORD attrib = snd_speed_compensate_pitch.getBool() ? BASS_ATTRIB_TEMPO : BASS_ATTRIB_TEMPO_FREQ;
	const float value = snd_speed_compensate_pitch.getBool() ? (speed - 1.0f) * 100.0f : speed * originalFreq;

	setBassAttribute(attrib, value, "speed");
	m_fActualSpeedForDisabledPitchCompensation = speed;
}

void BassSound::setPitch(float pitch)
{
	if (!m_bReady)
		return;

	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);
	setBassAttribute(BASS_ATTRIB_TEMPO_PITCH, (pitch - 1.0f) * 60.0f, "pitch");
}

void BassSound::setFrequency(float frequency)
{
	if (!m_bReady)
		return;

	frequency = (frequency > 99.0f ? std::clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);
	setBassAttribute(BASS_ATTRIB_FREQ, frequency, "frequency");
}

void BassSound::setPan(float pan)
{
	if (!m_bReady)
		return;

	pan = std::clamp<float>(pan, -1.0f, 1.0f);
	setBassAttribute(BASS_ATTRIB_PAN, pan, "pan");
}

void BassSound::setLoop(bool loop)
{
	if (!m_bReady)
		return;

	m_bIsLooped = loop;

	const SOUNDHANDLE handle = getHandle();
	BASS_ChannelFlags(handle, m_bIsLooped ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}

float BassSound::getPosition()
{
	if (!m_bReady)
		return 0.0f;

	const SOUNDHANDLE handle = (m_HCHANNELBACKUP != 0 ? m_HCHANNELBACKUP : getHandle());
	const QWORD lengthBytes = BASS_ChannelGetLength(handle, BASS_POS_BYTE);
	const QWORD positionBytes = BASS_ChannelGetPosition(handle, BASS_POS_BYTE);

	return (float)((double)(positionBytes) / (double)(lengthBytes));
}

unsigned long BassSound::getPositionMS()
{
	if (!m_bReady)
		return 0;

	const SOUNDHANDLE handle = getHandle();
	const QWORD position = BASS_ChannelGetPosition(handle, BASS_POS_BYTE);
	const double positionInSeconds = BASS_ChannelBytes2Seconds(handle, position);
	const auto positionMS = static_cast<unsigned long>(positionInSeconds * 1000.0);

	// special case: freshly started channel position jitters, lerp with engine time over set duration
	const double interpDuration = snd_play_interp_duration.getFloat();
	const unsigned long interpDurationMS = interpDuration * 1000;
	if (interpDuration > 0.0 && positionMS < interpDurationMS)
	{
		const float speedMultiplier = getSpeed();
		const double delta = (engine->getTime() - m_fLastPlayTime) * speedMultiplier;
		if (m_fLastPlayTime > 0.0 && delta < interpDuration && isPlaying())
		{
			const double lerpPercent =
			    std::clamp<double>(((delta / interpDuration) - snd_play_interp_ratio.getFloat()) / (1.0 - snd_play_interp_ratio.getFloat()), 0.0, 1.0);
			return static_cast<unsigned long>(std::lerp(delta * 1000.0, (double)positionMS, lerpPercent));
		}
	}

	return positionMS;
}

unsigned long BassSound::getLengthMS()
{
	if (!m_bReady)
		return 0;

	const SOUNDHANDLE handle = getHandle();
	const QWORD length = BASS_ChannelGetLength(handle, BASS_POS_BYTE);
	const double lengthInSeconds = BASS_ChannelBytes2Seconds(handle, length);

	return static_cast<unsigned long>(lengthInSeconds * 1000.0);
}

float BassSound::getSpeed()
{
	if (!m_bReady)
		return 1.0f;

	// special case: disabled pitch compensation means BASS returns 1.0x always
	if (!snd_speed_compensate_pitch.getBool())
		return m_fActualSpeedForDisabledPitchCompensation;

	const SOUNDHANDLE handle = getHandle();
	float speed = 0.0f;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_TEMPO, &speed);

	return ((speed / 100.0f) + 1.0f);
}

float BassSound::getPitch()
{
	if (!m_bReady)
		return 1.0f;

	const SOUNDHANDLE handle = getHandle();
	float pitch = 0.0f;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_TEMPO_PITCH, &pitch);

	return ((pitch / 60.0f) + 1.0f);
}

float BassSound::getFrequency()
{
	const float defaultFreq = convar->getConVarByName("snd_freq")->getFloat();
	if (!m_bReady)
		return defaultFreq;

	const SOUNDHANDLE handle = getHandle();
	float frequency = defaultFreq;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_FREQ, &frequency);

	return frequency;
}

bool BassSound::isPlaying()
{
	if (!m_bReady)
		return false;

	const SOUNDHANDLE handle = getHandle();

#ifdef MCENGINE_FEATURE_BASS_WASAPI
	return BASS_ChannelIsActive(handle) == BASS_ACTIVE_PLAYING && ((!m_bStream && m_bIsOverlayable) || BASS_Mixer_ChannelGetMixer(handle) != 0);
#else
	return BASS_ChannelIsActive(handle) == BASS_ACTIVE_PLAYING;
#endif
}

bool BassSound::isFinished()
{
	if (!m_bReady)
		return false;

	const SOUNDHANDLE handle = getHandle();
	return BASS_ChannelIsActive(handle) == BASS_ACTIVE_STOPPED;
}

void BassSound::rebuild(UString newFilePath)
{
	m_sFilePath = newFilePath;
	resourceManager->reloadResource(this);
}

// helper methods

bool BassSound::setBassAttribute(DWORD attrib, float value, const char *debugName)
{
	const SOUNDHANDLE handle = getHandle();
	const BOOL result = BASS_ChannelSetAttribute(handle, attrib, value);

	if (!result && debug_snd.getBool() && debugName)
		debugLog("BASS_ChannelSetAttribute({}) failed with error {} on file {:s}\n", debugName, BASS_ErrorGetCode(), m_sFilePath);

	return result != FALSE;
}

void BassSound::updatePlayInterpolationTime(double positionSeconds)
{
	if (positionSeconds < snd_play_interp_duration.getFloat())
		m_fLastPlayTime = engine->getTime() - positionSeconds;
	else
		m_fLastPlayTime = 0.0;
}

void BassSound::cleanupWasapiStreams()
{
	if constexpr (!Env::cfg(AUD::WASAPI))
		return;

	for (size_t i = 0; i < m_danglingWasapiStreams.size(); i++)
	{
		if (BASS_ChannelIsActive(m_danglingWasapiStreams[i]) != BASS_ACTIVE_PLAYING)
		{
			BASS_StreamFree(m_danglingWasapiStreams[i]);
			m_danglingWasapiStreams.erase(m_danglingWasapiStreams.begin() + i);
			i--;
		}
	}
}

SOUNDHANDLE BassSound::createWasapiChannel()
{
	if constexpr (!Env::cfg(AUD::WASAPI))
		return 0;

	SOUNDHANDLE channel = BASS_StreamCreateFile(TRUE, m_wasapiSampleBuffer, 0, m_iWasapiSampleBufferSize,
	                                            BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_UNICODE | (m_bIsLooped ? BASS_SAMPLE_LOOP : 0));

	if (channel == 0)
	{
		debugLog("BASS_StreamCreateFile() error {}\n", BASS_ErrorGetCode());
	}
	else
	{
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, m_fVolume);
		m_danglingWasapiStreams.push_back(channel);
	}

	return channel;
}

#endif // MCENGINE_FEATURE_BASS
