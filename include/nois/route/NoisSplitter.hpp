#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Splitter
{
public:
	virtual std::shared_ptr<Channel> GetChannel() = 0;
};

std::shared_ptr<Splitter> CreateSplitter(std::shared_ptr<Channel> channel, size_t numChannels);

}