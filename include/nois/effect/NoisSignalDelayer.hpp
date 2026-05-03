#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class SignalDelayer : public ProcessStream
{
public:
	static Ref_t<SignalDelayer> Create();

public:
	NOIS_INTERFACE(SignalDelayer)
	NOIS_INTERFACE_PARAM(DelayMs, FloatBlockParameter)
};

}
