#include "nois/effect/NoisTimeStretcher.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class TimeStretcherImpl : public TimeStretcher
{
public:
	TimeStretcherImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
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
			
		for (count_t i = 0; i < buffer.GetNumFrames(); ++i)
		{
			const f32_t stretchFactor = m_StretchFactor->Get(i);
			const f32_t grainSize = m_GrainSize->Get(i);
			const f32_t grainBlend = m_GrainBlend->Get(i);

			m_GrainOffset = grainSize * (1.0f - grainBlend) * (1.0f / stretchFactor);

			if (m_StretchActive->Changed(i))
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

				for (auto &samples : m_Samples)
				{
					samples.Wipe();
				}
			}

			for (count_t j = 0; j < buffer.GetNumChannels(); ++j)
			{
				buffer(i, j) = ConsumeChannel(j, buffer(i, j), 1.0f);
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
			m_NumChannels != numChannels ||
			m_StretchTimeMs->Changed(0))
		{
			const f32_t stretchTimeMs = m_StretchTimeMs->Get(0);

			count_t timeStretchSamples = static_cast<count_t>((stretchTimeMs * sampleRate) / 1000.0f);

			m_Phases.resize(numChannels, { f32_t{ 0 }, f32_t{ -1 } });
			m_Grains.resize(numChannels, { f32_t{ 0 }, f32_t{ 0 } });
			m_GrainPlayings.resize(numChannels, 0);

			for (auto &samples : m_Samples)
			{
				samples.Resize(timeStretchSamples);
			}

			m_Samples.resize(numChannels, WindowStream<f32_t>(timeStretchSamples));

			m_SampleRate = sampleRate;
			m_NumChannels = numChannels;
		}
	}

	virtual Ref_t<FloatParameter> GetStretchActive() override
	{
		return m_StretchActive;
	}

	virtual void SetStretchActive(Ref_t<FloatParameter> stretchActive) override
	{
		m_StretchActive = stretchActive;
	}

	virtual Ref_t<FloatParameter> GetStretchTimeMs() override
	{
		return m_StretchTimeMs;
	}

	virtual void SetStretchTimeMs(Ref_t<FloatParameter> stretchTimeMs) override
	{
		m_StretchTimeMs = stretchTimeMs;
	}

	virtual Ref_t<FloatParameter> GetStretchFactor() override
	{
		return m_StretchFactor;
	}

	virtual void SetStretchFactor(Ref_t<FloatParameter> stretchFactor) override
	{
		m_StretchFactor = stretchFactor;
	}

	virtual Ref_t<FloatParameter> GetGrainSize() override
	{
		return m_GrainSize;
	}

	virtual void SetGrainSize(Ref_t<FloatParameter> grainSize) override
	{
		m_GrainSize = grainSize;
	}

	virtual Ref_t<FloatParameter> GetGrainBlend() override
	{
		return m_GrainBlend;
	}

	virtual void SetGrainBlend(Ref_t<FloatParameter> grainBlend) override
	{
		m_GrainBlend = grainBlend;
	}

private:
	f32_t ConsumeChannel(count_t frameIndex, f32_t input, f32_t pitchRatio)
	{
		const bool stretchActive = m_StretchActive->Get(frameIndex) > 0.0f;
		const f32_t grainSize = m_GrainSize->Get(frameIndex);
		const f32_t grainBlend = m_GrainBlend->Get(frameIndex);

		f32_t output = input;

		if (m_Samples[frameIndex].GetCount() < m_Samples[frameIndex].GetSize())
		{
			m_Samples[frameIndex].Add(input);
		}

		if (stretchActive)
		{
			if ((m_Phases[frameIndex][0] > -1.0f) && (m_Phases[frameIndex][1] > -1.0f))
			{
				f32_t m = 1.0f / (grainSize * grainBlend);
				f32_t phi = m_Phases[frameIndex][m_GrainPlayings[frameIndex]];
				f32_t window = m * phi;
				
				f32_t x0 = m_Grains[frameIndex][m_GrainPlayings[frameIndex]] + m_Phases[frameIndex][m_GrainPlayings[frameIndex]];
				f32_t x1 = m_Grains[frameIndex][!m_GrainPlayings[frameIndex]] + m_Phases[frameIndex][!m_GrainPlayings[frameIndex]];
				count_t i0 = static_cast<count_t>(m_Samples[frameIndex].GetOffset() - x0);
				count_t i1 = static_cast<count_t>(m_Samples[frameIndex].GetOffset() - x1);

				output = m_Samples[frameIndex].Get(i0) * window + m_Samples[frameIndex].Get(i1) * (1.0f - window);
			}
			else
			{
				f32_t x = m_Grains[frameIndex][m_GrainPlayings[frameIndex]] + m_Phases[frameIndex][m_GrainPlayings[frameIndex]];
				count_t i = static_cast<count_t>(m_Samples[frameIndex].GetOffset() - x);

				output = m_Samples[frameIndex].Get(i);
			}

			m_Phases[frameIndex][m_GrainPlayings[frameIndex]] += pitchRatio;

			if (m_Phases[frameIndex][!m_GrainPlayings[frameIndex]] > -1.0f)
			{
				m_Phases[frameIndex][!m_GrainPlayings[frameIndex]] += pitchRatio;
			}

			if (m_Phases[frameIndex][!m_GrainPlayings[frameIndex]] >= grainSize)
			{
				m_Phases[frameIndex][!m_GrainPlayings[frameIndex]] = -1.0f;
			}

			if (m_Phases[frameIndex][m_GrainPlayings[frameIndex]] >= grainSize * (1.0f - grainBlend))
			{
				m_Phases[frameIndex][!m_GrainPlayings[frameIndex]] = 0.0f;
				m_Grains[frameIndex][!m_GrainPlayings[frameIndex]] = m_Grains[frameIndex][m_GrainPlayings[frameIndex]] + m_GrainOffset;
				m_GrainPlayings[frameIndex] = 1 - m_GrainPlayings[frameIndex];
			}
		}

		return output;
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_StretchActive = MakeRef<FloatConstantParameter>(0.0f);
	Ref_t<FloatParameter> m_StretchTimeMs = MakeRef<FloatConstantParameter>(1000.0f);
	Ref_t<FloatParameter> m_StretchFactor = MakeRef<FloatConstantParameter>(1.0f);
	Ref_t<FloatParameter> m_GrainSize = MakeOwn<FloatConstantParameter>(1.0f);
	Ref_t<FloatParameter> m_GrainBlend = MakeRef<FloatConstantParameter>(0.1f);

	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;

	std::vector<std::array<f32_t, 2>> m_Phases;
	std::vector<std::array<f32_t, 2>> m_Grains;
	std::vector<count_t> m_GrainPlayings;
	f32_t m_GrainOffset = 0.0f;

	std::vector<WindowStream<f32_t>> m_Samples;
};

Ref_t<TimeStretcher> CreateTimeStretcher(Ref_t<Stream> stream)
{
	return MakeRef<TimeStretcherImpl>(stream);
}

}

