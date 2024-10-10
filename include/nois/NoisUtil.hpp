#pragma once

#include "NoisTypes.hpp"
#include "NoisConfig.hpp"

#include <algorithm>
#include <vector>

namespace nois {

inline data_t ToDb(data_t amp)
{
	return 10.0f * std::log10(amp - k_DcOffset);
}

inline data_t FromDb(data_t db)
{
	return std::pow(10.0f, (db) / 10.0f) + k_DcOffset;
}

template<typename T>
struct WindowStream
{
	WindowStream(count_t length)
		: m_Data(length, T{ 0 })
		, m_Offset(0)
		, m_Count(0)
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
		count_t offset = (m_Offset - n) % m_Data.size();
		return m_Data[offset];
	}

	inline void Wipe()
	{
		std::fill(m_Data.begin(), m_Data.end(), T{ 0 });
		m_Offset = 0;
		m_Count = 0;
	}

	inline void Resize(count_t length)
	{
		m_Data.resize(length, T{ 0 });
		m_Offset = m_Offset % length;
		m_Count = std::min(m_Count, length);
	}

	inline const T *GetData() const { return m_Data.data(); }
	inline count_t GetSize() const { return m_Data.size(); }
	inline count_t GetCount() const { return m_Count; }
	inline count_t GetOffset() const { return m_Offset; }

private:
	std::vector<T> m_Data;
	count_t m_Offset;
	count_t m_Count;
};


};
