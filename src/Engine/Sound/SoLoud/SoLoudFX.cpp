//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoundTouch filter for independent pitch/speed control (for use with SoLoud)
//
// $NoKeywords: $snd $soloud $soundtouch
//================================================================================//

#include "SoLoudFX.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include "SoLoudSoundEngine.h"

#include "ConVar.h"
#include "Engine.h"

#include <SoundTouch.h>
#include <soloud/soloud_wavstream.h>

#include <cstddef>

#if __has_include("Osu.h")
#define DO_AUTO_OFFSET
#endif

namespace cv
{
ConVar snd_enable_auto_offset("snd_enable_auto_offset", true, FCVAR_NONE, "Control automatic offset calibration for SoLoud + rate change");

#ifdef _DEBUG
ConVar snd_st_debug("snd_st_debug", false, FCVAR_NONE, "Enable detailed SoundTouch filter logging");
#define ST_DEBUG_ENABLED cv::snd_st_debug.getBool()
#else
#define ST_DEBUG_ENABLED 0
#endif
} // namespace cv

#define ST_DEBUG_LOG(...) \
	if (ST_DEBUG_ENABLED) \
	{ \
		debugLog(__VA_ARGS__); \
	}

namespace SoLoud
{
//-------------------------------------------------------------------------
// SLFXStream (WavStream + SoundTouch wrapper) implementation
//-------------------------------------------------------------------------

SLFXStream::SLFXStream()
    : mSpeedFactor(1.0f),
      mPitchFactor(1.0f),
      mSource(std::make_unique<WavStream>())
{
	ST_DEBUG_LOG("SoundTouchFilter: Constructor called\n");
}

SLFXStream::~SLFXStream()
{
	ST_DEBUG_LOG("SoundTouchFilter: Destructor called\n");
	// stop all instances before cleanup
	stop();
}

AudioSourceInstance *SLFXStream::createInstance()
{
	if (!mSource)
		return nullptr;

	ST_DEBUG_LOG("SoundTouchFilter: Creating instance with speed={:f}, pitch={:f}\n", mSpeedFactor, mPitchFactor);

	return new SoundTouchFilterInstance(this);
}

void SLFXStream::setSpeedFactor(float aSpeed)
{
	ST_DEBUG_LOG("SoundTouchFilter: Speed changed from {:f} to {:f}\n", mSpeedFactor, aSpeed);
	mSpeedFactor = aSpeed;
}

void SLFXStream::setPitchFactor(float aPitch)
{
	ST_DEBUG_LOG("SoundTouchFilter: Pitch changed from {:f} to {:f}\n", mPitchFactor, aPitch);
	mPitchFactor = aPitch;
}

float SLFXStream::getSpeedFactor() const
{
	return mSpeedFactor;
}

float SLFXStream::getPitchFactor() const
{
	return mPitchFactor;
}

// WavStream-compatibility methods
result SLFXStream::load(const char *aFilename)
{
	if (!mSource)
		return INVALID_PARAMETER;

	result result = mSource->load(aFilename);
	if (result == SO_NO_ERROR)
	{
		// copy properties from the loaded wav stream
		mChannels = mSource->mChannels;
		mBaseSamplerate = mSource->mBaseSamplerate;
		mFlags = mSource->mFlags;

		ST_DEBUG_LOG("SLFXStream: Loaded with {:d} channels at {:f} Hz\n", mChannels, mBaseSamplerate);
	}

	return result;
}

result SLFXStream::loadMem(const unsigned char *aData, unsigned int aDataLen, bool aCopy, bool aTakeOwnership)
{
	if (!mSource)
		return INVALID_PARAMETER;

	result result = mSource->loadMem(aData, aDataLen, aCopy, aTakeOwnership);
	if (result == SO_NO_ERROR)
	{
		// copy properties from the loaded wav stream
		mChannels = mSource->mChannels;
		mBaseSamplerate = mSource->mBaseSamplerate;
		mFlags = mSource->mFlags;

		ST_DEBUG_LOG("SLFXStream: Loaded from memory with {:d} channels at {:f} Hz\n", mChannels, mBaseSamplerate);
	}

	return result;
}

result SLFXStream::loadToMem(const char *aFilename)
{
	if (!mSource)
		return INVALID_PARAMETER;

	result result = mSource->loadToMem(aFilename);
	if (result == SO_NO_ERROR)
	{
		// copy properties from the loaded wav stream
		mChannels = mSource->mChannels;
		mBaseSamplerate = mSource->mBaseSamplerate;
		mFlags = mSource->mFlags;

		ST_DEBUG_LOG("SLFXStream: Loaded to memory with {:d} channels at {:f} Hz\n", mChannels, mBaseSamplerate);
	}

	return result;
}

result SLFXStream::loadFile(File *aFile)
{
	if (!mSource)
		return INVALID_PARAMETER;

	result result = mSource->loadFile(aFile);
	if (result == SO_NO_ERROR)
	{
		// copy properties from the loaded wav stream
		mChannels = mSource->mChannels;
		mBaseSamplerate = mSource->mBaseSamplerate;
		mFlags = mSource->mFlags;

		ST_DEBUG_LOG("SLFXStream: Loaded from file with {:d} channels at {:f} Hz\n", mChannels, mBaseSamplerate);
	}

	return result;
}

result SLFXStream::loadFileToMem(File *aFile)
{
	if (!mSource)
		return INVALID_PARAMETER;

	result result = mSource->loadFileToMem(aFile);
	if (result == SO_NO_ERROR)
	{
		// copy properties from the loaded wav stream
		mChannels = mSource->mChannels;
		mBaseSamplerate = mSource->mBaseSamplerate;
		mFlags = mSource->mFlags;

		ST_DEBUG_LOG("SLFXStream: Loaded file to memory with {:d} channels at {:f} Hz\n", mChannels, mBaseSamplerate);
	}

	return result;
}

double SLFXStream::getLength()
{
	return mSource ? mSource->getLength() : 0.0;
}

//-------------------------------------------------------------------------
// SoundTouchFilterInstance implementation
//-------------------------------------------------------------------------

SoundTouchFilterInstance::SoundTouchFilterInstance(SLFXStream *aParent)
    : mParent(aParent),
      mSourceInstance(nullptr),
      mSoundTouch(nullptr),
      mSoundTouchSpeed(0.0f),
      mSoundTouchPitch(0.0f),
      mBuffer(nullptr),
      mBufferSize(0),
      mInterleavedBuffer(nullptr),
      mInterleavedBufferSize(0),
      mProcessingCounter(0),
      mTotalSamplesProcessed(0)
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

