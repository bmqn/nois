#include "nois/dynamic/NoisExpander.hpp"

namespace nois {

class Expander::Impl
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

		f32_t thresholdDb = m_ThresholdDb.Get();
		f32_t ratio = m_Ratio.Get();
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
			f32_t underDb = m_EnvelopeDb - thresholdDb;
			f32_t targetGain = FromDb(underDb * ratio);
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

	void SetRatio(Ref_t<FloatBlockParameter> ratio)
	{
		m_Ratio.Use(ratio);
	}

	void SetThresholdDb(Ref_t<FloatBlockParameter> thesholdDb)
	{
		m_ThresholdDb.Use(thesholdDb);
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
	FloatSlotBlockParameter m_ThresholdDb = 0.0f;
	FloatSlotBlockParameter m_Ratio = 1.0f;
	FloatSlotBlockParameter m_AttackMs = 5.0f;
	FloatSlotBlockParameter m_ReleaseMs = 50.0f;
	FloatSlotBlockParameter m_Smoothing = 0.0f;

	f32_t m_EnvelopeDb = 0.0f;
	f32_t m_Gain = 1.0f;

	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;
};

NOIS_INTERFACE_IMPL(Expander)
NOIS_INTERFACE_PARAM_IMPL(Expander, Ratio, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Expander, ThresholdDb, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Expander, AttackMs, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Expander, ReleaseMs, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Expander, Smoothing, FloatBlockParameter)

Ref_t<Expander> Expander::Create()
{
	return MakeRef<Expander>(MakeOwn<Expander::Impl>());
}

}
