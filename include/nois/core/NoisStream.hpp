#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisBuffer.hpp"

namespace nois {

// TODO: create a chained stream type here

class Stream
{
public:
	enum Result
	{
		Success,
		Starved,
		Failure
	};

public:
	virtual ~Stream()
	{
	}

	virtual Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate)
	= 0;

	virtual void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
	}
};

}
