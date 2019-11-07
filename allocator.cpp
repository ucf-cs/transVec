#include "allocator.hpp"

// Initialize per-thread allocators.
void threadAllocatorInit([[maybe_unused]] int threadNum)
{
#ifdef COMPACTVEC
	Allocator<CompactElement>::threadInit(threadNum);
#endif
#if defined(COMPACTVEC) || defined(BOOSTEDVEC)
	Allocator<RWOperation>::threadInit(threadNum);
	Allocator<RWSet>::threadInit(threadNum);
#endif
	return;
}

void allocatorInit()
{
#ifdef ALLOC_COUNT
	printf("Initializing the memory allocators.\n");
	printf("sizeof(Operation *)=%lu\n", sizeof(Operation *));
#endif
	MemAllocator<Operation *>::init((NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT) + 1);
// NOTE: Other MemAllocators are implicitly initialized.
// Would be better to initialize them in advance for performance.
// It's not a big deal if we pre-fill the vector first.
#ifdef ALLOC_COUNT
	printf("Finished initializing the memory allocators.\n\n");
#endif

#ifdef COMPACTVEC
// Preallocate compact elements.
#ifdef ALLOC_COUNT
	printf("sizeof(CompactElement)=%lu\n", sizeof(CompactElement));
#endif
	Allocator<CompactElement>::init((NUM_TRANSACTIONS + 1) * TRANSACTION_SIZE);
#endif
#if defined(COMPACTVEC) || defined(BOOSTEDVEC)
// Preallocate the RWOperation elements.
#ifdef ALLOC_COUNT
	printf("sizeof(RWOperation)=%lu\n", sizeof(RWOperation));
#endif
	Allocator<RWOperation>::init(2 * NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
// Preallocate the RWSet elements.
#ifdef ALLOC_COUNT
	printf("sizeof(RWSet)=%lu\n", sizeof(RWSet));
#endif
	Allocator<RWSet>::init((1 + NUM_TRANSACTIONS) * THREAD_COUNT);
#endif
	return;
}

void allocatorReport()
{
	// Report memory allocator usage.
	MemAllocator<Operation *>::report();

// Report object allocator usage.
#ifdef COMPACTVEC
// Preallocate compact elements.
#ifdef ALLOC_COUNT
	printf("sizeof(CompactElement)=%lu\n", sizeof(CompactElement));
#endif
	Allocator<CompactElement>::report();
#endif
#if defined(COMPACTVEC) || defined(BOOSTEDVEC)
	Allocator<RWOperation>::report();
	Allocator<RWSet>::report();
#endif
	return;
}
