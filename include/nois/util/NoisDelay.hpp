#pragma once

#include "nois/NoisConfig.hpp"
#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {

template<typename T, count_t C = k_MaxChannels, count_t F = k_CacheOptimisedNumFrames>
struct Delay
{
	Delay(count_t numFrames = 0)
	{
		Reset(numFrames);
	}

	inline void Reset(count_t numFrames)
	{
		m_Offset = 0;
		m_NumFrames = numFrames;
		for (auto& buffer : m_Buffers)
		{
			buffer.resize(numFrames, T{});
			buffer.shrink_to_fit();
		}
	}

	inline T Process(T x, count_t c)
	{
		auto& buffer = m_Buffers[c];
		count_t offsetWrapped = m_Offset % m_NumFrames;
		T y = buffer[offsetWrapped];
		buffer[offsetWrapped] = x;
		++m_Offset;
		return y;
	}

	inline void Process(const T* inData, T* outData, count_t numFrames, count_t c)
	{
		for (count_t f = 0; f < numFrames; ++f)
		{
			outData[f] = Process(inData[f], c);
		}
	}

	inline count_t GetNumFrames() const
	{
		return m_NumFrames;
	}

private:
	count_t m_Offset = 0;
	count_t m_NumFrames = 0;
	std::array<SmallVector<T, F>, C> m_Buffers;
};

template<typename T, count_t C = k_MaxChannels, count_t F = k_CacheOptimisedNumFrames>
struct DelayFeedback
{
	DelayFeedback(count_t numFrames = 0)
	{
		Reset(numFrames);
	}

	inline void Reset(count_t numFrames)
	{
		m_Offset = 0;
		m_NumFrames = numFrames;
		for (auto& buffer : m_Buffers)
		{
			buffer.resize(numFrames, T{});
			buffer.shrink_to_fit();
		}
	}

	inline T Process(T x, count_t c)
	{
		auto& buffer = m_Buffers[c];
		count_t offsetWrapped = m_Offset % m_NumFrames;
		T y = buffer[offsetWrapped];
		buffer[offsetWrapped] = x + std::clamp(feedbackGain, T{ 0 }, T{ 1 }) * y;
		++m_Offset;
		return y;
	}

	inline void Process(const T* inData, T* outData, count_t numFrames, count_t c)
	{
		for (count_t f = 0; f < numFrames; ++f)
		{
			outData[f] = Process(inData[f], c);
		}
	}

	inline count_t GetNumFrames() const
	{
		return m_NumFrames;
	}

	T feedbackGain = T{ 0 };

private:
	count_t m_Offset = 0;
	count_t m_NumFrames = 0;
	std::array<SmallVector<T, F>, C> m_Buffers;
};

}