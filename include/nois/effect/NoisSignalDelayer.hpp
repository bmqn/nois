#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class SignalDelayer : public Stream
{
public:
	virtual ~SignalDelayer() {}

	virtual Ref_t<FloatParameter> GetDelayMs() = 0;
	virtual void SetDelayMs(Ref_t<FloatParameter> delayMs) = 0;
};

Ref_t<SignalDelayer> CreateSignalDelayer(Ref_t<Stream> stream);

}
