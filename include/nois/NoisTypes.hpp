#pragma once

#include <cstdint>
#include <limits>
#include <memory>

namespace nois {

using count_t = int32_t;
using ucount_t = uint32_t;
using f32_t = float;
using f64_t = double;
using s32_t = int32_t;
using u32_t = uint32_t;
using s64_t = int64_t;
using u64_t = uint64_t;

namespace f32 {

constexpr f32_t k_Min = std::numeric_limits<f32_t>::min();
constexpr f32_t k_Max = std::numeric_limits<f32_t>::max();
constexpr f32_t k_Infinity = std::numeric_limits<f32_t>::infinity();

}

template<typename T>
using Ref_t = std::shared_ptr<T>;

template<typename T>
using RefFromThis_t = std::enable_shared_from_this<T>;

template<typename T, typename... Args>
inline Ref_t<T> MakeRef(Args &&...args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename U>
inline Ref_t<T> StaticRefCast(Ref_t<U> ptr)
{
	return std::static_pointer_cast<T>(ptr);
}

template<typename T>
using Own_t = std::unique_ptr<T>;

template<typename T, typename... Args>
inline Own_t<T> MakeOwn(Args &&...args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename B>
inline Ref_t<T> PtrCast(Ref_t<B> ptr)
{	
	return std::static_pointer_cast<T>(ptr);
}

}
