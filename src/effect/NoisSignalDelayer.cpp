#include "nois/effect/NoisSignalDelayer.hpp"

#include "nois/core/NoisRingBuffer.hpp"

namespace nois {

class SignalDelayer::Impl
{
public:
	Impl()
		: m_DelayMs(0.0f)
	{
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer)
	{
		// Scratch storage for samples
		std::array<f32_t, k_MaxChannels> samples;

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			// Grab latest samples
			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				samples[c] = inBuffer(f, c);
			}

			// Add the latest samples to our cache
			m_CacheBuffer.Add(samples.data(), m_NumChannels);

			// Grab the oldest samples from our cache
			m_CacheBuffer.GetOldest(samples.data(), m_NumChannels);

			// Write the oldest samples
			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				outBuffer(f, c) = samples[c];
			}
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		if (m_SampleRate != sampleRate ||
			m_DelayMs.Changed())
		{
			const f32_t delayMs = m_DelayMs.Get();
			count_t numCacheFrames = static_cast<count_t>((delayMs * sampleRate) / 1000.0f);
			m_CacheBuffer.Resize(numCacheFrames, numChannels);
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

	// TODO: replace with non-interleaved since buffers are not interleaved
	FloatInterleavedRingBuffer m_CacheBuffer;

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
