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

	virtual void SetNumChannels(int32_t numChannels) = 0;
	virtual void SetSampleRate(int32_t sampleRate) = 0;
	virtual void SetCutoff(data_t cutoff) = 0;
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

	virtual void SetNumChannels(int32_t numChannels) = 0;
	virtual void SetSampleRate(int32_t sampleRate) = 0;
	virtual void SetCutoff(data_t cutoff) = 0;
	virtual void SetQ(data_t q) = 0;
};

std::shared_ptr<Filter> CreateFilter(std::shared_ptr<Stream> stream, Filter::Kind kind = Filter::k_N2ButterWorth);

};
