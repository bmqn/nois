#pragma once

#include "nois/NoisTypes.hpp"

namespace nois {

template<typename T>
using BinderFunc_t = std::function<T()>;

class FloatParameter
{
public:
	virtual float Get() = 0;
	virtual bool Changed() = 0;
	virtual float Peek() const = 0;

	virtual operator float() const = 0;
};

class FloatConstantParameter : public FloatParameter
{
public:
	FloatConstantParameter(float value)
		: m_Value(value)
	{}

	void Set(float value) { m_Value = value; }

	virtual float Get() override { return m_Value; }
	virtual bool Changed() override { return false; }
	virtual float Peek() const override { return m_Value; }

	virtual operator float() const override { return m_Value; }

private:
	float m_Value;
};

class FloatBinderParameter : public FloatParameter
{
public:
	FloatBinderParameter(BinderFunc_t<float> binder)
		: m_Binder(binder)
		, m_Value(0.0f)
		, m_Changed(false)
	{
	}

	void Bind(BinderFunc_t<float> binder) { m_Binder = binder; }

	virtual float Get() override { return GetImpl(); }
	virtual bool Changed() override { return ChangedImpl(); }
	virtual float Peek() const override { return PeekImpl(); }

	operator float() const override { return PeekImpl(); }

private:
	float GetImpl()
	{
		float value = m_Binder ? m_Binder() : 0.0f;

		if (value != m_Value.exchange(value, std::memory_order_acquire))
		{
			m_Changed.store(true, std::memory_order_release);
		}
		else
		{
			m_Changed.store(false, std::memory_order_release);
		}

		return value;
	}

	bool ChangedImpl()
	{
		bool changed = m_Changed.load(std::memory_order_acquire);

		if (!changed)
		{
			float value = m_Binder ? m_Binder() : 0.0f;

			changed = value != m_Value.load(std::memory_order_acquire);

			m_Changed.store(changed, std::memory_order_release);
		}

		return changed;
	}

	float PeekImpl() const
	{
		return m_Value.load(std::memory_order_acquire);
	}

private:
	BinderFunc_t<float> m_Binder;
	std::atomic<float> m_Value;
	std::atomic_bool m_Changed;
};

Ref_t<FloatBinderParameter> CreateParameter(BinderFunc_t<float> binder);

}
