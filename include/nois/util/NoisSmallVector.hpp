#pragma once

#include "nois/NoisConfig.hpp"
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
		: m_Size(0)
		, m_Fallback()
		, m_Data{T{}}
	{
		resize(n, value);
	}

	SmallVector(const SmallVector& other)
		: m_Size(other.m_Size)
		, m_Fallback(other.m_Fallback)
		, m_Data(other.m_Data)
	{
	}

	SmallVector(SmallVector &&other) noexcept
		: m_Size(other.m_Size)
		, m_Fallback(std::move(other.m_Fallback))
		, m_Data(std::move(other.m_Data))
	{
		other.m_Size = 0;
		other.m_Data = {};
		other.m_Fallback = {};
	}

	SmallVector& operator=(const SmallVector& other)
	{
		m_Size = other.m_Size;
		m_Data = other.m_Data;
		m_Fallback = other.m_Fallback;
	}

	SmallVector &operator=(SmallVector &&other) noexcept
	{
		m_Size = other.m_Size;
		m_Data = std::move(other.m_Data);
		m_Fallback = std::move(other.m_Fallback);
		other.m_Size = 0;
		other.m_Data = {};
		other.m_Fallback = {};
		return *this;
	}

	inline bool empty() const
	{
		return m_Size == 0;
	}

	inline bool full() const
	{
		return m_Size == N;
	}

	inline size_type size() const
	{
		return m_Size;
	}

	inline void shrink_to_fit()
	{
		if (m_Size <= N)
		{
			return;
		}
		else
		{
			ShrinkToFitFallback();
		}
	}

	inline void reserve(size_type n)
	{
		if (n <= N)
		{
			return;
		}
		else
		{
			ReserveFallback(n);
		}
	}

	inline void resize(size_type n, const T &value = T{})
	{
		if (m_Size == n)
		{
			return;
		}

		if (n <= N)
		{
			if (!m_Fallback.empty())
			{
				MoveFromFallback();
			}
			if (n > m_Size)
			{
				std::fill_n(m_Data.begin() + m_Size, n - m_Size, value);
			}
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback(n, value);
			}
			m_Fallback.resize(n);
		}
		m_Size = n;
	}

	inline void push_back(T value)
	{
		if (m_Size < N)
		{
			m_Data[m_Size] = value;
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}
			m_Fallback.push_back(value);
		}
		++m_Size;
	}

	template<typename ...Args>
	inline void emplace_back(Args &&...args)
	{
		if (m_Size < N)
		{
			new (&m_Data[m_Size]) T(std::forward<Args>(args)...);
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}
			m_Fallback.emplace_back(std::forward<Args>(args)...);
		}
		++m_Size;
	}

	inline void pop_back()
	{
		if (m_Size > N)
		{
			m_Fallback.pop_back();
		}
		--m_Size;
	}

	inline const T& front() const
	{
		return (*this)[0];
	}

	inline T& front()
	{
		return (*this)[0];
	}

	inline const T& back() const
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

	inline T *data()
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

	inline const T *data() const
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
	private:
		using vector = std::conditional_t<Const, const SmallVector*, SmallVector*>;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = typename std::conditional_t<Const, const T*, T*>;
		using reference = typename std::conditional_t<Const, const T&, T&>;

		Iterator(vector smallVector, size_type index)
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
		vector m_SmallVector;
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
	inline void ShrinkToFitFallback()
	{
		m_Fallback.shrink_to_fit();
	}

	inline void ReserveFallback(size_type n = N)
	{
		m_Fallback.reserve(n);
	}

	inline void MoveToFallback(size_type n = N, const T &value = T{})
	{
		m_Fallback.resize(n, value);
		std::copy_n(m_Data.begin(), N, m_Fallback.begin());
	}

	inline void MoveFromFallback()
	{
		std::copy_n(m_Fallback.begin(), N, m_Data.begin());
		m_Fallback.clear();
	}

private:
	size_type m_Size;
	std::vector<T> m_Fallback;
	alignas(kCacheLineSize) std::array<T, N> m_Data;
};


}