#include "NoisVst3Processor.hpp"

#include <pluginterfaces/base/funknown.h>
#include <public.sdk/source/main/pluginfactory_constexpr.h>

#include <algorithm>

namespace parameter
{
enum
{
	kStretchActive,
	kStretchFactor,
	kStretchLength,
	kGrainSize,
	kGrainBlend,
	kGrainPhaseInc,
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
	static constexpr const char* kOptions[] = { "Off", "On" };
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

struct StretchLength
{
	static constexpr const char* kTitle = "Length";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kStretchLength;
	static constexpr nois::f32_t kDefaultValue = 5.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 7.0f;
	static constexpr nois::s32_t kNumSteps = 7;
	static constexpr const char* kOptions[] = { "1/32", "1/16", "1/8", "1/4", "1/2", "1", "2", "4" };
};

struct GrainSize
{
	static constexpr const char* kTitle = "Grain Size";
	static constexpr const char* kUnits = "ms";
	static constexpr Vst::ParamID kPid = kGrainSize;
	static constexpr nois::f32_t kDefaultValue = 75.0f;
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

struct GrainPhaseInc
{
	static constexpr const char* kTitle = "Pitch";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kGrainPhaseInc;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = -1.5f;
	static constexpr nois::f32_t kMaxValue = 1.5f;
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
	static constexpr const char* kOptions[] = { "Off", "On" };
};
}

class Vst3StretchController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0x9ee7ec38, 0x41254df7, 0x8863c27f, 0x3782767d)

public:
	Vst3StretchController()
		: mStretchActive(nullptr)
		, mStretchFactor(nullptr)
		, mStretchLength(nullptr)
		, mGrainSize(nullptr)
		, mGrainBlend(nullptr)
		, mGrainPhaseInc(nullptr)
		, mGrainLockActive(nullptr)
	{
		mStretchActive = NoisVstControllerParameter::Create<parameter::StretchActive>();
		mStretchFactor = NoisVstControllerParameter::Create<parameter::StretchFactor>();
		mStretchLength = NoisVstControllerParameter::Create<parameter::StretchLength>();
		mGrainSize = NoisVstControllerParameter::Create<parameter::GrainSize>();
		mGrainBlend = NoisVstControllerParameter::Create<parameter::GrainBlend>();
		mGrainPhaseInc = NoisVstControllerParameter::Create<parameter::GrainPhaseInc>();
		mGrainLockActive = NoisVstControllerParameter::Create<parameter::GrainLockActive>();
	}

	static FUnknown* PLUGIN_API createInstance(void*)
	{
		return static_cast<Steinberg::Vst::IEditController*>(new Vst3StretchController());
	}

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE
	{
		tresult result = EditController::initialize(context);

		if (result != kResultOk)
		{
			return result;
		}

		parameters.addParameter(*mStretchActive);
		parameters.addParameter(*mStretchFactor);
		parameters.addParameter(*mStretchLength);
		parameters.addParameter(*mGrainSize);
		parameters.addParameter(*mGrainBlend);
		parameters.addParameter(*mGrainPhaseInc);
		parameters.addParameter(*mGrainLockActive);

		return result;
	}

	tresult PLUGIN_API terminate() SMTG_OVERRIDE
	{
		return EditController::terminate();
	}

private:
	nois::Own_t<NoisVstControllerParameter> mStretchActive;
	nois::Own_t<NoisVstControllerParameter> mStretchFactor;
	nois::Own_t<NoisVstControllerParameter> mStretchLength;
	nois::Own_t<NoisVstControllerParameter> mGrainSize;
	nois::Own_t<NoisVstControllerParameter> mGrainBlend;
	nois::Own_t<NoisVstControllerParameter> mGrainPhaseInc;
	nois::Own_t<NoisVstControllerParameter> mGrainLockActive;
};

