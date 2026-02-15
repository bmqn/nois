#pragma once

#include "nois/NoisConfig.hpp"
#include "nois/NoisTypes.hpp"

#include <algorithm>
#include <array>
#include <new>
#include <vector>

namespace nois {

template<typename T, std::size_t N>
class SmallVector
{
	static_assert(std::is_nothrow_move_constructible_v<T>);

public:
	using size_type = std::size_t;

	SmallVector(size_type n = 0, const T &value = T{})
		: m_Size(0)
	{
		resize(n, value);
	}

	SmallVector(const SmallVector& other)
		: m_Size(other.m_Size)
	{
		if (other.m_Size <= N)
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (DataPtr(i)) T(*other.DataPtr(i));
			}
		}
		else
		{
			m_Fallback = other.m_Fallback;
		}
	}

	SmallVector(SmallVector &&other) noexcept
		: m_Size(other.m_Size)
	{
		if (other.m_Size <= N)
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (DataPtr(i)) T(std::move(*other.DataPtr(i)));
			}
		}
		else
		{
			m_Fallback = std::move(other.m_Fallback);
		}
		other.m_Size = 0;
	}

	~SmallVector()
	{
		clear();
	}

	SmallVector& operator=(const SmallVector& other)
	{
		clear();
		m_Size = other.m_Size;
		if (other.m_Size <= N)
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (DataPtr(i)) T(*other.DataPtr(i));
			}
		}
		else
		{
			m_Fallback = other.m_Fallback;
		}
		return *this;
	}

	SmallVector &operator=(SmallVector &&other) noexcept
	{
		clear();
		m_Size = other.m_Size;
		if (other.m_Size <= N)
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (DataPtr(i)) T(std::move(*other.DataPtr(i)));
			}
		}
		else
		{
			m_Fallback = std::move(other.m_Fallback);
		}
		other.m_Size = 0;
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
		if (n > N && m_Fallback.empty())
		{
			MoveToFallback();
		}
		ReserveFallback(n);
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
				// Move first n elements
				for (size_type i = 0; i < std::min(n, m_Size); ++i)
				{
					new (DataPtr(i)) T(std::move(m_Fallback[i]));
				}

				// Fallback is now empty
				m_Fallback.clear();
			}

			// Expanding inline
			if (n > m_Size)
			{
				for (size_type i = m_Size; i < n; ++i)
				{
					new (DataPtr(i)) T(value);
				}
			}
			// Shrinking inline
			else
			{
				for (size_type i = n; i < m_Size; ++i)
				{
					DataPtr(i)->~T();
				}
			}
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}
			m_Fallback.resize(n, value);
		}
		m_Size = n;
	}

	inline void clear()
	{
		if (m_Size <= N)
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				DataPtr(i)->~T();
			}
		}
		else
		{
			m_Fallback.clear();
		}
		m_Size = 0;
	}

	inline void push_back(const T& value)
	{
		if (m_Size < N)
		{
			new (DataPtr(m_Size)) T(value);
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

	inline void push_back(T&& value)
	{
		if (m_Size < N)
		{
			new (DataPtr(m_Size)) T(std::move(value));
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}
			m_Fallback.push_back(std::forward<T>(value));
		}
		++m_Size;
	}

	template<typename ...Args>
	inline void emplace_back(Args &&...args)
	{
		if (m_Size < N)
		{
			new (DataPtr(m_Size)) T(std::forward<Args>(args)...);
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
		else
		{
			DataPtr(m_Size - 1)->~T();
		}
		--m_Size;
		if (m_Size == N)
		{
			MoveFromFallback();
		}
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
			return *DataPtr(index);
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
			return *DataPtr(index);
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
			return DataPtr(0);
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
			return DataPtr(0);
		}
		else
		{
			return m_Fallback.data();
		}
	}

	template<bool Const>
	class Iterator
	{
		friend class SmallVector;

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

		inline Iterator& operator+=(difference_type n)
		{
			m_Index += n;
			return *this;
		}

		inline Iterator& operator-=(difference_type n)
		{
			m_Index -= n;
			return *this;
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

	inline iterator insert(iterator it, const T& value)
	{
		size_type index = it.m_Index;

		if (index > m_Size)
		{
			index = m_Size;
		}

		if (m_Size == 0)
		{
			push_back(value);
			return iterator(this, 0);
		}

		if (index == m_Size)
		{
			push_back(value);
			return iterator(this, index);
		}

		if (m_Size < N)
		{
			// Construct the new tail
			new (DataPtr(m_Size)) T(std::move(*DataPtr(m_Size - 1)));

			// Move-assign backward
			for (size_type i = m_Size - 1; i > index; --i)
			{
				*DataPtr(i) = std::move(*DataPtr(i - 1));
			}

			// Assign or construct index
			*DataPtr(index) = value;
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}

			m_Fallback.insert(m_Fallback.begin() + index, value);
		}
		++m_Size;
		return iterator(this, index);
	}

	inline iterator insert(iterator it, T&& value)
	{
		size_type index = it.m_Index;

		if (m_Size == 0)
		{
			push_back(std::move(value));
			return iterator(this, 0);
		}

		if (index == m_Size)
		{
			push_back(std::move(value));
			return iterator(this, index);
		}

		if (m_Size < N)
		{
			// Construct the new tail
			new (DataPtr(m_Size)) T(std::move(*DataPtr(m_Size - 1)));

			// Move-assign backward
			for (size_type i = m_Size - 1; i > index; --i)
			{
				*DataPtr(i) = std::move(*DataPtr(i - 1));
			}

			// Assign or construct index
			*DataPtr(index) = std::move(value);

			++m_Size;
		}
		else
		{
			if (m_Fallback.empty())
			{
				MoveToFallback();
			}

			m_Fallback.insert(m_Fallback.begin() + index, std::forward<T>(value));
		}
		++m_Size;
		return iterator(this, index);
	}

	inline iterator erase(iterator it)
	{
		size_type index = it.m_Index;

		if (index >= m_Size)
		{
			return end();
		}

		if (m_Size <= N)
		{
			// Shift elements left
			for (size_type i = index; i + 1 < m_Size; ++i)
			{
				*DataPtr(i) = std::move(*DataPtr(i + 1));
			}

			// Destroy old tail
			DataPtr(m_Size - 1)->~T();
		}
		else
		{
			m_Fallback.erase(m_Fallback.begin() + index);
		}
		--m_Size;
		if (m_Size == N)
		{
			MoveFromFallback();
		}
		return iterator(this, index);
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

	inline void MoveToFallback()
	{
		m_Fallback.reserve(m_Size);
		for (size_type i = 0; i < m_Size; ++i)
		{
			m_Fallback.emplace_back(std::move(*DataPtr(i)));
			DataPtr(i)->~T();
		}
	}

	inline void MoveFromFallback()
	{
		for (size_type i = 0; i < m_Size; ++i)
		{
			new (DataPtr(i)) T(std::move(m_Fallback[i]));
		}
		m_Fallback.clear();
	}

	inline T* DataPtr(size_type i = 0)
	{
		return std::launder(reinterpret_cast<T*>(&m_Data[i]));
	}

	inline const T* DataPtr(size_type i = 0) const
	{
		return std::launder(reinterpret_cast<const T*>(&m_Data[i]));
	}

private:
	size_type m_Size;
	std::vector<T> m_Fallback;
	alignas(kCacheLineSize) std::array<std::aligned_storage_t<sizeof(T), alignof(T)>, N> m_Data;
};

}

