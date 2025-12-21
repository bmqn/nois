#pragma once

#include <nois/Nois.hpp>

#include <public.sdk/source/vst/vstparameters.h>
#include <pluginterfaces/vst/vsttypes.h>

using namespace Steinberg;

class NoisVstProcessorParameter
{
public:
	virtual Vst::ParamID GetPid() const = 0;

	virtual Vst::ParamValue ApplyStep(Vst::ParamValue valuePlain) const = 0;
	virtual Vst::ParamValue ToPlain(Vst::ParamValue valueNormalized) const = 0;
	virtual Vst::ParamValue ToNormalized(Vst::ParamValue valuePlain) const = 0;

	virtual void Prepare(nois::count_t numFrames, nois::f32_t sampleRate) = 0;

	virtual void WritePlain(nois::count_t frame, nois::f32_t value) = 0;
	virtual void WritePlain(nois::count_t frame, nois::count_t count, nois::f32_t value) = 0;
	virtual nois::f32_t GetLastPlain() const = 0;

	virtual operator nois::Ref_t<nois::FloatParameter>() = 0;
	virtual operator nois::Ref_t<nois::FloatBlockParameter>() = 0;
};

class NoisVstControllerParameter
{
public:
	virtual Vst::ParamID GetPid() const = 0;

	virtual operator Vst::Parameter*() = 0;
};

namespace parameter
{

enum
{
	kSubFreq,
	kDrive,
	kWet
};

nois::Own_t<NoisVstProcessorParameter> CreateProcessor(Vst::ParamID pid, nois::FloatParameterRegistry& registry);
nois::Own_t<NoisVstControllerParameter> CreateController(Vst::ParamID pid);

}
