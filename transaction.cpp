#include "transaction.hpp"

Desc::Desc(bool (*func)(Desc *desc, valMap *localMap), valMap *inMap, valMap *outMap, size_t size)
{
#ifndef BOOSTEDVEC
	// Transactions are always active at start.
	status.store(active);

	this->func = func;
	inputMap = inMap;
	outputMap = outMap;
	returnValues = new std::atomic<int> [size];
#else
	set = NULL;
#endif
	return;
}

Desc::~Desc()
{
	delete[] returnValues;
	return;
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
}
