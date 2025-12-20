#include "nois/dynamic/NoisTransientShaper.hpp"

#include "nois/effect/NoisFilter.hpp"

namespace nois {

class TransientShaperImpl : public TransientShaper
{
public:
	TransientShaperImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_AttackRatio(MakeRef<FloatConstantParameter>(0.0f))
		, m_SustainRatio(MakeRef<FloatConstantParameter>(0.0f))
		, m_AttackMs(MakeRef<FloatConstantParameter>(50.0f))
		, m_ReleaseMs(MakeRef<FloatConstantParameter>(100.0f))
		, m_Smoothing(MakeRef<FloatConstantParameter>(0.0f))
	{
	}

	virtual ~TransientShaperImpl()
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
			constexpr f32_t k_DcOffset = 0.005f;

			f32_t attackTau = 1000.0f / m_AttackMs->Get(i);
			f32_t releaseTau = 1000.0f / m_ReleaseMs->Get(i);

			f32_t inverseSampleRate = 1.0f / sampleRate;

			f32_t attackFactor = 1.0f - std::exp(-1.0f * attackTau * inverseSampleRate);
			f32_t releaseFactor = 1.0f - std::exp(-1.0f * releaseTau * inverseSampleRate);

			f32_t signal = 0.0f;

			for (count_t j = 0; j < buffer.GetNumChannels(); ++j)
			{
				signal = std::max(signal, std::abs(buffer(i, j)));
			}

			signal += k_DcOffset;

			f32_t signalDb = ToDb(signal);
			f32_t envMultiplier = (signalDb > m_EnvelopeDb) ? attackFactor : releaseFactor;
			m_EnvelopeDb += (signalDb - m_EnvelopeDb) * envMultiplier;
			m_EnvelopeDb = std::clamp(m_EnvelopeDb, -36.0f, 24.0f);

			f32_t transientDb = signalDb - m_EnvelopeDb;
			transientDb = std::clamp(transientDb, -36.0f, 24.0f);
			f32_t targetGain = (transientDb > 0.0f) ? FromDb(transientDb * m_AttackRatio->Get(i)) : FromDb(transientDb * m_SustainRatio->Get(i));

			m_Gain += (targetGain - m_Gain) * (1.0f - m_Smoothing->Get(i));

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

	virtual Ref_t<FloatParameter> GetAttackRatio() const override
	{
		return m_AttackRatio;
	}

	virtual void SetAttackRatio(Ref_t<FloatParameter> attackRatio) override
	{
		m_AttackRatio = attackRatio;
	}

	virtual Ref_t<FloatParameter> GetSustainRatio() const override
	{
		return m_SustainRatio;
	}

	virtual void SetSustainRatio(Ref_t<FloatParameter> sustainRatio) override
	{
		m_SustainRatio = sustainRatio;
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

	virtual float GetGain() const override
	{
		return m_Gain;
	}

protected:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_AttackRatio;
	Ref_t<FloatParameter> m_SustainRatio;
	Ref_t<FloatParameter> m_AttackMs;
	Ref_t<FloatParameter> m_ReleaseMs;
	Ref_t<FloatParameter> m_Smoothing;

	f32_t m_EnvelopeDb = 0.0f;
	f32_t m_Gain = 1.0f;
};

class MultibandTransientShaperImpl : public MultibandTransientShaper
{
public:
	MultibandTransientShaperImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_AttackRatio(MakeRef<FloatConstantParameter>(1.0f))
		, m_SustainRatio(MakeRef<FloatConstantParameter>(1.0f))
		, m_AttackMs(MakeRef<FloatConstantParameter>(10.0f))
		, m_ReleaseMs(MakeRef<FloatConstantParameter>(10.0f))
		, m_Smoothing(MakeRef<FloatConstantParameter>(0.8f))
	{
		m_LowFilter = Filter::Create(m_Stream, Filter::k_N2ButterworthLow);
		m_HighFilter = Filter::Create(m_Stream, Filter::k_N2ButterworthHigh);
		m_BandFilter = CreateBandpassFilter(m_Stream, BandpassFilter::k_Biquad);

		m_LowTransientShaper = MakeRef<TransientShaperImpl>(m_LowFilter);
		m_HighTransientShaper = MakeRef<TransientShaperImpl>(m_HighFilter);
		m_BandTransientShaper = MakeRef<TransientShaperImpl>(m_BandFilter);

		m_LowFilter->SetCutoffRatio(CreateParameter(0.03f));
		m_HighFilter->SetCutoffRatio(CreateParameter(0.518f));
		m_BandFilter->SetCutoffRatio(CreateParameter(0.075f));
		m_BandFilter->SetQ(CreateParameter(0.67f));
	}

	virtual Stream::Result Consume(
		FloatBuffer& buffer,
		f32_t sampleRate) override
	{
		/*m_Stream->Consume(buffer, sampleRate);

		m_Scratch2.Fill(0.0f);
		m_LowFilter->Consume(m_Scratch2, sampleRate);
		m_Scratch3.Fill(0.0f);
		m_LowTransientShaper->Consume(m_Scratch3, sampleRate);
		m_Scratch3.Subtract(m_Scratch2);
		buffer.Add(m_Scratch3);

		m_Scratch2.Fill(0.0f);
		m_HighFilter->Consume(m_Scratch2, sampleRate);
		m_Scratch3.Fill(0.0f);
		m_HighTransientShaper->Consume(m_Scratch3, sampleRate);
		m_Scratch3.Subtract(m_Scratch2);
		buffer.Add(m_Scratch3);

		m_Scratch2.Fill(0.0f);
		m_BandFilter->Consume(m_Scratch2, sampleRate);
		m_Scratch3.Fill(0.0f);
		m_BandTransientShaper->Consume(m_Scratch3, sampleRate);
		m_Scratch3.Subtract(m_Scratch2);
		buffer.Add(m_Scratch3);*/

		m_LowTransientShaper->Consume(m_Scratch1, sampleRate);
		buffer.Add(m_Scratch1);
		m_HighTransientShaper->Consume(m_Scratch1, sampleRate);
		buffer.Add(m_Scratch1);
		m_BandTransientShaper->Consume(m_Scratch1, sampleRate);
		buffer.Add(m_Scratch1);

		return Stream::Success;
	}

