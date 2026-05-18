#pragma once

#include <nois/Nois.hpp>

#include "NoisVst3Parameter.hpp"

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstparameters.h>

#include <unordered_map>

using namespace Steinberg;

template<typename T, typename C>
class NoisVstProcessor : public Vst::AudioEffect
{
public:
	NoisVstProcessor();
	virtual ~NoisVstProcessor() {}

	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API process(Vst::ProcessData& data) SMTG_OVERRIDE;

protected:
	template<typename Param>
	nois::Own_t<NoisVstProcessorParameter> CreateParameter();
	
	template<typename Strm>
	nois::Ref_t<Strm> CreateStream();

	template<typename F, typename... Params>
	nois::Ref_t<nois::FloatParameter> CreateTransformer(F&& transformer, Params&&... transformees);

	nois::Ref_t<nois::FloatParameter> GetTempo();

protected:
	nois::f32_t mSampleRate;
	nois::f32_t mTempo;

private:
	nois::FloatBuffer mSourceBuffer;
	nois::FloatBuffer mSinkBuffer;

	nois::FloatRegistry mRegistry;
	std::unordered_map<Vst::ParamID, NoisVstProcessorParameter*> mParameters;

	nois::Ref_t<nois::FloatParameter> mTempoParameter;
	nois::Ref_t<nois::FloatBlockParameter> mTempoBlockParameter;
};

template<typename T>
class NoisVstController : public Vst::EditController
{
public:
	NoisVstController();
	virtual ~NoisVstController() {}

	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;

protected:
	template<typename Param>
	nois::Own_t<NoisVstControllerParameter> CreateParameter();

private:
	std::unordered_map<Vst::ParamID, NoisVstControllerParameter*> mParameters;
};

#include "NoisVst3Processor.inl"
