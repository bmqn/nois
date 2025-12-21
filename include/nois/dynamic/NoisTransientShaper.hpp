#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class TransientShaper : public ProcessStream
{
public:
	static Ref_t<TransientShaper> Create();

	NOIS_INTERFACE(TransientShaper)
	NOIS_INTERFACE_PARAM(AttackRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(SustainRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(AttackMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(ReleaseMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Smoothing, FloatBlockParameter)
};

class MultibandTransientShaper : public ProcessStream
{
public:
	static Ref_t<MultibandTransientShaper> Create();

	NOIS_INTERFACE(MultibandTransientShaper)
	NOIS_INTERFACE_PARAM(AttackRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(SustainRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(AttackMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(ReleaseMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Smoothing, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(LowCutoffRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(HighCutoffRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(BandCutoffRatio, FloatBlockParameter)
};

}
