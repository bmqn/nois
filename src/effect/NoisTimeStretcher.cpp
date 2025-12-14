#include "nois/effect/NoisTimeStretcher.hpp"

#include "nois/core/NoisBuffer.hpp"
#include "nois/core/NoisRingBuffer.hpp"
#include "nois/core/NoisStream.hpp"
#include "nois/effect/NoisFilter.hpp"

namespace nois {

class TimeStretcher::Impl
{
public:
	Impl(Ref_t<Stream> stream)
		: m_Stream(stream)
	{
	}

	Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate)
	{
		NOIS_PROFILE_SCOPE_NAMED("TimeStretcher - Consume");

		Stream::Result result = Stream::Success;

		if (Stream::Result streamResult =
			m_Stream->Consume(
				buffer,
				sampleRate);
			streamResult != Stream::Success)
		{
			result = streamResult;
		}

		for (count_t frameIndex = 0;
			frameIndex < buffer.GetNumFrames();
			++frameIndex)
		{
			// Enable stretch if it became active this frame
			if (!m_IsStretchActive &&
				m_StretchActive->Get(frameIndex) > 0.0f)
			{
				for (auto &phases : m_Phases)
				{
					phases[0] = 0.0f;
					phases[1] = -1.0f;
				}

				for (auto &grains : m_Grains)
				{
					grains[0] = 0.0f;
					grains[1] = 0.0f;
				}

				for (auto &grainPlaying : m_GrainPlayings)
				{
					grainPlaying = 0;
				}

				// Start the cache counter fresh
				m_CacheBuffer.Reset();

				m_IsStretchActive = true;
			}
			// Disable stretch if it became inactive this frame
			else if (m_IsStretchActive &&
				m_StretchActive->Get(frameIndex) <= 0.0f)
			{
				m_IsStretchActive = false;
			}

			// Enable grain phase lock if it became active this frame
			if (!m_IsGrainLockActive &&
				m_GrainLockActive->Get(frameIndex) > 0.0f)
			{
				m_IsGrainLockActive = true;
			}
			// Disable grain phase lock if it became inactive this frame
			else if (m_IsGrainLockActive &&
				m_GrainLockActive->Get(frameIndex) <= 0.0f)
			{
				m_IsGrainLockActive = false;
			}

			// Do stretch
			if (m_IsStretchActive)
			{
				f32_t stretchFactor = m_StretchFactor->Get(frameIndex);
				f32_t grainSize = m_GrainSize->Get(frameIndex);
				f32_t grainBlend = m_GrainBlend->Get(frameIndex);
				f32_t grainPhaseInc = m_GrainPhaseInc->Get(frameIndex);

				stretchFactor = std::max(stretchFactor, 1.0f);
				grainSize = std::max(grainSize, 100.0f);
				grainBlend = std::clamp(grainBlend, 0.01f, 0.49f);

				DoStretch(
					buffer,
					frameIndex,
					stretchFactor,
					grainSize,
					grainBlend,
					grainPhaseInc);
			}
		}

		return result;
	}

	void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		NOIS_PROFILE_SCOPE_NAMED("TimeStretcher - PrepareToConsume");

