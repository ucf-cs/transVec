#ifndef RWSET_H
#define RWSET_H

#include <atomic>
#include <cstddef>
#include <map>
#include <vector>
#include "deltaPage.hpp"
#include "transaction.hpp"
#include "transVector.hpp"

template <class T>
class TransactionalVector;

// An individual operation on a single element location.
template <class T>
class RWOperation
{
  public:
	typedef enum Assigned
	{
		yes,
		no,
		unset
	} Assigned;
	// Set to no only if we know the current size of our vector.
	// Yes means we need to check if the element contains an UNSET.
	// Unset means we don't need to check bounds, as an operation never touched this element.
	// This happens only if our first operation in this spot was a read or write.
	Assigned checkBounds = unset;

	// Keep track of the last write operation to handle internally matching pops and reads from this location.
	// If this isn't NULL, we can infer a write for our page's bitset.
	Operation<T> *lastWriteOp = NULL;
	// Keep a list of operations that want to read the old value.
	// If this isn't empty, we can infer a read for our page's bitset.
	std::vector<Operation<T> *> readList;
};

// All transactions are converted into a read/write set before modifying the vector.
template <class T>
class RWSet
{
  public:
	// Map vector locations to read/write operations.
	// Used for absolute reads/writes.
	std::map<size_t, std::map<size_t, RWOperation<T>>> operations;
	// An absolute reserve position.
	size_t maxReserveAbsolute = 0;
	// Our size descriptor. After reading size, we use this to write a new size value later.
	Page<size_t, T, 1> *sizeDesc = NULL;
	// Set this if size changes.
	size_t size = 0;

	// Map vector operations to pushes and pops relative to size.
	// Read size, then they can be resolved to absolute indexes.
	//RWOperation<T> *operationsList;
	// A relative reserve position.
	//signed long int maxReserveRelative = 0;

	~RWSet();

	// Return the indexes associated with a RW operation access.
	std::pair<size_t, size_t> access(size_t pos);

	// Converts a transaction descriptor into a read/write set.
	bool createSet(Desc<T> *descriptor, TransactionalVector<T> *vector);

	// Convert from a set of reads and writes to a list of pages.
	std::map<size_t, Page<T, T, 8> *> setToPages(Desc<T> *descriptor);

	// Set the values in a transaction to the retrived values.
	void setOperationVals(Desc<T> *descriptor, std::map<size_t, Page<T, T, 8> *> *pages);

	size_t getSize(Desc<T> *transaction, TransactionalVector<T> *sizeHead);
};

#endif