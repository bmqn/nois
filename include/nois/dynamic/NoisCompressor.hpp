#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Compressor : public ProcessStream
{
public:
	static Ref_t<Compressor> Create();

	NOIS_INTERFACE(Compressor)
	NOIS_INTERFACE_PARAM(Ratio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(ThresholdDb, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(AttackMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(ReleaseMs, FloatBlockParameter)
};

}
