#pragma once

#include <cinttypes>

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

namespace nois {

using data_t = float;
using count_t = int64_t;

template<typename T>
using Ref_t = std::shared_ptr<T>;

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
