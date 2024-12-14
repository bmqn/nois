#include "nois/effect/NoisGainer.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class GainerImpl : public Gainer
{
public:
	
	GainerImpl(Stream *stream)
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

			for (count_t i = 0; i < count; ++i)
			{
				data[i] *= m_Gain;
			}

			return count;
		}

		return 0;
	}

	virtual void PrepareToConsume(count_t numSamples,
	                              int32_t sampleRate,
	                              int32_t numChannels) override
	{
		if (m_Stream)
		{
			m_Stream->PrepareToConsume(
				numSamples,
				sampleRate,
				numChannels);
		}
	}

	virtual data_t GetGain() const override { return m_Gain; }
	virtual data_t GetGainDb() const override { return ToDb(m_Gain); }

	virtual void SetGain(data_t gain) override { m_Gain = gain; };
	virtual void SetGainDb(data_t gainDb) override { m_Gain = FromDb(gainDb); };

private:
	Stream *m_Stream;
	data_t m_Gain = data_t{ 1.0f };

};

Ref_t<Gainer> CreateGainer(Stream *stream)
{
	return MakeRef<GainerImpl>(stream);
}

};

