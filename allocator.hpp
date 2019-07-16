#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <assert.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "define.hpp"

// Contains classes preallocated by the Allocator.
#include "deltaPage.hpp"
#include "rwSet.hpp"

template <class T, size_t S>
class Page;

class RWOperation;

template <typename DataType>
class Allocator
{
public:
  static bool isInit;
  // Lock used to improve memory allocation rate.
  //static std::mutex allocLock;
  thread_local static size_t threadNum;
  // The pool of objects.
  static std::vector<std::vector<DataType *> *> pool;
  // DEBUG: A counter used to keep track of allocations. Use this to tune allocation sizes.
  static std::atomic<size_t> count;

  // Initialize the allocator.
  static void init(size_t poolSize = 0)
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
  // Initialize the thread-local allocator pool.
  static void threadInit(int threadNum = -1)
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
  // Get a delta page with the appropriate size.
  static DataType *alloc()
  {
    // DEBUG: Allocation counter.
    Allocator<DataType>::count.fetch_add(1);

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
  // Return a page that is no longer needed into the pool.
  static void dealloc(DataType *object)
  {
    // Reset the object.
    ::new (object) DataType();
    // Return the object to the pool.
    pool[threadNum]->push_back(object);
    return;

    // Old: Deallocate the object.
    //delete object;
  }
  // Report how many times the pool was used.
  static void report()
  {
    size_t count = Allocator<DataType>::count.load();
    //printf("Used %lu allocations in this pool of %lu byte objects. About %lu allocations per thread\n", count, sizeof(DataType), count / THREAD_COUNT);
    return;
  }
};

template <typename DataType>
bool Allocator<DataType>::isInit = false;

template <typename DataType>
thread_local size_t Allocator<DataType>::threadNum = SIZE_MAX;

template <typename DataType>
std::vector<std::vector<DataType *> *> Allocator<DataType>::pool;

template <typename DataType>
std::atomic<size_t> Allocator<DataType>::count(0);

void threadAllocatorInit(int threadNum);

void allocatorInit();

void allocatorReport();

#endif