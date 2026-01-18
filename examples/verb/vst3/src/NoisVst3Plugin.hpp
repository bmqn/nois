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
	inline static DECLARE_UID(kUid, 0x2ae41d67, 0x8b034025, 0x9b11c63f, 0xd5b5a738)

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

	nois::FloatParameterRegistry mRegistry;
	nois::Own_t<NoisVstProcessorParameter> mWet;
	nois::Own_t<NoisVstProcessorParameter> mDecayMs;

	nois::Ref_t<nois::Reverb> mReverb;

	std::unordered_map<Vst::ParamID, NoisVstProcessorParameter*> mParameterLookup;

	double mSampleRate;
};

class NoisController : public Vst::EditController
{
public:
	inline static DECLARE_UID(kUid, 0x8fa5b042, 0xda9a4944, 0x835130d0, 0xf0c6c27a)

public:
	NoisController();
	~NoisController() override = default;

	static FUnknown* PLUGIN_API createInstance(void*);

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;

private:
	nois::Own_t<NoisVstControllerParameter> mWet;
	nois::Own_t<NoisVstControllerParameter> mDecayMs;
};
