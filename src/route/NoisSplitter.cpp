#include "nois/route/NoisSplitter.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class SplitterImpl : public Splitter
{
	friend class SplitterStreamImpl;

public:
	SplitterImpl(Stream *stream = nullptr)
		: m_Stream(stream)
	{
	}

	virtual void PrepareToConsume(count_t numSamples,
	                              int32_t sampleRate,
	                              int32_t numChannels)
	{
		if (m_NumSamples != numSamples)
		{
			m_Samples.resize(numSamples);

			m_NumSamples = numSamples;
		}

		// Very important ! This prevents infinite recursion
		m_NumSamplesConsumed += numSamples;

		if (m_Stream)
		{
			m_Stream->PrepareToConsume(numSamples, sampleRate, numChannels);
			m_Stream->Consume(m_Samples.data(), numSamples, sampleRate, numChannels);
		}
	}

	virtual void SetStream(Stream *stream) override
	{
		m_Stream = stream;
	}

	virtual Stream *GetStream() override;

	virtual void Refresh() override
	{
		std::fill(m_Samples.begin(), m_Samples.end(), 0.0f);
		m_NumSamplesConsumed = 0;
	}

private:
	count_t ConsumeFromCache(data_t *data,
	                         count_t numSamples,
	                         int32_t sampleRate,
	                         int32_t numChannels)
	{
		count_t count = 0;

		if (m_Stream)
		{
			std::copy_n(m_Samples.data(), numSamples, data);

			count = numSamples;
		}

		return count;
	}

private:
	Stream *m_Stream;
	std::vector<Own_t<Stream>> m_SplitterStreams;

	count_t m_NumSamples = 0;

	count_t m_NumSamplesConsumed = 0;
	std::vector<data_t> m_Samples;
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
		return m_Splitter->ConsumeFromCache(data, numSamples, sampleRate, numChannels);
	}

	virtual void PrepareToConsume(count_t numSamples,
	                              int32_t sampleRate,
	                              int32_t numChannels) override
	{
		if (m_Splitter->m_NumSamplesConsumed <= m_NumSamplesConsumed)
		{
			m_Splitter->PrepareToConsume(numSamples, sampleRate, numChannels);
		}

		m_NumSamplesConsumed += numSamples;
	}

private:
	SplitterImpl *m_Splitter;

	count_t m_NumSamplesConsumed = 0;
};

Stream *SplitterImpl::GetStream()
{
	auto splitterStream = MakeOwn<SplitterStreamImpl>(this);
	m_SplitterStreams.push_back(std::move(splitterStream));
	return m_SplitterStreams.back().get();
}

Ref_t<Splitter> CreateSplitter()
{
	return MakeRef<SplitterImpl>();
}

Ref_t<Splitter> CreateSplitter(Stream *stream)
{
	return MakeRef<SplitterImpl>(stream);
}

};
