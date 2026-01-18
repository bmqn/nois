#include "nois/analysis/NoisFilterBank.hpp"

#include "nois/effect/NoisFilter.hpp"

#include <chrono>
#include <future>

namespace nois {

class FilterBank::Impl
{
public:
	Impl(
		count_t numBands,
		f32_t minCutoffRatio,
		f32_t maxCutoffRatio)
		: m_NumBands(numBands)
	{
		minCutoffRatio =
			std::clamp(minCutoffRatio, 0.001f, 0.999f);
		maxCutoffRatio =
			std::clamp(maxCutoffRatio, 0.001f, 0.999f);

		f32_t totalOctaves =
			std::log2(maxCutoffRatio / minCutoffRatio);
		f32_t bandsPerOctave =
			f32_t(numBands - 1) / totalOctaves;

		for (count_t i = 0; i < numBands; i++)
		{
			f32_t t = f32_t(i) / f32_t(numBands - 1);

			Biquad<f32_t> filter;
			f32_t cutoffRatio =
				minCutoffRatio *
				std::pow(maxCutoffRatio / minCutoffRatio, t);
			f32_t q =
				std::numbers::sqrt2 *
				bandsPerOctave;
			filter.MakeBandpass(cutoffRatio, q);

			m_Filters.emplace_back(filter);
			m_FilterBuffers.emplace_back();
			m_FilterJobs.emplace_back();

			m_BandRmses.emplace_back();
		}
	}

	Stream::Result Process(
		ConstFloatBufferView inBuffer,
		FloatBufferView outBuffer)
	{
		NOIS_PROFILE_SCOPE();

		for (count_t i = 0; i < m_NumBands; ++i)
		{
			// Run filters in parallel
			m_FilterJobs[i] = std::async(std::launch::async,
			[
				&inBuffer,
				&filter = m_Filters[i],
				&filterBuffer = m_FilterBuffers[i],
				&bandRms = m_BandRmses[i],
				this
			]()
			{
				// Process filter into the scratch buffer
				for (count_t c = 0; c < m_NumChannels; ++c)
				{
					filter.Process(inBuffer.View(c), filterBuffer.View(c), m_NumFrames, c);
				}

				// Determine energy of filter
				f32_t energy = 0.0f;
				for (count_t c = 0; c < m_NumChannels; ++c)
				{
					for (count_t f = 0; f < m_NumFrames; ++f)
					{
						f32_t s = filterBuffer(f, c);
						energy += s * s;
					}
				}

				// Update the rms for this band
				bandRms = std::sqrt(energy / f32_t(m_NumFrames * m_NumChannels));
			});
		}

		// Wait for jobs to complete
		for (count_t i = 0; i < m_NumBands; ++i)
		{
			m_FilterJobs[i].wait();
			m_ResultBuffer.Add(m_FilterBuffers[i]);
		}

		// Copy the result buffer to the output buffer
		outBuffer.Copy(m_ResultBuffer);

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate)
	{
		NOIS_PROFILE_SCOPE();

		for (count_t i = 0; i < m_NumBands; ++i)
		{
			m_FilterBuffers[i].Resize(numFrames, numChannels);
		}

		m_ResultBuffer.Resize(numFrames, numChannels);
		m_ResultBuffer.Zero();

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_SampleRate = sampleRate;
	}

	f32_t GetRms(count_t b) const
	{
		return m_BandRmses[b];
	}

private:
	count_t m_NumBands;

	std::vector<Biquad<f32_t>> m_Filters;
	std::vector<FloatBuffer> m_FilterBuffers;
	std::vector<std::future<void>> m_FilterJobs;

	std::vector<f32_t> m_BandRmses;

	FloatBuffer m_ResultBuffer;

	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	f32_t m_SampleRate = 0.0f;
};

NOIS_INTERFACE_IMPL(FilterBank)

f32_t FilterBank::GetRms(count_t b) const
{
	return m_Impl->GetRms(b);
}

Ref_t<FilterBank> FilterBank::Create(
	count_t numBands,
	f32_t minCutoffRatio,
	f32_t maxCutoffRatio)
{
	return MakeRef<FilterBank>(
		MakeOwn<FilterBank::Impl>(
			numBands,
			minCutoffRatio,
			maxCutoffRatio));
}

}

