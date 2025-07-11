//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoundTouch filter for independent pitch/speed control (for use with SoLoud)
//
// $NoKeywords: $snd $soloud $soundtouch
//================================================================================//

#include "SoLoudFX.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include "ConVar.h"
#include "Engine.h"
#include "SoundEngine.h" // for shouldDetectBPM

#include <BPMDetect.h> // from SoundTouch, to implement getBPM()
#include <SoundTouch.h>

#include <soloud_error.h>
#include <soloud_wavstream.h>

namespace cv
{
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

SLFXStream::SLFXStream(bool preferFFmpeg)
    : mSpeedFactor(1.0f),
      mPitchFactor(1.0f),
      mSource(std::make_unique<WavStream>(preferFFmpeg)),
      mActiveInstance(nullptr)
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

	ST_DEBUG_LOG("SoundTouchFilter: Creating instance with speed={:f}, pitch={:f}\n", mSpeedFactor.load(), mPitchFactor.load());

	auto *instance = new SoundTouchFilterInstance(this);
	mActiveInstance = instance; // track the active instance for position queries
	return instance;
}

void SLFXStream::setSpeedFactor(float aSpeed)
{
	ST_DEBUG_LOG("SoundTouchFilter: Speed changed from {:f} to {:f}\n", mSpeedFactor.load(), aSpeed);
	mSpeedFactor = aSpeed;
	if (mActiveInstance)
		mActiveInstance->requestSettingUpdate(mSpeedFactor.load(), mPitchFactor.load());
}

void SLFXStream::setPitchFactor(float aPitch)
{
	ST_DEBUG_LOG("SoundTouchFilter: Pitch changed from {:f} to {:f}\n", mPitchFactor.load(), aPitch);
	mPitchFactor = aPitch;
	if (mActiveInstance)
		mActiveInstance->requestSettingUpdate(mSpeedFactor.load(), mPitchFactor.load());
}

float SLFXStream::getSpeedFactor() const
{
	return mSpeedFactor.load();
}

float SLFXStream::getPitchFactor() const
{
	return mPitchFactor.load();
}

time SLFXStream::getRealStreamPosition() const
{
	if (mActiveInstance)
		return mActiveInstance->getRealStreamPosition();
	return 0.0;
}

float SLFXStream::getCurrentBPM() const
{
	if (mActiveInstance)
		return mActiveInstance->getCurrentBPM();
	return -1.0f;
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

UString SLFXStream::getDecoder()
{
	if (!mSource)
		return "<NULL>";

	switch (mSource->mFiletype)
	{
	case WAVSTREAM_WAV:
		return "dr_wav";
	case WAVSTREAM_OGG:
		return "dr_ogg";
	case WAVSTREAM_FLAC:
		return "dr_flac";
	case WAVSTREAM_MPG123:
		return "libmpg123";
	case WAVSTREAM_DRMP3:
		return "dr_mp3";
	case WAVSTREAM_FFMPEG:
		return "ffmpeg";
	default:
		return "unknown";
	}
}

//-------------------------------------------------------------------------
// SoundTouchFilterInstance implementation
//-------------------------------------------------------------------------

SoundTouchFilterInstance::SoundTouchFilterInstance(SLFXStream *aParent)
    : mParent(aParent),
      mSourceInstance(nullptr),
      mSoundTouch(nullptr),
      mInitialSTLatencySamples(0),
      mSTOutputSequenceSamples(0),
      mSTLatencySeconds(0.0),
      mSoundTouchSpeed(1.0f),
      mSoundTouchPitch(1.0f),
      mNeedsSettingUpdate(false),
      mBuffer(nullptr),
      mBufferSize(0),
      mInterleavedBuffer(nullptr),
      mInterleavedBufferSize(0),
      mBPMDetect(nullptr),
      mBPMSamplesProcessed(0),
      mBPMChunkSizeInSamples(0),
      mCurrentBPM(-1.0f),
      mProcessingCounter(0)
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
				mSoundTouch->setSetting(SETTING_AA_FILTER_LENGTH, 64);
				mSoundTouch->setSetting(SETTING_USE_QUICKSEEK, 0);
				mSoundTouch->setSetting(SETTING_SEQUENCE_MS, 10); // wtf should these numbers be?
				mSoundTouch->setSetting(SETTING_SEEKWINDOW_MS, 25);
				mSoundTouch->setSetting(SETTING_OVERLAP_MS, 5);

				// set the actual speed and pitch factors
				mSoundTouch->setTempo(mParent->mSpeedFactor.load());
				mSoundTouch->setPitch(mParent->mPitchFactor.load());

				mSoundTouchSpeed = mParent->mSpeedFactor.load();
				mSoundTouchPitch = mParent->mPitchFactor.load();

				ST_DEBUG_LOG("SoundTouch: Initialized with speed={:f}, pitch={:f}\n", mSoundTouchSpeed.load(), mSoundTouchPitch.load());
				ST_DEBUG_LOG("SoundTouch: Version: {:s}\n", mSoundTouch->getVersionString());

				// sync cache latency info for offset calc
				updateSTLatency();

				ST_DEBUG_LOG("SoundTouch: Initial latency: {:} samples ({:.1f}ms), Output sequence: {:} samples, Average latency: {:.1f}ms\n",
				             mInitialSTLatencySamples, (mInitialSTLatencySamples * 1000.0f) / mBaseSamplerate, mSTOutputSequenceSamples, mSTLatencySeconds * 1000.0);
			}

			// initialize bpm detection
			mBPMChunkSizeInSamples = static_cast<unsigned int>(mBaseSamplerate * 15.0f); // 15 seconds
			resetBPMDetection();

			ST_DEBUG_LOG("BPM: Initialized detection with chunk size {:} samples (15 seconds)\n", mBPMChunkSizeInSamples);
		}
	}
}

