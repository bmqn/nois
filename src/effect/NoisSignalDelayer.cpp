#include "nois/effect/NoisSignalDelayer.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class SignalDelayerImpl : public SignalDelayer
{
public:
	
	SignalDelayerImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_DelayMs(MakeRef<FloatConstantParameter>(0.0f))
		, m_Samples(0)
		, m_SampleRate(0)
	{
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		m_Stream->Consume(buffer, sampleRate);

		for (count_t i = 0; i < buffer.GetNumFrames(); ++i)
		{
			buffer.SetStereoSample(i, ConsumeSample(buffer.GetStereoSample(i)));
		}

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

		if (m_SampleRate != sampleRate ||
			m_DelayMs->Changed(0))
		{
			const f32_t delayMs = m_DelayMs->Get(0);

			count_t delaySamples = static_cast<count_t>((delayMs * sampleRate) / 1000.0f);

			m_Samples.Resize(delaySamples);
	
			m_SampleRate = sampleRate;
		}
	}

	virtual Ref_t<FloatParameter> GetDelayMs() override
	{
		return m_DelayMs;
	}

	virtual void SetDelayMs(Ref_t<FloatParameter> delayMs) override
	{
		m_DelayMs = delayMs;
	}

private:
	FloatStereoSample ConsumeSample(FloatStereoSample input)
	{
		FloatStereoSample output = input;

		if (m_Samples.GetSize() > 0)
		{
			output = m_Samples.GetOldest();

			m_Samples.Add(input);
		}

		return output;
	}

private:
	Ref_t<Stream> m_Stream;

	// TODO: make me a block-based parameter
	Ref_t<FloatParameter> m_DelayMs;

	WindowStream<FloatStereoSample> m_Samples;

	s32_t m_SampleRate;
};

Ref_t<SignalDelayer> CreateSignalDelayer(Ref_t<Stream> stream)
{
	return MakeRef<SignalDelayerImpl>(stream);
}

}
