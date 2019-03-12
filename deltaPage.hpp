#ifndef DELTAPAGE_H
#define DELTAPAGE_H

#include <atomic>
#include <bitset>
#include <cstddef>
#include "transaction.hpp"

#define NEW_VAL 1
#define OLD_VAL 0

// We can use bitsets of arbitrary size, so long as we decide at compile time.
template <size_t size>
// This bitset keeps track of whether or not an element was read from or written to by this transaction.
struct Bitset
{
    std::bitset<size> read;
    std::bitset<size> write;
    std::bitset<size> checkBounds;
};

// T: Type of data to store in the page.
// U: Type of transaction to link to.
template <class T, class U>
// A delta update page.
class Page
{
  private:
    // A contiguous list of updated values, allocated dynamically on page generation.
    T *newVal;
    // A contiguous list of old values, allocated dynamically on page generation.
    // We need these in case the associated transaction aborts.
    T *oldVal;

  public:
    // The number of elements represented by each segment.
    // TUNE
    const static size_t SEG_SIZE = 8;
    // The number of elements being updated by this page.
    size_t size = 0;
    // A list of what modification types this transaction performs.
    Bitset<SEG_SIZE> bitset;
    // A pointer to the transaction associated with this page.
    Desc<U> *transaction = NULL;
    // A pointer to the next page in the delta update list for this segment.
    Page *next = NULL;

    // Constructor that initializes a segment based on desired length.
    // Default assumes the whole segment is replaced, rather than just some of them.
    Page(size_t size = SEG_SIZE);
    // Always deallocate our dynamic memory.
    ~Page();

    // Get the new or old value at the given index for the current page.
    T *at(size_t index, bool newVals = true);
};

#endif