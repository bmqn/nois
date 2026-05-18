#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Reverb : public Stream<f32_t>
{
public:
	NOIS_INTERFACE(Reverb)
	NOIS_INTERFACE_PARAM(Wet, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(DecayMs, FloatBlockParameter)
};

}
