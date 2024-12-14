#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

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

	virtual const FloatParameter &GetStretchFactor() const = 0;
	template<typename T, typename... Args>
	void SetStretchFactor(Args &&...args);

	virtual const FloatParameter &GetGrainSize() const = 0;
	template<typename T, typename... Args>
	void SetGrainSize(Args &&...args);

	virtual data_t GetGrainBlend() = 0;
	virtual void SetGrainBlend(data_t grainBlend) = 0;

private:
	virtual void SetStretchFactorImpl(Own_t<FloatParameter> &&stretchFactor) = 0;
	virtual void SetGrainSizeImpl(Own_t<FloatParameter> &&grainSize) = 0;
};

Ref_t<TimeStretcher> CreateTimeStretcher(Stream *stream);

template<typename T, typename... Args>
void TimeStretcher::SetStretchFactor(Args &&...args)
{
	SetStretchFactorImpl(MakeOwn<T>(std::forward<Args>(args)...));
}

template<typename T, typename... Args>
void TimeStretcher::SetGrainSize(Args &&...args)
{
	SetGrainSizeImpl(MakeOwn<T>(std::forward<Args>(args)...));
}

}
