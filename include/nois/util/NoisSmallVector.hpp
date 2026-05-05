#pragma once

#include "nois/NoisTypes.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <new>

namespace nois {

template<typename T, std::size_t N, typename FallbackAllocator = std::allocator<T>>
class SmallVector
{
	static_assert(std::is_nothrow_move_constructible_v<T>);

public:
	using value_type = T;
	using size_type = std::size_t;
	using fallback_allocator = FallbackAllocator;

	SmallVector(size_type n = 0, const T &value = T{})
		: m_Size(0)
		, m_Data(reinterpret_cast<T*>(m_Inline))
		, m_FallbackCapacity(0)
		, m_Inline{}
	{
		resize(n, value);
	}

	SmallVector(const SmallVector& other)
		: m_Size(0)
		, m_Data(nullptr)
		, m_FallbackCapacity(0)
		, m_Inline{}
	{
		m_Size = other.m_Size;
		if (other.m_FallbackCapacity > 0)
		{
			m_Data = AllocateFallback(other.m_FallbackCapacity);
			m_FallbackCapacity = other.m_FallbackCapacity;
		}
		else
		{
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackCapacity = 0;
		}
		for (size_type i = 0; i < m_Size; ++i)
		{
			new (&m_Data[i]) T(other[i]);
		}
	}

	SmallVector(SmallVector &&other) noexcept
		: m_Size(0)
		, m_Data(nullptr)
		, m_FallbackCapacity(0)
		, m_Inline{}
	{
		m_Size = other.m_Size;
		if (other.m_FallbackCapacity > 0)
		{
			m_Data = other.m_Data;
			m_FallbackCapacity = other.m_FallbackCapacity;
			other.m_FallbackCapacity = 0;
			other.m_Data = reinterpret_cast<T*>(other.m_Inline);
		}
		else
		{
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackCapacity = 0;
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (&m_Data[i]) T(std::move(other[i]));
				other.m_Data[i].~T();
			}
		}
		other.m_Size = 0;
	}

	~SmallVector() noexcept
	{
		clear();
		if (m_FallbackCapacity > 0)
		{
			DeallocateFallback(m_Data);
		}
	}

	SmallVector& operator=(const SmallVector& other)
	{
		if (this == &other)
		{
			return *this;
		}

		clear();
		if (m_FallbackCapacity > 0)
		{
			DeallocateFallback(m_Data);
		}
		m_Size = other.m_Size;
		if (other.m_FallbackCapacity > 0)
		{
			m_Data = AllocateFallback(other.m_FallbackCapacity);
			m_FallbackCapacity = other.m_FallbackCapacity;
		}
		else
		{
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackCapacity = 0;
		}
		for (size_type i = 0; i < m_Size; ++i)
		{
			new (&m_Data[i]) T(other[i]);
		}
		
		return *this;
	}

	SmallVector &operator=(SmallVector &&other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		clear();
		if (m_FallbackCapacity > 0)
		{
			DeallocateFallback(m_Data);
		}
		m_Size = other.m_Size;
		if (other.m_FallbackCapacity > 0)
		{
			m_Data = other.m_Data;
			m_FallbackCapacity = other.m_FallbackCapacity;
			other.m_FallbackCapacity = 0;
			other.m_Data = reinterpret_cast<T*>(other.m_Inline);
		}
		else
		{
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackCapacity = 0;
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (&m_Data[i]) T(std::move(other[i]));
				other.m_Data[i].~T();
			}
		}
		other.m_Size = 0;

