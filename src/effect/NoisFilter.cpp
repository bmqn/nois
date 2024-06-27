#include "nois/effect/NoisFilter.hpp"

#include "nois/NoisUtil.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <mutex>
#include <numbers>
#include <vector>

namespace nois {

class N2ButterworthFilterImpl : public Filter
{
public:
	
	N2ButterworthFilterImpl(std::shared_ptr<Stream> stream)
		: m_Stream(stream)
	{
		CalculateCoefficients();
	}

	virtual count_t Consume(data_t *data, count_t len) override
	{
		if (m_Stream)
		{
			count_t count = m_Stream->Consume(data, len);

			if (!m_ParamLocker.load(std::memory_order_acquire))
			{
				for (int32_t i = 0; i < count; i += m_NumChannels)
				{
					for (int32_t j = 0; j < m_NumChannels; ++j)
					{
						data[i + j] = ConsumeChannel(j, data[i + j]);
					}
				}
			}

			return count;
		}

		return 0;
	}

	virtual data_t GetCutoff() override
	{
		return m_Cutoff;
	}

	virtual void SetNumChannels(int32_t numChannels) override
	{
		int32_t oldNumChannels = m_NumChannels;
		m_NumChannels = numChannels;
		if (m_NumChannels != numChannels)
		{
			CalculateCoefficients();
		}
	}

	virtual void SetSampleRate(int32_t sampleRate) override
	{
		data_t oldSampleRate = m_SampleRate;
		m_SampleRate = static_cast<data_t>(sampleRate);
		if (oldSampleRate != sampleRate)
		{
			CalculateCoefficients();
		}
	}
	virtual void SetCutoff(data_t cutoff) override
	{
		data_t oldCutoff = m_Cutoff;
		m_Cutoff = std::clamp(cutoff, 20.0f, m_SampleRate / 2 - 20.0f);
		if (oldCutoff != cutoff)
		{
			CalculateCoefficients();
		}
	}

private:
	void CalculateCoefficients()
	{
		m_ParamLocker.store(true, std::memory_order_release);

		constexpr data_t pi = std::numbers::pi;
		constexpr data_t sqrt2 = std::numbers::sqrt2;
		data_t wc = std::tan(pi * (m_Cutoff / m_SampleRate));
		data_t invwc = 1.0f / wc;

		m_B0 = 1.0f / (1.0f + sqrt2 * invwc + invwc * invwc);
		m_B1 = 2.0f * m_B0;
		m_B2 = m_B0;
		m_A1 = -2.0f * (invwc * invwc - 1.0f) * m_B0;
		m_A2 = (1.0f - sqrt2 * invwc + invwc * invwc) * m_B0;

		if (m_Inps.size() != m_NumChannels)
		{
			m_Inps.resize(m_NumChannels, WindowStream<data_t>(2));
		}

		if (m_Outs.size() != m_NumChannels)
		{
			m_Outs.resize(m_NumChannels, WindowStream<data_t>(2));
		}

		m_ParamLocker.store(false, std::memory_order_release);
	}

	data_t ConsumeChannel(size_t index, data_t input)
	{
		data_t output = input;
		WindowStream<data_t> &inpWindow = m_Inps[index];
		WindowStream<data_t> &outWindow = m_Outs[index];

		output = m_B0 * input + m_B1 * inpWindow.Get(1) +
			m_B2 * inpWindow.Get(2) - m_A1 * outWindow.Get(1)
			- m_A2 * outWindow.Get(2);

		inpWindow.Add(input);
		outWindow.Add(output);

		return output;
	}

private:
	std::shared_ptr<Stream> m_Stream;

	int32_t m_NumChannels = 2;

	data_t m_Cutoff = 11025.0f;
	data_t m_SampleRate = 44100.0f;

	data_t m_A0, m_A1 = 0.0f, m_A2 = 0.0f;
	data_t m_B0 = 0.0f, m_B1 = 0.0f, m_B2 = 0.0f;

	std::vector<WindowStream<data_t>> m_Inps;
	std::vector<WindowStream<data_t>> m_Outs;

	std::atomic_bool m_ParamLocker;
};

std::shared_ptr<Filter> CreateFilter(std::shared_ptr<Stream> stream, Filter::Kind kind)
{
	switch (kind)
	{
	case nois::Filter::k_N2ButterWorth:
		return std::make_shared<N2ButterworthFilterImpl>(stream);
	default:
		break;
	}

	return nullptr;
}

};

