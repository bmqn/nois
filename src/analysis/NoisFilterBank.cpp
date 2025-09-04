#include "nois/analysis/NoisFilterBank.hpp"

#include "nois/effect/NoisFilter.hpp"

namespace nois {

class FilterBankImpl : public FilterBank
{
public:
	FilterBankImpl(
		Ref_t<Stream> stream,
		count_t numBands,
		f32_t minCutoffRatio,
		f32_t maxCutoffRatio)
		: m_Stream(stream)
		, m_StreamBufferStream(nullptr)
	{
		minCutoffRatio = std::max(minCutoffRatio, 0.001f);
		maxCutoffRatio = std::min(maxCutoffRatio, 0.999f);

		m_StreamBufferStream = MakeRef<FloatBufferStream>(&m_SteamBuffer);

		for (count_t i = 0; i < numBands; i++)
		{
			f32_t cutoffRatio =
				minCutoffRatio *
				std::pow(maxCutoffRatio / minCutoffRatio, f32_t(i) / f32_t(numBands - 1));

			auto filter = CreateBandpassFilter(m_StreamBufferStream, BandpassFilter::k_Biquad);
			filter->SetCutoffRatio(MakeRef<FloatConstantParameter>(cutoffRatio));
			filter->SetQ(MakeRef<FloatConstantParameter>(2.0f));
			m_Filters.emplace_back(filter);

			auto bandRms = CreateBlockParameter(0.0f);
			m_BandRmses.emplace_back(bandRms);

			m_BandCutoffRatios.emplace_back(cutoffRatio);
		}
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		Stream::Result result = Stream::Success;

		// Consume the stream into the stream buffer
		// The filters consume from the stream buffer
		if (Stream::Result streamResult =
			m_Stream->Consume(
				m_SteamBuffer,
				sampleRate);
			streamResult != Stream::Success)
		{
			result = streamResult;
		}

		// Zero the result buffer
		m_ResultBuffer.Zero();

		for (count_t i = 0; i < m_Filters.size(); ++i)
		{
			// Consume filter into the scratch buffer
			m_Filters[i]->Consume(m_ScratchBuffer, sampleRate);

			// Apply band gains, if one is specified for this band
			if (i < m_BandGains.size())
			{
				m_ScratchBuffer.Multiply(m_BandGains[i]->Get());
			}

			count_t numFrames = m_ScratchBuffer.GetNumFrames();
			count_t numChannels = m_ScratchBuffer.GetNumChannels();

			// Determine energy of filter
			f32_t energy = 0.0f;
			for (count_t f = 0; f < numFrames; ++f)
			{
				for (count_t c = 0; c < numChannels; ++c)
				{
					energy += m_ScratchBuffer(f, c) * m_ScratchBuffer(f, c);
				}
			}

			// Update the rms for this band
			m_BandRmses[i]->Set(std::sqrt(energy / f32_t(numFrames)));

			// Accumulate filter into result buffer
			m_ResultBuffer.Add(m_ScratchBuffer);
		}

		// Copy the result buffer to the output buffer
		buffer.Copy(m_ResultBuffer);

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

		m_SteamBuffer.Resize(
			numFrames,
			numChannels);

		for (auto &filter : m_Filters)
		{
			filter->PrepareToConsume(
				numFrames,
				numChannels,
				sampleRate);
		}

		for (auto &bandRms : m_BandRmses)
		{
			bandRms->Prepare(
				numFrames,
				sampleRate);
		}

		m_ResultBuffer.Resize(
			numFrames,
			numChannels);

		m_ScratchBuffer.Resize(
			numFrames,
			numChannels);
	}

	virtual count_t GetNumBands() const override
	{
		return m_BandCutoffRatios.size();
	}

	virtual f32_t GetBandFrequency(
		count_t bandIndex,
		f32_t sampleRate) const override
	{
		if (bandIndex >= m_BandCutoffRatios.size())
		{
			return 0.0f;
		}

		return m_BandCutoffRatios[bandIndex] * (sampleRate * 0.5f);
	}

	virtual void SetBandGains(FloatBlockParameterList bandGains)
	{
		m_BandGains = bandGains;
	}

	virtual FloatBlockParameterList GetBandRmses() const override
	{
		FloatBlockParameterList bandRmses;
		bandRmses.reserve(m_BandRmses.size());
		for (auto &bandRms : m_BandRmses)
		{
			bandRmses.emplace_back(bandRms);
		}
		return bandRmses;
	}

private:
	Ref_t<Stream> m_Stream;
	Ref_t<FloatBufferStream> m_StreamBufferStream;
	FloatBuffer m_SteamBuffer;
	std::vector<Ref_t<BandpassFilter>> m_Filters;
	FloatBlockParameterList m_BandGains;
	FloatConstantBlockParameterList m_BandRmses;
	std::vector<f32_t> m_BandCutoffRatios;
	FloatBuffer m_ResultBuffer;
	FloatBuffer m_ScratchBuffer;
};

Ref_t<FilterBank> CreateFilterBank(
	Ref_t<Stream> stream,
	count_t numBands,
	f32_t minCutoffRatio,
	f32_t maxCutoffRatio)
{
	return MakeRef<FilterBankImpl>(
		stream,
		numBands,
		minCutoffRatio,
		maxCutoffRatio);
}

}

