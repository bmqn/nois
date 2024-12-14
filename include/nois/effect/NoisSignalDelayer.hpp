#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class SignalDelayer : public Stream
{
public:
	virtual ~SignalDelayer() {}

	virtual data_t GetDelayMs() = 0;
	virtual void SetDelayMs(data_t delayMs) = 0;
};

Ref_t<SignalDelayer> CreateSignalDelayer(Stream *stream);

}
