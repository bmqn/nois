#pragma once

#include "nois/NoisConfig.hpp"
#include "nois/NoisMacros.hpp"
#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {

template<typename T, count_t C = 1, count_t F = k_MaxNumInplaceFrames>
struct Delay
{
	Delay(count_t numFrames = 0)
	{
		Reset(numFrames);
	}

	inline void Reset(count_t numFrames)
	{
		ucount_t realNumFrames = NextPowerOfTwo(numFrames);
		m_NumFrames = realNumFrames;
		m_ModuloMask = realNumFrames - 1;
		for (auto& offset : m_Offsets)
		{
			offset = 0;
		}
		for (auto& buffer : m_Buffers)
		{
			buffer.resize(realNumFrames, T{});
			buffer.shrink_to_fit();
		}
	}

	inline T Process(T x, T m = T{ 0 }, count_t c = 0)
	{
		if (m_NumFrames == 0)
		{
			return x;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];

		// Grab write/read indices
		ucount_t indexWrite = offset & m_ModuloMask;
		T indexReadFrac = m_NumDelayFrames + m;
		indexReadFrac = (indexWrite - static_cast<ucount_t>(indexReadFrac)) & m_ModuloMask;
		ucount_t indexReadFrac0 = indexReadFrac;
		ucount_t indexReadFrac1 = (indexReadFrac0 + 1) & m_ModuloMask;

		// Interpolate read & write
		T factor = indexReadFrac - T{ indexReadFrac0 };
		T y0 = buffer[indexReadFrac0];
		T y1 = buffer[indexReadFrac1];
		T y = y0 + (y1 - y0) * factor;
		buffer[indexWrite] = x;
	
		++offset;

		return y;
	}

	inline void Process(const T* inData, const T* modData, T* outData, count_t numFrames, count_t c = 0)
	{
		for (count_t f = 0; f < numFrames; ++f)
		{
			outData[f] = Process(inData[f], modData[f], c);
		}
	}

	inline void Write(T x, count_t c = 0)
	{
		if (m_NumFrames == 0)
		{
			return;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];

		// Grab write/read indices
		ucount_t indexWrite = offset & m_ModuloMask;
		
		// Write
		buffer[indexWrite] = x;
	
		++offset;
	}

	inline T Search(T d, count_t c = 0) const
	{
		if (m_NumFrames == 0)
		{
			return 0.0f;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];

		// Grab write/read indices
		ucount_t indexWrite = offset & m_ModuloMask;
		T indexReadFrac = d;
		indexReadFrac = (indexWrite - static_cast<ucount_t>(indexReadFrac)) & m_ModuloMask;
		ucount_t indexReadFrac0 = indexReadFrac;
		ucount_t indexReadFrac1 = (indexReadFrac0 + 1) & m_ModuloMask;

		// Interpolate read & write
		T factor = indexReadFrac - T{ indexReadFrac0 };
		T y0 = buffer[indexReadFrac0];
		T y1 = buffer[indexReadFrac1];
		T y = y0 + (y1 - y0) * factor;

		return y;
	}

	inline void SetDelay(T numDelayFrames)
	{
		m_NumDelayFrames = std::clamp(numDelayFrames, T{ 0 }, T{ m_NumFrames - 1 });
	}

private:
	ucount_t NextPowerOfTwo(count_t n)
	{
		ucount_t power = 1;

		while (power < n)
		{
			power <<= 1;
		}
		
		return power;
	}

private:
	ucount_t m_NumFrames = 0;
	ucount_t m_ModuloMask;
	T m_NumDelayFrames  = T{ 0 };
	std::array<ucount_t, C> m_Offsets = { 0 };
	std::array<SmallVector<T, F>, C> m_Buffers;
};

template<typename T, count_t C = 1, count_t F = k_MaxNumInplaceFrames>
struct DelayFeedback
{
	DelayFeedback(count_t numFrames = 0)
	{
		Reset(numFrames);
	}

	inline void Reset(count_t numFrames)
	{
		ucount_t realNumFrames = NextPowerOfTwo(numFrames);
		m_NumFrames = realNumFrames;
		m_ModuloMask = realNumFrames - 1;
		for (auto& offset : m_Offsets)
		{
			offset = 0;
		}
		for (auto& buffer : m_Buffers)
		{
			buffer.resize(realNumFrames, T{});
			buffer.shrink_to_fit();
		}
	}

	inline T Process(T x, T m = T{ 0 }, T f = T{ 0 }, count_t c = 0)
	{
		if (m_NumFrames == 0)
		{
			return x;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];

		// Grab write/read indices
		ucount_t indexWrite = offset & m_ModuloMask;
		T indexReadFrac = m_NumDelayFrames + m;
		indexReadFrac = (indexWrite - static_cast<ucount_t>(indexReadFrac)) & m_ModuloMask;
		ucount_t indexReadFrac0 = indexReadFrac;
		ucount_t indexReadFrac1 = (indexReadFrac0 + 1) & m_ModuloMask;

		// Interpolate read & write
		T factor = indexReadFrac - T{ indexReadFrac0 };
		T y0 = buffer[indexReadFrac0];
		T y1 = buffer[indexReadFrac1];
		T y = y0 + (y1 - y0) * factor;
		buffer[indexWrite] = x + f * y;
	
		++offset;

		return y;
	}

	inline void Process(const T* inData, const T* modData, const T* feedbackData, T* outData, count_t numFrames, count_t c = 0)
	{
		for (count_t f = 0; f < numFrames; ++f)
		{
			outData[f] = Process(inData[f], modData[f], feedbackData[f], c);
		}
	}

	inline void SetDelay(T numDelayFrames)
	{
		m_NumDelayFrames = std::clamp(numDelayFrames, T{ 0 }, T{ m_NumFrames - 1 });
	}

private:
	ucount_t NextPowerOfTwo(count_t n)
	{
		ucount_t power = 1;

		while (power < n)
		{
			power <<= 1;
		}
		
		return power;
	}

private:
	ucount_t m_NumFrames = 0;
	ucount_t m_ModuloMask;
	T m_NumDelayFrames  = T{ 0 };
	std::array<ucount_t, C> m_Offsets = { 0 };
	std::array<SmallVector<T, F>, C> m_Buffers;
};

}