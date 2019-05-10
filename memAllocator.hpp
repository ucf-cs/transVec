#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <assert.h>
#include <atomic>
#include <malloc.h>
#include <mutex>
#include <stdlib.h>

#include "define.hpp"

// TUNE
// TODO: Cannot make this higher than around 15000000 it seems or may fail to compute return address.
#define POOL_SIZE 12000000//NUM_TRANSACTIONS * TRANSACTION_SIZE * 3

template <class T>
class MemAllocator
{
private:
    // The pool of allocated memory.
    static std::atomic<char *> pool;
    // An atomic ticket counter to keep track of what spot the next thread should claim.
    static std::atomic<size_t> ticket;

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
        // Only initialize the pool once.
        if (pool.load() == NULL)
        {
            size_t objectSize = sizeof(T);
            // *2 to ensure we have pools for our two rounds of threads.
            size_t poolSize = sizeof(T) * THREAD_COUNT * 2 * POOL_SIZE;
            char *newPool = (char *)memalign(objectSize, poolSize);
            char *nullVal = NULL;
            if (!pool.compare_exchange_strong(nullVal, newPool))
            {
                ::operator delete(newPool);
            }
        }

        if (!hasTicket)
        {
            threadId = ticket.fetch_add(1);
            assert(threadId < THREAD_COUNT * 2);
            hasTicket = true;
            base = pool.load() + threadId * sizeof(T) * POOL_SIZE;
            freeIndex = 0;
        }
        return;
    }

    template <class U>
    MemAllocator(MemAllocator<U> const &) noexcept {}

    // Use pointer if pointer is not a value_type*
    value_type *allocate(std::size_t n)
    {
        // Ensure everything needed is allocated.
        MemAllocator();

        value_type *ret = (value_type *)(base + freeIndex);
        assert(ret != NULL && "Failed to compute a return address.");

        freeIndex += n * sizeof(value_type);
        if (freeIndex > (sizeof(T) * POOL_SIZE))
        {
            printf("freeIndex=%lu\tmax=%lu\n", freeIndex, (size_t)(sizeof(T) * POOL_SIZE));
            assert(false && "Out of capacity.");
        }
        return ret;
    }

    void deallocate(value_type *p, std::size_t n) noexcept // Use pointer if pointer is not a value_type*
    {
        // Do nothing. Difficult to deallocate with this setup, so just don't do it.

        //freeIndex -= n;
        //assert(freeIndex<T> > base);
        //memccpy(freeIndex, p, n);
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
thread_local char *MemAllocator<T>::base(NULL);

template <typename T>
thread_local size_t MemAllocator<T>::freeIndex(0);

template <typename T>
thread_local bool MemAllocator<T>::hasTicket(false);

template <typename T>
thread_local size_t MemAllocator<T>::threadId(0);

#endif