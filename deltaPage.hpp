#ifndef DELTAPAGE_H
#define DELTAPAGE_H

#include <atomic>
#include <bitset>
#include <cstddef>
#include "transaction.hpp"

#define NEW_VALS 1
#define OLD_VALS 0

// We can use bitsets of arbitrary size, so long as we decide at compile time.
template <size_t size>
// This bitset keeps track of whether or not an element was read from or written to by this transaction.
struct Bitset
{
    std::bitset<size> read;
    std::bitset<size> write;
    // TODO: Do we need to seperate this between read and write bounds checking?
    std::bitset<size> checkBounds;
};

template <class T>
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
    size_t size;
    // A list of what modification types this transaction performs.
    Bitset<SEG_SIZE> bitset;
    // A pointer to the transaction associated with this page.
    Desc<T> *transaction;
    // A pointer to the next page in the delta update list for this segment.
    Page *next;

    // Constructor that initializes a segment based on desired length.
    // Default assumes the whole segment is replaced, rather than just some of them.
    Page(size_t size = SEG_SIZE)
    {
        this->size = size;
        newVal = new T[size];
        oldVal = new T[size];

        next = NULL;
        this->transaction = NULL;

        return;
    }
    // Always deallocate our dynamic memory.
    ~Page()
    {
        if (newVal != NULL)
        {
            delete newVal;
        }
        if (oldVal != NULL)
        {
            delete oldVal;
        }
    }

    // Get the new or old value at the given index for the current page.
    T *at(size_t index, bool newVals = true)
    {
        // Don't go out of bounds.
        if (index > SEG_SIZE)
        {
            return NULL;
        }
        // Used bits are any locations that are read from or written to.
        std::bitset<SEG_SIZE> usedBits = bitset.read | bitset.write;
        // If the bit isn't even in this page, we can't return a valid value.
        if (usedBits[index] != 1)
        {
            return NULL;
        }
        // Get the number of bits before the bit at our target index.
        size_t pos = 0;
        for (size_t i = 0; i < index; i++)
        {
            if (usedBits[i])
            {
                pos++;
            }
        }
        // Return the address of the position in question.
        // This way, we can change the value stored in the page, if needed.
        return newVals ? &newVal[pos] : &oldVal[pos];
    }
};

#endif