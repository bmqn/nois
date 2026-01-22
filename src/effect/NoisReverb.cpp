#include "nois/effect/NoisReverb.hpp"

#include "nois/NoisUtil.hpp"
#include "nois/util/NoisDelay.hpp"

namespace nois {

template<typename T>
class EarlyReflectionStep
{
	static constexpr T k_MinDelayMs = T{ 5.0 };
	static constexpr T k_MaxDelayMs = T{ 50.0 };

public:
	inline void Prepare(
		count_t numFrames,
		count_t numChannels,
		count_t numReflections,
		f32_t sampleRate)
	{
		if (m_NumFrames != numFrames ||
			m_NumChannels != numChannels ||
			m_NumReflections != numReflections)
		{
			m_Delays.resize(numChannels * numReflections);

			for (count_t c = 0; c < numChannels; ++c)
			{
				for (count_t r = 0; r < numReflections; ++r)
				{
					T t = static_cast<T>(r + 1) / static_cast<T>(numReflections);
					T delayMs = std::lerp(k_MinDelayMs, k_MaxDelayMs, t * t);
					count_t delayNumFrames = delayMs * T{ 0.001 } * sampleRate;
					NZ_ASSERT(delayNumFrames != 0);
					auto& delay = m_Delays[c * numReflections + r];
					delay.Reset(delayNumFrames);
					delay.SetDelay(delayNumFrames);
				}
			}
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_NumReflections = numReflections;
	}

	inline void Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		T gainPerDelay = T{ 1.0 } / std::sqrt(static_cast<T>(m_NumReflections));
		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			outBuffer.View(c).Zero();

			for (count_t f = 0; f < m_NumFrames; ++f)
			{
				T acc{ 0 };
				for (count_t r = 0; r < m_NumReflections; ++r)
				{
					auto& delay = m_Delays[c * m_NumReflections + r];
					acc += delay.Process(inBuffer(f, c));
				}

				outBuffer(f, c) += acc * gainPerDelay;
			}
		}
	}

private:
	std::vector<Delay<T>> m_Delays;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	count_t m_NumReflections = 0;
};


template<typename T>
class DiffusionStep
{
	static constexpr T k_MinDelayMs = T{ 20.0 };
	static constexpr T k_MaxDelayMs = T{ 150.0 };

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
			m_Mix.Resize(numDelays);

			for (count_t c = 0; c < numChannels; ++c)
			{
				for (count_t d = 0; d < numDelays; ++d)
				{
					T t = static_cast<T>(d + 1) / static_cast<T>(numDelays);
					T delayMs = std::lerp(k_MinDelayMs, k_MaxDelayMs, t * t);
					count_t delayNumFrames = delayMs * T{ 0.001 } * sampleRate;
					auto& delay = m_Delays[c * numDelays + d];
					delay.Reset(delayNumFrames);
					delay.SetDelay(delayNumFrames);
				}
			}

			BuildHadamard();
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_NumDelays = numDelays;
		m_SampleRate = sampleRate;
	}

	inline f32_t Noise()
	{
		static u32_t seed = 12345;
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		return static_cast<f32_t>(seed & 0xFFFFFF) / static_cast<f32_t>(0x1000000);
	}

	inline void Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		// TODO: replace simple modulation...
		static float mod = 0.0f;
		static int modx = 0;
		mod = 5.0f * std::sin(2.0f * M_PI * (float)modx / (float)(1 << 10));
		modx = (modx + 1) & ((1 << 10) - 1);

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			auto delayInBuffer = inBuffer.View(c * m_NumDelays, m_NumDelays);
			auto delayOutBuffer = outBuffer.View(c * m_NumDelays, m_NumDelays);

			for (count_t f = 0; f < m_NumFrames; ++f)
			{
				for (count_t d = 0; d < m_NumDelays; ++d)
				{
					auto& delay = m_Delays[c * m_NumDelays + d];
					delayOutBuffer(f, d) = delay.Process(delayInBuffer(f, d), mod);
				}
			}

			delayOutBuffer.Multiply(m_Mix);
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
	std::vector<Delay<T>> m_Delays;
	math::FloatMat m_Mix;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	count_t m_NumDelays = 0;
	f32_t m_SampleRate = 0.0f;
};

