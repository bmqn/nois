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
	static constexpr f32_t k_DelayMsRange = 100.0f;

public:
	inline void Prepare(
		count_t numFrames,
		count_t numChannels,
		count_t numDelays,
		f32_t sampleRate)
	{
		if (m_NumFrames != numFrames ||
			m_NumChannels != numChannels ||
			m_NumDelays != numDelays)
		{
			m_Delays.resize(numChannels * numDelays);
			m_DelaysMixBuffer.Resize(numFrames, numChannels * numDelays);
			m_Mix.Resize(numChannels * numDelays);

			count_t delayFramesRange = k_DelayMsRange * 0.001f * sampleRate;
			for (count_t c = 0; c < numChannels; ++c)
			{
				for (count_t d = 0; d < numDelays; ++d)
				{
					count_t delayFramesLow = delayFramesRange * d / numDelays;
					count_t delayFramesHigh = delayFramesRange * (d + 1) / numDelays;
					count_t delayFrames = delayFramesLow + rand() % (delayFramesHigh - delayFramesLow);
					m_Delays[c * numDelays + d].Reset(delayFrames);
				}
			}

			BuildHadamard();
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_NumDelays = numDelays;
	}

	inline void Process(
		const FloatBuffer& inBuffer,
		FloatBuffer& outBuffer)
	{
		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t d = 0; d < m_NumDelays; ++d)
			{
				m_Delays[c * m_NumDelays + d].Process(inBuffer.View(c * m_NumDelays + d), m_DelaysMixBuffer.View(c * m_NumDelays + d), m_NumFrames, 0);
			}
		}

		m_DelaysMixBuffer.Multiply(m_Mix);

		outBuffer.Copy(m_DelaysMixBuffer);
	}

private:
	void BuildHadamard()
	{
		count_t dim = std::min(m_Mix.GetM(), m_Mix.GetN());

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
	count_t m_NumDelays = 0;
};
template<typename T>
class FeedbackStep
{
	static constexpr f32_t k_DelayMsRange = 100.0f;

public:
	inline void Prepare(
		count_t numFrames,
		count_t numChannels,
		count_t numDelays,
		f32_t sampleRate)
	{
		if (m_NumFrames != numFrames ||
			m_NumChannels != numChannels ||
			m_NumDelays != numDelays)
		{
			m_DelayFeedbacks.resize(numChannels * numDelays);
			m_DelayFeedbacksMixBuffer.Resize(numFrames, numChannels * numDelays);
			m_Mix.Resize(numChannels * numDelays);

			for (count_t c = 0; c < numChannels; ++c)
			{
				count_t delayFramesRange = k_DelayMsRange * 0.001f * (c + 1) * sampleRate;
				for (count_t d = 0; d < numDelays; ++d)
				{
					count_t delayFramesLow = delayFramesRange * d / numDelays;
					count_t delayFramesHigh = delayFramesRange * (d + 1) / numDelays;
					count_t delayFrames = delayFramesLow + rand() % (delayFramesHigh - delayFramesLow);
					m_DelayFeedbacks[c * numDelays + d].Reset(delayFrames);
				}
			}

			BuildHouseholder();
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_NumDelays = numDelays;
	}

	inline void Process(
		const FloatBuffer& inBuffer,
		FloatBuffer& outBuffer)
	{
		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t d = 0; d < m_NumDelays; ++d)
			{
				m_DelayFeedbacks[c * m_NumDelays + d].Process(inBuffer.View(c * m_NumDelays + d), m_DelayFeedbacksMixBuffer.View(c * m_NumDelays + d), m_NumFrames, 0);
			}
		}

		m_DelayFeedbacksMixBuffer.Multiply(m_Mix);

		outBuffer.Copy(m_DelayFeedbacksMixBuffer);
	}

private:
	void BuildHouseholder()
	{
		count_t dim = std::min(m_Mix.GetM(), m_Mix.GetN());

		f32_t factor = 2.0f / dim;
		for (count_t i = 0; i < dim; ++i)
		{
			for (count_t j = 0; j < dim; ++j)
			{
				if (i == j)
				{
					m_Mix(i, j) = 1.0f - factor;
				}
				else
				{
					m_Mix(i, j) = -factor;
				}
			}
		}
	}

private:
	std::vector<DelayFeedback<T, 1>> m_DelayFeedbacks;
	FloatBuffer m_DelayFeedbacksMixBuffer;
	math::FloatMat m_Mix;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	count_t m_NumDelays = 0;
};


class Reverb::Impl
{
	static constexpr count_t k_NumDiffusers = 4;
	static constexpr count_t k_NumDiffuserChannels = 8;

public:	
	Impl()
		: m_Wet(0.0f)
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
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer)
	{
		NOIS_PROFILE_SCOPE_NAMED("Reverb - Process");

		outBuffer.Zero();

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

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t dc = 0; dc < k_NumDiffuserChannels; ++dc)
			{
				f32_t sign = (dc & 1) ? -1.0f : 1.0f;
				outBuffer.View(c).Add(m_SplitBuffer.View(c * k_NumDiffuserChannels + dc), sign);
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
			m_SplitBuffer.Resize(numFrames, k_NumDiffuserChannels * numChannels);
		}

		for (count_t d = 0; d < m_NumDiffusionSteps; ++d)
		{
			m_DiffusionSteps[d].Prepare(numFrames, numChannels, k_NumDiffuserChannels, sampleRate);
		}

		m_FeedbackStep.Prepare(numFrames, numChannels, k_NumDiffuserChannels, sampleRate);

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetWet(Ref_t<FloatBlockParameter> wet)
	{
		m_Wet.Use(wet);
	}

private:
	FloatSlotBlockParameter m_Wet;

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

Ref_t<Reverb> Reverb::Create()
{
	return MakeRef<Reverb>(MakeOwn<Reverb::Impl>());
}

}
