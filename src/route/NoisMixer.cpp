#include "nois/route/NoisMixer.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class MixerImpl : public Mixer
{
public:
	
	MixerImpl(std::initializer_list<Ref_t<Channel>> channels)
		: m_Channels(channels.size())
	{
		for (Ref_t<Channel> channel : channels)
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
			Ref_t<Stream> stream = channel->GetStream();

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

	virtual void AddChannel(Ref_t<Channel> channel) override
	{
		if (auto it = m_Channels.find(channel);
			it == m_Channels.end())
		{
			m_Channels[channel] = data_t{ 1.0f };
		}
	}

	virtual void RemoveChannel(Ref_t<Channel> channel) override
	{
		if (auto it = m_Channels.find(channel);
			it != m_Channels.end())
		{
			m_Channels.erase(it);
		}
	}

	virtual data_t GetGain(Ref_t<Channel> channel) override
	{
		if (auto it = m_Channels.find(channel);
			it != m_Channels.end())
		{
			return m_Channels[channel];
		}

		return 1.0f;
	}

	virtual void SetGain(Ref_t<Channel> channel, data_t gain) override
	{
		if (auto it = m_Channels.find(channel);
			it != m_Channels.end())
		{
			m_Channels[channel] = gain;
		}
	}

	virtual data_t GetGainDb(Ref_t<Channel> channel) override
	{
		if (auto it = m_Channels.find(channel);
			it != m_Channels.end())
		{
			return ToDb(m_Channels[channel]);
		}

		return 0.0f;
	}

	virtual void SetGainDb(Ref_t<Channel> channel, data_t gainDb) override
	{
		if (auto it = m_Channels.find(channel);
			it != m_Channels.end())
		{
			m_Channels[channel] = FromDb(gainDb);
		}
	}

private:
	std::unordered_map<Ref_t<Channel>, data_t> m_Channels;
};

Ref_t<Mixer> CreateMixer(std::initializer_list<Ref_t<Channel>> channels)
{
	return MakeRef<MixerImpl>(channels);
}

};
