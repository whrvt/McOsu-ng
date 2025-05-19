//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoundTouch filter for independent pitch/speed control (for use with SoLoud)
//
// $NoKeywords: $snd $soloud $soundtouch
//================================================================================//

#include "SoundTouchFilter.h"

#include <cstddef>

#ifdef MCENGINE_FEATURE_SOLOUD

#include "SoLoudSoundEngine.h"

#include "ConVar.h"
#include "Engine.h"

extern ConVar osu_universal_offset_hardcoded; // TODO: literally just return a position with this offset instead of messing with convars (maybe?)
ConVar snd_enable_auto_offset("snd_enable_auto_offset", true, FCVAR_NONE, "Control automatic offset calibration for SoLoud + rate change");

#ifdef _DEBUG
ConVar snd_st_debug("snd_st_debug", false, FCVAR_NONE, "Enable detailed SoundTouch filter logging");
#define ST_DEBUG_ENABLED snd_st_debug.getBool()
#else
#define ST_DEBUG_ENABLED 0
#endif

#define ST_DEBUG_LOG(...)                                                                                                                                                          \
	if (ST_DEBUG_ENABLED)                                                                                                                                                          \
	{                                                                                                                                                                              \
		debugLog(__VA_ARGS__);                                                                                                                                                     \
	}

