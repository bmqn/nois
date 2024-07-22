#include "nois/route/NoisSplitter.hpp"

#include "nois/NoisUtil.hpp"

#include <cstring>
#include <unordered_map>

namespace nois {

class SplitterImpl : public Splitter
{
	friend class SplitterStreamImpl;

public:
	SplitterImpl(std::shared_ptr<Channel> channel)
		: m_Channel(channel)
	{
	}

	virtual std::shared_ptr<Channel> GetChannel() override;

private:
	count_t Consume(data_t *data,
					count_t numSamples,
					int32_t sampleRate,
					int32_t numChannels)
	{
		if (m_Channel->GetStream())
		{
			if (m_SampleRate != sampleRate ||
				m_NumChannels != numChannels)
			{
				ASSERT(numChannels * sampleRate >= numSamples);
				m_Buffer.resize(numChannels * sampleRate);

				m_SampleRate = sampleRate;
				m_NumChannels = numChannels;
			}

			count_t count = m_Channel->GetStream()->Consume(
				m_Buffer.data() + m_NumSamplesConsumed,
				numSamples, sampleRate, numChannels);

			std::memcpy(data, m_Buffer.data() + m_NumSamplesConsumed,
				count);

			m_NumSamplesConsumed += count;

			return count;
		}

		return 0;
	}

	count_t ConsumeFromCache(data_t *data,
							 count_t numSamples,
							 int32_t sampleRate,
							 int32_t numChannels)
	{
		if (m_Channel->GetStream())
		{
			if (m_SampleRate != sampleRate ||
				m_NumChannels != numChannels)
			{
				ASSERT(numChannels * sampleRate >= numSamples);
				m_Buffer.resize(numChannels * sampleRate);

				m_SampleRate = sampleRate;
				m_NumChannels = numChannels;
			}

			count_t count = m_Channel->GetStream()->Consume(m_Buffer.data(), numSamples,
				sampleRate, numChannels);

			return count;
		}

		return 0;
	}

private:
	std::shared_ptr<Channel> m_Channel;
	std::vector<std::shared_ptr<Channel>> m_Channels;

	int32_t m_SampleRate = 0;
	int32_t m_NumChannels = 0;

	std::vector<data_t> m_Buffer;
	count_t m_NumSamplesConsumed = 0;
};

class SplitterStreamImpl : public Stream
{
public:
	SplitterStreamImpl(SplitterImpl *splitter)
		: m_Splitter(splitter)
	{
	}

	virtual count_t Consume(data_t *data,
							count_t numSamples,
							int32_t sampleRate,
							int32_t numChannels) override
	{
		count_t count = 0;

		if (m_NumSamplesConsumed + numSamples > m_Splitter->m_NumSamplesConsumed)
		{
			count = m_Splitter->Consume(data, numSamples,
				sampleRate, numChannels);
		}
		else
		{
			count = m_Splitter->ConsumeFromCache(data, numSamples,
				sampleRate, numChannels);
		}

		m_NumSamplesConsumed += count;

		return count;
	}

private:
	SplitterImpl *m_Splitter;
	count_t m_NumSamplesConsumed = 0;
};

std::shared_ptr<Channel> SplitterImpl::GetChannel()
{
	auto channel = CreateChannel();
	auto stream = std::make_shared<SplitterStreamImpl>(this);
	channel->SetStream(stream);

	m_Channels.push_back(m_Channel);
}

std::shared_ptr<Splitter> CreateMixer(std::shared_ptr<Channel> channel)
{
	return std::make_shared<SplitterImpl>(channel);
}

};