SoundTouchFilterInstance::~SoundTouchFilterInstance()
{
	// clear the active instance reference in parent
	if (mParent && mParent->mActiveInstance == this)
		mParent->mActiveInstance = nullptr;

	delete mBPMDetect;
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
		ST_DEBUG_LOG("Current position: {:f} seconds\n", mStreamPosition);
	}

	if (!mSourceInstance || !mSoundTouch)
	{
		// return silence if not initialized
		memset(aBuffer, 0, sizeof(float) * aSamplesToRead * mChannels);
		return aSamplesToRead;
	}

	// update SoundTouch parameters if they've changed, right at the start
	if (mNeedsSettingUpdate.load())
	{
		mNeedsSettingUpdate = false;

		ST_DEBUG_LOG("(Deferred) Updating speed: {:f}->{:f}, pitch: {:f}->{:f}\n", mSoundTouchSpeed.load(), mParent->mSpeedFactor.load(), mSoundTouchPitch.load(),
		             mParent->mPitchFactor.load());

		mSoundTouchSpeed = mParent->mSpeedFactor.load();
		mSoundTouchPitch = mParent->mPitchFactor.load();

		// actually update the parameters
		mSoundTouch->setTempo(mSoundTouchSpeed.load());
		mSoundTouch->setPitch(mSoundTouchPitch.load());

		// SoLoud AudioStreamInstance inherited, allows the main SoLoud mixer to advance the mStreamPosition by the correct proportional amount
		mSetRelativePlaySpeed = mOverallRelativePlaySpeed = mSoundTouchSpeed;

		updateSTLatency();
	}

	if (logThisCall)
		ST_DEBUG_LOG("speed: {:f}, pitch: {:f}\n", mSoundTouchSpeed.load(), mSoundTouchPitch.load());

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

		if (logThisCall && samplesReceived > 0)
		{
			const float samplesInSeconds = (static_cast<float>(samplesReceived) / mBaseSamplerate);
			ST_DEBUG_LOG("Updated position: received {:} samples ({:.4f} seconds), new position: {:.4f} seconds\n", samplesReceived, samplesInSeconds,
			             mStreamPosition);
		}
	}
	else if (logThisCall)
	{
		ST_DEBUG_LOG("No samples available from SoundTouch, returning silence\n");
	}

	if (logThisCall && mProcessingCounter % 100 == 0)
		ST_DEBUG_LOG("Position: mStreamPosition={:.3f}s, init_latency={:}, output_seq={:}, avg_latency={:.1f}ms, ratio={:.3f}, real_pos={:.3f}s\n", mStreamPosition,
		             mInitialSTLatencySamples, mSTOutputSequenceSamples, mSTLatencySeconds * 1000.0, mSoundTouch->getInputOutputSampleRatio(),
		             getRealStreamPosition());

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
	if (aSeconds <= 0)
		return this->rewind();

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
	ST_DEBUG_LOG("Rewinding\n");

	if (mStreamPosition == 0)
	{
		reSynchronize();
		ST_DEBUG_LOG("Position was already at the start, cleared ST buffers\n");
		return SO_NO_ERROR;
	}

	result res = UNKNOWN_ERROR;
	if (mSourceInstance)
		res = mSourceInstance->rewind();

	if (res == SO_NO_ERROR)
	{
		reSynchronize();
		ST_DEBUG_LOG("Rewinded, position now {:.3f}\n", mStreamPosition);
	}

	return res;
}

// end public methods

// private helpers below (not called by SoLoud)

