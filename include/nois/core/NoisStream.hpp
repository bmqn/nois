#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisBuffer.hpp"

namespace nois {

template<typename T>
class Stream;

template<typename T>
class Stream
{
	friend class Registry<T>;
	
public:
	enum Result
	{
		Success,
		Starved,
		Failure
	};
	
public:
	virtual ~Stream() {}

	virtual void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) = 0;
	
	virtual void Update() = 0;
	
	virtual Result Process(
		ConstBufferView<T> inBuffer,
		BufferView<T> outBuffer) = 0;
	
private:
	Registry<T>* mRegistry = nullptr;
};

}
