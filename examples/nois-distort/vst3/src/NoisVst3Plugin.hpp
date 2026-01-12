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
	inline static DECLARE_UID(kUid, 0x51632f93, 0x63414709, 0x8f2949e3, 0x49fe1815)

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
	nois::FloatBuffer mHpBuffer;
	nois::FloatBuffer mDistBuffer;

	nois::Own_t<NoisVstProcessorParameter> mSubFreq;
	nois::Own_t<NoisVstProcessorParameter> mDrive;
	nois::Own_t<NoisVstProcessorParameter> mWet;

	nois::FloatParameterRegistry mRegistry;

	nois::SourceStream mSource;
	nois::Ref_t<nois::Filter> mHpFilter;
	nois::Ref_t<nois::Filter> mLpFilter;
	nois::Ref_t<nois::AllpassFilter> mAp1Filter;
	nois::Ref_t<nois::AllpassFilter> mAp2Filter;
	nois::Ref_t<nois::Distorter> mDist;
	nois::Ref_t<nois::Filter> mLpDistFilter;

	std::unordered_map<Vst::ParamID, NoisVstProcessorParameter*> mParameterLookup;

	double mSampleRate;
};

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0xa4a2f37a, 0xafa543af, 0x803e37c8, 0x35785df8)

public:
	NoisController();
	~NoisController() override = default;

	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;

private:
	nois::Own_t<NoisVstControllerParameter> mSubFreq;
	nois::Own_t<NoisVstControllerParameter> mDrive;
	nois::Own_t<NoisVstControllerParameter> mWet;
};
