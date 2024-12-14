#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisChannel.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Mixer : public Stream
{
public:
	virtual void AddChannel(Ref_t<Channel> channel) = 0;
	virtual void RemoveChannel(Ref_t<Channel> channel) = 0;

	virtual data_t GetGain(Ref_t<Channel> channel) = 0;
	virtual void SetGain(Ref_t<Channel> channel, data_t gain) = 0;

	virtual data_t GetGainDb(Ref_t<Channel> channel) = 0;
	virtual void SetGainDb(Ref_t<Channel> channel, data_t gainDb) = 0;
};

Ref_t<Mixer> CreateMixer(std::initializer_list<Ref_t<Channel>> channels = {});

}
