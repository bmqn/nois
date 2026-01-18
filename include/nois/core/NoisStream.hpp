#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisBuffer.hpp"

namespace nois {

class StreamGraph;
class Stream;
class ProcessStream;
class SourceStream;

class StreamGraph
{
public:

};

class Stream
{
	friend class StreamGraph;

public:
	enum Result
	{
		Success,
		Starved,
		Failure
	};

	virtual void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
	}
};

class ProcessStream : public Stream
{
	friend class StreamGraph;

	virtual Result Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	= 0;
};

class SourceStream : public Stream
{
	friend class StreamGraph;

public:
	SourceStream(FloatBuffer *buffer)
		: m_Buffer(buffer)
	{
	}

	virtual Result Process(
		FloatBufferView outBuffer)
	{
		outBuffer.Copy(*m_Buffer);
		return Stream::Success;
	}

private:
	FloatBuffer *m_Buffer;
};

}
