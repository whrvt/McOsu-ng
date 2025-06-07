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
    : mSource(nullptr),
      mSpeedFactor(1.0f),
      mPitchFactor(1.0f),
      mWavStream(nullptr)
{
	ST_DEBUG_LOG("SoundTouchFilter: Constructor called\n");
	// initialize as streaming audio source
	mWavStream = new SoLoud::WavStream();
}

SLFXStream::~SLFXStream()
{
	ST_DEBUG_LOG("SoundTouchFilter: Destructor called\n");
	// stop all instances before cleanup
	stop();

	delete mWavStream;
}

result SLFXStream::setSource(AudioSource *aSource)
{
	if (!aSource)
		return INVALID_PARAMETER;

	// copy source params
	mSource = aSource;
	mChannels = mSource->mChannels;
	mBaseSamplerate = mSource->mBaseSamplerate;
	mFlags = mSource->mFlags;

	ST_DEBUG_LOG("SoundTouchFilter: Set source with {:d} channels at {:f} Hz, mFlags={:x}\n", mChannels, mBaseSamplerate, mFlags);

	return SO_NO_ERROR;
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

void SLFXStream::initializeFilter()
{
	if (mWavStream)
	{
		// connect the filter to the wav stream
		setSource(mWavStream);

		ST_DEBUG_LOG("SLFXStream: Initialized filter with {:d} channels at {:f} Hz\n", mChannels, mBaseSamplerate);
	}
}

// WavStream-compatibility methods
result SLFXStream::load(const char *aFilename)
{
	if (!mWavStream)
		return INVALID_PARAMETER;

	result result = mWavStream->load(aFilename);
	if (result == SO_NO_ERROR)
		initializeFilter();

	return result;
}

result SLFXStream::loadMem(const unsigned char *aData, unsigned int aDataLen, bool aCopy, bool aTakeOwnership)
{
	if (!mWavStream)
		return INVALID_PARAMETER;

	result result = mWavStream->loadMem(aData, aDataLen, aCopy, aTakeOwnership);
	if (result == SO_NO_ERROR)
		initializeFilter();

	return result;
}

result SLFXStream::loadToMem(const char *aFilename)
{
	if (!mWavStream)
		return INVALID_PARAMETER;

	result result = mWavStream->loadToMem(aFilename);
	if (result == SO_NO_ERROR)
		initializeFilter();

	return result;
}

result SLFXStream::loadFile(File *aFile)
{
	if (!mWavStream)
		return INVALID_PARAMETER;

	result result = mWavStream->loadFile(aFile);
	if (result == SO_NO_ERROR)
		initializeFilter();

	return result;
}

result SLFXStream::loadFileToMem(File *aFile)
{
	if (!mWavStream)
		return INVALID_PARAMETER;

	result result = mWavStream->loadFileToMem(aFile);
	if (result == SO_NO_ERROR)
		initializeFilter();

	return result;
}

double SLFXStream::getLength()
{
	return mWavStream ? mWavStream->getLength() : 0.0;
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
      mOggFrameBuffer(nullptr),
      mOggFrameBufferSize(0),
      mOggSamplesInBuffer(0),
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

				// pre-fill and make sync streams
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

void SoundTouchFilterInstance::ensureOggFrameBuffer(unsigned int samples)
{
	if (samples > mOggFrameBufferSize)
	{
		ST_DEBUG_LOG("SoundTouchFilterInstance: Resizing OGG frame buffer from {:} to {:} samples\n", mOggFrameBufferSize, samples);

		delete[] mOggFrameBuffer;
		mOggFrameBufferSize = samples;
		mOggFrameBuffer = new float[static_cast<unsigned long>(mOggFrameBufferSize * mChannels)];
		memset(mOggFrameBuffer, 0, sizeof(float) * mOggFrameBufferSize * mChannels);
	}
}

bool SoundTouchFilterInstance::isOggSource() const
{
	if (!mParent || !mParent->mWavStream)
		return false;

	auto *parentStream = static_cast<SoLoud::WavStream *>(mParent->mWavStream);
	return parentStream && parentStream->mFiletype == WAVSTREAM_OGG;
}

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
		ST_DEBUG_LOG("Source type: {:s}\n", isOggSource() ? "OGG" : "Other");
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
		// SoundTouch outputs interleaved audio, but SoLoud expects non-interleaved
		ensureInterleavedBufferSize(samplesToReceive);

		// get interleaved samples from SoundTouch
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

		// track total samples processed (just for debugging)
		if (samplesReceived > 0)
		{
			const float samplesInSeconds = (static_cast<float>(samplesReceived) / mBaseSamplerate);

			mTotalSamplesProcessed += samplesReceived;

			if (logThisCall)
				ST_DEBUG_LOG("Updated position: received {:} samples ({:.4f} seconds), new position: {:.4f} seconds\n", samplesReceived, samplesInSeconds,
				             mStreamPosition);
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

// for OGG, accumulate complete frames in the frame buffer before sending to SoundTouch, because it needs properly aligned frame data
void SoundTouchFilterInstance::feedSoundTouchFromOggFrames(unsigned int targetBufferSize, bool logThis)
{
	// keep reading until we have enough samples or the source ends
	while (mOggSamplesInBuffer < targetBufferSize && !mSourceInstance->hasEnded())
	{
		if (logThis)
			ST_DEBUG_LOG("OGG: Requesting {:} samples for frame buffer (current buffer: {:})\n", targetBufferSize * 2, mOggSamplesInBuffer);

		ensureBufferSize(targetBufferSize * 2);

		// read from source into the standard buffer
		unsigned int sourceSamplesRead = mSourceInstance->getAudio(mBuffer, targetBufferSize * 2, mBufferSize);

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
				ST_DEBUG_LOG("OGG: Added {:} samples to frame buffer, total now: {:}\n", sourceSamplesRead, mOggSamplesInBuffer);
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
			ST_DEBUG_LOG("OGG: Sending {:} accumulated samples to SoundTouch\n", samplesToSend);

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
			ST_DEBUG_LOG("After OGG putSamples, SoundTouch now has {:} samples\n", mSoundTouch->numSamples());

		// clear the frame buffer for next iteration
		mOggSamplesInBuffer = 0;
	}
}

// standard ST feeding logic for non-ogg sources (mp3, wav, etc.)
void SoundTouchFilterInstance::feedSoundTouchStandard(unsigned int targetBufferSize, bool logThis)
{
	unsigned int sourceSamplesToRead = (unsigned int)(targetBufferSize * mParent->mSpeedFactor) + 512;

	if (logThis)
		ST_DEBUG_LOG("Standard: Requesting {:} samples from source\n", sourceSamplesToRead);

	ensureBufferSize(sourceSamplesToRead);

	// source audio - SoLoud's non-interleaved format (LLLL...RRRR...)
	unsigned int sourceSamplesRead = mSourceInstance->getAudio(mBuffer, sourceSamplesToRead, mBufferSize);

	if (logThis)
		ST_DEBUG_LOG("Standard: Read {:} samples from source\n", sourceSamplesRead);

	if (sourceSamplesRead > 0)
	{
		if (logThis)
			ST_DEBUG_LOG("Converting {:} samples from non-interleaved to interleaved format\n", sourceSamplesRead);

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
			ST_DEBUG_LOG("After putSamples, SoundTouch now has {:} samples\n", mSoundTouch->numSamples());
	}
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

void SoundTouchFilterInstance::primeBuffers()
{
	if (!mSourceInstance || !mSoundTouch)
		return;

	// get configuration for buffer sizing
	int initialLatency = mSoundTouch->getSetting(SETTING_INITIAL_LATENCY);
	int nominalInputSeq = mSoundTouch->getSetting(SETTING_NOMINAL_INPUT_SEQUENCE);

	// initial latency + nominal sequence should be good for # priming samples
	unsigned int primingSamples = initialLatency + nominalInputSeq;

	ST_DEBUG_LOG("Priming SoundTouch buffers: initialLatency={:d}, nominalInput={:d}, priming={:} samples\n", initialLatency, nominalInputSeq, primingSamples);

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

		ST_DEBUG_LOG("Fed {:} priming samples to SoundTouch, buffer now has {:} samples\n", primingRead, mSoundTouch->numSamples());

		// discard initial output to remove transition artifacts
		unsigned int samplesToDiscard = mSoundTouch->numSamples();
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

	// clear OGG frame buffer
	mOggSamplesInBuffer = 0;

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

		float latencyInMs = (initialLatencyInSamples / (float)mBaseSamplerate) * 1000.0f;

		// add buffer compensation factor (~half of processing window)
		float processingBufferDelay = 0.0f;
		if (nominalInputSeq > 0 && nominalOutputSeq > 0)
		{
			processingBufferDelay = ((nominalInputSeq - nominalOutputSeq) / 2.0f / (float)mBaseSamplerate) * 1000.0f;
		}

		float totalOffset = std::clamp<float>(-(latencyInMs + processingBufferDelay), -200.0f, 0.0f);

		if (cv::debug_snd.getBool())
			debugLog("Calculated universal offset = {:.2f} ms (latency: {:.2f}, buffer: {:.2f}), setting final offset to {:.2f}\n", totalOffset, latencyInMs,
			         processingBufferDelay, cv::osu::universal_offset_hardcoded.getDefaultFloat() + totalOffset);

		cv::osu::universal_offset_hardcoded.setValue(cv::osu::universal_offset_hardcoded.getDefaultFloat() + totalOffset);
	}
#endif
}

} // namespace SoLoud

#endif // MCENGINE_FEATURE_SOLOUD
