//======= Copyright (c) 2014, PG, 2024, CW, 2025, WH All rights reserved. =======//
//
// Purpose:		BASS-specific sound implementation
//
// $NoKeywords: $snd $bass
//===============================================================================//

#include "BassSound2.h"

#if defined(MCENGINE_FEATURE_BASS) && defined(MCENGINE_NEOSU_BASS_PORT_FINISHED)

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

extern ConVar debug_snd;
extern ConVar snd_speed_compensate_pitch;
extern ConVar snd_play_interp_duration;
extern ConVar snd_play_interp_ratio;
extern ConVar snd_file_min_size;
extern ConVar snd_freq;

ConVar snd_async_buffer("snd_async_buffer", 65536, FCVAR_NONE, "BASS_CONFIG_ASYNCFILE_BUFFER length in bytes. Set to 0 to disable.");

BassSound2::BassSound2(UString filepath, bool stream, bool threeD, bool loop, bool prescan) : Sound(filepath, stream, threeD, loop, prescan)
{
	m_sample = 0;
	m_stream = 0;
	m_bStream = stream;
	m_bIsLooped = loop;
	m_bIsOverlayable = !stream;
	m_fSpeed = 1.0f;
	m_fVolume = 1.0f;
}

std::vector<HCHANNEL> BassSound2::getActiveChannels()
{
	std::vector<HCHANNEL> channels;

	if (m_bStream)
	{
		if (BASS_Mixer_ChannelGetMixer(m_stream) != 0)
		{
			channels.push_back(m_stream);
		}
	}
	else
	{
		for (auto chan : m_vMixerChannels)
		{
			if (BASS_Mixer_ChannelGetMixer(chan) != 0)
			{
				channels.push_back(chan);
			}
		}

		// Only keep channels that are still playing
		m_vMixerChannels = channels;
	}

	return channels;
}

HCHANNEL BassSound2::getChannel()
{
	if (m_bStream)
	{
		return m_stream;
	}
	else
	{
		// If we want to be able to control samples after playing them, we
		// have to store them here, since bassmix only accepts DECODE streams.
		auto chan = BASS_SampleGetChannel(m_sample, BASS_SAMCHAN_STREAM | BASS_STREAM_DECODE);
		m_vMixerChannels.push_back(chan);
		return chan;
	}
}

void BassSound2::init()
{
	if (m_bIgnored || m_sFilePath.length() < 2 || !(m_bAsyncReady.load()))
		return;

	// HACKHACK: re-set some values to their defaults (only necessary because of the existence of rebuild())
	m_fSpeed = 1.0f;

	// error checking
	if (m_sample == 0 && m_stream == 0)
		debugLog(0xffdd3333,
		         R"(Couldn't load sound "{}", stream = {}, errorcode = {:d}, file = {})"
		         "\n",
		         m_sFilePath, (int)m_bStream, BASS_ErrorGetCode(), m_sFilePath);
	else
		m_bReady = true;
}

void BassSound2::initAsync()
{
	Sound::initAsync();
	if (m_bIgnored)
		return;

	constexpr DWORD unicodeFlag = Env::cfg(OS::WINDOWS) ? BASS_UNICODE : 0;

	if (m_bStream)
	{
		auto flags = BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT | BASS_STREAM_PRESCAN | unicodeFlag;
		if (snd_async_buffer.getInt() > 0)
			flags |= BASS_ASYNCFILE;

		m_stream = BASS_StreamCreateFile(false, m_sFilePath.plat_str(), 0, 0, flags);
		if (!m_stream)
		{
			debugLog("BASS_StreamCreateFile() returned error {:d} on file {:s}\n", BASS_ErrorGetCode(), m_sFilePath);
			return;
		}

		m_stream = BASS_FX_TempoCreate(m_stream, BASS_FX_FREESOURCE | BASS_STREAM_DECODE);
		if (!m_stream)
		{
			debugLog("BASS_FX_TempoCreate() returned error {:d} on file {:s}\n", BASS_ErrorGetCode(), m_sFilePath);
			return;
		}

		// Only compute the length once
		QWORD length = BASS_ChannelGetLength(m_stream, BASS_POS_BYTE);
		double lengthInSeconds = BASS_ChannelBytes2Seconds(m_stream, length);
		double lengthInMilliSeconds = lengthInSeconds * 1000.0;
		m_dLength = static_cast<int>(lengthInMilliSeconds);
	}
	else
	{
		auto flags = BASS_SAMPLE_FLOAT | unicodeFlag;

		m_sample = BASS_SampleLoad(false, m_sFilePath.plat_str(), 0, 0, 1, flags);
		if (!m_sample)
		{
			auto code = BASS_ErrorGetCode();
			if (code == BASS_ERROR_EMPTY)
			{
				debugLog("Sound: Ignoring empty file {:s}\n", m_sFilePath);
				return;
			}
			else
			{
				debugLog("BASS_SampleLoad() returned error {:d} on file {:s}\n", code, m_sFilePath);
				return;
			}
		}

		// Only compute the length once
		QWORD length = BASS_ChannelGetLength(m_sample, BASS_POS_BYTE);
		double lengthInSeconds = BASS_ChannelBytes2Seconds(m_sample, length);
		double lengthInMilliSeconds = lengthInSeconds * 1000.0;
		m_dLength = static_cast<int>(lengthInMilliSeconds);
	}

	m_bAsyncReady = true;
}

