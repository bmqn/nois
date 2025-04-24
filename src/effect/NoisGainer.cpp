#include "nois/effect/NoisGainer.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class GainerImpl : public Gainer
{
public:
	
	GainerImpl(Stream *stream)
		: m_Stream(stream)
		, m_Gain(MakeOwn<FloatConstantParameter>(0.0f))
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
				data[i] *= m_Gain->Get(i);
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

	virtual const FloatParameter &GetGain() const
	{
		return *m_Gain;
	}

	virtual void SetGain(Ref_t<FloatParameter> gain) override
	{
		m_Gain = gain;
	}

private:
	Stream *m_Stream;
	Ref_t<FloatParameter> m_Gain;
};

Ref_t<Gainer> CreateGainer(Stream *stream)
{
	return MakeRef<GainerImpl>(stream);
}

};

