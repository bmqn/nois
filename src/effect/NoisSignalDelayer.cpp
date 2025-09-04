#include "nois/effect/NoisSignalDelayer.hpp"

#include "nois/core/NoisRingBuffer.hpp"

namespace nois {

class SignalDelayerImpl : public SignalDelayer
{
public:
	SignalDelayerImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_DelayMs(MakeRef<FloatConstantParameter>(0.0f))
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

		count_t numFrames = buffer.GetNumFrames();
		count_t numChannels = buffer.GetNumChannels();

		// Scratch storage for samples
		std::array<f32_t, k_MaxChannels> samples;

		for (count_t f = 0; f < numFrames; ++f)
		{
			// Add the latest samples to our cache
			{
				for (count_t c = 0; c < numChannels; ++c)
				{
					samples[c] = buffer(f, c);
				}

				m_CacheBuffer.Add(samples.data(), numChannels);
			}

			// Grab the oldest samples
			for (count_t c = 0; c < numChannels; ++c)
			{
				m_CacheBuffer.GetOldest(samples.data(), numChannels);
				buffer(f, c) = samples[c];
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

		if (m_SampleRate != sampleRate ||
			m_DelayMs->Changed(0))
		{
			const f32_t delayMs = m_DelayMs->Get(0);

			count_t numCacheFrames = static_cast<count_t>((delayMs * sampleRate) / 1000.0f);
			m_CacheBuffer.Resize(numCacheFrames, numChannels);
	
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
	Ref_t<Stream> m_Stream;
	// TODO: make me a block-based parameter
	Ref_t<FloatParameter> m_DelayMs;

	FloatInterleavedRingBuffer m_CacheBuffer;

	s32_t m_SampleRate = 0.0f;
};

Ref_t<SignalDelayer> CreateSignalDelayer(Ref_t<Stream> stream)
{
	return MakeRef<SignalDelayerImpl>(stream);
}

}
