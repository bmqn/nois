#include "nois/dynamic/NoisCompressor.hpp"

namespace nois {

class Compressor::Impl
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

		f32_t ratio = m_Ratio.Get();

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			f32_t signal = 0.0f;

			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				signal = std::max(signal, std::abs(inBuffer(f, c)));
			}

			f32_t envelope = (signal > m_Envelope) ? m_AttackFactor : m_ReleaseFactor;
			m_Envelope += (signal - m_Envelope) * envelope;
			
			f32_t gain = 1.0f;

			if (m_Envelope > m_Threshold)
			{
				gain = std::pow(m_Threshold / m_Envelope, 1.0f - 1.0f / ratio);
			}

			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				outBuffer(f, c) = inBuffer(f, c) * gain;
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
		
		if (m_ThresholdDb.PollChanged())
		{
			m_Threshold = FromDb(m_ThresholdDb.Get());
		}
		
		if (m_AttackMs.PollChanged())
		{
			m_AttackFactor = 1.0f - std::exp(-1.0f / (m_AttackMs.Get() * 0.001f * sampleRate));
		}
		
		if (m_ReleaseMs.PollChanged())
		{
			m_ReleaseFactor= 1.0f - std::exp(-1.0f / (m_ReleaseMs.Get() * 0.001f * sampleRate));
		}

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

private:
	FloatSlotBlockParameter m_ThresholdDb = { 0.0f, -96.0f, 0.0f };
	FloatSlotBlockParameter m_Ratio = { 1.0f, 1.0f, 16.0f };
	FloatSlotBlockParameter m_AttackMs = { 5.0f, 0.001f, 100.0f };
	FloatSlotBlockParameter m_ReleaseMs = { 50.0f, 1.0f, 500.0f };

	f32_t m_Threshold = 0.0f;
	f32_t m_AttackFactor = 1.0f;
	f32_t m_ReleaseFactor = 1.0f;
	f32_t m_Envelope = 0.0f;

	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;
};

NOIS_INTERFACE_IMPL(Compressor)
NOIS_INTERFACE_PARAM_IMPL(Compressor, Ratio, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Compressor, ThresholdDb, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Compressor, AttackMs, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Compressor, ReleaseMs, FloatBlockParameter)

Ref_t<Compressor> Compressor::Create()
{
	return MakeRef<Compressor>(MakeOwn<Compressor::Impl>());
}

}
