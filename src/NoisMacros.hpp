#pragma once

#include "nois/NoisMacros.hpp"

#define NOIS_INTERFACE_IMPL_MULTI() \
	public: \
	Impl() = default; \
	virtual ~Impl() {} \
	virtual Stream::Result Process(ConstFloatBufferView, FloatBufferView) = 0; \
	virtual void Prepare(count_t, count_t, f32_t) = 0;

#define NOIS_INTERFACE_IMPL_MULTI_PARAM(_name, _type) \
	public: \
	virtual void Set##_name(Ref_t<_type> value) = 0;

#define NOIS_INTERFACE_IMPL(_class) \
	_class::_class(Own_t<Impl> impl) \
	: m_Impl(std::move(impl)) \
	{ \
	} \
	Stream::Result _class::Process(ConstFloatBufferView inBuffer, FloatBufferView outBuffer) \
	{ \
		return m_Impl->Process(inBuffer, outBuffer); \
	} \
	void _class::Prepare(count_t numFrames, count_t numChannels, f32_t sampleRate) \
	{ \
		m_Impl->Prepare(numFrames, numChannels, sampleRate); \
	}

#define NOIS_INTERFACE_PARAM_IMPL(_class, _name, _type) \
	void _class::Set##_name(Ref_t<_type> value) \
	{ \
		m_Impl->Set##_name(value); \
	}
