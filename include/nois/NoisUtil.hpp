#pragma once

#include "NoisTypes.hpp"
#include "NoisConfig.hpp"

#include <algorithm>
#include <cmath>
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
	return std::pow(10.0f, (db) / 10.0f) + k_DcOffset;
}

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
