#pragma once

#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {
namespace math {

template<typename T>
class Mat;

using FloatMat = Mat<f32_t>;

template<typename T>
class Mat
{
public:
	Mat()
		: m_M(0)
		, m_N(0)
		, m_Data()
	{
	}

	Mat(count_t m)
		: m_M(m)
		, m_N(m)
		, m_Data(m * m, T{ 0 })
	{
	}

	Mat(count_t m, count_t n)
		: m_M(m)
		, m_N(n)
		, m_Data(m * n, T{ 0 })
	{
	}

	inline count_t GetM() const
	{
		return m_M;
	}

	inline count_t GetN() const
	{
		return m_N;
	}

	inline void Zero()
	{
		Fill(T{ 0 });
	}

	inline void Fill(T value)
	{
		std::fill(m_Data.begin(), m_Data.end(), value);
	}

	inline void Copy(const T* data, count_t size)
	{
		std::copy_n(data, std::min(m_Data.size(), size), m_Data.begin());
	}

	void Resize(count_t m)
	{
		m_M = m;
		m_N = m;
		m_Data.resize(m * m, T{ 0 });
	}

	void Resize(count_t m, count_t n)
	{
		m_M = m;
		m_N = n;
		m_Data.resize(m * n, T{ 0 });
	}

	inline Mat<T> Multiply(const Mat<T>& mat) const
	{
		// TODO: vectorize

		Mat<T> result(m_M, mat.GetN());
		for (count_t i = 0; i < m_M; ++i)
		{
			for (count_t j = 0; j < mat.GetN(); ++j)
			{
				T acc = T{ 0 };
				for (count_t k = 0; k < m_N; ++k)
				{
					acc += (*this)(i, k) * mat(k, j);
				}
				result(i, j) = acc;
			}
		}
		return result;
	}

	inline T& operator()(count_t i, count_t j)
	{
		return m_Data[i * m_N + j];
	}

	inline const T& operator()(count_t i, count_t j) const
	{
		return m_Data[i * m_N + j];
	}

private:
	count_t m_M;
	count_t m_N;
	SmallVector<T, 64> m_Data;
};

}
}