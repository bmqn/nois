#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class SignalDelayer : public Stream<f32_t>
{
public:
	NOIS_INTERFACE(SignalDelayer)
	NOIS_INTERFACE_PARAM(DelayMs, FloatBlockParameter)
};

}
