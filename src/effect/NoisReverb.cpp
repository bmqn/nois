#include "nois/effect/NoisReverb.hpp"

#include "nois/NoisUtil.hpp"
#include "nois/effect/NoisSignalDelayer.hpp"
#include "nois/route/NoisCombiner.hpp"
#include "nois/route/NoisSplitter.hpp"
#include "nois/util/NoisDelay.hpp"

namespace nois {

template<typename T>
class DiffusionStep
{
	static constexpr T k_DelayMsRange = T{ 300.0 };

public:
	inline void Prepare(
		count_t numFrames,
		count_t numChannels,
		count_t numDiffusionChannels,
		f32_t sampleRate)
	{
		if (m_NumFrames != numFrames ||
			m_NumChannels != numChannels ||
			m_NumDiffusionChannels != numDiffusionChannels)
		{
			m_Delays.resize(numChannels * numDiffusionChannels);
			m_DelaysMixBuffer.Resize(numFrames, numChannels * numDiffusionChannels);
			m_Mix.Resize(numDiffusionChannels);

			for (count_t c = 0; c < numChannels; ++c)
			{
				count_t delayNumFramesRange = k_DelayMsRange * T{ 0.001 } * sampleRate;
				for (count_t d = 0; d < numDiffusionChannels; ++d)
				{
					count_t delayNumFramesLow = delayNumFramesRange * d / numDiffusionChannels;
					count_t delayNumFramesHigh = delayNumFramesRange * (d + 1) / numDiffusionChannels;
					count_t delayNumFrames = delayNumFramesLow + rand() % (delayNumFramesHigh - delayNumFramesLow);
					NZ_ASSERT(delayNumFrames != 0);
					m_Delays[c * numDiffusionChannels + d].Reset(delayNumFrames);
				}
			}

			BuildHadamard();
		}

		m_DelaysMixBuffer.Zero();

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_NumDiffusionChannels = numDiffusionChannels;
	}

	inline void Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t d = 0; d < m_NumDiffusionChannels; ++d)
			{
				m_Delays[c * m_NumDiffusionChannels + d].Process(inBuffer.View(c * m_NumDiffusionChannels + d), m_DelaysMixBuffer.View(c * m_NumDiffusionChannels + d), m_NumFrames, 0);
			}

			m_DelaysMixBuffer.View(c * m_NumDiffusionChannels, m_NumDiffusionChannels).Multiply(m_Mix);
			outBuffer.View(c * m_NumDiffusionChannels, m_NumDiffusionChannels).Copy(m_DelaysMixBuffer.View(c * m_NumDiffusionChannels, m_NumDiffusionChannels));
		}
	}

private:
	void BuildHadamard()
	{
		count_t dim = std::min(m_Mix.GetM(), m_Mix.GetN());

		m_Mix.Zero();

		m_Mix(0, 0) = 1.0f;
		for (count_t size = 1; size < dim; size *= 2)
		{
			for (count_t i = 0; i < size; ++i)
			{
				for (count_t j = 0; j < size; ++j)
				{
					f32_t v = m_Mix(i, j);
					m_Mix(i, j + size) = v;
					m_Mix(i + size, j) = v;
					m_Mix(i + size, j + size) = -v;
				}
			}
		}

		f32_t scale = 1.0f / std::sqrt(static_cast<f32_t>(dim));
		for (count_t i = 0; i < dim; ++i)
		{
			for (count_t j = 0; j < dim; ++j)
			{
				m_Mix(i, j) *= scale;
			}
		}
	}

private:
	std::vector<Delay<T, 1>> m_Delays;
	FloatBuffer m_DelaysMixBuffer;
	math::FloatMat m_Mix;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	count_t m_NumDiffusionChannels = 0;
};

template<typename T>
class FeedbackStep
{
	static constexpr T k_DelayMsBase = T{ 50.0 };

public:
	inline void Prepare(
		count_t numFrames,
		count_t numChannels,
		count_t numFeedbacks,
		f32_t decayTimeMs,
		f32_t sampleRate)
	{
		if (m_NumFrames != numFrames ||
			m_NumChannels != numChannels ||
			m_NumFeedbacks != numFeedbacks)
		{
			m_DelayFeedbacks.resize(numChannels * numFeedbacks);
			m_DelayFeedbacksMixBuffer.Resize(numFrames, numChannels * numFeedbacks);
			m_Mix.Resize(numFeedbacks);

			for (count_t c = 0; c < numChannels; ++c)
			{
				count_t delaySamplesBase = k_DelayMsBase * T{ 0.001 } * sampleRate;
				for (count_t d = 0; d < numFeedbacks; ++d)
				{
					T r = d * T{ 1.0 } / numFeedbacks;
					count_t delayNumFrames = std::pow(2, r) * delaySamplesBase;
					NZ_ASSERT(delayNumFrames != 0);
					m_DelayFeedbacks[c * numFeedbacks + d].Reset(delayNumFrames);
				}
			}

			BuildHouseholder();
		}

		if (m_DecayTimeMs != decayTimeMs)
		{
			f32_t t60 = decayTimeMs / 1000.0f;
			for (count_t c = 0; c < numChannels; ++c)
			{
				for (count_t d = 0; d < numFeedbacks; ++d)
				{
					auto& delayFeedback = m_DelayFeedbacks[c * numFeedbacks + d];
					delayFeedback.SetFeedbackTarget(std::pow(10.0f, (-3.0f * delayFeedback.GetNumFrames()) / (sampleRate * t60)));
				}
			}
		}

		m_DelayFeedbacksMixBuffer.Zero();

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_NumFeedbacks = numFeedbacks;
		m_DecayTimeMs = decayTimeMs;
	}

