#include "NoisVst3Plugin.hpp"

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/base/ustring.h>

#include <algorithm>

nois::f32_t ComputeRMS(const nois::FloatBuffer& buffer, size_t channelIndex)
{
	nois::f32_t sumSquares = 0.0f;
	nois::count_t numFrames = buffer.GetNumFrames();

	for (nois::count_t frameIndex = 0; frameIndex < numFrames; ++frameIndex)
	{
		nois::f32_t s = buffer(frameIndex, channelIndex);
		sumSquares += s * s;
	}

	return std::sqrt(sumSquares / static_cast<nois::f32_t>(numFrames));
}

class NoisVstParameter : public Vst::Parameter
{
public:
	NoisVstParameter(const char* title, int pid, double value, double min, double max, int steps = 0);

	Vst::ParamValue applyStep(Vst::ParamValue valuePlain) const;

	void toString(Vst::ParamValue valueNormalized, Vst::String128 string) const SMTG_OVERRIDE;
	bool fromString(const Vst::TChar* string, Vst::ParamValue& valueNormalized) const SMTG_OVERRIDE;

	Vst::ParamValue toPlain(Vst::ParamValue valueNormalized) const SMTG_OVERRIDE;
	Vst::ParamValue toNormalized(Vst::ParamValue valuePlain) const SMTG_OVERRIDE;

private:
	static constexpr Vst::TChar kOffStr[] = {'O','f','f','\0'};
	static constexpr Vst::TChar kOnStr[] = {'O','n','\0'};

private:
	double mMin;
	double mMax;
	int mSteps;
};

NoisVstParameter::NoisVstParameter(const char* title, int pid, double value, double min, double max, int steps)
	: Parameter(USTRING(title), pid)
	, mMin(min)
	, mMax(max)
	, mSteps(steps)
{
	setNormalized(toNormalized(value));
	setPrecision(2);
}

Vst::ParamValue NoisVstParameter::applyStep(Vst::ParamValue valuePlain) const
{
	if (mSteps <= 0)
	{
		return valuePlain;
	}

	double step = (mMax - mMin) / float(mSteps);
	int steps = (valuePlain - mMin) / step;

	return mMin + steps * step;
}

Vst::ParamValue NoisVstParameter::toPlain(Vst::ParamValue valueNormalized) const
{
	return applyStep(valueNormalized * (mMax - mMin) + mMin);
}

Vst::ParamValue NoisVstParameter::toNormalized(Vst::ParamValue valuePlain) const
{
	return (applyStep(valuePlain) - mMin) / (mMax - mMin);
}

void NoisVstParameter::toString(Vst::ParamValue valueNormalized, Vst::String128 string) const
{
	UString wrapper(string, USTRINGSIZE(Vst::String128));

	if (mSteps == 1)
	{
		wrapper.assign(valueNormalized > 0.0f ? kOnStr : kOffStr);
	}
	else
	{
		wrapper.printFloat(toPlain(valueNormalized), precision);
	}
}

bool NoisVstParameter::fromString(const Vst::TChar* string, Vst::ParamValue& valueNormalized) const
{
	UString wrapper(const_cast<Vst::TChar*>(string), USTRINGSIZE(Vst::String128));

	double value = 0.0;

	if (mSteps == 1 && std::memcmp(wrapper, kOffStr, sizeof(kOffStr)) == 0)
	{
		value = 0.0;
	}
	else if (mSteps == 1 && std::memcmp(wrapper, kOnStr, sizeof(kOnStr)) == 0)
	{
		value = 1.0;
	}
	else if (wrapper.scanFloat(value))
	{
		if (value < mMin)
		{
			value = mMin;
		}
		else if (value > mMax)
		{
			value = mMax;
		}
	
		valueNormalized = toNormalized(value);

		return true;
	}

	return false;
}

nois::Stream::Result NoisVstSource::Consume(nois::FloatBuffer& buffer, nois::f32_t sampleRate)
{
	buffer.Copy(mBuffer);

	return nois::Stream::Result::Success;
}

void NoisVstSource::PrepareToConsume(nois::count_t numFrames, nois::count_t numChannels, nois::f32_t sampleRate)
{
	mBuffer.Resize(numFrames, numChannels);
}

nois::FloatBuffer& NoisVstSource::GetBuffer()
{
	return mBuffer;
}

