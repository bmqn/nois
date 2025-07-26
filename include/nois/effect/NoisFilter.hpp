#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Filter : public Stream
{
public:
	enum Kind
	{
		k_N2ButterWorthLow,
		k_N2ButterWorthHigh
	};

public:
	virtual ~Filter() {}

	virtual Ref_t<FloatParameter> GetCutoffRatio() = 0;
	virtual void SetCutoffRatio(Ref_t<FloatParameter> cutoffRatio) = 0;

	virtual f32_t GetResponseMagnitude(f32_t freqRatio) const = 0;
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

	virtual Ref_t<FloatParameter> GetCutoffRatio() = 0;
	virtual void SetCutoffRatio(Ref_t<FloatParameter> cutoffRatio) = 0;

	virtual Ref_t<FloatParameter> GetQ() = 0;
	virtual void SetQ(Ref_t<FloatParameter> q) = 0;

	virtual f32_t GetResponseMagnitude(f32_t freqRatio) const = 0;
};

Ref_t<Filter> CreateFilter(Ref_t<Stream> stream,
	Filter::Kind kind = Filter::k_N2ButterWorthLow);

Ref_t<BandpassFilter> CreateBandpassFilter(Ref_t<Stream> stream,
	BandpassFilter::Kind kind = BandpassFilter::k_Biquad);

}
