#ifndef COMPACT_VECTOR_HPP
#define COMPACT_VECTOR_HPP

#include <assert.h>
#include <atomic>
#include <map>

#include "allocator.hpp"
#include "define.hpp"
#include "rwSet.hpp"
#include "segmentedVector.hpp"
#include "transaction.hpp"

#ifdef COMPACTVEC

class RWOperation;
class RWSet;
template <typename T>
class SegmentedVector;
class Desc;

struct alignas(16) CompactElement
{
    // NOTE: Use bit fields here if needed.
    VAL oldVal = UNSET;
    VAL newVal = UNSET;
    Desc *descriptor = NULL;
    CompactElement() noexcept;
    void print();
};

class CompactVector
{
private:
    // An array of compact elements.
    SegmentedVector<CompactElement> *array = NULL;
    // A generic, committed transaction.
    // This is used to resolve uninitialized pages.
    Desc *endTransaction = NULL;
    // Reserve simply passes the request along to the underlying segmented vector.
    bool reserve(size_t size);
    // Performs an atomic 16 byte exchange of an element.
    bool updateElement(size_t index, CompactElement &newElem);
    // Insert the elements in the set.
    void insertElements(RWSet *set, bool helping = false, unsigned int startElement = UINT32_MAX);
    // Create a RWSet for the transaction.
    // Only used in helping on size conflict.
    bool prepareTransaction(Desc *descriptor);
    // Finish the vector transaction.
    // Used for helping.
    bool completeTransaction(Desc *descriptor, bool helping = false, unsigned int startElement = UINT32_MAX);

public:
    // A page holding our shared size variable.
    // Access is public because the RWSet must be able to change it.
    std::atomic<CompactElement> size;
    // Default constructor.
    CompactVector();
    // Apply a transaction to a vector.
    void executeTransaction(Desc *descriptor);
    // Called if a transaction is blocking on size.
    void sizeHelp(Desc *descriptor);
    // Print out the values stored in the vector.
    void printContents();
};

#endif
#endif