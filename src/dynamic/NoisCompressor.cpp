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

		f32_t thresholdDb = m_ThresholdDb.Get();
		f32_t ratio = m_Ratio.Get();
		f32_t attackSec = m_AttackMs.Get() * 0.001f;
		f32_t releaseSec = m_ReleaseMs.Get() * 0.001f;
		f32_t attackFactor = 1.0f - std::exp(-1.0f / (attackSec * m_SampleRate));
		f32_t releaseFactor = 1.0f - std::exp(-1.0f / (releaseSec * m_SampleRate));

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			f32_t signal = 0.0f;

			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				signal = std::max(signal, std::abs(inBuffer(f, c)));
			}

			f32_t signalDb = ToDb(signal);
			f32_t envelope = (signalDb > m_EnvelopeDb) ? attackFactor : releaseFactor;
			m_EnvelopeDb += (signalDb - m_EnvelopeDb) * envelope;

			f32_t gainDb = m_EnvelopeDb > thresholdDb ? (thresholdDb - m_EnvelopeDb) * (1.0f - 1.0f / ratio) : 0.0f;
			f32_t gain = FromDb(gainDb);

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

	f32_t m_EnvelopeDb = -100.0f;

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