		m_Stream->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);

		m_StretchActive->Prepare(
			numFrames,
			sampleRate);
		m_StretchFactor->Prepare(
			numFrames,
			sampleRate);
		m_StretchTimeMs->Prepare(
			numFrames,
			sampleRate);
		m_GrainSize->Prepare(
			numFrames,
			sampleRate);
		m_GrainBlend->Prepare(
			numFrames,
			sampleRate);
		m_GrainPhaseInc->Prepare(
			numFrames,
			sampleRate);
		m_GrainLockActive->Prepare(
			numFrames,
			sampleRate);

		if (m_SampleRate != sampleRate ||
			m_NumChannels != numChannels ||
			m_StretchTimeMs->Changed())
		{
			const f32_t stretchTimeMs = m_StretchTimeMs->Get();

			count_t numCacheFrames = static_cast<count_t>((stretchTimeMs * sampleRate) / 1000.0f);

			m_Phases.resize(numChannels, { 0.0f, -1.0f });
			m_Grains.resize(numChannels, { 0.0f, 0.0f });
			m_GrainPlayings.resize(numChannels, 0);
			m_CacheBuffer.Resize(numCacheFrames, numChannels);

			m_SampleRate = sampleRate;
			m_NumChannels = numChannels;
		}
	}

	Ref_t<FloatParameter> GetStretchActive()
	{
		return m_StretchActive;
	}

	void SetStretchActive(Ref_t<FloatParameter> stretchActive)
	{
		m_StretchActive = stretchActive;
	}

	Ref_t<FloatBlockParameter> GetStretchTimeMs()
	{
		return m_StretchTimeMs;
	}

	void SetStretchTimeMs(Ref_t<FloatBlockParameter> stretchTimeMs)
	{
		m_StretchTimeMs = stretchTimeMs;
	}

	Ref_t<FloatParameter> GetStretchFactor()
	{
		return m_StretchFactor;
	}

	void SetStretchFactor(Ref_t<FloatParameter> stretchFactor)
	{
		m_StretchFactor = stretchFactor;
	}

	Ref_t<FloatParameter> GetGrainSize()
	{
		return m_GrainSize;
	}

	void SetGrainSize(Ref_t<FloatParameter> grainSize)
	{
		m_GrainSize = grainSize;
	}

	Ref_t<FloatParameter> GetGrainBlend()
	{
		return m_GrainBlend;
	}

	void SetGrainBlend(Ref_t<FloatParameter> grainBlend)
	{
		m_GrainBlend = grainBlend;
	}

	Ref_t<FloatParameter> GetGrainPhaseInc()
	{
		return m_GrainPhaseInc;
	}

	void SetGrainPhaseInc(Ref_t<FloatParameter> grainPhaseInc)
	{
		m_GrainPhaseInc = grainPhaseInc;
	}

	Ref_t<FloatParameter> GetGrainLockActive()
	{
		return m_GrainLockActive;
	}

	void SetGrainLockActive(Ref_t<FloatParameter> grainLockActive)
	{
		m_GrainLockActive = grainLockActive;
	}

