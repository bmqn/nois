#include "NoisVst3Plugin.hpp"

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/base/ustring.h>

#include <algorithm>

namespace parameter
{

enum
{
	kWet,
	kDecayMs
};

struct Wet
{
	static constexpr const char* kTitle = "Wet";
	static constexpr const char* kUnits = "%";
	static constexpr Vst::ParamID kPid = kWet;
	static constexpr nois::f32_t kDefaultValue = 50.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 100.0f;
	static constexpr nois::s32_t kNumSteps = 0;

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return (value - kMinValue) / (kMaxValue - kMinValue);
	}
};

struct DecayMs
{
	static constexpr const char* kTitle = "Decay";
	static constexpr const char* kUnits = "ms";
	static constexpr Vst::ParamID kPid = kDecayMs;
	static constexpr nois::f32_t kDefaultValue = 50.0f;
	static constexpr nois::f32_t kMinValue = 1.0f;
	static constexpr nois::f32_t kMaxValue = 10000.0f;
	static constexpr nois::s32_t kNumSteps = 0;

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value;
	}
};

}

NoisPlugin::NoisPlugin()
	: mSourceBuffer()
	, mSinkBuffer()
	, mRegistry()
	, mWet(nullptr)
	, mDecayMs(nullptr)
	, mReverb()
	, mSampleRate(0.0)
{
	mWet = NoisVstProcessorParameter::Create<parameter::Wet>(mRegistry);
	mDecayMs = NoisVstProcessorParameter::Create<parameter::DecayMs>(mRegistry);
	
	mReverb = nois::Reverb::Create();
	mReverb->SetWet(*mWet);
	mReverb->SetDecayMs(*mDecayMs);

	mParameterLookup[mWet->GetPid()] = mWet.get();
	mParameterLookup[mDecayMs->GetPid()] = mDecayMs.get();

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
	Steinberg::Vst::SpeakerArrangement arrangement;

	if (getBusArrangement(Steinberg::Vst::kInput, 0, arrangement) == Steinberg::kResultTrue)
	{
		nois::s32_t numChannels = Steinberg::Vst::SpeakerArr::getChannelCount(arrangement);
		mSourceBuffer.Reserve(setup.maxSamplesPerBlock, numChannels);
	}

	if (getBusArrangement(Steinberg::Vst::kOutput, 0, arrangement) == Steinberg::kResultTrue)
	{
		nois::s32_t numChannels = Steinberg::Vst::SpeakerArr::getChannelCount(arrangement);
		mSinkBuffer.Reserve(setup.maxSamplesPerBlock, numChannels);
	}

	mSampleRate = setup.sampleRate;

	return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API NoisPlugin::process(Vst::ProcessData& data)
{
	if (data.numInputs == 0 || data.numOutputs == 0 || mSampleRate == 0.0)
	{
		return kResultOk;
	}

	NOIS_PROFILE_MARK();

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

		for (auto it = mParameterLookup.begin();
			it != mParameterLookup.end();
			++it)
		{
			it->second->Prepare(data.numSamples, mSampleRate);
		}
	}

	if (data.inputParameterChanges)
	{
		NOIS_PROFILE_SCOPE_NAMED("Process parameter changes");

		int numParamsChanged = data.inputParameterChanges->getParameterCount();
		for (int i = 0; i < numParamsChanged; i++)
		{
			Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
			if (!queue)
			{
				continue;
			}
			
			Vst::ParamID pid = queue->getParameterId();
			auto it = mParameterLookup.find(pid);
			if (it == mParameterLookup.end())
			{
				continue;
			}

			auto& parameter = it->second;
			nois::count_t currentSampleOffset = 0;
			nois::f32_t lastValuePlain = parameter->GetLastPlain();
			int numPoints = queue->getPointCount();
			for (int j = 0; j < numPoints; ++j)
			{
				int sampleOffset;
				Vst::ParamValue valueNormalized;
				if (queue->getPoint(j, sampleOffset, valueNormalized) == kResultTrue)
				{
					nois::f32_t valuePlain = parameter->ToPlain(valueNormalized);

					if (numPoints == 1)
					{
						parameter->WritePlain(
							currentSampleOffset,
							data.numSamples - currentSampleOffset,
							valuePlain);

						break;
					}

					nois::count_t nextSampleOffset = sampleOffset;
					nois::count_t lastSampleOffset = currentSampleOffset;

					for (;
						currentSampleOffset <= nextSampleOffset &&
						currentSampleOffset < data.numSamples;
						++currentSampleOffset)
					{
						if (currentSampleOffset == nextSampleOffset ||
							nextSampleOffset == lastSampleOffset)
						{
							parameter->WritePlain(
								currentSampleOffset,
								valuePlain);
						}
						else
						{
							nois::f32_t t =
								nois::f32_t(currentSampleOffset - lastSampleOffset) /
								nois::f32_t(nextSampleOffset - lastSampleOffset);

							parameter->WritePlain(
								currentSampleOffset,
								valuePlain * t +
								lastValuePlain * (1.0f - t));
						}
					}

					lastValuePlain = valuePlain;
				}
			}
		}
	}

	auto& inSource = data.inputs[0];
	auto& outSink = data.outputs[0];

	mSourceBuffer.Resize(data.numSamples, inSource.numChannels);
	mSinkBuffer.Resize(data.numSamples, outSink.numChannels);

	{
		NOIS_PROFILE_SCOPE_NAMED("Read from source");

		for (int c = 0; c < inSource.numChannels; ++c)
		{
			const float* samples = inSource.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				mSourceBuffer(f, c) = samples[f];
			}
		}
	}

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

		mRegistry.Prepare(mSinkBuffer.GetNumFrames(), mSampleRate);
	}

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare processors");

		mReverb->Prepare(mSinkBuffer.GetNumFrames(), mSinkBuffer.GetNumChannels(), mSampleRate);
	}

	{
		nois::ScopedNoDenorms noDenorms;

		NOIS_PROFILE_SCOPE_NAMED("Process processors");

		mReverb->Process(mSourceBuffer, mSinkBuffer);
	}
	
	{
		NOIS_PROFILE_SCOPE_NAMED("Write to sink");

		for (int c = 0; c < outSink.numChannels; ++c)
		{
			float* samples = outSink.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				samples[f] = mSinkBuffer(f, c);
			}
		}
	}

	return kResultOk;
}

NoisController::NoisController()
	: mWet(nullptr)
	, mDecayMs(nullptr)
{
	mWet = NoisVstControllerParameter::Create<parameter::Wet>();
	mDecayMs = NoisVstControllerParameter::Create<parameter::DecayMs>();
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

	parameters.addParameter(*mWet);
	parameters.addParameter(*mDecayMs);

	return result;
}

tresult PLUGIN_API NoisController::terminate()
{
	return EditController::terminate();
}