namespace SoLoud
{
//-------------------------------------------------------------------------
// SoundTouchFilter implementation
//-------------------------------------------------------------------------

SoundTouchFilter::SoundTouchFilter() : mSource(nullptr), mSpeedFactor(1.0f), mPitchFactor(1.0f)
{
	ST_DEBUG_LOG("SoundTouchFilter: Constructor called\n");
}

SoundTouchFilter::~SoundTouchFilter()
{
	ST_DEBUG_LOG("SoundTouchFilter: Destructor called\n");
}

result SoundTouchFilter::setSource(AudioSource *aSource)
{
	if (!aSource)
		return INVALID_PARAMETER;

	// copy source params
	mSource = aSource;
	mChannels = mSource->mChannels;
	mBaseSamplerate = mSource->mBaseSamplerate;
	mFlags = mSource->mFlags;

	ST_DEBUG_LOG("SoundTouchFilter: Set source with %d channels at %f Hz, mFlags=%x\n", mChannels, mBaseSamplerate, mFlags);

	return SO_NO_ERROR;
}

AudioSourceInstance *SoundTouchFilter::createInstance()
{
	if (!mSource)
		return nullptr;

	ST_DEBUG_LOG("SoundTouchFilter: Creating instance with speed=%f, pitch=%f\n", mSpeedFactor, mPitchFactor);

	return new SoundTouchFilterInstance(this);
}

void SoundTouchFilter::setSpeedFactor(float aSpeed)
{
	ST_DEBUG_LOG("SoundTouchFilter: Speed changed from %f to %f\n", mSpeedFactor, aSpeed);
	mSpeedFactor = aSpeed;
}

void SoundTouchFilter::setPitchFactor(float aPitch)
{
	ST_DEBUG_LOG("SoundTouchFilter: Pitch changed from %f to %f\n", mPitchFactor, aPitch);
	mPitchFactor = aPitch;
}

float SoundTouchFilter::getSpeedFactor() const
{
	return mSpeedFactor;
}

float SoundTouchFilter::getPitchFactor() const
{
	return mPitchFactor;
}

//-------------------------------------------------------------------------
// SoundTouchFilterInstance implementation
//-------------------------------------------------------------------------

SoundTouchFilterInstance::SoundTouchFilterInstance(SoundTouchFilter *aParent)
    : mParent(aParent), mSourceInstance(nullptr), mSoundTouch(nullptr), mBuffer(nullptr), mBufferSize(0), mInterleavedBuffer(nullptr), mInterleavedBufferSize(0),
      mProcessingCounter(0), mTotalSamplesProcessed(0), mSoundTouchPitch(0.0f), mSoundTouchSpeed(0.0f), mOggFrameBuffer(nullptr), mOggFrameBufferSize(0), mOggSamplesInBuffer(0)
{
	ST_DEBUG_LOG("SoundTouchFilterInstance: Constructor called\n");

	// create source instance
	if (mParent && mParent->mSource)
	{
		mSourceInstance = mParent->mSource->createInstance();

		if (mSourceInstance)
		{
			// initialize the source instance with the parent source
			mSourceInstance->init(*mParent->mSource, 0);

			// initialize with properties from parent filter
			mChannels = mParent->mChannels;
			mBaseSamplerate = mParent->mBaseSamplerate;
			mFlags = mParent->mFlags;
			mSetRelativePlaySpeed = mParent->mSpeedFactor;
			mOverallRelativePlaySpeed = mParent->mSpeedFactor;

			ST_DEBUG_LOG("SoundTouchFilterInstance: Creating with %d channels at %f Hz, mFlags=%x\n", mChannels, mBaseSamplerate, mFlags);

			// initialize SoundTouch
			mSoundTouch = new soundtouch::SoundTouch();
			if (mSoundTouch)
			{
				mSoundTouch->setSampleRate((uint)mBaseSamplerate);
				mSoundTouch->setChannels(mChannels);

				// quality settings pulled out of my ass, there is NO documentation for this library...
				mSoundTouch->setSetting(SETTING_USE_AA_FILTER, 1);
				mSoundTouch->setSetting(SETTING_AA_FILTER_LENGTH, 32);
				mSoundTouch->setSetting(SETTING_USE_QUICKSEEK, 0);
				mSoundTouch->setSetting(SETTING_SEQUENCE_MS, 30); // wtf should these numbers be?
				mSoundTouch->setSetting(SETTING_SEEKWINDOW_MS, 15);
				mSoundTouch->setSetting(SETTING_OVERLAP_MS, 4);

				// set the actual speed and pitch factors
				mSoundTouch->setTempo(mParent->mSpeedFactor);
				mSoundTouch->setPitch(mParent->mPitchFactor);

				mSoundTouchSpeed = mParent->mSpeedFactor;
				mSoundTouchPitch = mParent->mPitchFactor;

				ST_DEBUG_LOG("SoundTouch: Initialized with speed=%f, pitch=%f\n", mParent->mSpeedFactor, mParent->mPitchFactor);
				ST_DEBUG_LOG("SoundTouch: Version: %s\n", mSoundTouch->getVersionString());

				// pre-fill
				primeBuffers();

				// estimate processing delay, somewhat following the SoundTouch header idea
				if (snd_enable_auto_offset.getBool())
				{
					int initialLatencyInSamples = mSoundTouch->getSetting(SETTING_INITIAL_LATENCY);
					int nominalInputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_INPUT_SEQUENCE);
					int nominalOutputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_OUTPUT_SEQUENCE);

					float latencyInMs = (initialLatencyInSamples / (float)mBaseSamplerate) * 1000.0f;

					// add buffer compensation factor (~half of processing window)
					float processingBufferDelay = 0.0f;
					if (nominalInputSeq > 0 && nominalOutputSeq > 0)
					{
						processingBufferDelay = ((nominalInputSeq - nominalOutputSeq) / 2.0f / (float)mBaseSamplerate) * 1000.0f;
					}

					float totalOffset = std::clamp<float>(-(latencyInMs + processingBufferDelay), -200.0f, 0.0f);

					ST_DEBUG_LOG("SoundTouch: Calculated universal offset = %.2f ms (latency: %.2f, buffer: %.2f)\n", totalOffset, latencyInMs, processingBufferDelay);

					osu_universal_offset_hardcoded.setValue(osu_universal_offset_hardcoded.getDefaultFloat() + totalOffset);
				}
			}
		}
	}
}

SoundTouchFilterInstance::~SoundTouchFilterInstance()
{
	osu_universal_offset_hardcoded.setValue(osu_universal_offset_hardcoded.getDefaultFloat());
	delete[] mOggFrameBuffer;
	delete[] mInterleavedBuffer;
	delete[] mBuffer;
	delete mSoundTouch;
	delete mSourceInstance;
}

