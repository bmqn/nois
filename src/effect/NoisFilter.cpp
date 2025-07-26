#include "nois/effect/NoisFilter.hpp"

#include "nois/NoisUtil.hpp"

namespace nois {

class N2ButterworthFilterLowImpl : public Filter
{
public:
	
	N2ButterworthFilterLowImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_CutoffRatio(MakeRef<FloatConstantParameter>(1.0f))
	{
		CalculateCoefficients(0);
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
			if (m_CutoffRatio->Changed(i))
			{
				CalculateCoefficients(i);
			}

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

		if (m_NumChannels != numChannels)
		{
			m_Inps.resize(numChannels, WindowStream<f32_t>(2));
			m_Outs.resize(numChannels, WindowStream<f32_t>(2));
		
			m_NumChannels = numChannels;
		}
	}

	virtual Ref_t<FloatParameter> GetCutoffRatio() override
	{
		return m_CutoffRatio;
	}

	virtual void SetCutoffRatio(Ref_t<FloatParameter> cutoffRatio) override
	{
		m_CutoffRatio = cutoffRatio;
	}

private:
	void CalculateCoefficients(count_t frameIndex)
	{
		constexpr f32_t k_Pi = static_cast<f32_t>(std::numbers::pi);
		constexpr f32_t k_Sqrt2 = static_cast<f32_t>(std::numbers::sqrt2);

		f32_t cutoffRatio = std::clamp(m_CutoffRatio->Get(frameIndex), 0.001f, 0.999f) * 0.5f;

		float q = 1.0f / k_Sqrt2;

		float w0 = k_Pi * cutoffRatio;
		float cosW0 = std::cos(w0);
		float sinW0 = std::sin(w0);
		float alpha = sinW0 / (2.0f * q);

		float a0 = 1.0f + alpha;
		m_B0 = (1.0f - cosW0) / 2.0f / a0;
		m_B1 = (1.0f - cosW0) / a0;
		m_B2 = (1.0f - cosW0) / 2.0f / a0;
		m_A1 = -2.0f * cosW0 / a0;
		m_A2 = (1.0f - alpha) / a0;
	}

	f32_t ConsumeChannel(count_t index, f32_t input)
	{
		WindowStream<f32_t> &inpWindow = m_Inps[index];
		WindowStream<f32_t> &outWindow = m_Outs[index];

		f32_t output = 0.0f;
		output += m_B0 * input + m_B1 * inpWindow.Get(0) + m_B2 * inpWindow.Get(1);
		output -= m_A1 * outWindow.Get(0) + m_A2 * outWindow.Get(1);

		inpWindow.Add(input);
		outWindow.Add(output);

		return output;
	}

	virtual f32_t GetResponseMagnitude(f32_t freqRatio) const override
	{
		constexpr f32_t k_Pi = static_cast<f32_t>(std::numbers::pi);

		f32_t omega = k_Pi * freqRatio;

		f32_t cosOmega = std::cos(omega);
		f32_t sinOmega = std::sin(omega);
		f32_t cos2Omega = std::cos(2.0f * omega);
		f32_t sin2Omega = std::sin(2.0f * omega);

		f32_t numReal = m_B0 + m_B1 * cosOmega + m_B2 * cos2Omega;
		f32_t numImag = m_B1 * sinOmega + m_B2 * sin2Omega;
		f32_t numMag2 = numReal * numReal + numImag * numImag;

		f32_t denReal = 1.0f + m_A1 * cosOmega + m_A2 * cos2Omega;
		f32_t denImag = m_A1 * sinOmega + m_A2 * sin2Omega;
		f32_t denMag2 = denReal * denReal + denImag * denImag;

		return std::sqrt(numMag2 / denMag2);
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_CutoffRatio;

	f32_t m_A1 = 0.0f, m_A2 = 0.0f;
	f32_t m_B0 = 0.0f, m_B1 = 0.0f, m_B2 = 0.0f;

	count_t m_NumChannels = 0;

	std::vector<WindowStream<f32_t>> m_Inps;
	std::vector<WindowStream<f32_t>> m_Outs;
};

class N2ButterworthFilterHighImpl : public Filter
{
public:
	
