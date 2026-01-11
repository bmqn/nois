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
	static constexpr nois::f32_t kDefaultValue = 0.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 1;

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value;
	}
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

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value;
	}
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

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return (value / 1000.0f) * sampleRate;
	}
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

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value / 2.0f;
	}
};

struct GrainPhaseLockActive
{
	static constexpr const char* kTitle = "Grain Lock";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kGrainLockActive;
	static constexpr nois::f32_t kDefaultValue = 0.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 1;

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
	, mStretchActive(nullptr)
	, mStretchFactor(nullptr)
	, mGrainBlend(nullptr)
	, mGrainPhaseLockActive(nullptr)
	, mSource(&mSourceBuffer)
	, mTimeStretcher(nullptr)
	, mSampleRate(0.0)
{
	mStretchActive = CreateProcessor<parameter::StretchActive>(mRegistry);
	mStretchFactor = CreateProcessor<parameter::StretchFactor>(mRegistry);
	mGrainSize = CreateProcessor<parameter::GrainSize>(mRegistry);
	mGrainBlend = CreateProcessor<parameter::GrainBlend>(mRegistry);
	mGrainPhaseLockActive = CreateProcessor<parameter::GrainPhaseLockActive>(mRegistry);

	mTimeStretcher = nois::TimeStretcher::Create();
	mTimeStretcher->SetStretchActive(*mStretchActive);
	mTimeStretcher->SetStretchFactor(*mStretchFactor);
	mTimeStretcher->SetGrainSize(*mGrainSize);
	mTimeStretcher->SetGrainBlend(*mGrainBlend);
	mTimeStretcher->SetGrainLockActive(*mGrainPhaseLockActive);

	mParameterLookup[mStretchActive->GetPid()] = mStretchActive.get();
	mParameterLookup[mStretchFactor->GetPid()] = mStretchFactor.get();
	mParameterLookup[mGrainSize->GetPid()] = mGrainSize.get();
	mParameterLookup[mGrainBlend->GetPid()] = mGrainBlend.get();
	mParameterLookup[mGrainPhaseLockActive->GetPid()] = mGrainPhaseLockActive.get();

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
	, mGrainPhaseLockActive(nullptr)
{
	mStretchActive = CreateController<parameter::StretchActive>();
	mStretchFactor = CreateController<parameter::StretchFactor>();
	mGrainSize = CreateController<parameter::GrainSize>();
	mGrainBlend = CreateController<parameter::GrainBlend>();
	mGrainPhaseLockActive = CreateController<parameter::GrainPhaseLockActive>();
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
	parameters.addParameter(*mGrainPhaseLockActive);

	return result;
}

tresult PLUGIN_API NoisController::terminate()
{
	return EditController::terminate();
}
