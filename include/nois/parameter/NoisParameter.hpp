#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {

template<typename T>
class ParameterRegistry;

template<typename T>
class Parameter;
template<typename T>
class SlotParameter;
template<typename T>
class BinderParameter;
template<typename T>
class TransformerParameter;

template<typename T>
class BlockParameter;
template<typename T>
class ConstantBlockParameter;
template<typename T>
class SlotBlockParameter;
template<typename T>
class BinderBlockParameter;
template<typename T>
class TransformerBlockParameter;

template<typename T>
using BinderFunc_t = std::function<T(count_t)>;
template<typename T>
using BlockBinderFunc_t = std::function<T()>;

template<typename T>
using TransformerFunc_t = std::function<T(T, count_t)>;
template<typename T>
using BlockTransformerFunc_t = std::function<T(T)>;

template<typename F, typename... Args>
auto ParamBind_t(F&& f, Args &&...args)
{
	using T = std::invoke_result_t<F, Args..., count_t>;
	return BinderFunc_t<T>(std::bind(
		std::forward<F>(f),
		std::forward<Args>(args)...,
		std::placeholders::_1));
}

template<typename F, typename... Args>
auto ParamBlockBind_t(F&& f, Args &&...args)
{
	using T = std::invoke_result_t<F, Args...>;
	return BlockBinderFunc_t<T>(std::bind(
		std::forward<F>(f),
		std::forward<Args>(args)...));
}

using FloatParameterRegistry = ParameterRegistry<f32_t>;

using FloatParameter = Parameter<f32_t>;
using FloatSlotParameter = SlotParameter<f32_t>;
using FloatBinderParameter = BinderParameter<f32_t>;
using FoatTransformerParameter = TransformerParameter<f32_t>;
using FloatBinderFunc_t = BinderFunc_t<f32_t>;

using FloatBlockParameter = BlockParameter<f32_t>;
using FloatConstantBlockParameter = ConstantBlockParameter<f32_t>;
using FloatSlotBlockParameter = SlotBlockParameter<f32_t>;
using FloatBinderBlockParameter = BinderBlockParameter<f32_t>;
using FloatTransformerBlockParameter = TransformerBlockParameter<f32_t>;
using FloatBlockBinderFunc_t = BlockBinderFunc_t<f32_t>;
using FloatBlockTransformerFunc_t = BlockTransformerFunc_t<f32_t>;

template<typename T>
class ParameterRegistry
{
	// TODO: create parameters through this manager so they're auto-registered

public:
	Ref_t<Parameter<T>> CreateConstant(T value)
	{
		auto parameter = MakeRef<BinderParameter<T>>([value](count_t) { return value; });
		m_Parameters.emplace_back(parameter);
		return parameter;
	}

	Ref_t<BlockParameter<T>> CreateBlockConstant(T value)
	{
		auto parameter = MakeRef<ConstantBlockParameter<T>>(value);
		m_BlockParameters.emplace_back(parameter);
		return parameter;
	}

	Ref_t<Parameter<T>> CreateBinder(BinderFunc_t<T> binder)
	{
		auto parameter = MakeRef<BinderParameter<T>>(binder);
		m_Parameters.emplace_back(parameter);
		return parameter;
	}

	Ref_t<BlockParameter<T>> CreateBlockBinder(BlockBinderFunc_t<T> binder, T min, T max)
	{
		auto parameter = MakeRef<BinderBlockParameter<T>>(binder, min, max);
		m_BlockParameters.emplace_back(parameter);
		return parameter;
	}

	Ref_t<Parameter<T>> CreateTransformer(Ref_t<Parameter<T>> transformee, TransformerFunc_t<T> transformer)
	{
		auto parameter = MakeRef<TransformerParameter<T>>(transformee, transformer);
		m_Parameters.emplace_back(parameter);
		return parameter;
	}

