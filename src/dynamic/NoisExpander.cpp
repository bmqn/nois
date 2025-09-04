#include "nois/dynamic/NoisExpander.hpp"

namespace nois {

class ExpanderImpl : public Expander
{
public:
	ExpanderImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_ThresholdDb(MakeRef<FloatConstantParameter>(-12.0f))
		, m_Ratio(MakeRef<FloatConstantParameter>(1.25f))
		, m_AttackMs(MakeRef<FloatConstantParameter>(10.0f))
		, m_ReleaseMs(MakeRef<FloatConstantParameter>(10.0f))
		, m_Smoothing(MakeRef<FloatConstantParameter>(0.8f))
	{
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		Stream::Result result = Stream::Success;

		if (Stream::Result streamResult =
			m_Stream->Consume(
				buffer,
				sampleRate);
			streamResult != Stream::Success)
		{
			result = streamResult;
		}

		for (count_t i = 0; i < buffer.GetNumFrames(); i += buffer.GetNumChannels())
		{
			constexpr float k_DcOffset = 0.005f;

			f32_t attackTau = 1000.0f / m_AttackMs->Get(i);
			f32_t releaseTau = 1000.0f / m_ReleaseMs->Get(i);

			f32_t inverseSampleRate = 1.0f / sampleRate;

			float attackFactor = 1.0f - std::exp(-1.0f * attackTau * inverseSampleRate);
			float releaseFactor = 1.0f - std::exp(-1.0f * releaseTau * inverseSampleRate);

			float signal = 0.0f;

			for (count_t j = 0; j < buffer.GetNumChannels(); ++j)
			{
				signal = std::max(signal, std::abs(buffer(i, j)));
			}

			signal += k_DcOffset;

			float signalDb = ToDb(signal);
			float envMultiplier = (signalDb > m_EnvelopeDb) ? attackFactor : releaseFactor;
			m_EnvelopeDb += (signalDb - m_EnvelopeDb) * envMultiplier;
			m_EnvelopeDb = std::clamp(m_EnvelopeDb, -36.0f, 24.0f);

			float underDb = m_EnvelopeDb - m_ThresholdDb->Get(i);
			float targetGain = FromDb(underDb * m_Ratio->Get(i));

			m_Gain += (targetGain - m_Gain) * m_Smoothing->Get(i);

			for (count_t j = 0; j < buffer.GetNumChannels(); ++j)
			{
				buffer(i, j) *= m_Gain;
			}
		}

		return result;
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

	virtual Ref_t<FloatParameter> GetRatio() const override
	{
		return m_Ratio;
	}

	virtual void SetRatio(Ref_t<FloatParameter> ratio) override
	{
		m_Ratio = ratio;
	}

	virtual Ref_t<FloatParameter> GetThresholdDb() const override
	{
		return m_ThresholdDb;
	}

	virtual void SetThresholdDb(Ref_t<FloatParameter> thesholdDb) override
	{
		m_ThresholdDb = thesholdDb;
	}

	virtual Ref_t<FloatParameter> GetAttackMs() const override
	{
		return m_AttackMs;
	}

	virtual void SetAttackMs(Ref_t<FloatParameter> attackMs) override
	{
		m_AttackMs = attackMs;
	}

	virtual Ref_t<FloatParameter> GetReleaseMs() const override
	{
		return m_ReleaseMs;
	}

	virtual void SetReleaseMs(Ref_t<FloatParameter> releaseMs) override
	{
		m_ReleaseMs = releaseMs;
	}

	virtual Ref_t<FloatParameter> GetSmoothing() const override
	{
		return m_Smoothing;
	}

	virtual void SetSmoothing(Ref_t<FloatParameter> smoothing) override
	{
		m_Smoothing = smoothing;
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_ThresholdDb;
	Ref_t<FloatParameter> m_Ratio;
	Ref_t<FloatParameter> m_AttackMs;
	Ref_t<FloatParameter> m_ReleaseMs;
	Ref_t<FloatParameter> m_Smoothing;

	float m_EnvelopeDb = 0.0f;
	float m_Gain = 1.0f;
};

Ref_t<Expander> CreateExpander(Ref_t<Stream> stream)
{
	return MakeRef<ExpanderImpl>(stream);
}

}
