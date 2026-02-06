#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

#include <unordered_map>

namespace nois {

template<typename T>
class ParameterRegistry;

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

using FloatParameterRegistry = ParameterRegistry<f32_t>;

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
class ParameterRegistry
{
	// TODO: create parameters through this manager so they're auto-registered

private:
	enum class NodeState : uint8_t
	{
		Unvisited,
		Visited
	};

	template <typename P>
	struct Node
	{
		Ref_t<P> parameter = nullptr;
		std::vector<Node<P>*> dependencies;
		NodeState state = NodeState::Unvisited;
	};

public:
	Ref_t<Parameter<T>> CreateConstant(T value)
	{
		if (m_ParameterNodes.full())
		{
			return nullptr;
		}

		auto parameter = MakeRef<ConstantParameter<T>>(value);
		
		Node<Parameter<T>> node;
		node.parameter = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, &m_ParameterNodes.back());

		return parameter;
	}

	Ref_t<BlockParameter<T>> CreateBlockConstant(T value)
	{
		if (m_BlockParameterNodes.full())
		{
			return nullptr;
		}

		auto parameter = MakeRef<ConstantBlockParameter<T>>(value);
		
		Node<BlockParameter<T>> node;
		node.parameter = parameter;
		m_BlockParameterNodes.emplace_back(node);
		m_BlockParameterLookup.emplace(parameter, &m_BlockParameterNodes.back());

		return parameter;
	}

	template<typename F>
	Ref_t<Parameter<T>> CreateBinder(F&& binder)
	{
		if (m_ParameterNodes.full())
		{
			return nullptr;
		}

		auto parameter = MakeRef<BinderParameter<T, F>>(std::move(binder));
		
		Node<Parameter<T>> node;
		node.parameter = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, &m_ParameterNodes.back());

		return parameter;
	}

	template<typename F>
	Ref_t<BlockParameter<T>> CreateBlockBinder(F&& binder, T min, T max)
	{
		if (m_BlockParameterNodes.full())
		{
			return nullptr;
		}

		auto parameter = MakeRef<BinderBlockParameter<T, F>>(std::move(binder), min, max);
		
		Node<BlockParameter<T>> node;
		node.parameter = parameter;
		m_BlockParameterNodes.emplace_back(node);
		m_BlockParameterLookup.emplace(parameter, &m_BlockParameterNodes.back());

		return parameter;
	}

	template<typename F>
	Ref_t<Parameter<T>> CreateTransformer(Ref_t<Parameter<T>> transformee, F&& transformer)
	{
		if (m_ParameterNodes.full())
		{
			return nullptr;
		}

		auto parameter = MakeRef<TransformerParameter<T, F>>(transformee, std::move(transformer));
		
		Node<Parameter<T>> node;
		node.dependencies.emplace_back(m_ParameterLookup[transformee]);
		node.parameter = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, &m_ParameterNodes.back());

		return parameter;
	}

	template<typename F>
	Ref_t<BlockParameter<T>> CreateBlockTransformer(Ref_t<BlockParameter<T>> transformee, F&& transformer)
	{
		if (m_BlockParameterNodes.full())
		{
			return nullptr;
		}

		auto parameter = MakeRef<TransformerBlockParameter<T, F>>(transformee, std::move(transformer));
	
		Node<BlockParameter<T>> node;
		node.dependencies.emplace_back(m_BlockParameterLookup[transformee]);
		node.parameter = parameter;
		m_BlockParameterNodes.emplace_back(node);
		m_BlockParameterLookup.emplace(parameter, &m_BlockParameterNodes.back());

		return parameter;
	}

	void PrepareParameterVisit(Node<Parameter<T>>* node, count_t numFrames, f32_t sampleRate)
	{
		if (node->state == NodeState::Visited)
		{
			return;
		}

		for (auto* depNode : node->dependencies)
		{
			PrepareParameterVisit(depNode, numFrames, sampleRate);
		}

		node->parameter->Prepare(numFrames, sampleRate);
		node->state = NodeState::Visited;
	}

	void PrepareBlockParameterVisit(Node<BlockParameter<T>>* node, f32_t sampleRate)
	{
		if (node->state == NodeState::Visited)
		{
			return;
		}

		for (auto* depNode : node->dependencies)
		{
			PrepareBlockParameterVisit(depNode, sampleRate);
		}

		node->parameter->Prepare(sampleRate);
		node->state = NodeState::Visited;
	}

	void Prepare(count_t numFrames, f32_t sampleRate)
	{
		for (auto& node : m_ParameterNodes)
		{
			node.state = NodeState::Unvisited;
		}

		for (auto& node : m_BlockParameterNodes)
		{
			node.state = NodeState::Unvisited;
		}

		for (auto& node : m_ParameterNodes)
		{
			PrepareParameterVisit(&node, numFrames, sampleRate);
		}

		for (auto& node : m_BlockParameterNodes)
		{
			PrepareBlockParameterVisit(&node, sampleRate);
		}
	}

