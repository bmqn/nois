#pragma once

#include "NoisTypes.hpp"
#include "NoisConfig.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace nois {

inline f32_t ToDb(f32_t x)
{
	return 10.0f * std::log10(x - k_DcOffset);
}

inline f32_t FromDb(f32_t db)
{
	return std::pow(10.0f, (db) / 10.0f) + k_DcOffset;
}

template<typename T>
struct WindowStream
{
	WindowStream(count_t length)
		: m_Offset(0)
		, m_Count(0)
		, m_Data(length, T{})
	{
	}

	inline void Add(T data)
	{
		m_Data[m_Offset] = data;
		m_Offset = (m_Offset + 1) % m_Data.size();
		m_Count = m_Count < m_Data.size() ? m_Count + 1 : m_Count;
	}

	inline T Get(count_t n) const
	{
		count_t size = m_Data.size();

		count_t index = (m_Offset + size - n - 1) % size;

		if (index < 0)
		{
			index += size;
		}

		return m_Data[index];
	}

	inline T GetOldest() const
	{
		return Get(m_Count - 1);
	}

	inline T GetNewest() const
	{
		return Get(0);
	}

	inline void Wipe()
	{
		m_Offset = 0;
		m_Count = 0;
		std::fill(m_Data.begin(), m_Data.end(), T{});
	}

	inline void Resize(count_t length)
	{
		if (length == 0)
		{
			m_Offset = 0;
			m_Count = 0;
			m_Data.resize(0);
		}

		if (length == m_Data.size())
		{
			return;
		}

		std::vector<T> newData(length, T{});

		for (count_t i = std::min(m_Count, length) - 1; i >= 0; --i)
		{
			newData[i] = Get(i);
		}

		m_Offset = m_Offset % length;
		m_Count = (std::min)(m_Count, length);
		m_Data = std::move(newData);
	}

	inline const T *GetData() const
	{
		return m_Data.data();
	}
	
	inline count_t GetSize() const
	{
		return m_Data.size();
	}

	inline count_t GetCount() const
	{
		return m_Count;
	}

	inline count_t GetOffset() const
	{
		return m_Offset;
	}

private:
	count_t m_Offset;
	count_t m_Count;
	std::vector<T> m_Data;
};


};
