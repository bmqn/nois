#pragma once

#include "nois/NoisTypes.hpp"

#include <algorithm>
#include <variant>
#include <vector>

namespace nois {

template<typename T>
class Registry;

template<typename T>
class Parameter;
template<typename T, typename F, typename... Params>
class TransformerParameter;

template<typename T>
class SampleParameter;
template<typename T, typename F>
class BinderSampleParameter;

template<typename T>
class BlockParameter;
template<typename T>
class ConstantBlockParameter;
template<typename T, typename F>
class BinderBlockParameter;

template<typename T>
class SlotParameter;

using FloatParameter = Parameter<f32_t>;
using FloatSampleParameter = SampleParameter<f32_t>;
using FloatBlockParameter = BlockParameter<f32_t>;

// Stream reader
// Can only parameter by streaming each value.
template<typename T>
class IStreamReader
{
public:
	struct Point
	{
		T value;
		bool changed;

		inline T Value() const { return value; }
		inline bool Changed() const { return changed; }

		inline operator T() const { return Value(); }
	};

public:
	virtual ~IStreamReader() = default;

	virtual Point Next() = 0;
};

// Smoothed reader
// Value is smoothed by interpolating towards target.
template<typename T>
class SmoothedStreamReader : public IStreamReader<T>
{
public:
	SmoothedStreamReader(Ref_t<IStreamReader<T>> reader, f32_t sampleRate, f32_t timeSec = 0.1f)
		: m_Reader(reader)
		, m_Coeff(1.0f - std::exp(-1.0f / (timeSec * sampleRate)))
	{
	}

	IStreamReader<T>::Point Next() override final
	{
		T value = T{ 0 };
		bool changed = false;
		
		auto next = m_Reader->Next();
		
		if (!m_Initialized)
		{
			m_Value = next.Value();
			m_Target = next.Value();
			m_Initialized = true;
			return next;
		}
		
		m_Target = next.Value();
		
		T prev = m_Value;
		m_Value += (m_Target - m_Value) * m_Coeff;
		
		value = m_Value;
		changed = m_Value != prev;

		return { value, changed };
	}

private:
	Ref_t<IStreamReader<T>> m_Reader = nullptr;
	f32_t m_Coeff = 0.0f;
	T m_Value = T{ 0 };
	T m_Target = T{ 0 };
	bool m_Initialized = false;
};

// Block reader
// Can read parameter at any given offset in the block.
template<typename T>
class IBlockReader
{
public:
	struct Point
	{
		T value;
		bool changed;

		inline T Value() const { return value; }
		inline bool Changed() const { return changed; }

		inline operator T() const { return Value(); }
	};

public:
	virtual ~IBlockReader() = default;

	virtual Point Get(count_t f) const = 0;
};

template<typename T>
class Parameter : public RefFromThis_t<Parameter<T>>
{
	friend class Registry<T>;

public:
	virtual ~Parameter() {}

	// Called when block size or rate changes
	virtual void Prepare(count_t numFrames, f32_t sampleRate) {}
	
	// Called for every block
	virtual void Update() {}

	virtual T Min() const { return T{ 0 }; }
	virtual T Max() const { return T{ 0 }; }

	virtual Ref_t<IStreamReader<T>> Stream() const = 0;
	virtual Ref_t<IBlockReader<T>> Block() const = 0;

	template<typename F>
	Ref_t<Parameter<T>> Transform(F&& transformer)
	{
		if (mRegistry)
		{
			return mRegistry->CreateTransformer(
				std::forward<F>(transformer),
				this->shared_from_this());
		}

		return nullptr;
	}

private:
	Registry<T>* mRegistry = nullptr;
};

template<typename T, typename F, typename... Params>
class TransformerParameter : public Parameter<T>
{
public:
	class Reader : public IStreamReader<T>, public IBlockReader<T>
	{
	public:
		struct Frame
		{
			T value = T{ 0 };
			bool changed = false;
		};

	public:
		Reader(const Frame* frames, count_t numFrames)
			: m_NumFrames(numFrames)
			, m_FrameOffset(0)
			, m_Frames(frames)
		{
		}

		IStreamReader<T>::Point Next() override final
		{
			T value = T{ 0 };
			bool changed = false;

			value = m_Frames[m_FrameOffset].value;
			changed = m_Frames[m_FrameOffset].changed;
			++m_FrameOffset;

			if (m_FrameOffset >= m_NumFrames)
			{
				m_FrameOffset = 0;
			}

			return { value, changed };
		}

