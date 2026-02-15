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

	std::lock_guard<std::mutex> m_Lock(m_Mutex);

	// Try to find a block in the existing spans
	for (auto& span : m_Spans)
	{
		if (void* ptr = AllocateBlock(span, size))
		{
			return ptr;
		}
	}

	size_t spanSize = size;
	if (spanSize < k_MinSpanSize)
	{
		spanSize = k_MinSpanSize;
	}

	spanSize = NOIS_ALIGN_TO(spanSize, k_MinBlockSize);

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

bool SpanAllocator::Deallocate(void* ptr)
{
	if (!ptr)
	{
		return false;
	}

	std::lock_guard<std::mutex> m_Lock(m_Mutex);

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
						it = span.blocks.erase(it);
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

	// Try to dealloate the span
	if (didDeallocate)
	{
		DeallocateSpan(ptr);
	}

	return didDeallocate;
}

void SpanAllocator::CoalesceFreeBlocks()
{
	std::lock_guard<std::mutex> m_Lock(m_Mutex);

	for (auto& span : m_Spans)
	{
		for (auto it = span.blocks.begin(); it != span.blocks.end();)
		{
			if (it->state == BlockState::Free)
			{
				if (auto itNext = std::next(it); itNext != span.blocks.end())
				{
					NZ_ASSERT(it->endAddr == itNext->startAddr);
					if (itNext->state == BlockState::Free)
					{
						itNext->startAddr = it->startAddr;
						it = span.blocks.erase(it);
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
	std::lock_guard<std::mutex> m_Lock(m_Mutex);

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
	std::lock_guard<std::mutex> m_Lock(m_Mutex);

	std::string result;

	size_t spanIndex = 0;
	for (const Span& span : m_Spans)
	{
		size_t spanSize = span.size;
		size_t usedBytes = 0;
		size_t totalBytes = 0;
		std::string line(width, '?');

		for (const Block& block : span.blocks)
		{
			size_t blockSize = block.endAddr - block.startAddr;
			totalBytes += blockSize;
			if (block.state == BlockState::Allocated)
			{
				usedBytes += blockSize;
			}

			size_t blockStart = block.startAddr - reinterpret_cast<uintptr_t>(span.ptr);
			size_t blockEnd   = block.endAddr   - reinterpret_cast<uintptr_t>(span.ptr);
			size_t startCol = (blockStart * width) / spanSize;
			size_t endCol   = (blockEnd   * width) / spanSize;
			for (size_t i = startCol; i < endCol && i < width; ++i)
			{
				line[i] = (block.state == BlockState::Free) ? '.' : '#';
			}
		}

		result += std::format("Span {} | {} | {} / {} MB\n",
			spanIndex++,
			line,
			static_cast<float>(usedBytes) / (1024.0f * 1024.0f),
			static_cast<float>(totalBytes) / (1024.0f * 1024.0f));
	}

	return result;
}


SpanAllocator::Span* SpanAllocator::AllocateSpan(size_t size)
{
	NZ_ASSERT(size >= k_MinSpanSize);

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
			block.startAddr = NOIS_ALIGN_TO(ptr, k_MinBlockSize);
			block.endAddr = block.startAddr + size;
			block.state = BlockState::Free;
			span.blocks.push_back(std::move(block));
		}

		// Store the span
		m_Spans.push_back(std::move(span));
	}

	return &m_Spans.back();
}

bool SpanAllocator::DeallocateSpan(void* ptr)
{
	bool didDeallocate = false;

	uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
	for (auto it = m_Spans.begin(); it != m_Spans.end(); ++it)
	{
		uintptr_t spanStartAddr = reinterpret_cast<uintptr_t>(it->ptr);
		uintptr_t spanEndAddr = spanStartAddr + it->size;
		if (ptrAddr >= spanStartAddr && ptrAddr <= spanEndAddr)
		{
			if (it->blocks.size() == 1 && it->blocks.back().state == BlockState::Free)
			{
				DeallocateVm(it->ptr, it->size);
				m_Spans.erase(it);

				didDeallocate = true;

				break;
			}
		}
	}

	return didDeallocate;
}

void* SpanAllocator::AllocateBlock(Span& span, size_t size)
{
	size_t allocSize = NOIS_ALIGN_TO(size, k_MinBlockSize);
	for (auto it = span.blocks.begin(); it != span.blocks.end(); ++it)
	{
		size_t blockSize = it->endAddr - it->startAddr;
		if (it->state == BlockState::Free && blockSize >= allocSize)
		{
			// If block is bigger than needed, split it
			if (blockSize > allocSize)
			{
				uintptr_t blockStartAddr = it->startAddr;
				uintptr_t blockEndAddr = it->endAddr;
				uintptr_t newBlockStartAddr = blockStartAddr + allocSize;
				uintptr_t newBlockEndAddr = blockEndAddr;

				{
					// The new block is the remainder
					Block newBlock;
					newBlock.startAddr = newBlockStartAddr;
					newBlock.endAddr = newBlockEndAddr;
					newBlock.state = BlockState::Free;
					it = std::prev(span.blocks.insert(std::next(it), std::move(newBlock)));
				}

				NZ_ASSERT(
					it->startAddr == blockStartAddr &&
					it->endAddr == blockEndAddr);

				// The existing block is start
				it->endAddr = newBlockStartAddr;
			}

			void* newBlockPtr = reinterpret_cast<void*>(it->startAddr);
			size_t newBlockSize = it->endAddr - it->startAddr;
			AcquireVm(newBlockPtr, newBlockSize);
			it->state = BlockState::Allocated;

			return newBlockPtr;
		}
	}

	NZ_ASSERT(!span.blocks.empty());

	return nullptr;
}

bool SpanAllocator::DeallocateBlock(Span& span, void* ptr)
{
	bool didDeallocate = false;

	uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
	for (auto it = span.blocks.begin(); it != span.blocks.end(); ++it)
	{
		if (ptrAddr >= it->startAddr && ptrAddr < it->endAddr)
		{
			size_t blockSize = it->endAddr - it->startAddr;
			void* blockPtr = reinterpret_cast<void*>(it->startAddr);
			RelieveVm(blockPtr, blockSize);
			it->state = BlockState::Free;

			didDeallocate = true;

			break;
		}
	}

	NZ_ASSERT(!span.blocks.empty());

	return didDeallocate;
}

void* SpanAllocator::AllocateVmAnon(size_t size) const
{
	void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	NZ_POSIX_ASSERT(ptr != MAP_FAILED, "mmap of %zu bytes failed", size);
	if (ptr == MAP_FAILED)
	{
		return nullptr;
	}
	if (ptr)
	{
		int sysErr = madvise(ptr, size, MADV_DONTNEED);
		NZ_POSIX_ASSERT(sysErr == 0, "madvise of %p with %zu bytes failed", ptr, size);
	}

	return ptr;
}

void SpanAllocator::DeallocateVm(void* ptr, size_t size) const
{
	int sysErr = munmap(ptr, size);
	NZ_POSIX_ASSERT(sysErr != -1, "munmap of %p with %zu bytes failed", ptr, size);
}

void SpanAllocator::AcquireVm(void* ptr, size_t size) const
{
	int sysErr = madvise(ptr, size, MADV_NORMAL);
	NZ_POSIX_ASSERT(sysErr == 0, "madvise of %p with %zu bytes failed", ptr, size);
}

void SpanAllocator::RelieveVm(void* ptr, size_t size) const
{
	int sysErr = madvise(ptr, size, MADV_DONTNEED);
	NZ_POSIX_ASSERT(sysErr == 0, "madvise of %p with %zu bytes failed", ptr, size);
}

}
