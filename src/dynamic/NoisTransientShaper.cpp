#include "nois/dynamic/NoisTransientShaper.hpp"

#include "nois/effect/NoisFilter.hpp"

namespace nois {

class TransientShaper::Impl
{
public:
	Impl()
	{
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer)
	{
		NOIS_PROFILE_SCOPE();

		f32_t attackRatio = m_AttackRatio.Get();
		f32_t sustainRatio = m_SustainRatio.Get();
		f32_t attackTau = 1000.0f / m_AttackMs.Get();
		f32_t releaseTau = 1000.0f / m_ReleaseMs.Get();
		f32_t smoothing = m_Smoothing.Get();

		f32_t inverseSampleRate = 1.0f / m_SampleRate;
		f32_t attackFactor = 1.0f - std::exp(-1.0f * attackTau * inverseSampleRate);
		f32_t releaseFactor = 1.0f - std::exp(-1.0f * releaseTau * inverseSampleRate);

		for (count_t f = 0; f < m_NumFrames; f += m_NumChannels)
		{
			f32_t signal = 0.0f;

			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				signal = std::max(signal, std::abs(inBuffer(f, c)));
			}

			f32_t signalDb = ToDb(signal);
			f32_t envMultiplier = (signalDb > m_EnvelopeDb) ? attackFactor : releaseFactor;
			m_EnvelopeDb += (signalDb - m_EnvelopeDb) * envMultiplier;
			m_EnvelopeDb = std::clamp(m_EnvelopeDb, -36.0f, 24.0f);
			f32_t transientDb = signalDb - m_EnvelopeDb;
			transientDb = std::clamp(transientDb, -36.0f, 24.0f);
			f32_t targetGain = (transientDb > 0.0f) ? FromDb(transientDb * attackRatio) : FromDb(transientDb * sustainRatio);
			m_Gain += (targetGain - m_Gain) * (1.0f - smoothing);

			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				outBuffer(f, c) *= m_Gain;
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
		m_SampleRate = sampleRate;
	}

	void SetAttackRatio(Ref_t<FloatBlockParameter> attackRatio)
	{
		m_AttackRatio.Use(attackRatio);
	}

	void SetSustainRatio(Ref_t<FloatBlockParameter> sustainRatio)
	{
		m_SustainRatio.Use(sustainRatio);
	}

	void SetAttackMs(Ref_t<FloatBlockParameter> attackMs)
	{
		m_AttackMs.Use(attackMs);
	}

	void SetReleaseMs(Ref_t<FloatBlockParameter> releaseMs)
	{
		m_ReleaseMs.Use(releaseMs);
	}

	void SetSmoothing(Ref_t<FloatBlockParameter> smoothing)
	{
		m_Smoothing.Use(smoothing);
	}

private:
	FloatSlotBlockParameter m_AttackRatio = 1.0f;
	FloatSlotBlockParameter m_SustainRatio = 1.0f;
	FloatSlotBlockParameter m_AttackMs = 5.0f;
	FloatSlotBlockParameter m_ReleaseMs = 50.0;
	FloatSlotBlockParameter m_Smoothing = 0.0f;