			ST_DEBUG_LOG("SoundTouchFilterInstance: Creating with {:d} channels at {:f} Hz, mFlags={:x}\n", mChannels, mBaseSamplerate, mFlags);

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

				ST_DEBUG_LOG("SoundTouch: Initialized with speed={:f}, pitch={:f}\n", mParent->mSpeedFactor, mParent->mPitchFactor);
				ST_DEBUG_LOG("SoundTouch: Version: {:s}\n", mSoundTouch->getVersionString());

				// pre-fill and make sure source/filter are synchronized
				reSynchronize();
			}
		}
	}
}

SoundTouchFilterInstance::~SoundTouchFilterInstance()
{
#ifdef DO_AUTO_OFFSET
	cv::osu::universal_offset_hardcoded.setValue(cv::osu::universal_offset_hardcoded.getDefaultFloat());
#endif
	delete[] mInterleavedBuffer;
	delete[] mBuffer;
	delete mSoundTouch;
	delete mSourceInstance;
}

// public methods below (to be called by SoLoud)
unsigned int SoundTouchFilterInstance::getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize)
{
	if (aBuffer == nullptr || mParent->mSoloud == nullptr)
		return 0;

	mProcessingCounter++;

	bool logThisCall = ST_DEBUG_ENABLED && (mProcessingCounter == 1 || mProcessingCounter % 100 == 0);

	if (logThisCall)
	{
		ST_DEBUG_LOG("=== SoundTouchFilterInstance::getAudio [{:}] ===\n", mProcessingCounter);
		ST_DEBUG_LOG("Request: {:} samples, bufferSize: {:}, channels: {:}\n", aSamplesToRead, aBufferSize, mChannels);
		ST_DEBUG_LOG("Current position: {:f} seconds, total samples processed: {:}\n", mStreamPosition, mTotalSamplesProcessed);
	}

	if (!mSourceInstance || !mSoundTouch)
	{
		// return silence if not initialized
		memset(aBuffer, 0, sizeof(float) * aSamplesToRead * mChannels);
		return aSamplesToRead;
	}

	// update SoundTouch parameters if they've changed
	if (mSoundTouchSpeed != mParent->mSpeedFactor || mSoundTouchPitch != mParent->mPitchFactor)
	{
		ST_DEBUG_LOG("Updating parameters, speed: {:f}->{:f}, pitch: {:f}->{:f}\n", mSoundTouchSpeed, mParent->mSpeedFactor, mSoundTouchPitch,
		             mParent->mPitchFactor);

		mSoundTouch->setTempo(mParent->mSpeedFactor);
		mSoundTouch->setPitch(mParent->mPitchFactor);
		mSoundTouchSpeed = mParent->mSpeedFactor;          // custom
		mSoundTouchPitch = mParent->mPitchFactor;          // custom
		mSetRelativePlaySpeed = mParent->mSpeedFactor;     // SoLoud inherited
		mOverallRelativePlaySpeed = mParent->mSpeedFactor; // SoLoud inherited
		setAutoOffset();                                   // re-calculate audio offset
	}

	if (logThisCall)
		ST_DEBUG_LOG("mSoundTouchSpeed: {:f}, mParent->mSpeedFactor: {:f}, mSoundTouchPitch: {:f}, mParent->mPitchFactor: {:f}\n", mSoundTouchSpeed,
		             mParent->mSpeedFactor, mSoundTouchPitch, mParent->mPitchFactor);

	unsigned int samplesInSoundTouch = mSoundTouch->numSamples();

	if (logThisCall)
		ST_DEBUG_LOG("SoundTouch has {:} samples in buffer\n", samplesInSoundTouch);

	// keep the SoundTouch buffer well-stocked to ensure consistent output, use a target buffer size larger than necessary
	// EXCEPT: if the source ended, then we just need to get out the last bits of SoundTouch audio,
	//			and don't try to receive more from the source, otherwise we keep flushing empty audio data that SoundTouch generates,
	//			and thus returning the wrong amount of real data we actually "got" (see return samplesReceived)
	if (!mSourceInstance->hasEnded())
	{
		unsigned int targetBufferLevel = calculateTargetBufferLevel(aSamplesToRead, logThisCall);

		// below the target buffer level, feed more data using fixed chunks
		if (samplesInSoundTouch < targetBufferLevel)
		{
			feedSoundTouch(targetBufferLevel, logThisCall);

			// if source ended and we still need more samples, flush
			if (mSourceInstance->hasEnded() && mSoundTouch->numSamples() < aSamplesToRead)
			{
				if (logThisCall)
					ST_DEBUG_LOG("Source has ended, flushing SoundTouch\n");
				mSoundTouch->flush();
			}
		}
	}

	// get processed samples from SoundTouch
	unsigned int samplesAvailable = mSoundTouch->numSamples();
	unsigned int samplesToReceive = samplesAvailable < aSamplesToRead ? samplesAvailable : aSamplesToRead;
	unsigned int samplesReceived = samplesToReceive;

	if (logThisCall)
		ST_DEBUG_LOG("SoundTouch now has {:} samples available, will receive {:}\n", samplesAvailable, samplesToReceive);

	// clear output buffer first
	memset(aBuffer, 0, sizeof(float) * aSamplesToRead * mChannels);

	if (samplesToReceive > 0)
	{
		// get interleaved samples from SoundTouch and convert to non-interleaved
		ensureInterleavedBufferSize(samplesToReceive);
		samplesReceived = mSoundTouch->receiveSamples(mInterleavedBuffer, samplesToReceive);

		if (logThisCall)
			ST_DEBUG_LOG("Received {:} samples from SoundTouch, converting to non-interleaved\n", samplesReceived);

		// convert from interleaved (LRLRLR...) to non-interleaved (LLLL...RRRR...)
		for (unsigned int i = 0; i < samplesReceived; i++)
		{
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				aBuffer[ch * aSamplesToRead + i] = mInterleavedBuffer[i * mChannels + ch];
			}
		}

		// track total samples processed
		if (samplesReceived > 0)
		{
			mTotalSamplesProcessed += samplesReceived;

			if (logThisCall)
			{
				const float samplesInSeconds = (static_cast<float>(samplesReceived) / mBaseSamplerate);
				ST_DEBUG_LOG("Updated position: received {:} samples ({:.4f} seconds), new position: {:.4f} seconds\n", samplesReceived, samplesInSeconds,
				             mStreamPosition);
			}
		}
	}
	else if (logThisCall)
	{
		ST_DEBUG_LOG("No samples available from SoundTouch, returning silence\n");
	}

	if (logThisCall)
		ST_DEBUG_LOG("=== End of getAudio [{:}] ===\n", mProcessingCounter);

	// reference in soloud_wavstream.cpp shows that only the amount of real data actually "read" (taken from SoundTouch in this instance) is actually returned.
	// it just means that the stream ends properly when the parent source has ended, after the remaining data from the SoundTouch buffer is played out.
	return samplesReceived;
}

