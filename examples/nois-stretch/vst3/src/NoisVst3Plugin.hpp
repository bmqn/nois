#pragma once

#include <nois/Nois.hpp>

#include "NoisVst3Parameter.hpp"

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstparameters.h>

#include <unordered_map>

using namespace Steinberg;

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
	nois::FloatBuffer mSourceBuffer;
	nois::FloatBuffer mSinkBuffer;

	nois::Own_t<NoisVstProcessorParameter> mStretchActive;
	nois::Own_t<NoisVstProcessorParameter> mStretchFactor;
	nois::Own_t<NoisVstProcessorParameter> mGrainSize;
	nois::Own_t<NoisVstProcessorParameter> mGrainBlend;
	nois::Own_t<NoisVstProcessorParameter> mGrainPhaseLockActive;

	nois::FloatParameterRegistry mRegistry;

	nois::SourceStream mSource;
	nois::Ref_t<nois::TimeStretcher> mTimeStretcher;

	std::unordered_map<Vst::ParamID, NoisVstProcessorParameter*> mParameterLookup;

	double mSampleRate;
};

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0x9ee7ec38, 0x41254df7, 0x8863c27f, 0x3782767d)

public:
	NoisController();
	~NoisController() override = default;

	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;

private:
	nois::Own_t<NoisVstControllerParameter> mStretchActive;
	nois::Own_t<NoisVstControllerParameter> mStretchFactor;
	nois::Own_t<NoisVstControllerParameter> mGrainSize;
	nois::Own_t<NoisVstControllerParameter> mGrainBlend;
	nois::Own_t<NoisVstControllerParameter> mGrainPhaseLockActive;
};