	f32_t m_EnvelopeDb = 0.0f;
	f32_t m_Gain = 1.0f;

	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;
};

class MultibandTransientShaper::Impl
{
public:
	Impl()
	{
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer)
	{
		NOIS_PROFILE_SCOPE();

		f32_t attackRatio = m_AttackRatio.Get();
		f32_t sustainRatio = m_SustainRatio.Get();
		f32_t attackTau = 1000.0f / m_AttackMs.Get();
		f32_t releaseTau = 1000.0f / m_ReleaseMs.Get();
		f32_t smoothing = m_Smoothing.Get();

		f32_t inverseSampleRate = 1.0f / m_SampleRate;
		f32_t attackFactor = 1.0f - std::exp(-1.0f * attackTau * inverseSampleRate);
		f32_t releaseFactor = 1.0f - std::exp(-1.0f * releaseTau * inverseSampleRate);

		for (count_t i = 0; i < 3; ++i)
		{
			f32_t& envelopeDb = m_EnvelopeDbs[i];
			f32_t& gain = m_Gains[i];
			Biquad<f32_t>& biquad = m_Biquads[i];

			for (count_t f = 0; f < m_NumFrames; f += m_NumChannels)
			{
				f32_t signal = 0.0f;

				for (count_t c = 0; c < m_NumChannels; ++c)
				{
					signal = std::max(signal, std::abs(biquad.Process(inBuffer(f, c), c)));
				}

				f32_t signalDb = ToDb(signal);
				f32_t envMultiplier = (signalDb > envelopeDb) ? attackFactor : releaseFactor;
				envelopeDb += (signalDb - envelopeDb) * envMultiplier;
				envelopeDb = std::clamp(envelopeDb, -36.0f, 24.0f);
				f32_t transientDb = signalDb - envelopeDb;
				transientDb = std::clamp(transientDb, -36.0f, 24.0f);
				f32_t targetGain = (transientDb > 0.0f) ? FromDb(transientDb * attackRatio) : FromDb(transientDb * sustainRatio);
				gain += (targetGain - gain) * (1.0f - smoothing);

				for (count_t c = 0; c < m_NumChannels; ++c)
				{
					outBuffer(f, c) *= gain;
				}
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

		if (m_LowCutoffRatio.Changed())
		{
			m_Biquads[0].MakeButterworthLow(m_LowCutoffRatio.Get());
		}

		if (m_HighCutoffRatio.Changed())
		{
			m_Biquads[1].MakeButterworthHigh(m_HighCutoffRatio.Get());
		}

		if (m_BandCutoffRatio.Changed())
		{
			m_Biquads[2].MakeBandpass(m_BandCutoffRatio.Get(), 2.7f);
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_SampleRate = sampleRate;
	}

	void SetAttackRatio(Ref_t<FloatBlockParameter> attackRatio)
	{
		m_AttackRatio.Use(attackRatio);
	}

	void SetSustainRatio(Ref_t<FloatBlockParameter> sustainRatio)
	{
		m_SustainRatio.Use(sustainRatio);
	}

	void SetAttackMs(Ref_t<FloatBlockParameter> attackMs)
	{
		m_AttackMs.Use(attackMs);
	}

	void SetReleaseMs(Ref_t<FloatBlockParameter> releaseMs)
	{
		m_ReleaseMs.Use(releaseMs);
	}

	void SetSmoothing(Ref_t<FloatBlockParameter> smoothing)
	{
		m_Smoothing.Use(smoothing);
	}

	void SetLowCutoffRatio(Ref_t<FloatBlockParameter> lowCutoffRatio)
	{
		m_LowCutoffRatio.Use(lowCutoffRatio);
	}

	void SetHighCutoffRatio(Ref_t<FloatBlockParameter> highCutoffRatio)
	{
		m_HighCutoffRatio.Use(highCutoffRatio);
	}

	void SetBandCutoffRatio(Ref_t<FloatBlockParameter> bandCutoffRatio)
	{
		m_BandCutoffRatio.Use(bandCutoffRatio);
	}

private:
	FloatSlotBlockParameter m_AttackRatio = 1.0f;
	FloatSlotBlockParameter m_SustainRatio = 1.0f;
	FloatSlotBlockParameter m_AttackMs = 5.0f;
	FloatSlotBlockParameter m_ReleaseMs = 50.0;
	FloatSlotBlockParameter m_Smoothing = 0.0f;
	FloatSlotBlockParameter m_LowCutoffRatio = 0.006f;
	FloatSlotBlockParameter m_HighCutoffRatio = 0.1f;
	FloatSlotBlockParameter m_BandCutoffRatio = 0.47f;

	std::array<f32_t, 3> m_EnvelopeDbs = { 0.0f, 0.0f, 0.0f };
	std::array<f32_t, 3> m_Gains = { 1.0f, 1.0f, 1.0f };
	std::array<Biquad<f32_t>, 3> m_Biquads;

	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;
};

Ref_t<TransientShaper> TransientShaper::Create()
{
	return MakeRef<TransientShaper>(MakeOwn<TransientShaper::Impl>());
}

Ref_t<MultibandTransientShaper> MultibandTransientShaper::Create()
{
	return MakeRef<MultibandTransientShaper>(MakeOwn<MultibandTransientShaper::Impl>());
}

}
