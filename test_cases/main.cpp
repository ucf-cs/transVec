// This file includes methods that will be shared between all test cases

#include "main.hpp"

// Input: 1- Array of threads that will execute a fucntion.
//        2- A function pointer to that function. The fn ptr takes 4 inputs
//        The last three inputs are the inputs to the fn ptr.
void threadRunner(std::thread *threads,
				  void function(int threadNum,
								TransactionalVector *transVector,
								std::vector<Desc *> *transactions,
								RandomNumberPool *numPool),
				  TransactionalVector *transVector,
				  std::vector<Desc *> *transactions,
				  RandomNumberPool *numPool)
{
	// Start our threads.
	for (size_t i = 0; i < THREAD_COUNT; i++)
		threads[i] = std::thread(function, i, transVector, transactions, numPool);

	// Wait for all threads to complete.
	for (size_t i = 0; i < THREAD_COUNT; i++)
		threads[i].join();

	return;
}

// Goes through the vector of transactions and executes them.
void executeTransactions(int threadNum,
						 TransactionalVector *transVector,
						 std::vector<Desc *> *transactions,
						 RandomNumberPool *numPool)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// Each thread is allocated an interval to work on
	int start = transactions->size() / THREAD_COUNT * threadNum;
	int end = transactions->size() / THREAD_COUNT * (threadNum + 1);

	for (int i = start; i < end; i++)
	{
		Desc *desc = transactions->at(i);
		transVector->executeTransaction(desc);

		if (desc->status.load() != Desc::TxStatus::committed)
		{
			continue;
		}
	}
}

// Prepushes a bunch of objects into the vector
void preinsert(int threadNum,
			   TransactionalVector *transVector,
			   std::vector<Desc *> *transactions,
			   RandomNumberPool *numPool)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	int opsPerThread = NUM_TRANSACTIONS / THREAD_COUNT;

	// A list of operations for the current thread.
	Operation *pushOps = new Operation[opsPerThread];

	// Each thread is allocated an interval to work on
	int start = opsPerThread * threadNum;
	int end = opsPerThread * (threadNum + 1);

	// For each operation.
	for (size_t j = 0; j < opsPerThread; j++)
	{
		// All operations are pushes.
		pushOps[j].type = Operation::OpType::pushBack;

		// Push random values into the vector.
		VAL val = UNSET;
		// Ensure we never get an UNSET.
		while (val == UNSET)
		{
			val = numPool->getNum(threadNum) % std::numeric_limits<VAL>::max();
		}
		pushOps[j].val = val;
	}

	// Create a transaction containing the these operations.
	Desc *pushDesc = new Desc(opsPerThread, pushOps);

	// Execute the transaction.
	transVector->executeTransaction(pushDesc);

	return;
}

int countAborts(std::vector<Desc *> *transactions)
{
	int retVal = 0;
	for (int i = 0; i < transactions->size(); i++)
	{
		Desc *desc = transactions->at(i);

		if (desc->status.load() == Desc::TxStatus::aborted)
			retVal++;
	}

	return retVal;
}