void BassSound2::destroy()
{
	if (!m_bReady)
		return;

	m_bStarted = false;
	m_bReady = false;
	m_bAsyncReady = false;
	m_fLastPlayTime = 0.0;
	m_dChannelCreationTime = 0.0;
	m_bPaused = false;
	m_iPausePositionMS = 0;

	if (m_bStream)
	{
		BASS_Mixer_ChannelRemove(m_stream);
		BASS_ChannelStop(m_stream);
		BASS_StreamFree(m_stream);
		m_stream = 0;
	}
	else
	{
		for (auto chan : m_vMixerChannels)
		{
			BASS_Mixer_ChannelRemove(chan);
			BASS_ChannelStop(chan);
			BASS_ChannelFree(chan);
		}
		m_vMixerChannels.clear();

		BASS_SampleStop(m_sample);
		BASS_SampleFree(m_sample);
		m_sample = 0;
	}
}

void BassSound2::setPosition(double percent)
{
	unsigned int ms = std::clamp<double>(percent, 0.0, 1.0) * m_dLength;
	setPositionMS(ms);
	return;
}

void BassSound2::setPositionMS(unsigned long ms)
{
	if (!m_bReady || ms > getLengthMS())
		return;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called setPositionMS on a sample!");
		return;
	}

	QWORD target_pos = BASS_ChannelSeconds2Bytes(m_stream, ms / 1000.0);
	if (target_pos < 0)
	{
		debugLog("setPositionMS: error {:d} while calling BASS_ChannelSeconds2Bytes\n", BASS_ErrorGetCode());
		return;
	}

	// Naively setting position breaks with the current BASS version (& addons).
	//
	// BASS_STREAM_PRESCAN no longer seems to work, so our only recourse is to use the BASS_POS_DECODETO
	// flag which renders the whole audio stream up until the requested seek point.
	//
	// The downside of BASS_POS_DECODETO is that it can only seek forward... furthermore, we can't
	// just seek to 0 before seeking forward again, since BASS_Mixer_ChannelGetPosition breaks
	// in that case. So, our only recourse is to just reload the whole fucking stream just for seeking.

	bool was_playing = isPlaying();
	auto pos = getPositionMS();
	if (pos <= ms)
	{
		// Lucky path, we can just seek forward and be done
		if (isPlaying())
		{
			if (!BASS_Mixer_ChannelSetPosition(m_stream, target_pos, BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_MIXER_RESET))
			{
				if (debug_snd.getBool())
				{
					debugLog("BassSound2::setPositionMS( {} ) BASS_ChannelSetPosition() error {} on file {:s}\n", ms, BASS_ErrorGetCode(), m_sFilePath);
				}
			}

			m_fLastPlayTime = m_dChannelCreationTime - ((double)ms / 1000.0);
		}
		else
		{
			if (!BASS_ChannelSetPosition(m_stream, target_pos, BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_FLUSH))
			{
				if (debug_snd.getBool())
				{
					debugLog("BassSound2::setPositionMS( {} ) BASS_ChannelSetPosition() error {} on file {:s}\n", ms, BASS_ErrorGetCode(), m_sFilePath);
				}
			}
		}
	}
	else
	{
		// Unlucky path, we have to reload the stream
		auto pan = getPan();
		auto loop = isLooped();
		auto speed = getSpeed();

		reload();

		setSpeed(speed);
		setPan(pan);
		setLoop(loop);
		m_bPaused = true;
		m_iPausePositionMS = ms;

		if (!BASS_ChannelSetPosition(m_stream, target_pos, BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_FLUSH))
		{
			if (debug_snd.getBool())
			{
				debugLog("BassSound2::setPositionMS( {} ) BASS_ChannelSetPosition() error {} on file {:s}\n", ms, BASS_ErrorGetCode(), m_sFilePath);
			}
		}

		// if (was_playing)
		// {
		// 	osu->music_unpause_scheduled = true;
		// }
	}
}

