#include "NoisVst3Processor.hpp"

#include <pluginterfaces/base/funknown.h>
#include <public.sdk/source/main/pluginfactory_constexpr.h>

#include <algorithm>

namespace parameter
{

enum
{
	kSubFreq,
	kDrive,
	kWet
};


struct SubFreq
{
	static constexpr const char* kTitle = "Sub Frequency";
	static constexpr const char* kUnits = "Hz";
	static constexpr Vst::ParamID kPid = kSubFreq;
	static constexpr nois::f32_t kDefaultValue = 80.0f;
	static constexpr nois::f32_t kMinValue = 50.0f;
	static constexpr nois::f32_t kMaxValue = 140.0f;
	static constexpr nois::s32_t kNumSteps = 0;

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value / (sampleRate * 0.5f);
	}
};

struct Drive
{
	static constexpr const char* kTitle = "Drive";
	static constexpr const char* kUnits = "dB";
	static constexpr Vst::ParamID kPid = kDrive;
	static constexpr nois::f32_t kDefaultValue = 0.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
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

struct Wet
{
	static constexpr const char* kTitle = "Wet";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kWet;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
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

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0xa4a2f37a, 0xafa543af, 0x803e37c8, 0x35785df8)

public:
	NoisController()
		: mSubFreq(nullptr)
		, mDrive(nullptr)
		, mWet(nullptr)
	{
		mSubFreq = NoisVstControllerParameter::Create<parameter::SubFreq>();
		mDrive = NoisVstControllerParameter::Create<parameter::Drive>();
		mWet = NoisVstControllerParameter::Create<parameter::Wet>();
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

		parameters.addParameter(*mSubFreq);
		parameters.addParameter(*mDrive);
		parameters.addParameter(*mWet);

		return result;
	}

	tresult PLUGIN_API terminate() SMTG_OVERRIDE
	{
		return EditController::terminate();
	}

private:
	nois::Own_t<NoisVstControllerParameter> mSubFreq;
	nois::Own_t<NoisVstControllerParameter> mDrive;
	nois::Own_t<NoisVstControllerParameter> mWet;
};