void SoundTouchFilterInstance::requestSettingUpdate(float speed, float pitch)
{
	if (mSoundTouchSpeed.load() != speed || mSoundTouchPitch.load() != pitch)
		mNeedsSettingUpdate = true;
}

time SoundTouchFilterInstance::getRealStreamPosition() const
{
	if (!mSoundTouch)
		return mStreamPosition;

	// current output position is tracked by SoLoud in mStreamPosition
	// subtract SoundTouch's latency to get the real source position
	return std::max(0.0, mStreamPosition - mSTLatencySeconds.load());
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

unsigned int SoundTouchFilterInstance::calculateTargetBufferLevel(unsigned int aSamplesToRead, bool logThis)
{
	// get SoundTouch's processing requirements
	unsigned int nominalOutputSeq = mSTOutputSequenceSamples <= 0 ? SAMPLE_GRANULARITY * 2 : mSTOutputSequenceSamples;

	// target buffer level should be enough to satisfy the current request plus some headroom
	// we want at least 2x the nominal output sequence to ensure stable processing
	unsigned int baseTargetLevel = std::max(aSamplesToRead, static_cast<unsigned int>(nominalOutputSeq * 2));

	// add additional headroom for rate changes - more headroom for faster rates
	float headroomMultiplier = 1.0f + (mSoundTouchSpeed - 1.0f) * 0.5f;
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

		// process bpm detection on interleaved samples
		if (soundEngine->shouldDetectBPM())
			processBPMDetection(mInterleavedBuffer, static_cast<int>(samplesRead));

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

void SoundTouchFilterInstance::updateSTLatency()
{
	if (!mSoundTouch)
		return;

	mInitialSTLatencySamples = mSoundTouch->getSetting(SETTING_INITIAL_LATENCY);
	mSTOutputSequenceSamples = mSoundTouch->getSetting(SETTING_NOMINAL_OUTPUT_SEQUENCE);
	mSTLatencySeconds = std::max(0.0, (static_cast<double>(mInitialSTLatencySamples) - (static_cast<double>(mSTOutputSequenceSamples) / 2.0)) /
	                                      static_cast<double>(mBaseSamplerate));
}

void SoundTouchFilterInstance::reSynchronize()
{
	// clear SoundTouch buffers to reset its internal state
	if (mSoundTouch)
	{
		mSoundTouch->clear();
		updateSTLatency();
	}

	// reset bpm detection on seek/rewind
	resetBPMDetection();

	if (mSourceInstance)
	{
		// update the position tracking. without this, SoLoud wouldn't know that "we" (as in, this voice handle) has manually had its stream position/time changed
		// like on seek/rewind. normally, mStreamPosition and mStreamTime are advanced by SoLoud in the internal mixing function for all voices, so we only have to
		// take care to manually update it when seek/rewind are performed on the underlying stream.
		mStreamPosition = mSourceInstance->mStreamPosition;
		mStreamTime = mSourceInstance->mStreamTime;
	}
}

void SoundTouchFilterInstance::processBPMDetection(const soundtouch::SAMPLETYPE *interleavedSamples, int numSamples)
{
	if (numSamples == 0)
		return;

	if (!mBPMDetect && soundEngine->shouldDetectBPM()) // it was enabled post-stream init, so re-initialize it
		resetBPMDetection();

	if (!mBPMDetect)
		return;

	mBPMDetect->inputSamples(interleavedSamples, numSamples);
	mBPMSamplesProcessed += numSamples;

	// check if we've processed a full 15-second chunk
	if (mBPMSamplesProcessed >= mBPMChunkSizeInSamples)
	{
		float detectedBPM = mBPMDetect->getBpm() * mSoundTouchSpeed;

		if (detectedBPM > 0.0f)
		{
			mCurrentBPM = detectedBPM;
			ST_DEBUG_LOG("BPM: Detected {:f} BPM from {:} samples ({:.1f} seconds)\n", detectedBPM, mBPMSamplesProcessed,
			             static_cast<float>(mBPMSamplesProcessed) / mBaseSamplerate);
		}
		else
		{
			ST_DEBUG_LOG("BPM: No clear beat detected in {:} samples ({:.1f} seconds)\n", mBPMSamplesProcessed,
			             static_cast<float>(mBPMSamplesProcessed) / mBaseSamplerate);
		}

		// reset for next chunk
		resetBPMDetection();
	}
}

void SoundTouchFilterInstance::resetBPMDetection()
{
	delete mBPMDetect;
	mBPMDetect = nullptr;
	if (soundEngine->shouldDetectBPM())
		mBPMDetect = new soundtouch::BPMDetect(static_cast<int>(mChannels), static_cast<int>(mBaseSamplerate));
	mBPMSamplesProcessed = 0;
}

} // namespace SoLoud

#endif // MCENGINE_FEATURE_SOLOUD