private:
	SmallVector<Node<Parameter<T>>, 64> m_ParameterNodes;
	std::unordered_map<Ref_t<Parameter<T>>, Node<Parameter<T>>*> m_ParameterLookup;

	SmallVector<Node<BlockParameter<T>>, 64> m_BlockParameterNodes;
	std::unordered_map<Ref_t<BlockParameter<T>>, Node<BlockParameter<T>>*> m_BlockParameterLookup;
};

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
class Parameter
{
	friend class ParameterRegistry<T>;

public:
	virtual ~Parameter() {}

	virtual ParameterBlock<T> Get() const = 0;

	virtual void Prepare(count_t numFrames, f32_t sampleRate)
	{
	}
};

template<typename T>
class ConstantParameter : public Parameter<T>
{
public:
	ConstantParameter(T value)
		: m_Value(value)
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
			frame.value = m_Value;
			frame.changed = false;
		}
	}

private:
	T m_Value;
	SmallVector<typename ParameterBlock<T>::Frame, k_MaxNumInplaceFrames> m_Frames;
};

template<typename T, typename F>
class BinderParameter : public Parameter<T>
{
public:
	BinderParameter(
		F&& binder)
		: m_Binder(std::forward<F>(binder))
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
		, m_Transformer(std::forward<F>(transformer))
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
			auto& frame = m_Frames[f];
			T value = m_Transformer(block.Get(f), f);
			frame.value = value;
			frame.changed = f > 0
				? value != m_Frames[f - 1].value
				: value != m_Frames.back().value;
		}
	}

private:
	Ref_t<Parameter<T>> m_Used;
	F m_Transformer;
	SmallVector<typename ParameterBlock<T>::Frame, k_MaxNumInplaceFrames> m_Frames;
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

	virtual void Prepare(f32_t sampleRate)
	{
	}
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

private:
	T m_Value;
};

template<typename T, typename F>
class BinderBlockParameter : public BlockParameter<T>
{
public:
	BinderBlockParameter(F&& binder, T min, T max)
		: m_Binder(std::forward<F>(binder))
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
		, m_Transformer(std::forward<F>(transformer))
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
		T value = std::clamp(m_Transformer(m_Used->Get()), m_Min, m_Max);
		// TODO: this new min/max isn't always true, make user specify it
		m_Min = m_Transformer(m_Used->Min());
		m_Max = m_Transformer(m_Used->Max());
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
		, m_Default{value, false}
	{
	}

	void Use(Ref_t<Parameter<T>> parameter)
	{
		m_Used = parameter;
	}

	ParameterBlock<T> Get() const
	{
		return m_Used ? m_Used->Get() : ParameterBlock<T>(&m_Default);
	}

private:
	Ref_t<Parameter<T>> m_Used;
	ParameterBlock<T>::Frame m_Default;
};

template<typename T>
class SlotBlockParameter
{
public:
	SlotBlockParameter(T value, T min, T max)
		: m_Used(nullptr)
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
		}
	}

	T Get() const
	{
		return m_Used ? m_Used->Get() : m_Default;
	}

	T Min() const
	{
		return m_Used ? m_Used->Min() : m_Min;
	}

	T Max() const
	{
		return m_Used ? m_Used->Max() : m_Max;
	}

	bool Changed() const
	{
		return m_Used ? m_Used->Changed() : false;
	}

private:
	Ref_t<BlockParameter<T>> m_Used;
	T m_Min;
	T m_Max;
	T m_Default;
};

}