	N2ButterworthFilterHighImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_CutoffRatio(MakeRef<FloatConstantParameter>(1.0f))
	{
		CalculateCoefficients(0);
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
			if (m_CutoffRatio->Changed(i))
			{
				CalculateCoefficients(i);
			}

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

		if (m_NumChannels != numChannels)
		{
			m_Inps.resize(numChannels, WindowStream<f32_t>(2));
			m_Outs.resize(numChannels, WindowStream<f32_t>(2));
		
			m_NumChannels = numChannels;
		}
	}

	virtual Ref_t<FloatParameter> GetCutoffRatio() override
	{
		return m_CutoffRatio;
	}

	virtual void SetCutoffRatio(Ref_t<FloatParameter> cutoffRatio) override
	{
		m_CutoffRatio = cutoffRatio;

		CalculateCoefficients(0);
	}

private:
	void CalculateCoefficients(count_t frameIndex)
	{
		constexpr f32_t k_Pi = static_cast<f32_t>(std::numbers::pi);
		constexpr f32_t k_Sqrt2 = static_cast<f32_t>(std::numbers::sqrt2);

		f32_t cutoffRatio = std::clamp(m_CutoffRatio->Get(frameIndex), 0.001f, 0.999f) * 0.5f;

		float q = 1.0f / k_Sqrt2;

		float w0 = k_Pi * cutoffRatio;
		float cosW0 = std::cos(w0);
		float sinW0 = std::sin(w0);
		float alpha = sinW0 / (2.0f * q);

		float a0 = 1.0f + alpha;
		m_B0 = (1.0f + cosW0) / 2.0f / a0;
		m_B1 = -(1.0f + cosW0) / a0;
		m_B2 = (1.0f + cosW0) / 2.0f / a0;
		m_A1 = -2.0f * cosW0 / a0;
		m_A2 = (1.0f - alpha) / a0;
	}

	f32_t ConsumeChannel(count_t index, f32_t input)
	{
		WindowStream<f32_t> &inpWindow = m_Inps[index];
		WindowStream<f32_t> &outWindow = m_Outs[index];

		f32_t output = 0.0f;
		output += m_B0 * input + m_B1 * inpWindow.Get(0) + m_B2 * inpWindow.Get(1);
		output -= m_A1 * outWindow.Get(0) + m_A2 * outWindow.Get(1);

		inpWindow.Add(input);
		outWindow.Add(output);

		return output;
	}

	virtual f32_t GetResponseMagnitude(f32_t freqRatio) const override
	{
		constexpr f32_t k_Pi = static_cast<f32_t>(std::numbers::pi);

		f32_t omega = k_Pi * freqRatio;

		f32_t cosOmega = std::cos(omega);
		f32_t sinOmega = std::sin(omega);
		f32_t cos2Omega = std::cos(2.0f * omega);
		f32_t sin2Omega = std::sin(2.0f * omega);

		f32_t numReal = m_B0 + m_B1 * cosOmega + m_B2 * cos2Omega;
		f32_t numImag = m_B1 * sinOmega + m_B2 * sin2Omega;
		f32_t numMag2 = numReal * numReal + numImag * numImag;

		f32_t denReal = 1.0f + m_A1 * cosOmega + m_A2 * cos2Omega;
		f32_t denImag = m_A1 * sinOmega + m_A2 * sin2Omega;
		f32_t denMag2 = denReal * denReal + denImag * denImag;

		return std::sqrt(numMag2 / denMag2);
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_CutoffRatio;

	f32_t m_A1 = 0.0f, m_A2 = 0.0f;
	f32_t m_B0 = 0.0f, m_B1 = 0.0f, m_B2 = 0.0f;

	count_t m_NumChannels = 0;

	std::vector<WindowStream<f32_t>> m_Inps;
	std::vector<WindowStream<f32_t>> m_Outs;
};

class BiquadBandpassFilterImpl : public BandpassFilter
{
public:

	BiquadBandpassFilterImpl(Ref_t<Stream> stream)
		: m_Stream(stream)
		, m_CutoffRatio(MakeOwn<FloatConstantParameter>(1.0f))
		, m_Q(MakeRef<FloatConstantParameter>(1.0f))
	{
		CalculateCoefficients(0);
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
			if (m_CutoffRatio->Changed(i) ||
				m_Q->Changed(i))
			{
				CalculateCoefficients(i);
			}

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

		if (m_NumChannels != numChannels)
		{
			m_Inps.resize(numChannels, WindowStream<f32_t>(2));
			m_Outs.resize(numChannels, WindowStream<f32_t>(2));
		
			m_NumChannels = numChannels;
		}
	}

