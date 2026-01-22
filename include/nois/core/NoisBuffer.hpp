#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/math/NoisMatrix.hpp"
#include "nois/parameter/NoisParameter.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {

template<typename>
class Buffer;
template<typename>
class BufferView;
template<typename T>
using ConstBufferView = BufferView<const T>;

using FloatBuffer = Buffer<f32_t>;
using FloatBufferView = BufferView<f32_t>;
using ConstFloatBufferView = ConstBufferView<f32_t>;

template<typename T>
class Buffer
{
public:
	Buffer()
		: m_Size(0)
		, m_NumFrames(0)
		, m_NumChannels(0)
		, m_Data(0, T{ 0 })
	{
	}

	Buffer(count_t numFrames, count_t numChannels)
		: m_Size(numFrames * numChannels)
		, m_NumFrames(numFrames)
		, m_NumChannels(numChannels)
		, m_Data(numFrames * numChannels, T{ 0 })
	{
	}

	Buffer(const T* data, count_t numFrames, count_t numChannels)
		: m_Size(numFrames * numChannels)
		, m_NumFrames(numFrames)
		, m_NumChannels(numChannels)
		, m_Data(numFrames * numChannels, T{ 0 })
	{
		Copy(data, numFrames * numChannels);
	}

	bool IsEmpty() const
	{
		return m_Size == 0;
	}

	count_t GetSize() const
	{
		return m_Size;
	}

	count_t GetBytes() const
	{
		return m_Size * sizeof(T);
	}

	count_t GetNumFrames() const
	{
		return m_NumFrames;
	}

	count_t GetNumChannels() const
	{
		return m_NumChannels;
	}

	void Zero()
	{
		Fill(T{ 0 });
	}

	void Fill(T sample)
	{
		if (count_t fillSize = m_Size)
		{
			std::fill_n(m_Data.data(), fillSize, sample);
		}
	}

	void Copy(const T* data, count_t size)
	{
		if (count_t copySize = std::min(m_Size, size))
		{
			std::copy_n(data, copySize, m_Data.data());
		}
	}

	void Copy(const Buffer<T>& buffer)
	{
		if (count_t copySize = std::min(m_Size, buffer.GetSize()))
		{
			std::copy_n(static_cast<const T*>(buffer), copySize, m_Data.data());
		}
	}

	void Copy(const BufferView<T>& buffer)
	{
		if (count_t copySize = std::min(m_Size, buffer.GetSize()))
		{
			std::copy_n(static_cast<const T*>(buffer), copySize, m_Data.data());
		}
	}

	template<typename U = T>
	auto Copy(const ConstBufferView<U>& buffer) -> std::enable_if_t<!std::is_const_v<U>>
	{
		if (count_t copySize = std::min(m_Size, buffer.GetSize()))
		{
			std::copy_n(static_cast<const T*>(buffer), copySize, m_Data.data());
		}
	}

	void Reserve(count_t numFrames, count_t numChannels)
	{
		count_t size = numFrames * numChannels;

		m_Data.reserve(size);
	}

	void Resize(count_t numFrames, count_t numChannels)
	{
		count_t size = numFrames * numChannels;

		if (m_Size == size)
		{
			return;
		}

		SmallVector newData(size, T{ 0 });

		if (m_Size > 0 && size > 0)
		{
			std::copy_n(m_Data.data(), std::min(m_Size, size), newData.data());
		}

		m_Size = size;
		m_NumFrames = numFrames;
		m_NumChannels = numChannels;
		m_Data = std::move(newData);
	}

	void Extend(count_t numCopies, count_t channelCount = 0)
	{
		if (numCopies <= 1)
		{
			return;
		}

		count_t oldNumChannels = channelCount == 0 ? m_NumChannels : std::min(m_NumChannels, channelCount);
		count_t newNumChannels = numCopies * oldNumChannels;
		count_t numChannelsStride = oldNumChannels;

		Resize(m_NumFrames, newNumChannels);

		for (count_t c = oldNumChannels; c < newNumChannels; c += numChannelsStride)
		{
			for (count_t s = 0; s < numChannelsStride; ++s)
			{
				const T* srcData = &m_Data[s * m_NumFrames];
				T* dstData = &m_Data[(c + s) * m_NumFrames];
				std::copy_n(srcData, m_NumFrames, dstData);
			}
		}
	}

	Buffer<T> Take(count_t c, count_t numChannels = 1) const
	{
		return Buffer<T>(
			&m_Data[c * m_NumFrames],
			m_NumFrames,
			std::min(m_NumChannels - c, numChannels)
		);
	}