private:
	void DoStretch(
		FloatBuffer &buffer,
		count_t frameIndex,
		f32_t stretchFactor,
		f32_t grainSize,
		f32_t grainBlend,
		f32_t grainPhaseInc)
	{
		count_t numChannels = buffer.GetNumChannels();

		// Scratch storage for samples
		std::array<f32_t, k_MaxChannels> samples;

		// Add the latest samples to our cache
		//
		// Don't add samples if we're full.
		// Otherwise we could overwrite stuff we're reading.
		if (!m_CacheBuffer.IsFull())
		{
			for (count_t c = 0; c < numChannels; ++c)
			{
				samples[c] = buffer(frameIndex, c);
			}

			m_CacheBuffer.Add(samples.data(), numChannels);
		}
		
		// Read samples from cache and update grains
		for (count_t c = 0; c < numChannels; ++c)
		{
			f32_t &sample = buffer(frameIndex, c);

			f32_t stretchRatio = (1.0f / stretchFactor);
			// How far the input index should advance when launching a new grain
			f32_t grainOffset = grainSize * (1.0f - grainBlend) * stretchRatio;
			// The phase in which we should start blending in the next grain
			f32_t startBlendPhase = grainSize * (1.0f - grainBlend);
			// The phase in which we should stop blending out the old grain
			f32_t endBlendPhase = grainSize * grainBlend;

			// If blending both grains
			if (m_Phases[c][m_GrainPlayings[c]] >= 0.0f &&
				m_Phases[c][!m_GrainPlayings[c]] >= startBlendPhase)
			{
				f32_t g0 = m_Grains[c][m_GrainPlayings[c]];
				f32_t g1 = m_Grains[c][!m_GrainPlayings[c]];
				f32_t phi0 = m_Phases[c][m_GrainPlayings[c]];
				f32_t phi1 = m_Phases[c][!m_GrainPlayings[c]];

				// Zero when new grain just started blending
				// One when old grain has finished
				f32_t t = phi0 / endBlendPhase;
				
				f32_t x0 = g0 + phi0;
				f32_t x1 = g1 + phi1;
		
				count_t i0 = static_cast<count_t>(x0);
				count_t i1 = static_cast<count_t>(x1);

				f32_t s0 = 0.0f;
				f32_t s1 = 0.0f;

				m_CacheBuffer.GetChronological(i0, samples.data(), numChannels);
				s0 = samples[c];
				m_CacheBuffer.GetChronological(i1, samples.data(), numChannels);
				s1 = samples[c];

				sample = s0 * (1.0f - t) + s1 * t;
			}
			// If playing one grain
			else
			{
				f32_t g = m_Grains[c][m_GrainPlayings[c]];
				f32_t phi = m_Phases[c][m_GrainPlayings[c]];

				f32_t x = g + phi;

				count_t i = static_cast<count_t>(x);

				m_CacheBuffer.GetChronological(i, samples.data(), numChannels);
				sample = samples[c];
			}

			// Increment current grain phase
			m_Phases[c][m_GrainPlayings[c]] += grainPhaseInc;

			// If the previous grain is blending out
			if (m_Phases[c][!m_GrainPlayings[c]] >= startBlendPhase)
			{
				// Increment previous grain phase
				m_Phases[c][!m_GrainPlayings[c]] += grainPhaseInc;

				// If the previous grain has finished
				if (m_Phases[c][!m_GrainPlayings[c]] >= grainSize)
				{
					// Stop the previous grain
					m_Phases[c][!m_GrainPlayings[c]] = -1.0f;
				}
			}

			// If we should start blending in the next grain
			if (m_Phases[c][m_GrainPlayings[c]] >= startBlendPhase)
			{
				count_t newGrainOffset = m_Grains[c][m_GrainPlayings[c]] + grainOffset;

				if (m_IsGrainLockActive)
				{
					// Lock the offset to the nearest zero crossing
					newGrainOffset = FindZeroCrossing(
						m_CacheBuffer,
						newGrainOffset,
						c,
						grainOffset);
				}

				// Start the next grain
				m_Phases[c][!m_GrainPlayings[c]] = 0.0f;
				m_Grains[c][!m_GrainPlayings[c]] = newGrainOffset;
				m_GrainPlayings[c] = 1 - m_GrainPlayings[c];
			}
		}
	}

	inline count_t FindZeroCrossing(
		const InterleavedRingBuffer<f32_t> &buffer,
		count_t frameIndex,
		count_t channelIndex,
		count_t searchLength)
	{
		count_t numChannels = buffer.GetNumChannels();

		// Scratch storage for samples
		std::array<f32_t, k_MaxChannels> samples;

		f32_t prevSample = 0.0f;
		f32_t currSample = 0.0f;

		// Scan backward to find zero-crossing
		for (count_t searchOffset = 0; searchOffset < searchLength; ++searchOffset)
		{
			buffer.GetChronological(frameIndex - searchOffset, samples.data(), numChannels);

			currSample = samples[channelIndex];

			// Check for zero crossing
			if (prevSample <= 0.0f && currSample > 0.0f)
			{
				return frameIndex - searchOffset;
			}

			prevSample = currSample;
		}

		// Fallback on provided frame index
		return frameIndex;
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_StretchActive = CreateParameter(0.0f);
	Ref_t<FloatParameter> m_StretchFactor = CreateParameter(1.0f);
	Ref_t<FloatBlockParameter> m_StretchTimeMs = CreateBlockParameter(10000.0f);
	Ref_t<FloatParameter> m_GrainSize = CreateParameter(2500.0f);
	Ref_t<FloatParameter> m_GrainBlend = CreateParameter(0.4f);
	Ref_t<FloatParameter> m_GrainPhaseInc = CreateParameter(1.0f);
	Ref_t<FloatParameter> m_GrainLockActive = CreateParameter(0.0f);

	bool m_IsStretchActive = false;
	bool m_IsGrainLockActive = false;
	std::vector<std::array<f32_t, 2>> m_Phases;
	std::vector<std::array<f32_t, 2>> m_Grains;
	std::vector<count_t> m_GrainPlayings;
	FloatInterleavedRingBuffer m_CacheBuffer;

	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;
};

TimeStretcher::TimeStretcher(Ref_t<Stream> stream)
	: m_Impl(MakeOwn<Impl>(stream))
{
}

Stream::Result TimeStretcher::Consume(FloatBuffer& buffer, f32_t sampleRate)
{
	return m_Impl->Consume(
		buffer,
		sampleRate);
}

void TimeStretcher::PrepareToConsume(count_t numFrames, count_t numChannels, f32_t sampleRate)
{
	m_Impl->PrepareToConsume(
		numFrames,
		numChannels,
		sampleRate);
}

NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchActive, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchTimeMs, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchFactor, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainSize, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainBlend, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainPhaseInc, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainLockActive, FloatParameter)

Ref_t<TimeStretcher> CreateTimeStretcher(Ref_t<Stream> stream)
{
	return MakeRef<TimeStretcher>(stream);
}

}

