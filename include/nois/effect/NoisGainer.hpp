#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Gainer : public Stream<f32_t>
{
public:
	NOIS_INTERFACE(Gainer)
	NOIS_INTERFACE_PARAM(Gain, FloatParameter)
};

}
