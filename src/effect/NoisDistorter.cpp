#include "nois/effect/NoisDistorter.hpp"

#include "nois/util/NoisSmallVector.hpp"

namespace nois {

class Distorter::Impl
{
public:
	NOIS_INTERFACE_IMPL_MULTI()
	NOIS_INTERFACE_IMPL_MULTI_PARAM(DriveDb, FloatParameter)
	NOIS_INTERFACE_IMPL_MULTI_PARAM(MakeupDb, FloatParameter)
	NOIS_INTERFACE_IMPL_MULTI_PARAM(Bias, FloatParameter)
	NOIS_INTERFACE_IMPL_MULTI_PARAM(Wet, FloatParameter)
};

class TanhDistorterImpl : public Distorter::Impl
{
public:
	TanhDistorterImpl()
	{
	}

	Stream::Result Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer) override final
	{
		NOIS_PROFILE_SCOPE();

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t f = 0; f < m_NumFrames; ++f)
			{
				f32_t x = inBuffer(f, c);
				f32_t& y = outBuffer(f, c);

				f32_t drive = FromDb(m_DriveDb.Get(f));
				f32_t makeup = FromDb(m_MakeupDb.Get(f));
				f32_t bias = m_Bias.Get(f);
				f32_t wet = m_Wet.Get(f);

				if (wet == 0.0f)
				{
					y = x;
				}
				else
				{
					f32_t z = makeup * std::tanh(drive * (x + bias));
					y = x * (1.0f - wet) + z * wet;
				}
			}
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE();

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetDriveDb(Ref_t<FloatParameter> driveDb) override final
	{
		m_DriveDb.Use(driveDb);
	}

	void SetMakeupDb(Ref_t<FloatParameter> makeupDb) override final
	{
		m_MakeupDb.Use(makeupDb);
	}

	void SetBias(Ref_t<FloatParameter> bias) override final
	{
		m_Bias.Use(bias);
	}

	void SetWet(Ref_t<FloatParameter> wet) override final
	{
		m_Wet.Use(wet);
	}

private:
	SlotParameter<f32_t> m_DriveDb = 0.0f;
	SlotParameter<f32_t> m_MakeupDb = 0.0f;
	SlotParameter<f32_t> m_Bias = 0.0f;
	SlotParameter<f32_t> m_Wet = 0.0f;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
};

NOIS_INTERFACE_IMPL(Distorter)
NOIS_INTERFACE_PARAM_IMPL(Distorter, DriveDb, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(Distorter, MakeupDb, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(Distorter, Bias, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(Distorter, Wet, FloatParameter)

Ref_t<Distorter> Distorter::Create(Kind kind)
{
	switch (kind)
	{
	case nois::Distorter::k_Tanh:
		return MakeRef<Distorter>(MakeOwn<TanhDistorterImpl>());
	default:
		break;
	}

	return nullptr;
}

}

