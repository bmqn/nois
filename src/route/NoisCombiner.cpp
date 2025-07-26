#include "nois/route/NoisCombiner.hpp"

namespace nois {

class CombinerStreamImpl;

class CombinerImpl : public Combiner, public RefFromThis_t<CombinerImpl>
{
	friend class CombinerStreamImpl;
	friend Ref_t<Combiner> CreateCombiner();

public:
	CombinerImpl()
		: m_CombinerStream(nullptr)
		, m_BlockConsumed(false)
		, m_BlockPrepared(false)
	{
	}

	virtual Stream::Result ConsumeIntoCache(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override
	{
		Stream::Result result = Stream::Success;

		if (!m_BlockConsumed && !m_Streams.empty())
		{
			m_BlockConsumed = true;

			m_Cache.Zero();

			for (const auto &stream : m_Streams)
			{
				if (Stream::Result streamResult =
					stream->Consume(
						m_Scratch,
						sampleRate);
					streamResult != Stream::Success)
				{
					result = streamResult;
				}

				m_Cache.Add(m_Scratch);
			}

			m_BlockPrepared = false;
		}

		return result;
	}

	void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		m_Cache.Resize(numFrames, numChannels);
		m_Scratch.Resize(numFrames, numChannels);

		if (!m_BlockPrepared)
		{
			m_BlockPrepared = true;

			for (const auto &stream : m_Streams)
			{
				stream->PrepareToConsume(
					numFrames,
					numChannels,
					sampleRate);
			}

			m_BlockConsumed = false;
		}
	}

	virtual void AddStream(
		Ref_t<Stream> stream) override
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

	virtual void RemoveStream(
		Ref_t<Stream> stream) override
	{
		std::erase_if(m_Streams,
			[stream](const auto &candidate) {
				return candidate == stream;
			});
	}

	virtual Ref_t<Stream> GetStream() override;

private:
	Stream::Result ConsumeFromCache(
		FloatBuffer &buffer,
		f32_t sampleRate)
	{
		buffer.Copy(m_Cache);

		return Stream::Success;
	}

private:
	Ref_t<CombinerStreamImpl> m_CombinerStream;

	std::vector<Ref_t<Stream>> m_Streams;

	bool m_BlockConsumed;
	bool m_BlockPrepared;

	FloatBuffer m_Cache;
	FloatBuffer m_Scratch;
};

class CombinerStreamImpl : public Stream
{
public:
	CombinerStreamImpl(Ref_t<CombinerImpl> combiner)
		: m_Combiner(combiner)
	{
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		return m_Combiner->ConsumeFromCache(
			buffer,
			sampleRate);
	}

	virtual void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override
	{
		m_Combiner->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
	}

private:
	Ref_t<CombinerImpl> m_Combiner;
};

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