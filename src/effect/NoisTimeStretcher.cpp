#include "nois/effect/NoisTimeStretcher.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class TimeStretcherImpl : public TimeStretcher
{
public:
	TimeStretcherImpl(Stream *stream)
		: m_Stream(stream)
	{
	}

	virtual count_t Consume(data_t *data,
	                        count_t numSamples,
	                        int32_t sampleRate,
	                        int32_t numChannels) override
	{
		if (m_Stream)
		{
			count_t count = m_Stream->Consume(
				data,
				numSamples,
				sampleRate,
				numChannels);
			
			for (count_t i = 0; i < count; i += numChannels)
			{
				bool stretchActiveChanged = m_StretchActiveChanged.load(std::memory_order_acquire);
				if (!stretchActiveChanged && m_StretchActiveFunc)
				{
					const bool stretchActive = m_StretchActiveFunc();
					if (m_StretchActive.load(std::memory_order_acquire) != stretchActive)
					{
						m_StretchActive.store(stretchActive, std::memory_order_release);
						stretchActiveChanged = true;
					}
				}

				const float stretchFactor = m_StretchFactor->Get();
				const float grainSize = m_GrainSize->Get();
				const data_t grainBlend = m_GrainBlend.load(std::memory_order_acquire);

				m_GrainOffset = grainSize * (1.0f - grainBlend) * (1.0f / stretchFactor);

				if (stretchActiveChanged)
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

					m_StretchActiveChanged.store(false, std::memory_order_release);
				}

				for (count_t j = 0; j < static_cast<count_t>(numChannels); ++j)
				{
					data[i + j] = ConsumeChannel(j, data[i + j], 1.0f);
				}
			}

			return count;
		}

		return 0;
	}

	virtual void PrepareToConsume(count_t numSamples,
	                              int32_t sampleRate,
	                              int32_t numChannels) override
	{
		bool stretchActiveChanged = m_StretchActiveChanged.load(std::memory_order_acquire);
		if (!stretchActiveChanged && m_StretchActiveFunc)
		{
			const bool stretchActive = m_StretchActiveFunc();
			if (m_StretchActive.load(std::memory_order_acquire) != stretchActive)
			{
				m_StretchActive.store(stretchActive, std::memory_order_release);
				stretchActiveChanged = true;
			}
		}

		const bool stretchTimeChanged = m_StretchTimeChanged.load(std::memory_order_acquire);
		const bool grainBlendChanged = m_GrainBlendChanged.load(std::memory_order_acquire);

		if (m_SampleRate != sampleRate ||
		    m_NumChannels != numChannels ||
		    stretchActiveChanged ||
		    stretchTimeChanged ||
		    grainBlendChanged)
		{
			const bool stretchActive = m_StretchActive.load(std::memory_order_acquire);
			const data_t stretchTimeMs = m_StretchTimeMs.load(std::memory_order_acquire);
			const float stretchFactor = m_StretchFactor->Get();
			const float grainSize = m_GrainSize->Get();
			const data_t grainBlend = m_GrainBlend.load(std::memory_order_acquire);

			m_GrainOffset = grainSize * (1.0f - grainBlend) * (1.0f / stretchFactor);

			if (m_SampleRate != sampleRate || m_NumChannels != numChannels || stretchTimeChanged)
			{
				m_Phases.resize(numChannels, { data_t{ 0 }, data_t{ -1 } });
				m_Grains.resize(numChannels, { data_t{ 0 }, data_t{ 0 } });
				m_GrainPlayings.resize(numChannels, 0);

				count_t timeStretchSamples = static_cast<data_t>(stretchTimeMs / 1000.0f) * sampleRate;

				for (auto &samples : m_Samples)
				{
					samples.Resize(timeStretchSamples);
				}

				m_Samples.resize(numChannels, WindowStream<data_t>(timeStretchSamples));
			}

			if (stretchActiveChanged || stretchTimeChanged)
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

			m_SampleRate = sampleRate;
			m_NumChannels = numChannels;

			if (stretchActiveChanged)
			{
				m_StretchActiveChanged.store(false, std::memory_order_release);
			}

			if (stretchTimeChanged)
			{
				m_StretchTimeChanged.store(false, std::memory_order_release);
			}

			if (grainBlendChanged)
			{
				m_GrainBlendChanged.store(false, std::memory_order_release);
			}
		}

		m_Stream->PrepareToConsume(
			numSamples,
			sampleRate,
			numChannels);
	}

	virtual bool GetStretchActive() override
	{
		return m_StretchActive.load(std::memory_order_acquire);
	}

	virtual void SetStretchActive(bool stretchActive) override
	{
		if (m_StretchActive.load(std::memory_order_acquire) != stretchActive)
		{
			m_StretchActive.store(stretchActive, std::memory_order_release);
			m_StretchActiveChanged.store(true, std::memory_order_release);
		}
	}

	virtual void BindStretchActive(std::function<bool()> stretchActiveFunc) override
	{
		m_StretchActiveFunc = stretchActiveFunc;
	}

	virtual data_t GetStretchTimeMs() override
	{
		return m_StretchTimeMs.load(std::memory_order_acquire);
	}

	virtual void SetStretchTimeMs(data_t stretchTimeMs) override
	{
		stretchTimeMs = std::max(0.0f, stretchTimeMs);

		if (m_StretchTimeMs.load(std::memory_order_acquire) != stretchTimeMs)
		{
			m_StretchTimeMs.store(stretchTimeMs, std::memory_order_release);
			m_StretchTimeChanged.store(true, std::memory_order_release);
		}
	}

	virtual const FloatParameter &GetStretchFactor() const override
	{
		return *m_StretchFactor;
	}

	virtual const FloatParameter &GetGrainSize() const override
	{
		return *m_GrainSize;
	}

	virtual data_t GetGrainBlend() override
	{
		return m_GrainBlend.load(std::memory_order_acquire);
	}

	virtual void SetGrainBlend(data_t grainBlend) override
	{
		grainBlend = std::clamp(grainBlend, 0.05f, 0.45f);

		if (m_GrainBlend.load(std::memory_order_acquire) != grainBlend)
		{
			m_GrainBlend.store(grainBlend, std::memory_order_release);
			m_GrainBlendChanged.store(true, std::memory_order_release);
		}
	}

