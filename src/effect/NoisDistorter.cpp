#include "nois/effect/NoisDistorter.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class FolderDistorterImpl : public FolderDistorter
{
public:
	
	FolderDistorterImpl(Stream *stream, FolderFunc folderFunc)
		: m_Stream(stream)
		, m_FolderFunc(folderFunc)
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
			
			for (int32_t i = 0; i < count; i += numChannels)
			{
				for (int32_t j = 0; j < numChannels; ++j)
				{
					data[i + j] = ConsumeChannel(j, data[i + j]);
				}
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

	virtual data_t GetPreGainDb() override
	{
		return ToDb(m_PreGain);
	}

	virtual void SetPreGainDb(data_t preGainDb) override
	{
		m_PreGain = FromDb(preGainDb);
	}

	virtual data_t GetThresholdGainDb() override
	{
		return ToDb(m_ThresholdGain);
	}

	virtual void SetThresholdGainDb(data_t thresholdGainDb) override
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
	data_t ConsumeChannel(count_t index, data_t input)
	{
		data_t output = input;

		if (m_FolderFunc == k_FolderFuncRizzler)
		{
			data_t A = std::asin(input);
			data_t B = std::sin((1.3f / m_ThresholdGain) * input);
			data_t C = std::cos((1.3f / m_ThresholdGain) * input);
			output = std::atan(m_PreGain * A * B * B * C);
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
	Stream *m_Stream;

	data_t m_PreGain = 1.0f;
	data_t m_ThresholdGain = 1.0f;
	count_t m_NumFolds = 1;
	FolderFunc m_FolderFunc = k_FolderFuncBasic;
};

template<>
FolderDistorter &Distorter<FolderDistorter>::GetDistorter()
{
	return *reinterpret_cast<FolderDistorter*>(this);
}

Ref_t<Distorter<FolderDistorter>> CreateFolderDistorter(Stream *stream,
	FolderDistorter::FolderFunc folderFunc)
{
	return MakeRef<FolderDistorterImpl>(stream, folderFunc);
}

};

