#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/parameter/NoisParameter.hpp"

namespace nois {

template<typename>
class Buffer;

using FloatBuffer = Buffer<f32_t>;

template<typename T>
class Buffer
{
public:
	Buffer()
		: m_NumFrames(0)
		, m_NumChannels(0)
		, m_Size(0)
		, m_Data(0, T{ 0 })
	{
	}

	Buffer(count_t numFrames, count_t numChannels)
		: m_NumFrames(numFrames)
		, m_NumChannels(numChannels)
		, m_Size(numFrames * numChannels)
		, m_Data(numFrames * numChannels, T{ 0 })
	{
	}

	inline bool IsEmpty() const
	{
		return m_Size == 0;
	}

	inline count_t GetSize() const
	{
		return m_Size;
	}

	inline count_t GetBytes() const
	{
		return m_Size * sizeof(T);
	}

	inline void Zero()
	{
		Fill(T{ 0 });
	}

	inline void Fill(T sample)
	{
		std::fill(m_Data.begin(), m_Data.begin() + m_Size, sample);
	}

	inline void Resize(count_t numFrames, count_t numChannels)
	{
		count_t size = numFrames * numChannels;

		if (m_Size == size)
		{
			return;
		}

		std::vector<T> newData(size, T{ 0 });

		if (m_Size > 0)
		{
			std::copy_n(m_Data.begin(), std::min(m_Size, size), newData.begin());
		}

		m_NumFrames = numFrames;
		m_NumChannels = numChannels;

		m_Size = size;

		m_Data = std::move(newData);
	}

	inline void Copy(const T *data, count_t size)
	{
		std::copy_n(data, std::min(m_Size, size), m_Data.begin());
	}

	inline void Copy(const Buffer<T> &buffer)
	{
		std::copy_n(buffer.m_Data.begin(), std::min(m_Size, buffer.m_Size), m_Data.begin());
	}

	inline void Add(const Buffer<T> &buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.m_Size); ++i)
		{
			m_Data[i] += buffer.m_Data[i];
		}
	}

	inline void Subtract(const Buffer<T> &buffer)
	{
		// TODO: vectorize

		for (count_t i = 0; i < std::min(m_Size, buffer.m_Size); ++i)
		{
			m_Data[i] -= buffer.m_Data[i];
		}
	}

	inline void Multiply(T value)
	{
		// TODO: vectorize

		for (count_t i = 0; i < m_Size; ++i)
		{
			m_Data[i] *= value;
		}
	}

	inline void Multiply(Parameter<T> &parameter)
	{
		// TODO: vectorize
		// TODO: how can we validate size of parameters?

		for (count_t i = 0; i < m_NumFrames; ++i)
		{
			T value = parameter.Get(i);

			for (count_t j = 0; j < m_NumChannels; ++j)
			{
				GetSample(i, j) *= value;
			}
		}
	}

	inline T &GetSample(count_t frameIndex, count_t channelIndex)
	{
		return m_Data[frameIndex * m_NumChannels + channelIndex];
	}

	inline const T &GetSample(count_t frameIndex, count_t channelIndex) const
	{
		return m_Data[frameIndex * m_NumChannels + channelIndex];
	}

	inline StereoSample<T> GetMonoSample(count_t frameIndex) const
	{
		const T *data = &m_Data[frameIndex * m_NumChannels];

		MonoSample<T> monoSample;
		monoSample.s = data[0];

		return monoSample;
	}

	inline void SetStereoSample(count_t frameIndex, MonoSample<T> monoSample)
	{
		T *data = &m_Data[frameIndex * m_NumChannels];

		data[0] = monoSample.s;
	}

	inline StereoSample<T> GetStereoSample(count_t frameIndex) const
	{
		const T *data = &m_Data[frameIndex * m_NumChannels];

		StereoSample<T> stereoSample;
		stereoSample.s1 = data[0];
		stereoSample.s2 = data[1];

		return stereoSample;
	}

	inline void SetStereoSample(count_t frameIndex, StereoSample<T> stereoSample)
	{
		T *data = &m_Data[frameIndex * m_NumChannels];

		data[0] = stereoSample.s1;
		data[1] = stereoSample.s2;
	}

	inline T &operator()(count_t frameIndex, count_t channelIndex)
	{
		return GetSample(frameIndex, channelIndex);
	}

	inline const T &operator()(count_t frameIndex, count_t channelIndex) const
	{
		return GetSample(frameIndex, channelIndex);
	}

	inline count_t GetNumFrames() const
	{
		return m_NumFrames;
	}

	inline count_t GetNumChannels() const
	{
		return m_NumChannels;
	}

	inline T *GetData()
	{
		return m_Data.data();
	}

	inline const T *GetData() const
	{
		return m_Data.data();
	}

private:
	count_t m_NumFrames;
	count_t m_NumChannels;

	count_t m_Size;

	std::vector<T> m_Data;
};

}