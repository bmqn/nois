#pragma once

#include "nois/NoisConfig.hpp"
#include "nois/NoisMacros.hpp"
#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

namespace nois {
namespace math {

template<typename T>
class Mat;
template<typename T>
class MatView;
template<typename T>
using ConstMatView = MatView<const T>;

using FloatMat = Mat<f32_t>;
using FloatMatView = MatView<f32_t>;
using ConstFloatMatView = ConstMatView<f32_t>;

template<typename T>
class Mat
{
	friend class MatView<T>;

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

	Mat(count_t m, count_t n, const T* data)
		: m_M(m)
		, m_N(n)
		, m_Data(m * n, T{ 0 })
	{
		Copy(data, m * n);
	}

	Mat(MatView<T> mat)
		: m_M(mat.m_M)
		, m_N(mat.m_N)
		, m_Data(mat.m_M * mat.m_N, T{ 0 })
	{
		Copy(mat);
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
		std::copy_n(data, std::min(m_M * m_N, size), m_Data.begin());
	}

	inline void Copy(MatView<T> mat)
	{
		std::copy_n(mat.m_Data, std::min(m_M * m_N, mat.m_M * mat.m_N), m_Data.begin());
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

	inline void Multiply(ConstMatView<T> mat, MatView<T> outMat) const
	{
		// TODO: vectorize

		for (count_t i = 0; i < m_M; ++i)
		{
			for (count_t j = 0; j < mat.GetN(); ++j)
			{
				T acc = T{ 0 };
				for (count_t k = 0; k < m_N; ++k)
				{
					acc += (*this)(i, k) * mat(k, j);
				}
				outMat(i, j) = acc;
			}
		}
	}

	inline T& operator()(count_t i, count_t j)
	{
		return m_Data[i * m_N + j];
	}

	inline const T& operator()(count_t i, count_t j) const
	{
		return m_Data[i * m_N + j];
	}

	operator MatView<T>()
	{
		return MatView<T>(m_M, m_N, m_Data.data());
	}

	operator ConstMatView<T>() const
	{
		return ConstMatView<T>(m_M, m_N, m_Data.data());
	}

private:
	count_t m_M;
	count_t m_N;
	SmallVector<T, 128> m_Data;
};

template<typename T>
class MatView
{
	friend class Mat<T>;

public:
	MatView(count_t m, count_t n, T* data)
		: m_M(m)
		, m_N(n)
		, m_Data(data)
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
		std::fill(m_Data, m_Data + (m_M * m_N), value);
	}

	inline void Copy(const T* data, count_t size)
	{
		std::copy_n(data, std::min(m_M * m_N, size), m_Data);
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

	inline void Multiply(ConstMatView<T> mat, MatView<T> outMat) const
	{
		// TODO: vectorize

		for (count_t i = 0; i < m_M; ++i)
		{
			for (count_t j = 0; j < mat.GetN(); ++j)
			{
				T acc = T{ 0 };
				for (count_t k = 0; k < m_N; ++k)
				{
					acc += (*this)(i, k) * mat(k, j);
				}
				outMat(i, j) = acc;
			}
		}
	}

	inline T& operator()(count_t i, count_t j)
	{
		return m_Data[i * m_N + j];
	}

	inline const T& operator()(count_t i, count_t j) const
	{
		return m_Data[i * m_N + j];
	}

	operator ConstMatView<T>() const
	{
		return ConstMatView<T>(m_M, m_N, m_Data);
	}

private:
	count_t m_M;
	count_t m_N;
	T* m_Data;
};

}
}

#if NOIS_ENABLE_AVX_SIMD
#include "NoisMatrixAvx.inl"
#endif // NOIS_ENABLE_AVX_SIMD
