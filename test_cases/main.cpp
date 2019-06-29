// This file includes methods that will be shared between all test cases

#include "main.hpp"

// Input: 1- Array of threads that will execute a fucntion.
//        2- A function pointer to that function. The fn ptr takes 4 inputs
//        The last three inputs are the inputs to the fn ptr.
void threadRunner(std::thread *threads,
				  void function(int threadNum,
						        TransactionalVector *transVector,
						        std::vector<Desc *> transactions,
						        RandomNumberPool *numPool),
				  TransactionalVector *transVector,
				  std::vector<Desc *> transactions,
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
						 std::vector<Desc *> transactions,
						 RandomNumberPool *numPool)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// Each thread is allocated an interval to work on
	double start = NUM_TRANSACTIONS / THREAD_COUNT * threadNum;
	double end   = NUM_TRANSACTIONS / THREAD_COUNT * (threadNum +1 );

	for (int i = start; i < end; i++)
	{
		Desc *desc = transactions.at(i);	
		transVector->executeTransaction(desc);

		if (desc->status.load() != Desc::TxStatus::committed)
		{
			printf("Error on thread %d. Transaction failed.\n", threadNum);
			return;
		}
		
		// Busy wait until they are ready. Should never happen, but we need to be safe.
		while (desc->returnedValues.load() == false)
		{
			printf("Thread %d had to wait on returned values.\n", threadNum);
			continue;
		}
	}
}

// Preinserts a bunch of objects into the vector
void preinsert(int threadNum,
			   TransactionalVector *transVector,
			   std::vector<Desc *> transactions,
			   RandomNumberPool *numPool)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// A list of operations for the current thread.
	Operation *insertOps = new Operation[NUM_TRANSACTIONS];

	// For each operation.
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		// All operations are pushes.
		insertOps[j].type = Operation::OpType::pushBack;

		// Push random values into the vector.
		insertOps[j].val = numPool->getNum(threadNum) % UNSET;
	}

	// Create a transaction containing the these operations.
	Desc *insertDesc = new Desc(NUM_TRANSACTIONS, insertOps);

	// Execute the transaction.
	transVector->executeTransaction(insertDesc);

	return;
}
