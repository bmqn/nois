#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

#include <memory>

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

	virtual data_t GetCutoffRatio() = 0;
	virtual void SetCutoffRatio(data_t cutoffRatio) = 0;
};

class BandpassFilter : public Stream
{
public:
	enum Kind
	{
		k_Biquad
	};

public:
	virtual ~BandpassFilter() {}

	virtual void SetCutoffRatio(data_t cutoffRatio) = 0;
	virtual void SetQ(data_t q) = 0;
};

std::shared_ptr<Filter> CreateFilter(std::shared_ptr<Stream> stream, Filter::Kind kind = Filter::k_N2ButterWorth);

}