	BufferView<T> View(count_t c, count_t numChannels = 1)
	{
		return BufferView<T>(
			&m_Data[c * m_NumFrames],
			m_NumFrames,
			std::min(m_NumChannels - c, numChannels)
		);
	}

	ConstBufferView<T> View(count_t c, count_t numChannels = 1) const
	{
		return ConstBufferView<T>(
			&m_Data[c * m_NumFrames],
			m_NumFrames,
			std::min(m_NumChannels - c, numChannels)
		);
	}

	void Add(const Buffer<T>& buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] += buffer[i];
		}
	}

	void Add(const BufferView<T>& buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] += buffer[i];
		}
	}

	void AddLinearily(const BufferView<T>& buffer1, const BufferView<T>& buffer2, T factor)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min({ m_Size, buffer1.GetSize(), buffer2.GetSize() }); ++i)
		{
			m_Data[i] += buffer1[i] * factor + buffer2[i] * (T{ 1 } - factor);
		}
	}

	void Subtract(const Buffer<T>& buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] -= buffer[i];
		}
	}

	void Multiply(T value)
	{
		// TODO: vectorize

		for (count_t i = 0; i < m_Size; ++i)
		{
			m_Data[i] *= value;
		}
	}

	void Multiply(const Parameter<T>& parameter)
	{
		// TODO: vectorize
		// TODO: how can we validate size of parameters?

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			T value = parameter.Get(f);
			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				(*this)(f, c) *= value;
			}
		}
	}

	void Multiply(const math::Mat<T>& mat)
	{
		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			count_t m = std::min(m_NumChannels, mat.GetM());
			math::Mat<T> samples(m, 1);
			for (count_t i = 0; i < m; ++i)
			{
				samples(i, 0) = (*this)(f, i);
			}
			math::Mat<T> newSamples = mat.Multiply(samples);
			for (count_t i = 0; i < m; ++i)
			{
				(*this)(f, i) = newSamples(i, 0);
			}
		}
	}

	T* Data()
	{
		return m_Data.data();
	}

	const T* Data() const
	{
		return m_Data.data();
	}

	operator BufferView<T>()
	{
		return View(0, m_NumChannels);
	}

	operator ConstBufferView<T>() const
	{
		return View(0, m_NumChannels);
	}

	T& operator()(count_t f, count_t c)
	{
		return m_Data[f + c * m_NumFrames];
	}

	const T& operator()(count_t f, count_t c) const
	{
		return m_Data[f + c * m_NumFrames];
	}

	operator T*()
	{
		return m_Data.data();
	}

	operator const T*() const
	{
		return m_Data.data();
	}

	T& operator[](count_t i)
	{
		return m_Data[i];
	}

	const T& operator[](count_t i) const
	{
		return m_Data[i];
	}

private:
	count_t m_Size;
	count_t m_NumFrames;
	count_t m_NumChannels;
	SmallVector<T> m_Data;
};

template<typename T>
class BufferView
{
public:
	BufferView(T* data, count_t numFrames, count_t numChannels)
		: m_Size(numFrames * numChannels)
		, m_NumFrames(numFrames)
		, m_NumChannels(numChannels)
		, m_Data(data)
	{
	}

	bool IsEmpty() const
	{
		return m_Size == 0;
	}

	count_t GetSize() const
	{
		return m_Size;
	}

	count_t GetBytes() const
	{
		return m_Size * sizeof(T);
	}

	count_t GetNumFrames() const
	{
		return m_NumFrames;
	}

	count_t GetNumChannels() const
	{
		return m_NumChannels;
	}

	void Zero()
	{
		Fill(T{ 0 });
	}

	void Fill(T sample)
	{
		std::fill_n(m_Data, m_Size, sample);
	}

	void Copy(const T* data, count_t size)
	{
		if (count_t copySize = std::min(m_Size, size))
		{
			std::copy_n(data, copySize, m_Data);
		}
	}

	void Copy(const Buffer<T>& buffer)
	{
		if (count_t copySize = std::min(m_Size, buffer.GetSize()))
		{
			std::copy_n(static_cast<const T*>(buffer), copySize, m_Data);
		}
	}

	void Copy(const BufferView<T>& buffer)
	{
		if (count_t copySize = std::min(m_Size, buffer.GetSize()))
		{
			std::copy_n(static_cast<const T*>(buffer), copySize, m_Data);
		}
	}

	template<typename U = T>
	auto Copy(const ConstBufferView<U>& buffer) -> std::enable_if_t<!std::is_const_v<U>>
	{
		if (count_t copySize = std::min(m_Size, buffer.GetSize()))
		{
			std::copy_n(static_cast<const T*>(buffer), copySize, m_Data);
		}
	}

