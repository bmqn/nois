#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class FilterBank : public Stream
{
public:
	virtual ~FilterBank() = default;

	virtual count_t GetNumBands() const = 0;

	virtual f32_t GetBandFrequency(
		count_t bandIndex,
		f32_t sampleRate) const = 0;

	virtual void SetBandGains(
		FloatBlockParameterList bandGains) = 0;

	virtual FloatBlockParameterList GetBandRmses() const = 0;
};

Ref_t<FilterBank> CreateFilterBank(
	Ref_t<Stream> stream,
	count_t numBands,
	f32_t minCutoffRatio,
	f32_t maxCutoffRatio);

}
