#include <nois/util/NoisSpanAllocator.hpp>

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include <vector>

void PrintStats(const nois::SpanAllocator& allocator, const char* stage)
{
	auto stats = allocator.GetStats();
	printf("\n=== %s ===\n", stage);
	printf("Allocator:\n");
	printf("  Total spans: %zu, Total VM: %zu MB\n", stats.totalSpans, stats.totalBytes / (1024*1024));
	printf("  Total blocks: %zu, Free blocks: %zu\n", stats.totalBlocks, stats.freeBlocks);
	printf("  Used: %zu MB, Free: %zu MB\n", stats.usedBytes / (1024*1024), stats.freeBytes / (1024*1024));

	std::ifstream statm("/proc/self/statm");
	size_t size = 0, resident = 0, shared = 0, text = 0, lib = 0, data = 0, dt = 0;
	if (statm >> size >> resident >> shared >> text >> lib >> data >> dt)
	{
		float pageSizeMB = (float)sysconf(_SC_PAGESIZE) / (1024*1024);
		printf("Real Memory (MB):\n");
		printf("  Virtual size: %f MB\n", size * pageSizeMB);
		printf("  Resident size: %f MB\n", resident * pageSizeMB);
	}

	printf("Spans:\n");
	printf("%s\n", allocator.DumpSpanMap().data());
}

int main()
{
	nois::SpanAllocator allocator;
	std::vector<void*> ptrs;
	std::vector<void*> ptrsLarge;
	std::vector<void*> ptrsRandom;

	// --- Phase 0: No allocation ---
	PrintStats(allocator, "After Phase 0: no allocation");

	// --- Phase 1: Allocate 512 blocks of increasing size ---
	for (int i = 0; i < 512; ++i)
	{
		int size = 1 + 16 * i;
		void* ptr = allocator.Allocate(size);
		assert(ptr != nullptr);
		memset(ptr, 0, size);
		*reinterpret_cast<uint32_t*>(ptr) = i;
		ptrs.push_back(ptr);
	}
	PrintStats(allocator, "After Phase 1: initial allocations");

	// --- Phase 2: Deallocate every other block ---
	for (int i = 0; i < 512; i += 2)
	{
		allocator.Deallocate(ptrs[i]);
		ptrs[i] = nullptr;
	}
	PrintStats(allocator, "After Phase 2: every other block deallocated");

	// --- Phase 3: Reallocate freed blocks with larger size ---
	for (int i = 0; i < 512; i += 2)
	{
		int size = 1 + 32 * i;
		void* ptr = allocator.Allocate(size);
		assert(ptr != nullptr);
		memset(ptr, 0, size);
		*reinterpret_cast<uint32_t*>(ptr) = 69;
		ptrs[i] = ptr;
	}
	PrintStats(allocator, "After Phase 3: reallocated freed blocks");

	// --- Phase 4: Verify memory contents ---
	for (int i = 0; i < 512; ++i)
	{
		uint32_t val = *reinterpret_cast<uint32_t*>(ptrs[i]);
		if (i % 2 == 0)
			assert(val == 69);
		else
			assert(val == i);
	}
	printf("Phase 4: All values verified!\n");

	// --- Phase 5: Allocate large blocks ---
	for (int i = 0; i < 16; ++i)
	{
		size_t size = 128 * 1024 * 1024;
		void* ptr = allocator.Allocate(size);
		assert(ptr != nullptr);
		memset(ptr, 0, size);
		*reinterpret_cast<uint32_t*>(ptr) = i + 1000;
		ptrsLarge.push_back(ptr);
	}
	PrintStats(allocator, "After Phase 5: large blocks allocated");

	// Deallocate half of the large blocks
	for (int i = 0; i < 16; i += 2)
	{
		allocator.Deallocate(ptrsLarge[i]);
		ptrsLarge[i] = nullptr;
	}
	PrintStats(allocator, "After Phase 5b: some large blocks freed");

	// --- Phase 6: Randomized allocations ---
	std::mt19937 rng(12345);
	std::uniform_int_distribution<size_t> sizeDist(1, 64*1024*1024);
	std::uniform_int_distribution<int> choiceDist(0, 1);

	for (int i = 0; i < 1024; ++i)
	{
		if (!ptrsRandom.empty() && choiceDist(rng) == 0)
		{
			size_t idx = rng() % ptrsRandom.size();
			allocator.Deallocate(ptrsRandom[idx]);
			ptrsRandom.erase(ptrsRandom.begin() + idx);
		}
		else
		{
			size_t size = sizeDist(rng);
			void* ptr = allocator.Allocate(size);
			if (ptr)
			{
				memset(ptr, 0, size);
				*reinterpret_cast<uint32_t*>(ptr) = static_cast<uint32_t>(i);
				ptrsRandom.push_back(ptr);
			}
		}
	}
	PrintStats(allocator, "After Phase 6: randomized allocations");

	// --- Phase 7: Cleanup everything ---
	for (void* ptr : ptrs)
		allocator.Deallocate(ptr);
	for (void* ptr : ptrsLarge)
		allocator.Deallocate(ptr);
	for (void* ptr : ptrsRandom)
		allocator.Deallocate(ptr);
	PrintStats(allocator, "After phase 7: cleanup everything");

	// Coalesce free blocks
	allocator.CoalesceFreeBlocks();
	PrintStats(allocator, "After phase 7b: coalesce free blocks");

	printf("Full allocator test completed successfully!\n");

	return 0;
}
