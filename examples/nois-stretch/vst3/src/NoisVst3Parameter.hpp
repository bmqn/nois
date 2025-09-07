#pragma once

#include <nois/Nois.hpp>

#include <public.sdk/source/vst/vstparameters.h>
#include <pluginterfaces/base/ustring.h>
#include <pluginterfaces/vst/vsttypes.h>

using namespace Steinberg;

namespace parameter
{

enum
{
	kStretchActive,
	kStretchFactor,
	kGrainPhaseInc,
	kGrainLockActive
};

struct StretchActive
{
	static constexpr Vst::ParamID kPid = kStretchActive;
	static constexpr nois::f32_t kDefaultValue = 0.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 1;
};

struct StretchFactor
{
	static constexpr Vst::ParamID kPid = kStretchFactor;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = 1.0f;
	static constexpr nois::f32_t kMaxValue = 32.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct GrainPhaseInc
{
	static constexpr Vst::ParamID kPid = kGrainPhaseInc;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 0;
};

struct GrainPhaseLockActive
{
	static constexpr Vst::ParamID kPid = kGrainLockActive;
	static constexpr nois::f32_t kDefaultValue = 0.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 1;
};

template<typename Parameter>
inline nois::f32_t ApplyStep(nois::f32_t valuePlain)
{
	if (Parameter::kNumSteps <= 0)
	{
		return valuePlain;
	}

	nois::f32_t step = (Parameter::kMaxValue - Parameter::kMinValue) / nois::f32_t(Parameter::kNumSteps);
	nois::s32_t steps = nois::s32_t(std::round((valuePlain - Parameter::kMinValue) / step));

	return Parameter::kMinValue + steps * step;
}

template<typename Parameter>
inline nois::f32_t  ToPlain(nois::f32_t valueNormalized)
{
	return ApplyStep<Parameter>(valueNormalized * (Parameter::kMaxValue - Parameter::kMinValue) + Parameter::kMinValue);
}

template<typename Parameter>
inline nois::f32_t  ToNormalized(nois::f32_t valuePlain)
{
	return (ApplyStep<Parameter>(valuePlain) - Parameter::kMinValue) / (Parameter::kMaxValue - Parameter::kMinValue);
}

}

template<typename Param>
class NoisVstProcessorParameter
{
public:
	static constexpr Vst::ParamID kPid = Param::kPid;
	static constexpr Vst::ParamID kNumSteps = Param::kNumSteps;

	NoisVstProcessorParameter()
		: mParameter(nois::CreateParameter(nois::FloatBinderFunc_t(
		[&](nois::count_t f) -> nois::f32_t
		{
			return mParameterValues[f];
		})))
	{
	}

	static Vst::ParamValue ApplyStep(Vst::ParamValue valuePlain)
	{
		return parameter::ApplyStep<Param>(valuePlain);
	}

	static Vst::ParamValue ToPlain(Vst::ParamValue valueNormalized)
	{
		return parameter::ToPlain<Param>(valueNormalized);
	}

	static Vst::ParamValue ToNormalized(Vst::ParamValue valuePlain)
	{
		return parameter::ToNormalized<Param>(valuePlain);
	}

	inline void PrepareForWrite(nois::count_t numFrames)
	{
		mParameterValues.resize(numFrames, Param::kDefaultValue);
	}

	inline void WriteValue(nois::count_t f, nois::f32_t value)
	{
		mParameterValues[f] = value;
	}

	inline nois::f32_t GetLastValue() const
	{
		return mParameterValues.back();
	}

	inline operator nois::Ref_t<nois::FloatParameter>()
	{
		return mParameter;
	}

private:
	using ValueStore = nois::SmallVector<nois::f32_t, 1024>;

	nois::Ref_t<nois::FloatParameter> mParameter;
	ValueStore mParameterValues;
};

template<typename Param>
class NoisVstControllerParameter : public Vst::Parameter
{
public:
	NoisVstControllerParameter(const char* title)
		: Vst::Parameter(USTRING(title), Param::kPid)
	{
		setNormalized(toNormalized(Param::kDefaultValue));
		setPrecision(2);
	}

	Vst::ParamValue applyStep(Vst::ParamValue valuePlain) const
	{
		return parameter::ApplyStep<Param>(valuePlain);
	}

	Vst::ParamValue toPlain(Vst::ParamValue valueNormalized) const SMTG_OVERRIDE
	{
		return parameter::ToPlain<Param>(valueNormalized);
	}

	Vst::ParamValue toNormalized(Vst::ParamValue valuePlain) const SMTG_OVERRIDE
	{
		return parameter::ToNormalized<Param>(valuePlain);
	}

	void toString(Vst::ParamValue valueNormalized, Vst::String128 string) const SMTG_OVERRIDE
	{
		UString wrapper(string, USTRINGSIZE(Vst::String128));

		if (Param::kNumSteps == 1)
		{
			wrapper.assign(toPlain(valueNormalized) > 0.0f ? kOnStr : kOffStr);
		}
		else
		{
			wrapper.printFloat(toPlain(valueNormalized), precision);
		}
	}

	bool fromString(const Vst::TChar* string, Vst::ParamValue& valueNormalized) const SMTG_OVERRIDE
	{
		UString wrapper(const_cast<Vst::TChar*>(string), USTRINGSIZE(Vst::String128));

		Vst::ParamValue value = 0.0;

		if (Param::kNumSteps == 1 && std::memcmp(wrapper, kOffStr, sizeof(kOffStr)) == 0)
		{
			value = 0.0;
		}
		else if (Param::kNumSteps == 1 && std::memcmp(wrapper, kOnStr, sizeof(kOnStr)) == 0)
		{
			value = 1.0;
		}
		else if (wrapper.scanFloat(value))
		{
			if (value < Param::kMinValue)
			{
				value = Param::kMinValue;
			}
			else if (value > Param::kMaxValue)
			{
				value = Param::kMaxValue;
			}
		
			valueNormalized = toNormalized(value);

			return true;
		}

		return false;
	}

private:
	static constexpr Vst::TChar kOffStr[] = {'O','f','f','\0'};
	static constexpr Vst::TChar kOnStr[] = {'O','n','\0'};
};