void SoundTouchFilterInstance::ensureBufferSize(unsigned int samples)
{
	if (samples > mBufferSize)
	{
		ST_DEBUG_LOG("SoundTouchFilterInstance: Resizing non-interleaved buffer from %u to %u samples\n", mBufferSize, samples);

		delete[] mBuffer;
		mBufferSize = samples;
		mBuffer = new float[static_cast<unsigned long>(mBufferSize * mChannels)];
		memset(mBuffer, 0, sizeof(float) * mBufferSize * mChannels);
	}
}

void SoundTouchFilterInstance::ensureInterleavedBufferSize(unsigned int samples)
{
	unsigned int requiredSize = samples * mChannels;
	if (requiredSize > mInterleavedBufferSize)
	{
		ST_DEBUG_LOG("SoundTouchFilterInstance: Resizing interleaved buffer from %u to %u samples\n", mInterleavedBufferSize / mChannels, samples);

		delete[] mInterleavedBuffer;
		mInterleavedBufferSize = requiredSize;
		mInterleavedBuffer = new float[mInterleavedBufferSize];
		memset(mInterleavedBuffer, 0, sizeof(float) * mInterleavedBufferSize);
	}
}

void SoundTouchFilterInstance::ensureOggFrameBuffer(unsigned int samples)
{
	if (samples > mOggFrameBufferSize)
	{
		ST_DEBUG_LOG("SoundTouchFilterInstance: Resizing OGG frame buffer from %u to %u samples\n", mOggFrameBufferSize, samples);

		delete[] mOggFrameBuffer;
		mOggFrameBufferSize = samples;
		mOggFrameBuffer = new float[static_cast<unsigned long>(mOggFrameBufferSize * mChannels)];
		memset(mOggFrameBuffer, 0, sizeof(float) * mOggFrameBufferSize * mChannels);
	}
}

bool SoundTouchFilterInstance::isOggSource() const
{
	if (!mParent || !mParent->mSource)
		return false;

	auto *wavStream = static_cast<SoLoud::WavStream *>(mParent->mSource); // can assume it's a WavStream, no Wavs will pass through here (no dynamic_cast needed)
	return wavStream && wavStream->mFiletype == WAVSTREAM_OGG;
}

