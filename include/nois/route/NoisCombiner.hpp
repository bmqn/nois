#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

namespace nois {

class Combiner
{
public:
	virtual ~Combiner() {}

	virtual Stream::Result ConsumeIntoCache(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	= 0;

	virtual void AddStream(Ref_t<Stream> stream) = 0;
	virtual void RemoveStream(Ref_t<Stream> stream) = 0;
	virtual Ref_t<Stream> GetStream() = 0;
};

Ref_t<Combiner> CreateCombiner();

}