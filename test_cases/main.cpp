// This file includes methods that will be shared between all test cases

#include "main.hpp"

// All global variables are initialized here
// Preallocate the random number generator.
RandomNumberPool *numPool = new RandomNumberPool(THREAD_COUNT, NUM_TRANSACTIONS * (2 + 3 * TRANSACTION_SIZE));
std::vector<Desc *> *transactions = new std::vector<Desc *>();
#ifdef SEGMENTVEC
	TransactionalVector *transVector = new TransactionalVector();
#endif
#ifdef COMPACTVEC
	transVector = new CompactVector();
#endif
#ifdef COARSEVEC
	transVector = new CoarseTransVector();
#endif
#ifdef STMVEC
	transVector = new GCCSTMVector();
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
		transVector->executeTransaction(desc);

		if (desc->status.load() != Desc::TxStatus::committed)
		{
			continue;
		}
	}
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
	for (size_t j = 0; j < opsPerThread; j++)
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
