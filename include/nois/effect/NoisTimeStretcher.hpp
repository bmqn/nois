#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class TimeStretcher : public ProcessStream
{
public:
	static Ref_t<TimeStretcher> Create();

	NOIS_INTERFACE(TimeStretcher)
	NOIS_INTERFACE_PARAM(StretchActive, FloatParameter)
	NOIS_INTERFACE_PARAM(StretchTimeMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(StretchFactor, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainSize, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainBlend, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainPhaseInc, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainLockActive, FloatParameter)
};

}
