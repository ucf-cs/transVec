#ifndef COMPACT_VECTOR_HPP
#define COMPACT_VECTOR_HPP

#include <assert.h>
#include <atomic>
#include <cstdarg>
#include <map>
#include <set>

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
    // This is the actual array.
    SegmentedVector<CompactElement> *array = NULL;
    // A generic, committed transaction.
    // This is used to resolve uninitialized pages.
    Desc *endTransaction = NULL;
    // Reserve simply passes the request along to the underlying segmented vector.
    bool reserve(size_t size);
    // Performs an atomic 16 byte exchange of an element.
    VAL updateElement(size_t index, CompactElement &newElem, Operation::OpType type, bool checkSize);
    // Finish the vector transaction.
    // Used for helping.
    bool helpTransaction(Desc *descriptor);

public:
    // A page holding our shared size variable.
    // Access is public because the RWSet must be able to change it.
    std::atomic<CompactElement> size;
    // Default constructor.
    CompactVector();
    // Apply a transaction to a vector.
    bool executeTransaction(bool (*func)(Desc *desc, valMap *localMap), valMap *inMap, valMap *outMap, size_t size);
    // Print out the values stored in the vector.
    void printContents();
    // Run an operation on a data structure.
    // Called from within a transaction function.
    callOp(Desc *desc, Operation::OpType type, void *args...);
};

#endif
#endif