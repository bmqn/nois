#pragma once

#include <cstdint>

#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace nois {

using count_t = int64_t;
using f32_t = float;
using s32_t = int32_t;

template<typename T>
struct MonoSample
{
	T s;

	MonoSample<T> &operator+=(const MonoSample<T>& rhs)
	{
		s += rhs.s;
		return *this;
	}
};

template<typename T>
struct StereoSample
{
	union
	{
		T left;
		T s1;
	};
	union
	{
		T right;
		T s2;
	};

	StereoSample<T> &operator+=(const StereoSample<T>& rhs)
	{
		s1 += rhs.s1;
		s2 += rhs.s2;
		return *this;
	}
};

using FloatMonoSample = MonoSample<f32_t>;
using FloatStereoSample = StereoSample<f32_t>;

template<typename T>
using Ref_t = std::shared_ptr<T>;

template<typename T>
using RefFromThis_t = std::enable_shared_from_this<T>;

template<typename T, typename... Args>
Ref_t<T> MakeRef(Args &&...args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using Own_t = std::unique_ptr<T>;

template<typename T, typename... Args>
Own_t<T> MakeOwn(Args &&...args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

}