NoisPlugin::NoisPlugin()
	: mSource(nullptr)
	, mTimeStretcher(nullptr)
	, mStretchActive(nullptr)
	, mStretchFactor(nullptr)
	, mAuxSource(nullptr)
	, mAuxFilterBank(nullptr)
	, mSampleRate(0.0)
{
	mSource = nois::MakeRef<NoisVstSource>();
	mTimeStretcher = nois::CreateTimeStretcher(mSource);

	mStretchActive = nois::CreateParameter(
		nois::FloatBinderFunc_t([this]
		(nois::count_t frameIndex)
		{
			return mStretchActiveValues[frameIndex];
		}));
	mStretchFactor = nois::CreateParameter(
		nois::FloatBinderFunc_t([this]
		(nois::count_t frameIndex)
		{
			return mStretchFactorValues[frameIndex];
		}));
	mGrainGain = nois::CreateParameter(
		nois::FloatBinderFunc_t([this]
		(nois::count_t frameIndex)
		{
			return mGrainGainValues[frameIndex];
		}));
	mGrainPhaseInc = nois::CreateParameter(
		nois::FloatBinderFunc_t([this]
		(nois::count_t frameIndex)
		{
			return mGrainPhaseIncValues[frameIndex];
		}));
	mGrainLockActive = nois::CreateParameter(
		nois::FloatBinderFunc_t([this]
		(nois::count_t frameIndex)
		{
			return mGrainLockActiveValues[frameIndex];
		}));
	
	mTimeStretcher->SetStretchActive(mStretchActive);
	mTimeStretcher->SetStretchFactor(mStretchFactor);
	mTimeStretcher->SetGrainGain(mGrainGain);
	mTimeStretcher->SetGrainPhaseInc(mGrainPhaseInc);
	mTimeStretcher->SetGrainLockActive(mGrainLockActive);

	mAuxSource = nois::MakeRef<NoisVstSource>();
	mAuxFilterBank = nois::CreateFilterBank(mAuxSource, 16, 0.1f, 0.9f);

	setControllerClass(NoisController::kUid);
}

FUnknown* PLUGIN_API NoisPlugin::createInstance(void*)
{
	return static_cast<IAudioProcessor*>(new NoisPlugin());
}

tresult PLUGIN_API NoisPlugin::initialize(FUnknown* context)
{
	tresult result = AudioEffect::initialize(context);

	if (result != kResultOk)
	{
		return result;
	}

	addAudioInput(STR16("Input"), Vst::SpeakerArr:: kStereo);
	addAudioOutput(STR16("Output"), Vst::SpeakerArr::kStereo);

	addAudioInput(STR16("Aux Input"), Vst::SpeakerArr::kMono, Vst::BusTypes::kAux);

	return kResultOk;
}

tresult PLUGIN_API NoisPlugin::setState(IBStream* state)
{
	return kResultOk;
}

tresult PLUGIN_API NoisPlugin::getState(IBStream* state)
{
	return kResultOk;
}

