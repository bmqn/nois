#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/midi/NoisMidiBuffer.hpp"

namespace nois {

class MidiStream
{
public:
	~MidiStream()
	{
	}

	virtual void Process(
		MidiMessageBuffer &messageBuffer,
		f32_t sampleRate)
	= 0;

	virtual void PrepareToProcess(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
	}
};

}
