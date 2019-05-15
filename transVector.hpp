#ifndef TRANSVECTOR_HPP
#define TRANSVECTOR_HPP

#include <assert.h>
#include <atomic>
#include <cstddef>
#include <map>
#include <ostream>

#include "allocator.hpp"
#include "define.hpp"
#include "deltaPage.hpp"
#include "rwSet.hpp"
#include "segmentedVector.hpp"
#include "transaction.hpp"

class RWOperation;
class RWSet;

template <class T, size_t S>
class Page;
template <typename T>
class SegmentedVector;
class Desc;

class TransactionalVector
{
private:
	// An array of page pointers.
	SegmentedVector<Page<VAL, SGMT_SIZE> *> *array = NULL;

	// A generic ending page, used to get values.
	Page<VAL, SGMT_SIZE> *endPage = NULL;
	// A generic committed transaction.
	Desc *endTransaction = NULL;

	bool reserve(size_t size);

	// Prepends a delta update on an existing page.
	// Only sets oldVal values and the next pointer here.
	bool prependPage(size_t index, Page<VAL, SGMT_SIZE> *page);

	// Takes in a set of pages and inserts them into our vector.
	// startPage is used in the helping scheme to start inserting at later pages.
	void insertPages(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MemAllocator<std::pair<size_t, Page<VAL, SGMT_SIZE> *>>> *pages, bool helping = false, size_t startPage = SIZE_MAX);

public:
	// A page holding our shared size variable.
	// Access is public because the RWSet must be able to change it.
	std::atomic<Page<size_t, 1> *> size;
	// Default contructor.
	TransactionalVector();
	// Create a RWSet for the transaction.
	// Only used in helping on size conflict.
	void prepareTransaction(Desc *descriptor);
	// Finish the vector transaction.
	// Used for helping.
	bool completeTransaction(Desc *descriptor, bool helping = false, size_t startPage = SIZE_MAX);
	// Apply a transaction to a vector.
	void executeTransaction(Desc *descriptor);
	// Called if a transaction is blocking on size.
	void sizeHelp(Desc *descriptor);
	// Print out the values stored in the vector.
	void printContents();
};

// Only needed if running C++ 2011 or older.
// Should not use on C++ 2014 or later, since it will conflict with a built-in function.
#if __cplusplus < 201402L
template <class Iter>
constexpr std::reverse_iterator<Iter> make_reverse_iterator(Iter i)
{
	return std::reverse_iterator<Iter>(i);
}
#endif

#endif