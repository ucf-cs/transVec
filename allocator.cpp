#include "allocator.hpp"

// Initialize per-thread allocators.
void threadAllocatorInit([[maybe_unused]] int threadNum)
{
#ifdef SEGMENTVEC
	Allocator<Page<VAL, SGMT_SIZE>>::threadInit(threadNum);
	Allocator<Page<size_t, 1>>::threadInit(threadNum);
	Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::threadInit(threadNum);
#endif
#ifdef COMPACTVEC
	Allocator<CompactElement>::threadInit(threadNum);
#endif
#if defined(SEGMENTVEC) || defined(COMPACTVEC) || defined(BOOSTEDVEC)
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

#ifdef SEGMENTVEC
// Preallocate the pages.
#ifdef ALLOC_COUNT
	printf("sizeof(Page<VAL, SGMT_SIZE>)=%lu\n", sizeof(Page<VAL, SGMT_SIZE>));
#endif
	Allocator<Page<VAL, SGMT_SIZE>>::init((size_t)(NUM_TRANSACTIONS * 1.064) * TRANSACTION_SIZE);
// Preallocate the size pages.
#ifdef ALLOC_COUNT
	printf("sizeof(Page<size_t, 1>)=%lu\n", sizeof(Page<size_t, 1>));
#endif
	Allocator<Page<size_t, 1>>::init((NUM_TRANSACTIONS + 1) * TRANSACTION_SIZE);
// Preallocate page maps.
#ifdef ALLOC_COUNT
	printf("sizeof(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>)=%lu\n", sizeof(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>));
#endif
	Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::init((NUM_TRANSACTIONS + 1) * TRANSACTION_SIZE);
#endif
#ifdef COMPACTVEC
// Preallocate compact elements.
#ifdef ALLOC_COUNT
	printf("sizeof(CompactElement)=%lu\n", sizeof(CompactElement));
#endif
	Allocator<CompactElement>::init((NUM_TRANSACTIONS + 1) * TRANSACTION_SIZE);
#endif
#if defined(SEGMENTVEC) || defined(COMPACTVEC) || defined(BOOSTEDVEC)
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
#ifdef SEGMENTVEC
	Allocator<Page<VAL, SGMT_SIZE>>::report();
	Allocator<Page<size_t, 1>>::report();
	Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::report();
#endif
#ifdef COMPACTVEC
// Preallocate compact elements.
#ifdef ALLOC_COUNT
	printf("sizeof(CompactElement)=%lu\n", sizeof(CompactElement));
#endif
	Allocator<CompactElement>::report();
#endif
#if defined(SEGMENTVEC) || defined(COMPACTVEC) || defined(BOOSTEDVEC)
	Allocator<RWOperation>::report();
	Allocator<RWSet>::report();
#endif
	return;
}
