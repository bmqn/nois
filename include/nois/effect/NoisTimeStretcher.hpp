#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisStream.hpp"

#include <functional>
#include <memory>

namespace nois {

class TimeStretcher : public Stream
{
public:
	virtual ~TimeStretcher() {}

	virtual bool GetStretchActive() = 0;
	virtual void SetStretchActive(bool stretchActive) = 0;
	virtual void BindStretchActive(std::function<bool()> stretchActiveFunc) = 0;

	virtual data_t GetStretchTimeMs() = 0;
	virtual void SetStretchTimeMs(data_t stretchTimeMs) = 0;

	virtual data_t GetStretchFactor() = 0;
	virtual void SetStretchFactor(data_t stretchFactor) = 0;
	virtual void BindStretchFactor(std::function<data_t()> stretchFactorFunc) = 0;

	virtual data_t GetGrainSize() = 0;
	virtual void SetGrainSize(data_t grainSize) = 0;

	virtual data_t GetGrainBlend() = 0;
	virtual void SetGrainBlend(data_t grainBlend) = 0;
};

std::shared_ptr<TimeStretcher> CreateTimeStretcher(Stream *stream);

}
