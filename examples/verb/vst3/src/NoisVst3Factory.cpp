#include "NoisVst3Plugin.hpp"

#include <public.sdk/source/main/pluginfactory_constexpr.h>

using namespace Steinberg;

BEGIN_FACTORY_DEF(
	"Nois",
	"http://www.nois.co.uk",
	"mailto:info@nois.co.uk",
	2)

DEF_CLASS(
	NoisPlugin::kUid,
	PClassInfo::kManyInstances,
	kVstAudioEffectClass,
	"NzVerb",
	Vst::kDistributable,
	Vst::PlugType::kFxReverb,
	"1.0.0",
	kVstVersionString,
	NoisPlugin::createInstance,
	nullptr)

DEF_CLASS(
	NoisController::kUid,
	PClassInfo::kManyInstances,
	kVstComponentControllerClass,
	"NzVerb",
	0,
	"",
	"1.0.0",
	kVstVersionString,
	NoisController::createInstance,
	nullptr)

END_FACTORY
