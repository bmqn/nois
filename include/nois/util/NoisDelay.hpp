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
		m_NumFrames = numFrames;
		for (auto& offset : m_Offsets)
		{
			offset = 0;
		}
		for (auto& buffer : m_Buffers)
		{
			buffer.resize(numFrames, T{});
			buffer.shrink_to_fit();
		}
	}

	inline T Process(T x, count_t c)
	{
		if (m_NumFrames == 0)
		{
			return x;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];

		count_t indexRead = (offset + 1) % m_NumFrames;
		count_t indexWrite = offset % m_NumFrames;
		T y = buffer[indexRead];
		buffer[indexWrite] = x;
	
		++offset;

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
	count_t m_NumFrames = 0;
	std::array<count_t, C> m_Offsets = { 0 };
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
		m_NumFrames = numFrames;
		for (auto& offset : m_Offsets)
		{
			offset = 0;
		}
		for (auto& feedback : m_Feedbacks)
		{
			feedback = m_FeedbackTarget;
		}
		for (auto& buffer : m_Buffers)
		{
			buffer.resize(numFrames, T{});
			buffer.shrink_to_fit();
		}
	}

	inline void SetFeedbackTarget(f32_t feedbackTarget)
	{
		m_FeedbackTarget = feedbackTarget;
	}

	inline T Process(T x, count_t c)
	{
		if (m_NumFrames == 0)
		{
			return x;
		}

		auto& offset = m_Offsets[c];
		auto& feedback = m_Feedbacks[c];
		auto& buffer = m_Buffers[c];

		count_t indexRead = (offset + 1) % m_NumFrames;
		count_t indexWrite = offset % m_NumFrames;
		T y = buffer[indexRead];
		buffer[indexWrite] = x + feedback * y;
	
		++offset;
		feedback += (m_FeedbackTarget - feedback) * T{ 0.1 };
	
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
	count_t m_NumFrames = 0;
	T m_FeedbackTarget = T{ 0 };
	std::array<count_t, C> m_Offsets = { 0 };
	std::array<T, C> m_Feedbacks = { T{ 0 } };
	std::array<SmallVector<T, F>, C> m_Buffers;
};

}