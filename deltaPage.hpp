#ifndef DELTAPAGE_HPP
#define DELTAPAGE_HPP

#include <atomic>
#include <bitset>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <typeinfo>
#include <type_traits>

#include "allocator.hpp"
#include "define.hpp"
#include "transaction.hpp"

#define NEW_VAL 1
#define OLD_VAL 0

class Desc;

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
template <typename T, size_t S>
// NOTE: Alignment actually has a small negative impact on read-only performance.
class
#ifdef ALIGNED
	alignas(64)
#endif
		Page
{
private:
	// A contiguous list of updated values.
	T newVal[S];
	// A contiguous list of old values.
	// We need these in case the associated transaction aborts.
	T oldVal[S];
	// Get the new or old value at the given index for the current page.
	T *at(size_t index, bool newVals)
	{
		// Don't go out of bounds.
		if (index > Page<T, S>::SEG_SIZE)
		{
			return NULL;
		}
		// Used bits are any locations that are read from or written to.
		std::bitset<Page<T, S>::SEG_SIZE> usedBits =
			Page::bitset.read | Page::bitset.write;
		// If the bit isn't even in this page, we can't return a valid value.
		if (usedBits[index] != 1)
		{
			return NULL;
		}

		// Return the address of the position in question.
		// This way, we can change the value stored in the page, if needed.
		if (newVals)
		{
			return &newVal[index];
		}
		else
		{
			return &oldVal[index];
		}
	}

public:
	// Constructor that initializes a segment.
	Page()
	{
		for (size_t i = 0; i < S; i++)
		{
			// NOTE: UNSET is unsuitable for size pages.
			// This is not an issue because size pages are set to 0 after initialization.
			newVal[i] = UNSET;
			oldVal[i] = UNSET;
		}
		return;
	}
	// The number of elements represented by each segment.
	// TUNE
	const static size_t SEG_SIZE = S;
	// A list of what modification types this transaction performs.
	Bitset<SEG_SIZE> bitset;
	// A pointer to the transaction associated with this page.
	Desc *transaction = NULL;
	// A pointer to the next page in the update list for this segment.
	Page *next = NULL;

	// Read the element from the page.
	bool get(size_t index, bool newVals, T &val)
	{
		T *pointer = at(index, newVals);
		if (pointer == NULL)
		{
			return false;
		}
		val = *pointer;
		return true;
	}
	// Write the element into the page.
	bool set(size_t index, bool newVals, T val)
	{
		T *element = at(index, newVals);
		if (element == NULL)
		{
			return false;
		}
		*element = val;
		return true;
	}

	// Copy some of the values from one page into this one.
	bool copyFrom(Page *page)
	{
		this->bitset.read = page->bitset.read;
		this->bitset.write = page->bitset.write;
		this->bitset.checkBounds = page->bitset.checkBounds;
		this->transaction = page->transaction;
		// This will be set later. No need to copy it.
		//next = page->next;
		for (size_t i = 0; i < page->SEG_SIZE; i++)
		{
			this->newVal[i] = page->newVal[i];
			// This will be set later. No need to copy it.
			//this->oldVal[i] = page->oldVal[i];
		}
		return true;
	}

	// Print out the data stored in the page.
	void print()
	{
		std::cout << "SEG_SIZE \t= " << SEG_SIZE << std::endl;
		std::cout << "Bitset:" << std::endl;
		std::cout << "read \t\t= " << bitset.read.to_string() << std::endl;
		std::cout << "write \t\t= " << bitset.write.to_string() << std::endl;
		std::cout << "checkBounds \t= " << bitset.checkBounds.to_string() << std::endl;
		std::cout << "transaction \t= " << transaction << std::endl;
		std::cout << "next \t\t= " << next << std::endl;
		for (size_t i = 0; i < this->SEG_SIZE; i++)
		{
			std::cout << "oldVal[" << i << "] \t= " << std::setw(11) << oldVal[i] << "\t"
					  << "newVal[" << i << "] \t= " << std::setw(11) << newVal[i] << std::endl;
		}
		return;
	}
};

#endif