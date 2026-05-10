#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

#include <unordered_map>
#include <vector>

namespace nois {

template<typename T>
class Registry;

using FloatRegistry = Registry<f32_t>;

template<typename T>
class Registry
{
	// TODO: create processors here
	// They'll be auto-registered and dependencies can be resolved.

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
		std::vector<size_t> dependencies;
		NodeState state = NodeState::Unvisited;
	};

public:
	Ref_t<Parameter<T>> CreateConstant(T value)
	{
		auto parameter = MakeRef<ConstantParameter<T>>(value);
		parameter->mRegistry = this;
		
		Node<Parameter<T>> node;
		node.parameter = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, m_ParameterNodes.size() - 1);

		return parameter;
	}

	Ref_t<BlockParameter<T>> CreateBlockConstant(T value)
	{
		auto parameter = MakeRef<ConstantBlockParameter<T>>(value);
		parameter->mRegistry = this;
		
		Node<BlockParameter<T>> node;
		node.parameter = parameter;
		m_BlockParameterNodes.emplace_back(node);
		m_BlockParameterLookup.emplace(parameter, m_BlockParameterNodes.size() - 1);

		return parameter;
	}

	template<typename F>
	Ref_t<Parameter<T>> CreateBinder(F&& binder)
	{
		auto parameter = MakeRef<BinderParameter<T, F>>(std::move(binder));
		parameter->mRegistry = this;
		
		Node<Parameter<T>> node;
		node.parameter = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, m_ParameterNodes.size() - 1);

		return parameter;
	}

	template<typename F>
	Ref_t<BlockParameter<T>> CreateBlockBinder(F&& binder, T min, T max)
	{
		auto parameter = MakeRef<BinderBlockParameter<T, F>>(std::move(binder), min, max);
		parameter->mRegistry = this;
		
		Node<BlockParameter<T>> node;
		node.parameter = parameter;
		m_BlockParameterNodes.emplace_back(node);
		m_BlockParameterLookup.emplace(parameter, m_BlockParameterNodes.size() - 1);

		return parameter;
	}

	template<typename F, typename... Params>
	Ref_t<Parameter<T>> CreateTransformer(F&& transformer, Params&&... transformees)
	{
		Node<Parameter<T>> node;

		// Add the dependencies
		(node.dependencies.emplace_back(m_ParameterLookup[transformees]), ...);

		auto parameter =
			MakeRef<TransformerParameter<T, std::decay_t<F>, Params...>>(
				std::forward<F>(transformer),
				std::forward<Params>(transformees)...);

		parameter->mRegistry = this;

		node.parameter = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, m_ParameterNodes.size() - 1);

		return parameter;
	}

	template<typename F, typename... Params>
	Ref_t<BlockParameter<T>> CreateBlockTransformer(F&& transformer, Params&&... transformees)
	{
		// TODO: type the transformer function to also provide min/max etc.

		Node<BlockParameter<T>> node;

		// Add the dependencies
		(node.dependencies.emplace_back(m_BlockParameterLookup[transformees]), ...);

		auto parameter =
			MakeRef<TransformerBlockParameter<T, std::decay_t<F>, Params...>>(
				std::forward<F>(transformer),
				std::forward<Params>(transformees)...);

		parameter->mRegistry = this;

		node.parameter = parameter;
		m_BlockParameterNodes.emplace_back(node);
		m_BlockParameterLookup.emplace(parameter, m_BlockParameterNodes.size() - 1);

		return parameter;
	}

	void PrepareParameterVisit(Node<Parameter<T>>* node, count_t numFrames, f32_t sampleRate)
	{
		if (node->state == NodeState::Visited)
		{
			return;
		}

		for (auto index : node->dependencies)
		{
			PrepareParameterVisit(&m_ParameterNodes[index], numFrames, sampleRate);
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

		for (auto index : node->dependencies)
		{
			PrepareBlockParameterVisit(&m_BlockParameterNodes[index], sampleRate);
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
	std::unordered_map<Ref_t<Parameter<T>>, size_t> m_ParameterLookup;

	SmallVector<Node<BlockParameter<T>>, 64> m_BlockParameterNodes;
	std::unordered_map<Ref_t<BlockParameter<T>>, size_t> m_BlockParameterLookup;
};

} // namespace nois
