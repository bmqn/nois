#pragma once

#include "nois/NoisConfig.hpp"
#include "nois/NoisMacros.hpp"
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
	using value_type = T;
	using size_type = std::size_t;

	SmallVector(size_type n = 0, const T &value = T{})
		: m_Size(0)
		, m_Data(reinterpret_cast<T*>(m_Inline))
		, m_FallbackActive(false)
		, m_Inline{}
	{
		resize(n, value);
	}

	SmallVector(const SmallVector& other)
		: m_Size(0)
		, m_Data(nullptr)
		, m_FallbackActive(false)
		, m_Inline{}
	{
		if (other.m_FallbackActive)
		{
			m_Fallback = other.m_Fallback;

			m_Size = other.m_Size;
			m_Data = m_Fallback.data();
			m_FallbackActive = true;
		}
		else
		{
			m_Size = other.m_Size;
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackActive = false;

			for (size_type i = 0; i < m_Size; ++i)
			{
				new (InlinePtr(i)) T(other[i]);
			}
		}
	}

	SmallVector(SmallVector &&other) noexcept
		: m_Size(0)
		, m_Data(nullptr)
		, m_FallbackActive(false)
		, m_Inline{}
	{
		if (other.m_FallbackActive)
		{
			m_Fallback = std::move(other.m_Fallback);

			m_Size = other.m_Size;
			m_Data = m_Fallback.data();
			m_FallbackActive = true;
		}
		else
		{
			m_Size = other.m_Size;
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackActive = false;

			for (size_type i = 0; i < m_Size; ++i)
			{
				new (InlinePtr(i)) T(std::move(other[i]));
				other.InlinePtr(i)->~T();
			}
		}

		other.m_Size = 0;
	}

	~SmallVector() noexcept
	{
		clear();
	}

	SmallVector& operator=(const SmallVector& other)
	{
		clear();

		if (other.m_FallbackActive)
		{
			m_Fallback = other.m_Fallback;

			m_Size = other.m_Size;
			m_Data = reinterpret_cast<T*>(m_Fallback.data());
			m_FallbackActive = true;
		}
		else
		{
			m_Size = other.m_Size;
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackActive = false;

			for (size_type i = 0; i < m_Size; ++i)
			{
				new (InlinePtr(i)) T(other[i]);
			}
		}
		
		return *this;
	}

	SmallVector &operator=(SmallVector &&other) noexcept
	{
		clear();

		if (other.m_FallbackActive)
		{
			m_Fallback = std::move(other.m_Fallback);

			m_Size = other.m_Size;
			m_Data = reinterpret_cast<T*>(m_Fallback.data());
			m_FallbackActive = true;
		}
		else
		{
			m_Size = other.m_Size;
			m_Data = reinterpret_cast<T*>(m_Inline);
			m_FallbackActive = false;

			for (size_type i = 0; i < m_Size; ++i)
			{
				new (InlinePtr(i)) T(std::move(other[i]));
				other.InlinePtr(i)->~T();
			}
		}

		other.m_Size = 0;

		return *this;
	}

	[[nodiscard]] inline bool empty() const
	{
		return m_Size == 0;
	}

	[[nodiscard]] inline bool full() const
	{
		return m_Size == N;
	}

	[[nodiscard]] inline size_type size() const
	{
		return m_Size;
	}

	inline void shrink_to_fit()
	{
		if (m_FallbackActive)
		{
			// Shink space
			m_Fallback.shrink_to_fit();

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
	}

	inline void reserve(size_type n)
	{
		if (m_FallbackActive)
		{
			// Reserve space
			m_Fallback.reserve(n);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else if (n > N)
		{
			MoveToFallback();

			// Reserve space
			m_Fallback.reserve(n);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
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
			if (m_FallbackActive)
			{
				MoveFromFallback(std::min(n, m_Size));
			}

			// Expanding inline
			if (n > m_Size)
			{
				for (size_type i = m_Size; i < n; ++i)
				{
					new (InlinePtr(i)) T(value);
				}
			}
			// Shrinking inline
			else if (n < N)
			{
				for (size_type i = n; i < m_Size; ++i)
				{
					InlinePtr(i)->~T();
				}
			}
		}
		else
		{
			if (!m_FallbackActive)
			{
				MoveToFallback();
			}

			m_Fallback.resize(n, value);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}

		m_Size = n;
	}

	inline void clear() noexcept
	{
		if (m_FallbackActive)
		{
			m_Fallback.clear();
		}
		else
		{
			for (size_type i = 0; i < m_Size; ++i)
			{
				InlinePtr(i)->~T();
			}
		}

		m_Size = 0;
	}

	inline void push_back(const T& value)
	{
		if (m_FallbackActive) [[unlikely]]
		{
			// Push the data
			m_Fallback.push_back(value);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();

			// Push the data
			m_Fallback.push_back(value);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else
		{
			// Construct new value
			new (InlinePtr(m_Size)) T(value);
		}

		++m_Size;
	}

	inline void push_back(T&& value)
	{
		if (m_FallbackActive) [[unlikely]]
		{
			// Push the data
			m_Fallback.push_back(std::move(value));

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();

			// Push the data
			m_Fallback.push_back(std::move(value));

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else
		{
			// Move-construct new value
			new (InlinePtr(m_Size)) T(std::move(value));
		}

		++m_Size;
	}

	template<typename ...Args>
	inline void emplace_back(Args &&...args)
	{
		if (m_FallbackActive) [[unlikely]]
		{
			// Construct the data
			m_Fallback.emplace_back(std::forward<Args>(args)...);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();

			// Construct the data
			m_Fallback.emplace_back(std::forward<Args>(args)...);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else
		{
			// Construct new value
			new (InlinePtr(m_Size)) T(std::forward<Args>(args)...);
		}

		++m_Size;
	}

	inline void pop_back() noexcept
	{
		if (m_FallbackActive) [[unlikely]]
		{
			// Pop the data
			m_Fallback.pop_back();
		}
		else
		{
			// Erase tail
			InlinePtr(m_Size - 1)->~T();
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
		assert(index < m_Size);
		assert(m_FallbackActive || index < N);
		return m_Data[index];
	}

	inline const T &operator[](size_type index) const
	{
		assert(index < m_Size);
		assert(m_FallbackActive || index < N);
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

		if (m_FallbackActive) [[unlikely]]
		{
			// Insert the data
			m_Fallback.insert(m_Fallback.begin() + index, value);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();

			// Insert the data
			m_Fallback.insert(m_Fallback.begin() + index, value);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else
		{
			// Construct new tail
			new (InlinePtr(m_Size)) T(std::move(*InlinePtr(m_Size - 1)));

			if constexpr (std::is_trivially_copyable_v<T>)
			{
				// Shift right
				std::memmove(
					InlinePtr(index + 1),
					InlinePtr(index),
					(m_Size - index) * sizeof(T));
			}
			else
			{
				// Move-assign backward
				for (size_type i = m_Size - 1; i > index; --i)
				{
					*InlinePtr(i) = std::move(*InlinePtr(i - 1));
				}
			}

			// Assign new value
			*InlinePtr(index) = value;
		}

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

		if (m_FallbackActive) [[unlikely]]
		{
			// Move-insert the data
			m_Fallback.insert(m_Fallback.begin() + index, std::move(value));

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else if (m_Size == N) [[unlikely]]
		{
			MoveToFallback();

			// Move-insert the data
			m_Fallback.insert(m_Fallback.begin() + index, std::move(value));

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else
		{
			// Construct new tail
			new (InlinePtr(m_Size)) T(std::move(*InlinePtr(m_Size - 1)));

			if constexpr (std::is_trivially_copyable_v<T>)
			{
				// Shift right
				std::memmove(
					InlinePtr(index + 1),
					InlinePtr(index),
					(m_Size - index) * sizeof(T));
			}
			else
			{
				// Move-assign backward
				for (size_type i = m_Size - 1; i > index; --i)
				{
					*InlinePtr(i) = std::move(*InlinePtr(i - 1));
				}
			}

			// Move-assign new value
			*InlinePtr(index) = std::move(value);
		}

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

		if (m_FallbackActive) [[unlikely]]
		{
			// Erase the data
			m_Fallback.erase(m_Fallback.begin() + index);

			// Update data in case vector realloc'd
			m_Data = m_Fallback.data();
		}
		else
		{
			if constexpr (std::is_trivially_copyable_v<T>)
			{
				// Shift left
				std::memmove(
					InlinePtr(index),
					InlinePtr(index + 1),
					(m_Size - index) * sizeof(T));
			}
			else
			{
				// Move-assign forward
				for (size_type i = index; i + 1 < m_Size; ++i)
				{
					*InlinePtr(i) = std::move(*InlinePtr(i + 1));
				}
			}

			// Erase old tail
			InlinePtr(m_Size - 1)->~T();
		}
	
		--m_Size;

		return MakeIt(index);
	}

private:
	NOIS_ALWAYS_INLINE T* InlinePtr(size_type i = 0)
	{
		return &reinterpret_cast<T*>(m_Inline)[i];
	}

	NOIS_ALWAYS_INLINE const T* InlinePtr(size_type i = 0) const
	{
		return &reinterpret_cast<const T*>(m_Inline)[i];
	}

	inline iterator MakeIt(size_type i)
	{
		return iterator(m_Data, i);
	}

	inline const_iterator MakeIt(size_type i) const
	{
		return iterator(m_Data, i);
	}

	inline void MoveToFallback()
	{
		assert(m_Size <= N);

		if constexpr (std::is_trivially_copyable_v<T>)
		{
			m_Fallback.resize(m_Size);
			std::memcpy(
				m_Fallback.data(),
				InlinePtr(0),
				m_Size * sizeof(T));
		}
		else
		{
			m_Fallback.reserve(m_Size);
			for (size_type i = 0; i < m_Size; ++i)
			{
				m_Fallback.push_back(std::move(*InlinePtr(i)));
				InlinePtr(i)->~T();
			}
		}

		m_Data = m_Fallback.data();
		m_FallbackActive = true;
	}

	inline void MoveFromFallback(size_type n) noexcept
	{
		assert(n <= m_Size);
		assert(n <= N);

		m_Data = std::launder(reinterpret_cast<T*>(m_Inline));
		m_FallbackActive = false;

		if constexpr (std::is_trivially_copyable_v<T>)
		{
			std::memcpy(
				InlinePtr(0),
				m_Fallback.data(),
				n * sizeof(T));
		}
		else
		{
			for (size_type i = 0; i < n; ++i)
			{
				new (InlinePtr(i)) T(std::move(m_Fallback[i]));
			}
		}

		m_Fallback.clear();
	}

private:
	size_type m_Size;
	value_type* m_Data;
	bool m_FallbackActive;

	alignas(alignof(value_type)) std::byte m_Inline[N * sizeof(value_type)];

	std::vector<value_type> m_Fallback;
};

}