// Inaccurate but fast seeking, to use at song select
void BassSound2::setPositionMS_fast(unsigned int ms)
{
	if (!m_bReady || ms > getLengthMS())
		return;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called setPositionMS_fast on a sample!");
		return;
	}

	QWORD target_pos = BASS_ChannelSeconds2Bytes(m_stream, ms / 1000.0);
	if (target_pos < 0)
	{
		debugLog("setPositionMS_fast: error {:d} while calling BASS_ChannelSeconds2Bytes\n", BASS_ErrorGetCode());
		return;
	}

	if (isPlaying())
	{
		if (!BASS_Mixer_ChannelSetPosition(m_stream, target_pos, BASS_POS_BYTE | BASS_POS_MIXER_RESET))
		{
			if (debug_snd.getBool())
			{
				debugLog("BassSound2::setPositionMS_fast( {} ) BASS_ChannelSetPosition() error {} on file {:s}\n", ms, BASS_ErrorGetCode(), m_sFilePath);
			}
		}

		m_fLastPlayTime = m_dChannelCreationTime - ((double)ms / 1000.0);
	}
	else
	{
		if (!BASS_ChannelSetPosition(m_stream, target_pos, BASS_POS_BYTE | BASS_POS_FLUSH))
		{
			if (debug_snd.getBool())
			{
				debugLog("BassSound2::setPositionMS( {} ) BASS_ChannelSetPosition() error {} on file {:s}\n", ms, BASS_ErrorGetCode(), m_sFilePath);
			}
		}
	}
}

void BassSound2::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 2.0f);

	for (auto channel : getActiveChannels())
	{
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, m_fVolume);
	}
}

void BassSound2::setSpeed(float speed)
{
	if (!m_bReady)
		return;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called setSpeed on a sample!");
		return;
	}

	speed = std::clamp<float>(speed, 0.05f, 50.0f);

	float freq = snd_freq.getFloat();
	BASS_ChannelGetAttribute(m_stream, BASS_ATTRIB_FREQ, &freq);

	BASS_ChannelSetAttribute(m_stream, (snd_speed_compensate_pitch.getBool() ? BASS_ATTRIB_TEMPO : BASS_ATTRIB_TEMPO_FREQ),
	                         (snd_speed_compensate_pitch.getBool() ? (speed - 1.0f) * 100.0f : speed * freq));

	m_fSpeed = speed;
}

float BassSound2::getPitch()
{
	if (!m_bReady)
		return 1.0f;

	const SOUNDHANDLE handle = getChannel();

	float pitch = 0.0f;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_TEMPO_PITCH, &pitch);

	return ((pitch / 60.0f) + 1.0f);
}

void BassSound2::setPitch(float pitch)
{
	if (!m_bReady)
		return;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called setPitch on a sample!");
		return;
	}

	pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

	for (auto channel : getActiveChannels())
	{
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_TEMPO_PITCH, (pitch - 1.0f) * 60.0f);
	}
}

void BassSound2::setFrequency(float frequency)
{
	if (!m_bReady)
		return;

	frequency = (frequency > 99.0f ? std::clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

	for (auto channel : getActiveChannels())
	{
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_FREQ, frequency);
	}
}

void BassSound2::setPan(float pan)
{
	if (!m_bReady)
		return;

	m_fPan = std::clamp<float>(pan, -1.0f, 1.0f);

	for (auto channel : getActiveChannels())
	{
		BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, m_fPan);
	}
}

