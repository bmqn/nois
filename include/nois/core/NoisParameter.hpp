#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {

template<typename T>
class Registry;

template<typename T>
class Parameter;
template<typename T>
class ConstantParameter;
template<typename T, typename F>
class BinderParameter;
template<typename T, typename F>
class TransformerParameter;

template<typename T>
class BlockParameter;
template<typename T>
class ConstantBlockParameter;
template<typename T, typename F>
class BinderBlockParameter;
template<typename T, typename F>
class TransformerBlockParameter;

template<typename T>
class SlotParameter;
template<typename T>
class SlotBlockParameter;

using FloatParameter = Parameter<f32_t>;
using FloatConstantParameter = ConstantParameter<f32_t>;
template<typename F>
using FloatBinderParameter = BinderParameter<f32_t, F>;
template<typename F>
using FloatTransformerParameter = TransformerParameter<f32_t, F>;

using FloatBlockParameter = BlockParameter<f32_t>;
using FloatConstantBlockParameter = ConstantBlockParameter<f32_t>;
template<typename F>
using FloatBinderBlockParameter = BinderBlockParameter<f32_t, F>;
template<typename F>
using FloatTransformerBlockParameter = TransformerBlockParameter<f32_t, F>;

using FloatSlotParameter = SlotParameter<f32_t>;
using FloatSlotBlockParameter = SlotBlockParameter<f32_t>;

template<typename T>
class ParameterBlock
{
public:
	struct Frame
	{
		T value = T{ 0 };
		bool changed = false;
	};

public:
	ParameterBlock(const Frame* frames, count_t numFrames = 1)
		: m_NumFrames(numFrames)
		, m_Frames(frames)
	{
	}

	T Get(count_t f) const
	{
		return m_NumFrames > 1 ? m_Frames[f].value : m_Frames->value;
	}

	bool Changed(count_t f) const
	{
		return m_NumFrames > 1 ? m_Frames[f].changed : m_Frames->changed;
	}

private:
	count_t m_NumFrames;
	const Frame* m_Frames;
};

template<typename T>
class Parameter : public RefFromThis_t<Parameter<T>>
{
	friend class Registry<T>;

public:
	virtual ~Parameter() {}

	virtual ParameterBlock<T> Get() const = 0;
	virtual void Prepare(count_t numFrames, f32_t sampleRate) = 0;

	template<typename F>
	Ref_t<Parameter<T>> Transform(F&& transformer)
	{
		if (mRegistry)
		{
			return mRegistry->CreateTransformer(this->shared_from_this(), std::move(transformer));
		}

		return nullptr;
	}

private:
	Registry<T>* mRegistry = nullptr;
};

template<typename T>
class ConstantParameter : public Parameter<T>
{
public:
	ConstantParameter(T value)
		: m_Frame{value, false}
	{
	}

	ParameterBlock<T> Get() const final
	{
		return ParameterBlock<T>(&m_Frame);
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
	}

private:
	ParameterBlock<T>::Frame m_Frame;
};

template<typename T, typename F>
class BinderParameter : public Parameter<T>
{
public:
	BinderParameter(F&& binder)
		: m_Binder(std::move(binder))
		, m_Frames()
	{
	}

	ParameterBlock<T> Get() const final
	{
		return ParameterBlock<T>(m_Frames.data(), m_Frames.size());
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		m_Frames.resize(numFrames);

		for (count_t f = 0; f < numFrames; ++f)
		{
			auto& frame = m_Frames[f];
			T value = m_Binder(f);
			frame.value = value;
			frame.changed = f > 0
				? value != m_Frames[f - 1].value
				: value != m_Frames.back().value;
		}
	}

private:
	F m_Binder;
	SmallVector<typename ParameterBlock<T>::Frame, k_MaxNumInplaceFrames> m_Frames;
};

template<typename T, typename F>
class TransformerParameter : public Parameter<T>
{
public:
	TransformerParameter(Ref_t<Parameter<T>> parameter, F&& transformer)
		: m_Used(parameter)
		, m_Transformer(std::move(transformer))
	{
	}

	ParameterBlock<T> Get() const final
	{
		return ParameterBlock<T>(m_Frames.data(), m_Frames.size());
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		m_Frames.resize(numFrames);

		auto block = m_Used->Get();

		for (count_t f = 0; f < numFrames; ++f)
		{
			T value = 0.0f;
			auto& frame = m_Frames[f];
			if constexpr (std::is_invocable_v<F, T, count_t, f32_t>)
			{
				value = std::invoke(m_Transformer, block.Get(f), f, sampleRate);
			}
			else if constexpr (std::is_invocable_v<F, T, count_t>)
			{
				value = std::invoke(m_Transformer, block.Get(f), f);
			}
			frame.value = value;
			frame.changed = f > 0 ? value != m_Frames[f - 1].value : value != m_Frames.back().value;
		}
	}

private:
	Ref_t<Parameter<T>> m_Used;
	F m_Transformer;
	SmallVector<typename ParameterBlock<T>::Frame, k_MaxNumInplaceFrames> m_Frames;
};

template<typename T>
class BlockParameter : public RefFromThis_t<BlockParameter<T>>
{
	friend class Registry<T>;

public:
	virtual ~BlockParameter() {}

	virtual T Get() const = 0;
	virtual T Min() const = 0;
	virtual T Max() const = 0;
	virtual bool Changed() const = 0;
	virtual void Prepare(f32_t sampleRate) = 0;

	template<typename F>
	Ref_t<BlockParameter<T>> TransformBlock(F&& transformer)
	{
		if (mRegistry)
		{
			return mRegistry->CreateBlockTransformer(this->shared_from_this(), std::move(transformer));
		}

		return nullptr;
	}

private:
	Registry<T>* mRegistry = nullptr;
};

