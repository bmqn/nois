#include "nois/effect/NoisGainer.hpp"

namespace nois {

class Gainer::Impl
{
public:
	Impl()
		: m_Gain(1.0f)
	{
	}

	Stream::Result Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		outBuffer.Copy(inBuffer);
		outBuffer.Multiply(m_Gain);

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
	}

	void SetGain(Ref_t<FloatParameter> gainDb)
	{
		m_Gain.Use(gainDb);
	}

private:
	FloatSlotParameter m_Gain;
};

NOIS_INTERFACE_IMPL(Gainer)
NOIS_INTERFACE_PARAM_IMPL(Gainer, Gain, FloatParameter)

Ref_t<Gainer> Gainer::Create()
{
	return MakeRef<Gainer>(MakeOwn<Gainer::Impl>());
}

}

