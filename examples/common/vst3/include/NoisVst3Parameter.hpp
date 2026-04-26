#pragma once

#include <nois/Nois.hpp>

#include <base/source/fstring.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <pluginterfaces/base/ustring.h>
#include <public.sdk/source/vst/vstparameters.h>

#include <optional>

using namespace Steinberg;

class NoisVstProcessorParameter
{
public:
	virtual ~NoisVstProcessorParameter() {}

	virtual Vst::ParamID GetPid() const = 0;

	virtual Vst::ParamValue ApplyStep(Vst::ParamValue valuePlain) const = 0;
	virtual Vst::ParamValue ToPlain(Vst::ParamValue valueNormalized) const = 0;
	virtual Vst::ParamValue ToNormalized(Vst::ParamValue valuePlain) const = 0;

	virtual void Setup(nois::count_t numFrames, nois::f32_t sampleRate) = 0;

	virtual void WritePlain(nois::count_t frame, nois::f32_t valuePlain) = 0;
	virtual void RequestPlain(nois::f32_t valuePlain) = 0;
	virtual nois::f32_t GetLastPlain() const = 0;

	virtual operator nois::Ref_t<nois::FloatParameter>() = 0;
	virtual operator nois::Ref_t<nois::FloatBlockParameter>() = 0;

	template<typename F>
	nois::Ref_t<nois::FloatParameter> Transform(F&& transformer)
	{
		return operator nois::Ref_t<nois::FloatParameter>()->Transform(std::move(transformer));
	}

	template<typename F>
	nois::Ref_t<nois::FloatBlockParameter> TransformBlock(F&& transformer)
	{
		return operator nois::Ref_t<nois::FloatBlockParameter>()->Transform(std::move(transformer));
	}

public:
	template<typename Param>
	static nois::Own_t<NoisVstProcessorParameter> Create(nois::FloatParameterRegistry& registry);
};

class NoisVstControllerParameter
{
public:
	virtual ~NoisVstControllerParameter() {}

	virtual Vst::ParamID GetPid() const = 0;

	virtual void RequestPlain(nois::f32_t valuePlain) = 0;
	virtual nois::f32_t GetLastPlain() const = 0;

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
	valuePlain = std::clamp(valuePlain, Param::kMinValue, Param::kMaxValue);
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
	valueNormalized = std::clamp(valueNormalized, 0.0f, 1.0f);
	return ApplyStep<Param>(valueNormalized * (Param::kMaxValue - Param::kMinValue) + Param::kMinValue);
}

template<typename Param>
constexpr inline nois::f32_t ToNormalized(nois::f32_t valuePlain)
{
	valuePlain = std::clamp(valuePlain, Param::kMinValue, Param::kMaxValue);
	return (ApplyStep<Param>(valuePlain) - Param::kMinValue) / (Param::kMaxValue - Param::kMinValue);
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

	Vst::ParamValue toPlain(Vst::ParamValue valueNormalized) const override final
	{
		return util::ToPlain<Param>(valueNormalized);
	}

	Vst::ParamValue toNormalized(Vst::ParamValue valuePlain) const override final
	{
		return util::ToNormalized<Param>(valuePlain);
	}

	void toString(Vst::ParamValue valueNormalized, Vst::String128 string) const override final
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

	bool fromString(const Vst::TChar* string, Vst::ParamValue& valueNormalized) const override final
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
	template<typename, typename>
	friend class NoisVstProcessor;

public:
	NoisVstProcessorParameterImpl(nois::FloatParameterRegistry& registry)
		: mNumFrames(0)
		, mSampleRate(0.0f)
		, mRegistry(registry)
		, mParameter(nullptr)
		, mBlockParameter(nullptr)
		, mNextValue(std::nullopt)
		, mValues()
	{
		mParameter = mRegistry.CreateBinder(
			[this](nois::count_t f)
			{
				return GetValue(f);
			});

		mBlockParameter = mRegistry.CreateBlockBinder(
			[this]()
			{
				return GetBlockValue();
			},
			Param::kMinValue,
			Param::kMaxValue);
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

	void Setup(nois::count_t numFrames, nois::f32_t sampleRate) override final
	{
		mNumFrames = numFrames;
		mSampleRate = sampleRate;

		nois::f32_t valuePlain = mNextValue.value_or(GetLastPlain());
		mNextValue = std::nullopt;

		mValues.resize(numFrames, Param::kDefaultValue);
		std::fill(mValues.begin(), mValues.end(), valuePlain);
	}

	void WritePlain(nois::count_t f, nois::f32_t valuePlain) override final
	{
		if (f < mValues.size())
		{
			mValues[f] = valuePlain;
		}
	}

	void RequestPlain(nois::f32_t valuePlain) override final
	{
		mNextValue = valuePlain;
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
	nois::f32_t GetValue(nois::count_t f) const
	{
		return mValues[f];
	}

	nois::f32_t GetBlockValue() const
	{
		return mValues.back();
	}

private:
	nois::count_t mNumFrames;
	nois::f32_t mSampleRate;
	nois::FloatParameterRegistry& mRegistry;
	nois::Ref_t<nois::FloatParameter> mParameter;
	nois::Ref_t<nois::FloatBlockParameter> mBlockParameter;
	std::optional<nois::f32_t> mNextValue;
	nois::SmallVector<nois::f32_t, nois::k_MaxNumInplaceFrames> mValues;
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

	void RequestPlain(nois::f32_t valuePlain) override final
	{
		mParameter->setNormalized(util::ToNormalized<Param>(valuePlain));
	}

	nois::f32_t GetLastPlain() const override final
	{
		return util::ToPlain<Param>(mParameter->getNormalized());
	}

	operator Vst::Parameter*() override final
	{
		return mParameter;
	}

private:
	Vst::Parameter* mParameter;
};

template<typename Param>
auto NoisVstProcessorParameter::Create(nois::FloatParameterRegistry& registry) -> nois::Own_t<NoisVstProcessorParameter>
{
	return nois::MakeOwn<NoisVstProcessorParameterImpl<Param>>(registry);
}

template<typename Param>
auto NoisVstControllerParameter::Create() -> nois::Own_t<NoisVstControllerParameter>
{
	return nois::MakeOwn<NoisVstControllerParameterImpl<Param>>();
}
