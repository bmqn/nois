#pragma once

#include "nois/NoisTypes.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace nois {

template<typename T, std::size_t N>
class SmallVector
{
public:
	using size_type = std::size_t;

	SmallVector(size_type n = 0, const T &value = T{})
		: m_Size(n)
	{
		if (n >= N)
		{
			MoveToFallback();
		}
		std::fill(begin(), end(), value);
	}

	SmallVector(const SmallVector &other) = default;
	SmallVector(SmallVector &&other) = default;
	SmallVector &operator=(const SmallVector &other) = default;
	SmallVector &operator=(SmallVector &&other) = default;

	inline size_type size() const
	{
		return m_Size;
	}

	inline void resize(size_type n, const T &value = T{})
	{
		if (n <= N)
		{
			if (n > m_Size)
			{
				std::fill_n(m_Data.begin() + m_Size, n - m_Size, value);
			}
			m_Size = n;
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback(n, value);
			}
			m_Size = n;
		}
	}

	inline void push_back(T value)
	{
		if (m_Size <= N)
		{
			m_Data[m_Size] = value;
			++m_Size;
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}
			m_Fallback.push_back(value);
			++m_Size;
		}
	}

	template<typename ...Args>
	inline void emplace_back(Args &&...args)
	{
		if (m_Size <= N)
		{
			new (&m_Data[m_Size]) T(std::forward<Args>(args)...);
			++m_Size;
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}
			m_Fallback.emplace_back(std::forward<Args>(args)...);
			++m_Size;
		}
	}

	inline void pop_back()
	{
		if (m_Size <= N)
		{
			--m_Size;
		}
		else
		{
			m_Fallback.pop_back();
			--m_Size;
		}
	}

	inline T back() const
	{
		return (*this)[m_Size - 1];
	}

	inline T &back()
	{
		return (*this)[m_Size - 1];
	}

	inline T &operator[](size_type index)
	{
		if (m_Size <= N)
		{
			return m_Data[index];
		}
		else
		{
			return m_Fallback[index];
		}
	}

	inline const T &operator[](size_type index) const
	{
		if (m_Size <= N)
		{
			return m_Data[index];
		}
		else
		{
			return m_Fallback[index];
		}
	}

	inline T &data()
	{
		if (m_Size <= N)
		{
			return m_Data.data();
		}
		else
		{
			return m_Fallback.data();
		}
	}

	inline const T &data() const
	{
		if (m_Size <= N)
		{
			return m_Data.data();
		}
		else
		{
			return m_Fallback.data();
		}
	}

	template<bool Const>
	class Iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = typename std::conditional<Const, const T*, T*>::type;
		using reference = typename std::conditional<Const, const T&, T&>::type;

		Iterator(SmallVector *smallVector, size_type index)
			: m_SmallVector(smallVector)
			, m_Index(index)
		{
		}

		inline reference operator*() const
		{
			return (*m_SmallVector)[m_Index];
		}

		inline pointer operator->() const
		{
			return &(*m_SmallVector)[m_Index];
		}

		inline Iterator &operator++()
		{
			++m_Index;
			return *this;
		}

		inline Iterator operator++(int)
		{
			Iterator temp = *this;
			++(*this);
			return temp;
		}

		inline Iterator &operator--()
		{
			--m_Index;
			return *this;
		}

		inline Iterator operator--(int)
		{
			Iterator temp = *this;
			--(*this);
			return temp;
		}

		inline Iterator operator+(difference_type n) const
		{
			return Iterator(m_SmallVector, m_Index + n);
		}

		inline Iterator operator-(difference_type n) const
		{
			return Iterator(m_SmallVector, m_Index - n);
		}

		inline difference_type operator-(const Iterator &other) const
		{
			return m_Index - other.m_Index;
		}

		inline bool operator==(const Iterator &other) const
		{
			return m_Index == other.m_Index;
		}

		inline bool operator!=(const Iterator &other) const
		{
			return m_Index != other.m_Index;
		}

		inline bool operator<(const Iterator &other) const
		{
			return m_Index < other.m_Index;
		}

		inline bool operator>(const Iterator &other) const
		{
			return m_Index > other.m_Index;
		}

		inline bool operator<=(const Iterator &other) const
		{
			return m_Index <= other.m_Index;
		}

		inline bool operator>=(const Iterator &other) const
		{
			return m_Index >= other.m_Index;
		}

	private:
		SmallVector* m_SmallVector;
		size_type m_Index;
	};

	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;

	inline iterator begin()
	{
		return iterator(this, 0);
	}

	inline const_iterator begin() const
	{
		return const_iterator(this, 0);
	}

	inline iterator end()
	{
		return iterator(this, m_Size);
	}

	inline const_iterator end() const
	{
		return const_iterator(this, m_Size);
	}

private:
	inline void MoveToFallback(size_type n = N, const T &value = T{})
	{
		m_Fallback.resize(n, value);
		std::copy_n(m_Data.begin(), m_Size, m_Fallback.begin());
	}

private:
	size_type m_Size;
	std::array<T, N> m_Data;
	std::vector<T> m_Fallback;
};


}