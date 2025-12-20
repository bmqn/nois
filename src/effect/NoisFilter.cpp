#include "nois/effect/NoisFilter.hpp"

#include "nois/util/NoisSmallVector.hpp"

namespace nois {

class Filter::Impl
{
public:
	NOIS_INTERFACE_IMPL_MULTI()
	NOIS_INTERFACE_IMPL_MULTI_PARAM(CutoffRatio, FloatBlockParameter)

	virtual f32_t GetResponseMagnitude(f32_t ratio) const = 0;
};

class BandpassFilter::Impl
{
public:
	NOIS_INTERFACE_IMPL_MULTI()
	NOIS_INTERFACE_IMPL_MULTI_PARAM(CutoffRatio, FloatBlockParameter)
	NOIS_INTERFACE_IMPL_MULTI_PARAM(Q, FloatBlockParameter)

	virtual f32_t GetResponseMagnitude(f32_t ratio) const = 0;
};


class AllpassFilter::Impl
{
public:
	NOIS_INTERFACE_IMPL_MULTI()
	NOIS_INTERFACE_IMPL_MULTI_PARAM(CutoffRatio, FloatBlockParameter)
	NOIS_INTERFACE_IMPL_MULTI_PARAM(Q, FloatBlockParameter)

	virtual f32_t GetResponseMagnitude(f32_t ratio) const = 0;
};

class N2ButterworthFilterLowImpl : public Filter::Impl
{
public:
	N2ButterworthFilterLowImpl()
	{
		m_Biquad.MakeButterworthLow(m_CutoffRatio.Default());
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("N2ButterworthFilterLowImpl - Consume");

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			m_Biquad.Process(inBuffer.View(c), outBuffer.View(c), m_NumFrames, c);
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("N2ButterworthFilterLowImpl - Prepare");

		if (m_CutoffRatio.Changed())
		{
			m_Biquad.MakeButterworthLow(m_CutoffRatio.Get());
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetCutoffRatio(Ref_t<FloatBlockParameter> cutoffRatio) override final
	{
		m_CutoffRatio.Use(cutoffRatio);
	}

	f32_t GetResponseMagnitude(f32_t freqRatio) const override final
	{
		return m_Biquad.GetMagnitude(freqRatio);
	}

private:
	SlotBlockParameter<f32_t> m_CutoffRatio = 1.0f;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	Biquad<f32_t> m_Biquad;

};

class N2ButterworthFilterHighImpl : public Filter::Impl
{
public:
	N2ButterworthFilterHighImpl()
	{
		m_Biquad.MakeButterworthHigh(m_CutoffRatio.Default());
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("N2ButterworthFilterHighImpl - Consume");

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			m_Biquad.Process(inBuffer.View(c), outBuffer.View(c), m_NumFrames, c);
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("N2ButterworthFilterHighImpl - Prepare");

		if (m_CutoffRatio.Changed())
		{
			m_Biquad.MakeButterworthHigh(m_CutoffRatio.Get());
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetCutoffRatio(Ref_t<FloatBlockParameter> cutoffRatio) override final
	{
		m_CutoffRatio.Use(cutoffRatio);
	}

	f32_t GetResponseMagnitude(f32_t freqRatio) const override final
	{
		return m_Biquad.GetMagnitude(freqRatio);
	}

private:
	SlotBlockParameter<f32_t> m_CutoffRatio = 1.0f;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	Biquad<f32_t> m_Biquad;
};

class LR4FilterLowImpl : public Filter::Impl
{
public:
	LR4FilterLowImpl()
	{
		m_Biquad1.MakeButterworthLow(m_CutoffRatio.Default());
		m_Biquad2.MakeButterworthLow(m_CutoffRatio.Default());
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("LR4FilterLowImpl - Consume");

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			auto inBufferView = inBuffer.View(c);
			auto outBufferView = outBuffer.View(c);
			m_Biquad1.Process(inBufferView, outBufferView, m_NumFrames, c);
			m_Biquad2.Process(outBufferView, outBufferView, m_NumFrames, c);
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("LR4FilterLowImpl - Prepare");

		if (m_CutoffRatio.Changed())
		{
			f32_t cutoffRatio = m_CutoffRatio.Get();
			m_Biquad1.MakeButterworthLow(cutoffRatio);
			m_Biquad2.MakeButterworthLow(cutoffRatio);
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetCutoffRatio(Ref_t<FloatBlockParameter> cutoffRatio) override final
	{
		m_CutoffRatio.Use(cutoffRatio);
	}

	f32_t GetResponseMagnitude(f32_t freqRatio) const override final
	{
		return m_Biquad1.GetMagnitude(freqRatio) * m_Biquad2.GetMagnitude(freqRatio);
	}

private:
	SlotBlockParameter<f32_t> m_CutoffRatio = 1.0f;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	Biquad<f32_t> m_Biquad1;
	Biquad<f32_t> m_Biquad2;
};

class LR4FilterHighImpl : public Filter::Impl
{
public:
	LR4FilterHighImpl()
	{
		m_Biquad1.MakeButterworthHigh(m_CutoffRatio.Default());
		m_Biquad2.MakeButterworthHigh(m_CutoffRatio.Default());
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("LR4FilterHighImpl - Consume");

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			auto inBufferView = inBuffer.View(c);
			auto outBufferView = outBuffer.View(c);
			m_Biquad1.Process(inBufferView, outBufferView, m_NumFrames, c);
			m_Biquad2.Process(outBufferView, outBufferView, m_NumFrames, c);
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("LR4FilterHighImpl - Prepare");

		if (m_CutoffRatio.Changed())
		{
			f32_t cutoffRatio = m_CutoffRatio.Get();
			m_Biquad1.MakeButterworthHigh(cutoffRatio);
			m_Biquad2.MakeButterworthHigh(cutoffRatio);
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetCutoffRatio(Ref_t<FloatBlockParameter> cutoffRatio) override final
	{
		m_CutoffRatio.Use(cutoffRatio);
	}

	f32_t GetResponseMagnitude(f32_t freqRatio) const override final
	{
		return m_Biquad1.GetMagnitude(freqRatio) * m_Biquad2.GetMagnitude(freqRatio);
	}

private:
	SlotBlockParameter<f32_t> m_CutoffRatio = 1.0f;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	Biquad<f32_t> m_Biquad1;
	Biquad<f32_t> m_Biquad2;
};

class RBJBiquadAllpassFilterImpl : public AllpassFilter::Impl
{
public:
	RBJBiquadAllpassFilterImpl()
	{
		m_Biquad.MakeAllpass(m_CutoffRatio.Default(), m_Q.Default());
	}

	Stream::Result Process(
		const FloatBufferView& inBuffer,
		FloatBuffer& outBuffer) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("LR4FilterHighImpl - Consume");

		for (count_t c = 0; c < m_NumChannels; ++c)
		{
			m_Biquad.Process(inBuffer.View(c), outBuffer.View(c), m_NumFrames, c);
		}

		return Stream::Success;
	}

	void Prepare(
		count_t numFrames,
		count_t numChannels,
		f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE_NAMED("LR4FilterHighImpl - Prepare");

		if (m_CutoffRatio.Changed() ||
			m_Q.Changed())
		{
			m_Biquad.MakeAllpass(m_CutoffRatio.Get(), m_Q.Get());
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
	}

	void SetCutoffRatio(Ref_t<FloatBlockParameter> cutoffRatio) override final
	{
		m_CutoffRatio.Use(cutoffRatio);
	}

	void SetQ(Ref_t<FloatBlockParameter> q) override final
	{
		m_Q.Use(q);
	}

	f32_t GetResponseMagnitude(f32_t freqRatio) const override final
	{
		return m_Biquad.GetMagnitude(freqRatio);
	}

private:
	SlotBlockParameter<f32_t> m_CutoffRatio = 1.0f;
	SlotBlockParameter<f32_t> m_Q = 1.0f / std::numbers::sqrt2;
	count_t m_NumFrames = 0;
	count_t m_NumChannels = 0;
	Biquad<f32_t> m_Biquad;
};

NOIS_INTERFACE_IMPL(Filter)
NOIS_INTERFACE_PARAM_IMPL(Filter, CutoffRatio, FloatBlockParameter)

NOIS_INTERFACE_IMPL(BandpassFilter)
NOIS_INTERFACE_PARAM_IMPL(BandpassFilter, CutoffRatio, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(BandpassFilter, Q, FloatBlockParameter)

NOIS_INTERFACE_IMPL(AllpassFilter)
NOIS_INTERFACE_PARAM_IMPL(AllpassFilter, CutoffRatio, FloatBlockParameter)
NOIS_INTERFACE_PARAM_IMPL(AllpassFilter, Q, FloatBlockParameter)

Ref_t<Filter> Filter::Create(Kind kind)
{
	switch (kind)
	{
		case nois::Filter::k_N2ButterworthLow:
			return MakeRef<Filter>(MakeOwn<N2ButterworthFilterLowImpl>());
		case nois::Filter::k_N2ButterworthHigh:
			return MakeRef<Filter>(MakeOwn<N2ButterworthFilterHighImpl>());
		case nois::Filter::k_LR4Low:
			return MakeRef<Filter>(MakeOwn<LR4FilterLowImpl>());
		case nois::Filter::k_LR4High:
			return MakeRef<Filter>(MakeOwn<LR4FilterHighImpl>());
		default:
			break;
	}

	return nullptr;
}

Ref_t<BandpassFilter> BandpassFilter::Create(Kind kind)
{
	switch (kind)
	{
	default:
		break;
	}

	return nullptr;
}

Ref_t<AllpassFilter> AllpassFilter::Create(Kind kind)
{
	switch (kind)
	{
	case nois::AllpassFilter::k_RBJBiquad:
		return MakeRef<AllpassFilter>(MakeOwn<RBJBiquadAllpassFilterImpl>());
	default:
		break;
	}

	return nullptr;
}

}

