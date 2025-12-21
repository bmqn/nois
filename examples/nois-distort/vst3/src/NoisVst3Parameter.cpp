#include "NoisVst3Parameter.hpp"

#include <base/source/fstring.h>
#include <pluginterfaces/base/ustring.h>

using namespace Steinberg;

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
constexpr inline Vst::ParameterInfo MakeParameterInfo()
{
	Vst::ParameterInfo info;
	ConstString title(Param::kTitle);
	ConstString units(Param::kUnits);

	info.id = Param::kPid;
	title.copyTo(info.title, 0, title.length());
	title.copyTo(info.shortTitle, 0, title.length());
	units.copyTo(info.units, 0, units.length());
	info.stepCount = 0;
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
					mValues.front(),
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

namespace parameter
{

struct SubFreq
{
	static constexpr const char* kTitle = "Sub Frequency";
	static constexpr const char* kUnits = "Hz";
	static constexpr Vst::ParamID kPid = kSubFreq;
	static constexpr nois::f32_t kDefaultValue = 80.0f;
	static constexpr nois::f32_t kMinValue = 50.0f;
	static constexpr nois::f32_t kMaxValue = 140.0f;
	static constexpr nois::s32_t kNumSteps = 0;

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value / (sampleRate * 0.5f);
	}
};

struct Drive
{
	static constexpr const char* kTitle = "Drive";
	static constexpr const char* kUnits = "dB";
	static constexpr Vst::ParamID kPid = kDrive;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = 1.0f;
	static constexpr nois::f32_t kMaxValue = 16.0f;
	static constexpr nois::s32_t kNumSteps = 0;

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value;
	}
};

struct Wet
{
	static constexpr const char* kTitle = "Wet";
	static constexpr const char* kUnits = "";
	static constexpr Vst::ParamID kPid = kWet;
	static constexpr nois::f32_t kDefaultValue = 1.0f;
	static constexpr nois::f32_t kMinValue = 0.0f;
	static constexpr nois::f32_t kMaxValue = 1.0f;
	static constexpr nois::s32_t kNumSteps = 0;

	static nois::f32_t ToProcessor(
		nois::f32_t value,
		nois::count_t numSamples,
		nois::f32_t sampleRate)
	{
		return value;
	}
};

nois::Own_t<NoisVstProcessorParameter> CreateProcessor(Vst::ParamID pid, nois::FloatParameterRegistry& registry)
{
	switch (pid)
	{
		case kSubFreq:
			return nois::MakeOwn<NoisVstProcessorParameterImpl<SubFreq>>(registry);
		case kDrive:
			return nois::MakeOwn<NoisVstProcessorParameterImpl<Drive>>(registry);
		case kWet:
			return nois::MakeOwn<NoisVstProcessorParameterImpl<Wet>>(registry);
		default:
			break;
	}

	return nullptr;
}

nois::Own_t<NoisVstControllerParameter> CreateController(Vst::ParamID pid)
{
	switch (pid)
	{
		case kSubFreq:
			return nois::MakeOwn<NoisVstControllerParameterImpl<SubFreq>>();
		case kDrive:
			return nois::MakeOwn<NoisVstControllerParameterImpl<Drive>>();
		case kWet:
			return nois::MakeOwn<NoisVstControllerParameterImpl<Wet>>();
		default:
			break;
	}

	return nullptr;
}

}
