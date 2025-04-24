#include "nois/route/NoisCombiner.hpp"

namespace nois {

class CombinerStreamImpl;

class CombinerImpl : public Combiner, public RefFromThis_t<CombinerImpl>
{
	friend class CombinerStreamImpl;
	friend Ref_t<Combiner> CreateCombiner();

public:
	CombinerImpl();

	virtual void AddStream(Ref_t<Stream> stream) override
	{
		if (auto it = std::find(
			m_Streams.begin(),
			m_Streams.end(),
			stream);
			it == m_Streams.end())
		{
			m_Streams.push_back(stream);
		}
	}

	virtual void RemoveStream(Ref_t<Stream> stream) override
	{
		std::erase_if(m_Streams,
			[stream](const auto &candidate) {
				return candidate == stream;
			});
	}

	virtual Ref_t<Stream> GetStream() override;

private:
	Ref_t<CombinerStreamImpl> m_CombinerStream;

	std::vector<Ref_t<Stream>> m_Streams;
};

class CombinerStreamImpl : public Stream
{
public:
	CombinerStreamImpl(Ref_t<CombinerImpl> combiner)
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
				std::fill(m_Samples.begin(), m_Samples.end(), data_t{ 0 });

				count_t thisCount = stream->Consume(
					m_Samples.data(), numSamples,
					sampleRate, numChannels);

				std::transform(data, data + thisCount,
					m_Samples.begin(), data,
					std::plus<data_t>());
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
			stream->PrepareToConsume(numSamples, sampleRate, numChannels);
		}
	}

private:
	Ref_t<CombinerImpl> m_Combiner;

	int32_t m_NumSamples = 0;
	std::vector<data_t> m_Samples;
};

CombinerImpl::CombinerImpl()
{
}

Ref_t<Stream> CombinerImpl::GetStream()
{
	return m_CombinerStream;
}

Ref_t<Combiner> CreateCombiner()
{
	auto combiner = MakeRef<CombinerImpl>();
	combiner->m_CombinerStream = MakeOwn<CombinerStreamImpl>(combiner);

	return combiner;
}

}