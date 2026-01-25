#include "nois/effect/NoisTimeStretcher.hpp"

#include "NoisLog.h"
#include "nois/NoisConfig.hpp"
#include "nois/util/NoisDelay.hpp"

namespace nois {

class TimeStretcher::Impl
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

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			// Enable stretch if it became active this frame
			if (!m_IsStretchActive &&
				m_StretchActive.Get(f) > 0.0f)
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

				for (auto &fillOffset : m_FillOffsets)
				{
					fillOffset = 0;
				}

				m_Delay.Reset(m_NumCacheFrames);

				m_IsStretchActive = true;
			}
			// Disable stretch if it became inactive this frame
			else if (m_IsStretchActive &&
				m_StretchActive.Get(f) <= 0.0f)
			{
				m_IsStretchActive = false;
			}

			// Enable grain phase lock if it became active this frame
			if (!m_IsGrainLockActive &&
				m_GrainLockActive.Get(f) > 0.0f)
			{
				m_IsGrainLockActive = true;
			}
			// Disable grain phase lock if it became inactive this frame
			else if (m_IsGrainLockActive &&
				m_GrainLockActive.Get(f) <= 0.0f)
			{
				m_IsGrainLockActive = false;
			}

			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				f32_t x = inBuffer(f, c);

				// Do stretch
				if (m_IsStretchActive)
				{
					f32_t stretchFactor = m_StretchFactor.Get(f);
					f32_t grainSize = m_GrainSize.Get(f);
					f32_t grainBlend = m_GrainBlend.Get(f);
					f32_t grainPhaseInc = m_GrainPhaseInc.Get(f);

					stretchFactor = std::max(stretchFactor, 1.0f);
					grainSize = std::max(grainSize, 100.0f);
					grainBlend = std::clamp(grainBlend, 0.01f, 0.49f);

					x = DoStretch(
						x,
						c,
						stretchFactor,
						grainSize,
						grainBlend,
						grainPhaseInc);
				}

				outBuffer(f, c) = x;
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

		if (m_SampleRate != sampleRate ||
			m_NumChannels != numChannels ||
			m_StretchTimeMs.Changed())
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

			for (auto &fillOffset : m_FillOffsets)
			{
				fillOffset = 0;
			}

			f32_t stretchTimeMs = m_StretchTimeMs.Get();
			m_NumCacheFrames = static_cast<count_t>((stretchTimeMs * sampleRate) / 1000.0f);
			NZ_ASSERT(m_NumCacheFrames != 0);
			m_Delay.Reset(m_NumCacheFrames);
		}

		m_NumFrames = numFrames;
		m_SampleRate = sampleRate;
		m_NumChannels = numChannels;
	}

	void SetStretchActive(Ref_t<FloatParameter> stretchActive)
	{
		m_StretchActive.Use(stretchActive);
	}

	void SetStretchTimeMs(Ref_t<FloatBlockParameter> stretchTimeMs)
	{
		m_StretchTimeMs.Use(stretchTimeMs);
	}

	void SetStretchFactor(Ref_t<FloatParameter> stretchFactor)
	{
		m_StretchFactor.Use(stretchFactor);
	}

	void SetGrainSize(Ref_t<FloatParameter> grainSize)
	{
		m_GrainSize.Use(grainSize);
	}

	void SetGrainBlend(Ref_t<FloatParameter> grainBlend)
	{
		m_GrainBlend.Use(grainBlend);
	}

	void SetGrainPhaseInc(Ref_t<FloatParameter> grainPhaseInc)
	{
		m_GrainPhaseInc.Use(grainPhaseInc);
	}

	void SetGrainLockActive(Ref_t<FloatParameter> grainLockActive)
	{
		m_GrainLockActive.Use(grainLockActive);
	}

