#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class TimeStretcher : public Stream
{
public:
	virtual ~TimeStretcher() {}

	virtual Ref_t<FloatParameter> GetStretchActive() = 0;
	virtual void SetStretchActive(Ref_t<FloatParameter> stretchActive) = 0;

	virtual Ref_t<FloatParameter> GetStretchTimeMs() = 0;
	virtual void SetStretchTimeMs(Ref_t<FloatParameter> stretchTimeMs) = 0;

	virtual Ref_t<FloatParameter> GetStretchFactor() = 0;
	virtual void SetStretchFactor(Ref_t<FloatParameter> stretchFactor) = 0;

	virtual Ref_t<FloatParameter> GetGrainSize() = 0;
	virtual void SetGrainSize(Ref_t<FloatParameter> grainSize) = 0;

	virtual Ref_t<FloatParameter> GetGrainBlend() = 0;
	virtual void SetGrainBlend(Ref_t<FloatParameter> grainBlend) = 0;
};

Ref_t<TimeStretcher> CreateTimeStretcher(Ref_t<Stream> stream);

}
