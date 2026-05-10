#include "nois/effect/NoisTimeStretcher.hpp"

#include "NoisLog.h"
#include "NoisMacros.hpp"
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

		auto stretchActiveBlock = m_StretchActive.Get();
		auto stretchFactorBlock = m_StretchFactor.Get();
		auto grainSizeBlock = m_GrainSize.Get();
		auto grainBlendBlock = m_GrainBlend.Get();
		auto grainPhaseIncBlock = m_GrainPhaseInc.Get();
		auto grainLockActiveBlock = m_GrainLockActive.Get();

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			// Enable stretch if it became active this frame
			if (!m_IsStretchActive &&
				stretchActiveBlock.Get(f) > 0.0f)
			{
				for (count_t c = 0; c < m_GrainBases.size(); ++c)
				{
					m_GrainBases[c] = m_Delay.GetOffset(c);

					m_Phases[c][0] = 0.0f;
					m_Phases[c][1] = -1.0f;
					m_Grains[c][0] = 0.0f;
					m_Grains[c][1] = 0.0f;
					m_GrainReads[c][0] = 0.0f;
					m_GrainReads[c][1] = 0.0f;
					m_GrainPlayings[c] = 0;
				}

				m_Delay.RunUntilFull();

				m_IsStretchActive = true;
			}
			// Disable stretch if it became inactive this frame
			else if (m_IsStretchActive &&
				stretchActiveBlock.Get(f) <= 0.0f)
			{
				m_IsStretchActive = false;
			}

			// Enable grain phase lock if it became active this frame
			if (!m_IsGrainLockActive &&
				grainLockActiveBlock.Get(f) > 0.0f)
			{
				m_IsGrainLockActive = true;
			}
			// Disable grain phase lock if it became inactive this frame
			else if (m_IsGrainLockActive &&
				grainLockActiveBlock.Get(f) <= 0.0f)
			{
				m_IsGrainLockActive = false;
			}

			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				f32_t x = inBuffer(f, c);

				m_Delay.Write(x, c);
				
				// Do stretch
				if (m_IsStretchActive)
				{
					f32_t stretchFactor = stretchFactorBlock.Get(f);
					f32_t grainSize = grainSizeBlock.Get(f);
					f32_t grainBlend = grainBlendBlock.Get(f);
					f32_t grainPhaseInc = grainPhaseIncBlock.Get(f);

					stretchFactor = std::max(stretchFactor, 1.0f);
					grainSize = std::max(grainSize, 1.0f);
					grainBlend = std::clamp(grainBlend, 0.0f, 0.5f);

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

		bool stretchTimeMsChanged = m_StretchTimeMs.PollChanged();

		if (m_SampleRate != sampleRate ||
			m_NumChannels != numChannels ||
			stretchTimeMsChanged)
		{
			const f32_t stretchTimeMs = m_StretchTimeMs.Get();
			m_StetchNumFrames = (stretchTimeMs * sampleRate) / 1000.0f;
			m_Delay.Configure(static_cast<count_t>(m_StetchNumFrames));
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_SampleRate = sampleRate;
	}

	void SetStretchTimeMs(Ref_t<FloatBlockParameter> stretchTimeMs)
	{
		m_StretchTimeMs.Use(stretchTimeMs);
	}

	void SetStretchActive(Ref_t<FloatParameter> stretchActive)
	{
		m_StretchActive.Use(stretchActive);
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
		count_t c,
		f32_t stretchFactor,
		f32_t grainSize,
		f32_t grainBlend,
		f32_t grainPhaseInc)
	{
		f32_t grainPhaseIncAbs = std::abs(grainPhaseInc);
		f32_t grainPhaseIncDir = grainPhaseInc >= 0.0f ? 1.0f : -1.0f;
		
		// The stretch ratio
		f32_t stretchRatio = grainPhaseIncDir * (1.0f / stretchFactor);
		// How far the input index should advance when launching a new grain
		f32_t grainOffset = grainSize * (1.0f - grainBlend) * stretchRatio;
		// The length of the grain blend
		f32_t blendLength = grainSize * grainBlend;
		// The phase in which we should start blending in the next grain
		f32_t startBlendPhase = grainSize * (1.0f - grainBlend);

		f32_t y = x;

		// If blending both grains
		if (m_Phases[c][m_GrainPlayings[c]] >= 0.0f &&
			m_Phases[c][!m_GrainPlayings[c]] >= 0.0f)
		{
			f32_t g0 = m_GrainReads[c][m_GrainPlayings[c]];
			f32_t g1 = m_GrainReads[c][!m_GrainPlayings[c]];
			f32_t phi0 = m_Phases[c][m_GrainPlayings[c]];
			f32_t phi1 = m_Phases[c][!m_GrainPlayings[c]];

			// Zero when new grain just started
			// One when old grain has stopped
			f32_t t = std::clamp(phi0 / blendLength, 0.0f, 1.0f);
			
			f32_t x0 = g0 + phi0 * grainPhaseIncDir;
			f32_t x1 = g1 + phi1 * grainPhaseIncDir;
			f32_t y0 = m_Delay.Get(m_GrainBases[c] + x0, c);
			f32_t y1 = m_Delay.Get(m_GrainBases[c] + x1, c);

			// 100% of old grain when new grain just started
			// 100% of new grain when old grain has stopped
			y = y1 + (y0 - y1) * t;
		}
		// If playing one grain
		else
		{
			f32_t g = m_GrainReads[c][m_GrainPlayings[c]];
			f32_t phi = m_Phases[c][m_GrainPlayings[c]];

			f32_t x = g + phi * grainPhaseIncDir;

			y = m_Delay.Get(m_GrainBases[c] + x, c);
		}

		// Increment current grain phase
		m_Phases[c][m_GrainPlayings[c]] += grainPhaseIncAbs;

		// Increment previous grain phase, if it's blending out
		if (m_Phases[c][!m_GrainPlayings[c]] >= 0.0f)
		{
			// Increment previous grain phase
			m_Phases[c][!m_GrainPlayings[c]] += grainPhaseIncAbs;

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
			f32_t g = m_Grains[c][m_GrainPlayings[c]] + grainOffset;
			if (g < 0.0f)
			{
				g += m_StetchNumFrames;
			}
			else if (g >= m_StetchNumFrames)
			{
				g -= m_StetchNumFrames;
			}

			f32_t gRead = g;
			if (m_IsGrainLockActive)
			{
				// Lock the offset to the nearest zero crossing
				gRead = FindZeroCrossing(g, c, std::abs(grainOffset), grainPhaseIncDir);
			}

			// Start the next grain
			m_Phases[c][!m_GrainPlayings[c]] = 0.0f;
			m_Grains[c][!m_GrainPlayings[c]] = g;
			m_GrainReads[c][!m_GrainPlayings[c]] = gRead;
			m_GrainPlayings[c] = 1 - m_GrainPlayings[c];
		}

		return y;
	}

	inline f32_t FindZeroCrossing(
		f32_t g,
		count_t c,
		f32_t searchLength,
		f32_t grainPhaseIncDir)
	{
		f32_t prevX  = 0.0f;
		f32_t currX  = 0.0f;

		// Scan backward to find zero-crossing
		for (count_t phi = 0; phi < searchLength; ++phi)
		{
			f32_t x = g - phi * grainPhaseIncDir;

			currX = m_Delay.Get(m_GrainBases[c] + x, c);

			// Check for zero crossing
			if (prevX < 0.0f && currX > 0.0f)
			{
				return x;
			}

			prevX = currX;
		}

		return g;
	}

private:
	FloatSlotBlockParameter m_StretchTimeMs = { 5000.0f, 50.0f, 25000.0f };
	FloatSlotParameter m_StretchActive = 0.0f;
	FloatSlotParameter m_StretchFactor = 1.0f;
	FloatSlotParameter m_GrainSize = 2500.0f;
	FloatSlotParameter m_GrainBlend = 0.5f;
	FloatSlotParameter m_GrainPhaseInc = 1.0f;
	FloatSlotParameter m_GrainLockActive = 0.0f;

	bool m_IsStretchActive = false;
	bool m_IsGrainLockActive = false;
	std::array<std::array<f32_t, 2>, k_MaxChannels> m_Phases = { {0.0f, -1.0f} };
	std::array<std::array<f32_t, 2>, k_MaxChannels> m_Grains = { {0.0f, 0.0f} };
	std::array<std::array<f32_t, 2>, k_MaxChannels> m_GrainReads = { {0.0f, 0.0f} };
	std::array<count_t, k_MaxChannels> m_GrainPlayings = { 0 };
	std::array<count_t, k_MaxChannels> m_GrainBases = { 0 };
	f32_t m_StetchNumFrames = 0.0f;
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
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchTimeMs, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchActive, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, StretchFactor, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainSize, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainBlend, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainPhaseInc, FloatParameter)
NOIS_INTERFACE_PARAM_IMPL(TimeStretcher, GrainLockActive, FloatParameter)

}

