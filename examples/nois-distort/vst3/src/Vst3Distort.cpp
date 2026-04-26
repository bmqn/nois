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
	kMakeup,
	kShape,
	kGlue,
	kWet,
};


struct SubFreq
{
	static constexpr const char* kTitle = "Cutoff";
	static constexpr const char* kUnits = "Hz";
	static constexpr Vst::ParamID kPid = kSubFreq;
	static constexpr nois::f32_t kDefaultValue = 80.0f;
	static constexpr nois::f32_t kMinValue = 50.0f;
	static constexpr nois::f32_t kMaxValue = 180.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct DriveDb
{
	static constexpr const char* kTitle = "Drive";
	static constexpr const char* kUnits = "dB";
	static constexpr Vst::ParamID kPid = kDrive;
	static constexpr nois::f32_t kDefaultValue = 0.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 16.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct MakeupDb
{
	static constexpr const char* kTitle = "Makeup";
	static constexpr const char* kUnits = "dB";
	static constexpr Vst::ParamID kPid = kMakeup;
	static constexpr nois::f32_t kDefaultValue = 0.0f;
	static constexpr nois::f32_t kMinValue = -12.0f;
	static constexpr nois::f32_t kMaxValue = 0.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct Shape
{
	static constexpr const char* kTitle = "Shape";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kShape;
	static constexpr nois::f32_t kDefaultValue = 0.6f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct Glue
{
	static constexpr const char* kTitle = "Glue";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kGlue;
	static constexpr nois::f32_t kDefaultValue = 0.5f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 0;
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
};

}

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0xa4a2f37a, 0xafa543af, 0x803e37c8, 0x35785df8)

public:
	NoisController()
		: mSubFreq(nullptr)
		, mDriveDb(nullptr)
		, mMakeupDb(nullptr)
		, mShape(nullptr)
		, mWet(nullptr)
	{
		mSubFreq = NoisVstControllerParameter::Create<parameter::SubFreq>();
		mDriveDb = NoisVstControllerParameter::Create<parameter::DriveDb>();
		mMakeupDb = NoisVstControllerParameter::Create<parameter::MakeupDb>();
		mShape = NoisVstControllerParameter::Create<parameter::Shape>();
		mGlue = NoisVstControllerParameter::Create<parameter::Glue>();
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
		parameters.addParameter(*mDriveDb);
		parameters.addParameter(*mMakeupDb);
		parameters.addParameter(*mShape);
		parameters.addParameter(*mGlue);
		parameters.addParameter(*mWet);

		return result;
	}

	tresult PLUGIN_API terminate() SMTG_OVERRIDE
	{
		return EditController::terminate();
	}

	tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE
	{
		return kResultOk;
	}

private:
	nois::Own_t<NoisVstControllerParameter> mSubFreq;
	nois::Own_t<NoisVstControllerParameter> mDriveDb;
	nois::Own_t<NoisVstControllerParameter> mMakeupDb;
	nois::Own_t<NoisVstControllerParameter> mShape;
	nois::Own_t<NoisVstControllerParameter> mGlue;
	nois::Own_t<NoisVstControllerParameter> mWet;
};

class VstDistort : public NoisVstProcessor<VstDistort, NoisController>
{
public:
	inline static DECLARE_UID(kUid, 0x51632f93, 0x63414709, 0x8f2949e3, 0x49fe1815)

public:
	VstDistort()
		: mHpBuffer()
		, mLpBuffer()
		, mDistBuffer()
		, mSubFreq(nullptr)
		, mDriveDb(nullptr)
		, mMakeupDb(nullptr)
		, mShape(nullptr)
		, mGlue(nullptr)
		, mWet(nullptr)
		, mHpFilter(nullptr)
		, mLpFilter(nullptr)
		, mAp1Filter(nullptr)
		, mAp2Filter(nullptr)
		, mDist(nullptr)
		, mLpDistFilter(nullptr)
		, mCompressor(nullptr)
	{
		mSubFreq = CreateParameter<parameter::SubFreq>();
		mDriveDb = CreateParameter<parameter::DriveDb>();
		mMakeupDb = CreateParameter<parameter::MakeupDb>();
		mShape = CreateParameter<parameter::Shape>();
		mGlue = CreateParameter<parameter::Glue>();
		mWet = CreateParameter<parameter::Wet>();

		auto subFreqRatio = 
			mSubFreq->TransformBlock(
				[](float x, float sampleRate) -> float
				{
					return x / (sampleRate * 0.5f);
				});
		auto ap1CutoffRatio =
			mSubFreq->TransformBlock(
				[](float x, float sampleRate) -> float
				{
					return std::clamp(x * 0.7f, 0.002f, 0.02f) / (sampleRate * 0.5f);
				});
		auto ap2CutoffRatio =
			mSubFreq->TransformBlock(
				[](float x, float sampleRate) -> float
				{
					return std::clamp(x * 1.2f, 0.002f, 0.03f) / (sampleRate * 0.5f);
				});
		auto apQ =
			CreateBlockConstant(0.67f);
		auto distAsym =
			mShape->TransformBlock(
				[](float x) -> float
				{
					return x > 0.5f ? 1.0f + (x - 0.5) : 1.0f;
				});
		auto lpDistCutoffRatio =
			mSubFreq->TransformBlock(
				[](float x) -> float
				{
					return x * 2.0f;
				});

		auto compRatio =
			mGlue->TransformBlock(
				[](float x) -> float
				{
					return 1.5f + x * 4.0f;
				});
		auto compThresholdDb =
			mGlue->TransformBlock(
				[](float x) -> float
				{
					return -40.0f + x * 30.0f;
				});
		auto compAttackMs =
			mGlue->TransformBlock(
				[](float x) -> float
				{
					return 30.0f - x * 20.0f;
				});
		auto compReleaseMs =
			mGlue->TransformBlock(
				[](float x) -> float
				{
					return 150.0f - x * 80.0f;
				});

		mHpFilter = nois::Filter::Create(nois::Filter::k_LR4High);
		mHpFilter->SetCutoffRatio(subFreqRatio);

		mLpFilter = nois::Filter::Create(nois::Filter::k_LR4Low);
		mLpFilter->SetCutoffRatio(subFreqRatio);

		mAp1Filter = nois::AllpassFilter::Create(nois::AllpassFilter::k_RBJBiquad);
		mAp1Filter->SetCutoffRatio(ap1CutoffRatio);
		mAp1Filter->SetQ(apQ);

		mAp2Filter = nois::AllpassFilter::Create(nois::AllpassFilter::k_RBJBiquad);
		mAp2Filter->SetCutoffRatio(ap2CutoffRatio);
		mAp2Filter->SetQ(apQ);

		mDist = nois::DynamicTanhDistorter::Create();
		mDist->SetDriveDb(*mDriveDb);
		mDist->SetMakeupDb(*mMakeupDb);
		mDist->SetShape(*mShape);
		mDist->SetAsym(distAsym);

		mLpDistFilter = nois::Filter::Create(nois::Filter::k_LR4Low);
		mLpDistFilter->SetCutoffRatio(lpDistCutoffRatio);

		mCompressor = nois::Compressor::Create();
		mCompressor->SetRatio(compRatio);
		mCompressor->SetThresholdDb(compThresholdDb);
		mCompressor->SetAttackMs(compAttackMs);
		mCompressor->SetReleaseMs(compReleaseMs);
	}

protected:
	void Process(
		nois::ConstFloatBufferView sourceBuffer,
		nois::FloatBufferView sinkBuffer) override
	{
		mHpBuffer.Resize(sourceBuffer.GetNumFrames(), sourceBuffer.GetNumChannels());
		mLpBuffer.Resize(sourceBuffer.GetNumFrames(), sourceBuffer.GetNumChannels());
		mDistBuffer.Resize(sourceBuffer.GetNumFrames(), sourceBuffer.GetNumChannels());

		{
			NOIS_PROFILE_SCOPE_NAMED("Prepare processors");

			mHpFilter->Prepare(mHpBuffer.GetNumFrames(), mHpBuffer.GetNumChannels(), mSampleRate);
			mLpFilter->Prepare(mLpBuffer.GetNumFrames(), mLpBuffer.GetNumChannels(), mSampleRate);
			mAp1Filter->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mAp2Filter->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mDist->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mLpDistFilter->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
			mCompressor->Prepare(mDistBuffer.GetNumFrames(), mDistBuffer.GetNumChannels(), mSampleRate);
		}

		{
			NOIS_PROFILE_SCOPE_NAMED("Process processors");

			nois::ScopedNoDenorms noDenorms;

			mHpFilter->Process(sourceBuffer, mHpBuffer);
			mLpFilter->Process(sourceBuffer, mLpBuffer);
			mAp1Filter->Process(mLpBuffer, mDistBuffer);
			mAp2Filter->Process(mDistBuffer, mDistBuffer);
			mDist->Process(mDistBuffer, mDistBuffer);
			mLpDistFilter->Process(mDistBuffer, mDistBuffer);
			mCompressor->Process(mDistBuffer, mDistBuffer);

			nois::f32_t wet = mWet->GetLastPlain();
			for (int c = 0; c < sinkBuffer.GetNumChannels(); ++c)
			{
				for (int f = 0; f < sinkBuffer.GetNumFrames(); ++f)
				{
					nois::f32_t high = mHpBuffer(f, c);
					nois::f32_t lowClean = mLpBuffer(f, c);
					nois::f32_t lowDist  = mDistBuffer(f, c);
					sinkBuffer(f, c) = high + lowClean * (1.0f - wet) + lowDist * wet;
				}
			}
		}
	}

private:
	nois::FloatBuffer mHpBuffer;
	nois::FloatBuffer mLpBuffer;
	nois::FloatBuffer mDistBuffer;

	nois::Own_t<NoisVstProcessorParameter> mSubFreq;
	nois::Own_t<NoisVstProcessorParameter> mDriveDb;
	nois::Own_t<NoisVstProcessorParameter> mMakeupDb;
	nois::Own_t<NoisVstProcessorParameter> mShape;
	nois::Own_t<NoisVstProcessorParameter> mGlue;
	nois::Own_t<NoisVstProcessorParameter> mWet;

	nois::Ref_t<nois::Filter> mHpFilter;
	nois::Ref_t<nois::Filter> mLpFilter;
	nois::Ref_t<nois::AllpassFilter> mAp1Filter;
	nois::Ref_t<nois::AllpassFilter> mAp2Filter;
	nois::Ref_t<nois::DynamicTanhDistorter> mDist;
	nois::Ref_t<nois::Filter> mLpDistFilter;
	nois::Ref_t<nois::Compressor> mCompressor;
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

