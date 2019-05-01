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
VAL, VAL, SGMT_SIZE, 1
// T: Type of data to store in the page.
// U: Type of transaction to link to.
// S: The full size of the given segment.
template <class T, class U, size_t S>
// A delta update page.
// NOTE: Alignment actually has a small negative impact on read-only performance.
class /*alignas(64)*/ Page
{
  public:
	// The number of elements represented by each segment.
	// TUNE
	const static size_t SEG_SIZE = S;
	// The number of elements being updated by this page.
	const static size_t SIZE = S;
	// A list of what modification types this transaction performs.
	Bitset<SEG_SIZE> bitset;
	// A pointer to the transaction associated with this page.
	Desc<U> *transaction = NULL;
	// A pointer to the next page in the delta update list for this segment.
	Page *next = NULL;

	virtual bool get(size_t index, bool newVals, T &val) = 0;
	virtual bool set(size_t index, bool newVals, T val) = 0;

	// Returns the size of the page. 
	virtual size_t getSize();

	// Print out the data stored in the page.
	void print();
};

template <class T, class U, size_t S, size_t V>
class /*alignas(64)*/ DeltaPage : public Page<T, U, S>
{
  private:
	// A contiguous list of updated values.
	T newVal[V];
	// A contiguous list of old values.
	// We need these in case the associated transaction aborts.
	T oldVal[V];
	// Get the new or old value at the given index for the current page.
	T *at(size_t index, bool newVals);

  public:
	// The number of elements being updated by this page.
	const static size_t SIZE = V;

	// Constructor that initializes a segment.
	DeltaPage();

	// Read the element from the page.
	bool get(size_t index, bool newVals, T &val);
	// Write the element into the page.
	bool set(size_t index, bool newVals, T val);

	// Returns the size of the page. 
	size_t getSize();

	// Copy some of the values from one page into this one.
	bool copyFrom(DeltaPage<T, U, S, V> *page);

	// Print out the data stored in the page.
	void print();
};

#endif