		IBlockReader<T>::Point Get(count_t f) const override final
		{
			T value = T{ 0 };
			bool changed = false;

			if (f < m_NumFrames)
			{
				value = m_Frames[f].value;
				changed = m_Frames[f].changed;
			}

			return { value, changed };
		}

	private:
		count_t m_NumFrames;
		count_t m_FrameOffset;
		const Frame* m_Frames;
	};

public:
	TransformerParameter(F&& transformer, Params&&... transformees)
		: m_Transformer(std::move(transformer))
		, m_NumFrames(0)
		, m_SampleRate(0.0f)
		, m_Used({ static_cast<Ref_t<Parameter<T>>>(transformees)... })
		, m_Readables({ nullptr })
	{
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE();
		
		m_Frames.resize(numFrames);
		
		for (count_t i = 0; i < m_Used.size(); ++i)
		{
			m_Readables[i] = m_Used[i]->Block();
		}
		
		m_NumFrames = numFrames;
		m_SampleRate = sampleRate;
	}
	
	void Update() override final
	{
		NOIS_PROFILE_SCOPE();
		
		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			T value = InvokeTransformer(
				[f](auto& p)
				{
					return p->Get(f);
				},
				m_SampleRate,
				std::make_index_sequence<sizeof...(Params)>{});

			m_Frames[f].value = value;
			m_Frames[f].changed = f > 0
				? value != m_Frames[f - 1].value
				: value != m_Frames.back().value;
		}
	}

	Ref_t<IStreamReader<T>> Stream() const override final
	{
		return MakeRef<Reader>(m_Frames.data(), m_Frames.size());
	}

	Ref_t<IBlockReader<T>> Block() const override final
	{
		return MakeRef<Reader>(m_Frames.data(), m_Frames.size());
	}

private:
	template<std::size_t... Is>
	T InvokeTransformer(auto getter, f32_t sampleRate, std::index_sequence<Is...>) const
	{
		if constexpr (std::is_invocable_v<
			F,
			decltype(std::invoke(getter, m_Readables[Is]))..., f32_t>)
		{
			return std::invoke(
				m_Transformer,
				std::invoke(getter, m_Readables[Is])..., sampleRate);
		}
		else
		{
			return std::invoke(
				m_Transformer,
				std::invoke(getter, m_Readables[Is])...);
		}
	}

private:
	F m_Transformer;
	count_t m_NumFrames;
	f32_t m_SampleRate;
	std::array<Ref_t<Parameter<T>>, sizeof...(Params)> m_Used;
	std::array<Ref_t<IBlockReader<T>>, sizeof...(Params)> m_Readables;
	std::vector<typename Reader::Frame> m_Frames;
};

// Sample-accurate parameter
// Value is specified per sample over block
template<typename T>
class SampleParameter : public Parameter<T>
{
public:
	class Reader : public IStreamReader<T>, public IBlockReader<T>
	{
	public:
		struct Frame
		{
			T value = T{ 0 };
			bool changed = false;
		};

	public:
		Reader(const Frame* frames, count_t numFrames)
			: m_NumFrames(numFrames)
			, m_FrameOffset(0)
			, m_Frames(frames)
		{
		}

		IStreamReader<T>::Point Next() override final
		{
			T value = T{ 0 };
			bool changed = false;

			value = m_Frames[m_FrameOffset].value;
			changed = m_Frames[m_FrameOffset].changed;
			++m_FrameOffset;

			if (m_FrameOffset >= m_NumFrames)
			{
				m_FrameOffset = 0;
			}

			return { value, changed };
		}

		IBlockReader<T>::Point Get(count_t f) const override final
		{
			T value = T{ 0 };
			bool changed = false;

			if (f < m_NumFrames)
			{
				value = m_Frames[f].value;
				changed = m_Frames[f].changed;
			}

			return { value, changed };
		}

	private:
		count_t m_NumFrames;
		count_t m_FrameOffset;
		const Frame* m_Frames;
	};
};

template<typename T, typename F>
class BinderSampleParameter : public SampleParameter<T>
{
public:
	BinderSampleParameter(F&& binder)
		: m_Binder(std::move(binder))
		, m_NumFrames(0)
		, m_SampleRate(0.0f)
		, m_Frames()
	{
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
		NOIS_PROFILE_SCOPE();
		
		m_Frames.resize(numFrames);

		m_NumFrames = numFrames;
		m_SampleRate = sampleRate;
	}
	