unsigned int SoundTouchFilterInstance::getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize)
{
	mProcessingCounter++;

	bool logThisCall = ST_DEBUG_ENABLED && (mProcessingCounter == 1 || mProcessingCounter % 100 == 0);

	if (logThisCall)
	{
		ST_DEBUG_LOG("=== SoundTouchFilterInstance::getAudio [%u] ===\n", mProcessingCounter);
		ST_DEBUG_LOG("Request: %u samples, bufferSize: %u, channels: %u\n", aSamplesToRead, aBufferSize, mChannels);
		ST_DEBUG_LOG("Current position: %f seconds, total samples processed: %u\n", mStreamPosition, mTotalSamplesProcessed);
		ST_DEBUG_LOG("Source type: %s\n", isOggSource() ? "OGG" : "Other");
	}

	if (!mSourceInstance || !mSoundTouch)
	{
		// return silence if not initialized
		for (unsigned int i = 0; i < aSamplesToRead * mChannels; i++)
			aBuffer[i] = 0.0f;
		return aSamplesToRead;
	}

	// update SoundTouch parameters if they've changed
	if (mSoundTouchSpeed != mParent->mSpeedFactor || mSoundTouchPitch != mParent->mPitchFactor)
	{
		ST_DEBUG_LOG("Updating parameters, speed: %f->%f, pitch: %f->%f\n", mSoundTouchSpeed, mParent->mSpeedFactor, mSoundTouchPitch, mParent->mPitchFactor);

		mSoundTouch->setTempo(mParent->mSpeedFactor);
		mSoundTouch->setPitch(mParent->mPitchFactor);
        mSoundTouchSpeed = mParent->mSpeedFactor;          // custom
        mSoundTouchPitch = mParent->mPitchFactor;          // custom
        mSetRelativePlaySpeed = mParent->mSpeedFactor;     // SoLoud inherited
        mOverallRelativePlaySpeed = mParent->mSpeedFactor; // SoLoud inherited
	}

	if (logThisCall)
		ST_DEBUG_LOG("mSoundTouchSpeed: %f, mParent->mSpeedFactor: %f, mSoundTouchPitch: %f, mParent->mPitchFactor: %f\n", mSoundTouchSpeed, mParent->mSpeedFactor,
		             mSoundTouchPitch, mParent->mPitchFactor);

	unsigned int samplesInSoundTouch = mSoundTouch->numSamples();

	if (logThisCall)
		ST_DEBUG_LOG("SoundTouch has %u samples in buffer\n", samplesInSoundTouch);

	// keep the SoundTouch buffer well-stocked to ensure consistent output, use a target buffer size larger than necessary
	// still experimental, not sure what the best value is
	const unsigned int targetBufferSize = aSamplesToRead * 4;

	// below the target buffer size, fetch more samples
	if (samplesInSoundTouch < targetBufferSize)
	{
		// .ogg sources need frame-aligned buffering to prevent corruption (messed up right channel)
		// there might be other/more correct ways to do this, but i can't find anything
		if (isOggSource())
			feedSoundTouchFromOggFrames(targetBufferSize, logThisCall);
		else
			feedSoundTouchStandard(targetBufferSize, logThisCall);

		// if the source ended and we need more samples, flush
		// not 100% sure about this, documentation is unhelpful at best and misleading at worst
		if (mSourceInstance->hasEnded() && samplesInSoundTouch < aSamplesToRead)
		{
			if (logThisCall)
				ST_DEBUG_LOG("Source has ended, flushing SoundTouch\n");

			mSoundTouch->flush();
		}
	}

	// get processed samples from SoundTouch
	unsigned int samplesAvailable = mSoundTouch->numSamples();
	unsigned int samplesToReceive = samplesAvailable < aSamplesToRead ? samplesAvailable : aSamplesToRead;

	if (logThisCall)
		ST_DEBUG_LOG("SoundTouch now has %u samples available, will receive %u\n", samplesAvailable, samplesToReceive);

	// clear output buffer first
	memset(aBuffer, 0, sizeof(float) * aSamplesToRead * mChannels);

	if (samplesToReceive > 0)
	{
		// SoundTouch outputs interleaved audio, but SoLoud expects non-interleaved
		ensureInterleavedBufferSize(samplesToReceive);

		// get interleaved samples from SoundTouch
		unsigned int samplesReceived = mSoundTouch->receiveSamples(mInterleavedBuffer, samplesToReceive);

		if (logThisCall)
			ST_DEBUG_LOG("Received %u samples from SoundTouch, converting to non-interleaved\n", samplesReceived);

		// convert from interleaved (LRLRLR...) to non-interleaved (LLLL...RRRR...)
		for (unsigned int i = 0; i < samplesReceived; i++)
		{
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				aBuffer[ch * aSamplesToRead + i] = mInterleavedBuffer[i * mChannels + ch];
			}
		}

		// track total samples processed (just for debugging)
		if (samplesReceived > 0)
		{
			const float samplesInSeconds = (static_cast<float>(samplesReceived) / mBaseSamplerate);

			mTotalSamplesProcessed += samplesReceived;

			if (logThisCall)
				ST_DEBUG_LOG("Updated position: received %u samples (%.4f seconds), new position: %.4f seconds\n", samplesReceived, samplesInSeconds, mStreamPosition);
		}
	}
	else if (logThisCall)
	{
		ST_DEBUG_LOG("No samples available from SoundTouch, returning silence\n");
	}

	if (logThisCall)
		ST_DEBUG_LOG("=== End of getAudio [%u] ===\n", mProcessingCounter);

	// always return requested amount (this is canonical for SoLoud filters)
	return aSamplesToRead;
}

