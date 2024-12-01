#include "nois/effect/NoisSignalDelayer.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class SignalDelayerImpl : public SignalDelayer
{
public:
	
	SignalDelayerImpl(Stream *stream)
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
				for (count_t j = 0; j < static_cast<count_t>(numChannels); ++j)
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
		bool delayChanged = m_DelayChanged.load(std::memory_order_acquire);

		if (m_SampleRate != sampleRate ||
			m_NumChannels != numChannels ||
			delayChanged)
		{
			data_t delayMs = m_DelayMs.load(std::memory_order_acquire);
			count_t delaySamples = static_cast<count_t>(delayMs / 1000.0f) * sampleRate;
			
			m_Inps.clear();
			m_Inps.resize(numChannels, WindowStream<data_t>(delaySamples));
		
			m_SampleRate = sampleRate;
			m_NumChannels = numChannels;

			if (delayChanged)
			{
				m_DelayChanged.store(false, std::memory_order_release);
			}
		}

		m_Stream->PrepareToConsume(
			numSamples,
			sampleRate,
			numChannels);
	}

	virtual data_t GetDelayMs() override
	{
		return m_DelayMs.load(std::memory_order_acquire);
	}

	virtual void SetDelayMs(data_t delayMs) override
	{
		if (m_DelayMs.load(std::memory_order_acquire) != delayMs)
		{
			m_DelayMs.store(delayMs, std::memory_order_release);
			m_DelayChanged.store(true, std::memory_order_release);
		}
	}

private:
	data_t ConsumeChannel(count_t index, data_t input)
	{
		data_t output = input;

		if (m_Inps[index].GetSize() > 0)
		{
			output = m_Inps[index].Get(-1);
			m_Inps[index].Add(input);
		}

		return output;
	}

private:
	Stream *m_Stream;

	std::atomic<data_t> m_DelayMs = 2000.0f;
	std::atomic_bool m_DelayChanged = false;

	int32_t m_SampleRate = 0;
	int32_t m_NumChannels = 0;

	std::vector<WindowStream<data_t>> m_Inps;
};

std::shared_ptr<SignalDelayer> CreateSignalDelayer(Stream *stream)
{
	return std::make_shared<SignalDelayerImpl>(stream);
}

};

