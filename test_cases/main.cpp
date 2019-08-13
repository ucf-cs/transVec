// This file includes methods that will be shared between all test cases

#include "main.hpp"

// All global variables are initialized here
std::vector<Desc *> *transactions = new std::vector<Desc *>();
#ifdef SEGMENTVEC
TransactionalVector *transVector = new TransactionalVector();
#endif
#ifdef COMPACTVEC
CompactVector *transVector = new CompactVector();
#endif
#ifdef BOOSTEDVEC
BoostedVector *transVector = new BoostedVector();
std::atomic<size_t> abortCount(0);
#endif
#ifdef STMVEC
GCCSTMVector *transVector = new GCCSTMVector();
#endif
#ifdef COARSEVEC
CoarseTransVector *transVector = new CoarseTransVector();
#endif

// Input: 1- Array of threads that will execute a fucntion.
//        2- A function pointer to that function.
void threadRunner(std::thread *threads, void function(int threadNum))
{
	// Start our threads.
	for (size_t i = 0; i < THREAD_COUNT; i++)
		threads[i] = std::thread(function, i);

	// Wait for all threads to complete.
	for (size_t i = 0; i < THREAD_COUNT; i++)
		threads[i].join();

	return;
}

// Goes through the vector of transactions and executes them.
void executeTransactions(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// Each thread is allocated an interval to work on
	int start = transactions->size() / THREAD_COUNT * threadNum;
	int end = transactions->size() / THREAD_COUNT * (threadNum + 1);

	for (int i = start; i < end; i++)
	{
		Desc *desc = transactions->at(i);
#ifndef BOOSTEDVEC
		transVector->executeTransaction(desc);
#else
		if (!transVector->executeTransaction(desc))
		{
			abortCount.fetch_add(1);
		}
#endif
	}
}

void executeRangedTransactions(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	Desc *desc = transactions->at(threadNum);
#ifndef BOOSTEDVEC
	transVector->executeTransaction(desc);
#else
	if (!transVector->executeTransaction(desc))
	{
		abortCount.fetch_add(1);
	}
#endif
}

// Prepushes a bunch of objects into the vector
void preinsert(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	int opsPerThread = NUM_TRANSACTIONS / THREAD_COUNT;

	// A list of operations for the current thread.
	Operation *pushOps = new Operation[opsPerThread];

	// For each operation.
	for (int j = 0; j < opsPerThread; j++)
	{
		// All operations are pushes.
		pushOps[j].type = Operation::OpType::pushBack;

		// Push random values into the vector.
		VAL val = UNSET;
		// Ensure we never get an UNSET.
		while (val == UNSET)
		{
			val = rand() % std::numeric_limits<VAL>::max();
		}
		pushOps[j].val = val;
	}

	// Create a transaction containing the these operations.
	Desc *pushDesc = new Desc(opsPerThread, pushOps);

// Execute the transaction.
#ifndef BOOSTEDVEC
	transVector->executeTransaction(pushDesc);
	if (pushDesc->status.load() != Desc::TxStatus::committed)
	{
		printf("Preinsert failed.\n");
		return;
	}
#else
	if (!transVector->executeTransaction(pushDesc))
	{
		printf("Preinsert failed.\n");
		return;
	}
#endif
	return;
}

size_t countAborts(std::vector<Desc *> *transactions)
{
#ifdef BOOSTEDVEC
	return abortCount.load();
#else
	size_t retVal = 0;
	for (size_t i = 0; i < transactions->size(); i++)
	{
		Desc *desc = transactions->at(i);

		if (desc->status.load() == Desc::TxStatus::aborted)
			retVal++;
	}
	return retVal;
#endif
}
