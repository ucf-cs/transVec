#ifndef RWSET_HPP
#define RWSET_HPP

#include <atomic>
#include <cstddef>
#include <ostream>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "define.hpp"
#include "deltaPage.hpp"
#include "memAllocator.hpp"
#include "transaction.hpp"
#ifdef SEGMENTVEC
#include "transVector.hpp"
#endif
#ifdef COMPACTVEC
#include "compactVector.hpp"
#endif

class TransactionalVector;

class CompactVector;
class CompactElement;

class RWOperation;
class Operation;
class Desc;

#ifdef SEGMENTVEC
typedef MemAllocator<std::pair<size_t, Page<VAL, SGMT_SIZE> *>> MyPageAllocator;
typedef MemAllocator<std::pair<size_t, RWOperation *>> MySecondRWOpAllocator;
typedef MemAllocator<std::pair<size_t, std::map<size_t, RWOperation *, std::less<size_t>, MySecondRWOpAllocator>>> MyRWOpAllocator;
#endif

// An individual operation on a single element location.
struct RWOperation
{
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
#ifdef COMPACTVEC
	// Map vector locations to read/write operations.
	std::map<size_t,
			 RWOperation *,
			 std::less<size_t>,
			 MemAllocator<std::pair<size_t, RWOperation *>>>
		operations;
	// The descriptor associated with this set.
	Desc *descriptor = NULL;
	// Set this if size changes.
	unsigned int size = UINT32_MAX;
	// A replacement size element, used by this RWSet.
	CompactElement *sizeElement = NULL;
	// Return the index associated with a RW operation access.
	static size_t access(unsigned int pos);
	// Converts a transaction descriptor into a read/write set.
	bool createSet(Desc *descriptor, CompactVector *vector);
	unsigned int getSize(CompactVector *vector, Desc *descriptor = NULL);
	// Get an op node from a map. Allocate it if it doesn't already exist.
	bool getOp(RWOperation *&op, size_t index);
#endif
#ifdef SEGMENTVEC
	// Map vector locations to read/write operations.
	std::unordered_map<size_t,
					   std::array<RWOperation *, SGMT_SIZE>,
					   std::hash<size_t>,
					   std::equal_to<size_t>,
					   MemAllocator<std::pair<size_t, std::array<RWOperation *, SGMT_SIZE>>>>
		operations;
	// Our size descriptor. After reading size, we use this to write a new size value later.
	std::atomic<Page<size_t, 1> *> sizeDesc;
	// Set this if size changes.
	size_t size = 0;
	// Return the indexes associated with a RW operation access.
	static std::pair<size_t, size_t> access(size_t pos);
	// Converts a transaction descriptor into a read/write set.
	bool createSet(Desc *descriptor, TransactionalVector *vector);
	// Convert from a set of reads and writes to a list of pages.
	// A pointer to the pages is stored in the descriptor.
	void setToPages(Desc *descriptor);
	size_t getSize(TransactionalVector *sizeHead, Desc *transaction);
	// Get an op node from a map. Allocate it if it doesn't already exist.
	void getOp(RWOperation *&op, std::pair<size_t, size_t> indexes);
	// Print out a list of all locations with operations associated with them.
	void printOps();
#endif
	// An absolute reserve position.
	size_t maxReserveAbsolute = 0;
	~RWSet();
};

#endif