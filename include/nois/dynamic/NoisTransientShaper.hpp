#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class TransientShaper : public Stream
{
public:
	virtual ~TransientShaper() {}

	virtual Ref_t<FloatParameter> GetAttackRatio() const = 0;
	virtual void SetAttackRatio(Ref_t<FloatParameter> attackRatio) = 0;

	virtual Ref_t<FloatParameter> GetSustainRatio() const = 0;
	virtual void SetSustainRatio(Ref_t<FloatParameter> sustainRatio) = 0;

	virtual Ref_t<FloatParameter> GetAttackMs() const = 0;
	virtual void SetAttackMs(Ref_t<FloatParameter> attackMs) = 0;

	virtual Ref_t<FloatParameter> GetReleaseMs() const = 0;
	virtual void SetReleaseMs(Ref_t<FloatParameter> releaseMs) = 0;

	virtual Ref_t<FloatParameter> GetSmoothing() const = 0;
	virtual void SetSmoothing(Ref_t<FloatParameter> smoothing) = 0;

	virtual f32_t GetGain() const = 0;
};

class MultibandTransientShaper : public Stream
{
public:
	virtual ~MultibandTransientShaper() {}

	virtual Ref_t<FloatParameter> GetAttackRatio() const = 0;
	virtual void SetAttackRatio(Ref_t<FloatParameter> attackRatio) = 0;

	virtual Ref_t<FloatParameter> GetSustainRatio() const = 0;
	virtual void SetSustainRatio(Ref_t<FloatParameter> sustainRatio) = 0;

	virtual Ref_t<FloatParameter> GetAttackMs() const = 0;
	virtual void SetAttackMs(Ref_t<FloatParameter> attackMs) = 0;

	virtual Ref_t<FloatParameter> GetReleaseMs() const = 0;
	virtual void SetReleaseMs(Ref_t<FloatParameter> releaseMs) = 0;

	virtual Ref_t<FloatParameter> GetSmoothing() const = 0;
	virtual void SetSmoothing(Ref_t<FloatParameter> smoothing) = 0;

	virtual void SetLowCutoffRatio(Ref_t<FloatParameter> cutoffRatio) = 0;
	virtual void SetHighCutoffRatio(Ref_t<FloatParameter> cutoffRatio) = 0;
	virtual void SetBandCutoffRatio(Ref_t<FloatParameter> cutoffRatio) = 0;

	virtual f32_t GetLowGain() const = 0;
	virtual f32_t GetHighGain() const = 0;
	virtual f32_t GetBandGain() const = 0;

	virtual f32_t GetLowReponseMagnitude(f32_t freqRatio) const = 0;
	virtual f32_t GetHighReponseMagnitude(f32_t freqRatio) const = 0;
	virtual f32_t GetBandReponseMagnitude(f32_t freqRatio) const = 0;
};

Ref_t<TransientShaper> CreateTransientShaper(
	Ref_t<Stream> stream);

Ref_t<MultibandTransientShaper> CreateMultibandTransientShaper(
	Ref_t<Stream> stream);

}