class Vst3StretchProcessor : public NoisVstProcessor<Vst3StretchProcessor, Vst3StretchController>
{
public:
	inline static DECLARE_UID(kUid, 0xe098fdac, 0xc4454fdd, 0xb14a0ef5, 0xc27122b1)

private:
	enum class NoteDivision
	{
		ThirtySecond,
		Sixteenth,
		Eighth,
		Quarter,
		Half,
		Whole,
		Double,
		Quadruple
	};

public:
	Vst3StretchProcessor()
		: mStretchActive(nullptr)
		, mStretchFactor(nullptr)
		, mStretchLength(nullptr)
		, mGrainSize(nullptr)
		, mGrainBlend(nullptr)
		, mGrainPhaseInc(nullptr)
		, mGrainLockActive(nullptr)
		, mTimeStretcher(nullptr)
	{
		mStretchActive = CreateParameter<parameter::StretchActive>();
		mStretchFactor = CreateParameter<parameter::StretchFactor>();
		mStretchLength = CreateParameter<parameter::StretchLength>();
		mGrainSize = CreateParameter<parameter::GrainSize>();
		mGrainBlend = CreateParameter<parameter::GrainBlend>();
		mGrainPhaseInc = CreateParameter<parameter::GrainPhaseInc>();
		mGrainLockActive = CreateParameter<parameter::GrainLockActive>();

		auto stretchTimeMs =
			CreateBlockTransformer(
				[](nois::f32_t bpm, nois::f32_t length)
				{
					int lengthClamped = std::clamp(static_cast<int>(length), 0, 7);
					NoteDivision division = static_cast<NoteDivision>(lengthClamped);
					return (60000.0f / bpm) * DivisionToBeats(division);
				},
				GetBlockTempo(),
				*mStretchLength);

		auto grainSize =
			mGrainSize->Transform(
				[this](nois::f32_t x, nois::count_t)
				{
					return (x / 1000.0f) * mSampleRate;
				});
		auto grainBlendNormalized =
			mGrainBlend->Transform(
				[](nois::f32_t x, nois::count_t)
				{
					return x / 2.0f;
				});

		mTimeStretcher = nois::TimeStretcher::Create();
		mTimeStretcher->SetStretchTimeMs(stretchTimeMs);
		mTimeStretcher->SetStretchActive(*mStretchActive);
		mTimeStretcher->SetStretchFactor(*mStretchFactor);
		mTimeStretcher->SetGrainSize(grainSize);
		mTimeStretcher->SetGrainBlend(grainBlendNormalized);
		mTimeStretcher->SetGrainPhaseInc(*mGrainPhaseInc);
		mTimeStretcher->SetGrainLockActive(*mGrainLockActive);
	}

protected:
	void Process(
		nois::ConstFloatBufferView sourceBuffer,
		nois::FloatBufferView sinkBuffer) override
	{
		{
			NOIS_PROFILE_SCOPE_NAMED("Prepare processors");

			mTimeStretcher->Prepare(sourceBuffer.GetNumFrames(), sourceBuffer.GetNumChannels(), mSampleRate);
		}

		{
			nois::ScopedNoDenorms noDenorms;

			NOIS_PROFILE_SCOPE_NAMED("Process processors");

			mTimeStretcher->Process(sourceBuffer, sinkBuffer);
		}
	}

private:
	static float DivisionToBeats(NoteDivision division)
	{
		switch (division)
		{
			case NoteDivision::ThirtySecond: return 0.125f;
			case NoteDivision::Sixteenth:    return 0.25f;
			case NoteDivision::Eighth:       return 0.5f;
			case NoteDivision::Quarter:      return 1.0f;
			case NoteDivision::Half:         return 2.0f;
			case NoteDivision::Whole:        return 4.0f;
			case NoteDivision::Double:       return 8.0f;
			case NoteDivision::Quadruple:    return 16.0f;
		}

		return 1.0f;
	}

private:
	nois::Own_t<NoisVstProcessorParameter> mStretchActive;
	nois::Own_t<NoisVstProcessorParameter> mStretchFactor;
	nois::Own_t<NoisVstProcessorParameter> mStretchLength;
	nois::Own_t<NoisVstProcessorParameter> mGrainSize;
	nois::Own_t<NoisVstProcessorParameter> mGrainBlend;
	nois::Own_t<NoisVstProcessorParameter> mGrainPhaseInc;
	nois::Own_t<NoisVstProcessorParameter> mGrainLockActive;

	nois::Ref_t<nois::TimeStretcher> mTimeStretcher;
};

using AudioEffectClass = NoisVstProcessor<Vst3StretchProcessor, Vst3StretchController>;
using ComponentControllerClass = Vst3StretchController;

BEGIN_FACTORY_DEF(
	"Nois",
	"http://www.nois.co.uk",
	"mailto:info@nois.co.uk",
	2)

DEF_CLASS(
	Vst3StretchProcessor::kUid,
	PClassInfo::kManyInstances,
	kVstAudioEffectClass,
	"NzStretch",
	Vst::kDistributable,
	Vst::PlugType::kFx,
	"1.0.0",
	kVstVersionString,
	AudioEffectClass::createInstance,
	nullptr)

DEF_CLASS(
	Vst3StretchController::kUid,
	PClassInfo::kManyInstances,
	kVstComponentControllerClass,
	"NzStretch",
	0,
	"",
	"1.0.0",
	kVstVersionString,
	ComponentControllerClass::createInstance,
	nullptr)

END_FACTORY
