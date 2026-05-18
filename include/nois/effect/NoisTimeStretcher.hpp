#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/NoisConfig.hpp"
#include "nois/NoisMacros.hpp"
#include "nois/NoisUtil.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class TimeStretcher : public Stream<f32_t>
{
public:
	NOIS_INTERFACE(TimeStretcher)
	NOIS_INTERFACE_PARAM(StretchTimeMs, FloatParameter)
	NOIS_INTERFACE_PARAM(StretchActive, FloatParameter)
	NOIS_INTERFACE_PARAM(StretchFactor, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainSize, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainBlend, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainPhaseInc, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainLockActive, FloatParameter)
};

}
