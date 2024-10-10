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
	
	N2ButterworthFilterImpl(Stream *stream)
		: m_Stream(stream)
	{
		CalculateCoefficients();
	}

	virtual count_t Consume(data_t *data,
	                        count_t numSamples,
	                        int32_t sampleRate,
	                        int32_t numChannels) override
	{
		if (m_Stream)
		{
			count_t count = m_Stream->Consume(data, numSamples,
				sampleRate, numChannels);
			
			for (int32_t i = 0; i < count; i += numChannels)
			{
				for (int32_t j = 0; j < numChannels; ++j)
				{
					data[i + j] = ConsumeChannel(j, data[i + j]);
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
		if (m_SampleRate != sampleRate || m_NumChannels != numChannels)
		{
			m_Inps.resize(numChannels, WindowStream<data_t>(2));
			m_Outs.resize(numChannels, WindowStream<data_t>(2));
		
			CalculateCoefficients();
		
			m_SampleRate = sampleRate;
			m_NumChannels = numChannels;
		}

		if (m_Stream)
		{
			m_Stream->PrepareToConsume(numSamples, sampleRate, numChannels);
		}
	}

	virtual data_t GetCutoffRatio() override
	{
		return m_CutoffRatio;
	}

	virtual void SetCutoffRatio(data_t cutoffRatio) override
	{
		m_CutoffRatio = std::clamp(cutoffRatio, 0.0f, 1.0f);

		CalculateCoefficients();
	}

private:
	void CalculateCoefficients()
	{
		constexpr data_t pi = static_cast<data_t>(std::numbers::pi);
		constexpr data_t sqrt2 = static_cast<data_t>(std::numbers::sqrt2);
		data_t wc = std::tan(pi * std::clamp(m_CutoffRatio, 0.001f, 0.999f) * 0.5f);
		data_t invwc = 1.0f / wc;

		m_B0 = 1.0f / (1.0f + sqrt2 * invwc + invwc * invwc);
		m_B1 = 2.0f * m_B0;
		m_B2 = m_B0;
		m_A1 = -2.0f * (invwc * invwc - 1.0f) * m_B0;
		m_A2 = (1.0f - sqrt2 * invwc + invwc * invwc) * m_B0;
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
	Stream *m_Stream;

	data_t m_CutoffRatio = 1.0f;

	data_t m_A1 = 0.0f, m_A2 = 0.0f;
	data_t m_B0 = 0.0f, m_B1 = 0.0f, m_B2 = 0.0f;

	int32_t m_SampleRate = 0;
	int32_t m_NumChannels = 0;

	std::vector<WindowStream<data_t>> m_Inps;
	std::vector<WindowStream<data_t>> m_Outs;
};

std::shared_ptr<Filter> CreateFilter(Stream *stream, Filter::Kind kind)
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

