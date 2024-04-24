#include "nois/effect/NoisGainer.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class GainerImpl : public Gainer
{
public:
	
	GainerImpl(std::shared_ptr<Stream> stream)
		: m_Stream(stream)
	{
	}

	virtual count_t Consume(data_t *data, count_t len) override
	{
		if (m_Stream)
		{
			count_t count = m_Stream->Consume(data, len);

			for (count_t i = 0; i < count; ++i)
			{
				data[i] *= m_Gain;
			}

			return count;
		}

		return 0;
	}

	virtual void SetGain(float gain) override { m_Gain = gain; };
	virtual void SetGainDb(float gainDb) override { m_Gain = FromDb(gainDb); };

private:
	std::shared_ptr<Stream> m_Stream;
	data_t m_Gain = data_t{ 1.0f };

};

std::shared_ptr<Gainer> CreateGainer(std::shared_ptr<Stream> stream)
{
	return std::make_shared<GainerImpl>(stream);
}

};

