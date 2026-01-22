#include "nois/effect/NoisSignalDelayer.hpp"

#include "NoisLog.h"
#include "nois/NoisConfig.hpp"
#include "nois/util/NoisDelay.hpp"

namespace nois {

class SignalDelayer::Impl
{
public:
	Impl()
		: m_DelayMs(0.0f)
	{
	}

	Stream::Result Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t f = 0; f < m_NumFrames; ++f)
			{
				outBuffer(f, c) = m_Delay.Process(inBuffer(f, c));
			}
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		if (m_SampleRate != sampleRate)
		{
			const f32_t maxDelayMs = m_DelayMs.Max();
			count_t maxDelayNumFrames = static_cast<count_t>((maxDelayMs * sampleRate) / 1000.0f);
			NZ_ASSERT(maxDelayNumFrames != 0);
			m_Delay.Reset(maxDelayNumFrames);
		}

		if (m_SampleRate != sampleRate ||
			m_DelayMs.Changed())
		{
			const f32_t delayMs = m_DelayMs.Get();
			count_t numDelayFrames = static_cast<count_t>((delayMs * sampleRate) / 1000.0f);
			NZ_ASSERT(numDelayFrames != 0);
			m_Delay.SetDelay(numDelayFrames);
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_SampleRate = sampleRate;
	}

	void SetDelayMs(Ref_t<FloatBlockParameter> delayMs)
	{
		m_DelayMs.Use(delayMs);
	}

private:
	SlotBlockParameter<f32_t> m_DelayMs;

	Delay<f32_t, k_MaxChannels> m_Delay;

	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	s32_t m_SampleRate = 0.0f;
};

NOIS_INTERFACE_IMPL(SignalDelayer)
NOIS_INTERFACE_PARAM_IMPL(SignalDelayer, DelayMs, FloatBlockParameter)

Ref_t<SignalDelayer> SignalDelayer::Create()
{
	return MakeRef<SignalDelayer>(MakeOwn<SignalDelayer::Impl>());
}

}