void BassSound2::setLoop(bool loop)
{
	if (!m_bReady)
		return;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called setLoop on a sample!");
		return;
	}

	m_bIsLooped = loop;
	BASS_ChannelFlags(m_stream, m_bIsLooped ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}

float BassSound2::getPosition()
{
	if (!m_bReady)
		return 0.f;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called getPosition on a sample!");
		return 0.f;
	}
	if (m_bPaused)
	{
		return (double)m_iPausePositionMS / (double)m_dLength;
	}

	QWORD lengthBytes = BASS_ChannelGetLength(m_stream, BASS_POS_BYTE);
	if (lengthBytes < 0)
	{
		// The stream ended and got freed by BASS_STREAM_AUTOFREE -> invalid handle!
		return 1.f;
	}

	QWORD positionBytes = 0;
	if (isPlaying())
	{
		positionBytes = BASS_Mixer_ChannelGetPosition(m_stream, BASS_POS_BYTE);
	}
	else
	{
		positionBytes = BASS_ChannelGetPosition(m_stream, BASS_POS_BYTE);
	}

	const float position = (float)((double)(positionBytes) / (double)(lengthBytes));
	return position;
}

unsigned long BassSound2::getPositionMS()
{
	if (!m_bReady)
		return 0;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called getPositionMS on a sample!");
		return 0;
	}
	if (m_bPaused)
	{
		return m_iPausePositionMS;
	}

	QWORD positionBytes = 0;
	if (isPlaying())
	{
		positionBytes = BASS_Mixer_ChannelGetPosition(m_stream, BASS_POS_BYTE);
	}
	else
	{
		positionBytes = BASS_ChannelGetPosition(m_stream, BASS_POS_BYTE);
	}
	if (positionBytes < 0)
	{
		// The stream ended and got freed by BASS_STREAM_AUTOFREE -> invalid handle!
		return m_dLength;
	}

	double positionInSeconds = BASS_ChannelBytes2Seconds(m_stream, positionBytes);
	double positionInMilliSeconds = positionInSeconds * 1000.0;
	unsigned int positionMS = (unsigned int)positionInMilliSeconds;
	if (!isPlaying())
	{
		return positionMS;
	}

	// special case: a freshly started channel position jitters, lerp with engine time over a set duration to smooth
	// things over
	double interpDuration = snd_play_interp_duration.getFloat();
	if (interpDuration <= 0.0)
		return positionMS;

	double channel_age = engine->getTime() - m_dChannelCreationTime;
	if (channel_age >= interpDuration)
		return positionMS;

	double speedMultiplier = getSpeed();
	double delta = channel_age * speedMultiplier;
	double interp_ratio = snd_play_interp_ratio.getFloat();
	if (delta < interpDuration)
	{
		delta = (engine->getTime() - m_fLastPlayTime) * speedMultiplier;
		double lerpPercent = std::clamp<double>(((delta / interpDuration) - interp_ratio) / (1.0 - interp_ratio), 0.0, 1.0);
		positionMS = static_cast<unsigned int>(std::lerp(delta * 1000.0, static_cast<double>(positionMS), static_cast<double>(lerpPercent)));
	}

	return positionMS;
}

unsigned long BassSound2::getLengthMS()
{
	if (!m_bReady)
		return 0;
	return m_dLength;
}

float BassSound2::getSpeed()
{
	return m_fSpeed;
}

float BassSound2::getFrequency()
{
	auto default_freq = snd_freq.getFloat();
	if (!m_bReady)
		return default_freq;
	if (!m_bStream)
	{
		engine->showMessageError("Programmer Error", "Called getFrequency on a sample!");
		return default_freq;
	}

	float frequency = default_freq;
	BASS_ChannelGetAttribute(m_stream, BASS_ATTRIB_FREQ, &frequency);
	return frequency;
}

bool BassSound2::isPlaying()
{
	return m_bReady && m_bStarted && !m_bPaused && !getActiveChannels().empty();
}

bool BassSound2::isFinished()
{
	return m_bReady && m_bStarted && !isPlaying();
}

void BassSound2::rebuild(UString newFilePath)
{
	m_sFilePath = newFilePath;
	reload();
}

#endif
