#pragma once

#include "NoisConfig.hpp"
#include "NoisMacros.hpp"
#include "NoisTypes.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#if NOIS_ARCH_X64
#include <xmmintrin.h>
#endif // NOIS_ARCH_X64

namespace nois {

inline f32_t ToDb(f32_t x)
{
	return 10.0f * std::log10(x - k_DcOffset);
}

inline f32_t FromDb(f32_t db)
{
	return std::pow(10.0f, db / 10.0f) + k_DcOffset;
}

template<typename T>
struct Biquad
{
	Biquad()
	{
		Reset();
	}

	inline void Reset()
	{
		std::fill(m_Z1s.begin(), m_Z1s.end(), T{0});
		std::fill(m_Z2s.begin(), m_Z2s.end(), T{0});
	}

	inline T Process(T x, count_t c)
	{
		T y = b0 * x + m_Z1s[c];
		m_Z1s[c] = b1 * x - a1 * y + m_Z2s[c];
		m_Z2s[c] = b2 * x - a2 * y;
		return y;
	}

	inline void Process(const T* inData, T* outData, count_t numFrames, count_t c)
	{
		for (count_t f = 0; f < numFrames; ++f)
		{
			outData[f] = Process(inData[f], c);
		}
	}

	T GetMagnitude(T freqRatio) const
	{
		T omega0 = std::numbers::pi * freqRatio;

		T cosw0 = std::cos(omega0);
		T sinw0 = std::sin(omega0);
		T cos2w0 = std::cos(2.0 * omega0);
		T sin2w0 = std::sin(2.0 * omega0);

		T numReal = b0 + b1 * cosw0 + b2 * cos2w0;
		T numImag = b1 * sinw0 + b2 * sin2w0;
		T numMag2 = numReal * numReal + numImag * numImag;

		T denReal = 1.0 + a1 * cosw0 + a2 * cos2w0;
		T denImag = a1 * sinw0 + a2 * sin2w0;
		T denMag2 = denReal * denReal + denImag * denImag;

		return std::sqrt(numMag2 / denMag2);
	}

	void MakeButterworthLow(T cutoffRatio, T q = 1.0 / std::numbers::sqrt2)
	{
		cutoffRatio = std::clamp(cutoffRatio, T(0.001), T(0.999));
		q = std::max(q, T(0.001));

		T omega0 = std::numbers::pi * cutoffRatio;
		T cosw0 = std::cos(omega0);
		T sinw0 = std::sin(omega0);
		T alpha = sinw0 / (2.0 * q);

		T a0 = 1.0 + alpha;
		a1 = -2.0 * cosw0 / a0;
		a2 = (1.0 - alpha) / a0;
		b0 = ((1.0 - cosw0) * 0.5) / a0;
		b1 = (1.0 - cosw0) / a0;
		b2 = ((1.0 - cosw0) * 0.5) / a0;
	}

	void MakeButterworthHigh(T cutoffRatio, T q = 1.0 / std::numbers::sqrt2)
	{
		cutoffRatio = std::clamp(cutoffRatio, T(0.001), T(0.999));
		q = std::max(q, T(0.001));

		T omega0 = std::numbers::pi * cutoffRatio;
		T cosw0 = std::cos(omega0);
		T sinw0 = std::sin(omega0);
		T alpha = sinw0 / (2.0 * q);

		T a0 = 1.0 + alpha;
		a1 = -2.0 * cosw0 / a0;
		a2 = (1.0 - alpha) / a0;
		b0 = ((1.0 + cosw0) * 0.5) / a0;
		b1 = -(1.0 + cosw0) / a0;
		b2 = ((1.0 + cosw0) * 0.5) / a0;
	}

	void MakeBandpass(T cutoffRatio, T q)
	{
		cutoffRatio = std::clamp(cutoffRatio, T(0.001), T(0.999));
		q = std::max(q, T(0.001));

		T omega0 = std::numbers::pi * cutoffRatio;
		T sinw0 = std::sin(omega0);
		T cosw0 = std::cos(omega0);
		T alpha = sinw0 / (2.0 * q);

		T a0 = 1.0 + alpha;
		b0 = alpha / a0;
		b1 = 0.0;
		b2 = -alpha / a0;
		a1 = -2.0 * cosw0 / a0;
		a2 = (1.0 - alpha) / a0;
	}

	void MakeAllpass(T cutoffRatio, T q = 1.0 / std::numbers::sqrt2)
	{
		cutoffRatio = std::clamp(cutoffRatio, T(0.001), T(0.999));
		q = std::max(q, T(0.001));

		T omega0 = std::numbers::pi * cutoffRatio;
		T sinw0 = std::sin(omega0);
		T cosw0 = std::cos(omega0);
		T alpha = sinw0 / (2.0 * q);

		T a0 = 1.0 + alpha;
		b0 = (1.0 - alpha) / a0;
		b1 = -(2.0 * cosw0) / a0;
		b2 = 1.0;
		a1 = -(2.0 * cosw0) / a0;
		a2 = (1.0 - alpha) / a0;
	}

	T a1 = 0.0, a2 = 0.0;
	T b0 = 0.0, b1 = 0.0, b2 = 0.0;

private:
	std::array<T, k_MaxChannels> m_Z1s;
	std::array<T, k_MaxChannels> m_Z2s;
};

class ScopedNoDenorms
{
public:
	ScopedNoDenorms()
#if NOIS_ARCH_X64
		: m_State(_mm_getcsr())
#elif NOIS_ARCH_ARM64
		: m_State(__builtin_aarch64_get_fpcr())
#endif // NOIS_ARCH_X64 + NOIS_ARCH_ARM64
	{
#if NOIS_ARCH_X64
		// Set DAZ (bit 6) and FTZ (bit 15)
		u32_t mxcsr = m_State | (1u << 6) | (1u << 15);
		_mm_setcsr(mxcsr);
#elif NOIS_ARCH_ARM64
		// Set DAZ (bit 19) and FTZ (bit 24)
		u32_t fpcr = m_State | (1u << 24) | (1u << 19);
		__builtin_aarch64_set_fpcr(fpcr);
#endif // NOIS_ARCH_X64 + NOIS_ARCH_ARM64
	}

	~ScopedNoDenorms()
	{
#if NOIS_ARCH_X64
		// Restore old state
		_mm_setcsr(m_State);
#elif NOIS_ARCH_ARM64
		__builtin_aarch64_set_fpcr(m_State);
#endif // NOIS_ARCH_X64 + NOIS_ARCH_ARM64
	}

private:
	u32_t m_State;
};


};
