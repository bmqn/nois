#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"

#include <memory>

namespace nois {

class Destination
{
public:
	virtual int32_t GetSampleRate() = 0;
	virtual int32_t GetNumChannels() = 0;

	virtual void SetChannel(std::shared_ptr<Channel>) = 0;
};

std::shared_ptr<Destination> CreateDestination();

}
