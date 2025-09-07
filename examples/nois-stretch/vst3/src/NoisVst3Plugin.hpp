#pragma once

#include <nois/Nois.hpp>

#include "NoisVst3Parameter.hpp"

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstparameters.h>

#include <variant>

using namespace Steinberg;

class NoisVstSource : public nois::Stream
{
public:
	nois::Stream::Result Consume(nois::FloatBuffer& buffer, nois::f32_t sampleRate) override;
	void PrepareToConsume(nois::count_t numFrames, nois::count_t numChannels, nois::f32_t sampleRate) override;

	nois::FloatBuffer& GetBuffer();

private:
	nois::FloatBuffer mBuffer;
};

class NoisPlugin : public Vst::AudioEffect
{
public:
	inline static DECLARE_UID(kUid, 0xe098fdac, 0xc4454fdd, 0xb14a0ef5, 0xc27122b1)

public:
	NoisPlugin();
	~NoisPlugin() override = default;

	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API process(Vst::ProcessData& data) SMTG_OVERRIDE;

private:
	nois::Ref_t<NoisVstSource> mSource;
	nois::Ref_t<nois::TimeStretcher> mTimeStretcher;

	nois::Ref_t<NoisVstSource> mAuxSource;
	nois::Ref_t<nois::FilterBank> mAuxFilterBank;

	NoisVstProcessorParameter<parameter::StretchActive> mStretchActive;
	NoisVstProcessorParameter<parameter::StretchFactor> mStretchFactor;
	NoisVstProcessorParameter<parameter::GrainPhaseInc> mGrainPhaseInc;
	NoisVstProcessorParameter<parameter::GrainPhaseLockActive> mGrainPhaseLockActive;

	using ParameterPtr = std::variant<
		NoisVstProcessorParameter<parameter::StretchActive>*,
		NoisVstProcessorParameter<parameter::StretchFactor>*,
		NoisVstProcessorParameter<parameter::GrainPhaseInc>*,
		NoisVstProcessorParameter<parameter::GrainPhaseLockActive>*
	>;

	std::unordered_map<Vst::ParamID, ParameterPtr> mParameterLookup;

	double mSampleRate;
};

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0x9ee7ec38,-0x41254df7, 0x8863c27f, 0x3782767d)

public:
	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;
};
