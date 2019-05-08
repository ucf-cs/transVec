#ifndef TRANSVECTOR_H
#define TRANSVECTOR_H

#include <assert.h>
#include <atomic>
#include <cstddef>
#include <map>

#include "allocator.hpp"
#include "deltaPage.hpp"
#include "rwSet.hpp"
#include "segmentedVector.hpp"
#include "transaction.hpp"

class RWOperation;
class RWSet;

template <typename T>
class SegmentedVector;

class TransactionalVector
{
private:
	// An array of page pointers.
	SegmentedVector<Page *> *array = NULL;

	// A generic ending page, used to get values.
	Page *endPage = NULL;
	// A generic committed transaction.
	Desc *endTransaction = NULL;

	bool reserve(size_t size);

	// Prepends a delta update on an existing page.
	// Only sets oldVal values and the next pointer here.
	bool prependPage(size_t index, Page *page);

	// Takes in a set of pages and inserts them into our vector.
	// startPage is used in the helping scheme to start inserting at later pages.
	void insertPages(std::map<size_t, Page *, std::less<size_t>, MemAllocator<std::pair<size_t, Page *>>> pages, bool helping = false, size_t startPage = SIZE_MAX);

public:
	// A page holding our shared size variable.
	// Access is public because the RWSet must be able to change it.
	std::atomic<Page *> size;

	TransactionalVector();

	void getSize(Desc *descriptor);

	Page *getSizePage(Desc *descriptor);

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

/*
template< class Iter >
constexpr std::reverse_iterator<Iter> make_reverse_iterator( Iter i )
{
    return std::reverse_iterator<Iter>(i);
}
*/

#endif