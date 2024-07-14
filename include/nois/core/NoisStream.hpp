#pragma once

#include "nois/NoisTypes.hpp"

namespace nois {

class Stream
{
public:
	virtual ~Stream() {}

	virtual count_t Consume(data_t *data,
							count_t numSamples,
							int32_t sampleRate,
							int32_t numChannels) = 0;
};

}
