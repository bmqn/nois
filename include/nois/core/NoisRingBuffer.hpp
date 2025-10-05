#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisBuffer.hpp"

namespace nois {

template<typename>
class InterleavedRingBuffer;

using FloatInterleavedRingBuffer = InterleavedRingBuffer<f32_t>;

template<typename T>
class InterleavedRingBuffer
{
public:
	InterleavedRingBuffer()
		: m_LogicalNumFrames(0)
		, m_InternalNumFrames(0)
		, m_FrameModuloMask(0)
		, m_FrameOffset(0)
		, m_FrameCount(0)
		, m_NumChannels(0)
		, m_Data(0, T{})
	{
	}

	inline count_t GetNumFrames() const
	{
		return m_LogicalNumFrames;
	}

	inline count_t GetNumChannels() const
	{
		return m_NumChannels;
	}

	inline void Resize(count_t numFrames, count_t numChannels)
	{
		if (numFrames == m_LogicalNumFrames)
		{
			return;
		}

		ucount_t newLogicalNumFrames = numFrames;
		ucount_t newInternalNumFrames = NextPowerOfTwo(numFrames);
		ucount_t newFrameCount = std::min(m_FrameCount, newInternalNumFrames);

		count_t newNumChannels = numChannels;

		std::vector<T> newData(newNumChannels * newInternalNumFrames, T{});

		for (ucount_t f = 0; f < newFrameCount; ++f)
		{
			// Copy in chronological order
			GetChronological(f, &newData[f * newNumChannels], newNumChannels);
		}

		// Replace
		m_LogicalNumFrames = newLogicalNumFrames;
		m_InternalNumFrames = newInternalNumFrames;
		m_FrameModuloMask = newInternalNumFrames - 1;
		m_FrameOffset = newFrameCount & m_FrameModuloMask;
		m_FrameCount = newFrameCount;
		m_NumChannels = newNumChannels;
		m_Data.swap(newData);
	}

	inline void Reset()
	{
		Zero();
		m_FrameOffset = 0;
		m_FrameCount = 0;
	}

	inline void Fill(T value)
	{
		std::fill(m_Data.begin(), m_Data.end(), value);
	}

	inline void Zero()
	{
		Fill(T{});
	}

	inline void Add(const T *samples, count_t numChannels)
	{
		ucount_t baseOffset = m_FrameOffset * m_NumChannels;

		for (size_t c = 0; c < std::min(m_NumChannels, numChannels); ++c)
		{
			m_Data[baseOffset + c] = samples[c];
		}

		m_FrameOffset = (m_FrameOffset + 1) & m_FrameModuloMask;

		if (m_FrameCount < m_InternalNumFrames)
		{
			++m_FrameCount;
		}
	}

	inline void GetChronological(count_t frameIndex, T *samples, count_t numChannels) const
	{
		ucount_t baseOffset = ((
			m_FrameOffset -
			m_FrameCount +
			frameIndex +
			m_InternalNumFrames) &
			m_FrameModuloMask) *
			m_NumChannels;

		for (count_t c = 0; c < std::min(m_NumChannels, numChannels); ++c)
		{
			samples[c] = m_Data[baseOffset + c];
		}
	}

	inline void GetNewest(T *samples, count_t numChannels) const
	{
		GetChronological(m_LogicalNumFrames, samples, numChannels);
	}

	inline void GetOldest(T *samples, count_t numChannels) const
	{
		if (m_FrameCount < m_LogicalNumFrames)
		{
			std::fill(samples, samples + numChannels, T{});
		}
		else
		{
			GetChronological(0, samples, numChannels);
		}
	}

	inline bool IsFull() const
	{
		return m_FrameCount == m_LogicalNumFrames;
	}

private:
	ucount_t NextPowerOfTwo(ucount_t n)
	{
		ucount_t power = 1;

		while (power < n)
		{
			power <<= 1;
		}
		
		return power;
	}

private:
	ucount_t m_LogicalNumFrames;
	ucount_t m_InternalNumFrames;
	ucount_t m_FrameModuloMask;
	ucount_t m_FrameOffset;
	ucount_t m_FrameCount;

	count_t m_NumChannels;

	std::vector<T> m_Data;
};

}