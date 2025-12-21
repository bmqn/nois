#include "nois/route/NoisSplitter.hpp"

namespace nois {

#if 0

class SplitterImpl : public Splitter, public RefFromThis_t<SplitterImpl>
{
	friend class SplitterStreamImpl;

public:
	SplitterImpl(Ref_t<Stream> stream = nullptr)
		: m_Stream(stream)
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

		if (!m_BlockConsumed && m_Stream)
		{
			m_BlockConsumed = true;

			m_Cache.Zero();

			if (Stream::Result streamResult =
				m_Stream->Consume(
					m_Cache,
					sampleRate);
				streamResult != Stream::Success)
			{
				result = streamResult;
			}

			m_BlockPrepared = false;
		}

		return result;
	}

	virtual const FloatBuffer &GetCacheBuffer() const override
	{
		return m_Cache;
	}

	void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		m_Cache.Resize(numFrames, numChannels);

		if (!m_BlockPrepared && m_Stream)
		{
			m_BlockPrepared = true;

			m_Stream->PrepareToConsume(
				numFrames,
				numChannels,
				sampleRate);

			m_BlockConsumed = false;
		}
	}

	virtual void SetStream(
		Ref_t<Stream> stream) override
	{
		m_Stream = stream;
	}

	Ref_t<Stream> GetStream() override;

private:
	Stream::Result ConsumeFromCache(
		FloatBuffer &buffer,
		f32_t sampleRate)
	{
		buffer.Copy(m_Cache);

		return Stream::Success;
	}

private:
	Ref_t<Stream> m_Stream;

	bool m_BlockConsumed;
	bool m_BlockPrepared;

	FloatBuffer m_Cache;
};

class SplitterStreamImpl : public Stream
{
public:
	SplitterStreamImpl(
		Ref_t<SplitterImpl> splitter)
		: m_Splitter(splitter)
	{
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		return m_Splitter->ConsumeFromCache(
			buffer,
			sampleRate);
	}

	virtual void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override
	{
		m_Splitter->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
	}

private:
	Ref_t<SplitterImpl> m_Splitter;
};

Ref_t<Stream> SplitterImpl::GetStream()
{
	return MakeRef<SplitterStreamImpl>(shared_from_this());
}

Ref_t<Splitter> CreateSplitter()
{
	return MakeRef<SplitterImpl>();
}

Ref_t<Splitter> CreateSplitter(Ref_t<Stream> stream)
{
	return MakeRef<SplitterImpl>(stream);
}

#endif

};
