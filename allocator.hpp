#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <assert.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "define.hpp"

// Contains classes preallocated by the Allocator.
#include "deltaPage.hpp"
#include "rwSet.hpp"

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

  // Initialize the allocator.
  static void init(size_t poolSize = 0);
  // Initialize the thread-local allocator pool.
  static void threadInit(int threadNum = -1);
  // Get a delta page with the appropriate size.
  static DataType *alloc();
  // Return a page that is no longer needed into the pool.
  static void dealloc(DataType *object);
};

#endif