template<typename T>
class ConstantBlockParameter : public BlockParameter<T>
{
public:
	ConstantBlockParameter(T value)
		:  m_Value(value)
	{
	}

	T Get() const override final
	{
		return m_Value;
	}

	T Min() const override final
	{
		return m_Value;
	}

	T Max() const override final
	{
		return m_Value;
	}

	bool Changed() const override final
	{
		return false;
	}
	
	void Prepare(f32_t sampleRate) override final
	{
	}

private:
	T m_Value;
};

template<typename T, typename F>
class BinderBlockParameter : public BlockParameter<T>
{
public:
	BinderBlockParameter(F&& binder, T min, T max)
		: m_Binder(std::move(binder))
		, m_Min(min)
		, m_Max(max)
		, m_Value(0.0f)
		, m_Changed(false)
	{
	}

	T Get() const override final
	{
		return m_Value;
	}

	T Min() const override final
	{
		return m_Min;
	}

	T Max() const override final
	{
		return m_Max;
	}

	bool Changed() const override final
	{
		return m_Changed;
	}

	void Prepare(f32_t sampleRate) override final
	{
		T value = std::clamp(m_Binder(), m_Min, m_Max);
		m_Changed = m_Value != value;
		m_Value = value;
	}

private:
	F m_Binder;
	T m_Min;
	T m_Max;
	T m_Value;
	bool m_Changed;
};

template<typename T, typename F>
class TransformerBlockParameter : public BlockParameter<T>
{
public:
	TransformerBlockParameter(Ref_t<BlockParameter<T>> parameter, F&& transformer)
		: m_Used(parameter)
		, m_Transformer(std::move(transformer))
		, m_Min(0.0f)
		, m_Max(0.0f)
		, m_Value(0.0f)
		, m_Changed(false)
	{
	}

	T Get() const override final
	{
		return m_Value;
	}

	T Min() const override final
	{
		return m_Min;
	}

	T Max() const override final
	{
		return m_Max;
	}

	bool Changed() const override final
	{
		return m_Changed;
	}

	void Prepare(f32_t sampleRate) override final
	{
		if constexpr (std::is_invocable_v<F, T, f32_t>)
		{
			m_Min = std::invoke(m_Transformer, m_Used->Min(), sampleRate);
		}
		else if constexpr (std::is_invocable_v<F, T>)
		{
			m_Min = std::invoke(m_Transformer, m_Used->Min());
		}

		if constexpr (std::is_invocable_v<F, T, f32_t>)
		{
			m_Max = std::invoke(m_Transformer, m_Used->Max(), sampleRate);
		}
		else if constexpr (std::is_invocable_v<F, T>)
		{
			m_Max = std::invoke(m_Transformer, m_Used->Max());
		}
		
		if (m_Min > m_Max)
		{
			std::swap(m_Min, m_Max);
		}

		T value = 0.0f;
		if constexpr (std::is_invocable_v<F, T, f32_t>)
		{
			value = std::invoke(m_Transformer, m_Used->Get(), sampleRate);
		}
		else if constexpr (std::is_invocable_v<F, T>)
		{
			value = std::invoke(m_Transformer, m_Used->Get());
		}
		value = std::clamp(value, m_Min, m_Max);
		m_Changed = m_Value != value;
		m_Value = value;
	}

private:
	Ref_t<BlockParameter<T>> m_Used;
	F m_Transformer;
	T m_Min;
	T m_Max;
	T m_Value;
	bool m_Changed;
};

template<typename T>
class SlotParameter
{
public:
	SlotParameter(T value)
		: m_Used(nullptr)
		, m_Reslotted(true)
		, m_Default(value)
		, m_Frame{value, false}
	{
	}

	void Use(Ref_t<Parameter<T>> parameter)
	{
		if (m_Used != parameter)
		{
			m_Used = parameter;
			m_Reslotted = true;
		}
	}

	ParameterBlock<T> Get()
	{
		if (m_Reslotted)
		{
			m_Frame.value = m_Default;
			m_Frame.changed = true;
			m_Reslotted = false;
			return ParameterBlock<T>(&m_Frame);
		}
		else if (!m_Used)
		{
			m_Frame.value = m_Default;
			m_Frame.changed = false;
			return ParameterBlock<T>(&m_Frame);
		}

		return m_Used->Get();
	}

private:
	Ref_t<Parameter<T>> m_Used;
	bool m_Reslotted;
	T m_Default;
	typename ParameterBlock<T>::Frame m_Frame;
};

template<typename T>
class SlotBlockParameter
{
public:
	SlotBlockParameter(T value, T min, T max)
		: m_Used(nullptr)
		, m_Reslotted(true)
		, m_Min(min)
		, m_Max(max)
		, m_Default(std::clamp(value, min, max))
	{
	}

	void Use(Ref_t<BlockParameter<T>> parameter)
	{
		if (m_Used != parameter)
		{
			m_Used = parameter;
			m_Reslotted = true;
		}
	}

	T Get() const
	{
		if (!m_Used)
		{
			return m_Default;
		}

		return std::clamp(m_Used->Get(), m_Min, m_Max);
	}

	T Min() const
	{
		return m_Min;
	}

	T Max() const
	{
		return m_Max;
	}

	bool PollChanged()
	{
		if (m_Reslotted)
		{
			m_Reslotted = false;
			return true;
		}
		else if (!m_Used)
		{
			return false;
		}

		return m_Used->Changed();
	}

private:
	Ref_t<BlockParameter<T>> m_Used;
	bool m_Reslotted;
	T m_Min;
	T m_Max;
	T m_Default;
};

}
