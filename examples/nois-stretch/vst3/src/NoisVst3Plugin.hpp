#pragma once

#include <nois/Nois.hpp>

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstparameters.h>

using namespace Steinberg;

enum
{
	kParameterTagStretchActive,
	kParameterTagStretchFactor,
	kParameterTagGrainPhaseInc,
	kParameterTagGrainLockActive
};

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
	nois::Ref_t<nois::FloatParameter> mStretchActive;
	nois::Ref_t<nois::FloatParameter> mStretchFactor;
	nois::Ref_t<nois::FloatParameter> mGrainGain;
	nois::Ref_t<nois::FloatParameter> mGrainPhaseInc;
	nois::Ref_t<nois::FloatParameter> mGrainLockActive;
	std::vector<nois::f32_t> mStretchActiveValues;
	std::vector<nois::f32_t> mStretchFactorValues;
	std::vector<nois::f32_t> mGrainGainValues;
	std::vector<nois::f32_t> mGrainPhaseIncValues;
	std::vector<nois::f32_t> mGrainLockActiveValues;

	nois::Ref_t<NoisVstSource> mAuxSource;
	nois::Ref_t<nois::FilterBank> mAuxFilterBank;

	double mSampleRate;
};

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0x9ee7ec38,-0x41254df7, 0x8863c27f, 0x3782767d);

public:
	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;
};
