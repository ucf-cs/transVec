#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <atomic>
#include <vector>

#include "deltaPage.hpp"
#include "define.hpp"

#define POOL_SIZE 10000000
//#define POOL_SIZE 0

template <class T, class U, size_t S>
class Page;
template <class T, class U, size_t S, size_t V>
class DeltaPage;

template <class T, class U, size_t S>
class Allocator
{
  private:
    thread_local static std::vector<DeltaPage<size_t, VAL, 1, 1> *> poolSize;

    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 1> *> pool1;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 2> *> pool2;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 3> *> pool3;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 4> *> pool4;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 5> *> pool5;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 6> *> pool6;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 7> *> pool7;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 8> *> pool8;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 9> *> pool9;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 10> *> pool10;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 11> *> pool11;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 12> *> pool12;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 13> *> pool13;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 14> *> pool14;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 15> *> pool15;
    thread_local static std::vector<DeltaPage<VAL, VAL, SGMT_SIZE, 16> *> pool16;

  public:
    // Initialize the allocator pool.
    static void initPool();
    // Get a delta page with the appropriate size.
    static Page<T, U, S> *getNewPage(size_t size = S);
    // Return a page that is no longer needed into the pool.
    static void freePage(Page<T, U, S> *page);
};

#endif