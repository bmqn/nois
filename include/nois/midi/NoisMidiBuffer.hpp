#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {

class MidiMessage
{
public:
	MidiMessage()
		: m_Status(0)
		, m_Data1(0)
		, m_Data2(0)
		, m_FrameOffset(0)
	{
	}

	MidiMessage(uint8_t status, uint8_t data1, uint8_t data2, count_t frameOffset)
		: m_Status(status)
		, m_Data1(data1)
		, m_Data2(data2)
		, m_FrameOffset(frameOffset)
	{
	}

	bool IsNoteOn() const
	{
		bool isNoteOn = false;
		isNoteOn = isNoteOn || ((m_Status & 0xf0) == 0x90 && m_Data2 > 0);
		return isNoteOn;
	}

	bool IsNoteOff() const
	{
		bool isNoteOff = false;
		isNoteOff = isNoteOff || ((m_Status & 0xf0) == 0x80);
		isNoteOff = isNoteOff || ((m_Status & 0xf0) == 0x90 && m_Data2 == 0);
		return isNoteOff;
	}

	uint8_t GetNoteNumber() const
	{
		return m_Data1;
	}

	uint8_t GetNoteVelocity() const
	{
		return m_Data2;
	}

	f32_t GetNoteVelocityNormalized() const
	{
		return static_cast<float>(m_Data2) / 127.0f;
	}

	uint8_t GetChannel() const
	{
		return m_Status & 0x0f;
	}

	uint8_t GetType() const
	{
		return m_Status & 0xf0;
	}

	count_t GetFrameOffset() const
	{
		return m_FrameOffset;
	}

private:
	uint8_t m_Status;
	uint8_t m_Data1;
	uint8_t m_Data2;
	count_t m_FrameOffset;
};

class MidiMessageBuffer
{
	using MidiMessageBuffer_t = SmallVector<MidiMessage, 32>;

public:
	using iterator = MidiMessageBuffer_t::iterator;

public:
	MidiMessageBuffer(count_t numFrames, count_t numChannels)
		: m_NumFrames(numFrames)
		, m_NumChannels(numChannels)
	{
	}

	count_t GetNumFrames() const
	{
		return m_NumFrames;
	}

	count_t GetNumChannels() const
	{
		return m_NumChannels;
	}

	void AddMessage(MidiMessage message)
	{
		m_Messages.push_back(message);
	}

	iterator Begin()
	{
		return m_Messages.begin();
	}

	iterator End()
	{
		return m_Messages.end();
	}

private:
	count_t m_NumFrames;
	count_t m_NumChannels;

	MidiMessageBuffer_t m_Messages;
};

}
