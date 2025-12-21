#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class FilterBank : public ProcessStream
{
public:
	static Ref_t<FilterBank> Create(
		count_t numBands,
		f32_t minCutoffRatio,
		f32_t maxCutoffRatio);

	NOIS_INTERFACE(FilterBank)

	f32_t GetRms(count_t b) const;
};

}
