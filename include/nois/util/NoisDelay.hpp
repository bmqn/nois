#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisBuffer.hpp"

#include <array>

namespace nois {

template<typename T, count_t C = 1>
struct Delay
{
	Delay(count_t numFrames = 0, bool stopWhenFull = false)
	{
		Configure(numFrames, stopWhenFull);
	}

	inline void Configure(count_t numFrames, bool stopWhenFull = false)
	{
		m_StopWhenFull = stopWhenFull;
		
		m_NumFrames = numFrames;
		if (numFrames > 0)
		{
			m_RealNumFrames = NextPowerOfTwo(numFrames);
			m_ModuloMask = m_RealNumFrames - 1;
		}
		else
		{
			m_RealNumFrames = 0;
			m_ModuloMask = 0;
		}

		for (auto& offset : m_Offsets)
		{
			offset = 0;
		}

		for (auto& buffer : m_Buffers)
		{
			buffer.Resize(m_RealNumFrames, 1);
		}
	}
	
	inline void Restart()
	{
		for (auto& offset : m_Offsets)
		{
			offset = 0;
		}
	}

	inline count_t GetOffset(count_t c)
	{
		return m_Offsets[c];
	}
	
	inline T GetMaxDelay() const
	{
		if (m_RealNumFrames == 0)
		{
			return 0;
		}

		return static_cast<T>(m_NumFrames - 1);
	}

	inline void SetDelay(T numDelayFrames)
	{
		m_NumDelayFrames = std::clamp(numDelayFrames, T{ 0 }, static_cast<T>(m_NumFrames - 1));
	}

	inline T GetDelay() const
	{
		return m_NumDelayFrames;
	}

	inline T Process(T x, count_t c = 0, T m = T{ 0 }, T f = T{ 0 })
	{
		if (m_RealNumFrames == 0)
		{
			return x;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];

		// Grab write/read indices
		ucount_t indexWrite = offset & m_ModuloMask;
		ucount_t d0 = static_cast<ucount_t>(m_NumDelayFrames) + static_cast<ucount_t>(m);
		ucount_t d1 = d0 + 1;
		ucount_t indexRead0 = (static_cast<count_t>(offset + m_RealNumFrames) - d0 - 1) & m_ModuloMask;
		ucount_t indexRead1 = (static_cast<count_t>(offset + m_RealNumFrames) - d1 - 1) & m_ModuloMask;

		// Interpolate read & write
		T factor = m_NumDelayFrames - static_cast<T>(d0);
		T y0 = buffer[indexRead0];
		T y1 = buffer[indexRead1];
		T y = y0 + (y1 - y0) * factor;
		buffer[indexWrite] = x + f * y;
	
		++offset;

		return y;
	}

	inline void Process(const T* inData, T* outData, const T* feedbackData, const T* modData, count_t numFrames, count_t c = 0)
	{
		for (count_t f = 0; f < numFrames; ++f)
		{
			outData[f] = Process(inData[f], c, modData ? modData[f] : 0.0f, feedbackData ? feedbackData[f] : 0.0f);
		}
	}

	inline void Write(T x, count_t c = 0)
	{
		if (m_RealNumFrames == 0)
		{
			return;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];
		
		if (m_StopWhenFull && offset >= m_RealNumFrames)
		{
			return;
		}

		// Grab write/read indices
		ucount_t indexWrite = offset & m_ModuloMask;
		
		// Write
		buffer[indexWrite] = x;
	
		++offset;
	}

	inline T Search(T d, count_t c = 0) const
	{
		if (m_RealNumFrames == 0)
		{
			return 0.0f;
		}

		auto& offset = m_Offsets[c];
		auto& buffer = m_Buffers[c];

		// Grab write/read indices
		count_t d0 = static_cast<count_t>(d);
		count_t d1 = d0 + 1;
		ucount_t indexRead0 = (static_cast<count_t>(offset + m_RealNumFrames) - d0 - 1) & m_ModuloMask;
		ucount_t indexRead1 = (static_cast<count_t>(offset + m_RealNumFrames) - d1 - 1) & m_ModuloMask;

		// Interpolate read & write
		T factor = d - static_cast<T>(d0);
		T y0 = buffer[indexRead0];
		T y1 = buffer[indexRead1];
		T y = y0 + (y1 - y0) * factor;

		return y;
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
	bool m_StopWhenFull = false;
	ucount_t m_NumFrames = 0;
	ucount_t m_RealNumFrames = 0;
	ucount_t m_ModuloMask;
	T m_NumDelayFrames  = T{ 0 };
	std::array<ucount_t, C> m_Offsets = { 0 };
	std::array<Buffer<T>, C> m_Buffers;
};

}
