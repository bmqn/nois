#pragma once

#include "nois/NoisConfig.hpp"
#include "nois/NoisTypes.hpp"
#include "nois/util/NoisSmallVector.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace nois {

static constexpr size_t k_PageSize = 64 * 1024 * 1024;
static constexpr size_t k_MinBlockSize = 4 * 1024;

struct SpanAllocatorStats
{
	size_t totalSpans = 0;   // number of spans
	size_t totalBlocks = 0;  // total blocks
	size_t freeBlocks = 0;   // free blocks
	size_t totalBytes = 0;   // total VM reserved
	size_t usedBytes = 0;    // allocated blocks
	size_t freeBytes = 0;    // free blocks
};

class SpanAllocator
{
private:
	enum class BlockState
	{
		Free,
		Allocated
	};

	struct Block
	{
		uintptr_t startAddr;
		uintptr_t endAddr;
		BlockState state;
	};

	struct Span
	{
		void* ptr;
		size_t size;
		SmallVector<Block, 512> blocks;
	};

public:
	SpanAllocator();

	void* Allocate(size_t size);
	void Deallocate(void* ptr);

	void CoalesceFreeBlocks();

	SpanAllocatorStats GetStats() const;
	std::string DumpSpanMap(size_t width = 64) const;

private:
	Span* AllocateSpan(size_t size);
	void DeallocateSpan(void* ptr);

	void* AllocateBlock(Span& span, size_t size);
	bool DeallocateBlock(Span& span, void* ptr);

	void* AllocateVmAnon(size_t size) const;
	void AcquireVm(void* ptr, size_t size) const;
	void RelieveVm(void* ptr, size_t size) const;

private:
	SmallVector<Span, 64> m_Spans;
};

}