// for OGG, accumulate complete frames in the frame buffer before sending to SoundTouch, because it needs properly aligned frame data
void SoundTouchFilterInstance::feedSoundTouchFromOggFrames(unsigned int targetBufferSize, bool logThis)
{
	const unsigned int maxSamplesToRequest = targetBufferSize * 2; // don't request too much at once

	// keep reading until we have enough samples or the source ends
	while (mOggSamplesInBuffer < targetBufferSize && !mSourceInstance->hasEnded())
	{
		unsigned int samplesToRequest = std::min(maxSamplesToRequest, targetBufferSize * 2);

		if (logThis)
			ST_DEBUG_LOG("OGG: Requesting %u samples for frame buffer (current buffer: %u)\n", samplesToRequest, mOggSamplesInBuffer);

		ensureBufferSize(samplesToRequest);

		// read from source into the standard buffer
		unsigned int sourceSamplesRead = mSourceInstance->getAudio(mBuffer, samplesToRequest, mBufferSize);

		if (sourceSamplesRead > 0)
		{
			// make sure the ogg frame buffer can fit the new samples
			ensureOggFrameBuffer(mOggSamplesInBuffer + sourceSamplesRead);

			// copy the new samples to the OGG frame buffer
			// source data is non-interleaved (LLLL...RRRR...)
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				float *srcChannel = mBuffer + (static_cast<size_t>(ch * sourceSamplesRead));
				float *dstChannel = mOggFrameBuffer + (static_cast<size_t>(ch * mOggFrameBufferSize)) + mOggSamplesInBuffer;

				memcpy(dstChannel, srcChannel, sizeof(float) * sourceSamplesRead);
			}

			mOggSamplesInBuffer += sourceSamplesRead;

			if (logThis)
				ST_DEBUG_LOG("OGG: Added %u samples to frame buffer, total now: %u\n", sourceSamplesRead, mOggSamplesInBuffer);
		}
		else
		{
			break; // no more data
		}
	}

	if (mOggSamplesInBuffer > 0)
	{
		// now, send collected frames to SoundTouch
		unsigned int samplesToSend = mOggSamplesInBuffer;

		if (logThis)
			ST_DEBUG_LOG("OGG: Sending %u accumulated samples to SoundTouch\n", samplesToSend);

		ensureInterleavedBufferSize(samplesToSend);

		// convert to interleaved
		for (unsigned int i = 0; i < samplesToSend; i++)
		{
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				mInterleavedBuffer[i * mChannels + ch] = mOggFrameBuffer[ch * mOggFrameBufferSize + i];
			}
		}

		// feed SoundTouch
		mSoundTouch->putSamples(mInterleavedBuffer, samplesToSend);

		if (logThis)
			ST_DEBUG_LOG("After OGG putSamples, SoundTouch now has %u samples\n", mSoundTouch->numSamples());

		// clear the frame buffer for next iteration
		mOggSamplesInBuffer = 0;
	}
}

// standard ST feeding logic for non-ogg sources (mp3, wav, etc.)
void SoundTouchFilterInstance::feedSoundTouchStandard(unsigned int targetBufferSize, bool logThis)
{
	unsigned int sourceSamplesToRead = (unsigned int)(targetBufferSize * mParent->mSpeedFactor) + 512;

	if (logThis)
		ST_DEBUG_LOG("Standard: Requesting %u samples from source\n", sourceSamplesToRead);

	ensureBufferSize(sourceSamplesToRead);

	// source audio - SoLoud's non-interleaved format (LLLL...RRRR...)
	unsigned int sourceSamplesRead = mSourceInstance->getAudio(mBuffer, sourceSamplesToRead, mBufferSize);

	if (logThis)
		ST_DEBUG_LOG("Standard: Read %u samples from source\n", sourceSamplesRead);

	if (sourceSamplesRead > 0)
	{
		if (logThis)
			ST_DEBUG_LOG("Converting %u samples from non-interleaved to interleaved format\n", sourceSamplesRead);

		// SoundTouch requires interleaved audio (L,R,L,R...)
		ensureInterleavedBufferSize(sourceSamplesRead);

		// convert from non-interleaved to interleaved
		for (unsigned int i = 0; i < sourceSamplesRead; i++)
		{
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				// output index: i * channels + ch
				// input index: ch * sourceSamplesRead + i
				mInterleavedBuffer[i * mChannels + ch] = mBuffer[ch * sourceSamplesRead + i];
			}
		}

		// feed the interleaved samples to SoundTouch
		mSoundTouch->putSamples(mInterleavedBuffer, sourceSamplesRead);

		if (logThis)
			ST_DEBUG_LOG("After putSamples, SoundTouch now has %u samples\n", mSoundTouch->numSamples());
	}
}

