#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Mixer : public Stream
{
public:
	virtual void AddChannel(std::shared_ptr<Channel> channel) = 0;
	virtual void RemoveChannel(std::shared_ptr<Channel> channel) = 0;

	virtual data_t GetGain(std::shared_ptr<Channel> channel) = 0;
	virtual void SetGain(std::shared_ptr<Channel> channel, data_t gain) = 0;

	virtual data_t GetGainDb(std::shared_ptr<Channel> channel) = 0;
	virtual void SetGainDb(std::shared_ptr<Channel> channel, data_t gainDb) = 0;
};

std::shared_ptr<Mixer> CreateMixer(std::initializer_list<std::shared_ptr<Channel>> channels = {});

}
