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
	virtual void Process(nois::ConstFloatBufferView sourceBuffer, nois::FloatBufferView sinkBuffer) = 0;

	template<typename Param>
	nois::Own_t<NoisVstProcessorParameter> CreateParameter();

	nois::Ref_t<nois::FloatParameter> CreateConstant(nois::f32_t value);
	nois::Ref_t<nois::FloatBlockParameter> CreateBlockConstant(nois::f32_t value);

protected:
	nois::f64_t mSampleRate;

private:
	nois::FloatBuffer mSourceBuffer;
	nois::FloatBuffer mSinkBuffer;

	nois::FloatParameterRegistry mRegistry;
	std::unordered_map<Vst::ParamID, NoisVstProcessorParameter*> mParameters;
};

#include "NoisVst3Processor.inl"
