// FIRST TESTCASE - RANDOM READS (predicateFind)
// This test case inserts a bunch of random elements and then reads then reads the entire
// vector and counts for even numbers.

#include "main.hpp"


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

void predicateFind(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// Execute the transactions
	int temp = counter++;
	while (temp < (NUM_TRANSACTIONS * THREAD_COUNT))
	{
		Desc *desc = transactions.at(temp);		
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
		
		// Check for predicate matches. If it's even, then record
		size_t matchCount = 0;
		for (size_t i = 0; i < desc->size; i++)
			if (desc->ops[i].ret % 2 == 0)
				matchCount++;

		// Add to the global total and increment temp to get the next desc
		totalMatches.fetch_add(matchCount);
		temp = counter++;
	}

	return;
}

// Insert random elements into the vector and count the number of elements that satisfy the predicate.
void randomReads()
{
	// Ensure we start with no matches.
	totalMatches.store(0);

	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	threadRunner(threads, predicatePreinsert);
	printf("Completed preinsertion!\n\n");

	// Prepare read transactions for each thread.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		// Each thread will have these many transactions
		for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
		{
			Operation *ops = new Operation[TRANSACTION_SIZE];
			
			// Each transaction will be of this size and only made of reads
			for (size_t k = 0; k < TRANSACTION_SIZE; k++)
			{
				// Read all elements, split among threads.
				ops[j].type = Operation::OpType::read;
				ops[j].index = rand() % NUM_TRANSACTIONS;
			}

			Desc *desc = new Desc(TRANSACTION_SIZE, ops);
			transactions.push_back(desc);
		}
	}

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	// Run the threads.
	threadRunner(threads, predicateFind);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

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

	// Initialize the atomic counter
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

	randomReads();

	// Report allocator usage.
	allocatorReport();

	return 0;
}