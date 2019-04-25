#ifndef TRANSVECTOR_H
#define TRANSVECTOR_H

#include <atomic>
#include <cstddef>
#include <map>
#include <assert.h>
#include "deltaPage.hpp"
#include "rwSet.hpp"
#include "segmentedVector.hpp"
#include "transaction.hpp"

template <class T>
class RWOperation;

template <class T>
class RWSet;

template <class T>
class TransactionalVector
{
  private:
	// An array of page pointers.
	SegmentedVector<Page<T, T, SGMT_SIZE> *> *array = NULL;

	// A generic ending page, used to get values.
	Page<T, T, SGMT_SIZE> *endPage = NULL;
	// A generic committed transaction.
	Desc<T> *endTransaction = NULL;

	bool reserve(size_t size);

	// Prepends a delta update on an existing page.
	// Only sets oldVal values and the next pointer here.
	bool prependPage(size_t index, Page<T, T, SGMT_SIZE> *page);

	// Takes in a set of pages and inserts them into our vector.
	// startPage is used in the helping scheme to start inserting at later pages.
	void insertPages(std::map<size_t, Page<T, T, SGMT_SIZE> *> pages, size_t startPage = 0);

  public:
	// A page holding our shared size variable.
	// Access is public because the RWSet must be able to change it.
	std::atomic<Page<size_t, T, 1> *> size;

	TransactionalVector();

	void getSize(Desc<T> *descriptor);

	Page<size_t, T, 1> *getSizePage(Desc<T> *descriptor);

	// Create a RWSet for the transaction.
	// Only used in helping on size conflict.
	RWSet<T> *prepareTransaction(Desc<T> *descriptor);

	// Finish the vector transaction.
	// Used for helping.
	bool completeTransaction(Desc<T> *descriptor, size_t startPage = 0, bool helping = false);

	// Apply a transaction to a vector.
	void executeTransaction(Desc<T> *descriptor);

	// Print out the values stored in the vector.
	void printContents();
};

#endif