	Ref_t<BlockParameter<T>> CreateBlockTransformer(Ref_t<BlockParameter<T>> transformee, BlockTransformerFunc_t<T> transformer)
	{
		auto parameter = MakeRef<TransformerBlockParameter<T>>(transformee, transformer);
		m_BlockParameters.emplace_back(parameter);
		return parameter;
	}

	void Prepare(count_t numFrames, f32_t sampleRate)
	{
		for (auto& parameter : m_Parameters)
		{
			parameter->Prepare(numFrames, sampleRate);
		}

		for (auto& parameter : m_BlockParameters)
		{
			parameter->Prepare(numFrames, sampleRate);
		}
	}

private:
	std::vector<Ref_t<Parameter<T>>> m_Parameters;
	std::vector<Ref_t<BlockParameter<T>>> m_BlockParameters;
};

template<typename T>
class Parameter
{
	friend class ParameterRegistry<T>;

public:
	virtual ~Parameter() {}

	virtual T Get(count_t f) const = 0;
	virtual bool Changed(count_t f) const = 0;

	virtual void Prepare(count_t numFrames, f32_t sampleRate)
	{
	}
};

template<typename T>
class SlotParameter : public Parameter<T>
{
public:
	SlotParameter(T value)
		: m_Default(value)
		, m_Used(nullptr)
		, m_Tranformer()
		, m_Dirty(false)
		, m_Dirtied(false)
	{
	}

	T Get(count_t f) const override final
	{
		if (m_Used)
		{
			T value = m_Used->Get(f);
			return m_Tranformer ? m_Tranformer(value, f) : value;
		}

		return m_Default;
	}

	bool Changed(count_t f) const override final
	{
		if (m_Dirty)
		{
			return true;
		}

		if (m_Used)
		{
			bool changed = m_Used->Changed(f);
			return changed;
		}

		return false;
	}

	void Use(Ref_t<Parameter<T>> parameter)
	{
		if (m_Used != parameter)
		{
			m_Used = parameter;
			m_Dirtied = true;
		}
	}

	void Transform(TransformerFunc_t<T> tranformer)
	{
		if (m_Tranformer != nullptr)
		{
			m_Tranformer = tranformer;
			m_Dirtied = true;
		}
	}

	T Default() const
	{
		return m_Default;
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		m_Dirty = m_Dirtied;
		m_Dirtied = false;
	}

private:
	T m_Default;
	Ref_t<Parameter<T>> m_Used;
	TransformerFunc_t<T> m_Tranformer;
	bool m_Dirty;
	bool m_Dirtied;
};

template<typename T>
class BinderParameter : public Parameter<T>
{
private:
	struct Frame
	{
		T value = T{ 0 };
		bool changed = false;
	};

public:
	BinderParameter(
		BinderFunc_t<T> binder)
		: m_Binder(binder)
		, m_Frames()
	{
	}

	T Get(count_t f) const override final
	{
		if (f < 0 || f >= m_Frames.size())
		{
			return T{ 0 };
		}

		return m_Frames[f].value;
	}

	bool Changed(count_t f) const override final
	{
		if (f < 0 || f >= m_Frames.size())
		{
			return false;
		}

		return m_Frames[f].changed;
	}

	void Bind(BinderFunc_t<T> binder)
	{
		m_Binder = binder;
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		m_Frames.resize(numFrames);

		if (m_Binder)
		{
			for (count_t f = 0; f < numFrames; ++f)
			{
				Frame& frame = m_Frames[f];
				T value = m_Binder(f);
				frame.value = value;
				frame.changed = f > 0
					? value != m_Frames[f - 1].value
					: value != m_Frames.back().value;
			}
		}
	}

private:
	BinderFunc_t<T> m_Binder;
	SmallVector<Frame> m_Frames;
};

template<typename T>
class TransformerParameter : public Parameter<T>
{
public:
	TransformerParameter(Ref_t<Parameter<T>> parameter, TransformerFunc_t<T> transformer)
		: m_Used(parameter)
		, m_Transformer(transformer)
	{
	}

