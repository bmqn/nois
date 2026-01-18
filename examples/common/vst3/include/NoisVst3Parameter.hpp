#pragma once

#include <nois/Nois.hpp>

#include <base/source/fstring.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <pluginterfaces/base/ustring.h>
#include <public.sdk/source/vst/vstparameters.h>

using namespace Steinberg;

class NoisVstProcessorParameter
{
public:
	virtual Vst::ParamID GetPid() const = 0;

	virtual Vst::ParamValue ApplyStep(Vst::ParamValue valuePlain) const = 0;
	virtual Vst::ParamValue ToPlain(Vst::ParamValue valueNormalized) const = 0;
	virtual Vst::ParamValue ToNormalized(Vst::ParamValue valuePlain) const = 0;

	virtual void Prepare(nois::count_t numFrames, nois::f32_t sampleRate) = 0;

	virtual void WritePlain(nois::count_t frame, nois::f32_t value) = 0;
	virtual void WritePlain(nois::count_t frame, nois::count_t count, nois::f32_t value) = 0;
	virtual nois::f32_t GetLastPlain() const = 0;

	virtual operator nois::Ref_t<nois::FloatParameter>() = 0;
	virtual operator nois::Ref_t<nois::FloatBlockParameter>() = 0;

public:
	template<typename Param>
	static nois::Own_t<NoisVstProcessorParameter> Create(nois::FloatParameterRegistry& registry);
};

class NoisVstControllerParameter
{
public:
	virtual Vst::ParamID GetPid() const = 0;

	virtual operator Vst::Parameter*() = 0;

public:
	template<typename Param>
	static nois::Own_t<NoisVstControllerParameter> Create();
};

namespace util
{

template<typename Param>
constexpr inline nois::f32_t ApplyStep(nois::f32_t valuePlain)
{
	nois::s32_t numSteps = Param::kNumSteps;

	if (numSteps > 0)
	{
		nois::f32_t step = (Param::kMaxValue - Param::kMinValue) / nois::f32_t(numSteps);
		nois::s32_t steps = nois::s32_t(std::round((valuePlain - Param::kMinValue) / step));

		return Param::kMinValue + steps * step;
	}

	return valuePlain;
}

template<typename Param>
constexpr inline nois::f32_t ToPlain(nois::f32_t valueNormalized)
{
	return ApplyStep<Param>(valueNormalized * (Param::kMaxValue - Param::kMinValue) + Param::kMinValue);
}

template<typename Param>
constexpr inline nois::f32_t ToNormalized(nois::f32_t valuePlain)
{
	return (ApplyStep<Param>(valuePlain) - Param::kMinValue) / (Param::kMaxValue - Param::kMinValue);
}

template<typename Param>
constexpr inline nois::f32_t ToProcessor(nois::f32_t valuePlain, nois::count_t numSamples, nois::f32_t sampleRate)
{
	return Param::ToProcessor(valuePlain, numSamples, sampleRate);
}

template<typename Param>
inline Vst::ParameterInfo MakeParameterInfo()
{
	Vst::ParameterInfo info;
	ConstString title(Param::kTitle);
	ConstString units(Param::kUnits);

	info.id = Param::kPid;
	title.copyTo(info.title, 0, title.length());
	title.copyTo(info.shortTitle, 0, title.length());
	units.copyTo(info.units, 0, units.length());
	info.stepCount = Param::kNumSteps;
	info.defaultNormalizedValue = util::ToNormalized<Param>(Param::kDefaultValue);
	info.unitId = Vst::kRootUnitId;
	info.flags = Vst::ParameterInfo::kCanAutomate;

	return info;
}

}

template<typename Param>
class NoisVstParameter : public Vst::Parameter
{
public:
	NoisVstParameter()
		: Vst::Parameter(util::MakeParameterInfo<Param>())
	{
		setPrecision(2);
	}

	Vst::ParamValue applyStep(Vst::ParamValue valuePlain) const
	{
		return util::ApplyStep<Param>(valuePlain);
	}

