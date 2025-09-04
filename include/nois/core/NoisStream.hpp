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

// TODO: implement sample-based streams for effects like filters

class SampleStream
{
public:
	enum Result
	{
		Success,
		Starved,
		Failure
	};

public:
	virtual ~SampleStream()
	{
	}

	virtual Result Consume(
		f32_t sample,
		f32_t sampleRate)
	= 0;

	virtual void PrepareToConsume(
		count_t numChannels,
		f32_t sampleRate)
	{
	}
};

template<typename T>
class BufferStream : public Stream
{
public:
	BufferStream(Buffer<T> *buffer)
		: m_Buffer(buffer)
	{
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		buffer.Copy(*m_Buffer);
		return Stream::Result::Success;
	}

private:
	Buffer<T> *m_Buffer;
};

using FloatBufferStream = BufferStream<f32_t>;

}
