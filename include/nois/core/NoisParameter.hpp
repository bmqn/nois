#pragma once

#include "nois/NoisTypes.hpp"

namespace nois {

template<typename T>
using BinderFunc_t = std::function<T()>;

using ImplType_t = float;

enum class ParameterType
{
	Float
};

class Parameter
{
public:
	virtual ~Parameter() {}

	// TODO: We should make block-based parameters for things that don't change per-parameter
	template<typename T = ImplType_t>
	T Get(count_t sampleIndex = 0) const;

	// TODO: We should make block-based parameters for things that don't change per-parameter
	template<typename T = ImplType_t>
	bool Changed(count_t sampleIndex = 0) const;

	virtual void PrepareToGet(count_t numSamples,
	                          int32_t sampleRate,
	                          int32_t numChannels) {}

	virtual ParameterType GetType() const = 0;

protected:
	virtual ImplType_t GetImpl(count_t sampleIndex) const = 0;
	virtual bool ChangedImpl(count_t sampleIndex) const = 0;
};

class FloatParameter : public Parameter
{
public:
	virtual ~FloatParameter() {}

	virtual ParameterType GetType() const = 0;

protected:
	virtual ImplType_t GetImpl(count_t sampleIndex) const = 0;
	virtual bool ChangedImpl(count_t sampleIndex) const = 0;
};

class FloatConstantParameter : public FloatParameter
{
public:
	FloatConstantParameter(float value)
		: m_Value(value)
	{}

	virtual ParameterType GetType() const override
	{
		return ParameterType::Float;
	}

	void Set(float value) { m_Value = value; }

	float *GetPtr() { return &m_Value; }

protected:
	virtual ImplType_t GetImpl(count_t sampleIndex) const override { return m_Value; }
	virtual bool ChangedImpl(count_t sampleIndex) const override { return false; }

private:
	float m_Value;
};

class FloatBinderParameter : public FloatParameter
{
public:
	FloatBinderParameter(BinderFunc_t<float> binder)
		: m_Binder(binder)
	{
	}

	virtual void PrepareToGet(count_t numSamples,
	                          int32_t sampleRate,
	                          int32_t numChannels) override
	{
		if (m_NumSamples != numSamples)
		{
			m_Values.resize(numSamples);
			m_Changes.resize(numSamples);

			m_NumSamples = numSamples;
		}

		if (m_Binder)
		{
			for (count_t i = 0; i < numSamples; ++i)
			{
				float value = m_Binder();

				m_Changes[i] = i > 0
					? value != m_Values[i - 1]
					: value != m_LastValue;

				m_Values[i] = value;
			}

			m_LastValue = m_Values.back();
		}
		else
		{
			std::fill(m_Values.begin(), m_Values.end(), 0.0f);
			std::fill(m_Changes.begin(), m_Changes.end(), false);
		}
	}

	virtual ParameterType GetType() const override
	{
		return ParameterType::Float;
	}

	void Bind(BinderFunc_t<float> binder) { m_Binder = binder; }

protected:
	virtual ImplType_t GetImpl(count_t sampleIndex) const override
	{
		if (sampleIndex >= m_Values.size())
		{
			return 0.0f;
		}

		return m_Values[sampleIndex];
	}

	virtual bool ChangedImpl(count_t sampleIndex) const override
	{
		if (sampleIndex >= m_Changes.size())
		{
			return false;
		}

		return m_Changes[sampleIndex];
	}

private:
	BinderFunc_t<float> m_Binder;

	count_t m_NumSamples = 0;
	float m_LastValue = 0.0f;
	std::vector<float> m_Values;
	std::vector<bool> m_Changes;
};

template<typename T>
T Parameter::Get(count_t sampleIndex) const
{
	return static_cast<T>(GetImpl(sampleIndex));
}

template<typename T>
bool Parameter::Changed(count_t sampleIndex) const
{
	// TODO: This should mean 'changed' from perspective of destination type!
	return ChangedImpl(sampleIndex);
}

Ref_t<FloatBinderParameter> CreateParameter(BinderFunc_t<float> binder);

}
