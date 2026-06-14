#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/core/NoisParameter.hpp"
#include "nois/core/NoisStream.hpp"

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

	struct ParameterNode
	{
		Ref_t<Parameter<T>> object = nullptr;
		std::vector<size_t> dependencies;
		NodeState state = NodeState::Unvisited;
	};
	
	struct StreamNode
	{
		Ref_t<Stream<T>> object = nullptr;
		std::vector<size_t> dependencies;
		NodeState state = NodeState::Unvisited;
		Buffer<T> buffer;
	};

public:
	Registry()
		: m_NumFrames(0)
		, m_NumChannels(0)
		, m_SampleRate(0.0f)
		, m_SourceIndex(0)
		, m_SinkIndex(0)
	{
	}

	template<typename F>
	Ref_t<SampleParameter<T>> CreateSampleBinder(F&& binder)
	{
		auto parameter = MakeRef<BinderSampleParameter<T, F>>(std::move(binder));
		parameter->mRegistry = this;
		
		ParameterNode node;
		node.object = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, m_ParameterNodes.size() - 1);

		return parameter;
	}

	template<typename F>
	Ref_t<BlockParameter<T>> CreateBlockBinder(F&& binder)
	{
		auto parameter = MakeRef<BinderBlockParameter<T, F>>(std::move(binder));
		parameter->mRegistry = this;
		
		ParameterNode node;
		node.object = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, m_ParameterNodes.size() - 1);

		return parameter;
	}

	template<typename F, typename... Params>
	Ref_t<Parameter<T>> CreateTransformer(F&& transformer, Params&&... transformees)
	{
		ParameterNode node;

		// Add the dependencies
		(node.dependencies.emplace_back(m_ParameterLookup[transformees]), ...);

		auto parameter =
			MakeRef<TransformerParameter<T, std::decay_t<F>, Params...>>(
				std::forward<F>(transformer),
				std::forward<Params>(transformees)...);
		parameter->mRegistry = this;

		node.object = parameter;
		m_ParameterNodes.emplace_back(node);
		m_ParameterLookup.emplace(parameter, m_ParameterNodes.size() - 1);

		return parameter;
	}
	
	template<typename S, typename... Args>
	Ref_t<S> CreateStream(Args&&... args)
	{
		auto stream =
			S::Create(
				std::forward<Args>(args)...);
		stream->mRegistry = this;

		StreamNode node;
		node.object = stream;
		
		m_StreamNodes.emplace_back(node);
		m_StreamLookup.emplace(stream, m_StreamNodes.size() - 1);

		return stream;
	}

	void Run(ConstBufferView<T> inBuffer, BufferView<T> outBuffer, f32_t sampleRate)
	{
		NOIS_PROFILE_SCOPE_NAMED("Run Graph");
		
		count_t numFrames = inBuffer.GetNumFrames();
		count_t numChannels = inBuffer.GetNumChannels();
		
		{
			NOIS_PROFILE_SCOPE_NAMED("Update Parameters");
			
			// TODO: prepare when MetaParameter changes
			bool doPrepare =
				numFrames != m_NumFrames ||
				sampleRate != m_SampleRate;
			
			for (auto& node : m_ParameterNodes)
			{
				node.state = NodeState::Unvisited;
			}
			
			for (auto& node : m_ParameterNodes)
			{
				ParameterUpdateVisit(
					&node,
					numFrames,
					sampleRate,
					doPrepare);
			}
		}
		
		{
			NOIS_PROFILE_SCOPE_NAMED("Update Streams");
			
			bool doPrepare =
				numFrames != m_NumFrames ||
				numChannels != m_NumChannels ||
				sampleRate != m_SampleRate;
			
			for (auto& node : m_StreamNodes)
			{
				node.state = NodeState::Unvisited;
			}
			
			for (auto& node : m_StreamNodes)
			{
				StreamUpdateVisit(
					&node,
					numFrames,
					numChannels,
					sampleRate,
					doPrepare);
			}
		}
		
		{
			NOIS_PROFILE_SCOPE_NAMED("Process");
			
			ScopedNoDenorms noDenorms;
			
			for (auto& node : m_StreamNodes)
			{
				node.state = NodeState::Unvisited;
			}
			
			for (auto& node : m_StreamNodes)
			{
				StreamProcessVisit(
					&node,
					inBuffer);
			}
			
			if (m_SinkIndex < m_StreamNodes.size())
			{
				outBuffer.Copy(m_StreamNodes[m_SinkIndex].buffer);
			}
		}
		
		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_SampleRate = sampleRate;
	}
	
	void SetSource(Ref_t<Stream<T>> stream)
	{
		m_SourceIndex = m_StreamLookup[stream];
	}
	
	void SetSink(Ref_t<Stream<T>> stream)
	{
		m_SinkIndex = m_StreamLookup[stream];
	}
	
private:
	void ParameterUpdateVisit(ParameterNode* node, count_t numFrames, f32_t sampleRate, bool doPrepare)
	{
		if (node->state == NodeState::Visited)
		{
			return;
		}

		for (auto index : node->dependencies)
		{
			ParameterUpdateVisit(&m_ParameterNodes[index], numFrames, sampleRate, doPrepare);
		}

		if (doPrepare)
		{
			node->object->Prepare(numFrames, sampleRate);
		}

		node->object->Update();
		
		node->state = NodeState::Visited;
	}
	
	void StreamUpdateVisit(StreamNode* node, count_t numFrames, count_t numChannels, f32_t sampleRate, bool doPrepare)
	{
		if (node->state == NodeState::Visited)
		{
			return;
		}

		for (auto index : node->dependencies)
		{
			StreamUpdateVisit(&m_StreamNodes[index], numFrames, numChannels, sampleRate, doPrepare);
		}

		if (doPrepare)
		{
			node->buffer.Resize(numFrames, numChannels);
			node->object->Prepare(numFrames, numChannels, sampleRate);
		}
		
		node->object->Update();

		node->state = NodeState::Visited;
	}
	
	Stream<T>::Result StreamProcessVisit(StreamNode* node, ConstBufferView<T> inBuffer)
	{
		if (node->state == NodeState::Visited)
		{
			return Stream<T>::Success;
		}

		typename Stream<T>::Result result = Stream<T>::Success;
		
		for (auto index : node->dependencies)
		{
			result = StreamProcessVisit(&m_StreamNodes[index], inBuffer);

			if (result != Stream<T>::Success)
			{
				break;
			}
		}

		if (node->dependencies.empty())
		{
			result = node->object->Process(inBuffer, node->buffer);
		}
		else
		{
			// TODO: mix multiple dependencies?
			auto& upstreamNode = m_StreamNodes[node->dependencies.front()];

			result = node->object->Process(upstreamNode.buffer, node->buffer);
		}
		
		node->state = NodeState::Visited;
		
		return result;
	}

private:
	count_t m_NumFrames;
	count_t m_NumChannels;
	f32_t m_SampleRate;

	std::vector<ParameterNode> m_ParameterNodes;
	std::unordered_map<Ref_t<Parameter<T>>, size_t> m_ParameterLookup;
	
	std::vector<StreamNode> m_StreamNodes;
	std::unordered_map<Ref_t<Stream<T>>, size_t> m_StreamLookup;
	size_t m_SourceIndex;
	size_t m_SinkIndex;
};

} // namespace nois
