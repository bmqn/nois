#pragma once

#include "nois/NoisTypes.hpp"

namespace nois {

using MallocFunc_t = void*(size_t);
using FreeFunc_t = void(void*);

void SetAllocFuncs(MallocFunc_t* mallocFunc, FreeFunc_t* freeFunc);
void* Malloc(size_t size);
void Free(void* ptr);

template<typename T>
struct Allocator
{
	using value_type = T;
	
	Allocator() = default;
	Allocator(const Allocator&) = delete;
	Allocator(Allocator&&) = delete;
	Allocator& operator=(const Allocator&) = delete;
	Allocator& operator=(Allocator&&) = delete;
	
	value_type* allocate(size_t n)
	{
		return static_cast<value_type*>(Malloc(sizeof(value_type) * n));
	}
	
	void deallocate(value_type* ptr, size_t n)
	{
		Free(ptr);
	}

	bool operator==(const Allocator&) const noexcept
	{
		return true;
	}

	bool operator!=(const Allocator&) const noexcept
	{
		return false;
	}
};

}
