#include "nois/effect/NoisDistorter.hpp"

namespace nois {

class FolderDistorterImpl : public FolderDistorter
{
public:
	FolderDistorterImpl(Ref_t<Stream> stream, FolderFunc folderFunc)
		: m_Stream(stream)
		, m_FolderFunc(folderFunc)
	{
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		Stream::Result result = Stream::Success;

		if (Stream::Result streamResult =
			m_Stream->Consume(
				buffer,
				sampleRate);
			streamResult != Stream::Success)
		{
			result = streamResult;
		}

		for (count_t i = 0; i < buffer.GetNumFrames(); ++i)
		{
			for (count_t j = 0; j < buffer.GetNumChannels(); ++j)
			{
				buffer(i, j) = ConsumeChannel(j, buffer(i, j));
			}
		}

		return result;
	}

	virtual void PrepareToConsume(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override
	{
		m_Stream->PrepareToConsume(
			numFrames,
			numChannels,
			sampleRate);
	}

	virtual f32_t GetPreGainDb() override
	{
		return ToDb(m_PreGain);
	}

	virtual void SetPreGainDb(f32_t preGainDb) override
	{
		m_PreGain = FromDb(preGainDb);
	}

	virtual f32_t GetThresholdGainDb() override
	{
		return ToDb(m_ThresholdGain);
	}

	virtual void SetThresholdGainDb(f32_t thresholdGainDb) override
	{
		m_ThresholdGain = FromDb(thresholdGainDb);
	}

	virtual count_t GetNumFolds() override
	{
		return m_NumFolds;
	}

	virtual void SetNumFolds(count_t numFolds) override
	{
		m_NumFolds = numFolds;
	}

	virtual FolderFunc GetFolderFunc() override
	{
		return m_FolderFunc;
	}

	virtual void SetFolderFunc(FolderFunc folderFunc) override
	{
		m_FolderFunc = folderFunc;
	}

private:
	f32_t ConsumeChannel(count_t frameIndex, f32_t input)
	{
		f32_t output = input;

		if (m_FolderFunc == k_FolderFuncRizzler)
		{
			f32_t a = std::asin(input);
			f32_t b = std::sin((1.3f / m_ThresholdGain) * input);
			f32_t c = std::cos((1.3f / m_ThresholdGain) * input);
			output = std::atan(m_PreGain * a * b * b * c);
		}
		else
		{
			output *= m_PreGain;

			count_t i = 0;
			while (i < m_NumFolds && std::abs(output) > m_ThresholdGain)
			{
				output += 2.0f * ((output >= 0.0f ? m_ThresholdGain : -m_ThresholdGain) - output);
				++i;
			}
		}

		return output;
	}

private:
	Ref_t<Stream> m_Stream;

	f32_t m_PreGain = 1.0f;
	f32_t m_ThresholdGain = 1.0f;
	count_t m_NumFolds = 1;
	FolderFunc m_FolderFunc = k_FolderFuncBasic;
};

template<>
FolderDistorter &Distorter<FolderDistorter>::GetDistorter()
{
	return *reinterpret_cast<FolderDistorter*>(this);
}

Ref_t<Distorter<FolderDistorter>> CreateFolderDistorter(
	Ref_t<Stream> stream,
	FolderDistorter::FolderFunc folderFunc)
{
	return MakeRef<FolderDistorterImpl>(stream, folderFunc);
}

}

