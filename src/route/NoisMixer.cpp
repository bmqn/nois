#include "nois/route/NoisMixer.hpp"

#include "nois/NoisUtil.hpp"

#include <unordered_map>

namespace nois {

class MixerImpl : public Mixer
{
public:
	
	MixerImpl(std::initializer_list<std::shared_ptr<Channel>> channels)
		: m_Channels(channels.size())
	{
		for (std::shared_ptr<Channel> channel : channels)
		{
			m_Channels[channel] = data_t{ 1.0f };
		}
	}

	virtual count_t Consume(data_t *data,
							count_t numSamples,
							int32_t sampleRate,
							int32_t numChannels) override
	{
		std::vector<data_t> buffer(numSamples);
		count_t count = numSamples;

		for (count_t i = 0; i < count; ++i)
		{
			data[i] = nois::data_t{ 0.0f };
		}

		for (auto &[channel, gain] : m_Channels)
		{
			std::shared_ptr<Stream> stream = channel->GetStream();

			if (stream)
			{
				count_t thisCount = stream->Consume(buffer.data(), count, sampleRate, numChannels);

				for (count_t i = 0; i < thisCount; ++i)
				{
					data[i] += buffer[i] * gain;
				}
			}
		}

		return count;
	}

	virtual void AddChannel(std::shared_ptr<Channel> channel) override
	{
		auto it = m_Channels.find(channel);
		if (it == m_Channels.end())
		{
			m_Channels[channel] = data_t{ 1.0f };
		}
	}

	virtual void RemoveChannel(std::shared_ptr<Channel> channel) override
	{
		auto it = m_Channels.find(channel);
		if (it != m_Channels.end())
		{
			m_Channels.erase(it);
		}
	}

	virtual data_t GetGain(std::shared_ptr<Channel> channel) override
	{
		auto it = m_Channels.find(channel);
		if (it != m_Channels.end())
		{
			return m_Channels[channel];
		}

		return 1.0f;
	}

	virtual void SetGain(std::shared_ptr<Channel> channel, data_t gain) override
	{
		auto it = m_Channels.find(channel);
		if (it != m_Channels.end())
		{
			m_Channels[channel] = gain;
		}
	}

	virtual data_t GetGainDb(std::shared_ptr<Channel> channel) override
	{
		auto it = m_Channels.find(channel);
		if (it != m_Channels.end())
		{
			return ToDb(m_Channels[channel]);
		}

		return 0.0f;
	}

	virtual void SetGainDb(std::shared_ptr<Channel> channel, data_t gainDb) override
	{
		auto it = m_Channels.find(channel);
		if (it != m_Channels.end())
		{
			m_Channels[channel] = FromDb(gainDb);
		}
	}

private:
	std::unordered_map<std::shared_ptr<Channel>, data_t> m_Channels;

};

std::shared_ptr<Mixer> CreateMixer(std::initializer_list<std::shared_ptr<Channel>> channels)
{
	return std::make_shared<MixerImpl>(channels);
}

};
