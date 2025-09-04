#pragma once

#include "nois/NoisTypes.hpp"

namespace nois {

template<typename T>
class BlockParameter;
template<typename T>
class ConstantBlockParameter;
template<typename T>
class BinderBlockParameter;

template<typename T>
class Parameter;
template<typename T>
class ConstantParameter;
template<typename T>
class BinderParameter;

template<typename T>
using BlockBinderFunc_t = std::function<T()>;
template<typename T>
using BinderFunc_t = std::function<T(count_t)>;

using FloatBlockParameter = BlockParameter<f32_t>;
using FloatConstantBlockParameter = ConstantBlockParameter<f32_t>;
using FloatBinderBlockParameter = BinderBlockParameter<f32_t>;
using FloatBlockBinderFunc_t = BlockBinderFunc_t<f32_t>;

using FloatParameter = Parameter<f32_t>;
using FloatConstantParameter = ConstantParameter<f32_t>;
using FloatBinderParameter = BinderParameter<f32_t>;
using FloatBinderFunc_t = BinderFunc_t<f32_t>;

using FloatBlockParameterList = std::vector<Ref_t<FloatBlockParameter>>;
using FloatConstantBlockParameterList = std::vector<Ref_t<FloatConstantBlockParameter>>;

using FloatParameterList = std::vector<Ref_t<FloatParameter>>;
using FloatConstantParameterList = std::vector<Ref_t<FloatConstantParameter>>;

template<typename T>
class BlockParameter
{
	// TODO: implement block-based parameters for values that can only change per-block

public:
	virtual ~BlockParameter()
	{
	}

	virtual void Prepare(
		count_t numFrames,
		f32_t sampleRate)
	{
	}

	virtual T Get() const = 0;

	virtual bool Changed() const = 0;
};

template<typename T>
class ConstantBlockParameter : public BlockParameter<T>
{
public:
	ConstantBlockParameter(T value)
		: m_Value(value)
		, m_LastValue(value)
	{
	}

	virtual void Prepare(
		count_t numFrames,
		f32_t sampleRate) override
	{
		m_LastValue = m_Value;
	}

	virtual T Get() const override
	{
		return m_Value;
	}

	virtual bool Changed() const override
	{
		return m_LastValue != m_Value;
	}

	void Set(
		T value)
	{
		m_Value = value;
	}

	T* GetPtr()
	{
		return &m_Value;
	}

private:
	T m_Value;
	T m_LastValue;
};

template<typename T>
class BinderBlockParameter : public BlockParameter<T>
{
public:
	BinderBlockParameter(
		BlockBinderFunc_t<T> binder)
		: m_Binder(binder)
		, m_Value(T{})
		, m_LastValue(T{})
	{
	}

	virtual void Prepare(
		count_t numFrames,
		f32_t sampleRate) override
	{
		if (m_Binder)
		{
			m_LastValue = m_Value;
			m_Value = m_Binder();
		}
	}

	virtual T Get() const override
	{
		return m_Value;
	}

	virtual bool Changed() const override
	{
		return m_Value != m_LastValue;
	}

	void Bind(
		BinderFunc_t<T> binder)
	{
		m_Binder = binder;
	}

private:
	BlockBinderFunc_t<T> m_Binder;
	T m_Value;
	T m_LastValue;
};

template<typename T>
class Parameter
{
public:
	virtual ~Parameter()
	{
	}

	virtual void Prepare(
		count_t numFrames,
		f32_t sampleRate)
	{
	}

	virtual T Get(
		count_t frameIndex) const = 0;

	virtual bool Changed(
		count_t frameIndex) const = 0;

	virtual void Dirty() = 0;
};

template<typename T>
class ConstantParameter : public Parameter<T>
{
public:
	ConstantParameter(T value)
		: m_Value(value)
		, m_Change(false)
		, m_Dirty(true)
	{
	}

	virtual void Prepare(
		count_t numFrames,
		f32_t sampleRate) override
	{
		m_Change = m_Dirty;
		m_Dirty = false;
	}

	virtual T Get(
		count_t frameIndex) const override
	{
		return m_Value;
	}

	virtual bool Changed(
		count_t frameIndex) const override
	{
		return m_Change;
	}

	virtual void Dirty() override
	{
		m_Dirty = true;
	}

	void Set(
		T value)
	{
		if (m_Value != value)
		{
			m_Dirty = true;
		}

		m_Value = value;
	}

	T *GetPtr()
	{
		return &m_Value;
	}

private:
	T m_Value;
	bool m_Change;
	bool m_Dirty;
};

template<typename T>
class BinderParameter : public Parameter<T>
{
public:
	BinderParameter(
		BinderFunc_t<T> binder)
		: m_Binder(binder)
		, m_NumFrames(0)
		, m_Values(0, T{})
		, m_Changes(0, false)
		, m_LastValue(T{})
		, m_Dirty(false)
	{
	}

	virtual void Prepare(
		count_t numFrames,
		f32_t sampleRate) override
	{
		if (m_NumFrames != numFrames)
		{
			m_Values.resize(numFrames);
			m_Changes.resize(numFrames);

			m_NumFrames = numFrames;
		}

		if (m_Binder)
		{
			for (count_t i = 0; i < numFrames; ++i)
			{
				T value = m_Binder(i);

				if (m_Dirty)
				{
					m_Changes[i] = true;
				}
				else
				{
					m_Changes[i] = i > 0
						? value != m_Values[i - 1]
						: value != m_LastValue;
				}

				m_Values[i] = value;
			}

			m_LastValue = m_Values.back();
		}

		m_Dirty = false;
	}

	virtual T Get(
		count_t frameIndex) const override
	{
		if (frameIndex < 0 || frameIndex >= m_Values.size())
		{
			return T{};
		}

		return m_Values[frameIndex];
	}

	virtual bool Changed(
		count_t frameIndex) const override
	{
		if (m_Dirty)
		{
			return true;
		}

		if (frameIndex < 0 || frameIndex >= m_Changes.size())
		{
			return false;
		}

		return m_Changes[frameIndex];
	}

	virtual void Dirty() override
	{
		m_Dirty = true;
	}

	void Bind(
		BinderFunc_t<T> binder)
	{
		m_Binder = binder;
	}

private:
	BinderFunc_t<T> m_Binder;
	count_t m_NumFrames;
	std::vector<T> m_Values;
	std::vector<bool> m_Changes;
	T m_LastValue;
	bool m_Dirty;
};

template<typename T>
Ref_t<ConstantBlockParameter<T>> CreateBlockParameter(
	T value)
{
	return MakeRef<ConstantBlockParameter<T>>(value);
}

template<typename T>
Ref_t<BinderBlockParameter<T>> CreateBlockParameter(
	BlockBinderFunc_t<T> binder)
{
	return MakeRef<BinderBlockParameter<T>>(binder);
}

template<typename T>
Ref_t<ConstantParameter<T>> CreateParameter(
	T value)
{
	return MakeRef<ConstantParameter<T>>(value);
}

template<typename T>
Ref_t<BinderParameter<T>> CreateParameter(
	BinderFunc_t<T> binder)
{
	return MakeRef<BinderParameter<T>>(binder);
}

Ref_t<FloatBinderParameter> CreateFloatParameter(
	FloatBinderFunc_t binder);

}
