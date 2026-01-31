#include "NoisVst3Processor.hpp"

#include <pluginterfaces/base/funknown.h>
#include <public.sdk/source/main/pluginfactory_constexpr.h>

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
	static constexpr nois::f32_t kDefaultValue = 100.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 100.0f;
	static constexpr nois::s32_t kNumSteps = 0;
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
};

}

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0x8fa5b042, 0xda9a4944, 0x835130d0, 0xf0c6c27a)

public:
	NoisController()
		: mWet(nullptr)
		, mDecayMs(nullptr)
	{
		mWet = NoisVstControllerParameter::Create<parameter::Wet>();
		mDecayMs = NoisVstControllerParameter::Create<parameter::DecayMs>();
	}

	static FUnknown* PLUGIN_API createInstance(void*)
	{
		return static_cast<Steinberg::Vst::IEditController*>(new NoisController());
	}

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE
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

	tresult PLUGIN_API terminate() SMTG_OVERRIDE
	{
		return EditController::terminate();
	}

private:
	nois::Own_t<NoisVstControllerParameter> mWet;
	nois::Own_t<NoisVstControllerParameter> mDecayMs;
};

class VstVerb : public NoisVstProcessor<VstVerb, NoisController>
{
public:
	inline static DECLARE_UID(kUid, 0x2ae41d67, 0x8b034025, 0x9b11c63f, 0xd5b5a738)

public:
	VstVerb()
		: mRegistry()
		, mWet(nullptr)
		, mDecayMs(nullptr)
		, mReverb()
	{
		mWet = NoisVstProcessorParameter::Create<parameter::Wet>(mRegistry);
		mDecayMs = NoisVstProcessorParameter::Create<parameter::DecayMs>(mRegistry);

		Register(mWet.get());
		Register(mDecayMs.get());

		auto wetNormalized =
			mRegistry.CreateBlockTransformer(
				*mWet,
				[](nois::f32_t x) { return x / 100.0f; });
		
		mReverb = nois::Reverb::Create();
		mReverb->SetWet(wetNormalized);
		mReverb->SetDecayMs(*mDecayMs);
	}

protected:
	void Process(
		nois::ConstFloatBufferView sourceBuffer,
		nois::FloatBufferView sinkBuffer) override
	{
		{
			NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

			mRegistry.Prepare(sourceBuffer.GetNumFrames(), mSampleRate);
		}

		{
			NOIS_PROFILE_SCOPE_NAMED("Prepare processors");

			mReverb->Prepare(sourceBuffer.GetNumFrames(), sourceBuffer.GetNumChannels(), mSampleRate);
		}

		{
			nois::ScopedNoDenorms noDenorms;

			NOIS_PROFILE_SCOPE_NAMED("Process processors");

			mReverb->Process(sourceBuffer, sinkBuffer);
		}
	}

private:
	nois::FloatParameterRegistry mRegistry;
	nois::Own_t<NoisVstProcessorParameter> mWet;
	nois::Own_t<NoisVstProcessorParameter> mDecayMs;

	nois::Ref_t<nois::Reverb> mReverb;
};

using AudioEffectClass = NoisVstProcessor<VstVerb, NoisController>;
using ComponentControllerClass = NoisController;

BEGIN_FACTORY_DEF(
	"Nois",
	"http://www.nois.co.uk",
	"mailto:info@nois.co.uk",
	2)

DEF_CLASS(
	VstVerb::kUid,
	PClassInfo::kManyInstances,
	kVstAudioEffectClass,
	"NzVerb",
	Vst::kDistributable,
	Vst::PlugType::kFxReverb,
	"1.0.0",
	kVstVersionString,
	AudioEffectClass::createInstance,
	nullptr)

DEF_CLASS(
	NoisController::kUid,
	PClassInfo::kManyInstances,
	kVstComponentControllerClass,
	"NzVerb",
	0,
	"",
	"1.0.0",
	kVstVersionString,
	ComponentControllerClass::createInstance,
	nullptr)

END_FACTORY

