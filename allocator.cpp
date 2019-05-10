#include "allocator.hpp"

template <typename DataType>
void Allocator<DataType>::init(size_t poolSize)
{
    // Only initialize the pool once.
    if (isInit)
    {
        printf("Attempted to initialize a pool that has already been initialized.\n");
        return;
    }
    // Initialize an empty pool for each thread.
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        std::vector<DataType *> *threadPool = new std::vector<DataType *>();
        threadPool->reserve(poolSize);

        // Initialize the pool of objects.
        for (size_t j = 0; j < poolSize; j++)
        {
            DataType *object = new DataType();
            if (object == NULL)
            {
                printf("Something bad happened!\n");
            }
            threadPool->push_back(object);
        }
        pool.push_back(threadPool);
    }
    // Mark as initilized, so it never gets initialized again.
    isInit = true;
    return;
}

template <typename DataType>
void Allocator<DataType>::threadInit(int threadNum)
{
    if (threadNum > pool.size() || threadNum < 0)
    {
        printf("Requested pool %d when %lu pools are allocated.\n", threadNum, pool.size());
        return;
    }
    // Give the thread a threadNum.
    // Multiple threads can have the same number, but access to the allocator shared between those threads is not thread safe.
    // The intention is that new threads can reuse the pool from old threads.
    Allocator<DataType>::threadNum = threadNum;
    return;
}

template <typename DataType>
DataType *Allocator<DataType>::alloc()
{
    if (threadNum == SIZE_MAX)
    {
        printf("A thread that did not initialize with the allocator has requested an object.\n");
        return NULL;
    }

    DataType *object = NULL;
    if (pool[threadNum]->size() > 0)
    {
        object = pool[threadNum]->back();
        pool[threadNum]->pop_back();
    }
    else
    {
        printf("Pool for thread %lu ran out of elements of size %lu.\n", threadNum, sizeof(DataType));
        object = new DataType();
    }
    if (object == NULL)
    {
        printf("Something bad happened!\n");
    }
    return object;
}

template <typename DataType>
void Allocator<DataType>::dealloc(DataType *object)
{
    // Reset the object.
    ::new (object) DataType();
    // Return the object to the pool.
    pool[threadNum]->push_back(object);
    return;

    // Old: Deallocate the object.
    //delete object;
}

template <typename DataType>
bool Allocator<DataType>::isInit = false;

template <typename DataType>
thread_local size_t Allocator<DataType>::threadNum = SIZE_MAX;

template <typename DataType>
std::vector<std::vector<DataType *> *> Allocator<DataType>::pool;

template class Allocator<Page>;
template class Allocator<RWOperation>;