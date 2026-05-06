#include "nois/memory/NoisAllocator.hpp"


namespace nois {

static MallocFunc_t* g_MallocFunc = std::malloc;
static FreeFunc_t* g_FreeFunc = std::free;

void SetAllocFuncs(MallocFunc_t* mallocFunc, FreeFunc_t* freeFunc)
{
	g_MallocFunc = mallocFunc;
	g_FreeFunc = freeFunc;
}

void* Malloc(size_t size)
{
	return g_MallocFunc(size);
}

void Free(void* ptr)
{
	g_FreeFunc(ptr);
}

} // namespace nois
