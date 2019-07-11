/*
This vector uses a 2-level array to store element.
While this does result in a maximum size, it should be infeasible to reach with enough buckets.
This implementation, by itself, is a lock-free vector, but it doesn't support push, pop, or size.
It also has no support for transactions.
We build off of this implementation to add support for those things.
*/
#ifndef SEGMENTEDVECTOR_HPP
#define SEGMENTEDVECTOR_HPP

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
	size_t firstBucketSize = 2;
// The array containing pointers to our array segments.
#ifndef BOOSTEDVEC
	// Pointer to array of atomic pointers to atomic generic type.
	std::atomic<std::atomic<T> *> *bucketArray;
#else
	std::atomic<T *> *bucketArray;
#endif

	// The maximum bit returnable for the highest bit.
	const size_t max = 8 * sizeof(size_t) - 1;
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
	// Initializes array segments as needed to meet capacity demands.
	// Must be safe to execute in a lock-free manner.
	// Pass in the highest index required for the operation and everything else will be properly allocated.
	// Call this at the beginning of each transaction to ensure space has been properly allocated.
	bool reserve(size_t size);

// Atomic read at a target location.
#ifndef BOOSTEDVEC
	bool read(size_t index, T &value);
#else
	bool read(size_t index, T *value);
#endif
#ifndef BOOSTEDVEC
	// Atomic write at a target location.
	bool write(size_t index, T val);
	// Atomic CAS at a target location.
	bool tryWrite(size_t index, T oldVal, T newVal);
#endif
	// Print out all of the buckets.
	// They should all initially be NULL.
	void printBuckets();
};

#endif