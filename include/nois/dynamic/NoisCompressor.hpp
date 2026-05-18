#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Compressor : public Stream<f32_t>
{
public:
	NOIS_INTERFACE(Compressor)
	NOIS_INTERFACE_PARAM(Ratio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(ThresholdDb, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(AttackMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(ReleaseMs, FloatBlockParameter)
};

}
