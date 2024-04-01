#pragma once

#include "NoisChannel.hpp"

#include <cinttypes>
#include <memory>

namespace nois {

class Source
{
public:
	virtual int32_t GetSampleRate() = 0;

	virtual std::shared_ptr<Channel> GetChannel() = 0;
};

std::shared_ptr<Source> CreateSource();

};
