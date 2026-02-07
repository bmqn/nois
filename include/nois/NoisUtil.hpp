#pragma once

#include "NoisConfig.hpp"
#include "NoisMacros.hpp"
#include "NoisTypes.hpp"

#include <cmath>

#if NOIS_ARCH_X64
#include <xmmintrin.h>
#endif // NOIS_ARCH_X64

namespace nois {

constexpr f32_t k_DcOffset = 0.001f;

inline f32_t ToDb(f32_t x)
{
	return 10.0f * std::log10(x - k_DcOffset);
}

inline f32_t FromDb(f32_t db)
{
	return std::pow(10.0f, db / 10.0f) + k_DcOffset;
}

template<typename T>
inline T FastTanh(T x)
{
	return x * (T{ 27.0 } + x * x) / (T{ 27.0 } + T{ 9.0 } * x * x);
}

class ScopedNoDenorms
{
public:
	ScopedNoDenorms()
	{
#if NOIS_ARCH_X64
		m_State = _mm_getcsr();
		// Set DAZ (bit 6) and FTZ (bit 15)
		u32_t mxcsr = m_State | (1u << 6) | (1u << 15);
		_mm_setcsr(mxcsr);
#elif NOIS_ARCH_ARM64
		asm volatile("mrs %0, fpcr" : "=r"(m_State));
		// Set FTZ (bit 24)
		u64_t fpcr = m_State | (1ul << 24);
		asm volatile("msr fpcr, %0" :: "r"(fpcr));
#endif // NOIS_ARCH_X64 + NOIS_ARCH_ARM64
	}

	~ScopedNoDenorms()
	{
#if NOIS_ARCH_X64
		// Restore old state
		_mm_setcsr(m_State);
#elif NOIS_ARCH_ARM64
		asm volatile("msr fpcr, %0" :: "r"(m_State));
#endif // NOIS_ARCH_X64 + NOIS_ARCH_ARM64
	}

private:
#if NOIS_ARCH_X64
        u32_t m_State;
#elif NOIS_ARCH_ARM64
	u64_t m_State;
#endif // NOIS_ARCH_X64 + NOIS_ARCH_ARM64
};


}