private:
	inline f32_t DoStretch(
		f32_t x,
		f32_t c,
		f32_t stretchFactor,
		f32_t grainSize,
		f32_t grainBlend,
		f32_t grainPhaseInc)
	{
		if (static_cast<count_t>(m_FillOffsets[c]) <= m_NumCacheFrames)
		{
			m_Delay.Write(x, c);
			m_FillOffsets[c] += 1.0f;
		}
		
		// The stretch ratio
		f32_t stretchRatio = (1.0f / stretchFactor);
		// How far the input index should advance when launching a new grain
		f32_t grainOffset = grainSize * (1.0f - grainBlend) * stretchRatio;
		// The phase in which we should start blending in the next grain
		f32_t startBlendPhase = grainSize * (1.0f - grainBlend);
		// The phase in which we should stop blending out the old grain
		f32_t endBlendPhase = grainSize * grainBlend;

		f32_t y = x;

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
			f32_t t = 1.0f - (phi0 / endBlendPhase);
			
			f32_t x0 = g0 + phi0;
			f32_t x1 = g1 + phi1;

			f32_t y0 = m_Delay.Search(m_FillOffsets[c] - x0);
			f32_t y1 = m_Delay.Search(m_FillOffsets[c] - x1);

			y = y0 + (y1 - y0) * t;
		}
		// If playing one grain
		else
		{
			f32_t g = m_Grains[c][m_GrainPlayings[c]];
			f32_t phi = m_Phases[c][m_GrainPlayings[c]];

			f32_t x = g + phi;

			y = m_Delay.Search(m_FillOffsets[c] - x);
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
			count_t g = m_Grains[c][m_GrainPlayings[c]] + grainOffset;

			if (m_IsGrainLockActive)
			{
				// Lock the offset to the nearest zero crossing
				g = FindZeroCrossing(g, c, grainOffset);
			}

			// Start the next grain
			m_Phases[c][!m_GrainPlayings[c]] = 0.0f;
			m_Grains[c][!m_GrainPlayings[c]] = g;
			m_GrainPlayings[c] = 1 - m_GrainPlayings[c];
		}

		return y;
	}

	inline count_t FindZeroCrossing(
		count_t g,
		count_t c,
		count_t searchLength)
	{
		f32_t prevX  = 0.0f;
		f32_t currX  = 0.0f;

		// Scan backward to find zero-crossing
		for (count_t s = 0; s < searchLength; ++s)
		{
			currX = m_Delay.Search(m_FillOffsets[c] - g + s);

			// Check for zero crossing
			if (prevX <= 0.0f && currX > 0.0f)
			{
				return g - s;
			}

			prevX = currX;
		}

		return g;
	}

private:
	FloatSlotParameter m_StretchActive = 0.0f;
	FloatSlotParameter m_StretchFactor = 1.0f;
	FloatSlotBlockParameter m_StretchTimeMs = 10000.0f;
	FloatSlotParameter m_GrainSize = 2500.0f;
	FloatSlotParameter m_GrainBlend = 0.4f;
	FloatSlotParameter m_GrainPhaseInc = 1.0f;
	FloatSlotParameter m_GrainLockActive = 0.0f;

	bool m_IsStretchActive = false;
	bool m_IsGrainLockActive = false;
	std::array<std::array<f32_t, 2>, k_MaxChannels> m_Phases;
	std::array<std::array<f32_t, 2>, k_MaxChannels> m_Grains;
	std::array<count_t, k_MaxChannels> m_GrainPlayings;
	std::array<f32_t, k_MaxChannels> m_FillOffsets;
	count_t m_NumCacheFrames = 0;
	Delay<f32_t, k_MaxChannels> m_Delay;

	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;
};

Ref_t<TimeStretcher> TimeStretcher::Create()
{
	return MakeRef<TimeStretcher>(MakeOwn<Impl>());
}

NOIS_INTERFACE_IMPL(TimeStretcher)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchActive, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchTimeMs, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchFactor, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainSize, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainBlend, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainPhaseInc, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainLockActive, FloatParameter)

}

