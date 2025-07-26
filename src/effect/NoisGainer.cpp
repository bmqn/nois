#include "nois/effect/NoisGainer.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class GainerImpl : public Gainer
{
public:
	
	GainerImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_Gain(MakeRef<FloatConstantParameter>(1.0f))
	{
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		m_Stream->Consume(buffer, sampleRate);

		buffer.Multiply(*m_Gain);

		return Stream::Success;
	}

	virtual void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override
	{
		m_Stream->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
	}

	virtual Ref_t<FloatParameter> GetGain() const override
	{
		return m_Gain;
	}

	virtual void SetGain(Ref_t<FloatParameter> gain) override
	{
		m_Gain = gain;
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_Gain;
};

Ref_t<Gainer> CreateGainer(Ref_t<Stream> stream)
{
	return MakeRef<GainerImpl>(stream);
}

}

