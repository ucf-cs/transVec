#include "allocator.hpp"


// Initialize per-thread allocators.
void threadAllocatorInit(int threadNum)
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
	// printf("Initializing the memory allocators.\n");
	// printf("Operation* allocator.\n");
	MemAllocator<Operation *>::init();
	// NOTE: Other MemAllocators are implicitly initialized.
	// Would be better to initialize them in advance for performance.
	// It's not a big deal if we pre-fill the vector first.
	// printf("Finished initializing the memory allocators.\n\n");

#ifdef SEGMENTVEC
	// Preallocate the pages.
	// printf("sizeof(Page<VAL, SGMT_SIZE>)=%lu\n", sizeof(Page<VAL, SGMT_SIZE>));
	Allocator<Page<VAL, SGMT_SIZE>>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
	// Preallocate the size pages.
	// printf("sizeof(Page<VAL, SGMT_SIZE>)=%lu\n", sizeof(Page<size_t, 1>));
	Allocator<Page<size_t, 1>>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
	// Preallocate page maps.
	// printf("sizeof(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>)=%lu\n", sizeof(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>));
	Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
#endif
#ifdef COMPACTVEC
	// Preallocate compact elements.
	printf("sizeof(CompactElement)=%lu\n", sizeof(CompactElement));
	Allocator<CompactElement>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
#endif
#ifdef COMPACTVEC
	// Preallocate the RWOperation elements.
	printf("sizeof(RWOperation)=%lu\n", sizeof(RWOperation));
	Allocator<RWOperation>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT * THREAD_COUNT);
#elif defined(SEGMENTVEC) || defined(BOOSTEDVEC)
	Allocator<RWOperation>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
#endif
#if defined(SEGMENTVEC) || defined(COMPACTVEC) || defined(BOOSTEDVEC)
	// Preallocate the RWSet elements.
	// printf("sizeof(RWSet)=%lu\n", sizeof(RWSet));
	Allocator<RWSet>::init(NUM_TRANSACTIONS * THREAD_COUNT);
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
#if defined(SEGMENTVEC) || defined(COMPACTVEC) || defined(BOOSTEDVEC)
	Allocator<RWOperation>::report();
	Allocator<RWSet>::report();
#endif
	return;
}
