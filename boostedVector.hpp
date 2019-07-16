#ifndef BOOSTED_VECTOR_HPP
#define BOOSTED_VECTOR_HPP

#include <assert.h>
#include <atomic>
#include <map>
#include <mutex>

#include "allocator.hpp"
#include "define.hpp"
#include "rwSet.hpp"
#include "segmentedVector.hpp"
#include "transaction.hpp"

#ifdef BOOSTEDVEC

class RWOperation;
class RWSet;
template <typename T>
class SegmentedVector;
class Desc;

struct BoostedElement
{
    std::mutex lock;
    VAL val = UNSET;
    //BoostedElement() noexcept;
    void print();
};

class BoostedVector
{
private:
    // An array of compact elements.
    SegmentedVector<BoostedElement> *array = NULL;
    // A generic, committed transaction.
    // This is used to resolve uninitialized pages.
    Desc *endTransaction = NULL;
    // Reserve simply passes the request along to the underlying segmented vector.
    bool reserve(size_t size);
    // Locks and updates an element.
    bool updateElement(size_t index, VAL newElem);
    // Insert the elements in the set.
    bool insertElements(Desc *descriptor);
    // Create a RWSet for the transaction.
    bool prepareTransaction(Desc *descriptor);

public:
    // The vector's shared size variable.
    // Access is public because the RWSet must be able to change it.
    size_t size;
    // NOTE: Only use the lock here, never the value.
    BoostedElement sizeLock;
    // Default constructor.
    BoostedVector();
    // Apply a transaction to a vector.
    bool executeTransaction(Desc *descriptor);
    // Print out the values stored in the vector.
    void printContents();
};

#endif
#endif