class VstDistort : public NoisVstProcessor<VstDistort, NoisController>
{
public:
	inline static DECLARE_UID(kUid, 0x51632f93, 0x63414709, 0x8f2949e3, 0x49fe1815)

public:
	VstDistort()
		: mHpBuffer()
		, mDistBuffer()
		, mRegistry()
		, mSubFreq(nullptr)
		, mDrive(nullptr)
		, mWet(nullptr)
		, mHpFilter(nullptr)
		, mLpFilter(nullptr)
		, mAp1Filter(nullptr)
		, mAp2Filter(nullptr)
		, mDist(nullptr)
		, mLpDistFilter(nullptr)
	{
		mSubFreq = NoisVstProcessorParameter::Create<parameter::SubFreq>(mRegistry);
		mDrive = NoisVstProcessorParameter::Create<parameter::Drive>(mRegistry);
		mWet = NoisVstProcessorParameter::Create<parameter::Wet>(mRegistry);

		Register(mSubFreq.get());
		Register(mDrive.get());
		Register(mWet.get());

		auto ap1CutoffRatio =
			mRegistry.CreateBlockTransformer(*mSubFreq,
				[](float x) { return std::clamp(x * 0.7f, 0.002f, 0.02f); });
		auto ap2CutoffRatio =
			mRegistry.CreateBlockTransformer(*mSubFreq,
				[](float x) { return std::clamp(x * 1.2f, 0.002f, 0.03f); });
		auto apQ =
			mRegistry.CreateBlockConstant(0.67f);
		auto distWet =
			mRegistry.CreateConstant(1.0f);
		auto lpDistCutoffRatio =
			mRegistry.CreateBlockTransformer(*mSubFreq,
				[](nois::f32_t x) { return x * 2.0f; });

		mHpFilter = nois::Filter::Create(nois::Filter::k_LR4High);
		mHpFilter->SetCutoffRatio(*mSubFreq);

		mLpFilter = nois::Filter::Create(nois::Filter::k_LR4Low);
		mLpFilter->SetCutoffRatio(*mSubFreq);

		mAp1Filter = nois::AllpassFilter::Create(nois::AllpassFilter::k_RBJBiquad);
		mAp1Filter->SetCutoffRatio(ap1CutoffRatio);
		mAp1Filter->SetQ(apQ);

		mAp2Filter = nois::AllpassFilter::Create(nois::AllpassFilter::k_RBJBiquad);
		mAp2Filter->SetCutoffRatio(ap2CutoffRatio);
		mAp2Filter->SetQ(apQ);

		mDist = nois::Distorter::Create(nois::Distorter::k_Tanh);
		mDist->SetDriveDb(*mDrive);
		mDist->SetWet(distWet);

		mLpDistFilter = nois::Filter::Create(nois::Filter::k_LR4Low);
		mLpDistFilter->SetCutoffRatio(lpDistCutoffRatio);
	}

protected:
	void Process(
		nois::ConstFloatBufferView sourceBuffer,
		nois::FloatBufferView sinkBuffer) override
	{
		mHpBuffer.Resize(sourceBuffer.GetNumFrames(), sourceBuffer.GetNumChannels());
		mDistBuffer.Resize(sourceBuffer.GetNumFrames(), sourceBuffer.GetNumChannels());

		{
			NOIS_PROFILE_SCOPE_NAMED("Prepare parameters");

			mRegistry.Prepare(mHpBuffer.GetNumFrames(), mSampleRate);
		}

		{
			NOIS_PROFILE_SCOPE_NAMED("Prepare processors");

			mHpFilter->Prepare(mHpBuffer.GetNumFrames(), mHpBuffer.GetNumChannels(), mSampleRate);
			mLpFilter->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mAp1Filter->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mAp2Filter->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mDist->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mLpDistFilter->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
		}

		{
			NOIS_PROFILE_SCOPE_NAMED("Process processors");

			nois::ScopedNoDenorms noDenorms;

			mHpFilter->Process(sourceBuffer, mHpBuffer);
			mLpFilter->Process(sourceBuffer, mDistBuffer);
			mAp1Filter->Process(mDistBuffer, mDistBuffer);
			mAp2Filter->Process(mDistBuffer, mDistBuffer);
			mDist->Process(mDistBuffer, mDistBuffer);
			mLpDistFilter->Process(mDistBuffer, mDistBuffer);

			for (int c = 0; c < sinkBuffer.GetNumChannels(); ++c)
			{
				for (int f = 0; f < sinkBuffer.GetNumFrames(); ++f)
				{
					nois::f32_t wet = mWet->GetLastPlain();
					nois::f32_t x = sourceBuffer(f, c);
					nois::f32_t z = mHpBuffer(f, c) + mDistBuffer(f, c);
					sinkBuffer[f] = x * (1.0f - wet) + z * wet;
				}
			}
		}
	}


private:
	nois::FloatBuffer mHpBuffer;
	nois::FloatBuffer mDistBuffer;

	nois::FloatParameterRegistry mRegistry;

	nois::Own_t<NoisVstProcessorParameter> mSubFreq;
	nois::Own_t<NoisVstProcessorParameter> mDrive;
	nois::Own_t<NoisVstProcessorParameter> mWet;

	nois::Ref_t<nois::Filter> mHpFilter;
	nois::Ref_t<nois::Filter> mLpFilter;
	nois::Ref_t<nois::AllpassFilter> mAp1Filter;
	nois::Ref_t<nois::AllpassFilter> mAp2Filter;
	nois::Ref_t<nois::Distorter> mDist;
	nois::Ref_t<nois::Filter> mLpDistFilter;
};

using AudioEffectClass = NoisVstProcessor<VstDistort, NoisController>;
using ComponentControllerClass = NoisController;

BEGIN_FACTORY_DEF(
	"Nois",
	"http://www.nois.co.uk",
	"mailto:info@nois.co.uk",
	2)

DEF_CLASS(
	VstDistort::kUid,
	PClassInfo::kManyInstances,
	kVstAudioEffectClass,
	"NzDistort",
	Vst::kDistributable,
	Vst::PlugType::kFxDistortion,
	"1.0.0",
	kVstVersionString,
	AudioEffectClass::createInstance,
	nullptr)

DEF_CLASS(
	NoisController::kUid,
	PClassInfo::kManyInstances,
	kVstComponentControllerClass,
	"NzDistort",
	Vst::kDistributable,
	"",
	"1.0.0",
	kVstVersionString,
	ComponentControllerClass::createInstance,
	nullptr)

END_FACTORY

