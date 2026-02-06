#include "nois/effect/NoisDistorter.hpp"

#include "nois/NoisUtil.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {

class DynamicTanhDistorter::Impl
{
public:
	Impl()
	{
	}

	Stream::Result Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		NOIS_PROFILE_SCOPE();

		f32_t drive = FromDb(m_DriveDb.Get());
		f32_t makeup = FromDb(m_MakeupDb.Get());
		f32_t wet = m_Wet.Get();
		f32_t shape = m_Shape.Get();
		f32_t asym = m_Asym.Get();

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t f = 0; f < m_NumFrames; ++f)
			{
				f32_t x = inBuffer(f, c);
				f32_t d = drive * x;
				f32_t s = d > 0.0f ? 1.0f : asym;
				f32_t k = 1.0f + shape;
				f32_t y = FastTanh(d * s * k);
				outBuffer(f, c) = makeup * (x + (y - x) * wet);
			}
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		NOIS_PROFILE_SCOPE();

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetDriveDb(Ref_t<FloatBlockParameter> driveDb)
	{
		m_DriveDb.Use(driveDb);
	}

	void SetMakeupDb(Ref_t<FloatBlockParameter> makeupDb)
	{
		m_MakeupDb.Use(makeupDb);
	}

	void SetWet(Ref_t<FloatBlockParameter> wet)
	{
		m_Wet.Use(wet);
	}

	void SetShape(Ref_t<FloatBlockParameter> shape)
	{
		m_Shape.Use(shape);
	}

	void SetAsym(Ref_t<FloatBlockParameter> asym)
	{
		m_Asym.Use(asym);
	}

private:
	SlotBlockParameter<f32_t> m_DriveDb = { 0.0f, 0.0f, 12.0f };
	SlotBlockParameter<f32_t> m_MakeupDb = { 0.0f, -12.0f, 0.0f };
	SlotBlockParameter<f32_t> m_Wet = { 1.0f, 0.0f, 1.0f };
	SlotBlockParameter<f32_t> m_Shape = { 0.0f, -1.0f, 1.0f };
	SlotBlockParameter<f32_t> m_Asym = { 1.0f, 1.0f, 4.0f };
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
};

NOIS_INTERFACE_IMPL(DynamicTanhDistorter)
NOIS_INTERFACE_PARAM_IMPL(DynamicTanhDistorter, DriveDb, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(DynamicTanhDistorter, MakeupDb, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(DynamicTanhDistorter, Wet, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(DynamicTanhDistorter, Shape, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(DynamicTanhDistorter, Asym, FloatBlockParameter)

Ref_t<Distorter> Distorter::Create(Kind kind)
{
	switch (kind)
	{
	case nois::Distorter::k_DynamicTanh:
		return DynamicTanhDistorter::Create();
	default:
		break;
	}

	return nullptr;
}

Ref_t<DynamicTanhDistorter> DynamicTanhDistorter::Create()
{
	return MakeRef<DynamicTanhDistorter>(MakeOwn<DynamicTanhDistorter::Impl>());
}

}