	T Get(count_t f) const override final
	{
		return m_Transformer(m_Used->Get(f), f);
	}

	bool Changed(count_t f) const override final
	{
		return m_Used->Changed(f);
	}

private:
	Ref_t<Parameter<T>> m_Used;
	TransformerFunc_t<T> m_Transformer;
};

template<typename T>
class BlockParameter
{
	friend class ParameterRegistry<T>;

public:
	virtual ~BlockParameter() {}

	virtual T Get() const = 0;
	virtual T Min() const = 0;
	virtual T Max() const = 0;
	virtual bool Changed() const = 0;

	virtual void Prepare(count_t numFrames, f32_t sampleRate)
	{
	}
};

template<typename T>
class ConstantBlockParameter : public BlockParameter<T>
{
public:
	ConstantBlockParameter(T value)
		: m_Value(value)
		, m_Dirty(false)
		, m_Initialised(true)
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
		return m_Dirty;
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		m_Dirty = m_Initialised;
		m_Initialised = false;
	}

private:
	T m_Value;
	bool m_Dirty;
	bool m_Initialised;
};

template<typename T>
class SlotBlockParameter : public BlockParameter<T>
{
public:
	SlotBlockParameter(T value)
		: m_Default(value)
		, m_Used(nullptr)
		, m_Dirty(false)
		, m_Reslotted(false)
	{
	}

	T Get() const override final
	{
		return m_Used ? m_Used->Get() : m_Default;
	}

	T Min() const override final
	{
		return m_Used ? m_Used->Min() : m_Default;
	}

	T Max() const override final
	{
		return m_Used ? m_Used->Max() : m_Default;
	}

	bool Changed() const override final
	{
		return m_Dirty || (m_Used ? m_Used->Changed() : false);
	}

	void Use(Ref_t<BlockParameter<T>> parameter)
	{
		if (m_Used != parameter)
		{
			m_Used = parameter;
			m_Reslotted = true;
		}
	}

	T Default() const
	{
		return m_Default;
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		m_Dirty = m_Reslotted;
		m_Reslotted = false;
	}

private:
	T m_Default;
	Ref_t<BlockParameter<T>> m_Used;
	bool m_Dirty;
	bool m_Reslotted;
};

template<typename T>
class BinderBlockParameter : public BlockParameter<T>
{
public:
	BinderBlockParameter(BlockBinderFunc_t<T> binder, T min, T max)
		: m_LastValue(min)
		, m_Value(min)
		, m_Min(min)
		, m_Max(max)
		, m_Binder(binder)
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
		return m_Value != m_LastValue;
	}

	void Bind(BinderFunc_t<T> binder)
	{
		m_Binder = binder;
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		if (m_Binder)
		{
			m_LastValue = m_Value;
			m_Value = std::clamp(m_Binder(), m_Min, m_Max);
		}
	}

private:
	T m_LastValue;
	T m_Value;
	T m_Min;
	T m_Max;
	BlockBinderFunc_t<T> m_Binder;
};

template<typename T>
class TransformerBlockParameter : public BlockParameter<T>
{
public:
	TransformerBlockParameter(Ref_t<BlockParameter<T>> parameter, BlockTransformerFunc_t<T> transformer)
		: m_Used(parameter)
		, m_Transformer(transformer)
	{
	}

	T Get() const override final
	{
		return m_Transformer(m_Used->Get());
	}

	T Min() const override final
	{
		return m_Transformer(m_Used->Min());
	}

	T Max() const override final
	{
		return m_Transformer(m_Used->Max());
	}

	bool Changed() const override final
	{
		return m_Used->Changed();
	}

private:
	Ref_t<BlockParameter<T>> m_Used;
	BlockTransformerFunc_t<T> m_Transformer;
};

}
