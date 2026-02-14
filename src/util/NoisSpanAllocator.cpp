#include "nois/util/NoisSpanAllocator.hpp"

#include <format>
#include <sys/mman.h>

namespace nois {

SpanAllocator::SpanAllocator()
{
}

void* SpanAllocator::Allocate(size_t size)
{
	if (size == 0)
	{
		return nullptr;
	}

	// Try to find a block in the existing spans
	for (auto& span : m_Spans)
	{
		if (void* ptr = AllocateBlock(span, size))
		{
			return ptr;
		}
	}

	size_t spanSize = NOIS_ALIGN_TO(size, k_PageSize);

	// Allocate new span if we don't have space
	if (Span* spanPtr = AllocateSpan(spanSize))
	{
		if (void* ptr = AllocateBlock(*spanPtr, size))
		{
			return ptr;
		}
	}

	return nullptr;
}

void SpanAllocator::Deallocate(void* ptr)
{
	if (!ptr)
	{
		return;
	}

	bool didDeallocate = false;

	for (auto& span : m_Spans)
	{
		// Try to dealloate the block
		didDeallocate = DeallocateBlock(span, ptr);

		// Coalesce free blocks
		for (auto it = span.blocks.begin(); it != span.blocks.end();)
		{
			if (it->state == BlockState::Free)
			{
				if (auto itNext = std::next(it); itNext != span.blocks.end())
				{
					if (itNext->state == BlockState::Free)
					{
						itNext->startAddr = it->startAddr;
						auto blockNextStartAddr = itNext->startAddr;
						auto blockNextEndAddr = itNext->endAddr;
						it = span.blocks.erase(it);
						NZ_ASSERT(
							it->startAddr == blockNextStartAddr &&
							it->endAddr == blockNextEndAddr);
						continue;
					}
				}
			}
			++it;
		}

		// Only coalesce while spans we tried deallocating within
		if (didDeallocate)
		{
			break;
		}
	}

	NZ_ASSERT(didDeallocate, "Could not deallocate %p", ptr);
}

void SpanAllocator::CoalesceFreeBlocks()
{
	for (auto& span : m_Spans)
	{
		for (auto it = span.blocks.begin(); it != span.blocks.end();)
		{
			if (it->state == BlockState::Free)
			{
				if (auto itNext = std::next(it); itNext != span.blocks.end())
				{
					if (itNext->state == BlockState::Free)
					{
						itNext->startAddr = it->startAddr;
						auto blockNextStartAddr = itNext->startAddr;
						auto blockNextEndAddr = itNext->endAddr;
						it = span.blocks.erase(it);
						NZ_ASSERT(
							it->startAddr == blockNextStartAddr &&
							it->endAddr == blockNextEndAddr);
						continue;
					}
				}
			}
			++it;
		}
	}
}

SpanAllocatorStats SpanAllocator::GetStats() const
{
	SpanAllocatorStats stats;

	stats.totalSpans = m_Spans.size();
	for (const auto& span : m_Spans)
	{
		stats.totalBytes += span.size;
		for (const auto& block : span.blocks)
		{
			size_t blockSize = block.endAddr - block.startAddr;
			stats.totalBlocks++;
			if (block.state == BlockState::Free)
			{
				stats.freeBlocks++;
				stats.freeBytes += blockSize;
			}
			else
			{
				stats.usedBytes += blockSize;
			}
		}
	}

	return stats;
}

std::string SpanAllocator::DumpSpanMap(size_t width) const
{
	std::string result;

	size_t spanIndex = 0;
	for (const Span& span : m_Spans)
	{
		std::string line(width, '?');

		for (const Block& block : span.blocks)
		{
			size_t spanSize = span.size;
			size_t blockStart = block.startAddr - reinterpret_cast<uintptr_t>(span.ptr);
			size_t blockEnd   = block.endAddr   - reinterpret_cast<uintptr_t>(span.ptr);

			size_t startCol = (blockStart * width) / spanSize;
			size_t endCol   = (blockEnd   * width) / spanSize;

			for (size_t i = startCol; i < endCol && i < width; ++i)
			{
				line[i] = (block.state == BlockState::Free) ? '.' : '#';
			}
		}

		result += std::format("Span {} | {}\n", spanIndex++, line);
	}

	return result;
}


SpanAllocator::Span* SpanAllocator::AllocateSpan(size_t size)
{
	NZ_ASSERT(NOIS_ALIGNED_TO(size, k_PageSize));

	void* ptr = AllocateVmAnon(size);

	if (!ptr)
	{
		return nullptr;
	}

	{
		Span span;
		span.ptr = ptr;
		span.size = size;

		{
			// Start with one big free block
			Block block;
			block.startAddr = reinterpret_cast<uintptr_t>(ptr);
			block.endAddr = block.startAddr + size;
			block.state = BlockState::Free;
			span.blocks.push_back(std::move(block));
		}

		// Store the span
		m_Spans.push_back(std::move(span));
	}

	return &m_Spans.back();
}

void* SpanAllocator::AllocateBlock(Span& span, size_t size)
{
	size_t allocSize = std::max(NOIS_ALIGN_TO(size, k_MinBlockSize), k_MinBlockSize);
	for (auto it = span.blocks.begin(); it != span.blocks.end(); ++it)
	{
		size_t blockSize = it->endAddr - it->startAddr;
		if (it->state == BlockState::Free && blockSize >= allocSize)
		{
			// If block is bigger than needed, split it
			if (blockSize > allocSize)
			{
				{
					// The new block is the remainder
					Block newBlock;
					newBlock.startAddr = it->startAddr + allocSize;
					newBlock.endAddr = it->endAddr;
					newBlock.state = BlockState::Free;
					it = std::prev(span.blocks.insert(std::next(it), std::move(newBlock)));
				}

				// The existing block is start
				it->endAddr = it->startAddr + allocSize;
			}

			void* blockPtr = reinterpret_cast<void*>(it->startAddr);
			size_t blockSize = it->endAddr - it->startAddr;
			AcquireVm(blockPtr, blockSize);
			it->state = BlockState::Allocated;

			return blockPtr;
		}
	}

	return nullptr;
}

bool SpanAllocator::DeallocateBlock(Span& span, void* ptr)
{
	uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);