bool SoundTouchFilterInstance::hasEnded()
{
	// end when source has ended and we're not looping
	const bool ended = mSourceInstance && mSourceInstance->hasEnded();
	// wait, why do i need to access the parent source flags instead of (mSourceInstance->mFlags & AudioSourceInstace::LOOPING) ???
	const bool looping = ended && (mParent->mSource->mFlags & AudioSource::SHOULD_LOOP);

	if (looping)
	{
		ST_DEBUG_LOG("looping source, rewinding\n");
		if (rewind() == SO_NO_ERROR)
			return false;
	}

	if (ended && ST_DEBUG_ENABLED)
		ST_DEBUG_LOG("ended, returning true (mSourceInstance->flags: %x mParent->mSource->mFlags: %x)\n", mSourceInstance->mFlags, mParent->mSource->mFlags);

	return ended;
}

result SoundTouchFilterInstance::seek(time aSeconds, float *mScratch, unsigned int mScratchSize)
{
	if (!mSourceInstance || !mSoundTouch)
		return INVALID_PARAMETER;

	ST_DEBUG_LOG("Seeking to %.3f seconds\n", aSeconds);

	// seek the source.
	result res = mSourceInstance->seek(aSeconds, mScratch, mScratchSize);

	if (res == SO_NO_ERROR)
	{
		// clear SoundTouch buffers on seek
		if (mSoundTouch)
			mSoundTouch->clear();

		// clear OGG frame buffer on seek
		mOggSamplesInBuffer = 0;

		// update the position tracking, without this the audio will seek but we'll have no way of knowing
		// from any outside function
		mStreamPosition = mSourceInstance->mStreamPosition;
		mStreamTime = mSourceInstance->mStreamTime;
		mTotalSamplesProcessed = (unsigned int)(mStreamTime * mBaseSamplerate);

		// prime buffers to prevent position desynchronization
		primeBuffers();

		ST_DEBUG_LOG("Seek complete to %.3f seconds\n", mStreamPosition);
	}

	return res;
}

result SoundTouchFilterInstance::rewind()
{
	ST_DEBUG_LOG("rewind called\n");

	result res = SO_NO_ERROR;
	if (mSourceInstance)
		res = mSourceInstance->rewind();

	if (res == SO_NO_ERROR)
	{
		if (mSoundTouch)
			mSoundTouch->clear();

		// clear OGG frame buffer on rewind
		mOggSamplesInBuffer = 0;

		mStreamPosition = mSourceInstance->mStreamPosition;
		mStreamTime = mSourceInstance->mStreamTime;
		mTotalSamplesProcessed = 0;

		// prime buffers to prevent position desynchronization
		primeBuffers();

		ST_DEBUG_LOG("Rewind complete, buffers primed\n");
	}

	return res;
}