bool SoundTouchFilterInstance::hasEnded()
{
	// end when source has ended
	return (!mSourceInstance || mSourceInstance->hasEnded());
}

result SoundTouchFilterInstance::seek(time aSeconds, float *mScratch, unsigned int mScratchSize)
{
	if (!mSourceInstance || !mSoundTouch)
		return INVALID_PARAMETER;

	ST_DEBUG_LOG("Seeking to {:.3f} seconds\n", aSeconds);

	// seek the source.
	result res = mSourceInstance->seek(aSeconds, mScratch, mScratchSize);

	if (res == SO_NO_ERROR)
	{
		reSynchronize();
		ST_DEBUG_LOG("Seek complete to {:.3f} seconds\n", mStreamPosition);
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
		reSynchronize();
		ST_DEBUG_LOG("Rewind complete, buffers primed\n");
	}

	return res;
}
// end public methods

// private helpers below (not called by SoLoud)
void SoundTouchFilterInstance::ensureBufferSize(unsigned int samples)
{
	if (samples > mBufferSize)
	{
		ST_DEBUG_LOG("SoundTouchFilterInstance: Resizing non-interleaved buffer from {:} to {:} samples\n", mBufferSize, samples);

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
		ST_DEBUG_LOG("SoundTouchFilterInstance: Resizing interleaved buffer from {:} to {:} samples\n", mInterleavedBufferSize / mChannels, samples);

		delete[] mInterleavedBuffer;
		mInterleavedBufferSize = requiredSize;
		mInterleavedBuffer = new float[mInterleavedBufferSize];
		memset(mInterleavedBuffer, 0, sizeof(float) * mInterleavedBufferSize);
	}
}