	bool didDeallocate = false;

	for (auto it = span.blocks.begin(); it != span.blocks.end(); ++it)
	{
		if (ptrAddr >= it->startAddr && ptrAddr < it->endAddr)
		{
			void* blockPtr = reinterpret_cast<void*>(it->startAddr);
			size_t blockSize = it->endAddr - it->startAddr;
			RelieveVm(blockPtr, blockSize);
			it->state = BlockState::Free;

			didDeallocate = true;

			break;
		}
	}

	return didDeallocate;
}

void* SpanAllocator::AllocateVmAnon(size_t size) const
{
	void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED)
	{
		NZ_LOG("mmap of %zu bytes failed: %s (errno=%d)", size, strerror(errno), errno);
		return nullptr;
	}
	if (ptr)
	{
		int err = madvise(ptr, size, MADV_DONTNEED);
		NZ_ASSERT(err == 0, "madvise of %p with %zu bytes failed: %s (errno=%d)", ptr, size, strerror(errno), errno);
	}

	return ptr;
}

void SpanAllocator::AcquireVm(void* ptr, size_t size) const
{
	int err = madvise(ptr, size, MADV_NORMAL);
	NZ_ASSERT(err == 0, "madvise of %p with %zu bytes failed: %s (errno=%d)", ptr, size, strerror(errno), errno);
}

void SpanAllocator::RelieveVm(void* ptr, size_t size) const
{
	int err = madvise(ptr, size, MADV_DONTNEED);
	NZ_ASSERT(err == 0, "madvise of %p with %zu bytes failed: %s (errno=%d)", ptr, size, strerror(errno), errno);
}

}
