#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class SignalDelayer : public Stream
{
public:
	virtual ~SignalDelayer() {}

	virtual data_t GetDelayMs() = 0;
	virtual void SetDelayMs(data_t delayMs) = 0;
};

std::shared_ptr<SignalDelayer> CreateSignalDelayer(Stream *stream);

}
