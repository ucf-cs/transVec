// FIRST TESTCASE - RANDOM WRITES (predicateFind)
// Preallocate a bunch of nodes and then preform a ranged series of writes
// See transaction.cpp for more detail on "write"
#include "main.hpp"


void threadRunner(std::thread *threads, void function(int threadNum))
{
	// Start our threads.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i] = std::thread(function, i);
	}

	// Wait for all threads to complete.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i].join();
	}
	return;
}

void predicatePreinsert(int threadNum)
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
		// TODO: This assumes UNSET is always the max value.
		insertOps[j].val = numPool->getNum(threadNum) % UNSET;
	}

	// Create a transaction containing the these operations.
	Desc *insertDesc = new Desc(NUM_TRANSACTIONS, insertOps);

	// Execute the transaction.
	transVector->executeTransaction(insertDesc);
	
	return;
}

void writeThread(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	int temp = counter++;

	// For each transaction.
	while (temp < (NUM_TRANSACTIONS * THREAD_COUNT))
	{
		// Execute the transaction.
		transVector->executeTransaction(transactions.at(temp));
		temp = counter++;
		printf("%d\n", temp);
	}
}


// Insert random elements into the vector and count the number of elements that satisfy the predicate.
void writeTest()
{
	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	threadRunner(threads, predicatePreinsert);
	printf("Completed preinsertion!\n\n");

	// Prepare write transactions for each thread.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		// Prepare to read the entire vector.
		for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
		{
			Operation *ops = new Operation[TRANSACTION_SIZE];

			for (size_t k = 0; k < TRANSACTION_SIZE; k++)
			{
				// All operations are writes.
				ops[k].type  = Operation::OpType::write;
				ops[k].val   = 0;
				ops[k].index = rand() % NUM_TRANSACTIONS;
			}

			Desc *desc = new Desc(NUM_TRANSACTIONS, ops);
			transactions.push_back(desc);
		}
	}

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	// Run the threads.
	threadRunner(threads, writeThread);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

	//transVector->printContents();

	std::cout << "" << THREAD_COUNT << " threads and " << NUM_TRANSACTIONS << " locations per thread" << std::endl;
	std::cout << total.count() << " milliseconds" << std::endl;

	printf("Total: %lu matched out of %lu\n", totalMatches.load(), (size_t)THREAD_COUNT * NUM_TRANSACTIONS);
}


int main(void)
{
	// Seed the random number generator.
	srand(time(NULL));

	// Pre-fill the allocators.
	allocatorInit();

	// Initialize atomic counter
	counter = 0;

	// Preallocate the random number generator.
	printf("Generating random numbers.\n");
	numPool = new RandomNumberPool(THREAD_COUNT, NUM_TRANSACTIONS * (2 + 3 * TRANSACTION_SIZE));

	// Reserve the transaction vector, for minor performance gains.
	transactions.reserve(THREAD_COUNT);

#ifdef SEGMENTVEC
	transVector = new TransactionalVector();
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

	writeTest();

	// Report allocator usage.
	allocatorReport();

	return 0;
}