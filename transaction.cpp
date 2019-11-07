#include "transaction.hpp"

Desc::Desc(unsigned int size, Operation *ops)
{
	this->size = size;
	this->ops = ops;
#ifndef BOOSTEDVEC
	// Transactions are always active at start.
	status.store(active);
	// The RWSet always starts out empty.
	set.store(NULL);
#else
	set = NULL;
#endif
	return;
}

Desc::~Desc()
{
	//delete ops;
	return;
}

VAL *Desc::getResult(size_t index)
{
	// If we request a result at an invalid operation index.
	if (index >= size)
	{
		return NULL;
	}
#ifndef BOOSTEDVEC
	// If the transaction has not yet committed.
	if (status.load() != committed)
	{
		return NULL;
	}
#endif
	// Return the value returned by the operation.
	return &(ops[index].ret);
}

#ifndef BOOSTEDVEC
void Desc::print()
{
	const char *statusStrList[] = {"active", "committed", "aborted"};
	size_t statusStrIndex = 0;
	switch (status.load())
	{
	case active:
		statusStrIndex = 0;
		break;
	case committed:
		statusStrIndex = 1;
		break;
	case aborted:
		statusStrIndex = 2;
		break;
	}
	printf("Transaction status:\t%s\n", statusStrList[statusStrIndex]);

	printf("Ops: (size=%u)\n", size);
	for (size_t i = 0; i < size; i++)
	{
		ops[i].print();
		printf("\n");
	}
}
#endif

void Operation::print()
{
	const char *typeStrList[] = {"pushBack", "popBack", "reserve", "read", "write", "size"};
	size_t typeStrIndex = 0;
	switch (type)
	{
	case pushBack:
		typeStrIndex = 0;
		break;
	case popBack:
		typeStrIndex = 1;
		break;
	case reserve:
		typeStrIndex = 2;
		break;
	case read:
		typeStrIndex = 3;
		break;
	case write:
		typeStrIndex = 4;
		break;
	case size:
		typeStrIndex = 5;
		break;
	}
	std::cout << "Type:\t" << typeStrList[typeStrIndex] << std::endl;
	std::cout << "index:\t" << index << std::endl;
	std::cout << "val:\t" << val << std::endl;
	std::cout << "ret:\t" << ret << std::endl;
}