		return *this;
	}

	[[nodiscard]] inline bool empty() const
	{
		return m_Size == 0;
	}

	[[nodiscard]] inline size_type size() const
	{
		return m_Size;
	}

	[[nodiscard]] inline size_type capacity() const
	{
		return m_FallbackCapacity > 0 ? m_FallbackCapacity : N;
	}

	inline void reserve(size_type n)
	{
		if (m_FallbackCapacity > 0)
		{
			if (n > m_FallbackCapacity)
			{
				// Reserve space
				GrowFallback(n);
			}
		}
		else if (n > N)
		{
			MoveToFallback();

			// Reserve space
			GrowFallback(n);
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
			if (m_FallbackCapacity > 0)
			{
				MoveFromFallback();
			}
		}
		else
		{
			if (m_FallbackCapacity == 0)
			{
				MoveToFallback();
			}

			if (n > m_FallbackCapacity)
			{
				GrowFallback(n);
			}
		}

		// Expanding inline
		if (m_Size < n)
		{
			for (size_type i = m_Size; i < n; ++i)
			{
				new (&m_Data[i]) T(value);
			}
		}
		// Shrinking inline
		else
		{
			for (size_type i = n; i < m_Size; ++i)
			{
				m_Data[i].~T();
			}
		}

		m_Size = n;
	}

	inline void clear() noexcept
	{
		for (size_type i = 0; i < m_Size; ++i)
		{
			m_Data[i].~T();
		}

		m_Size = 0;
	}

	inline void push_back(const T& value)
	{
		if (m_FallbackCapacity > 0) [[unlikely]]
		{
			if (m_Size == m_FallbackCapacity)
			{
				GrowFallback();
			}
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();
		}
		
		// Construct new value
		new (&m_Data[m_Size]) T(value);

		++m_Size;
	}

	inline void push_back(T&& value)
	{
		if (m_FallbackCapacity > 0) [[unlikely]]
		{
			if (m_Size == m_FallbackCapacity)
			{
				GrowFallback();
			}
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();
		}

		// Move-construct new value
		new (&m_Data[m_Size]) T(std::move(value));

		++m_Size;
	}

	template<typename ...Args>
	inline void emplace_back(Args &&...args)
	{
		if (m_FallbackCapacity > 0) [[unlikely]]
		{
			if (m_Size == m_FallbackCapacity)
			{
				GrowFallback();
			}
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();
		}

		// Construct new value
		new (&m_Data[m_Size]) T(std::forward<Args>(args)...);

		++m_Size;
	}

	inline void pop_back() noexcept
	{
		// Erase tail
		m_Data[m_Size - 1].~T();

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

	inline T *data()
	{
		return m_Data;
	}

	inline const T *data() const
	{
		return m_Data;
	}

	inline T &operator[](size_type index)
	{
		return m_Data[index];
	}

	inline const T &operator[](size_type index) const
	{
		return m_Data[index];
	}

	template<bool Const>
	class Iterator
	{
		friend class SmallVector;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = typename std::conditional_t<Const, const T*, T*>;
		using reference = typename std::conditional_t<Const, const T&, T&>;

		Iterator(pointer data, size_type index)
			: m_Data(data)
			, m_Index(index)
		{
		}

		inline reference operator*() const
		{
			return m_Data[m_Index];
		}

		inline pointer operator->() const
		{
			return &m_Data[m_Index];
		}

		inline Iterator &operator++()
		{
			++m_Index;
			return *this;
		}

		inline Iterator operator++(int)
		{
			Iterator it = *this;
			++(*this);
			return it;
		}

		inline Iterator &operator--()
		{
			--m_Index;
			return *this;
		}

		inline Iterator operator--(int)
		{
			Iterator it = *this;
			--(*this);
			return it;
		}

		inline Iterator operator+(difference_type n) const
		{
			return Iterator(m_Data, m_Index + n);
		}

		inline Iterator operator-(difference_type n) const
		{
			return Iterator(m_Data, m_Index - n);
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
		pointer m_Data;
		size_type m_Index;
	};

	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;

	inline iterator begin()
	{
		return MakeIt(0);
	}

	inline const_iterator begin() const
	{
		return MakeIt(0);
	}

	inline iterator end()
	{
		return MakeIt(m_Size);
	}

	inline const_iterator end() const
	{
		return MakeIt(m_Size);
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
			return MakeIt(0);
		}

		if (index == m_Size)
		{
			push_back(value);
			return MakeIt(index);
		}

		if (m_FallbackCapacity > 0) [[unlikely]]
		{
			if (m_Size == m_FallbackCapacity)
			{
				GrowFallback();
			}
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();
		}

		// Construct new tail
		new (&m_Data[m_Size]) T(std::move(m_Data[m_Size - 1]));

		if constexpr (std::is_trivially_copyable_v<T>)
		{
			// Shift right
			std::memmove(
				&m_Data[index + 1],
				&m_Data[index],
				(m_Size - index - 1) * sizeof(T));
		}
		else
		{
			// Move-assign backward
			for (size_type i = m_Size - 1; i > index; --i)
			{
				m_Data[i] = std::move(m_Data[i - 1]);
			}
		}

		// Assign new value
		m_Data[index] = value;

		++m_Size;

		return MakeIt(index);
	}

	inline iterator insert(iterator it, T&& value)
	{
		size_type index = it.m_Index;

		if (index > m_Size)
		{
			index = m_Size;
		}

		if (m_Size == 0)
		{
			push_back(std::move(value));
			return MakeIt(0);
		}

		if (index == m_Size)
		{
			push_back(std::move(value));
			return MakeIt(index);
		}

		if (m_FallbackCapacity > 0) [[unlikely]]
		{
			if (m_Size == m_FallbackCapacity)
			{
				GrowFallback();
			}
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();
		}

		// Construct new tail
		new (&m_Data[m_Size]) T(std::move(m_Data[m_Size - 1]));

		if constexpr (std::is_trivially_copyable_v<T>)
		{
			// Shift right
			std::memmove(
				&m_Data[index + 1],
				&m_Data[index],
				(m_Size - index - 1) * sizeof(T));
		}
		else
		{
			// Move-assign backward
			for (size_type i = m_Size - 1; i > index; --i)
			{
				m_Data[i] = std::move(m_Data[i - 1]);
			}
		}

		// Move-assign new value
		m_Data[index] = std::move(value);

		++m_Size;

		return MakeIt(index);
	}

	inline iterator erase(iterator it)
	{
		size_type index = it.m_Index;

		if (index >= m_Size)
		{
			return end();
		}

		if constexpr (std::is_trivially_copyable_v<T>)
		{
			// Shift left
			std::memmove(
				&m_Data[index],
				&m_Data[index + 1],
				(m_Size - index - 1) * sizeof(T));
		}
		else
		{
			// Move-assign forward
			for (size_type i = index; i + 1 < m_Size; ++i)
			{
				m_Data[i] = std::move(m_Data[i + 1]);
			}
		}

		// Erase old tail
		m_Data[m_Size - 1].~T();
	
		--m_Size;

		return MakeIt(index);
	}

private:
	inline iterator MakeIt(size_type i)
	{
		return iterator(m_Data, i);
	}

	inline const_iterator MakeIt(size_type i) const
	{
		return const_iterator(m_Data, i);
	}

	inline T* AllocateFallback(size_type n)
	{
		return fallback_allocator{}.allocate(n);
	}
	
	inline void DeallocateFallback(value_type* ptr)
	{
		fallback_allocator{}.deallocate(ptr, 0);
	}

	inline void MoveToFallback()
	{
		m_FallbackCapacity = std::max<size_type>(1, m_Size * 2);
		m_Data = AllocateFallback(m_FallbackCapacity);
		T* data = reinterpret_cast<T*>(m_Inline);
		if constexpr (std::is_trivially_copyable_v<T>)
		{
			std::memcpy(
				m_Data,
				data,
				m_Size * sizeof(T));
		}
		else
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (&m_Data[i]) T(std::move(data[i]));
				data[i].~T();
			}
		}
	}

	inline void MoveFromFallback() noexcept
	{
		size_type n = std::min(m_Size, N);
		T* data = reinterpret_cast<T*>(m_Inline);
		if constexpr (std::is_trivially_copyable_v<T>)
		{
			std::memcpy(
				data,
				m_Data,
				n * sizeof(T));
		}
		else
		{
			for (size_type i = 0; i < n; ++i)
			{
				new (&data[i]) T(std::move(m_Data[i]));
				m_Data[i].~T();
			}
		}
		for (size_type i = n; i < m_Size; ++i)
		{
			m_Data[i].~T();
		}
		DeallocateFallback(m_Data);
		m_Data = data;
		m_FallbackCapacity = 0;
	}

	inline void GrowFallback(size_type n = 0)
	{
		m_FallbackCapacity = n > 0 ? n : 2 * m_FallbackCapacity;
		T* data = AllocateFallback(m_FallbackCapacity);
		if constexpr (std::is_trivially_copyable_v<T>)
		{
			std::memcpy(
				data,
				m_Data,
				m_Size * sizeof(T));
		}
		else
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				new (&data[i]) T(std::move(m_Data[i]));
				m_Data[i].~T();
			}
		}
		DeallocateFallback(m_Data);
		m_Data = data;
	}

private:
	size_type m_Size;
	value_type* m_Data;
	size_type m_FallbackCapacity;
	alignas(alignof(value_type)) std::byte m_Inline[N * sizeof(value_type)];
};

}