	inline void Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t d = 0; d < m_NumFeedbacks; ++d)
			{
				m_DelayFeedbacks[c * m_NumFeedbacks + d].Process(inBuffer.View(c * m_NumFeedbacks + d), m_DelayFeedbacksMixBuffer.View(c * m_NumFeedbacks + d), m_NumFrames, 0);
			}

			m_DelayFeedbacksMixBuffer.View(c * m_NumFeedbacks, m_NumFeedbacks).Multiply(m_Mix);
			outBuffer.View(c * m_NumFeedbacks, m_NumFeedbacks).Copy(m_DelayFeedbacksMixBuffer.View(c * m_NumFeedbacks, m_NumFeedbacks));
		}
	}

private:
	void BuildHouseholder()
	{
		count_t dim = std::min(m_Mix.GetM(), m_Mix.GetN());

		m_Mix.Zero();

		f32_t factor = 2.0f / static_cast<f32_t>(dim);
		for (count_t i = 0; i < dim; ++i)
		{
			for (count_t j = 0; j < dim; ++j)
			{
				m_Mix(i, j) = (i == j) ? 1.0f - factor : -factor;
			}
		}
	}

private:
	std::vector<DelayFeedback<T, 1>> m_DelayFeedbacks;
	FloatBuffer m_DelayFeedbacksMixBuffer;
	math::FloatMat m_Mix;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	count_t m_NumFeedbacks = 0;
	f32_t m_DecayTimeMs = 50.0f;
};


class Reverb::Impl
{
	static constexpr count_t k_NumDiffusers = 4;
	static constexpr count_t k_NumDiffuserChannels = 8;
	static constexpr f32_t k_SplitSumGain = 1.0f / std::sqrt(f32_t(k_NumDiffuserChannels));

public:	
	Impl()
		: m_Wet(0.0f)
		, m_DecayMs(1000.0f)
		, m_SplitBuffer()
		, m_NumDiffusionSteps(k_NumDiffusers)
		, m_DiffusionSteps(k_NumDiffusers)
		, m_FeedbackStep()
		, m_NumFrames(0)
		, m_NumChannels(0)
	{
		srand(time(NULL));
	}

	Stream::Result Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		NOIS_PROFILE_SCOPE_NAMED("Reverb - Process");

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t dc = 0; dc < k_NumDiffuserChannels; ++dc)
			{
				m_SplitBuffer.View(c * k_NumDiffuserChannels + dc).Copy(inBuffer.View(c));
			}
		}

		for (count_t d = 0; d < m_NumDiffusionSteps; ++d)
		{
			m_DiffusionSteps[d].Process(m_SplitBuffer, m_SplitBuffer);
		}

		m_FeedbackStep.Process(m_SplitBuffer, m_SplitBuffer);

		outBuffer.Zero();

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t dc = 0; dc < k_NumDiffuserChannels; ++dc)
			{
				outBuffer.View(c).Add(m_SplitBuffer.View(c * k_NumDiffuserChannels + dc), k_SplitSumGain);
			}
		}

		outBuffer.CopyLinearily(inBuffer, m_Wet.Get());

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		NOIS_PROFILE_SCOPE_NAMED("Reverb - Prepare");

		if (m_NumFrames != numFrames ||
			m_NumChannels != numChannels)
		{
			m_SplitBuffer.Resize(numFrames, numChannels * k_NumDiffuserChannels);
		}

		for (count_t d = 0; d < m_NumDiffusionSteps; ++d)
		{
			m_DiffusionSteps[d].Prepare(numFrames, numChannels, k_NumDiffuserChannels, sampleRate);
		}

		m_FeedbackStep.Prepare(numFrames, numChannels, k_NumDiffuserChannels, m_DecayMs.Get(), sampleRate);

		m_SplitBuffer.Zero();

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetWet(Ref_t<FloatBlockParameter> wet)
	{
		m_Wet.Use(wet);
	}

	void SetDecayMs(Ref_t<FloatBlockParameter> decayMs)
	{
		m_DecayMs.Use(decayMs);
	}

private:
	FloatSlotBlockParameter m_Wet;
	FloatSlotBlockParameter m_DecayMs;

	FloatBuffer m_SplitBuffer;
	count_t m_NumDiffusionSteps;
	std::vector<DiffusionStep<f32_t>> m_DiffusionSteps;
	FeedbackStep<f32_t> m_FeedbackStep;

	count_t m_NumFrames;
	count_t m_NumChannels;

	Ref_t<Splitter> m_DelaySplitter;
	Ref_t<Combiner> m_DelayCombiner;
};

NOIS_INTERFACE_IMPL(Reverb)
NOIS_INTERFACE_PARAM_IMPL(Reverb, Wet, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(Reverb, DecayMs, FloatBlockParameter)

Ref_t<Reverb> Reverb::Create()
{
	return MakeRef<Reverb>(MakeOwn<Reverb::Impl>());
}

}
