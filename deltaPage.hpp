#ifndef DELTAPAGE_H
#define DELTAPAGE_H

#include <atomic>
#include <bitset>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <typeinfo>

#include "allocator.hpp"
#include "define.hpp"
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

// A delta update page.
// NOTE: Alignment actually has a small negative impact on read-only performance.
class /*alignas(64)*/ Page
{
  private:
	// A contiguous list of updated values.
	VAL newVal[SGMT_SIZE];
	// A contiguous list of old values.
	// We need these in case the associated transaction aborts.
	VAL oldVal[SGMT_SIZE];
	// Get the new or old value at the given index for the current page.
	VAL *at(size_t index, bool newVals);

  public:
	// Constructor that initializes a segment.
	Page();
	// The number of elements represented by each segment.
	// TUNE
	const static size_t SEG_SIZE = SGMT_SIZE;
	// A list of what modification types this transaction performs.
	Bitset<SEG_SIZE> bitset;
	// A pointer to the transaction associated with this page.
	Desc *transaction = NULL;
	// A pointer to the next page in the update list for this segment.
	Page *next = NULL;

	// Read the element from the page.
	bool get(size_t index, bool newVals, VAL &val);
	// Write the element into the page.
	bool set(size_t index, bool newVals, VAL val);

	// Copy some of the values from one page into this one.
	bool copyFrom(Page *page);

	// Print out the data stored in the page.
	void print();
};

#endif