private:
	virtual void SetStretchFactorImpl(Own_t<FloatParameter> &&stretchFactor) override
	{
		m_StretchFactor = std::move(stretchFactor);
	}

	virtual void SetGrainSizeImpl(Own_t<FloatParameter> &&grainSize) override
	{
		m_GrainSize = std::move(grainSize);
	}

	data_t ConsumeChannel(count_t index, data_t input, data_t pitchRatio)
	{
		const bool stretchActive = m_StretchActive.load(std::memory_order_acquire);
		const float grainSize = m_GrainSize->Get();
		const data_t grainBlend = m_GrainBlend.load(std::memory_order_acquire);

		data_t output = input;

		if (m_Samples[index].GetCount() < m_Samples[index].GetSize())
		{
			m_Samples[index].Add(input);
		}

		if (stretchActive)
		{
			if ((m_Phases[index][0] > -1.0f) && (m_Phases[index][1] > -1.0f))
			{
				data_t m = 1.0f / (grainSize * grainBlend);
				data_t phi = m_Phases[index][m_GrainPlayings[index]];
				data_t window = m * phi;
				
				data_t x0 = m_Grains[index][m_GrainPlayings[index]] + m_Phases[index][m_GrainPlayings[index]];
				data_t x1 = m_Grains[index][!m_GrainPlayings[index]] + m_Phases[index][!m_GrainPlayings[index]];
				count_t i0 = static_cast<count_t>(m_Samples[index].GetOffset() - x0);
				count_t i1 = static_cast<count_t>(m_Samples[index].GetOffset() - x1);

				output = m_Samples[index].Get(i0) * window + m_Samples[index].Get(i1) * (1.0f - window);
			}
			else
			{
				data_t x = m_Grains[index][m_GrainPlayings[index]] + m_Phases[index][m_GrainPlayings[index]];
				count_t i = static_cast<count_t>(m_Samples[index].GetOffset() - x);

				output = m_Samples[index].Get(i);
			}

			m_Phases[index][m_GrainPlayings[index]] += pitchRatio;

			if (m_Phases[index][!m_GrainPlayings[index]] > -1.0f)
			{
				m_Phases[index][!m_GrainPlayings[index]] += pitchRatio;
			}

			if (m_Phases[index][!m_GrainPlayings[index]] >= grainSize)
			{
				m_Phases[index][!m_GrainPlayings[index]] = -1.0f;
			}

			if (m_Phases[index][m_GrainPlayings[index]] >= grainSize * (1.0f - grainBlend))
			{
				m_Phases[index][!m_GrainPlayings[index]] = 0.0f;
				m_Grains[index][!m_GrainPlayings[index]] = m_Grains[index][m_GrainPlayings[index]] + m_GrainOffset;
				m_GrainPlayings[index] = 1 - m_GrainPlayings[index];
			}
		}

		return output;
	}

private:
	Stream *m_Stream;

	std::atomic_bool m_StretchActive = false;
	std::atomic_bool m_StretchActiveChanged = false;
	std::function<bool()> m_StretchActiveFunc;

	std::atomic<data_t> m_StretchTimeMs = 1000.0f;
	std::atomic_bool m_StretchTimeChanged = false;

	Own_t<FloatParameter> m_StretchFactor = MakeOwn<FloatConstantParameter>(1.0f);

	Own_t<FloatParameter> m_GrainSize = MakeOwn<FloatConstantParameter>(1.0f);

	std::atomic<data_t> m_GrainBlend = 0.1f;
	std::atomic_bool m_GrainBlendChanged = false;

	int32_t m_SampleRate = 0;
	int32_t m_NumChannels = 0;

	std::vector<std::array<data_t, 2>> m_Phases;
	std::vector<std::array<data_t, 2>> m_Grains;
	std::vector<count_t> m_GrainPlayings;
	data_t m_GrainOffset = 0.0f;

	std::vector<WindowStream<data_t>> m_Samples;
};

Ref_t<TimeStretcher> CreateTimeStretcher(Stream *stream)
{
	return MakeRef<TimeStretcherImpl>(stream);
}

};