unsigned int SoundTouchFilterInstance::calculateTargetBufferLevel(unsigned int aSamplesToRead, bool logThis)
{
	// get SoundTouch's processing requirements
	int nominalInputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_INPUT_SEQUENCE);
	int nominalOutputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_OUTPUT_SEQUENCE);

	// ensure we have minimum values
	if (nominalInputSeq <= 0)
		nominalInputSeq = 1024;
	if (nominalOutputSeq <= 0)
		nominalOutputSeq = 1024;

	// target buffer level should be enough to satisfy the current request plus some headroom
	// we want at least 2x the nominal output sequence to ensure stable processing
	unsigned int baseTargetLevel = std::max(aSamplesToRead, static_cast<unsigned int>(nominalOutputSeq * 2));

	// add additional headroom for rate changes - more headroom for faster rates
	float headroomMultiplier = 1.0f + (mParent->mSpeedFactor - 1.0f) * 0.5f;
	headroomMultiplier = std::max(1.0f, headroomMultiplier);

	unsigned int targetLevel = static_cast<unsigned int>(baseTargetLevel * headroomMultiplier);

	if (logThis)
		ST_DEBUG_LOG("Target buffer level: base={:}, headroom={:.2f}, target={:}\n", baseTargetLevel, headroomMultiplier, targetLevel);

	return targetLevel;
}

