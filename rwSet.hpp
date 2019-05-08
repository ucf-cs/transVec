#ifndef RWSET_H
#define RWSET_H

#include <atomic>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "define.hpp"
#include "deltaPage.hpp"
#include "memAllocator.hpp"
#include "transaction.hpp"
#include "transVector.hpp"

class TransactionalVector;
class RWOperation;

typedef MemAllocator<std::pair<size_t, Page *>> MyPageAllocator;
typedef MemAllocator<std::pair<size_t, RWOperation *>> MySecondRWOpAllocator;
typedef MemAllocator<std::pair<size_t, std::map<size_t, RWOperation *, std::less<size_t>, MySecondRWOpAllocator>>> MyRWOpAllocator;

// An individual operation on a single element location.
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
	Operation *lastWriteOp = NULL;
	// Keep a list of operations that want to read the old value.
	// If this isn't empty, we can infer a read for our page's bitset.
	std::vector<Operation *, MemAllocator<Operation *>> readList;
};

// All transactions are converted into a read/write set before modifying the vector.
class RWSet
{
  public:
	// Map vector locations to read/write operations.
	// Used for absolute reads/writes.
	std::unordered_map<size_t,
					   std::array<RWOperation *, SGMT_SIZE>,
					   std::hash<size_t>,
					   std::equal_to<size_t>,
					   MemAllocator<std::pair<const size_t, std::array<RWOperation *, SGMT_SIZE>>>>
		operations;
	// Old map of maps approach.
	//std::map<size_t, std::map<size_t, RWOperation *, std::less<size_t>, MySecondRWOpAllocator>, std::less<size_t>, MyRWOpAllocator> operations;

	// An absolute reserve position.
	size_t maxReserveAbsolute = 0;
	// Our size descriptor. After reading size, we use this to write a new size value later.
	Page *sizeDesc = NULL;
	// Set this if size changes.
	// TODO: Make size a size_t instead.
	VAL size = 0;

	// Map vector operations to pushes and pops relative to size.
	// Read size, then they can be resolved to absolute indexes.
	//RWOperation *operationsList;
	// A relative reserve position.
	//signed long int maxReserveRelative = 0;

	~RWSet();

	// Return the indexes associated with a RW operation access.
	std::pair<size_t, size_t> access(size_t pos);

	// Converts a transaction descriptor into a read/write set.
	bool createSet(Desc *descriptor, TransactionalVector *vector);

	// Convert from a set of reads and writes to a list of pages.
	// A pointer to the pages is stored in the descriptor.
	void setToPages(Desc *descriptor);

	// Set the values in a transaction to the retrived values.
	void setOperationVals(Desc *descriptor, std::map<size_t, Page *, std::less<size_t>, MyPageAllocator> pages);

	size_t getSize(Desc *transaction, TransactionalVector *sizeHead);

	// Get an op node from a map. Allocate it if it doesn't already exist.
	void getOp(RWOperation *&op, std::pair<size_t, size_t> indexes);
};

#endif