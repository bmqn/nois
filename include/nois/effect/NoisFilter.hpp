#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Filter : public ProcessStream
{
public:
	enum Kind
	{
		k_N2ButterworthLow,
		k_N2ButterworthHigh,
		k_LR4Low,
		k_LR4High,
	};

	static Ref_t<Filter> Create(Kind kind);

	NOIS_INTERFACE(Filter)
	NOIS_INTERFACE_PARAM(CutoffRatio, FloatBlockParameter)

	f32_t GetResponseMagnitude(f32_t ratio) const;
};

class BandpassFilter : public ProcessStream
{
public:
	enum Kind
	{
		k_Biquad,
		k_WindowedSinc
	};

	static Ref_t<BandpassFilter> Create(Kind kind);

	NOIS_INTERFACE(BandpassFilter)
	NOIS_INTERFACE_PARAM(CutoffRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Q, FloatBlockParameter)

	f32_t GetResponseMagnitude(f32_t ratio) const;
};

class AllpassFilter : public ProcessStream
{
public:
	enum Kind
	{
		k_RBJBiquad
	};

	static Ref_t<AllpassFilter> Create(Kind kind);

	NOIS_INTERFACE(AllpassFilter)
	NOIS_INTERFACE_PARAM(CutoffRatio, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(Q, FloatBlockParameter)

	f32_t GetResponseMagnitude(f32_t ratio) const;
};

}