	void Copy(const BufferView<T>& buffer, T factor)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] = buffer[i] * factor;
		}
	}

	template<typename U = T>
	auto Copy(const ConstBufferView<T>& buffer, T factor) -> std::enable_if_t<!std::is_const_v<U>>
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] = buffer[i] * factor;
		}
	}

	void CopyLinearily(const BufferView<T>& buffer, T factor)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] = m_Data[i] * factor + buffer[i] * (T{ 1 } - factor);
		}
	}

	template<typename U = T>
	auto CopyLinearily(const ConstBufferView<T>& buffer, T factor) -> std::enable_if_t<!std::is_const_v<U>>
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] = m_Data[i] * factor + buffer[i] * (T{ 1 } - factor);
		}
	}

	Buffer<T> Take(count_t c, count_t numChannels = 1) const
	{
		return Buffer<T>(
			&m_Data[c * m_NumFrames],
			m_NumFrames,
			std::min(m_NumChannels - c, numChannels)
		);
	}

	BufferView<T> View(count_t c, count_t numChannels = 1)
	{
		return BufferView<T>(
			&m_Data[c * m_NumFrames],
			m_NumFrames,
			std::min(m_NumChannels - c, numChannels)
		);
	}

	BufferView<const T> View(count_t c, count_t numChannels = 1) const
	{
		return BufferView<const T>(
			&m_Data[c * m_NumFrames],
			m_NumFrames,
			std::min(m_NumChannels - c, numChannels)
		);
	}

	void Add(const Buffer<T>& buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] += buffer[i];
		}
	}

	void Add(const BufferView<T>& buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] += buffer[i];
		}
	}

	void Add(const BufferView<T>& buffer, T factor)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] += buffer[i] * factor;
		}
	}

	void AddLinearily(const BufferView<T>& buffer1, const BufferView<T>& buffer2, T factor)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min({ m_Size, buffer1.GetSize(), buffer2.GetSize() }); ++i)
		{
			m_Data[i] += buffer1[i] * factor + buffer2[i] * (T{ 1 } - factor);
		}
	}

	void Subtract(const BufferView<T>& buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.GetSize()); ++i)
		{
			m_Data[i] -= buffer[i];
		}
	}

	void Multiply(T value)
	{
		// TODO: vectorize

		for (count_t i = 0; i < m_Size; ++i)
		{
			m_Data[i] *= value;
		}
	}

	void Multiply(const Parameter<T>& parameter)
	{
		// TODO: vectorize
		// TODO: how can we validate size of parameters?

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			T value = parameter.Get(f);
			for (count_t c = 0; c < m_NumChannels; ++c)
			{
				(*this)(f, c) *= value;
			}
		}
	}

	void Multiply(const math::Mat<T>& mat)
	{
		// TODO: crap, the fast matrix path requires an interleaved layout

		// for (count_t f = 0; f < m_NumFrames; ++f)
		// {
		// 	count_t m = std::min(m_NumChannels, mat.GetM());
		// 	math::MatView<T> samples = math::MatView<T>(m, 1, &(*this)(f, 0));
		// 	math::Mat<T> samplesCopy = samples;
		// 	mat.Multiply(samplesCopy, samples);
		// }

		for (count_t f = 0; f < m_NumFrames; ++f)
		{
			count_t m = std::min(m_NumChannels, mat.GetM());
			math::Mat<T> samples(m, 1);
			for (count_t i = 0; i < m; ++i)
			{
				samples(i, 0) = (*this)(f, i);
			}
			math::Mat<T> newSamples = mat.Multiply(samples);
			for (count_t i = 0; i < m; ++i)
			{
				(*this)(f, i) = newSamples(i, 0);
			}
		}
	}

	T* Data()
	{
		return m_Data;
	}

	const T* Data() const
	{
		return m_Data;
	}

	T& operator()(count_t f, count_t c)
	{
		return m_Data[f + c * m_NumFrames];
	}

	const T& operator()(count_t f, count_t c) const
	{
		return m_Data[f + c * m_NumFrames];
	}

	operator T*()
	{
		return m_Data;
	}

	operator const T*() const
	{
		return m_Data;
	}

	T& operator[](count_t i)
	{
		return m_Data[i];
	}

	const T& operator[](count_t i) const
	{
		return m_Data[i];
	}

private:
	count_t m_Size;
	count_t m_NumFrames;
	count_t m_NumChannels;
	T* m_Data;
};

}