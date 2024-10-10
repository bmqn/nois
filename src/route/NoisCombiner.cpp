#include "nois/route/NoisCombiner.hpp"

#include <algorithm>
#include <vector>

namespace nois {

class CombinerStreamImpl;

class CombinerImpl : public Combiner
{
	friend class CombinerStreamImpl;

public:
	CombinerImpl();
	CombinerImpl(std::initializer_list<Stream*> streams);

	virtual void AddStream(Stream *stream) override
	{
		auto it = std::find(m_Streams.begin(), m_Streams.end(), stream);
		if (it == m_Streams.end())
		{
			m_Streams.push_back(stream);
		}
	}

	virtual void RemoveStream(Stream *stream) override
	{
		std::erase_if(m_Streams, [stream](const auto &candidate) {
			return candidate == stream;
		});
	}

	virtual Stream *GetStream() override;

private:
	std::vector<Stream*> m_Streams;
	std::unique_ptr<CombinerStreamImpl> m_CombinerStream;
};

class CombinerStreamImpl : public Stream
{
public:
	CombinerStreamImpl(CombinerImpl *combiner)
		: m_Combiner(combiner)
	{
	}

	virtual count_t Consume(data_t *data,
	                        count_t numSamples,
	                        int32_t sampleRate,
	                        int32_t numChannels) override
	{
		count_t count = numSamples;

		std::fill(data, data + numSamples, data_t{ 0 });

		if (!m_Combiner->m_Streams.empty())
		{
			for (const auto &stream : m_Combiner->m_Streams)
			{
				if (stream)
				{
					std::fill(m_Samples.begin(), m_Samples.end(), data_t{ 0 });

					count_t thisCount = stream->Consume(
						m_Samples.data(), numSamples,
						sampleRate, numChannels);

					std::transform(data, data + thisCount,
						m_Samples.begin(), data,
						std::plus<data_t>());
				}
			}
		}

		return count;
	}

	virtual void PrepareToConsume(count_t numSamples,
	                              int32_t sampleRate,
	                              int32_t numChannels) override
	{
		if (m_NumSamples != numSamples)
		{
			m_Samples.resize(numSamples);

			m_NumSamples = numSamples;
		}

		for (const auto &stream : m_Combiner->m_Streams)
		{
			if (stream)
			{
				stream->PrepareToConsume(numSamples, sampleRate, numChannels);
			}
		}
	}

private:
	CombinerImpl *m_Combiner;

	int32_t m_NumSamples = 0;

	std::vector<data_t> m_Samples;
};

CombinerImpl::CombinerImpl()
{
	m_CombinerStream = std::make_unique<CombinerStreamImpl>(this);
}

CombinerImpl::CombinerImpl(std::initializer_list<Stream*> streams)
{
	m_CombinerStream = std::make_unique<CombinerStreamImpl>(this);

	for (const auto &stream : streams)
	{
		m_Streams.push_back(stream);
	}
}

Stream *CombinerImpl::GetStream()
{
	return m_CombinerStream.get();
}

std::shared_ptr<Combiner> CreateCombiner()
{
	return std::make_shared<CombinerImpl>();
}

std::shared_ptr<Combiner> CreateCombiner(std::initializer_list<Stream*> streams)
{
	return std::make_shared<CombinerImpl>(streams);
}

}