#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class Expander : public Stream
{
public:
	virtual ~Expander() {}

	virtual Ref_t<FloatParameter> GetRatio() const = 0;
	virtual void SetRatio(Ref_t<FloatParameter> ratio) = 0;

	virtual Ref_t<FloatParameter> GetThresholdDb() const = 0;
	virtual void SetThresholdDb(Ref_t<FloatParameter> thesholdDb) = 0;

	virtual Ref_t<FloatParameter> GetAttackMs() const = 0;
	virtual void SetAttackMs(Ref_t<FloatParameter> attackMs) = 0;

	virtual Ref_t<FloatParameter> GetReleaseMs() const = 0;
	virtual void SetReleaseMs(Ref_t<FloatParameter> releaseMs) = 0;

	virtual Ref_t<FloatParameter> GetSmoothing() const = 0;
	virtual void SetSmoothing(Ref_t<FloatParameter> smoothing) = 0;
};

Ref_t<Expander> CreateExpander(Ref_t<Stream> stream);

}