// feed SoundTouch using fixed-size chunks for consistent processing
void SoundTouchFilterInstance::feedSoundTouch(unsigned int targetBufferLevel, bool logThis)
{
	unsigned int currentSamples = mSoundTouch->numSamples();

	if (logThis)
		ST_DEBUG_LOG("Starting feedSoundTouch: current={:}, target={:}, chunk size={:}\n", currentSamples, targetBufferLevel, SAMPLE_GRANULARITY);

	// ensure we have buffers for the fixed chunk size
	ensureBufferSize(SAMPLE_GRANULARITY);
	ensureInterleavedBufferSize(SAMPLE_GRANULARITY);

	// feed chunks until we reach the target buffer level or source ends
	unsigned int chunksProcessed = 0;
	const unsigned int maxChunks = 8; // safety limit to prevent infinite loops

	while (currentSamples < targetBufferLevel && !mSourceInstance->hasEnded() && chunksProcessed < maxChunks)
	{
		// always request exactly SAMPLE_GRANULARITY samples
		unsigned int samplesRead = mSourceInstance->getAudio(mBuffer, SAMPLE_GRANULARITY, mBufferSize);

		if (samplesRead == 0) // no more data available
			break;

		if (logThis)
			ST_DEBUG_LOG("Chunk {:}: read {:} samples from source\n", chunksProcessed + 1, samplesRead);

		// convert from non-interleaved to interleaved format
		for (unsigned int i = 0; i < samplesRead; i++)
		{
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				mInterleavedBuffer[i * mChannels + ch] = mBuffer[ch * samplesRead + i];
			}
		}

		// feed the chunk to SoundTouch
		mSoundTouch->putSamples(mInterleavedBuffer, samplesRead);

		currentSamples = mSoundTouch->numSamples();
		chunksProcessed++;

		if (logThis)
			ST_DEBUG_LOG("After chunk {:}: SoundTouch has {:} samples\n", chunksProcessed, currentSamples);
	}

	if (logThis)
		ST_DEBUG_LOG("feedSoundTouch complete: processed {:} chunks, final buffer level={:}\n", chunksProcessed, currentSamples);
}

void SoundTouchFilterInstance::primeBuffers()
{
	if (!mSourceInstance || !mSoundTouch)
		return;

	// get SoundTouch's buffer requirements
	int initialLatency = mSoundTouch->getSetting(SETTING_INITIAL_LATENCY);
	int nominalInputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_INPUT_SEQUENCE);

	// use reasonable defaults if SoundTouch returns invalid values
	if (initialLatency <= 0)
		initialLatency = 1024;
	if (nominalInputSeq <= 0)
		nominalInputSeq = 1024;

	// calculate how many chunks we need to prime the buffer adequately
	unsigned int totalPrimingSamples = static_cast<unsigned int>(initialLatency + nominalInputSeq);
	unsigned int chunksNeeded = (totalPrimingSamples + SAMPLE_GRANULARITY - 1) / SAMPLE_GRANULARITY; // round up

	// ensure minimum number of chunks for stability
	chunksNeeded = std::max(chunksNeeded, 3U);

	ST_DEBUG_LOG("Priming SoundTouch buffers: initialLatency={:d}, nominalInput={:d}, chunksNeeded={:}, chunkSize={:}\n", initialLatency, nominalInputSeq,
	             chunksNeeded, SAMPLE_GRANULARITY);

	// ensure buffers are sized for our fixed chunk size
	ensureBufferSize(SAMPLE_GRANULARITY);
	ensureInterleavedBufferSize(SAMPLE_GRANULARITY);

	// store current source position so we can restore it after priming
	double currentPosition = mSourceInstance->mStreamPosition;

	// prime using fixed chunks for consistency
	unsigned int totalPrimingRead = 0;
	for (unsigned int chunk = 0; chunk < chunksNeeded && !mSourceInstance->hasEnded(); chunk++)
	{
		unsigned int chunkRead = mSourceInstance->getAudio(mBuffer, SAMPLE_GRANULARITY, mBufferSize);

		if (chunkRead == 0)
			break; // no more data

		// convert from non-interleaved to interleaved format for SoundTouch
		for (unsigned int i = 0; i < chunkRead; i++)
		{
			for (unsigned int ch = 0; ch < mChannels; ch++)
			{
				mInterleavedBuffer[i * mChannels + ch] = mBuffer[ch * chunkRead + i];
			}
		}

		// feed chunk to SoundTouch
		mSoundTouch->putSamples(mInterleavedBuffer, chunkRead);
		totalPrimingRead += chunkRead;

		ST_DEBUG_LOG("Priming chunk {:}: read {:} samples, total={:}, ST buffer={:}\n", chunk + 1, chunkRead, totalPrimingRead, mSoundTouch->numSamples());
	}

	if (totalPrimingRead > 0)
	{
		ST_DEBUG_LOG("Fed {:} total priming samples to SoundTouch, buffer now has {:} samples\n", totalPrimingRead, mSoundTouch->numSamples());

		// process and discard initial output to remove transition artifacts
		// discard approximately the initial latency worth of samples
		unsigned int samplesToDiscard = std::min(static_cast<unsigned int>(initialLatency), mSoundTouch->numSamples());
		if (samplesToDiscard > 0)
		{
			ensureInterleavedBufferSize(samplesToDiscard);
			unsigned int discarded = mSoundTouch->receiveSamples(mInterleavedBuffer, samplesToDiscard);

			ST_DEBUG_LOG("Discarded {:} transition samples, buffer now has {:} samples\n", discarded, mSoundTouch->numSamples());
		}
	}

	// restore the source to the pre-priming position
	if (mSourceInstance->seek(currentPosition, mBuffer, mBufferSize) == SO_NO_ERROR)
	{
		// make sure our position tracking matches
		mStreamPosition = mSourceInstance->mStreamPosition;
		mStreamTime = mSourceInstance->mStreamTime;
		mTotalSamplesProcessed = (unsigned int)(mStreamTime * mBaseSamplerate);

		ST_DEBUG_LOG("Buffer priming complete, position restored to {:.3f} seconds\n", mStreamPosition);
	}
	else
	{
		ST_DEBUG_LOG("Warning: Failed to restore position after buffer priming\n");
	}
}

