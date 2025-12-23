#include "nois/effect/NoisReverb.hpp"

#include "nois/NoisUtil.hpp"
#include "nois/effect/NoisSignalDelayer.hpp"
#include "nois/route/NoisCombiner.hpp"
#include "nois/route/NoisSplitter.hpp"

namespace nois {

class Reverb::Impl
{
	static constexpr count_t k_SplitChannels = 8;

public:	
	Impl()
		: m_Wet(0.0f)
		, m_NumDelays(k_SplitChannels)
		, m_DelayBuffer()
		, m_Delays(k_SplitChannels)
		, m_DelayMixBuffer()
		, m_DelayMix()
		, m_NumFrames(0)
		, m_NumChannels(0)
	{
		static constexpr f32_t k_Primes[k_SplitChannels] =
		{
			2, 3, 5, 7, 11, 13, 17, 19
		};

		for (count_t i = 0; i < m_NumDelays; ++i)
		{
			m_Delays[i] = SignalDelayer::Create();
			m_Delays[i]->SetDelayMs(MakeRef<FloatSlotBlockParameter>(7.0f * k_Primes[i]));
		}
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer)
	{
		NOIS_PROFILE_SCOPE_NAMED("Reverb - Process");

		for (count_t d = 0; d < m_NumDelays; ++d)
		{
			m_Delays[d]->Process(inBuffer, m_DelayBuffer);
			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				m_DelayMixBuffer.View(c * m_NumDelays + d).Copy(m_DelayBuffer.View(c));
			}
		}

		m_DelayMixBuffer.Multiply(m_DelayMix);

		for (count_t d = 0; d < m_NumDelays; ++d)
		{
			outBuffer.Add(m_DelayMixBuffer.View(d * m_NumChannels, m_NumChannels));
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		NOIS_PROFILE_SCOPE_NAMED("Reverb - Prepare");

		if (m_NumFrames != numFrames)
		{
			m_DelayBuffer.Resize(numFrames, numChannels);
			m_DelayMixBuffer.Resize(numFrames, m_NumDelays * numChannels);
			m_DelayMix.Resize(m_NumDelays * numChannels);

			f32_t factor = 2.0f / m_DelayMix.GetM();

			for (count_t i = 0; i < m_DelayMix.GetM(); ++i)
			{
				for (count_t j = 0; j < m_DelayMix.GetN(); ++j)
				{
					if (i == j)
					{
						m_DelayMix(i, j) = 1.0f - factor;
					}
					else
					{
						m_DelayMix(i, j) = -factor;
					}
					m_DelayMix(i, j) *= 0.75f;
				}
			}
		}

		for (count_t d = 0; d < m_NumDelays; ++d)
		{
			m_Delays[d]->Prepare(
				numFrames,
				numChannels,
				sampleRate);
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetWet(Ref_t<FloatParameter> wet)
	{
		m_Wet.Use(wet);
	}

private:
	FloatSlotParameter m_Wet;

	count_t m_NumDelays;
	FloatBuffer m_DelayBuffer;
	std::vector<Ref_t<SignalDelayer>> m_Delays;
	FloatBuffer m_DelayMixBuffer;
	math::FloatMat m_DelayMix;

	count_t m_NumFrames;
	count_t m_NumChannels;

	Ref_t<Splitter> m_DelaySplitter;
	Ref_t<Combiner> m_DelayCombiner;
};

NOIS_INTERFACE_IMPL(Reverb)
NOIS_INTERFACE_PARAM_IMPL(Reverb, Wet, FloatParameter)

Ref_t<Reverb> Reverb::Create()
{
	return MakeRef<Reverb>(MakeOwn<Reverb::Impl>());
}

}