template<typename T>
class FeedbackStep
{
	static constexpr T k_MinDelayMs = T{ 30.0 };
	static constexpr T k_MaxDelayMs = T{ 120.0 };

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
			m_Mix.Resize(numDelays);

			for (count_t c = 0; c < numChannels; ++c)
			{
				for (count_t d = 0; d < numDelays; ++d)
				{
					T t = static_cast<T>(d + 1) / static_cast<T>(numDelays);
					T delayMs = std::lerp(k_MinDelayMs, k_MaxDelayMs, t * t);
					count_t delayNumFrames = delayMs * T{ 0.001 } * sampleRate;
					NZ_ASSERT(delayNumFrames != 0);
					auto& delay = m_Delays[c * numDelays + d];
					delay.Reset(delayNumFrames);
					delay.SetDelay(delayNumFrames);
				}
			}

			BuildHouseholder();
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_NumDelays = numDelays;
	}

	inline void Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		// TODO: replace simple modulation...
		static float mod = 0.0f;
		static int modx = 0;
		mod = 5.0f * std::sin(2.0f * M_PI * (float)modx / (float)(1 << 10));
		modx = (modx + 1) & ((1 << 10) - 1);

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			auto delayInBuffer = inBuffer.View(c * m_NumDelays, m_NumDelays);
			auto delayOutBuffer = outBuffer.View(c * m_NumDelays, m_NumDelays);

			for (count_t f = 0; f < m_NumFrames; ++f)
			{
				for (count_t d = 0; d < m_NumDelays; ++d)
				{
					auto& delay = m_Delays[c * m_NumDelays + d];
					delayOutBuffer(f, d) = delay.Process(delayInBuffer(f, d), mod, 0.9f);
				}
			}

			delayOutBuffer.Multiply(m_Mix);
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
	std::vector<DelayFeedback<T>> m_Delays;
	math::FloatMat m_Mix;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	count_t m_NumDelays = 0;
	f32_t m_DecayTimeMs = 50.0f;
};


class Reverb::Impl
{
	static constexpr count_t k_NumEarlyReflections = 8;
	static constexpr count_t k_NumDiffusers = 4;
	static constexpr count_t k_NumDiffuserChannels = 8;

public:	
	Impl()
		: m_Wet(0.0f)
		, m_DecayMs(1000.0f)
		, m_EarlyReflectionBuffer()
		, m_DiffusionBuffer()
		, m_NumDiffusionSteps(k_NumDiffusers)
		, m_EarlyReflectionStep()
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

		m_EarlyReflectionStep.Process(inBuffer, m_EarlyReflectionBuffer);

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t dc = 0; dc < k_NumDiffuserChannels; ++dc)
			{
				m_DiffusionBuffer.View(c * k_NumDiffuserChannels + dc).Copy(inBuffer.View(c));
			}
		}

		for (count_t d = 0; d < m_NumDiffusionSteps; ++d)
		{
			m_DiffusionSteps[d].Process(m_DiffusionBuffer, m_DiffusionBuffer);
		}

		m_FeedbackStep.Process(m_DiffusionBuffer, m_DiffusionBuffer);

		outBuffer.Copy(m_EarlyReflectionBuffer);

		f32_t gainPerChannel = 1.0f / std::sqrt(f32_t(k_NumDiffuserChannels));
		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			for (count_t dc = 0; dc < k_NumDiffuserChannels; ++dc)
			{
				outBuffer.View(c).Add(
					m_DiffusionBuffer.View(c * k_NumDiffuserChannels + dc),
					gainPerChannel);
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
			m_EarlyReflectionBuffer.Resize(numFrames, numChannels);
			m_DiffusionBuffer.Resize(numFrames, numChannels * k_NumDiffuserChannels);
		}

		m_EarlyReflectionStep.Prepare(numFrames, numChannels, k_NumEarlyReflections, sampleRate);

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

	void SetDecayMs(Ref_t<FloatBlockParameter> decayMs)
	{
		m_DecayMs.Use(decayMs);
	}

private:
	FloatSlotBlockParameter m_Wet;
	FloatSlotBlockParameter m_DecayMs;

	FloatBuffer m_EarlyReflectionBuffer;
	FloatBuffer m_DiffusionBuffer;
	count_t m_NumDiffusionSteps;
	EarlyReflectionStep<f32_t> m_EarlyReflectionStep;
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