void SoundTouchFilterInstance::reSynchronize()
{
	// make sure we have nothing in the SoundTouch buffers
	if (mSoundTouch)
		mSoundTouch->clear();

	// update the position tracking, without this the audio will seek but we'll have no way of knowing
	// from any outside function
	mStreamPosition = mSourceInstance->mStreamPosition;
	mStreamTime = mSourceInstance->mStreamTime;
	mTotalSamplesProcessed = (unsigned int)(mStreamTime * mBaseSamplerate); // this should be 0 if rewinding or just starting

	// prime buffers to prevent position desynchronization
	primeBuffers();
	setAutoOffset();
}

void SoundTouchFilterInstance::setAutoOffset()
{
#ifdef DO_AUTO_OFFSET
	// estimate processing delay, somewhat following the idea from the SoundTouch header
	// !!FIXME!!: this is hot trash and inaccurate due to various factors that can influence the outcome
	if (cv::snd_enable_auto_offset.getBool())
	{
		// "This parameter value is not constant but change depending on
		// "tempo/pitch/rate/samplerate settings.
		int initialLatencyInSamples = mSoundTouch->getSetting(SETTING_INITIAL_LATENCY);
		int nominalInputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_INPUT_SEQUENCE);
		int nominalOutputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_OUTPUT_SEQUENCE);

		float latencyInMs = (initialLatencyInSamples / mBaseSamplerate) * 1000.0f;

		// add buffer compensation factor (~half of processing window)
		float processingBufferDelay = 0.0f;
		if (nominalInputSeq > 0 && nominalOutputSeq > 0)
		{
			processingBufferDelay = ((nominalInputSeq - nominalOutputSeq) / 2.0f / mBaseSamplerate) * 1000.0f;
		}

		const float calculatedOffset = std::clamp<float>(-(latencyInMs + processingBufferDelay), -200.0f, 0.0f);
		const float combinedUniversalOffset = cv::osu::universal_offset_hardcoded.getDefaultFloat() + calculatedOffset;

		if (cv::debug_snd.getBool())
			debugLog("Calculated offset = {:.2f} ms (latency: {:.2f}, buffer: {:.2f}), setting constant universal offset to {:.2f}\n", calculatedOffset, latencyInMs,
			         processingBufferDelay, combinedUniversalOffset);

		cv::osu::universal_offset_hardcoded.setValue(combinedUniversalOffset);
	}
#endif
}

} // namespace SoLoud

#endif // MCENGINE_FEATURE_SOLOUD
