#include "NoisVst3Plugin.hpp"

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/base/ustring.h>

#include <algorithm>

namespace parameter
{
enum
{
	kStretchActive,
	kStretchFactor,
	kGrainSize,
	kGrainBlend,
	kGrainLockActive
};

struct StretchActive
{
	static constexpr const char* kTitle = "Stretch";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kStretchActive;
	static constexpr nois::f32_t kDefaultValue = -1.0f;
	static constexpr nois::f32_t kMinValue = -1.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 1;
};

struct StretchFactor
{
	static constexpr const char* kTitle = "Factor";
	static constexpr const char* kUnits = "x";
	static constexpr Vst::ParamID kPid = kStretchFactor;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = 1.0f;
	static constexpr nois::f32_t kMaxValue = 16.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct GrainSize
{
	static constexpr const char* kTitle = "Grain Size";
	static constexpr const char* kUnits = "ms";
	static constexpr Vst::ParamID kPid = kGrainSize;
	static constexpr nois::f32_t kDefaultValue = 10.0f;
	static constexpr nois::f32_t kMinValue = 0.5f;
	static constexpr nois::f32_t kMaxValue = 100.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct GrainBlend
{
	static constexpr const char* kTitle = "Grain Blend";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kGrainBlend;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct GrainLockActive
{
	static constexpr const char* kTitle = "Grain Lock";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kGrainLockActive;
	static constexpr nois::f32_t kDefaultValue = -1.0f;
	static constexpr nois::f32_t kMinValue = -1.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 1;
};
}

NoisPlugin::NoisPlugin()
	: mSourceBuffer()
	, mSinkBuffer()
	, mStretchActive(nullptr)
	, mStretchFactor(nullptr)
	, mGrainBlend(nullptr)
	, mGrainLockActive(nullptr)
	, mSource(&mSourceBuffer)
	, mTimeStretcher(nullptr)
	, mSampleRate(0.0)
{
	mStretchActive = NoisVstProcessorParameter::Create<parameter::StretchActive>(mRegistry);
	mStretchFactor = NoisVstProcessorParameter::Create<parameter::StretchFactor>(mRegistry);
	mGrainSize = NoisVstProcessorParameter::Create<parameter::GrainSize>(mRegistry);
	mGrainBlend = NoisVstProcessorParameter::Create<parameter::GrainBlend>(mRegistry);
	mGrainLockActive = NoisVstProcessorParameter::Create<parameter::GrainLockActive>(mRegistry);

	auto grainSize =
		mRegistry.CreateTransformer(
			*mGrainSize,
			[this](nois::f32_t x, nois::count_t) { return (x / 1000.0f) * mSampleRate; });
	auto grainBlendNormalized =
		mRegistry.CreateTransformer(
			*mGrainBlend,
			[this](nois::f32_t x, nois::count_t) { return x / 2.0f; });

	mTimeStretcher = nois::TimeStretcher::Create();
	mTimeStretcher->SetStretchActive(*mStretchActive);
	mTimeStretcher->SetStretchFactor(*mStretchFactor);
	mTimeStretcher->SetGrainSize(grainSize);
	mTimeStretcher->SetGrainBlend(grainBlendNormalized);
	mTimeStretcher->SetGrainLockActive(*mGrainLockActive);

	mParameterLookup[mStretchActive->GetPid()] = mStretchActive.get();
	mParameterLookup[mStretchFactor->GetPid()] = mStretchFactor.get();
	mParameterLookup[mGrainSize->GetPid()] = mGrainSize.get();
	mParameterLookup[mGrainBlend->GetPid()] = mGrainBlend.get();
	mParameterLookup[mGrainLockActive->GetPid()] = mGrainLockActive.get();

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

	nois::FloatBuffer& sourceBuffer = mSourceBuffer;
	nois::FloatBuffer& sinkBuffer = mSinkBuffer;

	auto& inSource = data.inputs[0];
	auto& outSink = data.outputs[0];

	sourceBuffer.Resize(data.numSamples, inSource.numChannels);
	sinkBuffer.Resize(data.numSamples, outSink.numChannels);

	{
		NOIS_PROFILE_SCOPE_NAMED("Read from source");

		for (int c = 0; c < inSource.numChannels; ++c)
		{
			const float* samples = inSource.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				sourceBuffer(f, c) = samples[f];
			}
		}
	}

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

		mRegistry.Prepare(data.numSamples, mSampleRate);
	}

	{
		NOIS_PROFILE_SCOPE_NAMED("Prepare processors");

		mTimeStretcher->Prepare(data.numSamples, outSink.numChannels, mSampleRate);
	}

	{
		nois::ScopedNoDenorms noDenorms;

		NOIS_PROFILE_SCOPE_NAMED("Process processors");

		mTimeStretcher->Process(sourceBuffer, sinkBuffer);
	}
	
	{
		NOIS_PROFILE_SCOPE_NAMED("Write to sink");

		for (int c = 0; c < outSink.numChannels; ++c)
		{
			float* samples = outSink.channelBuffers32[c];
			for (int f = 0; f < data.numSamples; ++f)
			{
				samples[f] = sinkBuffer(f, c);
			}
		}
	}

	return kResultOk;
}

NoisController::NoisController()
	: mStretchActive(nullptr)
	, mStretchFactor(nullptr)
	, mGrainSize(nullptr)
	, mGrainBlend(nullptr)
	, mGrainLockActive(nullptr)
{
	mStretchActive = NoisVstControllerParameter::Create<parameter::StretchActive>();
	mStretchFactor = NoisVstControllerParameter::Create<parameter::StretchFactor>();
	mGrainSize = NoisVstControllerParameter::Create<parameter::GrainSize>();
	mGrainBlend = NoisVstControllerParameter::Create<parameter::GrainBlend>();
	mGrainLockActive = NoisVstControllerParameter::Create<parameter::GrainLockActive>();
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

	parameters.addParameter(*mStretchActive);
	parameters.addParameter(*mStretchFactor);
	parameters.addParameter(*mGrainSize);
	parameters.addParameter(*mGrainBlend);
	parameters.addParameter(*mGrainLockActive);

	return result;
}

tresult PLUGIN_API NoisController::terminate()
{
	return EditController::terminate();
}