	virtual void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override
	{
		m_Scratch1.Resize(
			numFrames,
			numChannels);
		m_Scratch2.Resize(
			numFrames,
			numChannels);
		m_Scratch3.Resize(
			numFrames,
			numChannels);

		m_Stream->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);

		m_LowFilter->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
		m_HighFilter->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
		m_BandFilter->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);

		m_LowTransientShaper->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
		m_HighTransientShaper->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
		m_BandTransientShaper->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
	}

	virtual Ref_t<FloatParameter> GetAttackRatio() const override
	{
		return m_AttackRatio;
	}

	virtual void SetAttackRatio(Ref_t<FloatParameter> attackRatio) override
	{
		m_AttackRatio = attackRatio;

		m_HighTransientShaper->SetAttackRatio(m_AttackRatio);
	}

	virtual Ref_t<FloatParameter> GetSustainRatio() const override
	{
		return m_SustainRatio;
	}

	virtual void SetSustainRatio(Ref_t<FloatParameter> sustainRatio) override
	{
		m_SustainRatio = sustainRatio;

		m_HighTransientShaper->SetSustainRatio(m_SustainRatio);
	}

	virtual Ref_t<FloatParameter> GetAttackMs() const override
	{
		return m_AttackMs;
	}

	virtual void SetAttackMs(Ref_t<FloatParameter> attackMs) override
	{
		m_AttackMs = attackMs;

		m_HighTransientShaper->SetAttackMs(m_AttackMs);
	}

	virtual Ref_t<FloatParameter> GetReleaseMs() const override
	{
		return m_ReleaseMs;
	}

	virtual void SetReleaseMs(Ref_t<FloatParameter> releaseMs) override
	{
		m_ReleaseMs = releaseMs;

		m_HighTransientShaper->SetReleaseMs(m_ReleaseMs);
	}

	virtual Ref_t<FloatParameter> GetSmoothing() const override
	{
		return m_Smoothing;
	}

	virtual void SetSmoothing(Ref_t<FloatParameter> smoothing) override
	{
		m_Smoothing = smoothing;

		m_HighTransientShaper->SetSmoothing(m_Smoothing);
	}

	virtual void SetLowCutoffRatio(Ref_t<FloatParameter> cutoffRatio) override
	{
		m_LowFilter->SetCutoffRatio(cutoffRatio);
	}

	virtual void SetHighCutoffRatio(Ref_t<FloatParameter> cutoffRatio) override
	{
		m_HighFilter->SetCutoffRatio(cutoffRatio);
	}

	virtual void SetBandCutoffRatio(Ref_t<FloatParameter> cutoffRatio) override
	{
		m_BandFilter->SetCutoffRatio(cutoffRatio);
	}

	virtual f32_t GetLowGain() const override
	{
		return m_LowTransientShaper->GetGain();
	}

	virtual f32_t GetHighGain() const override
	{
		return m_HighTransientShaper->GetGain();
	}

	virtual f32_t GetBandGain() const override
	{
		return m_BandTransientShaper->GetGain();
	}

	virtual f32_t GetLowReponseMagnitude(f32_t freqRatio) const override
	{
		return m_LowFilter->GetResponseMagnitude(freqRatio);
	}

	virtual f32_t GetHighReponseMagnitude(f32_t freqRatio) const override
	{
		return m_HighFilter->GetResponseMagnitude(freqRatio);
	}

	virtual f32_t GetBandReponseMagnitude(f32_t freqRatio) const override
	{
		return m_BandFilter->GetResponseMagnitude(freqRatio);
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_AttackRatio;
	Ref_t<FloatParameter> m_SustainRatio;
	Ref_t<FloatParameter> m_AttackMs;
	Ref_t<FloatParameter> m_ReleaseMs;
	Ref_t<FloatParameter> m_Smoothing;

	Ref_t<TransientShaper> m_LowTransientShaper;
	Ref_t<TransientShaper> m_HighTransientShaper;
	Ref_t<TransientShaper> m_BandTransientShaper;

	Ref_t<nois::Filter> m_LowFilter = nullptr;
	Ref_t<nois::Filter> m_HighFilter = nullptr;
	Ref_t<nois::BandpassFilter> m_BandFilter = nullptr;

	FloatBuffer m_Scratch1;
	FloatBuffer m_Scratch2;
	FloatBuffer m_Scratch3;
};

Ref_t<TransientShaper> CreateTransientShaper(
	Ref_t<Stream> stream)
{
	return MakeRef<TransientShaperImpl>(stream);
}

Ref_t<MultibandTransientShaper> CreateMultibandTransientShaper(
	Ref_t<Stream> stream)
{
	return MakeRef<MultibandTransientShaperImpl>(stream);
}

}
