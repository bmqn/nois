#include "nois/analysis/NoisFilterBank.hpp"

#include "nois/effect/NoisFilter.hpp"

#include <chrono>
#include <future>

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

		m_StreamBufferStream = MakeRef<BufferStream>(&m_SteamBuffer);

		for (count_t i = 0; i < numBands; i++)
		{
			f32_t cutoffRatio =
				minCutoffRatio *
				std::pow(maxCutoffRatio / minCutoffRatio, f32_t(i) / f32_t(numBands - 1));
			m_BandCutoffRatios.emplace_back(cutoffRatio);

			f32_t q = 0.5f;
			m_BandQs.emplace_back(q);

			f32_t rms = 0.0f;
			m_BandRmses.emplace_back(rms);

			auto filter = CreateBandpassFilter(m_StreamBufferStream, BandpassFilter::k_Biquad);
			filter->SetCutoffRatio(MakeRef<FloatConstantParameter>(cutoffRatio));
			filter->SetQ(MakeRef<FloatConstantParameter>(q));
			m_Filters.emplace_back(filter);
			m_FilterJobs.emplace_back();

			m_ScratchBuffers.emplace_back();
		}
	}

	virtual Stream::Result Consume(
		FloatBuffer &buffer,
		f32_t sampleRate) override
	{
		NOIS_PROFILE_SCOPE_NAMED("FilterBank - Consume");

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

		// Run filters in parallel
		for (count_t i = 0; i < m_Filters.size(); ++i)
		{
			// Get band gains, if one is specified for this band
			float bandGain = 1.0f;
			if (i < m_BandGains.size())
			{
				bandGain = m_BandGains[i]->Get();
			}

			m_FilterJobs[i] = std::async(std::launch::async,
			[
				&filter = m_Filters[i],
				bandGain,
				&bandRms = m_BandRmses[i],
				&scratchBuffer = m_ScratchBuffers[i],
				sampleRate
			]()
			{
				// Consume filter into the scratch buffer
				filter->Consume(scratchBuffer, sampleRate);

				// Apply band gain
				scratchBuffer.Multiply(bandGain);

				count_t numFrames = scratchBuffer.GetNumFrames();
				count_t numChannels = scratchBuffer.GetNumChannels();
				count_t numSamples = numFrames * numChannels;

				// Determine energy of filter
				f32_t energy = 0.0f;
				for (count_t f = 0; f < numFrames; ++f)
				{
					for (count_t c = 0; c < numChannels; ++c)
					{
						f32_t s = scratchBuffer(f, c);
						energy += s * s;
					}
				}

				// Update the rms for this band
				bandRms = std::sqrt(energy / f32_t(numSamples));
			});
		}

		// Wait for jobs to complete
		for (count_t i = 0; i < m_Filters.size(); ++i)
		{
			m_FilterJobs[i].wait();
			m_ResultBuffer.Add(m_ScratchBuffers[i]);
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

		for (count_t i = 0; i < m_Filters.size(); ++i)
		{
			m_Filters[i]->PrepareToConsume(
				numFrames,
				numChannels,
				sampleRate);

			m_ScratchBuffers[i].Resize(
				numFrames,
				numChannels);
		}

		m_ResultBuffer.Resize(
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

	virtual f32_t GetBandRms(
		count_t bandIndex) const override
	{
		if (bandIndex >= m_BandCutoffRatios.size())
		{
			return 0.0f;
		}

		return m_BandRmses[bandIndex];
	}

	virtual void SetBandGains(FloatBlockParameterList bandGains) override
	{
		m_BandGains = bandGains;
	}

private:
	Ref_t<Stream> m_Stream;
	Ref_t<BufferStream> m_StreamBufferStream;
	FloatBuffer m_SteamBuffer;

	std::vector<Ref_t<BandpassFilter>> m_Filters;
	std::vector<std::future<void>> m_FilterJobs;

	FloatBlockParameterList m_BandGains;
	std::vector<f32_t> m_BandCutoffRatios;
	std::vector<f32_t> m_BandQs;
	std::vector<f32_t> m_BandRmses;

	FloatBuffer m_ResultBuffer;
	std::vector<FloatBuffer> m_ScratchBuffers;
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