	virtual Ref_t<FloatParameter> GetCutoffRatio() override
	{
		return m_CutoffRatio;
	}

	virtual void SetCutoffRatio(Ref_t<FloatParameter> cutoffRatio) override
	{
		m_CutoffRatio = cutoffRatio;

		CalculateCoefficients(0);
	}

	virtual Ref_t<FloatParameter> GetQ() override
	{
		return m_Q;
	}

	virtual void SetQ(Ref_t<FloatParameter> q) override
	{
		m_Q = q;

		CalculateCoefficients(0);
	}

	virtual f32_t GetResponseMagnitude(f32_t freqRatio) const override
	{
		constexpr f32_t k_Pi = static_cast<f32_t>(std::numbers::pi);

		f32_t omega = k_Pi * freqRatio;

		f32_t cosOmega = std::cos(omega);
		f32_t sinOmega = std::sin(omega);
		f32_t cos2Omega = std::cos(2.0f * omega);
		f32_t sin2Omega = std::sin(2.0f * omega);

		f32_t numReal = m_B0 + m_B1 * cosOmega + m_B2 * cos2Omega;
		f32_t numImag = m_B1 * sinOmega + m_B2 * sin2Omega;
		f32_t numMag2 = numReal * numReal + numImag * numImag;

		f32_t denReal = 1.0f + m_A1 * cosOmega + m_A2 * cos2Omega;
		f32_t denImag = m_A1 * sinOmega + m_A2 * sin2Omega;
		f32_t denMag2 = denReal * denReal + denImag * denImag;

		return std::sqrt(numMag2 / denMag2);
	}

private:
	void CalculateCoefficients(count_t frameIndex)
	{
		constexpr f32_t k_Pi = static_cast<f32_t>(std::numbers::pi);

		f32_t cutoffRatio = m_CutoffRatio->Get(frameIndex);
		f32_t q = m_Q->Get(frameIndex);

		f32_t omega0 = 2.0f * k_Pi * std::clamp(cutoffRatio, 0.001f, 0.999f) * 0.5f;

		f32_t cosw0 = std::cos(omega0);
		f32_t alpha = std::sin(omega0) / (2.0f * std::max(q, 0.001f));

		f32_t a0inv = 1.0f / (1.0f + alpha);

		m_B0 = alpha * a0inv;
		m_B1 = 0.0f;
		m_B2 = -alpha * a0inv;
		m_A1 = -2.0f * cosw0 * a0inv;
		m_A2 = (1.0f - alpha) * a0inv;
	}

	f32_t ConsumeChannel(count_t frameIndex, f32_t input)
	{
		WindowStream<f32_t>& inpWindow = m_Inps[frameIndex];
		WindowStream<f32_t>& outWindow = m_Outs[frameIndex];

		f32_t output = 0.0f;
		output += m_B0 * input + m_B1 * inpWindow.Get(0) + m_B2 * inpWindow.Get(1);
		output -= m_A1 * outWindow.Get(0) + m_A2 * outWindow.Get(1);

		inpWindow.Add(input);
		outWindow.Add(output);

		return output;
	}

private:
	Ref_t<Stream> m_Stream;

	Ref_t<FloatParameter> m_CutoffRatio;
	Ref_t<FloatParameter> m_Q;

	f32_t m_A1 = 0.0f, m_A2 = 0.0f;
	f32_t m_B0 = 0.0f, m_B1 = 0.0f, m_B2 = 0.0f;

	count_t m_NumChannels = 0;

	std::vector<WindowStream<f32_t>> m_Inps;
	std::vector<WindowStream<f32_t>> m_Outs;
};

Ref_t<Filter> CreateFilter(Ref_t<Stream> stream, Filter::Kind kind)
{
	switch (kind)
	{
		case nois::Filter::k_N2ButterWorthLow:
			return MakeRef<N2ButterworthFilterLowImpl>(stream);
		case nois::Filter::k_N2ButterWorthHigh:
			return MakeRef<N2ButterworthFilterHighImpl>(stream);
		default:
			break;
	}

	return nullptr;
}

Ref_t<BandpassFilter> CreateBandpassFilter(Ref_t<Stream> stream, BandpassFilter::Kind kind)
{
	switch (kind)
	{
	case nois::BandpassFilter::k_Biquad:
		return MakeRef<BiquadBandpassFilterImpl>(stream);
	default:
		break;
	}

	return nullptr;
}

}

