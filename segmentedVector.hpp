/*
This vector uses a 2-level array to store element. 
While this does result in a maximum size, it should be infeasible to reach with enough buckets.
This implementation, by itself, is a lock-free vector, but it doesn't support push, pop, or size.
It also has no support for transactions.
We build off of this implementation to add support for those things.
*/
#ifndef SEGMENTEDVECTOR_H
#define SEGMENTEDVECTOR_H

#include <atomic>
#include <cstddef>
#include <utility>

// Included so our template knows what a page is.
#include "deltaPage.hpp"

template <class T>
class SegmentedVector
{
  private:
    // The maximum number of buckets that can be allocated.
    // Increase or decrease this as needed.
    // TUNE
    size_t buckets = 256;
    // An intial bucket size.
    // Can be set higher if we know how many elements we're expecting.
    // Must be a power of 2.
    // TUNE
    size_t firstBucketSize = 1;
    // The array containing pointers to our array segments.
    // Pointer to array of atomic pointers to atomic generic type.
    std::atomic<std::atomic<T> *> *bucketArray;

    // Returns the position of the highest bit used in the binary representation of val.
    size_t highestBit(size_t val);
    // Retrieves the bucket.
    size_t getBucket(size_t hiBit);
    // Retrieves the index.
    size_t getIdx(size_t pos, size_t hiBit);

    // Allocate a specific bucket.
    bool allocBucket(size_t bucket);

    // Return the bucket and element index associated with an access.
    std::pair<size_t, size_t> access(size_t index);

  public:
    SegmentedVector();
    SegmentedVector(size_t capacity);
    // Should only be performed by a single thread once all other threads are done with the vector.
    ~SegmentedVector();
    // Initializes array segments as needed to meet capacity demands.
    // Must be safe to execute in a lock-free manner.
    // Pass in the highest index required for the operation and everything else will be properly allocated.
    // Call this at the beginning of each transaction to ensure space has been properly allocated.
    bool reserve(size_t size);

    // Atomic read at a target location.
    T read(size_t index);
    // Atomic write at a target location.
    void write(size_t index, T val);
    // Atomic CAS at a target location.
    bool tryWrite(size_t index, T oldVal, T newVal);
};

#endif