#pragma once

#include "NoisTypes.hpp"
#include "NoisConfig.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <xmmintrin.h>

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
		: m_State(_mm_getcsr())
	{
		// Set DAZ (bit 6) and FTZ (bit 15)
		u32_t mxcsr = m_State | (1 << 6) | (1 << 15);
		_mm_setcsr(mxcsr);
	}

	~ScopedNoDenorms()
	{
		// Restore old state
		_mm_setcsr(m_State);
	}

private:
	u32_t m_State;
};


};
