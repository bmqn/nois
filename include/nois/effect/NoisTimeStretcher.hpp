#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

namespace nois {

class TimeStretcher : public Stream
{
public:
	TimeStretcher(Ref_t<Stream> stream);

	TimeStretcher(const TimeStretcher&) = delete;
	TimeStretcher& operator=(const TimeStretcher&) = delete;
	TimeStretcher(TimeStretcher&&) noexcept = delete;
	TimeStretcher& operator=(TimeStretcher&&) noexcept = delete;

	Stream::Result Consume(FloatBuffer& buffer, f32_t sampleRate) override final;
	void PrepareToConsume(count_t numFrames, count_t numChannels, f32_t sampleRate) override final;

	NOIS_INTERFACE_PARAM(StretchActive, FloatParameter)
	NOIS_INTERFACE_PARAM(StretchTimeMs, FloatBlockParameter)
	NOIS_INTERFACE_PARAM(StretchFactor, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainSize, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainBlend, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainGain, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainPhaseInc, FloatParameter)
	NOIS_INTERFACE_PARAM(GrainLockActive, FloatParameter)

private:
	class Impl;
	Own_t<Impl> m_Impl;
};

Ref_t<TimeStretcher> CreateTimeStretcher(Ref_t<Stream> stream);

}
