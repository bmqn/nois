#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Gainer : public ProcessStream
{
public:
	static Ref_t<Gainer> Create();

	NOIS_INTERFACE(Gainer)
	NOIS_INTERFACE_PARAM(GainDb, FloatParameter)
};

}
