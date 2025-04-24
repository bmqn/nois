#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Filter : public Stream
{
public:
	enum Kind
	{
		k_N2ButterWorth
	};

public:
	virtual ~Filter() {}

	virtual const FloatParameter &GetCutoffRatio() const = 0;
	virtual void SetCutoffRatio(Ref_t<FloatParameter> gain) = 0;
};

class BandpassFilter : public Stream
{
public:
	enum Kind
	{
		k_Biquad,
		k_WindowedSinc
	};

public:
	virtual ~BandpassFilter() {}

	virtual const FloatParameter &GetCutoffRatio() const = 0;
	virtual void SetCutoffRatio(Ref_t<FloatParameter> gain) = 0;

	virtual data_t GetQ() = 0;
	virtual void SetQ(data_t q) = 0;

	virtual data_t GetResponseMagnitude(data_t freqRatio) const = 0;
};

Ref_t<Filter> CreateFilter(Stream *stream,
	Filter::Kind kind = Filter::k_N2ButterWorth);

Ref_t<BandpassFilter> CreateBandpassFilter(Stream* stream,
	BandpassFilter::Kind kind = BandpassFilter::k_Biquad);

}