void SoundTouchFilterInstance::primeBuffers()
{
	if (!mSourceInstance || !mSoundTouch)
		return;

	// get configuration for buffer sizing
	int initialLatency = mSoundTouch->getSetting(SETTING_INITIAL_LATENCY);
	int nominalInputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_INPUT_SEQUENCE);

	// initial latency + nominal sequence should be good for # priming samples
	unsigned int primingSamples = initialLatency + nominalInputSeq;

	ST_DEBUG_LOG("Priming SoundTouch buffers: initialLatency=%d, nominalInput=%d, priming=%u samples\n", initialLatency, nominalInputSeq, primingSamples);

	// resize the buffers
	ensureBufferSize(primingSamples);
	ensureInterleavedBufferSize(primingSamples);

	// store current source position so we can restore it after priming
	double currentPosition = mSourceInstance->mStreamPosition;

	// clear OGG frame buffer before priming
	mOggSamplesInBuffer = 0;

	// get the primingSamples amount of data from the source
	unsigned int primingRead = mSourceInstance->getAudio(mBuffer, primingSamples, mBufferSize);

	if (primingRead > 0)
	{
		// weave the leaves
		for (unsigned int i = 0; i < primingRead; i++)
		{
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				mInterleavedBuffer[i * mChannels + ch] = mBuffer[ch * primingRead + i];
			}
		}

		// feed priming samples to SoundTouch
		mSoundTouch->putSamples(mInterleavedBuffer, primingRead);

		ST_DEBUG_LOG("Fed %u priming samples to SoundTouch, buffer now has %u samples\n", primingRead, mSoundTouch->numSamples());

		// discard initial output to remove transition artifacts
		unsigned int samplesToDiscard = mSoundTouch->numSamples();
		if (samplesToDiscard > 0)
		{
			ensureInterleavedBufferSize(samplesToDiscard);
			unsigned int discarded = mSoundTouch->receiveSamples(mInterleavedBuffer, samplesToDiscard);

			ST_DEBUG_LOG("Discarded %u transition samples, buffer now has %u samples\n", discarded, mSoundTouch->numSamples());
		}
	}

	// restore the source to the pre-priming position
	if (mSourceInstance->seek(currentPosition, mBuffer, mBufferSize) == SO_NO_ERROR)
	{
		// make sure our position tracking matches
		mStreamPosition = mSourceInstance->mStreamPosition;
		mStreamTime = mSourceInstance->mStreamTime;
		mTotalSamplesProcessed = (unsigned int)(mStreamTime * mBaseSamplerate);

		ST_DEBUG_LOG("Buffer priming complete, position restored to %.3f seconds\n", mStreamPosition);
	}
	else
	{
		ST_DEBUG_LOG("Warning: Failed to restore position after buffer priming\n");
	}
}

// DEBUG
void SoundTouchFilterInstance::logBufferData(const char *label, float *buffer, unsigned int channels, unsigned int samples, bool isInterleaved, unsigned int maxToLog)
{
	if (!ST_DEBUG_ENABLED)
		return;

	ST_DEBUG_LOG("--- %s (%u samples, %u channels) ---\n", label, samples, channels);

	unsigned int samplesToLog = samples < maxToLog ? samples : maxToLog;

	if (isInterleaved)
	{
		// interleaved format (LRLRLR...)
		for (unsigned int i = 0; i < samplesToLog; i++)
		{
			char logLine[256] = {};
			int offset = sprintf(logLine, "Sample %03u: ", i);
			for (unsigned int ch = 0; ch < channels; ch++)
			{
				offset += sprintf(logLine + offset, "[Ch%u: %+.4f] ", ch, buffer[i * channels + ch]);
			}
			ST_DEBUG_LOG("%s\n", logLine);
		}
	}
	else
	{
		// non-interleaved format (LLLL...RRRR...)
		for (unsigned int i = 0; i < samplesToLog; i++)
		{
			char logLine[256] = {};
			int offset = sprintf(logLine, "Sample %03u: ", i);
			for (unsigned int ch = 0; ch < channels; ch++)
			{
				offset += sprintf(logLine + offset, "[Ch%u: %+.4f] ", ch, buffer[ch * samples + i]);
			}
			ST_DEBUG_LOG("%s\n", logLine);
		}
	}

	if (samples > maxToLog)
		ST_DEBUG_LOG("... (and %u more samples)\n", samples - maxToLog);

	ST_DEBUG_LOG("--- End of %s ---\n", label);
}

} // namespace SoLoud

#endif // MCENGINE_FEATURE_SOLOUD
