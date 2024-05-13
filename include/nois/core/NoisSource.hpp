#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"

#include <memory>

namespace nois {

class Source
{
public:
	virtual int32_t GetSampleRate() = 0;

	virtual std::shared_ptr<Channel> GetChannel() = 0;
};

class SidechainSource
{
public:
	virtual int32_t GetSampleRate() = 0;

	virtual std::shared_ptr<Channel> GetChannel() = 0;
};

std::shared_ptr<Source> CreateSource();
std::shared_ptr<Source> CreateSidechainSource();

};