tresult PLUGIN_API NoisPlugin::setupProcessing(Vst::ProcessSetup& setup)
{
	mSampleRate = setup.sampleRate;

	return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API NoisPlugin::process(Vst::ProcessData& data)
{
	if (data.numInputs == 0 || data.numOutputs == 0 || mSampleRate == 0.0)
	{
		return kResultOk;
	}

	if (mStretchActiveValues.size() != data.numSamples)
	{
		mStretchActiveValues.resize(data.numSamples, 0.0f);
	}
	if (mStretchFactorValues.size() != data.numSamples)
	{
		mStretchFactorValues.resize(data.numSamples, 1.0f);
	}
	if (mGrainGainValues.size() != data.numSamples)
	{
		mGrainGainValues.resize(data.numSamples, 1.0f);
	}
	if (mGrainPhaseIncValues.size() != data.numSamples)
	{
		mGrainPhaseIncValues.resize(data.numSamples, 1.0f);
	}
	if (mGrainLockActiveValues.size() != data.numSamples)
	{
		mGrainLockActiveValues.resize(data.numSamples, 1.0f);
	}

	if (data.inputParameterChanges)
	{
		std::unordered_map<int, int> currentSampleOffsets;
		currentSampleOffsets[kParameterTagStretchActive] = 0;
		currentSampleOffsets[kParameterTagStretchFactor] = 0;
		currentSampleOffsets[kParameterTagGrainPhaseInc] = 0;
		currentSampleOffsets[kParameterTagGrainLockActive] = 0;

		std::unordered_map<int, nois::f32_t> parameterLastValues;
		parameterLastValues[kParameterTagStretchActive] =
			mStretchActiveValues[data.numSamples - 1];
		parameterLastValues[kParameterTagStretchFactor] =
			mStretchFactorValues[data.numSamples - 1];
		parameterLastValues[kParameterTagGrainPhaseInc] =
			mGrainPhaseIncValues[data.numSamples - 1];
		parameterLastValues[kParameterTagGrainLockActive] =
			mGrainLockActiveValues[data.numSamples - 1];

		int numParamsChanged = data.inputParameterChanges->getParameterCount();
		for (int i = 0; i < numParamsChanged; i++)
		{
			Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
			if (!queue)
			{
				continue;
			}

			int numPoints = queue->getPointCount();
			for (int j = 0; j < numPoints; ++j)
			{
				Vst::ParamID pid = queue->getParameterId();

				std::vector<nois::f32_t>* parameterValues = nullptr;

				switch (pid)
				{
					case kParameterTagStretchActive:
						parameterValues = &mStretchActiveValues;
						break;
					case kParameterTagStretchFactor:
						parameterValues = &mStretchFactorValues;
						break;
					case kParameterTagGrainPhaseInc:
						parameterValues = &mGrainPhaseIncValues;
						break;
					case kParameterTagGrainLockActive:
						parameterValues = &mGrainLockActiveValues;
						break;
					default:
						break;
				}

				if (!parameterValues)
				{
					continue;
				}

				int& currentSampleOffset = currentSampleOffsets[pid];
				nois::f32_t &lastPlainValue = parameterLastValues[pid];

				int sampleOffset;
				Vst::ParamValue value;
				if (queue->getPoint(j, sampleOffset, value) == kResultTrue)
				{
					nois::f32_t plainValue = value;

					if (pid == kParameterTagStretchFactor)
					{
						plainValue = 1.0f + (16.0f - 1.0f) * value;
					}
					else if (pid == kParameterTagGrainPhaseInc)
					{
						plainValue = 0.01f + (1.0f - 0.01f) * value;
					}

					if (numPoints == 1)
					{
						for (;
							currentSampleOffset < data.numSamples;
							++currentSampleOffset)
						{
							(*parameterValues)[currentSampleOffset] =
								plainValue;
						}

						continue;
					}

					int nextSampleOffset = sampleOffset;
					int lastSampleOffset = currentSampleOffset;

					for (;
						currentSampleOffset <= nextSampleOffset &&
						currentSampleOffset < data.numSamples;
						++currentSampleOffset)
					{
						if (currentSampleOffset == nextSampleOffset ||
							nextSampleOffset == lastSampleOffset)
						{
							(*parameterValues)[currentSampleOffset] =
								plainValue;
						}
						else
						{
							float t =
								float(currentSampleOffset - lastSampleOffset) /
								float(nextSampleOffset - lastSampleOffset);

							(*parameterValues)[currentSampleOffset] =
								plainValue * t +
								lastPlainValue * (1.0f - t);
						}
					}

					lastPlainValue = plainValue;
				}
			}
		}
	}

	nois::FloatBuffer& sourceBuffer = mSource->GetBuffer();
	nois::FloatBuffer sinkBuffer;

	nois::FloatBuffer& auxSourceBuffer = mAuxSource->GetBuffer();
	nois::FloatBuffer auxSinkBuffer;

	auto& inSource = data.inputs[0];
	auto& inAuxSource = data.inputs[1];
	auto& outSink = data.outputs[0];

	sourceBuffer.Resize(data.numSamples, inSource.numChannels);
	sinkBuffer.Resize(data.numSamples, outSink.numChannels);

	auxSourceBuffer.Resize(data.numSamples, inAuxSource.numChannels);
	auxSinkBuffer.Resize(data.numSamples, inAuxSource.numChannels);

	for (int channel = 0; channel < inAuxSource.numChannels; ++channel)
	{
		float* inSamplesStart  = inAuxSource.channelBuffers32[channel];

		for (int sample = 0; sample < data.numSamples; ++sample)
		{
			auxSourceBuffer(sample, channel) = inSamplesStart[sample];
		}
	}

	// mAuxFilterBank->PrepareToConsume(auxSinkBuffer.GetNumFrames(), auxSinkBuffer.GetNumChannels(), mSampleRate);
	// mAuxFilterBank->Consume(auxSinkBuffer, mSampleRate);

	// float principalAuxFrequency = 0.0f;
	// float principalAuxBandRms = 0.0f;

	// auto auxBandRmses = mAuxFilterBank->GetBandRmses();

	// for (int i = 0; i < mAuxFilterBank->GetNumBands(); ++i)
	// {
	// 	float auxFrequency = mAuxFilterBank->GetBandFrequency(i, mSampleRate);
	// 	float auxBandRms = auxBandRmses[i]->Get();

	// 	if (auxBandRms > principalAuxBandRms)
	// 	{
	// 		principalAuxFrequency = auxFrequency;
	// 		principalAuxBandRms = auxBandRms;
	// 	}
	// }

	for (int channel = 0; channel < inSource.numChannels; ++channel)
	{
		float* inSamplesStart  = inSource.channelBuffers32[channel];

		for (int sample = 0; sample < data.numSamples; ++sample)
		{
			sourceBuffer(sample, channel) = inSamplesStart[sample];
		}
	}

	{
		nois::ScopedNoDenorms noDenorms;

		mTimeStretcher->PrepareToConsume(sinkBuffer.GetNumFrames(), sinkBuffer.GetNumChannels(), mSampleRate);
		mTimeStretcher->Consume(sinkBuffer, mSampleRate);
	}

	// static nois::Ref_t<nois::FloatConstantParameter> cutoffRatio = nullptr;
	// if (!cutoffRatio)
	// {
	// 	cutoffRatio = nois::CreateParameter(0.0f);
	// }
	// static nois::Ref_t<nois::BandpassFilter> bandpass = nullptr;
	// if (!bandpass)
	// {
	// 	bandpass = nois::CreateBandpassFilter(mTimeStretcher);
	// 	bandpass->SetCutoffRatio(cutoffRatio);
	// 	bandpass->SetQ(nois::CreateParameter(0.01f));
	// }
	// float cutoffFrequency = 1.0f;
	// if (principalAuxFrequency > 0.0f)
	// {
	// 	cutoffFrequency = principalAuxFrequency;
	// }
	// cutoffRatio->Set(0.5f * cutoffFrequency / float(mSampleRate));
	// bandpass->PrepareToConsume(sinkBuffer.GetNumFrames(), sinkBuffer.GetNumChannels(), mSampleRate);
	// bandpass->Consume(sinkBuffer, mSampleRate);

	// static nois::FloatConstantBlockParameterList bandGains;
	// if (bandGains.empty())
	// {
	// 	bandGains.resize(auxBandRmses.size(), nois::CreateBlockParameter(1.0f));
	// 	nois::FloatBlockParameterList bandGainsCopy;
	// 	bandGainsCopy.reserve(bandGains.size());
	// 	for (auto& bandGain : bandGains)
	// 	{
	// 		bandGainsCopy.emplace_back(bandGain);
	// 	}
	// 	mAuxFilterBank->SetBandGains(bandGainsCopy);
	// }
	// for (int i = 0; i < bandGains.size(); ++i)
	// {
	// 	bandGains[i]->Set(auxBandRmses[i]->Get());
	// }
	// auxSourceBuffer.Copy(sourceBuffer);
	// mAuxFilterBank->PrepareToConsume(auxSinkBuffer.GetNumFrames(), auxSinkBuffer.GetNumChannels(), mSampleRate);
	// mAuxFilterBank->Consume(auxSinkBuffer, mSampleRate);
	// sinkBuffer.Copy(auxSinkBuffer);
	// for (int i = 0; i < bandGains.size(); ++i)
	// {
	// 	bandGains[i]->Set(1.0f);
	// }
	
	for (int channel = 0; channel < outSink.numChannels; ++channel)
	{
		float* outSamplesStart = outSink.channelBuffers32[channel];

		for (int sample = 0; sample < data.numSamples; ++sample)
		{
			outSamplesStart[sample] = sinkBuffer(sample, channel);
		}
	}

	return kResultOk;
}

FUnknown* PLUGIN_API NoisController::createInstance(void*)
{
	return static_cast<Steinberg::Vst::IEditController*>(new NoisController());
}

tresult PLUGIN_API NoisController::initialize(FUnknown* context)
{
	tresult result = EditController::initialize(context);

	if (result != kResultOk)
	{
		return result;
	}

	parameters.addParameter(new NoisVstParameter(
		"Stretch Active",
		kParameterTagStretchActive,
		0.0,
		0.0, 1.0,
		1));
	parameters.addParameter(new NoisVstParameter(
		"Stretch Factor",
		kParameterTagStretchFactor,
		1.0,
		1.0, 16.0));
	parameters.addParameter(new NoisVstParameter(
		"Grain Phase Increment",
		kParameterTagGrainPhaseInc,
		1.0,
		0.01, 1.0));
	parameters.addParameter(new NoisVstParameter(
		"Grain Lock Active",
		kParameterTagGrainLockActive,
		0.0,
		0.0, 1.0,
		1));

	return result;
}

tresult PLUGIN_API NoisController::terminate()
{
	return EditController::terminate();
}
