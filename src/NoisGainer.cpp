#include "nois/NoisGainer.hpp"

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
		if (!m_Stream)
		{
			return 0;
		}

		auto count = m_Stream->Consume(data, len);

		for (count_t i = 0; i < len; i += 2)
		{
			data[i + 0] *= m_Gain;
			data[i + 1] *= m_Gain;
		}

		return count;
	}

private:
	std::shared_ptr<Stream> m_Stream;

};

std::shared_ptr<Gainer> CreateGainer(std::shared_ptr<Stream> stream)
{
	return std::make_shared<GainerImpl>(stream);
}

};

