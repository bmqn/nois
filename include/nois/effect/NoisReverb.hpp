#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Reverb : public ProcessStream
{
public:
	static Ref_t<Reverb> Create();

	NOIS_INTERFACE(Reverb)
	NOIS_INTERFACE_PARAM(Wet, FloatBlockParameter)
};

}
