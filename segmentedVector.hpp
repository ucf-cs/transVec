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
    size_t highestBit(size_t val)
    {
        // Subtract 1 so the rightmost position is 0 instead of 1.
        return (sizeof(val) * 8) - __builtin_clz(val | 1) - 1;
    }
    // Retrieves the bucket.
    size_t getBucket(size_t hiBit)
    {
        // Subtract the first bucket's highest bit to offset for initial bucket size.
        return hiBit - highestBit(firstBucketSize);
    }
    // Retrieves the index.
    size_t getIdx(size_t pos, size_t hiBit)
    {
        // Removes the leftmost 1 from our position.
        // The remaining value is the index.
        return (pos) ^ (1 << (hiBit));
    }

    // Allocate a specific bucket.
    bool allocBucket(size_t bucket)
    {
        // Cannot allocate buckets beyond the number we allocated at the beginning.
        if (bucket > buckets)
        {
            return false;
        }
        // The size of the bucket we are allocating.
        // firstBucketSize^(bucket+1)
        size_t bucketSize = 1 << (highestBit(firstBucketSize) * (bucket + 1));
        // Allocate the new segment.
        std::atomic<T> *mem = new std::atomic<T>[bucketSize]();
        // We need to initialize the NULL pointer if we want to CAS.
        std::atomic<T> *null = NULL;
        // Attempt to place the segment in shared memory.
        if (!bucketArray[bucket].compare_exchange_strong(null, mem))
        {
            // Another thread already allocated this segment.
            // Deallocate the memory allocated by this thread.
            delete[] mem;
        }
        return true;
    }

    // Return the bucket and element index associated with an access.
    std::pair<size_t, size_t> access(size_t index)
    {
        // Determine the first and second dimensions that the element is contained in.
        // Add firstBucketSize to offset our results properly.
        size_t pos = index + firstBucketSize;
        // The highest bit is used to determine the bucket and index of our element.
        size_t hiBit = highestBit(pos);
        // Get the bucket asociated with this element.
        size_t bucket = getBucket(hiBit);
        // Get the index associated with this element.
        size_t idx = getIdx(pos, hiBit);
        // Ensure that the first dimension has allocated an array in the second dimension.
        // We don't call this, since we guarantee that all transactions call reserve before running the rest of their operations.
        //reserve(index);
        // Return the array indexes.
        return std::make_pair(bucket, idx);
    }

  public:
    SegmentedVector()
    {
        // Initialize the first level of the array.
        bucketArray = new std::atomic<std::atomic<T> *>[buckets]();
        // Initialize the first segment in the second level.
        reserve(1);
        return;
    }
    SegmentedVector(size_t capacity)
    {
        // The first bucket size should be the smallest power of 2 that can hold capacity elements.
        firstBucketSize = 1 << (highestBit(capacity) + 1);
        SegmentedVector();
        return;
    }
    // Should only be performed by a single thread once all other threads are done with the vector.
    ~SegmentedVector()
    {
        // Deallocate every bucket.
        for (size_t i = 0; i < buckets; i++)
        {
            // Determine the number of elements in this bucket.
            size_t bucketSize = 1 << (highestBit(firstBucketSize) * (i + 1));
            // Determine whether or not this bucket is initialized.
            std::atomic<T> *bucket = bucketArray[i].load();
            if (bucket != NULL)
            {
                // Deallocate every element in the bucket.
                for (size_t j = 0; j < bucketSize; j++)
                {
                    T *element = bucket[j].load();
                    if (element != NULL)
                    {
                        // Note: Element should have its own deconstructor, if needed.
                        delete element;
                    }
                }
                delete[] bucket;
            }
        }
        // Deallocate the first level of the array itself.
        delete[] bucketArray;
    }
    // Initializes array segments as needed to meet capacity demands.
    // Must be safe to execute in a lock-free manner.
    // Pass in the highest index required for the operation and everything else will be properly allocated.
    // Call this at the beginning of each transaction to ensure space has been properly allocated.
    bool reserve(size_t size)
    {
        // Determine what bucket needs to be allocated to fit an element in this position.
        size_t targetBucket = getBucket(highestBit(size + firstBucketSize));
        // Check to see if the highest bucket is already allocated.
        if (bucketArray[targetBucket].load() != NULL)
        {
            // If so, we do nothing.
            return true;
        }
        // Otherwise, we need to allocate it, and all previous buckets if they aren't already.
        // Allocate from low to high indexes.
        // This allows us to assume all previous buckets have been allocated by checking only if the highest has been allocated.
        for (size_t i = 0; i <= targetBucket; i++)
        {
            if (!allocBucket(i))
            {
                // If bucket allocation failed.
                return false;
            }
        }
        return true;
    }

    // Atomic read at a target location.
    T read(size_t index)
    {
        std::pair<size_t, size_t> indexes = access(index);
        return bucketArray[indexes.first][indexes.second].load();
    }
    // Atomic write at a target location.
    void write(size_t index, T val)
    {
        std::pair<size_t, size_t> indexes = access(index);
        return bucketArray[indexes.first][indexes.second].store(val);
    }
    // Atomic CAS at a target location.
    bool tryWrite(size_t index, T oldVal, T newVal)
    {
        std::pair<size_t, size_t> indexes = access(index);
        return bucketArray[indexes.first][indexes.second].compare_exchange_strong(oldVal, newVal);
    }
};

#endif