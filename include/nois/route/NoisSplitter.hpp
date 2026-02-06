#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Splitter
{
public:
	virtual ~Splitter() {}

	virtual Stream::Result ConsumeIntoCache(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	= 0;

	virtual const FloatBuffer &GetCacheBuffer() const = 0;

	virtual void SetStream(Ref_t<Stream> stream) = 0;
	virtual Ref_t<Stream> GetStream() = 0;
};

Ref_t<Splitter> CreateSplitter();
Ref_t<Splitter> CreateSplitter(Ref_t<Stream> stream);

}