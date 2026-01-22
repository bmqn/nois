#include "NoisMatrix.hpp"

#include <immintrin.h>

namespace nois {
namespace math {

namespace detail {

inline void Multiply(ConstMatView<f32_t> a, ConstMatView<f32_t> b, MatView<f32_t> y)
{
	const count_t M = a.GetM();
	const count_t N = b.GetN();
	const count_t K = b.GetM();
	for (count_t i = 0; i < M; ++i)
	{
		for (count_t j = 0; j < N; ++j)
		{
			count_t k = 0;
			f32_t acc = 0.0f;
			f32_t vaccBuf[8]{ 0.0f };
			__m256 vacc = _mm256_setzero_ps();

			for (; k + 7 < K; k += 8)
			{
				__m256 va = _mm256_loadu_ps(
					&a(i, k));
				__m256 vb = _mm256_set_ps(
					b(k + 7, j), b(k + 6, j),
					b(k + 5, j), b(k + 4, j),
					b(k + 3, j), b(k + 2, j),
					b(k + 1, j), b(k + 0, j));
				vacc = _mm256_add_ps(
					vacc,
					_mm256_mul_ps(va, vb));
			}
			_mm256_storeu_ps(vaccBuf, vacc);
			acc +=
				vaccBuf[0] + vaccBuf[1] +
				vaccBuf[2] + vaccBuf[3] +
				vaccBuf[4] + vaccBuf[5] +
				vaccBuf[6] + vaccBuf[7];
			for (; k < K; ++k)
			{
				acc += a(i, k) * b(k, j);
			}

			y(i, j) = acc;
		}
	}
}

}

template<>
inline Mat<f32_t> Mat<f32_t>::Multiply(const Mat<f32_t>& mat) const
{
	Mat<f32_t> result(m_M, mat.GetN());
	detail::Multiply(*this, mat, result);
	return result;
}

template<>
inline void Mat<f32_t>::Multiply(ConstMatView<f32_t> mat, MatView<f32_t> outMat) const
{
	detail::Multiply(*this, mat, outMat);
}

template<>
inline Mat<f32_t> MatView<f32_t>::Multiply(const Mat<f32_t>& mat) const
{
	Mat<f32_t> result(m_M, mat.GetN());
	detail::Multiply(*this, mat, result);
	return result;
}

template<>
inline void MatView<f32_t>::Multiply(ConstMatView<f32_t> mat, MatView<f32_t> outMat) const
{
	detail::Multiply(*this, mat, outMat);
}

}
}