	void Update() override final
	{
		NOIS_PROFILE_SCOPE();
		
		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			T value = m_Binder(f);

			m_Frames[f].value = value;
			m_Frames[f].changed = f > 0
				? value != m_Frames[f - 1].value
				: value != m_Frames.back().value;
		}
	}

	Ref_t<IStreamReader<T>> Stream() const override final
	{
		return MakeRef<typename SampleParameter<T>::Reader>(m_Frames.data(), m_Frames.size());
	}

	Ref_t<IBlockReader<T>> Block() const override final
	{
		return MakeRef<typename SampleParameter<T>::Reader>(m_Frames.data(), m_Frames.size());
	}

private:
	F m_Binder;
	count_t m_NumFrames;
	f32_t m_SampleRate;
	std::vector<typename SampleParameter<T>::Reader::Frame> m_Frames;
};

// Block parameter
// Has one value per block
template<typename T>
class BlockParameter : public Parameter<T>
{
public:
	class Reader : public IStreamReader<T>, public IBlockReader<T>
	{
	public:
		Reader(const T* value, const bool* changed)
			: m_Value(value)
			, m_Changed(changed)
		{
		}

		IStreamReader<T>::Point Next() override final
		{
			return { *m_Value, *m_Changed };
		}

		IBlockReader<T>::Point Get(count_t f) const override final
		{
			return { *m_Value, *m_Changed };
		}

	private:
		const T* m_Value = nullptr;
		const bool* m_Changed = nullptr;
	};
};

template<typename T, typename F>
class BinderBlockParameter : public BlockParameter<T>
{
public:
	BinderBlockParameter(F&& binder)
		: m_Binder(std::move(binder))
		, m_Value(0.0f)
		, m_Changed(false)
	{
	}

	void Prepare(count_t numFrames, f32_t sampleRate) override final
	{
	}
	
	void Update() override final
	{
		T value = m_Binder();
		m_Changed = m_Value != value;
		m_Value = value;
	}

	Ref_t<IStreamReader<T>> Stream() const override final
	{
		return MakeRef<typename BlockParameter<T>::Reader>(&m_Value, &m_Changed);
	}

	Ref_t<IBlockReader<T >> Block() const override final
	{
		return MakeRef<typename BlockParameter<T>::Reader>(&m_Value, &m_Changed);
	}

private:
	F m_Binder;
	T m_Value;
	bool m_Changed;
};

template<typename T>
class ParameterSlot
{
public:
	class Reader : public IStreamReader<T>
	{
		friend class ParameterSlot<T>;
		
	public:
		Reader(ParameterSlot<T>* slot)
			: m_Default(slot->m_Default)
			, m_Reslotted(false)
			, m_Stream(nullptr)
		{
		}

		IStreamReader<T>::Point Next() override final
		{
			T value = m_Default;
			bool changed = false;

			if (m_Stream)
			{
				auto next = m_Stream->Next();
				value = next.Value();
				changed = next.Changed();
			}

			if (m_Reslotted)
			{
				changed = true;
				m_Reslotted = false;
			}

			return { value, changed };
		}

	private:
		T m_Default;
		bool m_Reslotted;
		Ref_t<IStreamReader<T>> m_Stream;
	};

public:
	ParameterSlot(T value, T min = f32::k_Min, T max = f32::k_Max)
		: m_Default(value)
		, m_Used(nullptr)
	{
	}

	void Use(Ref_t<Parameter<T>> parameter)
	{
		if (m_Used != parameter)
		{
			m_Used = parameter;
			for (auto& reader : m_Readers)
			{
				reader->m_Reslotted = true;
				reader->m_Stream = m_Used->Stream();
			}
		}
	}

	Ref_t<IStreamReader<T>> Get()
	{
		auto reader = MakeRef<Reader>(this);

		if (m_Used)
		{
			reader->m_Reslotted = true;
			reader->m_Stream = m_Used->Stream();
		}

		m_Readers.push_back(reader);

		return reader;
	}

private:
	T m_Default;
	Ref_t<Parameter<T>> m_Used;
	std::vector<Ref_t<Reader>> m_Readers;
};

}
