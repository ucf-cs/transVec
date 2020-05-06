#ifndef MEM_ALLOC_HPP
#define MEM_ALLOC_HPP

#include <assert.h>
#include <atomic>
#include <malloc.h>
#include <mutex>
#include <stdlib.h>
#include <typeinfo>

#include "define.hpp"

template <class T>
class MemAllocator
{
private:
    // The pool of allocated memory.
    static std::atomic<char *> pool;
    // An atomic ticket counter to keep track of what spot the next thread should claim.
    static std::atomic<size_t> ticket;
    // The number of thread pools generated.
    static size_t NUM_POOLS;
    // The size of the pool for this template.
    static size_t POOL_SIZE;
    // Used to determine if we missed any pools.
    static bool isInit;
#ifdef ALLOC_COUNT
    // A counter used to keep track of allocations. Use this to tune allocation sizes.
    static std::atomic<size_t> count;
#endif

    // Offset in the pool for this thread's access.
    thread_local static char *base;
    // A pointer to the head of the allocator list.
    thread_local static size_t freeIndex;
    // Used to determine whether or not to get a ticket.
    thread_local static bool hasTicket;
    // Used to distinguish between different threads.
    thread_local static size_t threadId;

public:
    using value_type = T;

    using pointer = value_type *;
    using const_pointer = typename std::pointer_traits<pointer>::template rebind<value_type const>;
    using void_pointer = typename std::pointer_traits<pointer>::template rebind<void>;
    using const_void_pointer = typename std::pointer_traits<pointer>::template rebind<const void>;

    using difference_type = typename std::pointer_traits<pointer>::difference_type;
    using size_type = std::make_unsigned_t<difference_type>;

    template <class U>
    struct rebind
    {
        typedef MemAllocator<U> other;
    };

    // Preallocate the pool.
    MemAllocator() noexcept
    {
        // Used to make sure we are initializing all memory allocator instances.
        // Prints this error once per uninitialized object type.
        if (!isInit)
        {
            isInit = true;
#ifdef ALLOC_COUNT
            printf("Allocating an uninitialized object of type %s and size %lu.\n", typeid(T).name(), sizeof(T));
//printf("Will use default pool size or use malloc as a fallback.\n");
#endif
        }

        NUM_POOLS = THREAD_COUNT * 2;

        // Only initialize the pool once.
        char *newPool = NULL;
        while (pool.load() == NULL)
        {
            size_t objectSize = sizeof(T);
            // *2 to ensure we have pools for our two rounds of threads.
            size_t poolSize = sizeof(T) * NUM_POOLS * POOL_SIZE;
            newPool = (char *)memalign(objectSize, poolSize);
            if (newPool == NULL)
            {
                assert(false);
            }
            char *nullVal = NULL;
            // If newPool is NULL, while loop will retry.
            if (!pool.compare_exchange_strong(nullVal, newPool))
            {
                ::operator delete(newPool);
            }
        }
        return;
    }

    template <class U>
    MemAllocator(MemAllocator<U> const &) noexcept {}

    // Use pointer if pointer is not a value_type*
    value_type *allocate(std::size_t n)
    {
        //return (T *)malloc(sizeof(T) * n);
#ifdef ALLOC_COUNT
        // Increment counter.
        count.fetch_add(n);
#endif

        // Ensure everything needed is allocated.
        MemAllocator();

        // Ensure each thread is properly initialized.
        if (!hasTicket)
        {
            threadId = ticket.fetch_add(1);
            if (threadId >= NUM_POOLS)
            {
                assert(threadId < NUM_POOLS);
            }
            hasTicket = true;
            base = pool.load() + threadId * sizeof(T) * POOL_SIZE;
            freeIndex = 0;

            // DEBUG: Verify pool initialization.
            //printf("pool.load()=%p\tthreadNum=%lu\tbase=%p\tsizeof(T)=%lu\t\n", pool.load(), threadId, base, sizeof(T));
        }

        value_type *ret = (value_type *)(base + freeIndex);
        freeIndex += n * sizeof(value_type);
        if (ret == NULL || freeIndex >= (sizeof(T) * POOL_SIZE))
        {
            printf("Pool for thread ID %lu ran out of elements of size %lu.\n", threadId, sizeof(T));
            // Can use malloc as a fallback.
            ret = (T *)malloc(sizeof(T) * n);
            assert(ret != NULL && "Failed to malloc.");
        }
        return ret;
    }

    void deallocate([[maybe_unused]] value_type *p, [[maybe_unused]] std::size_t n) noexcept // Use pointer if pointer is not a value_type*
    {
        // Do nothing. Difficult to deallocate with this setup, so just don't do it.

        //freeIndex -= n;
        //assert(freeIndex<T> > base);
        //memccpy(freeIndex, p, n);
        return;
    }

    // Initialize the memory pool.
    // TUNE
    static void init(size_t size = 0)
    {
        assert(size != 0);
        // Set a per-template instance pool size.
        POOL_SIZE = size;
        // Indicate that the pool has been properly initialized.
        isInit = true;
        // Allocate the pool.
        MemAllocator();
        return;
    }

    // Print out allocator usage statistics.
    static void report()
    {
#ifdef ALLOC_COUNT
        printf("Used %lu allocations for %lu pools for elements of size %lu.\n", count.load(), NUM_POOLS, sizeof(T));
#endif
        return;
    }

    value_type *allocate(std::size_t n, const_void_pointer)
    {
        return allocate(n);
    }

    template <class U, class... Args>
    void construct(U *p, Args &&... args)
    {
        ::new (p) U(std::forward<Args>(args)...);
    }

    template <class U>
    void destroy(U *p) noexcept
    {
        p->~U();
    }

    std::size_t max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max();
    }

    MemAllocator select_on_container_copy_construction() const
    {
        return *this;
    }

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::is_empty<MemAllocator>;
};

template <class T, class U>
bool operator==(MemAllocator<T> const &, MemAllocator<U> const &) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(MemAllocator<T> const &x, MemAllocator<U> const &y) noexcept
{
    return !(x == y);
}

template <typename T>
std::atomic<char *> MemAllocator<T>::pool(NULL);

template <typename T>
std::atomic<size_t> MemAllocator<T>::ticket(0);

template <typename T>
size_t MemAllocator<T>::NUM_POOLS;

// NOTE: This size is used if the pool is never explicitly initialized.
template <typename T>
size_t MemAllocator<T>::POOL_SIZE(2200000);

template <typename T>
bool MemAllocator<T>::isInit(false);

#ifdef ALLOC_COUNT
template <typename T>
std::atomic<size_t> MemAllocator<T>::count(0);
#endif

template <typename T>
thread_local char *MemAllocator<T>::base(NULL);

template <typename T>
thread_local size_t MemAllocator<T>::freeIndex(0);

template <typename T>
thread_local bool MemAllocator<T>::hasTicket(false);

template <typename T>
thread_local size_t MemAllocator<T>::threadId(0);

#endif