	Vst::ParamValue toPlain(Vst::ParamValue valueNormalized) const SMTG_OVERRIDE
	{
		return util::ToPlain<Param>(valueNormalized);
	}

	Vst::ParamValue toNormalized(Vst::ParamValue valuePlain) const SMTG_OVERRIDE
	{
		return util::ToNormalized<Param>(valuePlain);
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
	static constexpr Vst::TChar kOffStr[] = { 'O','f','f','\0' };
	static constexpr Vst::TChar kOnStr[] = { 'O','n','\0' };
};

template<typename Param>
class NoisVstProcessorParameterImpl : public NoisVstProcessorParameter
{
public:
	NoisVstProcessorParameterImpl(nois::FloatParameterRegistry& registry)
		: mParameter(nullptr)
		, mNumFrames(0)
		, mSampleRate(0.0f)
		, mValues()
	{
		mParameter = registry.CreateBinder(
			[this](nois::count_t f) -> nois::f32_t
			{
				return util::ToProcessor<Param>(
					mValues[f],
					mNumFrames,
					mSampleRate);
			});

		mBlockParameter = registry.CreateBlockBinder(
			[this]() -> nois::f32_t
			{
				return util::ToProcessor<Param>(
					mValues.back(),
					mNumFrames,
					mSampleRate);
			});
	}

	Vst::ParamID GetPid() const override final
	{
		return Param::kPid;
	}

	Vst::ParamValue ApplyStep(Vst::ParamValue valuePlain) const override final
	{
		return util::ApplyStep<Param>(valuePlain);
	}

	Vst::ParamValue ToPlain(Vst::ParamValue valueNormalized) const override final
	{
		return util::ToPlain<Param>(valueNormalized);
	}

	Vst::ParamValue ToNormalized(Vst::ParamValue valuePlain) const override final
	{
		return util::ToNormalized<Param>(valuePlain);
	}

	void Prepare(nois::count_t numFrames, nois::f32_t sampleRate) override final
	{
		mNumFrames = numFrames;
		mSampleRate = sampleRate;
		mValues.resize(numFrames, Param::kDefaultValue);
	}

	void WritePlain(nois::count_t frame, nois::f32_t value) override final
	{
		if (frame < mValues.size())
		{
			mValues[frame] = value;
		}
	}

	void WritePlain(nois::count_t frame, nois::count_t count, nois::f32_t value) override final
	{
		if (frame < mValues.size() && count <= mValues.size() - frame)
		{
			std::fill(mValues.begin() + frame, mValues.begin() + frame + count, value);
		}
	}

	nois::f32_t GetLastPlain() const override final
	{
		if (!mValues.empty())
		{
			return mValues.back();
		}
		return 0.0f;
	}

	operator nois::Ref_t<nois::FloatParameter>() override final
	{
		return mParameter;
	}

	operator nois::Ref_t<nois::FloatBlockParameter>() override final
	{
		return mBlockParameter;
	}

private:
	nois::Ref_t<nois::FloatParameter> mParameter;
	nois::Ref_t<nois::FloatBlockParameter> mBlockParameter;
	nois::count_t mNumFrames;
	nois::f32_t mSampleRate;
	nois::SmallVector<nois::f32_t> mValues;
};

template<typename Param>
class NoisVstControllerParameterImpl : public NoisVstControllerParameter
{
public:
	NoisVstControllerParameterImpl()
		: mParameter(new NoisVstParameter<Param>())
	{
	}

	Vst::ParamID GetPid() const override final
	{
		return Param::kPid;
	}

	operator Vst::Parameter*() override final
	{
		return mParameter;
	}

private:
	Vst::Parameter* mParameter;
};

template<typename Param>
nois::Own_t<NoisVstProcessorParameter> NoisVstProcessorParameter::Create(nois::FloatParameterRegistry& registry)
{
	return nois::MakeOwn<NoisVstProcessorParameterImpl<Param>>(registry);
}

template<typename Param>
nois::Own_t<NoisVstControllerParameter> NoisVstControllerParameter::Create()
{
	return nois::MakeOwn<NoisVstControllerParameterImpl<Param>>();
}
